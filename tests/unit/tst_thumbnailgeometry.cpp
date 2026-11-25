#include <QSize>
#include <QTest>

#include "components/thumbnailer/thumbnailgeometry.h"
#include "testsuites.h"

namespace {
// The box of a 256 px cell: 4:3.
constexpr QSize kBox(256, 192);

// The size the thumbnailer chose while the box was a square of edge
// `edge`: kept as it was written.
QSize legacySquareScaledSize(QSize original, int edge, Qt::AspectRatioMode mode) {
  if (!original.isValid())
    return original;
  if (mode == Qt::KeepAspectRatioByExpanding) {
    if (original.width() < edge && original.height() < edge)
      return original;
  } else if (original.width() <= edge && original.height() <= edge) {
    return original;
  }
  return original.scaled(edge, edge, mode);
}
} // namespace

class ThumbnailGeometryTests : public QObject {
  Q_OBJECT

private slots:
  void squareSourceFitsTheHeightOfTheBox() {
    // No 256 x 256 texture that the GPU has to shrink to 192 afterwards.
    QCOMPARE(ThumbnailGeometry::noUpscaleScaledSize({1000, 1000}, kBox, Qt::KeepAspectRatio),
             QSize(192, 192));
  }

  void portraitSourcesFitTheHeightOfTheBox() {
    QCOMPARE(ThumbnailGeometry::noUpscaleScaledSize({3000, 4000}, kBox, Qt::KeepAspectRatio),
             QSize(144, 192));
    QCOMPARE(ThumbnailGeometry::noUpscaleScaledSize({1080, 1920}, kBox, Qt::KeepAspectRatio),
             QSize(108, 192));
  }

  void landscapeSourcesFitTheBox() {
    QCOMPARE(ThumbnailGeometry::noUpscaleScaledSize({4000, 3000}, kBox, Qt::KeepAspectRatio),
             QSize(256, 192));
    QCOMPARE(ThumbnailGeometry::noUpscaleScaledSize({2560, 1440}, kBox, Qt::KeepAspectRatio),
             QSize(256, 144));
  }

  void smallSourcesAreNeverEnlarged() {
    QCOMPARE(ThumbnailGeometry::noUpscaleScaledSize({100, 80}, kBox, Qt::KeepAspectRatio),
             QSize(100, 80));
    QCOMPARE(ThumbnailGeometry::noUpscaleScaledSize({256, 192}, kBox, Qt::KeepAspectRatio),
             QSize(256, 192));
    // Smaller than the box in both dimensions: kept when expanding as well.
    QCOMPARE(ThumbnailGeometry::noUpscaleScaledSize({100, 80}, {192, 192},
                                                    Qt::KeepAspectRatioByExpanding),
             QSize(100, 80));
  }

  void invalidSourceIsReturnedAsItIs() {
    QVERIFY(!ThumbnailGeometry::noUpscaleScaledSize({}, kBox, Qt::KeepAspectRatio).isValid());
  }

  void croppedThumbnailIsTheSquareOfTheShorterSide() {
    QCOMPARE(ThumbnailGeometry::cropBox(kBox), QSize(192, 192));
    QCOMPARE(ThumbnailGeometry::cropBox({192, 256}), QSize(192, 192));
    QCOMPARE(ThumbnailGeometry::cropBox({128, 128}), QSize(128, 128));
    QCOMPARE(ThumbnailGeometry::outputBox(kBox, false), kBox);
    QCOMPARE(ThumbnailGeometry::outputBox(kBox, true), QSize(192, 192));
  }

  void croppedImageCoversTheSquare() {
    const QSize square = ThumbnailGeometry::outputBox(kBox, true);
    // 2:1 source: scaled so that the shorter side is the square's edge.
    const QSize scaled = ThumbnailGeometry::noUpscaleScaledSize(
        {2400, 1200}, square, Qt::KeepAspectRatioByExpanding);
    QCOMPARE(scaled, QSize(384, 192));
    const QRect clip = ThumbnailGeometry::cropRect(scaled, square);
    QCOMPARE(clip.size(), square);
    QVERIFY(QRect(QPoint(0, 0), scaled).contains(clip));
    QCOMPARE(clip.center(), QRect(QPoint(0, 0), scaled).center());
  }

  void squareBoxBehavesAsTheSquareSizeDid_data() {
    QTest::addColumn<QSize>("source");
    QTest::addColumn<int>("edge");
    QTest::newRow("large landscape") << QSize(4000, 3000) << 256;
    QTest::newRow("large portrait") << QSize(3000, 4000) << 256;
    QTest::newRow("large square") << QSize(2000, 2000) << 128;
    QTest::newRow("wide") << QSize(6000, 800) << 192;
    QTest::newRow("tall") << QSize(300, 5000) << 192;
    QTest::newRow("fits") << QSize(100, 120) << 256;
    QTest::newRow("one side fits") << QSize(100, 900) << 256;
    QTest::newRow("exactly the box") << QSize(256, 256) << 256;
  }

  // The command line and the batch converter ask for a square box.
  void squareBoxBehavesAsTheSquareSizeDid() {
    QFETCH(QSize, source);
    QFETCH(int, edge);
    for (const Qt::AspectRatioMode mode :
         {Qt::KeepAspectRatio, Qt::KeepAspectRatioByExpanding}) {
      QCOMPARE(ThumbnailGeometry::noUpscaleScaledSize(source, {edge, edge}, mode),
               legacySquareScaledSize(source, edge, mode));
    }
    QCOMPARE(ThumbnailGeometry::outputBox({edge, edge}, true), QSize(edge, edge));
  }

