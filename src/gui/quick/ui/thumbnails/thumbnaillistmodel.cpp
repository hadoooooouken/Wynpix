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

//------------------------------------------------------------------------------
// Directory view

void ThumbnailListModel::populate(int count) {
    if (count < 0) {
        qWarning() << "ThumbnailListModel received a negative item count:" << count;
        return;
    }
    beginResetModel();
    mCount = count;
    mSelection.clear();
    mDragHover = -1;
    mLoaded.clear();
    mRequested.clear();
    mPendingFinal.clear();
    mUnavailable.clear();
    mReadyReported = false;
    mScrollDirection = ScrollDirection::Forwards;
    mRangeAnchor.clear();
    endResetModel();
    emit countChanged();
    emit selectionChanged();
    // The view starts at the beginning of the new directory.
    stopAnimation();
    mOffset = 0.0;
    emit scrollRequested(mOffset, 0);
    loadVisibleThumbnails();
    notifyIfVisibleThumbnailsReady();
}

void ThumbnailListModel::setThumbnail(int pos, const ThumbnailEntry &entry, QSize targetSize) {
    // The thumbnailer publishes to every view, and another view may request
    // the same path and box with the other crop.
    const bool otherCrop = entry.cropped.has_value() && *entry.cropped != mRequest.crop;
    if (targetSize != mRequest.targetSize || otherCrop || !checkRange(pos))
        return;
    mRequested.remove(pos);
    mUnavailable.remove(pos);
    // Without unloading every thumbnail is kept; otherwise only those the
    // view can reach soon. An inactive (hidden) view keeps them as well:
    // they are what its prefetch requested, or what another view requested
    // within its range.
    if (!mRequest.unloadOffscreen || preloadRange().contains(pos))
        mLoaded.insert(pos, entry);
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
    if (targetSize != mRequest.targetSize || !checkRange(pos))
        return;
    mRequested.remove(pos);
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
    if (!checkRange(index) || mExtent <= 0.0 || mScroll.itemExtent <= 0)
        return;
    const qreal start = itemStart(index);
    const qreal extent = mScroll.itemExtent;
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
    endInsertRows();
    emit countChanged();
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
    endRemoveRows();
    emit countChanged();
    // The neighbour takes the selection of the removed item.
    if (newSelection.isEmpty() && mCount > 0)
        newSelection.append(index >= mCount ? mCount - 1 : index);
    select(newSelection);
    loadVisibleThumbnailsDelayed();
}

void ThumbnailListModel::reloadItem(int index) {
    if (!checkRange(index))
        return;
    mLoaded.remove(index);
    mUnavailable.remove(index);
    mRequested.insert(index);
    emitRowChanged(index, kThumbnailRoles);
    emit thumbnailsNeeded({index}, mRequest.targetSize, mRequest.crop, true);
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
    const int last = qMin(qMax(newCount, mDirCount), mCount) - 1;
    mDirCount = newCount;
    if (last >= 0)
        emit dataChanged(index(0), index(last), {DirectoryRole});
    emit dirCountChanged();
}

//------------------------------------------------------------------------------
// Configuration

void ThumbnailListModel::configure(const ThumbnailRequestConfig &config) {
    if (config == mRequest)
        return;
    const bool unload = config.targetSize != mRequest.targetSize ||
                        config.crop != mRequest.crop ||
                        config.cacheResolution != mRequest.cacheResolution;
    mRequest = config;
    if (unload)
        unloadAllThumbnails();
    loadVisibleThumbnails();
}

void ThumbnailListModel::setScrollConfig(const ThumbnailScrollConfig &config) {
    if (config == mScroll)
        return;
    mScroll = config;
    focusOnSelection();
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
    if (mCount == 0 || mScroll.itemExtent <= 0)
        return {};
    const int lastLine = lineCount() - 1;
    const auto lineAt = [this, lastLine](qreal position) {
        const qreal line = std::floor((position - mScroll.leadingSpace) / mScroll.itemExtent);
        return static_cast<int>(qBound(0.0, line, static_cast<qreal>(lastLine)));
    };
    const int firstLine = lineAt(start);
    const int endLine = lineAt(end);
    if (firstLine > endLine)
        return {};
    const int columns = qMax(1, mScroll.columns);
    return ItemRange{firstLine * columns, qMin(mCount - 1, (endLine + 1) * columns - 1)};
}

ThumbnailListModel::ItemRange ThumbnailListModel::preloadRange() const {
    return itemRange(mOffset - mScroll.preloadDistance,
                     mOffset + mExtent + mScroll.preloadDistance);
}

ThumbnailListModel::ItemRange ThumbnailListModel::visibleRange() const {
    return itemRange(mOffset, mOffset + mExtent);
}

qreal ThumbnailListModel::contentExtent() const {
    return mScroll.leadingSpace + static_cast<qreal>(lineCount()) * mScroll.itemExtent;
}

int ThumbnailListModel::lineCount() const {
    const int columns = qMax(1, mScroll.columns);
    return (mCount + columns - 1) / columns;
}

qreal ThumbnailListModel::itemStart(int index) const {
    return mScroll.leadingSpace +
           static_cast<qreal>(index / qMax(1, mScroll.columns)) * mScroll.itemExtent;
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
    QList<int> loadList;
    for (int index = range.first; index <= range.last; ++index) {
        if (!mLoaded.contains(index) && !mRequested.contains(index) &&
            !mUnavailable.contains(index))
            loadList.append(index);
    }
    if (loadList.isEmpty())
        return;
    if (mScrollDirection == ScrollDirection::Backwards)
        std::reverse(loadList.begin(), loadList.end());
    for (const int index : std::as_const(loadList))
        mRequested.insert(index);
    emit thumbnailsNeeded(loadList, mRequest.targetSize, mRequest.crop, false);
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
void ThumbnailListModel::scrollTo(qreal offset) {
    stopAnimation();
    mOffset = qBound(0.0, offset, maximumOffset());
    emit scrollRequested(mOffset, 0);
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
    const qreal start = itemStart(index);
    const qreal end = start + mScroll.itemExtent;
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
