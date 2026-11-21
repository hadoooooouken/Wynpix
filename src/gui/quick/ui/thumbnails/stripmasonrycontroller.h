#pragma once

#include <QObject>
#include <QTimer>
#include <QtGlobal>
#include <QtQml/qqmlregistration.h>

#include <memory>

#include "gui/quick/ui/folderview/masonrytilesmodel.h"
#include "gui/quick/ui/thumbnails/stripmasonrylayout.h"
#include "gui/quick/ui/thumbnails/thumbnaillistmodel.h"
#include "gui/quick/ui/thumbnails/thumbnailstriplayout.h"

// The place of the view to put back when the tiles in front of it change in
// size: the tile that anchors the view, and how far its row starts before the
// start of the view.
struct StripScrollAnchor {
    int item = -1;
    qreal distance = 0.0;

    [[nodiscard]] bool isValid() const { return item >= 0; }
};

// The masonry layout of the thumbnail strip: the proportional layout of the
// tiles (StripMasonryLayout) kept up to date with the model while the strip
// is in the masonry mode. The sizes of the images arrive in a stream, so the
// layout follows them in steps, and the view stays where it is: the tile that
// anchors it (the selected one while it is in view, otherwise the first one
// in view) keeps its place; a directory that was just opened starts at its
// beginning. The strip is not shown until the sizes of the
// images around the selection are known (or a grace period has passed), so
// that it does not show a layout that is about to change.
//
// The controller does not configure the model: layoutChanged() delivers the
// anchor to ThumbnailPanelController, which gives the model the rows of
// layout() and calls restoreAnchor(). It asks the model for the sizes of the
// images only while the strip is in the masonry mode.
//
// Owned by ThumbnailPanelController. GUI thread only.
class StripMasonryController final : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by ThumbnailPanelController.masonry")
    Q_PROPERTY(bool enabled READ isEnabled NOTIFY enabledChanged FINAL)
    // The layout may be shown: false while the first sizes are awaited.
    Q_PROPERTY(bool ready READ isReady NOTIFY readyChanged FINAL)
    Q_PROPERTY(qreal contentExtent READ contentExtent NOTIFY contentExtentChanged FINAL)
    // Where the selected tile is along the strip, 0 - 1 of the content (the
    // marker of the scroll bar).
    Q_PROPERTY(qreal markerPosition READ markerPosition NOTIFY markerPositionChanged FINAL)
    Q_PROPERTY(MasonryTilesModel *tiles READ tiles CONSTANT FINAL)

public:
    // The layout follows a stream of sizes in steps of this delay; while the
    // sizes around the selection are still missing the steps are shorter.
    static constexpr int kLayoutDelayMs = 120;
    static constexpr int kVisibleLayoutDelayMs = 25;
    // The first layout waits this long for the sizes around the selection.
    static constexpr int kGracePeriodMs = 75;
    // Images around the selection whose sizes the first layout waits for.
    static constexpr int kInitialCheckCount = 24;
    // A view that is this close to its place is not moved.
    static constexpr qreal kAnchorTolerancePx = 0.5;

    // source must outlive the controller.
    explicit StripMasonryController(ThumbnailListModel &source, QObject *parent = nullptr);

    [[nodiscard]] bool isEnabled() const;
    [[nodiscard]] bool isReady() const;
    [[nodiscard]] qreal contentExtent() const;
    [[nodiscard]] qreal markerPosition() const;
    [[nodiscard]] MasonryTilesModel *tiles();
    // Null while the strip is not in the masonry mode.
    [[nodiscard]] std::shared_ptr<const StripMasonryLayout> layout() const;

    // The geometry of the strip; its masonry flag turns the layout on and
    // off, the layout is made again for any change.
    void setStripLayout(const ThumbnailStripLayout &strip);
    // Puts the view back where the anchor was, in the layout made last, and
    // keeps it inside the content: a layout that became shorter (items went,
    // images turned out smaller) must not leave the view past its end.
    void restoreAnchor(const StripScrollAnchor &anchor);

    // The item at a point of the content (logical pixels), -1 for none.
    Q_INVOKABLE int itemAt(qreal x, qreal y) const;

signals:
    void enabledChanged();
    void readyChanged();
    void contentExtentChanged();
    void markerPositionChanged();
    // The layout was made again or dropped. anchor says where the view was
    // before; the model has to follow the layout first (setScrollConfig()),
    // then the view is put back (restoreAnchor()).
    void layoutChanged(const StripScrollAnchor &anchor);

private:
    void setEnabled(bool enabled);
    void setReady(bool ready);
    void rebuild();
    [[nodiscard]] StripScrollAnchor captureAnchor() const;
    // The sizes of the images around the selection (the first layout) and of
    // those in view (the layout that follows them) are known.
    [[nodiscard]] bool initialSizesKnown() const;
    [[nodiscard]] bool visibleSizesKnown() const;
    void resetReadiness();
    void scheduleLayout();
    void scheduleFastLayout();
    void onModelReset();
    // Items came or went: the layout follows unless it already has them.
    void onStructureChanged();
    void onSourceSizesChanged();
    void onGraceTimeout();

    ThumbnailListModel &mSource;
    MasonryTilesModel mTiles;
    ThumbnailStripLayout mStrip;
    std::shared_ptr<const StripMasonryLayout> mLayout;
    // Folders of the items the layout was made for.
    int mLayoutDirCount = 0;
    QTimer mLayoutTimer;
    QTimer mGraceTimer;
    bool mEnabled = false;
    bool mReady = true;
    // The first sizes after the directory was populated are awaited.
    bool mInitialBatchPending = false;
    // The model was just populated: its view starts at the beginning, so the
    // place it had in the directory before is not kept (until the signals of
    // the population are through).
    bool mAnchorSuspended = false;
};
