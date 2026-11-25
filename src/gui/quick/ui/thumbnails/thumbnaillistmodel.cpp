#include "thumbnaillistmodel.h"

#include <QDebug>
#include <QLineF>
#include <QVariant>
#include <QtGlobal>

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace {
const QList<int> kThumbnailRoles = {
    ThumbnailListModel::NameRole,    ThumbnailListModel::InfoRole,
    ThumbnailListModel::ThumbnailRole, ThumbnailListModel::LoadedRole,
    ThumbnailListModel::PendingRole, ThumbnailListModel::UnavailableRole,
};

// Offsets the view reports are fractional: within this distance a row edge
// counts as at the view edge, and the view as at its start or end.
constexpr qreal kScrollEdgeTolerancePx = 0.5;

QSet<int> shiftedSet(const QSet<int> &set, int firstIndex, int offset) {
    QSet<int> shifted;
    shifted.reserve(set.size());
    for (const int index : set)
        shifted.insert(index >= firstIndex ? index + offset : index);
    return shifted;
}
} // namespace

ThumbnailListModel::ThumbnailListModel(QObject *parent) : QAbstractListModel(parent) {
    mLoadTimer.setSingleShot(true);
    mLoadTimer.setInterval(kLoadDelayMs);
    connect(&mLoadTimer, &QTimer::timeout, this, &ThumbnailListModel::loadVisibleThumbnails);
    mPrefetchTimer.setSingleShot(true);
    mPrefetchTimer.setInterval(kInactivePrefetchDelayMs);
    connect(&mPrefetchTimer, &QTimer::timeout, this,
            &ThumbnailListModel::prefetchInactiveThumbnails);
}

int ThumbnailListModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : mCount;
}

QVariant ThumbnailListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || !checkRange(index.row()))
        return {};
    const int row = index.row();
    const auto loaded = mLoaded.constFind(row);
    const bool isLoaded = loaded != mLoaded.cend();
    switch (role) {
    case NameRole:
        return isLoaded ? loaded->name : QString();
    case InfoRole:
        return isLoaded ? loaded->info : QString();
    case DirectoryRole:
        return row < mDirCount;
    case ThumbnailRole:
        return QVariant::fromValue(isLoaded ? loaded->handle : ThumbnailHandle());
    case LoadedRole:
        return isLoaded;
    case PendingRole:
        return mRequested.contains(row) || mPendingFinal.contains(row);
    case UnavailableRole:
        return mUnavailable.contains(row);
    case SelectedRole:
        return mSelection.contains(row);
    case DragHoverRole:
        return row == mDragHover;
    default:
        return {};
    }
}

QHash<int, QByteArray> ThumbnailListModel::roleNames() const {
    return {
        {NameRole, "name"},
        {InfoRole, "info"},
        {DirectoryRole, "isDir"},
        {ThumbnailRole, "thumbnail"},
        {LoadedRole, "loaded"},
        {PendingRole, "pending"},
        {UnavailableRole, "unavailable"},
        {SelectedRole, "selected"},
        {DragHoverRole, "dragHover"},
    };
}

int ThumbnailListModel::itemCount() const {
    return mCount;
}

int ThumbnailListModel::dirCount() const {
    return mDirCount;
}

int ThumbnailListModel::currentIndex() const {
    return mSelection.isEmpty() ? -1 : mSelection.constLast();
}

QList<int> ThumbnailListModel::selection() const {
    return mSelection;
}

ThumbnailRequestConfig ThumbnailListModel::requestConfig() const {
    return mRequest;
}

ThumbnailScrollConfig ThumbnailListModel::scrollConfig() const {
    return mScroll;
}

qreal ThumbnailListModel::viewOffset() const {
    return mOffset;
}

qreal ThumbnailListModel::viewExtent() const {
    return mExtent;
}

bool ThumbnailListModel::canScrollBackward() const {
    return mCanScrollBackward;
}

bool ThumbnailListModel::canScrollForward() const {
    return mCanScrollForward;
}

//------------------------------------------------------------------------------
// Directory view

void ThumbnailListModel::populate(int count, int dirCount, const QList<QSize> &knownSizes) {
    if (count < 0) {
        qWarning() << "ThumbnailListModel received a negative item count:" << count;
        return;
    }
    const int newDirCount = qMax(0, dirCount);
    const bool dirCountDiffers = newDirCount != mDirCount;
    beginResetModel();
    mCount = count;
    mDirCount = newDirCount;
    refreshUniformRows();
    mSelection.clear();
    mDragHover = -1;
    mLoaded.clear();
    mSourceSizes.clear();
    for (int i = 0; i < knownSizes.size() && i < count; ++i) {
        if (knownSizes.at(i).isValid())
            mSourceSizes.insert(i, knownSizes.at(i));
    }
    mRequested.clear();
    mPendingFinal.clear();
    mUnavailable.clear();
    mReadyReported = false;
    mScrollDirection = ScrollDirection::Forwards;
    mRangeAnchor.clear();
    endResetModel();
    emit countChanged();
    if (dirCountDiffers)
        emit dirCountChanged();
    emit selectionChanged();
    // The view starts at the beginning of the new directory.
    stopAnimation();
    mOffset = 0.0;
    emit scrollRequested(mOffset, 0);
    updateScrollEdges();
    loadVisibleThumbnails();
    notifyIfVisibleThumbnailsReady();
}

