#include <QList>
#include <QPair>
#include <QSize>
#include <QTest>

#include "components/thumbnailer/thumbnailgeometry.h"
#include "gui/quick/render/thumbnailitem.h"
#include "gui/quick/ui/thumbnails/stripmasonrylayout.h"
#include "gui/quick/ui/thumbnails/tileaspect.h"
#include "testsuites.h"

namespace {
// The lane of a horizontal strip whose preview size is 256: a 16:9 image is
// 256 wide.
constexpr int kLaneExtent = 144;
// The lane of a vertical strip at the same preview size.
constexpr int kVerticalLaneExtent = 256;
constexpr int kLaneOffset = 20;
constexpr QSize kWideImage(1920, 1080);
constexpr QSize kStandardImage(1600, 1200);
constexpr QSize kSquareImage(1000, 1000);
constexpr QSize kPortraitImage(1000, 1500);
constexpr QSize kUltraWideImage(2520, 1080);
constexpr QSize kExtremeWideImage(10000, 1000);
constexpr QSize kExtremeTallImage(1000, 10000);
// An image far larger than any box, so that the thumbnailer scales it down.
constexpr int kLargeSourceFactor = 4;
constexpr QSize kTinyImage(100, 56);
constexpr int kRoundingTolerancePx = 1;

StripMasonryLayoutInput horizontalInput(const QList<QSize> &sizes, int dirCount = 0) {
  return {.horizontal = true,
          .laneExtent = kLaneExtent,
          .laneOffset = kLaneOffset,
          .itemCount = static_cast<int>(sizes.size()),
          .dirCount = dirCount,
          .sourceSizes = sizes};
}

StripMasonryLayoutInput verticalInput(const QList<QSize> &sizes) {
  return {.horizontal = false,
          .laneExtent = kVerticalLaneExtent,
          .laneOffset = kLaneOffset,
          .itemCount = static_cast<int>(sizes.size()),
          .sourceSizes = sizes};
}

QSize scaled(QSize size, int factor) {
  return QSize(size.width() * factor, size.height() * factor);
}

bool withinTolerance(int value, int expected) {
  return qAbs(value - expected) <= kRoundingTolerancePx;
}
} // namespace

class StripMasonryTests : public QObject {
  Q_OBJECT

private slots:
  void emptyLayoutHasNoRows() {
    const auto layout = StripMasonryLayout::create(horizontalInput({}));
    QCOMPARE(layout->rowCount(), 0);
    QCOMPARE(layout->itemCount(), 0);
    QCOMPARE(layout->rowOf(0), -1);
    QCOMPARE(layout->rowAt(10.0), -1);
    QCOMPARE(layout->itemAt(QPointF(1.0, 1.0)), -1);
    QCOMPARE(layout->contentExtent(), 0.0);
    QVERIFY(layout->itemRect(0).isEmpty());
  }

  void horizontalTilesFollowProportions() {
    const auto layout = StripMasonryLayout::create(horizontalInput(
        {kWideImage, kStandardImage, kSquareImage, kPortraitImage}));
    QCOMPARE(layout->itemRect(0).size(), QSizeF(256, kLaneExtent));
    QCOMPARE(layout->itemRect(1).size(), QSizeF(192, kLaneExtent));
    QCOMPARE(layout->itemRect(2).size(), QSizeF(kLaneExtent, kLaneExtent));
    QCOMPARE(layout->itemRect(3).size(), QSizeF(96, kLaneExtent));
  }

  void horizontalTilesLieInTheLane() {
    const auto layout = StripMasonryLayout::create(horizontalInput({kWideImage, kWideImage}));
    QCOMPARE(layout->itemRect(0).top(), qreal(kLaneOffset));
    QCOMPARE(layout->itemRect(1).top(), qreal(kLaneOffset));
    QVERIFY(layout->itemRect(0).right() < layout->itemRect(1).left());
    QCOMPARE(layout->itemRect(1).left() - layout->itemRect(0).right(),
             qreal(StripMasonryLayout::kSpacing));
  }

