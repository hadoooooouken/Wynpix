#pragma once

#include <QAbstractListModel>
#include <QElapsedTimer>
#include <QHash>
#include <QList>
#include <QPoint>
#include <QPointF>
#include <QSet>
#include <QSize>
#include <QString>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

#include <optional>

#include "gui/quick/render/thumbnailitem.h"

// A thumbnail and its labels, as the thumbnailer made it.
struct ThumbnailEntry {
    ThumbnailHandle handle;
    QString name;
    QString info;
    // The crop the thumbnailer made it for; std::nullopt serves either crop
    // (a folder icon).
    std::optional<bool> cropped;
    // The target box this thumbnail was generated for.
    QSize targetSize;
};

// How thumbnails are requested: the target box in device pixels (the image
// area of a cell times the device pixel ratio), square crop, and whether
// thumbnails outside the preloaded range are dropped. Changing the crop or
// the thumbnail cache resolution unloads every thumbnail; changing the box
// keeps existing thumbnails while fetching the new resolution.
struct ThumbnailRequestConfig {
    QSize targetSize;
    bool crop = false;
    bool unloadOffscreen = false;
    // The thumbnailResolution setting (size of the cached thumbnails).
    int cacheResolution = 0;

    friend bool operator==(const ThumbnailRequestConfig &, const ThumbnailRequestConfig &) = default;
};

// How the view lays out and scrolls. Items fill rows of columns items each
// (1 for the strip); rows follow each other along the scroll axis after
// leadingSpace.
struct ThumbnailScrollConfig {
    // Preload distance of the thumbnail strip.
    static constexpr int kStripPreloadDistance = 3000;

    bool horizontal = true;
    // Size of a row along the scroll axis, in logical pixels.
    int itemExtent = 0;
    int columns = 1;
    int leadingSpace = 0;
    // Distance beyond the visible range in which thumbnails are loaded.
    int preloadDistance = kStripPreloadDistance;
    // Focusing an item keeps half a row of its neighbours in view when the
    // view has room for them (the strip); otherwise the item is only brought
    // into view (the folder grid).
    bool focusShowsNeighbours = true;
    // Edge of the thumbnail area (the minimum by-item wheel step derives
    // from it).
    int thumbnailSize = 0;
    bool centerSelection = false;
    bool smoothScroll = false;
    double wheelSpeed = 1.0;
    bool trackpadDetection = false;

    friend bool operator==(const ThumbnailScrollConfig &, const ThumbnailScrollConfig &) = default;
};

// Items of a thumbnail view (the thumbnail strip, the folder grid) with the
// behaviour of the widget ThumbnailView, in C++ so that QML only lays out and
// animates:
// - item state: thumbnail and labels, directory, requested / final pending,
//   unavailable, selected, drag hover;
// - visible range: the view reports its scroll offset and extent
//   (setViewport()); thumbnails within the preload distance of it that are not
//   loaded, requested or unavailable are requested in one batch, nearest
//   first in the scroll direction (thumbnailsNeeded); with unloadOffscreen,
//   thumbnails outside that range are dropped. The preload range is not
//   requested while the view is inactive, loading is blocked (panel
//   animation) or a smooth (wheel) scroll animation runs; the right-button
//   gesture to an end loads along the way. While the view is inactive it
//   keeps the results of its size and crop other views requested within
//   that range, and drops those outside it when it scrolls (the selection
//   moved).
//   With inactive prefetch (the hidden strip) an inactive, laid out view
//   also requests the missing thumbnails of the range it shows first, once
//   the change that moved it is over: the other views publish only what
//   they request themselves, and nothing is requested while the view slides
//   in, so it would show placeholders for thumbnails already cached;
// - visibleThumbnailsReady once per population when every thumbnail in the
//   visible range has arrived or is unavailable, and none waits for its
//   final image;
// - scrolling: focusing an item (centred or kept in view), mouse wheel
//   (smooth with acceleration, by item, touchpad pixels), the right-button
//   gesture to the ends; the model sends the target to the view
//   (scrollRequested) and the view reports when an animation ended. An
//   inactive view focuses without animation, and a view returns to the
//   selection when it becomes inactive, so it is shown there with what it
//   prefetched rather than where the user scrolled before it was hidden;
// - pointer input: activation on press (the strip) or selection on press
//   and activation by double click (the folder grid), Ctrl toggles, Shift
//   extends a range from the selection held when the range began, the
//   release selects within a multi-selection, drag out, back / forward
//   buttons.
//
// The signals carry the IDirectoryView requests under their own names; the
// application's DirectoryViewAdapter forwards them. GUI thread only.
class ThumbnailListModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by the Quick UI host")
    Q_PROPERTY(int count READ itemCount NOTIFY countChanged FINAL)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY selectionChanged FINAL)