void ThumbnailListModel::setThumbnail(int pos, const ThumbnailEntry &entry) {
    if (!checkRange(pos))
        return;
    mRequested.remove(pos);
    mUnavailable.remove(pos);
    // The thumbnailer publishes to every view, and another view may request
    // the same path and box with the other crop.
    const bool otherCrop = entry.cropped.has_value() && *entry.cropped != mRequest.crop;
    if (entry.targetSize != targetSizeFor(pos) || otherCrop)
        return;
    if (!entry.handle.isValid()) {
        mLoaded.remove(pos);
        mUnavailable.insert(pos);
    } else {
        // The thumbnail knows the size of its image, whether it stays loaded
        // or not, but reports it as stored: the tile follows the thumbnail,
        // which is rotated by the EXIF orientation. A folder icon shows no
        // image.
        if (pos >= mDirCount) {
            setSourceSize(pos, sourceSizeInThumbnailOrientation(entry.handle.pixelSize(),
                                                                 entry.handle.sourceSize));
        }
        if (!mRequest.unloadOffscreen || preloadRange().contains(pos))
            mLoaded.insert(pos, entry);
    }
    emitRowChanged(pos, kThumbnailRoles);
    notifyIfVisibleThumbnailsReady();
}

void ThumbnailListModel::setThumbnailPending(int pos, bool pending) {
    if (!checkRange(pos))
        return;
    if (pending)
        mPendingFinal.insert(pos);
    else
        mPendingFinal.remove(pos);
    emitRowChanged(pos, {PendingRole});
    notifyIfVisibleThumbnailsReady();
}

void ThumbnailListModel::setThumbnailUnavailable(int pos, QSize targetSize) {
    if (!checkRange(pos))
        return;
    mRequested.remove(pos);
    if (targetSize != targetSizeFor(pos))
        return;
    mLoaded.remove(pos);
    mUnavailable.insert(pos);
    emitRowChanged(pos, kThumbnailRoles);
    notifyIfVisibleThumbnailsReady();
}

void ThumbnailListModel::select(QList<int> indices) {
    indices.removeIf([this](int index) { return !checkRange(index); });
    const QList<int> previous = mSelection;
    mSelection = indices;
    QSet<int> changed(previous.cbegin(), previous.cend());
    changed.unite(QSet<int>(indices.cbegin(), indices.cend()));
    for (const int index : std::as_const(changed))
        emitRowChanged(index, {SelectedRole});
    emit selectionChanged();
}

void ThumbnailListModel::select(int index) {
    select(QList<int>{checkRange(index) ? index : 0});
}

void ThumbnailListModel::clearSelection() {
    select(QList<int>());
}

// The last selected item stays selected.
void ThumbnailListModel::deselect(int index) {
    if (!checkRange(index) || mSelection.count() <= 1)
        return;
    mSelection.removeAll(index);
    emitRowChanged(index, {SelectedRole});
    emit selectionChanged();
}

void ThumbnailListModel::focusOn(int index) {
    const ItemRows &itemRows = rows();
    if (!checkRange(index) || mExtent <= 0.0 || itemRows.rowCount() == 0)
        return;
    const int row = itemRows.rowOf(index);
    const qreal start = itemRows.rowStart(row);
    const qreal extent = itemRows.rowExtent(row);
    if (mScroll.centerSelection) {
        const qreal targetCenter = start + extent / 2.0;
        // A hidden view scrolls at once: nobody sees the animation, and it
        // is shown where the selection is rather than on the way there.
        if (mScroll.smoothScroll && mActive) {
            const qreal currentCenter = mOffset + mExtent / 2.0;
            scrollSmooth(static_cast<int>(currentCenter - targetCenter), 1.0, 1.0, false);
        } else {
            scrollTo(targetCenter - mExtent / 2.0);
        }
    } else {
        // Shows part of the next thumbnail when there is room
        // (QGraphicsView::ensureVisible() with a margin).
        const qreal margin =
            mScroll.focusShowsNeighbours && mExtent > extent * 2.0 ? extent / 2.0 : 0.0;
        if (start - margin < mOffset)
            scrollTo(start - margin);
        else if (start + extent + margin > mOffset + mExtent)
            scrollTo(start + extent + margin - mExtent);
    }
    loadVisibleThumbnails();
}

void ThumbnailListModel::focusOnSelection() {
    if (!mSelection.isEmpty())
        focusOn(mSelection.constLast());
}