  void verticalTilesFollowProportions() {
    const auto layout = StripMasonryLayout::create(
        verticalInput({kWideImage, kStandardImage, kSquareImage, kPortraitImage}));
    QCOMPARE(layout->itemRect(0).size(), QSizeF(kVerticalLaneExtent, 144));
    QCOMPARE(layout->itemRect(1).size(), QSizeF(kVerticalLaneExtent, 192));
    QCOMPARE(layout->itemRect(2).size(), QSizeF(kVerticalLaneExtent, kVerticalLaneExtent));
    QCOMPARE(layout->itemRect(3).size(), QSizeF(kVerticalLaneExtent, 384));
    QCOMPARE(layout->itemRect(0).left(), qreal(kLaneOffset));
    QVERIFY(layout->itemRect(0).bottom() < layout->itemRect(1).top());
  }

  void proportionsAreBounded() {
    const auto horizontal = StripMasonryLayout::create(
        horizontalInput({kExtremeWideImage, kExtremeTallImage}));
    QCOMPARE(horizontal->itemRect(0).width(), kLaneExtent * TileAspect::kMaximum);
    QCOMPARE(horizontal->itemRect(1).width(), kLaneExtent * TileAspect::kMinimum);
    const auto vertical = StripMasonryLayout::create(
        verticalInput({kExtremeWideImage, kExtremeTallImage}));
    QCOMPARE(vertical->itemRect(0).height(), kVerticalLaneExtent / TileAspect::kMaximum);
    QCOMPARE(vertical->itemRect(1).height(), kVerticalLaneExtent / TileAspect::kMinimum);
  }

  void unknownSizeIsFourToThree() {
    const auto layout = StripMasonryLayout::create(horizontalInput({QSize(), QSize(0, 100)}));
    QCOMPARE(layout->itemRect(0).width(), qreal(192));
    QCOMPARE(layout->itemRect(1).width(), qreal(192));
  }

  void foldersKeepTheCellProportions() {
    // The sizes of the folders are not known; whatever is given for them
    // does not matter.
    const auto layout = StripMasonryLayout::create(
        horizontalInput({kExtremeWideImage, kSquareImage, kWideImage}, 1));
    QCOMPARE(layout->itemRect(0).size(),
             QSizeF(StripMasonryLayout::folderTileSize(true, kLaneExtent)));
    QCOMPARE(layout->itemRect(0).width(), qreal(192));
    QCOMPARE(layout->itemRect(1).width(), qreal(kLaneExtent));
    QCOMPARE(layout->itemRect(2).width(), qreal(256));
  }

  void everyItemIsARowThatFollowsTheOneBefore() {
    const auto layout = StripMasonryLayout::create(
        horizontalInput({kWideImage, kPortraitImage, kSquareImage, kStandardImage}));
    QCOMPARE(layout->rowCount(), 4);
    qreal expectedStart = 0.0;
    for (int row = 0; row < layout->rowCount(); ++row) {
      QCOMPARE(layout->rowOf(row), row);
      QCOMPARE(layout->firstItem(row), row);
      QCOMPARE(layout->lastItem(row), row);
      QCOMPARE(layout->rowStart(row), expectedStart);
      const QRectF tile = layout->itemRect(row);
      QCOMPARE(layout->rowExtent(row), tile.width() + StripMasonryLayout::kSpacing);
      // The tile is centred in its row.
      QCOMPARE(tile.left() - layout->rowStart(row),
               qreal(StripMasonryLayout::kSpacing / 2));
      expectedStart += layout->rowExtent(row);
    }
    QCOMPARE(layout->rowStart(layout->rowCount()), expectedStart);
    QCOMPARE(layout->contentExtent(), expectedStart);
  }

