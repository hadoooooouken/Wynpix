#pragma once

#include <QList>
#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QtGlobal>

#include <memory>

#include "gui/quick/ui/thumbnails/thumbnailstriplayout.h"
#include "gui/quick/ui/thumbnails/tilerows.h"

// What the proportional layout of the thumbnail strip depends on.
struct StripMasonryLayoutInput {
    // The strip runs from left to right (top and bottom panel).
    bool horizontal = true;
    // Size of the tiles across the strip, in logical pixels: the height of a
    // tile in a horizontal strip, the width of a tile in a vertical one.
    int laneExtent = 0;
    // Distance from the start of the strip, across it, to the tiles.
    int laneOffset = 0;
    int itemCount = 0;
    // The first dirCount items are folders; their tile has the proportions
    // of the image area of a cell of the uniform strip.
    int dirCount = 0;
    // Size of the images by item index; an invalid or missing size is an
    // image whose proportions are not known yet.
    QList<QSize> sourceSizes;
};

// The tiles of the thumbnail strip in one lane that follow the proportions of
// their images: in a horizontal strip every tile is as high as the lane and as
// wide as its image requires, in a vertical one every tile is as wide as the
// lane and as high as its image requires. A tile is never given proportions
// beyond TileAspect::kMinimum - TileAspect::kMaximum; a thumbnail more extreme
// than that is fitted into its tile with empty space around.
//
// Every item is a row of its own (ItemRows): the row holds the tile and half
// of kSpacing before and after it. Positions are in content coordinates, in
// logical pixels. The layout never changes once created.
class StripMasonryLayout final : public TileRows {
public:
    // Space between neighbouring tiles.
    static constexpr int kSpacing = 4;
    // Proportions (width / height) of a folder tile: those of the image area
    // of a cell of the uniform strip.
    static constexpr double kFolderAspect = 1.0 / ThumbnailStripLayout::kImageAspect;

    [[nodiscard]] static std::shared_ptr<const StripMasonryLayout> create(
        const StripMasonryLayoutInput &input);
    // Size of a tile with the given proportions in the lane.
    [[nodiscard]] static QSize tileSizeFor(bool horizontal, int laneExtent, double aspect);
    // Size of the tile of a folder.
    [[nodiscard]] static QSize folderTileSize(bool horizontal, int laneExtent);
    // The box, in device pixels, the thumbnails of the images are made for:
    // the lane across the strip, the longest tile along it. The thumbnailer
    // fits an image into it with its proportions kept, so the thumbnail of
    // every image has exactly the size of its tile times the device pixel
    // ratio and is drawn without a resize.
    [[nodiscard]] static QSize thumbnailBoxFor(bool horizontal, int laneExtent,
                                               qreal devicePixelRatio);
    // The same for a folder: the box is its tile.
    [[nodiscard]] static QSize folderThumbnailBoxFor(bool horizontal, int laneExtent,
                                                     qreal devicePixelRatio);

    [[nodiscard]] int itemCount() const override;
    // Tile of the item; empty out of range.
    [[nodiscard]] QRectF itemRect(int item) const override;
    // The item at point, -1 on a gap or outside the tiles.
    [[nodiscard]] int itemAt(QPointF point) const;

    [[nodiscard]] int rowCount() const override;
    [[nodiscard]] int rowOf(int item) const override;
    [[nodiscard]] int firstItem(int row) const override;
    [[nodiscard]] int lastItem(int row) const override;
    [[nodiscard]] qreal rowStart(int row) const override;
    [[nodiscard]] qreal rowExtent(int row) const override;
    [[nodiscard]] int rowAt(qreal position) const override;
    [[nodiscard]] qreal contentExtent() const override;

private:
    explicit StripMasonryLayout(const StripMasonryLayoutInput &input);
    [[nodiscard]] int clampedItem(int item) const;

    bool mHorizontal = true;
    QList<QRectF> mRects;
    // Start of every row along the strip.
    QList<qreal> mStarts;
    qreal mContentExtent = 0.0;
};
