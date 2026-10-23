#include <QRegion>
#include <algorithm>
#include <QTest>
#include <cmath>
#include <vector>

#include "gui/quick/render/imagefiltermode.h"
#include "gui/quick/render/imagerenderitem.h"
#include "gui/quick/render/resamplegrid.h"
#include "testsuites.h"
#include "utils/coloradjustments.h"

namespace {
// Matrix entries are products of a few floats.
constexpr float kMatrixTolerance = 1e-4f;
constexpr float kDoubleExposure = 1.0f;
constexpr float kExposureFactor = 2.0f;
constexpr float kHighContrast = 2.0f;
constexpr float kBrightnessShift = 0.25f;
constexpr float kContrastPivot = 0.5f;
constexpr float kNoSaturation = 0.0f;
// A third of a turn around the grey axis maps blue -> red -> green -> blue.
constexpr float kThirdTurnDegrees = 120.0f;
constexpr float kLuma[3] = {0.2126f, 0.7152f, 0.0722f};

constexpr int kTestImageDimension = 64;
constexpr int kFirstChannelBase = 40;
constexpr int kFirstChannelStep = 2;
constexpr int kSecondChannelBase = 10;
constexpr int kSecondChannelStep = 3;
constexpr int kBlueBase = 50;
constexpr int kSecondImageRedOffset = 10;
constexpr int kSecondImageGreenOffset = 100;
constexpr float kSmartEnhanceHalf = 0.5f;
constexpr float kSmartEnhanceHigh = 0.8f;
constexpr float kSmartEnhanceOff = 0.0f;
constexpr float kIdentityScale = 1.0f;
constexpr float kIdentityBias = 0.0f;

QImage makeDynamicRangeImage(int rOffset, int gOffset) {
  QImage img(kTestImageDimension, kTestImageDimension, QImage::Format_ARGB32_Premultiplied);
  for (int y = 0; y < kTestImageDimension; ++y) {
    for (int x = 0; x < kTestImageDimension; ++x) {
      const int r = std::clamp(rOffset + x * kFirstChannelStep, 0, 255);
      const int g = std::clamp(gOffset + y * kSecondChannelStep, 0, 255);
      const int b = std::clamp(kBlueBase + (x + y), 0, 255);
      img.setPixel(x, y, qRgb(r, g, b));
    }
  }
  return img;
}

bool near(float actual, float expected) {
  return std::abs(actual - expected) <= kMatrixTolerance;
}

bool matrixEquals(const ColorMatrix &matrix, const float (&expected)[3][3],
                  float expectedOffset) {
  for (int row = 0; row < 3; ++row) {
    for (int column = 0; column < 3; ++column) {
      if (!near(matrix.m[row][column], expected[row][column]))
        return false;
    }
  }
  return near(matrix.offset, expectedOffset);
}
} // namespace