void ThumbnailListModel::insertItem(int index) {
    if (index < 0 || index > mCount) {
        qWarning() << "ThumbnailListModel cannot insert out-of-range index:" << index;
        return;
    }
    QList<int> newSelection = mSelection;
    for (int &selected : newSelection) {
        if (index <= selected)
            ++selected;
    }
    beginInsertRows({}, index, index);
    shiftCachedItems(index, 1);
    if (mDragHover >= index)
        ++mDragHover;
    // The selection is published again below; until then it must not name
    // rows beyond the count.
    mSelection.clear();
    ++mCount;
    refreshUniformRows();
    endInsertRows();
    emit countChanged();
    updateScrollEdges();
    select(newSelection);
    loadVisibleThumbnailsDelayed();
}

void ThumbnailListModel::removeItem(int index) {
    if (!checkRange(index)) {
        qWarning() << "ThumbnailListModel cannot remove out-of-range index:" << index;
        return;
    }
    QList<int> newSelection = mSelection;
    newSelection.removeAll(index);
    for (int &selected : newSelection) {
        if (selected >= index)
            --selected;
    }
    beginRemoveRows({}, index, index);
    mLoaded.remove(index);
    mSourceSizes.remove(index);
    mRequested.remove(index);
    mPendingFinal.remove(index);
    mUnavailable.remove(index);
    shiftCachedItems(index + 1, -1);
    if (mDragHover == index)
        mDragHover = -1;
    else if (mDragHover > index)
        --mDragHover;
    mSelection.clear();
    --mCount;
    refreshUniformRows();
    endRemoveRows();
    emit countChanged();
    updateScrollEdges();
    // The neighbour takes the selection of the removed item.
    if (newSelection.isEmpty() && mCount > 0)
        newSelection.append(index >= mCount ? mCount - 1 : index);
    select(newSelection);
    loadVisibleThumbnailsDelayed();
}

void ThumbnailListModel::reloadItem(int index) {
    if (!checkRange(index))
        return;
    const bool wasLoaded = mLoaded.contains(index);
    mUnavailable.remove(index);
    mRequested.insert(index);
    if (!wasLoaded)
        emitRowChanged(index, kThumbnailRoles);
    emit thumbnailsNeeded({ThumbnailItemRequest{index, targetSizeFor(index)}}, mRequest.crop, true);
}

void ThumbnailListModel::setDragHover(int index) {
    const int newIndex = checkRange(index) ? index : -1;
    if (newIndex == mDragHover)
        return;
    const int previous = mDragHover;
    mDragHover = newIndex;
    if (checkRange(previous))
        emitRowChanged(previous, {DragHoverRole});
    if (checkRange(newIndex))
        emitRowChanged(newIndex, {DragHoverRole});
}

void ThumbnailListModel::setDirCount(int count) {
    const int newCount = qMax(0, count);
    if (newCount == mDirCount)
        return;
    const int firstChanged = qMin(newCount, mDirCount);
    const int lastChanged = qMax(newCount, mDirCount) - 1;
    mDirCount = newCount;
    for (int i = firstChanged; i <= lastChanged; ++i)
        mRequested.remove(i);
    if (lastChanged >= 0)
        emit dataChanged(index(firstChanged), index(lastChanged), {DirectoryRole});
    emit dirCountChanged();
    loadVisibleThumbnails();
}

void ThumbnailListModel::setSourceSize(int pos, const QSize &size) {
    if (!checkRange(pos) || !size.isValid() || size.isEmpty())
        return;
    const auto known = mSourceSizes.constFind(pos);
    if (known != mSourceSizes.cend() && *known == size)
        return;
    mSourceSizes.insert(pos, size);
    emit sourceSizesChanged();
}

void ThumbnailListModel::setSourceSizes(const QList<QPair<int, QSize>> &sizes) {
    bool anyChanged = false;
    for (const auto &pair : sizes) {
        const int pos = pair.first;
        const QSize &size = pair.second;
        if (!checkRange(pos) || !size.isValid() || size.isEmpty())
            continue;
        const auto known = mSourceSizes.constFind(pos);
        if (known != mSourceSizes.cend() && *known == size)
            continue;
        mSourceSizes.insert(pos, size);
        anyChanged = true;
    }
    if (anyChanged)
        emit sourceSizesChanged();
}

bool ThumbnailListModel::hasSourceSize(int pos) const {
    const auto it = mSourceSizes.constFind(pos);
    return it != mSourceSizes.cend() && it.value().isValid() && !it.value().isEmpty();
}

QList<QSize> ThumbnailListModel::sourceSizes() const {
    QList<QSize> sizes(mCount);
    for (auto it = mSourceSizes.cbegin(); it != mSourceSizes.cend(); ++it) {
        if (it.key() >= 0 && it.key() < mCount)
            sizes[it.key()] = it.value();
    }
    return sizes;
}

bool ThumbnailListModel::imageSizesWanted() const {
    return mImageSizesWanted;
}