  void queriesOutOfRangeAreClamped() {
    const auto layout = StripMasonryLayout::create(horizontalInput({kWideImage, kWideImage}));
    QCOMPARE(layout->rowAt(-50.0), 0);
    QCOMPARE(layout->rowAt(layout->contentExtent() + 500.0), 1);
    QCOMPARE(layout->rowOf(99), 1);
    QCOMPARE(layout->firstItem(-3), 0);
    QCOMPARE(layout->lastItem(99), 1);
    QVERIFY(layout->itemRect(2).isEmpty());
    QVERIFY(layout->itemRect(-1).isEmpty());
  }

  void rowAtFindsTheRowOfAPosition() {
    const auto layout = StripMasonryLayout::create(horizontalInput({kWideImage, kSquareImage}));
    QCOMPARE(layout->rowAt(0.0), 0);
    QCOMPARE(layout->rowAt(layout->rowStart(1) - 0.5), 0);
    QCOMPARE(layout->rowAt(layout->rowStart(1)), 1);
  }

  void itemAtFindsTilesAndSkipsGaps() {
    const auto layout = StripMasonryLayout::create(horizontalInput({kWideImage, kSquareImage}));
    const QRectF first = layout->itemRect(0);
    const QRectF second = layout->itemRect(1);
    QCOMPARE(layout->itemAt(first.center()), 0);
    QCOMPARE(layout->itemAt(second.center()), 1);
    // The gap between the tiles, and the space across the strip around them.
    QCOMPARE(layout->itemAt(QPointF((first.right() + second.left()) / 2.0, first.center().y())),
             -1);
    QCOMPARE(layout->itemAt(QPointF(first.center().x(), first.top() - 1.0)), -1);
    QCOMPARE(layout->itemAt(QPointF(first.center().x(), first.bottom() + 1.0)), -1);
  }

  void verticalItemAtUsesTheLongAxis() {
    const auto layout = StripMasonryLayout::create(verticalInput({kWideImage, kSquareImage}));
    QCOMPARE(layout->itemAt(layout->itemRect(0).center()), 0);
    QCOMPARE(layout->itemAt(layout->itemRect(1).center()), 1);
    QCOMPARE(layout->itemAt(QPointF(layout->itemRect(0).center().x(),
                                    layout->itemRect(0).bottom() + 1.0)),
             -1);
  }

  // The thumbnail of every image, made in the box of the strip, has the size
  // of its tile times the device pixel ratio, and is drawn at the size of its
  // tile: no resize and no enlargement.
  void thumbnailsAreMadeAtTheSizeOfTheirTiles_data() {
    QTest::addColumn<bool>("horizontal");
    QTest::addColumn<int>("lane");
    QTest::addColumn<QSize>("source");
    QTest::addColumn<qreal>("ratio");
    const QList<qreal> ratios{1.0, 1.25, 1.5, 2.0};
    const QList<QPair<const char *, QSize>> images{
        {"16:9", kWideImage},          {"4:3", kStandardImage},
        {"1:1", kSquareImage},         {"2:3", kPortraitImage},
        {"21:9", kUltraWideImage},     {"10:1 (bounded)", kExtremeWideImage},
        {"1:10 (bounded)", kExtremeTallImage}};
    for (const bool horizontal : {true, false}) {
      for (const qreal ratio : ratios) {
        for (const auto &[name, size] : images) {
          QTest::addRow("%s %s x%g", horizontal ? "horizontal" : "vertical", name, ratio)
              << horizontal << (horizontal ? kLaneExtent : kVerticalLaneExtent)
              << scaled(size, kLargeSourceFactor) << ratio;
        }
      }
    }
  }