class ImageFilterTests : public QObject {
  Q_OBJECT

private slots:
  void neutralAdjustmentsAreIdentity() {
    const ColorAdjustments neutral;
    QVERIFY(!neutral.hasAdjustments());
    const float identity[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    QVERIFY(matrixEquals(colorAdjustmentMatrix(neutral), identity, 0.0f));
  }

  void exposureScalesAllChannels() {
    ColorAdjustments adjustments;
    adjustments.exposure = kDoubleExposure;
    const float f = kExposureFactor;
    const float expected[3][3] = {{f, 0, 0}, {0, f, 0}, {0, 0, f}};
    QVERIFY(matrixEquals(colorAdjustmentMatrix(adjustments), expected, 0.0f));
  }

  void contrastPivotsAroundMidGrey() {
    ColorAdjustments adjustments;
    adjustments.contrast = kHighContrast;
    adjustments.brightness = kBrightnessShift;
    const float c = kHighContrast;
    const float expected[3][3] = {{c, 0, 0}, {0, c, 0}, {0, 0, c}};
    const float offset = kBrightnessShift * c + kContrastPivot * (1.0f - c);
    QVERIFY(matrixEquals(colorAdjustmentMatrix(adjustments), expected, offset));
  }

  void zeroSaturationIsLuma() {
    ColorAdjustments adjustments;
    adjustments.saturation = kNoSaturation;
    const float expected[3][3] = {{kLuma[0], kLuma[1], kLuma[2]},
                                  {kLuma[0], kLuma[1], kLuma[2]},
                                  {kLuma[0], kLuma[1], kLuma[2]}};
    QVERIFY(matrixEquals(colorAdjustmentMatrix(adjustments), expected, 0.0f));
  }

  void thirdTurnHueRotatesChannels() {
    ColorAdjustments adjustments;
    adjustments.hue = kThirdTurnDegrees;
    // out.r = in.b, out.g = in.r, out.b = in.g
    const float expected[3][3] = {{0, 0, 1}, {1, 0, 0}, {0, 1, 0}};
    QVERIFY(matrixEquals(colorAdjustmentMatrix(adjustments), expected, 0.0f));
  }

  void vibranceLiftsMutedColoursAndSparesSaturatedOnes() {
    constexpr float kFullVibrance = 1.0f;
    float muted[3] = {0.6f, 0.5f, 0.5f};
    float saturated[3] = {1.0f, 0.0f, 0.0f};
    float grey[3] = {0.5f, 0.5f, 0.5f};
    applyColorAdjustmentVibrance(muted, kFullVibrance);
    applyColorAdjustmentVibrance(saturated, kFullVibrance);
    applyColorAdjustmentVibrance(grey, kFullVibrance);
    // Positive vibrance pivots on the peak channel (as Photoshop does): the
    // weaker channels of the muted red are pushed down while the peak stays;
    // fully saturated and grey pixels stay as they are.
    QVERIFY(near(muted[0], 0.6f));
    QVERIFY(muted[1] < 0.5f - kMatrixTolerance);
    QVERIFY(muted[2] < 0.5f - kMatrixTolerance);
    QVERIFY(near(saturated[0], 1.0f) && near(saturated[1], 0.0f) && near(saturated[2], 0.0f));
    QVERIFY(near(grey[0], 0.5f) && near(grey[1], 0.5f) && near(grey[2], 0.5f));

    float mutedAgain[3] = {0.6f, 0.5f, 0.5f};
    applyColorAdjustmentVibrance(mutedAgain, -kFullVibrance);
    QVERIFY(mutedAgain[0] < 0.6f - kMatrixTolerance);
  }

  void fullVibranceStaysCloseToPhotoshopSaturationLift() {
    // Photoshop's Vibrance +100 lifts HSV saturation by roughly 1.3x on
    // mid-saturated colours (skin, denim), never by 2x or more.
    constexpr float kMaxExpectedLift = 1.45f;
    constexpr float kMinExpectedLift = 1.15f;
    float skin[3] = {0.80f, 0.55f, 0.40f};
    const float satBefore = (skin[0] - skin[2]) / skin[0];
    applyColorAdjustmentVibrance(skin, 1.0f);
    const float satAfter = (skin[0] - skin[2]) / skin[0];
    QVERIFY(satAfter / satBefore < kMaxExpectedLift);
    QVERIFY(satAfter / satBefore > kMinExpectedLift);
    QVERIFY(near(skin[0], 0.80f));
  }

  void negativeVibrancePreservesChromaMonotonicity() {
    constexpr float kMinusFullVibrance = -1.0f;
    float previousChromaOut = 0.0f;
    for (int step = 0; step <= 20; ++step) {
      const float chromaIn = step / 20.0f;
      float rgb[3] = {0.5f + (1.0f - kColorLumaR) * chromaIn,
                      0.5f - kColorLumaR * chromaIn,
                      0.5f - kColorLumaR * chromaIn};
      applyColorAdjustmentVibrance(rgb, kMinusFullVibrance);
      const float peak = std::max({rgb[0], rgb[1], rgb[2]});
      const float low = std::min({rgb[0], rgb[1], rgb[2]});
      const float chromaOut = peak - low;
      QVERIFY(chromaOut >= previousChromaOut - 1e-5f);
      previousChromaOut = chromaOut;
    }
  }

  void vibranceIsNotPartOfTheMatrix() {
    ColorAdjustments adjustments;
    adjustments.vibrance = 1.0f;
    QVERIFY(adjustments.hasVibrance());
    QVERIFY(adjustments.hasAdjustments());
    QVERIFY(!adjustments.hasMatrixAdjustments());
    const float identity[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    QVERIFY(matrixEquals(colorAdjustmentMatrix(adjustments), identity, 0.0f));
  }

  void scalingFiltersMapToRenderModes_data() {
    QTest::addColumn<int>("filter");
    QTest::addColumn<RenderEnums::TextureSampling>("sampling");
    QTest::addColumn<RenderEnums::Sharpening>("sharpening");
    QTest::addColumn<RenderEnums::Resampling>("resampling");
    using RenderEnums::Resampling;
    using RenderEnums::Sharpening;
    using RenderEnums::TextureSampling;
    QTest::newRow("nearest") << int(QI_FILTER_NEAREST)
                             << TextureSampling::Nearest << Sharpening::None
                             << Resampling::None;
    QTest::newRow("bilinear") << int(QI_FILTER_BILINEAR)
                              << TextureSampling::Trilinear << Sharpening::None
                              << Resampling::None;
    // The CPU-only filters, should one reach the renderer, show as their GPU
    // counterparts.
    QTest::newRow("smart") << int(QI_FILTER_SMART)
                           << TextureSampling::Trilinear << Sharpening::Smart
                           << Resampling::None;
    QTest::newRow("cas") << int(QI_FILTER_CAS) << TextureSampling::Trilinear
                         << Sharpening::Cas << Resampling::None;
    QTest::newRow("smart gpu") << int(QI_FILTER_SMART_GPU)
                               << TextureSampling::Trilinear
                               << Sharpening::Smart << Resampling::None;
    QTest::newRow("mks2021") << int(QI_FILTER_MKS2021)
                             << TextureSampling::Trilinear << Sharpening::None
                             << Resampling::Mks2021;
    QTest::newRow("mks2021 gpu")
        << int(QI_FILTER_MKS2021_GPU) << TextureSampling::Trilinear
        << Sharpening::None << Resampling::Mks2021;
  }

  void scalingFiltersMapToRenderModes() {
    QFETCH(int, filter);
    QFETCH(RenderEnums::TextureSampling, sampling);
    QFETCH(RenderEnums::Sharpening, sharpening);
    QFETCH(RenderEnums::Resampling, resampling);
    const ImageFilterMode mode =
        imageFilterModeFor(static_cast<ScalingFilter>(filter));
    QCOMPARE(mode.sampling, sampling);
    QCOMPARE(mode.sharpening, sharpening);
    QCOMPARE(mode.resampling, resampling);
  }

  // Same taps as buildMksAxisTaps() in utils/imagelib.cpp: centred sampling,
  // a 4.5 px kernel half width widened by the reduction factor.
  void mks2021AxisMatchesCpuTaps() {
    const Mks2021Axis upscale{100, 300};
    QCOMPARE(upscale.filterScale(), 1.0);
    QCOMPARE(upscale.support(), Mks2021Axis::kSupport);
    QCOMPARE(upscale.centre(0), (0.5 / 3.0) - 0.5);
    QCOMPARE(upscale.firstTap(0), -5);
    QCOMPARE(upscale.lastTap(0), 5);

    const Mks2021Axis downscale{400, 100};
    QCOMPARE(downscale.ratio(), 4.0);
    QCOMPARE(downscale.support(), 4.0 * Mks2021Axis::kSupport);
    QCOMPARE(downscale.centre(10), 41.5);
    QCOMPARE(downscale.firstTap(10), 23);
    QCOMPARE(downscale.lastTap(10), 60);
  }

  // The weights of every output are the CPU's normalized taps: they sum to 1,
  // are symmetric for a centred output and follow the kernel's lobes.
  void mks2021WeightsAreNormalizedKernelTaps() {
    constexpr double kTolerance = 1e-12;
    const Mks2021Axis oneToOne{100, 100};
    const std::vector<double> identity = oneToOne.weights(50);
    QCOMPARE(int(identity.size()), oneToOne.lastTap(50) - oneToOne.firstTap(50) + 1);
    // At 1:1 the centre tap is k(0) and the neighbours are k(+-1), ...
    double sum = 0.0;
    for (std::size_t i = 0; i < identity.size(); ++i) {
      sum += identity[i];
      QVERIFY(std::abs(identity[i] - identity[identity.size() - 1 - i]) <
              kTolerance);
    }
    QVERIFY(std::abs(sum - 1.0) < kTolerance);

    const Mks2021Axis downscale{400, 100};
    const std::vector<double> wide = downscale.weights(10);
    double wideSum = 0.0;
    for (const double w : wide)
      wideSum += w;
    QVERIFY(std::abs(wideSum - 1.0) < kTolerance);
    // The sharp kernel has negative lobes.
    QVERIFY(*std::min_element(wide.begin(), wide.end()) < 0.0);
    QCOMPARE(Mks2021Axis::kernel(Mks2021Axis::kSupport), 0.0);
  }

  void resampledOutputSizeRoundsLikeTheCpuTarget() {
    QCOMPARE(ResampleGrid::outputSize(QSize(1000, 333), 0.5), QSize(500, 167));
    QCOMPARE(ResampleGrid::outputSize(QSize(3, 3), 0.01), QSize(1, 1));
  }

  // Every visible output belongs to exactly one tile.
  void tilesPartitionTheVisibleOutputs() {
    constexpr int kTextureLimit = 3 * TileGrid::kOverlap + 32;
    const QSize imageSize(1000, 700);
    const QList<ImageTile> tiles = TileGrid::layout(imageSize, kTextureLimit);
    QVERIFY(tiles.size() > 4);
    const QSize targetSize(640, 480);
    const QPoint origin(-37, 11);
    for (const double scale : {0.37, 1.7}) {
      const QSize outputSize = ResampleGrid::outputSize(imageSize, scale);
      QRegion covered;
      qint64 area = 0;
      for (const ImageTile &tile : tiles) {
        const QRect outputs = ResampleGrid::visibleTileOutputs(
            tile, imageSize, outputSize, origin, targetSize);
        if (outputs.isEmpty())
          continue;
        QVERIFY(!covered.intersects(outputs));
        covered += outputs;
        area += qint64(outputs.width()) * outputs.height();
      }
      const QRect expected =
          QRect(QPoint(0, 0), outputSize).intersected(QRect(-origin, targetSize));
      QCOMPARE(covered, QRegion(expected));
      QCOMPARE(area, qint64(expected.width()) * expected.height());
    }
  }

  void smartEnhanceDetection() {
    const ColorAdjustments neutral;
    QVERIFY(!neutral.hasSmartEnhance());
    ColorAdjustments withEnhance;
    withEnhance.smartEnhance = kSmartEnhanceHalf;
    QVERIFY(withEnhance.hasSmartEnhance());
    QVERIFY(withEnhance.hasAdjustments());
  }

  void renderItemCalculatesWhiteBalanceOnDemandAndCaches() {
    const QImage img1 = makeDynamicRangeImage(kFirstChannelBase, kSecondChannelBase);
    ImageRenderItem item;
    item.setImage(std::make_shared<const QImage>(img1));

    // Initially Smart Enhance is neutral: factors must remain identity.
    QVERIFY(!item.imageFilter().colorAdjustments.hasSmartEnhance());
    QCOMPARE(item.imageFilter().wbScale[0], kIdentityScale);
    QCOMPARE(item.imageFilter().wbScale[1], kIdentityScale);
    QCOMPARE(item.imageFilter().wbScale[2], kIdentityScale);
    QCOMPARE(item.imageFilter().wbBias[0], kIdentityBias);
    QCOMPARE(item.imageFilter().wbBias[1], kIdentityBias);
    QCOMPARE(item.imageFilter().wbBias[2], kIdentityBias);

    // Turn Smart Enhance on: factors must be calculated on demand.
    ColorAdjustments adjustments;
    adjustments.smartEnhance = kSmartEnhanceHalf;
    item.setColorAdjustments(adjustments);

    const float initialScaleR = item.imageFilter().wbScale[0];
    const float initialBiasR = item.imageFilter().wbBias[0];
    QVERIFY(initialScaleR != kIdentityScale || initialBiasR != kIdentityBias);

    // Adjusting the slider value should reuse cached factors without recalculating.
    adjustments.smartEnhance = kSmartEnhanceHigh;
    item.setColorAdjustments(adjustments);
    QCOMPARE(item.imageFilter().wbScale[0], initialScaleR);
    QCOMPARE(item.imageFilter().wbBias[0], initialBiasR);

    // An animation frame arrives: factors must stay stable and cached.
    const QImage img2 = makeDynamicRangeImage(kSecondImageRedOffset, kSecondImageGreenOffset);
    item.setImage(std::make_shared<const QImage>(img2),
                  ImageRenderItem::ImageUpdate::AnimationFrame);
    QCOMPARE(item.imageFilter().wbScale[0], initialScaleR);
    QCOMPARE(item.imageFilter().wbBias[0], initialBiasR);

    // A new image arrives: cache must be invalidated and factors recalculated.
    item.setImage(std::make_shared<const QImage>(img2),
                  ImageRenderItem::ImageUpdate::NewImage);
    const float newScaleR = item.imageFilter().wbScale[0];
    const float newBiasR = item.imageFilter().wbBias[0];
    QVERIFY(newScaleR != initialScaleR || newBiasR != initialBiasR);

    // Turning Smart Enhance off restores identity factors.
    adjustments.smartEnhance = kSmartEnhanceOff;
    item.setColorAdjustments(adjustments);
    QCOMPARE(item.imageFilter().wbScale[0], kIdentityScale);
    QCOMPARE(item.imageFilter().wbScale[1], kIdentityScale);
    QCOMPARE(item.imageFilter().wbScale[2], kIdentityScale);
    QCOMPARE(item.imageFilter().wbBias[0], kIdentityBias);
    QCOMPARE(item.imageFilter().wbBias[1], kIdentityBias);
    QCOMPARE(item.imageFilter().wbBias[2], kIdentityBias);
  }
};

int runImageFilterTests(int argc, char **argv) {
  ImageFilterTests tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "tst_imagefilter.moc"
