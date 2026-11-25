#pragma once

#include <QList>
#include <QRectF>
#include <QSize>

#include "gui/quick/ui/thumbnails/itemrows.h"
#include "gui/quick/ui/thumbnails/tileaspect.h"

// Rows of a thumbnail view whose items are tiles of their own size: every
// item has a rectangle in content coordinates. The masonry layout of the
// folder grid (MasonryLayout) and the proportional layout of the thumbnail
// strip (StripMasonryLayout) describe themselves with it, so that
// MasonryTilesModel draws either without knowing how the tiles were placed.
class TileRows : public ItemRows {
public:
    // Number of items the layout was made for.
    [[nodiscard]] virtual int itemCount() const = 0;
    // Tile of the item in content coordinates; empty out of range.
    [[nodiscard]] virtual QRectF itemRect(int item) const = 0;

    // Target box in device pixels for each item's tile.
    [[nodiscard]] QList<QSize> targetSizesFor(qreal devicePixelRatio) const {
        const int count = itemCount();
        QList<QSize> sizes;
        sizes.reserve(count);
        for (int i = 0; i < count; ++i)
            sizes.append(TileAspect::deviceTileSize(itemRect(i).size(), devicePixelRatio));
        return sizes;
    }

protected:
    TileRows() = default;
    // Copyable only through a concrete class, never sliced through the base.
    TileRows(const TileRows &) = default;
    TileRows &operator=(const TileRows &) = default;
};