  void exifOrientationTranspositionMatchesRotationCases() {
    for (int o = 0; o <= 3; ++o)
      QVERIFY(!ThumbnailGeometry::isExifOrientationTransposed(o));
    for (int o = 4; o <= 7; ++o)
      QVERIFY(ThumbnailGeometry::isExifOrientationTransposed(o));
    QCOMPARE(ThumbnailGeometry::orientSize({640, 160}, false), QSize(640, 160));
    QCOMPARE(ThumbnailGeometry::orientSize({640, 160}, true), QSize(160, 640));
  }

  void cachePlanTriggersUpgradeAndToleratesOnePixelRounding() {
    // 4:1 panorama, base resolution 256, tile asks for 576x144.
    const QSize source(4000, 1000);
    const auto upgradePlan = ThumbnailGeometry::planCacheDecode(
        QSize(256, 64), source, QSize(576, 144), false, 256);
    QCOMPARE(upgradePlan.desiredOutput, QSize(576, 144));
    QVERIFY(!upgradePlan.cacheSufficient);
    QCOMPARE(upgradePlan.decodeBox, QSize(576, 256));

    // Once upgraded to 576x144, equal or smaller tiles hit cache.
    QVERIFY(ThumbnailGeometry::planCacheDecode(
                QSize(576, 144), source, QSize(576, 144), false, 256)
                .cacheSufficient);
    QVERIFY(ThumbnailGeometry::planCacheDecode(
                QSize(576, 144), source, QSize(384, 96), false, 256)
                .cacheSufficient);

    // Non-round aspect (4000x1013): 576x145 cached satisfies 576x146 within 1 px.
    const QSize dirtySource(4000, 1013);
    const auto dirtyPlan = ThumbnailGeometry::planCacheDecode(
        QSize(576, 145), dirtySource, QSize(576, 146), false, 256);
    QCOMPARE(dirtyPlan.desiredOutput, QSize(576, 146));
    QVERIFY(dirtyPlan.cacheSufficient);
  }

  void cachePlanDoesNotLoopAtCacheCeilingOrNativeSize() {
    const QSize largeSource(8000, 2000);
    const auto missPlan = ThumbnailGeometry::planCacheDecode(
        QSize(), largeSource, QSize(2400, 600), false, 256);
    QCOMPARE(missPlan.decodeBox, QSize(2048, 640));
    const auto cappedHit = ThumbnailGeometry::planCacheDecode(
        QSize(2048, 512), largeSource, QSize(2400, 600), false, 256);
    QVERIFY(cappedHit.cacheSufficient);

    const QSize tinySource(200, 50);
    const auto tinyHit = ThumbnailGeometry::planCacheDecode(
        tinySource, tinySource, QSize(600, 150), false, 256);
    QCOMPARE(tinyHit.desiredOutput, tinySource);
    QVERIFY(tinyHit.cacheSufficient);
  }

  void evenSizeRoundsDownToNearestEven() {
    QCOMPARE(ThumbnailGeometry::evenSize({0, 0}), QSize(0, 0));
    QCOMPARE(ThumbnailGeometry::evenSize({1, 1}), QSize(1, 1));
    QCOMPARE(ThumbnailGeometry::evenSize({3, 5}), QSize(2, 4));
    QCOMPARE(ThumbnailGeometry::evenSize({438, 278}), QSize(438, 278));
  }

  void folderCoverFillsWindowForCacheCappedLargeSourcesOnly() {
    const QSize window(438, 278);

    // Empty inputs produce an empty size.
    QCOMPARE(ThumbnailGeometry::folderCoverSize({}, {4000, 3000}, window), QSize());
    QCOMPARE(ThumbnailGeometry::folderCoverSize({256, 192}, {4000, 3000}, {}), QSize());

    // Cache-capped 256px thumbnail for a 4000x3000 image scales up to fill window.
    QCOMPARE(ThumbnailGeometry::folderCoverSize({256, 192}, {4000, 3000}, window),
             QSize(370, 278));
    // Panorama from a large source fills window width cleanly with even height.
    QCOMPARE(ThumbnailGeometry::folderCoverSize({256, 100}, {5120, 2000}, window),
             QSize(438, 170));
    // Portrait cover with EXIF-transposed raw source smaller than window (200x300 oriented).
    QCOMPARE(ThumbnailGeometry::folderCoverSize({192, 256}, {300, 200}, window),
             QSize(200, 266));
    // Source larger than cached cover yet smaller than window caps enlargement cleanly.
    QCOMPARE(ThumbnailGeometry::folderCoverSize({256, 171}, {300, 200}, window),
             QSize(298, 200));
    // Oversized cover still shrinks to fit window.
    QCOMPARE(ThumbnailGeometry::folderCoverSize({512, 384}, {4000, 3000}, window),
             QSize(370, 278));

    // Tiny native sources keep their original dimensions without enlarging.
    QCOMPARE(ThumbnailGeometry::folderCoverSize({64, 64}, {64, 64}, window),
             QSize(64, 64));
    QCOMPARE(ThumbnailGeometry::folderCoverSize({65, 65}, {65, 65}, window),
             QSize(65, 65));

    // Without known source dimensions, never upscale small covers.
    QCOMPARE(ThumbnailGeometry::folderCoverSize({256, 192}, {}, window),
             QSize(256, 192));
    QCOMPARE(ThumbnailGeometry::folderCoverSize({512, 384}, {}, window),
             QSize(370, 278));
  }
};

int runThumbnailGeometryTests(int argc, char **argv) {
  ThumbnailGeometryTests tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "tst_thumbnailgeometry.moc"
