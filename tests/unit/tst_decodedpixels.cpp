#include <QDeadlineTimer>
#include <QTest>
#include <QThread>

#include <QColorSpace>
#include <QFloat16>

#include <array>
#include <atomic>
#include <latch>
#include <memory>
#include <vector>

#include "sourcecontainers/decodedpixels.h"
#include "testsuites.h"

namespace {
constexpr QSize kImageSize(64, 32);
// Threads asking for the SDR copy at the same time.
constexpr int kConcurrentReaders = 8;
// How long the first conversion waits for other readers to convert as well.
constexpr int kOverlapWindowMs = 100;
// Tone mapping as the settings default to it, a changed white level, and no
// tone mapping.
constexpr HdrToneMapParams kToneMapped{};
constexpr float kChangedTargetWhiteNits = 100.0f;
constexpr HdrToneMapParams kChangedWhite{.targetWhiteNits = kChangedTargetWhiteNits};
constexpr HdrToneMapParams kNotToneMapped{.enabled = false};
// A linear HDR gradient whose brightest pixels are well above SDR white, and
// a crop of it off the origin.
constexpr QSize kGradientSize(96, 48);
constexpr float kGradientPeak = 12.0f;
constexpr QRect kGradientCrop(17, 9, 40, 30);
constexpr int kChannels = 4;
const QString kKeptTextKey = QStringLiteral("Description");
const QString kHdrTextKey = QStringLiteral("HDR_Profile");

std::shared_ptr<const QImage> hdrImage() {
  QImage image(kImageSize, QImage::Format_RGBA16FPx4);
  image.fill(Qt::white);
  return std::make_shared<const QImage>(std::move(image));
}

std::shared_ptr<const QImage> sdrImage() {
  QImage image(kImageSize, QImage::Format_ARGB32);
  image.fill(Qt::darkCyan);
  return std::make_shared<const QImage>(std::move(image));
}

std::shared_ptr<const QImage> hdrGradient() {
  QImage image(kGradientSize, QImage::Format_RGBA16FPx4);
  for (int y = 0; y < image.height(); ++y) {
    auto *line = reinterpret_cast<qfloat16 *>(image.scanLine(y));
    for (int x = 0; x < image.width(); ++x) {
      const float red = kGradientPeak * x / (image.width() - 1);
      const float green = kGradientPeak * y / (image.height() - 1);
      const std::array<float, kChannels> pixel = {red, green, (red + green) / 2,
                                                  1.0f};
      for (int c = 0; c < kChannels; ++c)
        line[x * kChannels + c] = qfloat16(pixel[c]);
    }
  }
  image.setColorSpace(QColorSpace(QColorSpace::SRgbLinear));
  image.setText(kKeptTextKey, kKeptTextKey);
  image.setText(kHdrTextKey, QStringLiteral("scRGB"));
  return std::make_shared<const QImage>(std::move(image));
}

// Counts its calls and keeps the params it was given; converts like the CPU
// fallback (8-bit sRGB).
struct CountingConversion {
  std::shared_ptr<std::atomic<int>> calls = std::make_shared<std::atomic<int>>(0);
  std::shared_ptr<std::vector<HdrToneMapParams>> params =
      std::make_shared<std::vector<HdrToneMapParams>>();

  DecodedPixels::SdrConversion function() const {
    return [calls = calls, params = params](const QImage &hdr,
                                            const HdrToneMapParams &with) {
      ++*calls;
      params->push_back(with);
      return hdr.convertToFormat(QImage::Format_ARGB32);
    };
  }
};
} // namespace

Q_DECLARE_METATYPE(HdrToneMapParams)

