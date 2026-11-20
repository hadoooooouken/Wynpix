#include "masonrylayout.h"

#include <QtGlobal>

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>

namespace {
// The images of one row.
struct RowPlan {
    // Index of the last image of the row, among the images.
    int last = 0;
    // Sum of the proportions of the images of the row.
    double aspectSum = 0.0;
    // The row reaches the right margin; the last one may not.
    bool full = false;
};

double aspectOf(const QSize &size) {
    if (size.width() <= 0 || size.height() <= 0)
        return MasonryLayout::kUnknownAspect;
    const double aspect = static_cast<double>(size.width()) / size.height();
    return qBound(MasonryLayout::kMinimumAspect, aspect, MasonryLayout::kMaximumAspect);
}

// Height at which images with the proportions sum fill available with
// spacing between them.
double heightToFill(double available, int count, double aspectSum) {
    return (available - (count - 1) * MasonryLayout::kSpacing) / aspectSum;
}

// Images are taken while the row, filled to the full width, is still at least
// as high as the target. The image that makes it lower is kept when that
// brings the height closer to the target than leaving it for the next row.
RowPlan planRow(const QList<double> &aspects, int first, double available, double target) {
    double sum = 0.0;
    int count = 0;
    for (int image = first; image < aspects.size(); ++image) {
        const double withImage = sum + aspects.at(image);
        const double heightWith = heightToFill(available, count + 1, withImage);
        if (heightWith >= target) {
            sum = withImage;
            ++count;
            continue;
        }
        if (count > 0) {
            const double heightWithout = heightToFill(available, count, sum);
            // Penalize stretching rows taller than target more than compressing them,
            // avoiding upscale blur and keeping 16:9 thumbnails within the icon size.
            constexpr double kStretchPenalty = 1.5;
            if ((heightWithout - target) * kStretchPenalty < target - heightWith)
                return {image - 1, sum, true};
        }
        return {image, withImage, true};
    }
    return {static_cast<int>(aspects.size()) - 1, sum, false};
}
} // namespace

std::shared_ptr<const MasonryLayout> MasonryLayout::create(const MasonryLayoutInput &input) {
    return std::shared_ptr<const MasonryLayout>(new MasonryLayout(input));
}

int MasonryLayout::targetRowHeightFor(int iconSize) {
    return qRound(boundedFolderIconSize(iconSize) * kTargetAspect);
}

QSize MasonryLayout::thumbnailBoxFor(int targetRowHeight, qreal devicePixelRatio) {
    const qreal ratio = devicePixelRatio > 0.0 ? devicePixelRatio : 1.0;
    const int side = static_cast<int>(
        std::ceil(qMax(1, targetRowHeight) * kThumbnailLongSide * ratio));
    const int steps = (side + kThumbnailBoxStep - 1) / kThumbnailBoxStep;
    return QSize(steps * kThumbnailBoxStep, steps * kThumbnailBoxStep);
}

MasonryLayout::MasonryLayout(const MasonryLayoutInput &input) {
    const int count = qMax(0, input.itemCount);
    const int dirCount = qBound(0, input.dirCount, count);
    mRects.resize(count);
    addFolderRows(input.folders, dirCount);
    addImageRows(input, count, dirCount);
}

void MasonryLayout::addFolderRows(const FolderGridLayout &folders, int dirCount) {
    const int columns = qMax(1, folders.columns);
    for (int first = 0; first < dirCount; first += columns) {
        const int last = qMin(dirCount, first + columns) - 1;
        for (int item = first; item <= last; ++item)
            mRects[item] = folders.itemRect(item);
        const qreal start = mRects.at(first).top();
        const qreal extent = folders.cellHeight();
        mRows.append(Row{first, last, start, extent});
        mContentExtent = start + extent;
    }
}

