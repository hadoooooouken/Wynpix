#include "stripmasonrycontroller.h"

#include <QtGlobal>

#include <cmath>

StripMasonryController::StripMasonryController(ThumbnailListModel &source, QObject *parent)
    : QObject(parent), mSource(source) {
    mTiles.setSource(&mSource);
    mLayoutTimer.setSingleShot(true);
    connect(&mLayoutTimer, &QTimer::timeout, this, &StripMasonryController::rebuild);
    mGraceTimer.setSingleShot(true);
    connect(&mGraceTimer, &QTimer::timeout, this, &StripMasonryController::onGraceTimeout);
    connect(&mSource, &ThumbnailListModel::sourceSizesChanged, this,
            &StripMasonryController::onSourceSizesChanged);
    connect(&mSource, &QAbstractItemModel::modelReset, this,
            &StripMasonryController::onModelReset);
    connect(&mSource, &ThumbnailListModel::selectionChanged, this,
            &StripMasonryController::markerPositionChanged);
    // Items that came or went change the layout at once.
    connect(&mSource, &ThumbnailListModel::countChanged, this,
            &StripMasonryController::onStructureChanged);
    connect(&mSource, &ThumbnailListModel::dirCountChanged, this,
            &StripMasonryController::onStructureChanged);
}

bool StripMasonryController::isEnabled() const {
    return mEnabled;
}

bool StripMasonryController::isReady() const {
    return mReady;
}

qreal StripMasonryController::contentExtent() const {
    return mLayout ? mLayout->contentExtent() : 0.0;
}

qreal StripMasonryController::markerPosition() const {
    const int current = mSource.currentIndex();
    if (!mLayout || current < 0 || current >= mLayout->itemCount() ||
        mLayout->contentExtent() <= 0.0)
        return 0.0;
    const QPointF center = mLayout->itemRect(current).center();
    return (mStrip.horizontal ? center.x() : center.y()) / mLayout->contentExtent();
}

MasonryTilesModel *StripMasonryController::tiles() {
    return &mTiles;
}

std::shared_ptr<const StripMasonryLayout> StripMasonryController::layout() const {
    return mLayout;
}

void StripMasonryController::setStripLayout(const ThumbnailStripLayout &strip) {
    if (strip == mStrip)
        return;
    mStrip = strip;
    setEnabled(strip.masonry);
    rebuild();
}

void StripMasonryController::restoreAnchor(const StripScrollAnchor &anchor) {
    // A directory that was just opened starts at its beginning
    // (ThumbnailListModel::populate() moves the view there after the reset):
    // the place the view had in the one before is of no use, not even as a
    // limit.
    if (!mLayout || mAnchorSuspended)
        return;
    // Without an anchor (a view at the start, or its item went) the view
    // stays where it is, as far as the content reaches.
    qreal target = mSource.viewOffset();
    if (anchor.isValid() && anchor.item < mLayout->itemCount())
        target = mLayout->rowStart(mLayout->rowOf(anchor.item)) - anchor.distance;
    const qreal maximum = qMax(0.0, mLayout->contentExtent() - mSource.viewExtent());
    target = qBound(0.0, target, maximum);
    if (std::abs(target - mSource.viewOffset()) > kAnchorTolerancePx)
        mSource.restoreOffset(target);
}

int StripMasonryController::itemAt(qreal x, qreal y) const {
    return mLayout ? mLayout->itemAt(QPointF(x, y)) : -1;
}

//------------------------------------------------------------------------------

// Only a strip in the masonry mode needs the sizes of the images.
void StripMasonryController::setEnabled(bool enabled) {
    if (enabled == mEnabled)
        return;
    mEnabled = enabled;
    mSource.setImageSizesWanted(enabled);
    resetReadiness();
    emit enabledChanged();
}

void StripMasonryController::setReady(bool ready) {
    if (ready == mReady)
        return;
    mReady = ready;
    emit readyChanged();
}

void StripMasonryController::rebuild() {
    mLayoutTimer.stop();
    const StripScrollAnchor anchor = captureAnchor();
    if (!mEnabled) {
        if (!mLayout)
            return;
        mLayout.reset();
        mTiles.setLayout(nullptr, ThumbnailStripLayout());
    } else {
        mLayout = StripMasonryLayout::create({
            .horizontal = mStrip.horizontal,
            .laneExtent = mStrip.laneExtent,
            .laneOffset = mStrip.laneOffset,
            .itemCount = mSource.itemCount(),
            .dirCount = mSource.dirCount(),
            .sourceSizes = mSource.sourceSizes(),
        });
        mLayoutDirCount = mSource.dirCount();
        // Folders keep the proportions of the cell of the uniform strip.
        mTiles.setLayout(mLayout, thumbnailTileCellFor(StripMasonryLayout::folderTileSize(
                                      mStrip.horizontal, mStrip.laneExtent)));
    }
    emit contentExtentChanged();
    emit markerPositionChanged();
    emit layoutChanged(anchor);
}