public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        InfoRole,
        DirectoryRole,
        ThumbnailRole,
        LoadedRole,
        PendingRole,
        UnavailableRole,
        SelectedRole,
        DragHoverRole,
    };
    Q_ENUM(Role)

    // What a left-button press on an item does without modifiers when at
    // most one item is selected.
    enum class ItemActivation { OnPress, OnDoubleClick };

    // Delay of the load after items were inserted, removed or resized.
    static constexpr int kLoadDelayMs = 150;
    // Pointer travel that starts a drag out, and the right-button gesture.
    static constexpr int kDragThreshold = 40;
    static constexpr int kGestureThreshold = 40;
    // Smooth wheel scrolling: duration, distance per wheel delta unit, the
    // acceleration of a step that adds to a running scroll, and the time
    // into a running scroll within which a new step is shortened.
    static constexpr int kScrollDurationMs = 120;
    static constexpr double kWheelScrollMultiplier = 2.5;
    static constexpr double kScrollAcceleration = 1.4;
    static constexpr int kAccelerationThresholdMs = 50;
    // Scroll to an end: kEdgeScrollBaseMs + kEdgeScrollDistanceFactor *
    // sqrt(distance), within kEdgeScrollMinimumMs - kEdgeScrollMaximumMs.
    static constexpr int kEdgeScrollBaseMs = 250;
    static constexpr double kEdgeScrollDistanceFactor = 5.0;
    static constexpr int kEdgeScrollMinimumMs = 300;
    static constexpr int kEdgeScrollMaximumMs = 1500;
    // The by-item wheel step moves at least half a thumbnail, at most this.
    static constexpr int kMaximumMinimumScrollPx = 100;
    // Touchpad detection: a wheel event is a mouse wheel when its angle is a
    // multiple of kWheelAngleGranularity, at least kWheelStepAngle, and no
    // touchpad scroll happened within kTouchpadQuietMs.
    static constexpr int kWheelStepAngle = 120;
    static constexpr int kWheelAngleGranularity = 60;
    static constexpr int kTouchpadQuietMs = 250;
    // The inactive prefetch waits for the next event loop pass: a population
    // and the focus that follows it request once, after the requests the
    // shown views made in the same pass.
    static constexpr int kInactivePrefetchDelayMs = 0;

    explicit ThumbnailListModel(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] int itemCount() const;
    // Number of leading items that are directories.
    [[nodiscard]] int dirCount() const;
    // The last selected item, -1 without a selection.
    [[nodiscard]] int currentIndex() const;
    [[nodiscard]] QList<int> selection() const;
    [[nodiscard]] ThumbnailRequestConfig requestConfig() const;
    [[nodiscard]] ThumbnailScrollConfig scrollConfig() const;
    // Scroll offset of the view as last reported or requested.
    [[nodiscard]] qreal viewOffset() const;

    // --- directory view (IDirectoryView) ---------------------------------
    void populate(int count);
    // Ignored unless entry.targetSize is the requested target box (the box
    // the request was made for, whatever the pixel size of the entry is),
    // the entry is made for the requested crop (or either) and pos is an item.
    void setThumbnail(int pos, const ThumbnailEntry &entry);
    void setThumbnailPending(int pos, bool pending);
    void setThumbnailUnavailable(int pos, QSize targetSize);
    // Items out of range are dropped.
    void select(QList<int> indices);
    // An item out of range selects the first one.
    void select(int index);
    void focusOn(int index);
    void focusOnSelection();
    void insertItem(int index);
    void removeItem(int index);
    void reloadItem(int index);
    void setDragHover(int index);
    // The first count items are directories.
    void setDirCount(int count);
    [[nodiscard]] bool hasThumbnail(int pos) const;

    // --- configuration (ThumbnailPanelController) ------------------------
    void configure(const ThumbnailRequestConfig &config);
    void setScrollConfig(const ThumbnailScrollConfig &config);
    // While blocked (the panel slides), nothing is requested.
    void setLoadingBlocked(bool blocked);
    // Whether the inactive view requests what it shows first (the hidden
    // strip while the panel can be shown).
    void setInactivePrefetch(bool enabled);
    void setItemActivation(ItemActivation activation);

    // --- selection and scrolling (keyboard navigation) ---------------------
    // A Shift range starts from the current selection and ends with
    // endRangeSelection(); Shift-presses and selectRangeTo() extend it.
    void beginRangeSelection();
    void endRangeSelection();
    [[nodiscard]] bool rangeSelectionActive() const;
    // Selects the range anchor plus the items from the end of the anchor to
    // index; nothing happens without an anchor or a selection.
    void selectRangeTo(int index);
    // Scrolls until the item is fully visible (animated with smooth
    // scrolling).
    void scrollToItem(int index);
    // Activates the item (Enter in the folder grid).
    void activate(int index);
    // The right button moved far enough since its press to scroll to an end.
    [[nodiscard]] bool gestureActive() const;

    // --- view (QML) ------------------------------------------------------
    // The view is shown and laid out.
    Q_INVOKABLE void setActive(bool active);
    // Scroll offset and extent of the view along the strip.
    Q_INVOKABLE void setViewport(qreal offset, qreal extent);
    // The scroll animation of the last scrollRequested() ended.
    Q_INVOKABLE void scrollAnimationFinished();
    // A wheel event over the view (QWheelEvent deltas).
    Q_INVOKABLE void wheelScrolled(QPoint angleDelta, QPoint pixelDelta);
    // Pointer events of the view: index is the item under the pointer (-1
    // for none), button a Qt::MouseButton, buttons Qt::MouseButtons,
    // modifiers Qt::KeyboardModifiers, position in view coordinates.
    Q_INVOKABLE void press(int index, int button, int modifiers, QPointF position);
    Q_INVOKABLE void move(QPointF position, int buttons);
    Q_INVOKABLE void release(int index, int button, QPointF position);
    Q_INVOKABLE void doubleClick(int index, int button);

