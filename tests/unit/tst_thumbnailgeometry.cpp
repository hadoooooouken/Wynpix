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
};

int runThumbnailGeometryTests(int argc, char **argv) {
  ThumbnailGeometryTests tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "tst_thumbnailgeometry.moc"
