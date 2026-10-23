#include <QImage>
#include <QTest>

#include <memory>

#include "testsuites.h"
#include "utils/coloradjustments.h"
#include "utils/imagelib.h"

// CPU image operations of ImageLib (utils/imagelib.cpp). The SSE2 build runs
// their non-AVX2 paths, so these tests also run under Intel SDE (-mrm) there.
//
// Every float-to-8-bit conversion must round exactly once. A flat image keeps
// its level through any resampling whose weights sum to one, and a neutral
// colour adjustment is the identity, so any rounding bias shows up as a level
// change. The widths cover the 8-pixel AVX2 loops and their scalar tails.

namespace {

// Wider than two 8-pixel AVX2 blocks, with a remainder for the scalar tail.
constexpr int kOddWidth = 37;
constexpr int kOddHeight = 23;
// Upscale and downscale targets for kOddWidth x kOddHeight.
constexpr QSize kUpscaledSize(83, 51);
constexpr QSize kDownscaledSize(17, 11);
// A one-pixel-wide column and its vertical upscale (the sharpen pass of
// scaled_Smart then has no right neighbour at x = 0).
constexpr QSize kColumnSize(1, 9);
constexpr QSize kColumnUpscaledSize(1, 21);

constexpr int kMaxLevel = 255;
constexpr int kOpaque = 255;

// Odd channel levels: a +0.5 bias before round-to-nearest moves an exact odd
// level up by one (ties to even), so the bias cannot hide behind an even one.
constexpr int kFlatRed = 101;
constexpr int kFlatGreen = 37;
constexpr int kFlatBlue = 201;

enum class Scaler { Smart, Mks2021 };

QImage flatImage(QSize size) {
  QImage image(size, QImage::Format_ARGB32);
  image.fill(qRgba(kFlatRed, kFlatGreen, kFlatBlue, kOpaque));
  return image;
}

// Every 8-bit level in every channel at least once.
QImage levelRamp() {
  QImage image(kOddWidth, kOddHeight, QImage::Format_ARGB32);
  int level = 0;
  for (int y = 0; y < image.height(); ++y) {
    for (int x = 0; x < image.width(); ++x) {
      const int red = level % (kMaxLevel + 1);
      const int green = (level * 3) % (kMaxLevel + 1);
      const int blue = (kMaxLevel - level % (kMaxLevel + 1));
      image.setPixel(x, y, qRgba(red, green, blue, kOpaque));
      ++level;
    }
  }
  return image;
}

QImage scaledWith(Scaler scaler, const QImage &source, QSize target) {
  auto shared = std::make_shared<const QImage>(source);
  const QImage result = (scaler == Scaler::Smart)
                            ? ImageLib::scaled_Smart(shared, target)
                            : ImageLib::scaled_MKS2021(shared, target);
  return result.convertToFormat(QImage::Format_ARGB32);
}

// Index of the first pixel that differs from expected, or -1.
int firstMismatch(const QImage &image, QRgb expected) {
  for (int y = 0; y < image.height(); ++y)
    for (int x = 0; x < image.width(); ++x)
      if (image.pixel(x, y) != expected)
        return y * image.width() + x;
  return -1;
}
} // namespace

Q_DECLARE_METATYPE(Scaler)

class ImageLibTests : public QObject {
  Q_OBJECT

private slots:
  void scalersKeepFlatLevels_data() {
    QTest::addColumn<Scaler>("scaler");
    QTest::addColumn<QSize>("target");

    QTest::newRow("smart upscale") << Scaler::Smart << kUpscaledSize;
    QTest::newRow("smart downscale") << Scaler::Smart << kDownscaledSize;
    QTest::newRow("mks2021 upscale") << Scaler::Mks2021 << kUpscaledSize;
    QTest::newRow("mks2021 downscale") << Scaler::Mks2021 << kDownscaledSize;
  }

  void scalersKeepFlatLevels() {
    QFETCH(Scaler, scaler);
    QFETCH(QSize, target);

    const QImage result = scaledWith(scaler, flatImage(QSize(kOddWidth, kOddHeight)), target);
    QCOMPARE(result.size(), target);
    QCOMPARE(firstMismatch(result, qRgba(kFlatRed, kFlatGreen, kFlatBlue, kOpaque)), -1);
  }