void MasonryLayout::addImageRows(const MasonryLayoutInput &input, int count, int dirCount) {
    if (count <= dirCount)
        return;
    QList<double> aspects;
    aspects.reserve(count - dirCount);
    for (int item = dirCount; item < count; ++item)
        aspects.append(aspectOf(input.sourceSizes.value(item)));

    const double available = qMax(1, input.viewWidth - 2 * kMargin);
    const double target = qMax(1, input.targetRowHeight);
    qreal top = mRows.isEmpty() ? kMargin : mContentExtent + kFolderSectionGap;
    int firstImage = 0;
    double lastFullRowHeight = target;
    while (firstImage < aspects.size()) {
        const RowPlan plan = planRow(aspects, firstImage, available, target);
        const int imageCount = plan.last - firstImage + 1;
        const double height =
            plan.full ? heightToFill(available, imageCount, plan.aspectSum) : lastFullRowHeight;
        if (plan.full)
            lastFullRowHeight = height;
        const int rowHeight = qMax(1, qRound(height));
        // The edges are rounded one by one from their exact position, so that
        // the last tile of a full row ends exactly at the right margin.
        double occupied = 0.0;
        for (int offset = 0; offset < imageCount; ++offset) {
            const double leftEdge = kMargin + occupied + offset * kSpacing;
            occupied += aspects.at(firstImage + offset) * height;
            const double rightEdge = kMargin + occupied + offset * kSpacing;
            const int left = qRound(leftEdge);
            const int width = qMax(1, qRound(rightEdge) - left);
            mRects[dirCount + firstImage + offset] = QRectF(left, top, width, rowHeight);
        }
        mRows.append(Row{dirCount + firstImage, dirCount + plan.last, top,
                         static_cast<qreal>(rowHeight)});
        mContentExtent = top + rowHeight;
        top += rowHeight + kSpacing;
        firstImage = plan.last + 1;
    }
    mContentExtent += kMargin;
}

int MasonryLayout::itemCount() const {
    return static_cast<int>(mRects.size());
}

QRectF MasonryLayout::itemRect(int item) const {
    return item >= 0 && item < mRects.size() ? mRects.at(item) : QRectF();
}

int MasonryLayout::itemAt(QPointF point) const {
    if (mRows.isEmpty())
        return -1;
    const int row = rowAt(point.y());
    const Row &found = mRows.at(row);
    if (point.y() < found.start || point.y() >= found.start + found.extent)
        return -1;
    for (int item = found.first; item <= found.last; ++item) {
        if (mRects.at(item).contains(point))
            return item;
    }
    return -1;
}

QList<int> MasonryLayout::itemsIn(const QRectF &area) const {
    QList<int> items;
    if (mRows.isEmpty() || area.isEmpty())
        return items;
    const int firstRow = rowAt(area.top());
    const int lastRow = rowAt(area.bottom());
    for (int row = firstRow; row <= lastRow; ++row) {
        for (int item = mRows.at(row).first; item <= mRows.at(row).last; ++item) {
            if (mRects.at(item).intersects(area))
                items.append(item);
        }
    }
    return items;
}

int MasonryLayout::itemInRow(int row, qreal x) const {
    if (row < 0 || row >= mRows.size())
        return -1;
    int nearest = mRows.at(row).first;
    qreal nearestDistance = std::numeric_limits<qreal>::max();
    for (int item = mRows.at(row).first; item <= mRows.at(row).last; ++item) {
        const QRectF rect = mRects.at(item);
        const qreal distance = x < rect.left() ? rect.left() - x
                               : x > rect.right() ? x - rect.right()
                                                  : 0.0;
        if (distance < nearestDistance) {
            nearest = item;
            nearestDistance = distance;
        }
    }
    return nearest;
}

int MasonryLayout::rowCount() const {
    return static_cast<int>(mRows.size());
}

int MasonryLayout::rowOf(int item) const {
    if (mRows.isEmpty())
        return -1;
    const auto next = std::upper_bound(mRows.cbegin(), mRows.cend(), item,
                                       [](int value, const Row &row) { return value < row.first; });
    return qMax(0, static_cast<int>(std::distance(mRows.cbegin(), next)) - 1);
}

int MasonryLayout::firstItem(int row) const {
    if (mRows.isEmpty())
        return -1;
    return mRows.at(qBound(0, row, static_cast<int>(mRows.size()) - 1)).first;
}

int MasonryLayout::lastItem(int row) const {
    if (mRows.isEmpty())
        return -1;
    return mRows.at(qBound(0, row, static_cast<int>(mRows.size()) - 1)).last;
}

qreal MasonryLayout::rowStart(int row) const {
    if (mRows.isEmpty() || row >= mRows.size())
        return mContentExtent;
    return mRows.at(qMax(0, row)).start;
}

qreal MasonryLayout::rowExtent(int row) const {
    if (mRows.isEmpty())
        return 0.0;
    return mRows.at(qBound(0, row, static_cast<int>(mRows.size()) - 1)).extent;
}

int MasonryLayout::rowAt(qreal position) const {
    if (mRows.isEmpty())
        return -1;
    const auto next =
        std::upper_bound(mRows.cbegin(), mRows.cend(), position,
                         [](qreal value, const Row &row) { return value < row.start; });
    return qMax(0, static_cast<int>(std::distance(mRows.cbegin(), next)) - 1);
}

qreal MasonryLayout::contentExtent() const {
    return mContentExtent;
}