signals:
    void countChanged();
    void dirCountChanged();
    void selectionChanged();
    // Requests the thumbnails of indices for the target box (device pixels).
    void thumbnailsNeeded(const QList<int> &indices, QSize targetSize, bool crop, bool force);
    void activated(int index);
    void dragOutRequested();
    void backNavigationRequested();
    void forwardNavigationRequested();
    void visibleThumbnailsReady();
    // The view scrolls to offset, animated over durationMs (0: at once).
    void scrollRequested(qreal offset, int durationMs);

private:
    enum class ScrollDirection { Forwards, Backwards };
    // Whether thumbnails are requested while a scroll animation runs: smooth
    // wheel steps wait for its end, the scroll to an end loads along the way
    // (ThumbnailView blocked loading only in scrollSmooth()).
    enum class AnimationLoading { Deferred, Continuous };
    struct ItemRange {
        int first = -1;
        int last = -1;
        [[nodiscard]] bool isValid() const { return first >= 0 && first <= last; }
        [[nodiscard]] bool contains(int index) const {
            return isValid() && index >= first && index <= last;
        }
    };

    [[nodiscard]] bool checkRange(int index) const;
    [[nodiscard]] ItemRange itemRange(qreal start, qreal end) const;
    [[nodiscard]] ItemRange preloadRange() const;
    [[nodiscard]] ItemRange visibleRange() const;
    [[nodiscard]] qreal contentExtent() const;
    [[nodiscard]] qreal maximumOffset() const;
    [[nodiscard]] bool atStart() const;
    [[nodiscard]] bool atEnd() const;
    [[nodiscard]] bool loadingAllowed() const;

    void loadVisibleThumbnails();
    void loadVisibleThumbnailsDelayed();
    void requestMissingThumbnails(ItemRange range);
    void scheduleInactivePrefetch();
    void prefetchInactiveThumbnails();
    void unloadAllThumbnails();
    void unloadOffscreenThumbnails(ItemRange range);
    [[nodiscard]] bool visibleThumbnailsLoaded() const;
    void notifyIfVisibleThumbnailsReady();
    void shiftCachedItems(int firstIndex, int offset);
    void clearSelection();
    void deselect(int index);
    void emitRowChanged(int index, const QList<int> &roles);
    void emitAllRowsChanged(const QList<int> &roles);

    void scrollTo(qreal offset);
    void animateTo(qreal offset, int durationMs, AnimationLoading loading);
    void stopAnimation();
    void scrollSmooth(int delta, double multiplier, double acceleration, bool additive);
    void scrollPrecise(int delta);
    void scrollByItem(int delta);
    void scrollToEdge(bool end);
    [[nodiscard]] qreal itemStart(int index) const;
    [[nodiscard]] int lineCount() const;

    int mCount = 0;
    int mDirCount = 0;
    QList<int> mSelection;
    int mDragHover = -1;
    QHash<int, ThumbnailEntry> mLoaded;
    QSet<int> mRequested;
    QSet<int> mPendingFinal;
    QSet<int> mUnavailable;
    bool mReadyReported = false;

    ThumbnailRequestConfig mRequest;
    ThumbnailScrollConfig mScroll;
    ItemActivation mActivation = ItemActivation::OnPress;
    QList<int> mRangeAnchor;
    bool mRangeSelection = false;
    bool mActive = false;
    bool mLoadingBlocked = false;
    bool mInactivePrefetch = false;
    QTimer mLoadTimer;
    QTimer mPrefetchTimer;

    qreal mOffset = 0.0;
    qreal mExtent = 0.0;
    ScrollDirection mScrollDirection = ScrollDirection::Forwards;
    bool mAnimating = false;
    AnimationLoading mAnimationLoading = AnimationLoading::Deferred;
    qreal mAnimationTarget = 0.0;
    QElapsedTimer mAnimationClock;
    QElapsedTimer mLastTouchpadScroll;

    QPointF mPressPosition;
    int mPressIndex = -1;
    bool mReleaseSelects = false;
    bool mGestureActive = false;
    // The file drag of the current press started; it starts once per press.
    bool mDragOutRequested = false;
};