// The selected tile while it is in view, otherwise the first one in view; a
// view at the start stays there.
StripScrollAnchor StripMasonryController::captureAnchor() const {
    if (mAnchorSuspended || !mLayout || mLayout->rowCount() == 0)
        return {};
    const qreal offset = mSource.viewOffset();
    const qreal extent = mSource.viewExtent();
    const int current = mSource.currentIndex();
    int item = -1;
    if (current >= 0 && current < mLayout->itemCount()) {
        const int row = mLayout->rowOf(current);
        const qreal start = mLayout->rowStart(row);
        if (start + mLayout->rowExtent(row) > offset && start < offset + extent)
            item = current;
    }
    if (item < 0) {
        if (offset <= 0.0)
            return {};
        item = mLayout->firstItem(mLayout->rowAt(offset));
    }
    return {.item = item, .distance = mLayout->rowStart(mLayout->rowOf(item)) - offset};
}

// The images around the selection are the ones the strip shows first.
bool StripMasonryController::initialSizesKnown() const {
    const int count = mSource.itemCount();
    const int dirCount = mSource.dirCount();
    if (count <= dirCount)
        return true;
    const int center = qMax(dirCount, mSource.currentIndex());
    const int first = qMax(dirCount, center - kInitialCheckCount / 2);
    const int last = qMin(count, first + kInitialCheckCount);
    for (int item = first; item < last; ++item) {
        if (!mSource.hasSourceSize(item))
            return false;
    }
    return true;
}

// The images in view: what the layout that follows the sizes has to wait for
// sooner than for the rest.
bool StripMasonryController::visibleSizesKnown() const {
    const qreal extent = mSource.viewExtent();
    if (!mLayout || mLayout->rowCount() == 0 || extent <= 0.0)
        return initialSizesKnown();
    const qreal offset = mSource.viewOffset();
    const int first = qMax(mSource.dirCount(), mLayout->firstItem(mLayout->rowAt(offset)));
    const int last =
        qMin(mLayout->lastItem(mLayout->rowAt(offset + extent)), mSource.itemCount() - 1);
    for (int item = first; item <= last; ++item) {
        if (!mSource.hasSourceSize(item))
            return false;
    }
    return true;
}

void StripMasonryController::resetReadiness() {
    mLayoutTimer.stop();
    if (!mEnabled || initialSizesKnown()) {
        mGraceTimer.stop();
        mInitialBatchPending = false;
        setReady(true);
    } else {
        mInitialBatchPending = true;
        setReady(false);
        mGraceTimer.start(kGracePeriodMs);
    }
}

// Sizes arrive in a stream; the layout follows them in steps.
void StripMasonryController::scheduleLayout() {
    if (mLayoutTimer.isActive())
        return;
    mLayoutTimer.start(kLayoutDelayMs);
}

void StripMasonryController::scheduleFastLayout() {
    if (mLayoutTimer.isActive() && mLayoutTimer.remainingTime() <= kVisibleLayoutDelayMs)
        return;
    mLayoutTimer.start(kVisibleLayoutDelayMs);
}

// populate() resets the model and then announces the count and the folders
// that the layout made for the reset already has.
void StripMasonryController::onStructureChanged() {
    if (mLayout && mLayout->itemCount() == mSource.itemCount() &&
        mLayoutDirCount == mSource.dirCount())
        return;
    rebuild();
}

void StripMasonryController::onModelReset() {
    if (!mEnabled)
        return;
    // ThumbnailListModel::populate() moves the view to the start after the
    // reset and the signals that follow it.
    mAnchorSuspended = true;
    QTimer::singleShot(0, this, [this]() { mAnchorSuspended = false; });
    resetReadiness();
    rebuild();
}

void StripMasonryController::onSourceSizesChanged() {
    if (!mEnabled)
        return;
    if (mInitialBatchPending) {
        if (!initialSizesKnown())
            return;
        mGraceTimer.stop();
        mInitialBatchPending = false;
        rebuild();
        setReady(true);
        return;
    }
    if (visibleSizesKnown())
        scheduleLayout();
    else
        scheduleFastLayout();
}

void StripMasonryController::onGraceTimeout() {
    if (!mEnabled)
        return;
    mInitialBatchPending = false;
    rebuild();
    setReady(true);
}