  void smartUpscalesOnePixelWideColumn() {
    const QImage result = scaledWith(Scaler::Smart, flatImage(kColumnSize), kColumnUpscaledSize);
    QCOMPARE(result.size(), kColumnUpscaledSize);
    QCOMPARE(firstMismatch(result, qRgba(kFlatRed, kFlatGreen, kFlatBlue, kOpaque)), -1);
  }

  void neutralColorAdjustmentKeepsPixels() {
    const QImage source = levelRamp();
    const QImage result = ImageLib::applyColorAdjustments(
        std::make_shared<const QImage>(source), ColorAdjustments{});
    QCOMPARE(result.size(), source.size());
    for (int y = 0; y < source.height(); ++y)
      for (int x = 0; x < source.width(); ++x)
        QCOMPARE(result.pixel(x, y), source.pixel(x, y));
  }

  void smartEnhancePreservesMonochromeAndFlatImages() {
    // 1. Flat grey image should remain unchanged
    const QImage flat = flatImage(QSize(64, 64));
    ColorAdjustments adj;
    adj.smartEnhance = 1.0f;
    const QImage flatResult = ImageLib::applyColorAdjustments(
        std::make_shared<const QImage>(flat), adj);
    QCOMPARE(flatResult.size(), flat.size());
    for (int y = 0; y < flat.height(); ++y) {
      for (int x = 0; x < flat.width(); ++x) {
        QCOMPARE(qRed(flatResult.pixel(x, y)), kFlatRed);
        QCOMPARE(qGreen(flatResult.pixel(x, y)), kFlatGreen);
        QCOMPARE(qBlue(flatResult.pixel(x, y)), kFlatBlue);
      }
    }

    // 2. Pure red image with near-zero green and blue should not blow up into cyan
    QImage redImage(32, 32, QImage::Format_ARGB32);
    redImage.fill(qRgb(200, 0, 0));
    const QImage redResult = ImageLib::applyColorAdjustments(
        std::make_shared<const QImage>(redImage), adj);
    QCOMPARE(qGreen(redResult.pixel(0, 0)), 0);
    QCOMPARE(qBlue(redResult.pixel(0, 0)), 0);
  }

  void vibranceLiftsMutedPixelsAndKeepsGreyAndAlpha() {
    constexpr int kMutedRed = 153;
    constexpr int kMutedOther = 128;
    constexpr int kAlpha = 200;
    QImage source(kOddWidth, kOddHeight, QImage::Format_ARGB32);
    source.fill(qRgba(kMutedRed, kMutedOther, kMutedOther, kAlpha));
    source.setPixel(0, 0, qRgba(kMutedOther, kMutedOther, kMutedOther, kAlpha));
    ColorAdjustments adj;
    adj.vibrance = 1.0f;
    // Vibrance alone, then together with a matrix adjustment: both run the
    // same fused pass.
    for (const float contrast : {ColorAdjustments::kNeutralContrast, 1.1f}) {
      adj.contrast = contrast;
      const QImage result = ImageLib::applyColorAdjustments(
          std::make_shared<const QImage>(source), adj);
      QCOMPARE(result.size(), source.size());
      QCOMPARE(qAlpha(result.pixel(1, 1)), kAlpha);
      QVERIFY(qRed(result.pixel(1, 1)) > qRed(source.pixel(1, 1)));
      QVERIFY(qGreen(result.pixel(1, 1)) < qGreen(source.pixel(1, 1)));
      QCOMPARE(qRed(result.pixel(0, 0)), qGreen(result.pixel(0, 0)));
      QCOMPARE(qGreen(result.pixel(0, 0)), qBlue(result.pixel(0, 0)));
    }
  }