void ThumbnailListModel::setImageSizesWanted(bool wanted) {
    if (wanted == mImageSizesWanted)
        return;
    mImageSizesWanted = wanted;
    emit imageSizesWantedChanged();
}

bool ThumbnailListModel::hasThumbnail(int pos) const {
    return mLoaded.contains(pos);
}

//------------------------------------------------------------------------------
// Configuration

void ThumbnailListModel::configure(const ThumbnailRequestConfig &config) {
    if (config == mRequest)
        return;
    const ThumbnailRequestConfig previousRequest = mRequest;
    const bool targetSizeChanged = config.targetSize != mRequest.targetSize;
    const bool folderTargetSizeChanged =
        config.targetSizeFor(0, mDirCount) != mRequest.targetSizeFor(0, mDirCount);
    const bool itemTargetSizesChanged = config.itemTargetSizes != mRequest.itemTargetSizes;
    const bool unload = config.crop != mRequest.crop ||
                        config.cacheResolution != mRequest.cacheResolution ||
                        config.autoCacheResolution != mRequest.autoCacheResolution;
    mRequest = config;
    if (unload) {
        unloadAllThumbnails();
        loadVisibleThumbnails();
        return;
    }
    if (targetSizeChanged || folderTargetSizeChanged || itemTargetSizesChanged) {
        // Keep existing loaded thumbnails in mLoaded so they can be smoothly scaled
        // by ThumbnailItem on the GPU without visual blinking or showing placeholders.
        // In-flight requests for the obsolete target size are cleared selectively.
        auto removeMatching = [](QSet<int> &set, auto &&predicate) {
            for (auto it = set.begin(); it != set.end();) {
                if (predicate(*it))
                    it = set.erase(it);
                else
                    ++it;
            }
        };
        const auto itemTargetChanged = [this, &previousRequest](int pos) {
            return mRequest.targetSizeFor(pos, mDirCount) !=
                   previousRequest.targetSizeFor(pos, mDirCount);
        };
        removeMatching(mRequested, itemTargetChanged);
        removeMatching(mUnavailable, itemTargetChanged);
        if (mLoaded.isEmpty())
            loadVisibleThumbnails();
        else
            loadVisibleThumbnailsDelayed();
    } else {
        loadVisibleThumbnails();
    }
}

void ThumbnailListModel::setScrollConfig(const ThumbnailScrollConfig &config) {
    if (config == mScroll)
        return;
    // Rows laid out again (the sizes of the images became known) are no
    // reason to bring the selection back into view; the view keeps its place.
    ThumbnailScrollConfig withCurrentRows = config;
    withCurrentRows.rows = mScroll.rows;
    const bool onlyRowsChanged = withCurrentRows == mScroll;
    mScroll = config;
    refreshUniformRows();
    updateScrollEdges();
    if (!onlyRowsChanged)
        focusOnSelection();
    if (!mLoadTimer.isActive())
        loadVisibleThumbnails();
}

void ThumbnailListModel::setLoadingBlocked(bool blocked) {
    if (mLoadingBlocked == blocked)
        return;
    mLoadingBlocked = blocked;
    if (!blocked)
        loadVisibleThumbnails();
}

void ThumbnailListModel::setInactivePrefetch(bool enabled) {
    if (mInactivePrefetch == enabled)
        return;
    mInactivePrefetch = enabled;
    if (enabled)
        scheduleInactivePrefetch();
    else
        mPrefetchTimer.stop();
}

void ThumbnailListModel::setItemActivation(ItemActivation activation) {
    mActivation = activation;
}

//------------------------------------------------------------------------------
// Selection and scrolling

// ThumbnailView::keyPressEvent(): the selection held when Shift went down
// anchors the range.
void ThumbnailListModel::beginRangeSelection() {
    mRangeAnchor = mSelection;
    mRangeSelection = true;
}

void ThumbnailListModel::endRangeSelection() {
    mRangeSelection = false;
}

bool ThumbnailListModel::rangeSelectionActive() const {
    return mRangeSelection;
}

// ThumbnailView::addSelectionRange(): the items are appended in the order of
// the range, so that the last one becomes the current item.
void ThumbnailListModel::selectRangeTo(int index) {
    if (mRangeAnchor.isEmpty() || mSelection.isEmpty() || !checkRange(index))
        return;
    QList<int> range = mRangeAnchor;
    const int anchorEnd = mRangeAnchor.constLast();
    const int step = index > anchorEnd ? 1 : -1;
    for (int item = anchorEnd + step; step > 0 ? item <= index : item >= index; item += step) {
        range.removeAll(item);
        range.append(item);
    }
    select(range);
}

//------------------------------------------------------------------------------
// View

