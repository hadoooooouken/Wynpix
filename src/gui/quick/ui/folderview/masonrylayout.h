#pragma once

#include <QList>
#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QtGlobal>

#include <memory>

#include "gui/quick/ui/folderview/foldergridlayout.h"
#include "gui/quick/ui/thumbnails/itemrows.h"

// What the masonry layout depends on.
struct MasonryLayoutInput {
    // Width of the area without the scroll bar, in logical pixels.
    int viewWidth = 0;
    // Height the rows aim at (MasonryLayout::targetRowHeightFor()).
    int targetRowHeight = 0;
    int itemCount = 0;
    // The first dirCount items are folders; they keep the labelled cells of
    // the folder grid and form a strip above the images.
    int dirCount = 0;
    // Geometry of the folder cells.
    FolderGridLayout folders;
    // Size of the images by item index; an invalid or missing size is an
    // image whose proportions are not known yet.
    QList<QSize> sourceSizes;
};

// Folders in a strip of uniform cells, then the images in rows of equal
// height that run from left to right (a justified gallery): each row takes as
// many images as fit at about the target height, and its height is adjusted
// so that the row ends exactly at the right margin. The last row keeps the
// target height and is aligned to the left. Positions are in content
// coordinates, in logical pixels. The layout never changes once created.
class MasonryLayout final : public ItemRows {
public:
    // Gap between the images, and the margin around them.
    static constexpr int kSpacing = 4;
    static constexpr int kMargin = 4;
    // Space between the folder strip and the images.
    static constexpr int kFolderSectionGap = 8;
    // Proportions (width / height) of an image of unknown size, and the
    // limits the proportions of the others are held to; a thumbnail more
    // extreme than that is fitted into its tile with empty space around.
    static constexpr double kUnknownAspect = 4.0 / 3.0;
    static constexpr double kMinimumAspect = 0.25;
    static constexpr double kMaximumAspect = 4.0;
    // Thumbnails are made in a square box whose side is this many target
    // row heights, which fits the usual proportions up to 16:9, rounded up to
    // a step so that small size changes keep the same box.
    static constexpr double kThumbnailLongSide = 1.8;
    static constexpr int kThumbnailBoxStep = 32;

    [[nodiscard]] static std::shared_ptr<const MasonryLayout> create(const MasonryLayoutInput &input);
    // The row height that goes with the icon size of the grid.
    [[nodiscard]] static int targetRowHeightFor(int iconSize);
    // The box, in device pixels, the thumbnails of the images are made for.
    [[nodiscard]] static QSize thumbnailBoxFor(int targetRowHeight, qreal devicePixelRatio);

    [[nodiscard]] int itemCount() const;
    // Tile of the item (the cell of a folder); empty out of range.
    [[nodiscard]] QRectF itemRect(int item) const;
    // The item at point, -1 on a gap or outside the tiles.
    [[nodiscard]] int itemAt(QPointF point) const;
    // The items whose tile intersects area.
    [[nodiscard]] QList<int> itemsIn(const QRectF &area) const;
    // The item of row that is nearest to the horizontal position x, -1
    // without that row.
    [[nodiscard]] int itemInRow(int row, qreal x) const;

    [[nodiscard]] int rowCount() const override;
    [[nodiscard]] int rowOf(int item) const override;
    [[nodiscard]] int firstItem(int row) const override;
    [[nodiscard]] int lastItem(int row) const override;
    [[nodiscard]] qreal rowStart(int row) const override;
    [[nodiscard]] qreal rowExtent(int row) const override;
    [[nodiscard]] int rowAt(qreal position) const override;
    [[nodiscard]] qreal contentExtent() const override;

private:
    struct Row {
        int first = 0;
        int last = 0;
        qreal start = 0.0;
        qreal extent = 0.0;
    };

    explicit MasonryLayout(const MasonryLayoutInput &input);
    void addFolderRows(const FolderGridLayout &folders, int dirCount);
    void addImageRows(const MasonryLayoutInput &input, int count, int dirCount);

    QList<QRectF> mRects;
    QList<Row> mRows;
    qreal mContentExtent = 0.0;
};
