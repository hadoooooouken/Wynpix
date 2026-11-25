#include "stripmasonrylayout.h"

#include <QtGlobal>

#include <algorithm>
#include <iterator>

#include "gui/quick/ui/thumbnails/tileaspect.h"

namespace {
// Space held by a row on each side of its tile.
constexpr int kHalfSpacing = StripMasonryLayout::kSpacing / 2;
} // namespace

std::shared_ptr<const StripMasonryLayout> StripMasonryLayout::create(
    const StripMasonryLayoutInput &input) {
    return std::shared_ptr<const StripMasonryLayout>(new StripMasonryLayout(input));
}

QSize StripMasonryLayout::tileSizeFor(bool horizontal, int laneExtent, double aspect) {
    const int lane = qMax(1, laneExtent);
    if (horizontal)
        return QSize(qMax(1, qRound(lane * aspect)), lane);
    return QSize(lane, qMax(1, qRound(lane / aspect)));
}

QSize StripMasonryLayout::folderTileSize(bool horizontal, int laneExtent) {
    return tileSizeFor(horizontal, laneExtent, kFolderAspect);
}

QSize StripMasonryLayout::thumbnailBoxFor(bool horizontal, int laneExtent,
                                          qreal devicePixelRatio) {
    // The longest tile: TileAspect::kMaximum wide in a horizontal strip,
    // 1 / TileAspect::kMinimum high in a vertical one.
    const int lane = qMax(1, laneExtent);
    const QSize longest = horizontal ? tileSizeFor(true, lane, TileAspect::kMaximum)
                                     : tileSizeFor(false, lane, TileAspect::kMinimum);
    return TileAspect::deviceTileSize(longest, devicePixelRatio);
}

QSize StripMasonryLayout::folderThumbnailBoxFor(bool horizontal, int laneExtent,
                                                qreal devicePixelRatio) {
    return TileAspect::deviceTileSize(folderTileSize(horizontal, laneExtent), devicePixelRatio);
}

StripMasonryLayout::StripMasonryLayout(const StripMasonryLayoutInput &input)
    : mHorizontal(input.horizontal) {
    const int count = qMax(0, input.itemCount);
    const int dirCount = qBound(0, input.dirCount, count);
    mRects.reserve(count);
    mStarts.reserve(count);
    qreal start = 0.0;
    for (int item = 0; item < count; ++item) {
        const double aspect =
            item < dirCount ? kFolderAspect : TileAspect::of(input.sourceSizes.value(item));
        const QSize size = tileSizeFor(mHorizontal, input.laneExtent, aspect);
        const qreal along = start + kHalfSpacing;
        mRects.append(mHorizontal ? QRectF(QPointF(along, input.laneOffset), size)
                                  : QRectF(QPointF(input.laneOffset, along), size));
        mStarts.append(start);
        start += (mHorizontal ? size.width() : size.height()) + kSpacing;
    }
    mContentExtent = start;
}

int StripMasonryLayout::itemCount() const {
    return static_cast<int>(mRects.size());
}

QRectF StripMasonryLayout::itemRect(int item) const {
    return item >= 0 && item < mRects.size() ? mRects.at(item) : QRectF();
}

int StripMasonryLayout::itemAt(QPointF point) const {
    const int row = rowAt(mHorizontal ? point.x() : point.y());
    if (row < 0)
        return -1;
    return mRects.at(row).contains(point) ? row : -1;
}

int StripMasonryLayout::clampedItem(int item) const {
    return qBound(0, item, static_cast<int>(mRects.size()) - 1);
}

int StripMasonryLayout::rowCount() const {
    return static_cast<int>(mRects.size());
}

int StripMasonryLayout::rowOf(int item) const {
    return mRects.isEmpty() ? -1 : clampedItem(item);
}

int StripMasonryLayout::firstItem(int row) const {
    return mRects.isEmpty() ? -1 : clampedItem(row);
}

int StripMasonryLayout::lastItem(int row) const {
    return mRects.isEmpty() ? -1 : clampedItem(row);
}

qreal StripMasonryLayout::rowStart(int row) const {
    if (mStarts.isEmpty() || row >= mStarts.size())
        return mContentExtent;
    return mStarts.at(qMax(0, row));
}

qreal StripMasonryLayout::rowExtent(int row) const {
    if (mStarts.isEmpty())
        return 0.0;
    const int item = clampedItem(row);
    const qreal end = item + 1 < mStarts.size() ? mStarts.at(item + 1) : mContentExtent;
    return end - mStarts.at(item);
}

int StripMasonryLayout::rowAt(qreal position) const {
    if (mStarts.isEmpty())
        return -1;
    const auto next = std::upper_bound(mStarts.cbegin(), mStarts.cend(), position);
    return qMax(0, static_cast<int>(std::distance(mStarts.cbegin(), next)) - 1);
}

qreal StripMasonryLayout::contentExtent() const {
    return mContentExtent;
}