class DecodedPixelsTests : public QObject {
  Q_OBJECT

private slots:
  void keepsTheHdrSourceAndConvertsOnFirstUse() {
    CountingConversion conversion;
    const std::shared_ptr<const QImage> hdr = hdrImage();
    DecodedPixels pixels;
    pixels.assign(hdr, true, conversion.function());
    QCOMPARE(conversion.calls->load(), 0);
    QVERIFY(pixels.hasHdrSource());
    QCOMPARE(pixels.decoded(), hdr);
    QCOMPARE(pixels.size(), kImageSize);
    QCOMPARE(conversion.calls->load(), 0);

    const std::shared_ptr<const QImage> sdr = pixels.sdr(kToneMapped);
    QVERIFY(sdr);
    QCOMPARE(sdr->format(), QImage::Format_ARGB32);
    QCOMPARE(pixels.sdr(kToneMapped), sdr);
    QCOMPARE(conversion.calls->load(), 1);
    QVERIFY(conversion.params->front() == kToneMapped);
    // The viewer keeps showing the HDR source.
    QCOMPARE(pixels.decoded(), hdr);
  }

  void anSdrImageIsItsOwnCopy() {
    CountingConversion conversion;
    const std::shared_ptr<const QImage> sdr = sdrImage();
    DecodedPixels pixels;
    pixels.assign(sdr, false, conversion.function());
    QCOMPARE(pixels.sdr(kToneMapped), sdr);
    QCOMPARE(pixels.sdr(kChangedWhite), sdr);
    QCOMPARE(pixels.decoded(), sdr);
    QVERIFY(!pixels.hasHdrSource());
    QCOMPARE(conversion.calls->load(), 0);
  }

  void concurrentReadersConvertOnce() {
    std::atomic<int> calls = 0;
    DecodedPixels pixels;
    pixels.assign(hdrImage(), true, [&calls](const QImage &hdr,
                                             const HdrToneMapParams &) {
      ++calls;
      // The readers are released together; while this conversion runs, the
      // others must wait in sdr() instead of converting too. Holding it open
      // for the overlap window lets them all reach a conversion if they can.
      const QDeadlineTimer window(kOverlapWindowMs);
      while (calls.load() < kConcurrentReaders && !window.hasExpired())
        QThread::yieldCurrentThread();
      return hdr.convertToFormat(QImage::Format_ARGB32);
    });

    std::latch start(kConcurrentReaders);
    std::vector<std::shared_ptr<const QImage>> results(kConcurrentReaders);
    std::vector<std::unique_ptr<QThread>> readers;
    for (int i = 0; i < kConcurrentReaders; ++i) {
      readers.emplace_back(QThread::create([&pixels, &results, &start, i]() {
        start.arrive_and_wait();
        results[i] = pixels.sdr(kToneMapped);
      }));
      readers.back()->start();
    }
    for (const auto &reader : readers)
      reader->wait();

    QCOMPARE(calls.load(), 1);
    for (const auto &result : results)
      QCOMPARE(result, results.front());
    QVERIFY(results.front());
  }

  void aFailedConversionIsReportedOnce() {
    std::atomic<int> calls = 0;
    DecodedPixels pixels;
    pixels.assign(hdrImage(), true, [&calls](const QImage &,
                                             const HdrToneMapParams &) {
      ++calls;
      return QImage();
    });
    QTest::ignoreMessage(QtWarningMsg, "DecodedPixels: the HDR image could not be converted to SDR");
    QVERIFY(!pixels.sdr(kToneMapped));
    QVERIFY(!pixels.sdr(kToneMapped));
    QCOMPARE(calls.load(), 1);
    // The HDR source can still be shown.
    QVERIFY(pixels.decoded());

    // Other params are a new conversion, tried (and reported) again.
    QTest::ignoreMessage(QtWarningMsg, "DecodedPixels: the HDR image could not be converted to SDR");
    QVERIFY(!pixels.sdr(kChangedWhite));
    QCOMPARE(calls.load(), 2);
  }