  void thumbnailsAreMadeAtTheSizeOfTheirTiles() {
    QFETCH(bool, horizontal);
    QFETCH(int, lane);
    QFETCH(QSize, source);
    QFETCH(qreal, ratio);
    const QSize box = StripMasonryLayout::thumbnailBoxFor(horizontal, lane, ratio);
    const QSize pixelSize =
        ThumbnailGeometry::noUpscaleScaledSize(source, box, Qt::KeepAspectRatio);
    const QSize tile = StripMasonryLayout::tileSizeFor(horizontal, lane, TileAspect::of(source));
    // The thumbnail is the image fitted into the tile, kept in proportion.
    const QSize fitted = QSize(source).scaled(tile * ratio, Qt::KeepAspectRatio);
    QVERIFY2(withinTolerance(pixelSize.width(), fitted.width()) &&
                 withinTolerance(pixelSize.height(), fitted.height()),
             qPrintable(QStringLiteral("thumbnail %1x%2, tile fit %3x%4")
                            .arg(pixelSize.width()).arg(pixelSize.height())
                            .arg(fitted.width()).arg(fitted.height())));
    const QSize drawn = thumbnailDrawSize(pixelSize, source, tile, ratio);
    const QSize drawnFit = QSize(source).scaled(tile, Qt::KeepAspectRatio);
    QVERIFY2(withinTolerance(drawn.width(), drawnFit.width()) &&
                 withinTolerance(drawn.height(), drawnFit.height()),
             qPrintable(QStringLiteral("drawn %1x%2, tile fit %3x%4")
                            .arg(drawn.width()).arg(drawn.height())
                            .arg(drawnFit.width()).arg(drawnFit.height())));
  }

  void widePreviewIsAsWideAsThePreviewSize() {
    // The default preview size: a 16:9 thumbnail is 256 wide at 1:1.
    const QSize box = StripMasonryLayout::thumbnailBoxFor(true, kLaneExtent, 1.0);
    const QSize pixelSize = ThumbnailGeometry::noUpscaleScaledSize(
        scaled(kWideImage, kLargeSourceFactor), box, Qt::KeepAspectRatio);
    QCOMPARE(pixelSize, QSize(256, 144));
    const QSize verticalBox = StripMasonryLayout::thumbnailBoxFor(false, kVerticalLaneExtent, 1.0);
    QCOMPARE(ThumbnailGeometry::noUpscaleScaledSize(scaled(kWideImage, kLargeSourceFactor),
                                                     verticalBox, Qt::KeepAspectRatio),
             QSize(256, 144));
  }

  void smallImagesAreNeitherScaledNorEnlarged() {
    const QSize box = StripMasonryLayout::thumbnailBoxFor(true, kLaneExtent, 1.0);
    const QSize pixelSize =
        ThumbnailGeometry::noUpscaleScaledSize(kTinyImage, box, Qt::KeepAspectRatio);
    QCOMPARE(pixelSize, kTinyImage);
    const QSize tile = StripMasonryLayout::tileSizeFor(true, kLaneExtent, TileAspect::of(kTinyImage));
    // Drawn within the source image, in proportion.
    const QSize drawn = thumbnailDrawSize(pixelSize, kTinyImage, tile, 1.0);
    QVERIFY(drawn.width() <= kTinyImage.width() && drawn.height() <= kTinyImage.height());
    QVERIFY(withinTolerance(drawn.width(), kTinyImage.width()));
    QVERIFY(withinTolerance(drawn.height(), kTinyImage.height()));
  }

  void folderBoxIsTheFolderTile() {
    QCOMPARE(StripMasonryLayout::folderThumbnailBoxFor(true, kLaneExtent, 1.0), QSize(192, 144));
    QCOMPARE(StripMasonryLayout::folderThumbnailBoxFor(true, kLaneExtent, 2.0), QSize(384, 288));
    QCOMPARE(StripMasonryLayout::folderThumbnailBoxFor(false, kVerticalLaneExtent, 1.0),
             QSize(kVerticalLaneExtent, 192));
  }

  void boxIgnoresInvalidDevicePixelRatio() {
    QCOMPARE(StripMasonryLayout::thumbnailBoxFor(true, kLaneExtent, 0.0),
             StripMasonryLayout::thumbnailBoxFor(true, kLaneExtent, 1.0));
  }
};

int runStripMasonryTests(int argc, char **argv) {
  StripMasonryTests tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "tst_stripmasonry.moc"