void ThumbnailListModel::setActive(bool active) {
    if (mActive == active)
        return;
    mActive = active;
    if (!active) {
        // The view is gone or hidden: an animation it ran will not report.
        mAnimating = false;
        mPressIndex = -1;
        mGestureActive = false;
        // A view scrolled away from the selection (wheel, right-button
        // gesture) returns to it while hidden, so the thumbnails there are
        // loaded again before it is shown, as ThumbnailStripProxy showed the
        // strip focused on the selection.
        focusOnSelection();
        return;
    }
    focusOnSelection();
    loadVisibleThumbnails();
}

void ThumbnailListModel::setViewport(qreal offset, qreal extent) {
    const bool offsetChanged = !qFuzzyCompare(offset + 1.0, mOffset + 1.0);
    const bool extentChanged = !qFuzzyCompare(extent + 1.0, mExtent + 1.0);
    if (!offsetChanged && !extentChanged)
        return;
    const bool firstLayout = mExtent <= 0.0 && extent > 0.0;
    mOffset = offset;
    mExtent = extent;
    updateScrollEdges();
    if (firstLayout) {
        focusOnSelection();
        loadVisibleThumbnails();
    } else if (offsetChanged) {
        loadVisibleThumbnails();
    } else {
        loadVisibleThumbnailsDelayed();
    }
}

void ThumbnailListModel::scrollAnimationFinished() {
    if (!mAnimating)
        return;
    mAnimating = false;
    loadVisibleThumbnails();
}

void ThumbnailListModel::wheelScrolled(QPoint angleDelta, QPoint pixelDelta) {
    const int pixel = pixelDelta.y();
    int angle = angleDelta.y();
    bool isWheel = true;
    if (mScroll.trackpadDetection) {
        const bool touchpadQuiet = !mLastTouchpadScroll.isValid() ||
                                   mLastTouchpadScroll.elapsed() > kTouchpadQuietMs;
        isWheel = angle != 0 && std::abs(angle) >= kWheelStepAngle &&
                  angle % kWheelAngleGranularity == 0 && touchpadQuiet;
    }

    if (isWheel) {
        angle = static_cast<int>(angle * mScroll.wheelSpeed);
        if (!mScroll.smoothScroll) {
            if (pixel != 0)
                scrollByItem(pixel);
            else if (angle != 0)
                scrollByItem(angle);
        } else if (angle != 0) {
            scrollSmooth(angle, kWheelScrollMultiplier, kScrollAcceleration, true);
        }
        return;
    }
    // A touchpad: one of the deltas may be scaled, use the larger one.
    mLastTouchpadScroll.start();
    scrollPrecise(std::abs(angle) > std::abs(pixel) ? angle : pixel);
}

void ThumbnailListModel::scrollPage(bool forward) {
    const ItemRows &itemRows = rows();
    if (mCount == 0 || mExtent <= 0.0 || itemRows.rowCount() == 0)
        return;
    const qreal start = mAnimating ? mAnimationTarget : mOffset;
    if (forward ? start >= maximumOffset() - kScrollEdgeTolerancePx
                : start <= kScrollEdgeTolerancePx)
        return;
    qreal target = 0.0;
    if (forward) {
        // The first row that ends past the view end.
        target = itemRows.rowStart(
            itemRows.rowAt(start + mExtent + kScrollEdgeTolerancePx));
        // A view shorter than a row moves on to the next row.
        if (target <= start + kScrollEdgeTolerancePx)
            target = itemRows.rowStart(itemRows.rowAt(start) + 1);
    } else {
        // The last row that starts before the view start; the first row
        // brings back the space before it.
        const qreal before = start - kScrollEdgeTolerancePx;
        int row = itemRows.rowAt(before);
        if (itemRows.rowStart(row) >= before)
            --row;
        target = row <= 0 ? 0.0 : itemRows.rowStart(row + 1) - mExtent;
        if (target >= start - kScrollEdgeTolerancePx)
            target = itemRows.rowStart(qMax(row, 0));
    }
    mScrollDirection = forward ? ScrollDirection::Forwards : ScrollDirection::Backwards;
    if (!mScroll.smoothScroll) {
        scrollTo(target);
        return;
    }
    if (mAnimating) {
        // Thumbnails are loaded between consecutive page scrolls.
        mAnimating = false;
        loadVisibleThumbnails();
    }
    animateTo(target, kPageScrollDurationMs, AnimationLoading::Deferred);
}