  void smartEnhancePreservesMonochromaticImagesWithNoise() {
    constexpr int kWidth = 64;
    constexpr int kHeight = 64;
    ColorAdjustments adj;
    adj.smartEnhance = 1.0f;

    // Test each primary channel as the dominant color (red, green, blue)
    // against dark regions containing small non-zero noise in secondary channels.
    for (int dominantChannel = 0; dominantChannel < 3; ++dominantChannel) {
      QImage image(kWidth, kHeight, QImage::Format_ARGB32);

      for (int y = 0; y < kHeight; ++y) {
        for (int x = 0; x < kWidth; ++x) {
          if (x < kWidth / 2) {
            const int noise = 5 + (x + y) % 14;
            image.setPixel(x, y, qRgba(noise, noise, noise, kOpaque));
          } else {
            const int dominantVal = 200 + (x % 21);
            const int r = (dominantChannel == 0) ? dominantVal : 0;
            const int g = (dominantChannel == 1) ? dominantVal : 0;
            const int b = (dominantChannel == 2) ? dominantVal : 0;
            image.setPixel(x, y, qRgba(r, g, b, kOpaque));
          }
        }
      }

      const QImage result = ImageLib::applyColorAdjustments(
          std::make_shared<const QImage>(image), adj);
      QCOMPARE(result.size(), image.size());

      // Dark region: subordinate channels must remain bounded noise floor (<= 30),
      // preventing blowout into complementary color casts (cyan, magenta, yellow).
      for (int y = 0; y < kHeight; ++y) {
        for (int x = 0; x < kWidth / 2; ++x) {
          const QRgb pixel = result.pixel(x, y);
          if (dominantChannel != 0)
            QVERIFY2(qRed(pixel) <= 30, "Red channel blown up in dark area");
          if (dominantChannel != 1)
            QVERIFY2(qGreen(pixel) <= 30, "Green channel blown up in dark area");
          if (dominantChannel != 2)
            QVERIFY2(qBlue(pixel) <= 30, "Blue channel blown up in dark area");
        }
      }

      // Saturated region: dominant channel remains strong, subordinate channels near zero.
      for (int y = 0; y < kHeight; ++y) {
        for (int x = kWidth / 2; x < kWidth; ++x) {
          const QRgb pixel = result.pixel(x, y);
          const int dom = (dominantChannel == 0) ? qRed(pixel)
                        : (dominantChannel == 1) ? qGreen(pixel) : qBlue(pixel);
          const int sub1 = (dominantChannel == 0) ? qGreen(pixel) : qRed(pixel);
          const int sub2 = (dominantChannel == 2) ? qGreen(pixel) : qBlue(pixel);
          QVERIFY(dom >= 190);
          QVERIFY(sub1 <= 5);
          QVERIFY(sub2 <= 5);
        }
      }
    }
  }

  void smartEnhanceUsesProvidedWhiteBalanceFactors() {
    constexpr int kWidth = 64;
    constexpr int kHeight = 8;
    constexpr int kFirstLevel = 40;
    constexpr int kLevelStep = 2;
    // Low-contrast ramp: the measured stretch factors are clearly not neutral.
    QImage source(kWidth, kHeight, QImage::Format_ARGB32);
    for (int y = 0; y < kHeight; ++y)
      for (int x = 0; x < kWidth; ++x) {
        const int level = kFirstLevel + x * kLevelStep;
        source.setPixel(x, y, qRgba(level, level, level, kOpaque));
      }
    ColorAdjustments adj;
    adj.smartEnhance = 1.0f;
    const auto sourcePtr = std::make_shared<const QImage>(source);

    const WhiteBalanceFactors measured = calculateWhiteBalanceFactors(&source);
    QVERIFY(measured.scale[0] > 1.5f);

    const QImage implicitResult = ImageLib::applyColorAdjustmentsForBatch(sourcePtr, adj);
    const QImage explicitResult =
        ImageLib::applyColorAdjustmentsForBatch(sourcePtr, adj, &measured);
    const WhiteBalanceFactors neutral;
    const QImage neutralResult =
        ImageLib::applyColorAdjustmentsForBatch(sourcePtr, adj, &neutral);

    // Passing the factors the function would measure itself changes nothing...
    QCOMPARE(explicitResult, implicitResult);
    // ...while other factors are really used.
    QVERIFY(neutralResult != implicitResult);

    // The factors of the full-size image can be reused on a downscaled copy.
    const QImage small = source.scaled(kWidth / 2, kHeight / 2);
    const QImage smallResult = ImageLib::applyColorAdjustmentsForBatch(
        std::make_shared<const QImage>(small), adj, &measured);
    QCOMPARE(smallResult.size(), small.size());
    QVERIFY(qRed(smallResult.pixel(0, 0)) < qRed(small.pixel(0, 0)));
  }
};

int runImageLibTests(int argc, char **argv) {
  ImageLibTests tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "tst_imagelib.moc"