  // A cached image follows a tone mapping change (audit A-11): the copy built
  // with the old params is replaced, not kept beside the new one.
  void otherParamsRebuildTheSdrCopy() {
    CountingConversion conversion;
    const std::shared_ptr<const QImage> hdr = hdrImage();
    DecodedPixels pixels;
    pixels.assign(hdr, true, conversion.function());

    const std::shared_ptr<const QImage> first = pixels.sdr(kToneMapped);
    QVERIFY(first);
    const std::shared_ptr<const QImage> changed = pixels.sdr(kChangedWhite);
    QVERIFY(changed);
    QVERIFY(changed != first);
    QCOMPARE(pixels.sdr(kChangedWhite), changed);
    QCOMPARE(conversion.calls->load(), 2);
    QVERIFY(conversion.params->back() == kChangedWhite);
    QCOMPARE(pixels.sizeInBytes(), hdr->sizeInBytes() + changed->sizeInBytes());

    QVERIFY(pixels.sdr(kNotToneMapped));
    QCOMPARE(conversion.calls->load(), 3);
    QVERIFY(conversion.params->back() == kNotToneMapped);
  }

  // Upscayl converts only the visible crop (audit A-10); that is exact
  // because every conversion works per pixel.
  void convertingACropGivesTheCropOfTheConvertedImage_data() {
    QTest::addColumn<HdrToneMapParams>("params");
    QTest::newRow("bt2408") << HdrToneMapParams{.op = ToneMapOperator::Bt2408};
    QTest::newRow("reinhard-jodie")
        << HdrToneMapParams{.op = ToneMapOperator::ReinhardJodie};
    QTest::newRow("aces") << HdrToneMapParams{.op = ToneMapOperator::AcesFilmic};
    QTest::newRow("hable") << HdrToneMapParams{.op = ToneMapOperator::Hable};
    QTest::newRow("changed-white") << kChangedWhite;
    QTest::newRow("not-tone-mapped") << kNotToneMapped;
  }

  void convertingACropGivesTheCropOfTheConvertedImage() {
    QFETCH(HdrToneMapParams, params);
    const std::shared_ptr<const QImage> hdr = hdrGradient();
    const QImage whole = HdrToneMapper::toSdr(*hdr, params);
    const QImage crop = HdrToneMapper::toSdr(hdr->copy(kGradientCrop), params);
    QVERIFY(!whole.isNull());
    QCOMPARE(crop, whole.copy(kGradientCrop));
    QCOMPARE(crop.colorSpace(), QColorSpace(QColorSpace::SRgb));
    QCOMPARE(crop.text(kKeptTextKey), kKeptTextKey);
    QVERIFY(crop.text(kHdrTextKey).isEmpty());
  }

  void replacingDropsTheHdrSource() {
    CountingConversion conversion;
    DecodedPixels pixels;
    pixels.assign(hdrImage(), true, conversion.function());
    const std::shared_ptr<const QImage> edited = sdrImage();
    pixels.replace(edited);
    QVERIFY(!pixels.hasHdrSource());
    QCOMPARE(pixels.decoded(), edited);
    QCOMPARE(pixels.sdr(kToneMapped), edited);
    QCOMPARE(conversion.calls->load(), 0);
  }

  void assigningNothingClears() {
    CountingConversion conversion;
    DecodedPixels pixels;
    pixels.assign(sdrImage(), false, conversion.function());
    pixels.assign(nullptr, false, conversion.function());
    QVERIFY(!pixels.sdr(kToneMapped));
    QVERIFY(!pixels.decoded());
    QCOMPARE(pixels.size(), QSize());
    QCOMPARE(pixels.sizeInBytes(), qint64{0});
  }

  // The preload budget counts every kept copy.
  void sizeInBytesCountsTheHdrSourceAndTheBuiltSdrCopy() {
    CountingConversion conversion;
    const std::shared_ptr<const QImage> hdr = hdrImage();
    DecodedPixels pixels;
    pixels.assign(hdr, true, conversion.function());
    QCOMPARE(pixels.sizeInBytes(), hdr->sizeInBytes());

    const std::shared_ptr<const QImage> sdr = pixels.sdr(kToneMapped);
    QVERIFY(sdr);
    QCOMPARE(pixels.sizeInBytes(), hdr->sizeInBytes() + sdr->sizeInBytes());

    const std::shared_ptr<const QImage> replacement = sdrImage();
    pixels.replace(replacement);
    QCOMPARE(pixels.sizeInBytes(), replacement->sizeInBytes());
  }
};

int runDecodedPixelsTests(int argc, char **argv) {
  DecodedPixelsTests tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "tst_decodedpixels.moc"