void ThumbnailListModel::press(int index, int button, int modifiers, QPointF position) {
    const auto mouseButton = static_cast<Qt::MouseButton>(button);
    const auto keyboardModifiers = Qt::KeyboardModifiers::fromInt(modifiers);
    mReleaseSelects = false;
    mDragOutRequested = false;
    mPressPosition = position;
    mPressIndex = checkRange(index) ? index : -1;

    if (mouseButton == Qt::RightButton) {
        mGestureActive = false;
        return;
    }
    if (mPressIndex >= 0) {
        if (mouseButton == Qt::LeftButton) {
            if (keyboardModifiers & Qt::ControlModifier) {
                if (!mSelection.contains(mPressIndex))
                    select(mSelection + QList<int>{mPressIndex});
                else
                    deselect(mPressIndex);
            } else if (keyboardModifiers & Qt::ShiftModifier) {
                // Without a range anchor (the strip never takes the keyboard
                // focus, so it has none) this does nothing.
                selectRangeTo(mPressIndex);
            } else if (mSelection.count() <= 1) {
                if (mActivation == ItemActivation::OnPress) {
                    emit activated(mPressIndex);
                    return;
                }
                select(mPressIndex);
            } else {
                mReleaseSelects = true;
            }
        }
    } else if (mouseButton == Qt::LeftButton &&
               !(keyboardModifiers & (Qt::ControlModifier | Qt::ShiftModifier))) {
        clearSelection();
    }
    if (mouseButton == Qt::BackButton)
        emit backNavigationRequested();
    else if (mouseButton == Qt::ForwardButton)
        emit forwardNavigationRequested();
}

void ThumbnailListModel::move(QPointF position, int buttons) {
    const auto mouseButtons = Qt::MouseButtons::fromInt(buttons);
    if (mouseButtons & Qt::RightButton) {
        if (!mGestureActive) {
            const qreal travel = mScroll.horizontal ? position.x() - mPressPosition.x()
                                                    : position.y() - mPressPosition.y();
            if (std::abs(travel) > kGestureThreshold) {
                mGestureActive = true;
                scrollToEdge(travel < 0);
            }
        }
        return;
    }
    if (mouseButtons != Qt::LeftButton || mSelection.isEmpty())
        return;
    if (!mDragOutRequested && QLineF(mPressPosition, position).length() >= kDragThreshold &&
        mSelection.contains(mPressIndex)) {
        // The drag runs a modal loop that swallows the release; the latch
        // keeps hover moves with a stale pressed state from starting it again.
        mDragOutRequested = true;
        emit dragOutRequested();
    }
}

void ThumbnailListModel::release(int index, int button, QPointF position) {
    const auto mouseButton = static_cast<Qt::MouseButton>(button);
    mDragOutRequested = false;
    if (mouseButton == Qt::RightButton) {
        if (!mGestureActive && checkRange(index) && !mSelection.contains(index))
            select(index);
        mGestureActive = false;
        return;
    }
    if (mReleaseSelects && QLineF(mPressPosition, position).length() < kDragThreshold &&
        checkRange(index)) {
        select(index);
    }
    mReleaseSelects = false;
}

void ThumbnailListModel::doubleClick(int index, int button) {
    if (static_cast<Qt::MouseButton>(button) == Qt::LeftButton && checkRange(index))
        emit activated(index);
}

void ThumbnailListModel::activate(int index) {
    if (checkRange(index))
        emit activated(index);
}

bool ThumbnailListModel::gestureActive() const {
    return mGestureActive;
}

//------------------------------------------------------------------------------
// Ranges and loading

bool ThumbnailListModel::checkRange(int index) const {
    return index >= 0 && index < mCount;
}

// The items of the rows that intersect start - end.
ThumbnailListModel::ItemRange ThumbnailListModel::itemRange(qreal start, qreal end) const {
    const ItemRows &itemRows = rows();
    if (mCount == 0 || itemRows.rowCount() == 0)
        return {};
    const int firstRow = itemRows.rowAt(start);
    const int endRow = itemRows.rowAt(end);
    if (firstRow < 0 || firstRow > endRow)
        return {};
    return ItemRange{itemRows.firstItem(firstRow), qMin(mCount - 1, itemRows.lastItem(endRow))};
}

ThumbnailListModel::ItemRange ThumbnailListModel::preloadRange() const {
    return itemRange(mOffset - mScroll.preloadDistance,
                     mOffset + mExtent + mScroll.preloadDistance);
}

ThumbnailListModel::ItemRange ThumbnailListModel::visibleRange() const {
    return itemRange(mOffset, mOffset + mExtent);
}

qreal ThumbnailListModel::contentExtent() const {
    return rows().contentExtent();
}

const ItemRows &ThumbnailListModel::rows() const {
    return mScroll.rows ? *mScroll.rows : mUniformRows;
}

void ThumbnailListModel::refreshUniformRows() {
    mUniformRows =
        UniformItemRows(mCount, mScroll.columns, mScroll.itemExtent, mScroll.leadingSpace);
}

QSize ThumbnailListModel::targetSizeFor(int pos) const {
    return mRequest.targetSizeFor(pos, mDirCount);
}

qreal ThumbnailListModel::maximumOffset() const {
    return qMax(0.0, contentExtent() - mExtent);
}

bool ThumbnailListModel::atStart() const {
    return mOffset <= 0.0;
}

bool ThumbnailListModel::atEnd() const {
    return mOffset >= maximumOffset();
}

void ThumbnailListModel::updateScrollEdges() {
    const bool backward = mOffset > kScrollEdgeTolerancePx;
    const bool forward = mOffset < maximumOffset() - kScrollEdgeTolerancePx;
    if (backward == mCanScrollBackward && forward == mCanScrollForward)
        return;
    mCanScrollBackward = backward;
    mCanScrollForward = forward;
    emit scrollEdgesChanged();
}

bool ThumbnailListModel::loadingAllowed() const {
    return mActive && !mLoadingBlocked &&
           !(mAnimating && mAnimationLoading == AnimationLoading::Deferred);
}

void ThumbnailListModel::loadVisibleThumbnails() {
    mLoadTimer.stop();
    if (!mActive) {
        // A hidden view does not preload; the thumbnails it keeps from other
        // views follow its range as the selection moves, and with prefetch
        // it requests what it shows first.
        unloadOffscreenThumbnails(preloadRange());
        scheduleInactivePrefetch();
        return;
    }
    mPrefetchTimer.stop();
    if (!loadingAllowed())
        return;
    const ItemRange range = preloadRange();
    requestMissingThumbnails(range);
    unloadOffscreenThumbnails(range);
    notifyIfVisibleThumbnailsReady();
}

void ThumbnailListModel::loadVisibleThumbnailsDelayed() {
    mLoadTimer.start();
}

// Requests the items of range that are not loaded, requested or
// unavailable, nearest first in the scroll direction.
void ThumbnailListModel::requestMissingThumbnails(ItemRange range) {
    if (!range.isValid())
        return;
    QList<ThumbnailItemRequest> requests;
    requests.reserve(range.last - range.first + 1);
    for (int index = range.first; index <= range.last; ++index) {
        const QSize itemTarget = targetSizeFor(index);
        const auto it = mLoaded.constFind(index);
        const bool needsTargetSize = (it != mLoaded.cend() && it->targetSize != itemTarget);
        if ((it == mLoaded.cend() || needsTargetSize) && !mRequested.contains(index) &&
            !mUnavailable.contains(index)) {
            requests.append({index, itemTarget});
        }
    }
    if (requests.isEmpty())
        return;
    if (mScrollDirection == ScrollDirection::Backwards)
        std::reverse(requests.begin(), requests.end());
    for (const ThumbnailItemRequest &req : std::as_const(requests))
        mRequested.insert(req.index);
    emit thumbnailsNeeded(requests, mRequest.crop, false);
}

// A view that was never laid out (the strip before its first creation) has
// no range to prefetch.
void ThumbnailListModel::scheduleInactivePrefetch() {
    if (mInactivePrefetch && !mActive && !mLoadingBlocked && mExtent > 0.0)
        mPrefetchTimer.start();
}

void ThumbnailListModel::prefetchInactiveThumbnails() {
    // Shown or blocked since it was scheduled: the active load takes over.
    if (!mInactivePrefetch || mActive || mLoadingBlocked)
        return;
    requestMissingThumbnails(visibleRange());
}

void ThumbnailListModel::unloadOffscreenThumbnails(ItemRange range) {
    if (!mRequest.unloadOffscreen)
        return;
    for (auto it = mLoaded.begin(); it != mLoaded.end();) {
        if (!range.contains(it.key()))
            it = mLoaded.erase(it);
        else
            ++it;
    }
}

void ThumbnailListModel::unloadAllThumbnails() {
    mLoaded.clear();
    mRequested.clear();
    mPendingFinal.clear();
    mUnavailable.clear();
    mReadyReported = false;
    emitAllRowsChanged(kThumbnailRoles);
}

bool ThumbnailListModel::visibleThumbnailsLoaded() const {
    if (!mActive)
        return false;
    const ItemRange range = visibleRange();
    if (!range.isValid())
        return mCount == 0;
    for (int index = range.first; index <= range.last; ++index) {
        if (mPendingFinal.contains(index) ||
            (!mLoaded.contains(index) && !mUnavailable.contains(index)))
            return false;
    }
    return true;
}

void ThumbnailListModel::notifyIfVisibleThumbnailsReady() {
    if (!mReadyReported && visibleThumbnailsLoaded()) {
        mReadyReported = true;
        emit visibleThumbnailsReady();
    }
}

void ThumbnailListModel::shiftCachedItems(int firstIndex, int offset) {
    if (offset == 0)
        return;
    QHash<int, ThumbnailEntry> shiftedThumbnails;
    shiftedThumbnails.reserve(mLoaded.size());
    for (auto it = mLoaded.cbegin(); it != mLoaded.cend(); ++it)
        shiftedThumbnails.insert(it.key() >= firstIndex ? it.key() + offset : it.key(), it.value());
    mLoaded.swap(shiftedThumbnails);
    QHash<int, QSize> shiftedSizes;
    shiftedSizes.reserve(mSourceSizes.size());
    for (auto it = mSourceSizes.cbegin(); it != mSourceSizes.cend(); ++it)
        shiftedSizes.insert(it.key() >= firstIndex ? it.key() + offset : it.key(), it.value());
    mSourceSizes.swap(shiftedSizes);
    mRequested = shiftedSet(mRequested, firstIndex, offset);
    mPendingFinal = shiftedSet(mPendingFinal, firstIndex, offset);
    mUnavailable = shiftedSet(mUnavailable, firstIndex, offset);
}

void ThumbnailListModel::emitRowChanged(int row, const QList<int> &roles) {
    if (checkRange(row))
        emit dataChanged(index(row), index(row), roles);
}

void ThumbnailListModel::emitAllRowsChanged(const QList<int> &roles) {
    if (mCount > 0)
        emit dataChanged(index(0), index(mCount - 1), roles);
}

//------------------------------------------------------------------------------
// Scrolling (ThumbnailView::scroll*)

// At once; the view reports the offset back, which changes nothing then.
void ThumbnailListModel::restoreOffset(qreal offset) {
    scrollTo(offset);
}

void ThumbnailListModel::scrollTo(qreal offset) {
    stopAnimation();
    mOffset = qBound(0.0, offset, maximumOffset());
    emit scrollRequested(mOffset, 0);
    updateScrollEdges();
    loadVisibleThumbnails();
}

// With deferred loading nothing is requested until the view reports the end
// of the animation; otherwise each offset it reports loads its range.
void ThumbnailListModel::animateTo(qreal offset, int durationMs, AnimationLoading loading) {
    mAnimating = true;
    mAnimationLoading = loading;
    mAnimationTarget = qBound(0.0, offset, maximumOffset());
    mAnimationClock.start();
    emit scrollRequested(mAnimationTarget, durationMs);
}

void ThumbnailListModel::stopAnimation() {
    mAnimating = false;
}

void ThumbnailListModel::scrollSmooth(int delta, double multiplier, double acceleration,
                                      bool additive) {
    mScrollDirection = delta < 0 ? ScrollDirection::Forwards : ScrollDirection::Backwards;
    if ((delta > 0 && atStart()) || (delta < 0 && atEnd()))
        return;
    const qreal start = mOffset;
    qreal target = start - delta * multiplier;
    // A step against the running scroll starts over from here.
    const bool redirect = (target < start && start < mAnimationTarget) ||
                          (target > start && start > mAnimationTarget);
    bool accelerate = false;
    if (mAnimating) {
        accelerate = mAnimationClock.elapsed() < kAccelerationThresholdMs;
        if (!redirect && additive)
            target = mAnimationTarget - delta * multiplier * acceleration;
        // Thumbnails are loaded between consecutive scroll animations.
        mAnimating = false;
        loadVisibleThumbnails();
    }
    const int duration = accelerate ? static_cast<int>(kScrollDurationMs / kScrollAcceleration)
                                    : kScrollDurationMs;
    animateTo(target, duration, AnimationLoading::Deferred);
}

void ThumbnailListModel::scrollPrecise(int delta) {
    mScrollDirection = delta < 0 ? ScrollDirection::Forwards : ScrollDirection::Backwards;
    stopAnimation();
    if ((delta > 0 && atStart()) || (delta < 0 && atEnd()))
        return;
    scrollTo(mOffset - delta);
}

// Explorer-like: scrolls to the next item that is not fully visible.
void ThumbnailListModel::scrollByItem(int delta) {
    const qreal minimumScroll = qMin(mScroll.thumbnailSize / 2, kMaximumMinimumScrollPx);
    const ItemRange range =
        itemRange(mOffset - minimumScroll, mOffset + mExtent + minimumScroll);
    if (!range.isValid())
        return;
    scrollToItem(delta > 0 ? range.first - 1 : range.last + 1);
}

void ThumbnailListModel::scrollToItem(int index) {
    if (!checkRange(index))
        return;
    const ItemRows &itemRows = rows();
    if (itemRows.rowCount() == 0)
        return;
    const int row = itemRows.rowOf(index);
    const qreal start = itemRows.rowStart(row);
    const qreal end = start + itemRows.rowExtent(row);
    if (start >= mOffset && end <= mOffset + mExtent)
        return;
    const int delta = static_cast<int>(start >= mOffset ? (mOffset + mExtent) - end
                                                        : mOffset - start);
    if (mScroll.smoothScroll)
        scrollSmooth(delta, 1.0, 1.0, false);
    else
        scrollPrecise(delta);
}

void ThumbnailListModel::scrollToEdge(bool end) {
    const qreal center = mOffset + mExtent / 2.0;
    const qreal targetCenter = end ? contentExtent() : 0.0;
    const qreal distance = std::abs(targetCenter - center);
    if (distance <= 0.0)
        return;
    const int duration = qBound(
        kEdgeScrollMinimumMs,
        static_cast<int>(kEdgeScrollBaseMs + std::sqrt(distance) * kEdgeScrollDistanceFactor),
        kEdgeScrollMaximumMs);
    animateTo(targetCenter - mExtent / 2.0, duration, AnimationLoading::Continuous);
}
