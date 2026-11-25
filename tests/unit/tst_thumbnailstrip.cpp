#include <QColor>
#include <QImage>
#include <QSignalSpy>
#include <QSize>
#include <QTest>

#include <memory>

#include "gui/quick/adapters/directoryviewadapter.h"
#include "gui/quick/render/thumbnailitem.h"
#include "gui/quick/ui/thumbnails/thumbnaillistmodel.h"
#include "gui/quick/ui/thumbnails/thumbnailpanelcontroller.h"
#include "gui/quick/ui/thumbnails/thumbnailstriplayout.h"
#include "sourcecontainers/thumbnail.h"
#include "testsuites.h"

namespace {
using namespace Qt::StringLiterals;
namespace Enums = SettingsEnums;

// A 4:3 target box in device pixels, like the image area of a cell.
constexpr QSize kRequestSize(100, 75);
constexpr int kItemExtent = 100;
constexpr int kThumbnailSize = 80;
constexpr qreal kViewExtent = 500.0;
// The rows that intersect 0 - kViewExtent, both ends included.
constexpr int kVisibleItems = 6;
constexpr int kTextHeight = 15;
// A hide delay short enough to wait for.
constexpr int kShortHideDelayMs = 10;
constexpr int kWaitMarginMs = 100;
constexpr int kTryTimeoutMs = 2000;
// The default preview size: a 16:9 image is 256 wide in the masonry strip.
constexpr int kDefaultPreviewSize = 256;
constexpr QSize kLargeWindow(1200, 800);
constexpr QSize kSmallWindow(600, 400);
constexpr QSize kSampleImage(4, 3);
constexpr QSize kWideImage(1920, 1080);
constexpr QSize kClassicImage(1600, 1200);
constexpr QSize kSquareImage(1000, 1000);

// A width x height thumbnail, the answer to a request for targetSize.
ThumbnailEntry entryOf(int width, int height, QSize targetSize = kRequestSize) {
  QImage image(width, height, QImage::Format_ARGB32_Premultiplied);
  image.fill(Qt::red);
  return {.handle = {.image = image, .sourceSize = image.size()},
          .name = u"name"_s,
          .info = u"info"_s,
          .targetSize = targetSize};
}

ThumbnailEntry sampleEntry(QSize targetSize = kRequestSize) {
  return entryOf(kSampleImage.width(), kSampleImage.height(), targetSize);
}

// Extracts requested item indices from a thumbnailsNeeded signal emission.
QList<int> requestedIndices(const QList<QVariant> &args) {
  const auto requests = args.at(0).value<QList<ThumbnailItemRequest>>();
  QList<int> indices;
  indices.reserve(requests.size());
  for (const ThumbnailItemRequest &req : requests)
    indices.append(req.index);
  return indices;
}

// A model of count items, configured with kRequestSize thumbnails and
// kItemExtent cells, active over a view of kViewExtent at offset 0.
void prepare(ThumbnailListModel &model, int count, bool active = true) {
  model.configure({.targetSize = kRequestSize});
  model.setScrollConfig({.itemExtent = kItemExtent, .thumbnailSize = kThumbnailSize});
  model.populate(count);
  model.setViewport(0.0, kViewExtent);
  if (active)
    model.setActive(true);
}

bool isLoaded(const ThumbnailListModel &model, int row) {
  return model.data(model.index(row), ThumbnailListModel::LoadedRole).toBool();
}

UiSettingsSnapshot panelSettings(bool pinned, Enums::PanelPosition position) {
  UiSettingsSnapshot settings;
  settings.panel.enabled = true;
  settings.panel.pinned = pinned;
  settings.panel.position = position;
  settings.panel.previewsSize = kThumbnailSize;
  settings.panel.hideDelayMs = kShortHideDelayMs;
  settings.viewer.mouseScrollingSpeed = 1.0;
  return settings;
}

UiSettingsSnapshot masonrySettings() {
  UiSettingsSnapshot settings = panelSettings(false, Enums::PanelPosition::Bottom);
  settings.panel.previewsSize = kDefaultPreviewSize;
  settings.panel.masonry = true;
  return settings;
}

// Window coordinates in the area of a bottom panel and above it.
QPointF bottomPanelPoint() {
  return {kLargeWindow.width() / 2.0, kLargeWindow.height() - 2.0};
}
QPointF viewerPoint() {
  return {kLargeWindow.width() / 2.0, kLargeWindow.height() / 2.0};
}
} // namespace

class ThumbnailStripTests : public QObject {
  Q_OBJECT

private slots:
  //--- layout ---------------------------------------------------------------
  void layoutOfHorizontalSimpleStrip() {
    const ThumbnailStripLayout layout = thumbnailStripLayoutFor(
        {.previewsSize = 120,
         .style = Enums::PanelStyle::Simple,
         .position = Enums::PanelPosition::Bottom,
         .textHeight = kTextHeight});
    QVERIFY(layout.horizontal);
    QCOMPARE(layout.thumbnailSize, 120);
    // 120 + (9 + 2) * 2, 90 + (9 + 4) * 2 (ThumbnailWidget::updateBoundingRect).
    QCOMPARE(layout.cellWidth, 142);
    QCOMPARE(layout.cellHeight, 116);
    QCOMPARE(layout.itemExtent(), 142);
    QCOMPARE(layout.imageArea, QSize(120, 90));
    QCOMPARE(layout.imageCenterOffset, 0);
    QVERIFY(!layout.labels);
    // MainPanel::sizeHint(): + 16, + 3 at the bottom.
    QCOMPARE(layout.panelExtent, 135);
    const ThumbnailStripLayout top = thumbnailStripLayoutFor(
        {.previewsSize = 120, .position = Enums::PanelPosition::Top});
    QCOMPARE(top.panelExtent, 132);
  }

  void layoutOfVerticalExtendedStrip() {
    const ThumbnailStripLayout layout = thumbnailStripLayoutFor(
        {.previewsSize = 100,
         .style = Enums::PanelStyle::Extended,
         .position = Enums::PanelPosition::Left,
         .textHeight = kTextHeight});
    QVERIFY(!layout.horizontal);
    // 100 + (9 + 12) * 2; 75 + (9 + 2) * 2 + 9 + 2 * 15.
    QCOMPARE(layout.cellWidth, 142);
    QCOMPARE(layout.cellHeight, 136);
    QCOMPARE(layout.itemExtent(), 136);
    QCOMPARE(layout.panelExtent, 158);
    QVERIFY(layout.labels);
    QCOMPARE(layout.imageCenterOffset, -kTextHeight);
    QCOMPARE(layout.labelLeft, 21);
    QCOMPARE(layout.labelTop, 95);
    QCOMPARE(layout.infoTop, 112);
    QCOMPARE(layout.labelWidth, 100);
  }

  void layoutBoundsThePreviewSize() {
    QCOMPARE(thumbnailStripLayoutFor({.previewsSize = 5}).thumbnailSize,
             ThumbnailStripLayout::kMinimumPreviewSize);
    QCOMPARE(thumbnailStripLayoutFor(
                 {.previewsSize = ThumbnailStripLayout::kMaximumPreviewSize + 100})
                 .thumbnailSize,
             ThumbnailStripLayout::kMaximumPreviewSize);
  }

  void layoutOfMasonryStripHasNoLabelsAndKeepsTheButtonsInReach() {
    const ThumbnailStripLayout bottom = thumbnailStripLayoutFor(
        {.previewsSize = 256,
         .style = Enums::PanelStyle::Extended,
         .position = Enums::PanelPosition::Bottom,
         .textHeight = kTextHeight,
         .masonry = true});
    QVERIFY(bottom.masonry);
    QVERIFY(!bottom.labels);
    // A 16:9 image is as wide as the preview size: 256 * 9 / 16 high.
    QCOMPARE(bottom.laneExtent, 144);
    // The tiles ask for 144 + 2 * 4 + 16 + 3 = 171, the buttons for
    // (4 + 2) * 30 + 4 + the 3 px strip = 187.
    QCOMPARE(bottom.panelExtent, 187);
    // The lane is centred in the panel without its strip.
    QCOMPARE(bottom.laneOffset, 20);
    const ThumbnailStripLayout top = thumbnailStripLayoutFor(
        {.previewsSize = 256, .position = Enums::PanelPosition::Top, .masonry = true});
    // The border line is 1 px: (4 + 2) * 30 + 4 + 1.
    QCOMPARE(top.panelExtent, 185);
    QCOMPARE(top.laneOffset, 20);
  }

  void masonryStripIsAsThickAsItsTilesWhenTheyNeedMore() {
    const ThumbnailStripLayout large = thumbnailStripLayoutFor(
        {.previewsSize = 300, .position = Enums::PanelPosition::Bottom, .masonry = true});
    QCOMPARE(large.laneExtent, 169);
    QCOMPARE(large.panelExtent, 169 + 2 * ThumbnailStripLayout::kMasonryCrossMargin +
                                    ThumbnailStripLayout::kPanelAllowance +
                                    ThumbnailStripLayout::kBottomPanelStrip);
    const ThumbnailStripLayout small = thumbnailStripLayoutFor(
        {.previewsSize = 20, .position = Enums::PanelPosition::Bottom, .masonry = true});
    // Below minimum settings size, button allowance is clamped to fixed buttons (4 * 30 + 4 + 3 = 127).
    QCOMPARE(small.panelExtent, 127);
    const ThumbnailStripLayout minSettings = thumbnailStripLayoutFor(
        {.previewsSize = ThumbnailStripLayout::kMasonryMinButtonsSize,
         .position = Enums::PanelPosition::Bottom,
         .masonry = true});
    QCOMPARE(minSettings.panelExtent, 127);
    // Midpoint interpolation between min (104 -> 127) and full (200 -> 187):
    // previewsSize = 152 gives t = 0.5, buttonsExtent = 127 + 30 = 157.
    // tilesExtent is 86 + 27 = 113, so buttonsExtent dominates.
    const ThumbnailStripLayout midpoint = thumbnailStripLayoutFor(
        {.previewsSize = 152, .position = Enums::PanelPosition::Bottom, .masonry = true});
    QCOMPARE(midpoint.panelExtent, 157);
    // Upper bound of button scaling: at previewsSize = 200, t = 1.0, buttonsExtent = 187.
    // tilesExtent is 113 + 27 = 140, so buttonsExtent dominates.
    const ThumbnailStripLayout fullButtons = thumbnailStripLayoutFor(
        {.previewsSize = ThumbnailStripLayout::kMasonryFullButtonsSize,
         .position = Enums::PanelPosition::Bottom,
         .masonry = true});
    QCOMPARE(fullButtons.panelExtent, 187);
  }

  void verticalMasonryStripHasTilesAsWideAsThePreviewSize() {
    const ThumbnailStripLayout left = thumbnailStripLayoutFor(
        {.previewsSize = 256, .position = Enums::PanelPosition::Left, .masonry = true});
    QVERIFY(!left.horizontal);
    QCOMPARE(left.laneExtent, 256);
    QCOMPARE(left.panelExtent, 256 + 2 * ThumbnailStripLayout::kMasonryCrossMargin +
                                   ThumbnailStripLayout::kPanelAllowance);
    // 280 without the border line of 1 px, the lane in the middle.
    QCOMPARE(left.laneOffset, (280 - 1 - 256) / 2);
  }

  void uniformStripIsNotAffectedByTheMasonryFields() {
    const ThumbnailStripLayout layout = thumbnailStripLayoutFor(
        {.previewsSize = 120, .position = Enums::PanelPosition::Bottom});
    QVERIFY(!layout.masonry);
    QCOMPARE(layout.laneExtent, 0);
    QCOMPARE(layout.panelExtent, 135);
  }

  //--- draw size --------------------------------------------------------------
  void drawSizeFitsTheImageArea() {
    QCOMPARE(thumbnailDrawSize({200, 150}, {2000, 1500}, {120, 90}, 1.0), QSize(120, 90));
    QCOMPARE(thumbnailDrawSize({100, 200}, {1000, 2000}, {120, 90}, 1.0), QSize(45, 90));
  }

  void drawSizeIsLimitedByTheSource() {
    QCOMPARE(thumbnailDrawSize({200, 150}, {60, 45}, {120, 90}, 1.0), QSize(60, 45));
    // The source counts in device pixels.
    QCOMPARE(thumbnailDrawSize({200, 150}, {120, 90}, {120, 90}, 2.0), QSize(60, 45));
    // A source in the other orientation is compared turned.
    QCOMPARE(thumbnailDrawSize({90, 120}, {40, 30}, {120, 120}, 1.0), QSize(30, 40));
    // No source size: no limit.
    QCOMPARE(thumbnailDrawSize({200, 150}, {}, {120, 90}, 1.0), QSize(120, 90));
    QVERIFY(thumbnailDrawSize({}, {60, 45}, {120, 90}, 1.0).isEmpty());
  }

  //--- loading ----------------------------------------------------------------
  void requestsTheVisibleAndPreloadedItems() {
    constexpr int kLastPreloaded =
        static_cast<int>(kViewExtent + ThumbnailListModel::kStripPreloadDistance) / kItemExtent;
    constexpr int kPreloadedCount = kLastPreloaded + 1;
    ThumbnailListModel model;
    QSignalSpy requests(&model, &ThumbnailListModel::thumbnailsNeeded);
    prepare(model, 1000);
    QCOMPARE(requests.count(), 1);
    const auto batch = requests.first().at(0).value<QList<ThumbnailItemRequest>>();
    QCOMPARE(batch.first().index, 0);
    QCOMPARE(batch.last().index, kLastPreloaded);
    QCOMPARE(batch.count(), kPreloadedCount);
    QCOMPARE(batch.first().targetSize, kRequestSize);
    QCOMPARE(requests.first().at(2).toBool(), false);

    // Requested items are not requested again.
    model.setViewport(kItemExtent, kViewExtent);
    QCOMPARE(requests.count(), 2);
    QCOMPARE(requestedIndices(requests.last()), QList<int>{kPreloadedCount});
  }

  void requestsNothingWhileInactiveOrBlocked() {
    ThumbnailListModel model;
    QSignalSpy requests(&model, &ThumbnailListModel::thumbnailsNeeded);
    prepare(model, 100, false);
    QCOMPARE(requests.count(), 0);
    model.setLoadingBlocked(true);
    model.setActive(true);
    QCOMPARE(requests.count(), 0);
    model.setLoadingBlocked(false);
    QCOMPARE(requests.count(), 1);
  }

  void requestsBackwardsNearestFirst() {
    ThumbnailListModel model;
    QSignalSpy requests(&model, &ThumbnailListModel::thumbnailsNeeded);
    prepare(model, 1000, false);
    model.setScrollConfig({.itemExtent = kItemExtent,
                           .thumbnailSize = kThumbnailSize,
                           .trackpadDetection = true});
    model.setViewport(50000.0, kViewExtent);
    // A touchpad scroll towards the start.
    model.wheelScrolled({0, 10}, {0, 10});
    model.setActive(true);
    QCOMPARE(requests.count(), 1);
    const auto indices = requestedIndices(requests.first());
    QVERIFY(indices.first() > indices.last());
  }

  void reportsVisibleThumbnailsReadyOnce() {
    ThumbnailListModel model;
    QSignalSpy ready(&model, &ThumbnailListModel::visibleThumbnailsReady);
    prepare(model, 3);
    QCOMPARE(ready.count(), 0);
    model.setThumbnail(0, sampleEntry());
    model.setThumbnailUnavailable(1, kRequestSize);
    // Another target box is not the requested thumbnail.
    model.setThumbnail(2, sampleEntry(kRequestSize + QSize(1, 1)));
    QCOMPARE(ready.count(), 0);
    QVERIFY(!isLoaded(model, 2));
    model.setThumbnail(2, sampleEntry());
    QCOMPARE(ready.count(), 1);
    model.setThumbnail(2, sampleEntry());
    QCOMPARE(ready.count(), 1);
    QVERIFY(isLoaded(model, 0));
    QVERIFY(model.data(model.index(1), ThumbnailListModel::UnavailableRole).toBool());

    model.populate(0);
    QCOMPARE(ready.count(), 2);
  }

  void waitsForFinalThumbnails() {
    ThumbnailListModel model;
    QSignalSpy ready(&model, &ThumbnailListModel::visibleThumbnailsReady);
    prepare(model, 1);
    model.setThumbnailPending(0, true);
    model.setThumbnail(0, sampleEntry());
    QCOMPARE(ready.count(), 0);
    QVERIFY(model.data(model.index(0), ThumbnailListModel::PendingRole).toBool());
    model.setThumbnailPending(0, false);
    QCOMPARE(ready.count(), 1);
  }

  void insertAndRemoveShiftItems() {
    ThumbnailListModel model;
    prepare(model, 5);
    model.setThumbnail(3, sampleEntry());
    model.select(3);
    model.insertItem(1);
    QCOMPARE(model.itemCount(), 6);
    QVERIFY(isLoaded(model, 4));
    QVERIFY(!isLoaded(model, 3));
    QCOMPARE(model.selection(), QList<int>{4});
    QVERIFY(model.data(model.index(4), ThumbnailListModel::SelectedRole).toBool());

    model.removeItem(0);
    QCOMPARE(model.itemCount(), 5);
    QVERIFY(isLoaded(model, 3));
    QCOMPARE(model.selection(), QList<int>{3});
  }

  void removingTheSelectionSelectsANeighbour() {
    ThumbnailListModel model;
    prepare(model, 5);
    model.select(4);
    model.removeItem(4);
    QCOMPARE(model.selection(), QList<int>{3});
    model.select(1);
    model.removeItem(1);
    QCOMPARE(model.selection(), QList<int>{1});
  }

  void selectionDropsInvalidItems() {
    ThumbnailListModel model;
    prepare(model, 5);
    model.select(QList<int>{-1, 2, 9});
    QCOMPARE(model.selection(), QList<int>{2});
    QCOMPARE(model.currentIndex(), 2);
    model.select(42);
    QCOMPARE(model.selection(), QList<int>{0});
  }

  void unloadsThumbnailsFarOutOfView() {
    ThumbnailListModel model;
    model.configure({.targetSize = kRequestSize, .unloadOffscreen = true});
    model.setScrollConfig({.itemExtent = kItemExtent, .thumbnailSize = kThumbnailSize});
    model.populate(1000);
    model.setViewport(0.0, kViewExtent);
    model.setActive(true);
    model.setThumbnail(0, sampleEntry());
    QVERIFY(isLoaded(model, 0));
    model.setViewport(50000.0, kViewExtent);
    QVERIFY(!isLoaded(model, 0));
    // A thumbnail arriving out of range is not kept.
    model.setThumbnail(1, sampleEntry());
    QVERIFY(!isLoaded(model, 1));
  }

  void inactiveViewKeepsThumbnailsWithinItsRange() {
    ThumbnailListModel model;
    QSignalSpy requests(&model, &ThumbnailListModel::thumbnailsNeeded);
    model.configure({.targetSize = kRequestSize, .unloadOffscreen = true});
    model.setScrollConfig({.itemExtent = kItemExtent, .thumbnailSize = kThumbnailSize});
    model.populate(1000);
    model.setViewport(0.0, kViewExtent);
    // Results another view requested reach the hidden view.
    model.setThumbnail(0, sampleEntry());
    model.setThumbnail(999, sampleEntry());
    QVERIFY(isLoaded(model, 0));
    QVERIFY(!isLoaded(model, 999));
    QCOMPARE(requests.count(), 0);
    // Shown, the view does not request what it already holds.
    model.setActive(true);
    QCOMPARE(requests.count(), 1);
    QVERIFY(!requestedIndices(requests.first()).contains(0));
    // Hidden again, the thumbnails it holds follow its range.
    model.setActive(false);
    model.setViewport(50000.0, kViewExtent);
    QVERIFY(!isLoaded(model, 0));
  }

  void inactiveViewFocusesAtOnce() {
    constexpr int kCount = 1000;
    constexpr qreal kEndOffset = kCount * kItemExtent - kViewExtent;
    ThumbnailListModel model;
    model.configure({.targetSize = kRequestSize, .unloadOffscreen = true});
    model.setScrollConfig({.itemExtent = kItemExtent,
                           .thumbnailSize = kThumbnailSize,
                           .centerSelection = true,
                           .smoothScroll = true});
    model.populate(kCount);
    model.setViewport(0.0, kViewExtent);
    QSignalSpy scrolls(&model, &ThumbnailListModel::scrollRequested);
    model.select(kCount - 1);
    model.focusOn(kCount - 1);
    // Nobody sees a hidden view scroll: it is at the item at once.
    QCOMPARE(scrolls.count(), 1);
    QCOMPARE(scrolls.first().at(0).toReal(), kEndOffset);
    QCOMPARE(scrolls.first().at(1).toInt(), 0);
    QCOMPARE(model.viewOffset(), kEndOffset);
  }

  void hiddenViewReturnsToTheSelection() {
    // The selected item and the offset that centres it, the end of the
    // strip, and where the right-button gesture starts.
    constexpr int kSelectedItem = 10;
    constexpr qreal kSelectedOffset =
        kSelectedItem * kItemExtent + kItemExtent / 2.0 - kViewExtent / 2.0;
    constexpr int kCount = 1000;
    constexpr qreal kEndOffset = kCount * kItemExtent - kViewExtent;
    constexpr qreal kPressX = 200.0;
    ThumbnailListModel model;
    model.configure({.targetSize = kRequestSize, .unloadOffscreen = true});
    model.setScrollConfig({.itemExtent = kItemExtent,
                           .thumbnailSize = kThumbnailSize,
                           .centerSelection = true});
    model.setInactivePrefetch(true);
    model.populate(kCount);
    model.setViewport(0.0, kViewExtent);
    model.setActive(true);
    model.select(kSelectedItem);
    model.focusOn(kSelectedItem);
    model.setThumbnail(kSelectedItem, sampleEntry());
    // The right-button gesture to the end unloads the selected item.
    model.press(kSelectedItem, Qt::RightButton, Qt::NoModifier, {kPressX, 0});
    model.move({kPressX - ThumbnailListModel::kGestureThreshold - 1.0, 0}, Qt::RightButton);
    model.setViewport(kEndOffset, kViewExtent);
    model.scrollAnimationFinished();
    model.release(kSelectedItem, Qt::RightButton, {kPressX, 0});
    QVERIFY(!isLoaded(model, kSelectedItem));

    // Hidden, the view returns to the selection at once and prefetches what
    // it shows there, instead of scrolling back over unloaded items while
    // it slides in.
    QSignalSpy scrolls(&model, &ThumbnailListModel::scrollRequested);
    QSignalSpy requests(&model, &ThumbnailListModel::thumbnailsNeeded);
    model.setActive(false);
    QCOMPARE(scrolls.count(), 1);
    QCOMPARE(scrolls.first().at(0).toReal(), kSelectedOffset);
    QCOMPARE(scrolls.first().at(1).toInt(), 0);
    QTRY_COMPARE(requests.count(), 1);
    QVERIFY(requestedIndices(requests.first()).contains(kSelectedItem));
    // Shown again, it stays there.
    model.setActive(true);
    QCOMPARE(model.viewOffset(), kSelectedOffset);
  }

  void ignoresThumbnailsOfTheOtherCrop() {
    ThumbnailListModel model;
    model.configure({.targetSize = kRequestSize, .crop = true});
    model.setScrollConfig({.itemExtent = kItemExtent, .thumbnailSize = kThumbnailSize});
    model.populate(2);
    model.setViewport(0.0, kViewExtent);
    model.setActive(true);
    // The folder grid requested the same path and size uncropped.
    ThumbnailEntry uncropped = sampleEntry();
    uncropped.cropped = false;
    model.setThumbnail(0, uncropped);
    QVERIFY(!isLoaded(model, 0));
    QVERIFY(model.data(model.index(0), ThumbnailListModel::PendingRole).toBool());
    ThumbnailEntry cropped = entryOf(3, 3);
    cropped.cropped = true;
    model.setThumbnail(0, cropped);
    QVERIFY(isLoaded(model, 0));
    // A folder icon serves either crop.
    model.setThumbnail(1, sampleEntry());
    QVERIFY(isLoaded(model, 1));
  }

  void inactivePrefetchRequestsWhatTheViewShowsFirst() {
    // An item far from the start, and the first item the view shows when
    // it is focused there (scrolled into view with half a row of margin).
    constexpr int kFocusedItem = 500;
    constexpr int kFirstFocusedItem = 496;
    constexpr int kPrefetchWaitMs = ThumbnailListModel::kInactivePrefetchDelayMs + kWaitMarginMs;
    ThumbnailListModel model;
    QSignalSpy requests(&model, &ThumbnailListModel::thumbnailsNeeded);
    model.configure({.targetSize = kRequestSize, .unloadOffscreen = true});
    model.setScrollConfig({.itemExtent = kItemExtent, .thumbnailSize = kThumbnailSize});
    model.setInactivePrefetch(true);
    model.populate(1000);
    // Never laid out (the strip is not created yet): nothing to prefetch.
    QTest::qWait(kPrefetchWaitMs);
    QCOMPARE(requests.count(), 0);
    // Laid out, the hidden view requests its visible items, not the
    // preload range.
    model.setViewport(0.0, kViewExtent);
    QTRY_COMPARE(requests.count(), 1);
    QList<int> indices = requestedIndices(requests.last());
    QCOMPARE(indices.first(), 0);
    QCOMPARE(indices.count(), kVisibleItems);
    // A population and the focus of the opened item request once, for the
    // items around the focused one.
    model.populate(1000);
    model.select(kFocusedItem);
    model.focusOn(kFocusedItem);
    QTRY_COMPARE(requests.count(), 2);
    QTest::qWait(kPrefetchWaitMs);
    QCOMPARE(requests.count(), 2);
    indices = requestedIndices(requests.last());
    QCOMPARE(indices.first(), kFirstFocusedItem);
    QCOMPARE(indices.count(), kVisibleItems);
    // Shown, the view preloads the rest without requesting them again.
    model.setActive(true);
    QCOMPARE(requests.count(), 3);
    QVERIFY(!requestedIndices(requests.last()).contains(kFocusedItem));
    // Without prefetch a hidden view requests nothing.
    model.setActive(false);
    model.setInactivePrefetch(false);
    model.select(0);
    model.focusOn(0);
    QTest::qWait(kPrefetchWaitMs);
    QCOMPARE(requests.count(), 3);
  }

  void inactivePrefetchWaitsForTheSlide() {
    ThumbnailListModel model;
    QSignalSpy requests(&model, &ThumbnailListModel::thumbnailsNeeded);
    prepare(model, 100, false);
    model.setLoadingBlocked(true);
    model.setInactivePrefetch(true);
    QTest::qWait(ThumbnailListModel::kInactivePrefetchDelayMs + kWaitMarginMs);
    QCOMPARE(requests.count(), 0);
    model.setLoadingBlocked(false);
    QTRY_COMPARE(requests.count(), 1);
  }

  void newRequestConfigUnloadsAndRequestsAgain() {
    ThumbnailListModel model;
    QSignalSpy requests(&model, &ThumbnailListModel::thumbnailsNeeded);
    prepare(model, 2);
    model.setThumbnail(0, sampleEntry());
    requests.clear();
    model.configure({.targetSize = kRequestSize, .crop = true});
    QVERIFY(!isLoaded(model, 0));
    QCOMPARE(requests.count(), 1);
    QCOMPARE(requestedIndices(requests.first()), (QList<int>{0, 1}));
    QCOMPARE(requests.first().at(1).toBool(), true);
  }

  void reloadForcesARequest() {
    ThumbnailListModel model;
    QSignalSpy requests(&model, &ThumbnailListModel::thumbnailsNeeded);
    prepare(model, 2);
    model.setThumbnail(1, sampleEntry());
    requests.clear();
    model.reloadItem(1);
    // The loaded thumbnail stays until the new one arrives, so the item does
    // not flicker.
    QVERIFY(isLoaded(model, 1));
    QCOMPARE(requests.count(), 1);
    QCOMPARE(requestedIndices(requests.first()), QList<int>{1});
    QCOMPARE(requests.first().at(2).toBool(), true);
  }

  //--- scrolling --------------------------------------------------------------
  void focusCentresTheSelection() {
    ThumbnailListModel model;
    prepare(model, 100);
    model.setScrollConfig({.itemExtent = kItemExtent,
                           .thumbnailSize = kThumbnailSize,
                           .centerSelection = true});
    QSignalSpy scrolls(&model, &ThumbnailListModel::scrollRequested);
    model.focusOn(50);
    QCOMPARE(scrolls.count(), 1);
    QCOMPARE(scrolls.first().at(0).toReal(), 5050.0 - kViewExtent / 2.0);
    QCOMPARE(scrolls.first().at(1).toInt(), 0);
    // Near the end the offset stops at the last page.
    model.focusOn(99);
    QCOMPARE(scrolls.last().at(0).toReal(), 100 * kItemExtent - kViewExtent);
  }

  void focusKeepsTheItemInViewWithAMargin() {
    ThumbnailListModel model;
    prepare(model, 100);
    QSignalSpy scrolls(&model, &ThumbnailListModel::scrollRequested);
    model.focusOn(2);
    QCOMPARE(scrolls.count(), 0);
    // Half a cell of the next item stays visible.
    model.focusOn(10);
    QCOMPARE(scrolls.count(), 1);
    QCOMPARE(scrolls.first().at(0).toReal(), 1150.0 - kViewExtent);
    model.focusOn(0);
    QCOMPARE(scrolls.last().at(0).toReal(), 0.0);
  }

  void smoothWheelStepsAccumulate() {
    constexpr qreal kWheelStep = 120 * ThumbnailListModel::kWheelScrollMultiplier;
    ThumbnailListModel model;
    prepare(model, 100);
    model.setScrollConfig({.itemExtent = kItemExtent,
                           .thumbnailSize = kThumbnailSize,
                           .smoothScroll = true,
                           .wheelSpeed = 1.0});
    model.setViewport(1000.0, kViewExtent);
    QSignalSpy scrolls(&model, &ThumbnailListModel::scrollRequested);
    QSignalSpy requests(&model, &ThumbnailListModel::thumbnailsNeeded);
    model.wheelScrolled({0, -120}, {});
    QCOMPARE(scrolls.count(), 1);
    QCOMPARE(scrolls.first().at(0).toReal(), 1000.0 + kWheelStep);
    QCOMPARE(scrolls.first().at(1).toInt(), ThumbnailListModel::kScrollDurationMs);
    // Nothing loads while the animation runs.
    requests.clear();
    model.setViewport(1100.0, kViewExtent);
    QCOMPARE(requests.count(), 0);
    // A step right after the first adds to its target, faster.
    model.wheelScrolled({0, -120}, {});
    QCOMPARE(scrolls.count(), 2);
    QCOMPARE(scrolls.last().at(0).toReal(),
             1000.0 + kWheelStep + kWheelStep * ThumbnailListModel::kScrollAcceleration);
    QVERIFY(scrolls.last().at(1).toInt() < ThumbnailListModel::kScrollDurationMs);
    // The view arrives; its end loads what came into range.
    model.setViewport(1720.0, kViewExtent);
    model.scrollAnimationFinished();
    QCOMPARE(requests.count(), 2);
  }

  void wheelWithoutSmoothScrollingStepsByItem() {
    ThumbnailListModel model;
    prepare(model, 100);
    QSignalSpy scrolls(&model, &ThumbnailListModel::scrollRequested);
    model.wheelScrolled({0, -120}, {});
    // Items 0 - 5 are (nearly) visible: item 6 is brought into view.
    QCOMPARE(scrolls.count(), 1);
    QCOMPARE(scrolls.first().at(0).toReal(), 200.0);
    QCOMPARE(scrolls.first().at(1).toInt(), 0);
    // At the start nothing scrolls back.
    model.setViewport(0.0, kViewExtent);
    model.wheelScrolled({0, 120}, {});
    QCOMPARE(scrolls.count(), 1);
  }

  void touchpadScrollsByPixels() {
    ThumbnailListModel model;
    prepare(model, 100);
    model.setScrollConfig({.itemExtent = kItemExtent,
                           .thumbnailSize = kThumbnailSize,
                           .smoothScroll = true,
                           .trackpadDetection = true});
    QSignalSpy scrolls(&model, &ThumbnailListModel::scrollRequested);
    // Touchpad path picks the larger magnitude between angleDelta (-30) and pixelDelta (-12).
    model.wheelScrolled({0, -30}, {0, -12});
    QCOMPARE(scrolls.count(), 1);
    QCOMPARE(scrolls.first().at(0).toReal(), 30.0);
    QCOMPARE(scrolls.first().at(1).toInt(), 0);
  }

  void pageScrollKeepsPartlyHiddenItemsInView() {
    ThumbnailListModel model;
    prepare(model, 100);
    QSignalSpy scrolls(&model, &ThumbnailListModel::scrollRequested);
    // 50 - 550 shows item 5 in part: it becomes the first one.
    model.setViewport(50.0, kViewExtent);
    model.scrollPage(true);
    QCOMPARE(scrolls.count(), 1);
    QCOMPARE(scrolls.first().at(0).toReal(), 500.0);
    QCOMPARE(scrolls.first().at(1).toInt(), 0);
    // 1050 - 1550 shows item 10 in part: it becomes the last one.
    model.setViewport(1050.0, kViewExtent);
    model.scrollPage(false);
    QCOMPARE(model.viewOffset(), 1100.0 - kViewExtent);
    // Rows that fit exactly move by the whole view.
    model.setViewport(1000.0, kViewExtent);
    model.scrollPage(true);
    QCOMPARE(model.viewOffset(), 1500.0);
    model.scrollPage(false);
    QCOMPARE(model.viewOffset(), 1000.0);
  }

  void pageScrollStopsAtTheEnds() {
    ThumbnailListModel model;
    prepare(model, 100);
    const qreal endOffset = 100.0 * kItemExtent - kViewExtent;
    QSignalSpy scrolls(&model, &ThumbnailListModel::scrollRequested);
    model.scrollPage(false);
    QCOMPARE(scrolls.count(), 0);
    model.setViewport(endOffset - 200.0, kViewExtent);
    model.scrollPage(true);
    QCOMPARE(model.viewOffset(), endOffset);
    model.setViewport(endOffset, kViewExtent);
    scrolls.clear();
    model.scrollPage(true);
    QCOMPARE(scrolls.count(), 0);
    model.setViewport(300.0, kViewExtent);
    model.scrollPage(false);
    QCOMPARE(model.viewOffset(), 0.0);
  }

  void pageScrollMovesAViewShorterThanARow() {
    ThumbnailListModel model;
    prepare(model, 100);
    constexpr qreal kShortExtent = 50.0;
    model.setViewport(0.0, kShortExtent);
    model.scrollPage(true);
    QCOMPARE(model.viewOffset(), 100.0);
    model.scrollPage(false);
    QCOMPARE(model.viewOffset(), 0.0);
  }

  void smoothPageScrollPagesOnFromItsTarget() {
    ThumbnailListModel model;
    prepare(model, 100);
    model.setScrollConfig({.itemExtent = kItemExtent,
                           .thumbnailSize = kThumbnailSize,
                           .smoothScroll = true});
    model.setViewport(0.0, kViewExtent);
    QSignalSpy scrolls(&model, &ThumbnailListModel::scrollRequested);
    model.scrollPage(true);
    QCOMPARE(scrolls.count(), 1);
    QCOMPARE(scrolls.first().at(0).toReal(), 500.0);
    QCOMPARE(scrolls.first().at(1).toInt(), ThumbnailListModel::kPageScrollDurationMs);
    // Pressed again halfway: the next page follows the first one.
    model.setViewport(250.0, kViewExtent);
    model.scrollPage(true);
    QCOMPARE(scrolls.count(), 2);
    QCOMPARE(scrolls.last().at(0).toReal(), 1000.0);
  }

  void scrollEdgesFollowTheView() {
    ThumbnailListModel model;
    prepare(model, 100);
    QSignalSpy edges(&model, &ThumbnailListModel::scrollEdgesChanged);
    QVERIFY(!model.canScrollBackward());
    QVERIFY(model.canScrollForward());
    model.setViewport(1000.0, kViewExtent);
    QVERIFY(model.canScrollBackward());
    QVERIFY(model.canScrollForward());
    model.setViewport(100.0 * kItemExtent - kViewExtent, kViewExtent);
    QVERIFY(model.canScrollBackward());
    QVERIFY(!model.canScrollForward());
    QCOMPARE(edges.count(), 2);
    // A directory that fits the view scrolls neither way.
    model.populate(3);
    QVERIFY(!model.canScrollBackward());
    QVERIFY(!model.canScrollForward());
  }

  //--- pointer ----------------------------------------------------------------
  void pressActivatesAnItem() {
    ThumbnailListModel model;
    prepare(model, 10);
    QSignalSpy activated(&model, &ThumbnailListModel::activated);
    model.press(3, Qt::LeftButton, Qt::NoModifier, {});
    QCOMPARE(activated.count(), 1);
    QCOMPARE(activated.first().at(0).toInt(), 3);
    model.doubleClick(4, Qt::LeftButton);
    QCOMPARE(activated.count(), 2);
    model.doubleClick(4, Qt::RightButton);
    QCOMPARE(activated.count(), 2);
  }

  void controlPressTogglesTheSelection() {
    ThumbnailListModel model;
    prepare(model, 10);
    model.select(1);
    QSignalSpy activated(&model, &ThumbnailListModel::activated);
    model.press(3, Qt::LeftButton, Qt::ControlModifier, {});
    QCOMPARE(model.selection(), (QList<int>{1, 3}));
    model.press(1, Qt::LeftButton, Qt::ControlModifier, {});
    QCOMPARE(model.selection(), QList<int>{3});
    // The last item stays selected.
    model.press(3, Qt::LeftButton, Qt::ControlModifier, {});
    QCOMPARE(model.selection(), QList<int>{3});
    QCOMPARE(activated.count(), 0);
  }

  void releaseSelectsWithinAMultiSelection() {
    ThumbnailListModel model;
    prepare(model, 10);
    model.select(QList<int>{1, 2});
    QSignalSpy activated(&model, &ThumbnailListModel::activated);
    model.press(2, Qt::LeftButton, Qt::NoModifier, {10, 10});
    QCOMPARE(activated.count(), 0);
    model.release(2, Qt::LeftButton, {12, 10});
    QCOMPARE(model.selection(), QList<int>{2});
  }

  void pressOnTheBackgroundClearsTheSelection() {
    ThumbnailListModel model;
    prepare(model, 2);
    model.select(1);
    model.press(-1, Qt::LeftButton, Qt::ControlModifier, {});
    QCOMPARE(model.selection(), QList<int>{1});
    model.press(-1, Qt::LeftButton, Qt::NoModifier, {});
    QVERIFY(model.selection().isEmpty());
  }

  void dragOutStartsFromASelectedItem() {
    ThumbnailListModel model;
    prepare(model, 10);
    model.select(QList<int>{1, 2});
    QSignalSpy dragged(&model, &ThumbnailListModel::dragOutRequested);
    model.press(2, Qt::LeftButton, Qt::NoModifier, {0, 0});
    model.move({ThumbnailListModel::kDragThreshold / 2.0, 0}, Qt::LeftButton);
    QCOMPARE(dragged.count(), 0);
    model.move({ThumbnailListModel::kDragThreshold, 0}, Qt::LeftButton);
    QCOMPARE(dragged.count(), 1);
  }

  void rightButtonSelectsOrScrollsToAnEnd() {
    ThumbnailListModel model;
    prepare(model, 100);
    model.select(1);
    model.press(4, Qt::RightButton, Qt::NoModifier, {200, 0});
    model.release(4, Qt::RightButton, {200, 0});
    QCOMPARE(model.selection(), QList<int>{4});

    QSignalSpy scrolls(&model, &ThumbnailListModel::scrollRequested);
    model.press(4, Qt::RightButton, Qt::NoModifier, {200, 0});
    model.move({200 - ThumbnailListModel::kGestureThreshold - 1.0, 0}, Qt::RightButton);
    QCOMPARE(scrolls.count(), 1);
    QCOMPARE(scrolls.first().at(0).toReal(), 100 * kItemExtent - kViewExtent);
    QVERIFY(scrolls.first().at(1).toInt() >= ThumbnailListModel::kEdgeScrollMinimumMs);
    // Unlike a wheel step, the scroll to an end loads what passes into range,
    // so the thumbnails at the end are there when it arrives.
    QSignalSpy requests(&model, &ThumbnailListModel::thumbnailsNeeded);
    constexpr int kPassedItem = 50;
    model.setViewport(kPassedItem * kItemExtent, kViewExtent);
    QCOMPARE(requests.count(), 1);
    QVERIFY(requestedIndices(requests.first()).contains(kPassedItem));
    // A gesture selects nothing.
    model.release(6, Qt::RightButton, {100, 0});
    QCOMPARE(model.selection(), QList<int>{4});
  }

  void backAndForwardButtonsNavigate() {
    ThumbnailListModel model;
    prepare(model, 3);
    QSignalSpy back(&model, &ThumbnailListModel::backNavigationRequested);
    QSignalSpy forward(&model, &ThumbnailListModel::forwardNavigationRequested);
    model.press(1, Qt::BackButton, Qt::NoModifier, {});
    model.press(-1, Qt::ForwardButton, Qt::NoModifier, {});
    QCOMPARE(back.count(), 1);
    QCOMPARE(forward.count(), 1);
  }

  //--- rows -------------------------------------------------------------------
  void rowsRequestTheVisibleAndPreloadedItems() {
    constexpr int kColumns = 4;
    constexpr int kLeadingSpace = 6;
    constexpr int kPreloadDistance = 200;
    ThumbnailListModel model;
    model.configure({.targetSize = kRequestSize});
    model.setScrollConfig({.horizontal = false,
                           .itemExtent = kItemExtent,
                           .columns = kColumns,
                           .leadingSpace = kLeadingSpace,
                           .preloadDistance = kPreloadDistance});
    model.populate(100);
    QSignalSpy requests(&model, &ThumbnailListModel::thumbnailsNeeded);
    model.setViewport(0.0, kViewExtent);
    model.setActive(true);
    QCOMPARE(requests.count(), 1);
    // Rows 0 .. floor((500 + 200 - 6) / 100) = 6, four items each.
    const auto indices = requestedIndices(requests.first());
    QCOMPARE(indices.size(), 7 * kColumns);
    QCOMPARE(indices.constFirst(), 0);
    QCOMPARE(indices.constLast(), 7 * kColumns - 1);
  }

  void rowsFocusAndScrollByRow() {
    constexpr int kColumns = 4;
    constexpr int kLeadingSpace = 6;
    ThumbnailListModel model;
    model.configure({.targetSize = kRequestSize});
    model.setScrollConfig({.horizontal = false,
                           .itemExtent = kItemExtent,
                           .columns = kColumns,
                           .leadingSpace = kLeadingSpace,
                           .focusShowsNeighbours = false,
                           .thumbnailSize = kThumbnailSize});
    model.populate(100);
    model.setViewport(0.0, kViewExtent);
    model.setActive(true);
    QSignalSpy scrolls(&model, &ThumbnailListModel::scrollRequested);
    // Item 41 is in row 10, which ends at 6 + 11 * 100; no neighbour margin.
    model.focusOn(41);
    QCOMPARE(scrolls.count(), 1);
    QCOMPARE(scrolls.first().at(0).toReal(), kLeadingSpace + 11.0 * kItemExtent - kViewExtent);
    // The content ends after 25 rows.
    model.scrollToItem(99);
    QCOMPARE(model.viewOffset(), kLeadingSpace + 25.0 * kItemExtent - kViewExtent);
    // An item of a visible row does not scroll.
    scrolls.clear();
    model.scrollToItem(97);
    QCOMPARE(scrolls.count(), 0);
  }

  void doubleClickActivationSelectsOnPress() {
    ThumbnailListModel model;
    prepare(model, 10);
    model.setItemActivation(ThumbnailListModel::ItemActivation::OnDoubleClick);
    QSignalSpy activated(&model, &ThumbnailListModel::activated);
    model.press(3, Qt::LeftButton, Qt::NoModifier, {});
    QCOMPARE(activated.count(), 0);
    QCOMPARE(model.selection(), QList<int>{3});
    model.doubleClick(3, Qt::LeftButton);
    QCOMPARE(activated.count(), 1);
    QCOMPARE(activated.first().at(0).toInt(), 3);
  }

  void shiftExtendsARangeFromTheAnchor() {
    ThumbnailListModel model;
    prepare(model, 10);
    model.select(4);
    // Without an anchor Shift-press changes nothing (the strip).
    model.press(6, Qt::LeftButton, Qt::ShiftModifier, {});
    QCOMPARE(model.selection(), QList<int>{4});

    model.beginRangeSelection();
    QVERIFY(model.rangeSelectionActive());
    model.press(6, Qt::LeftButton, Qt::ShiftModifier, {});
    QCOMPARE(model.selection(), (QList<int>{4, 5, 6}));
    // A new end replaces the previous range; the end is the current item.
    model.selectRangeTo(2);
    QCOMPARE(model.selection(), (QList<int>{4, 3, 2}));
    QCOMPARE(model.currentIndex(), 2);
    model.endRangeSelection();
    QVERIFY(!model.rangeSelectionActive());
    // A new directory drops the anchor.
    model.populate(10);
    model.select(1);
    model.selectRangeTo(3);
    QCOMPARE(model.selection(), QList<int>{1});
  }

  //--- panel ------------------------------------------------------------------
  void pinnedPanelIsDockedInALargeWindow() {
    ThumbnailListModel model;
    ThumbnailPanelController panel(model, panelSettings(true, Enums::PanelPosition::Bottom));
    QVERIFY(!panel.isShown());
    panel.setWindowSize(kLargeWindow);
    QVERIFY(panel.isShown());
    QVERIFY(panel.isDocked());
    QVERIFY(!panel.isAnimated());
    panel.setWindowSize(kSmallWindow);
    QVERIFY(!panel.isShown());
    QVERIFY(!panel.isDocked());
    // A pinned panel ignores the pointer and the folder view.
    panel.setWindowSize(kLargeWindow);
    panel.pointerMoved(viewerPoint(), Qt::NoButton);
    panel.setFolderViewActive(true);
    QVERIFY(panel.isShown());
  }

  void floatingPanelFollowsThePointer() {
    ThumbnailListModel model;
    ThumbnailPanelController panel(model, panelSettings(false, Enums::PanelPosition::Bottom));
    panel.setWindowSize(kLargeWindow);
    QVERIFY(!panel.isShown());
    panel.pointerMoved(bottomPanelPoint(), Qt::NoButton);
    QVERIFY(panel.isShown());
    QVERIFY(panel.isAnimated());
    QVERIFY(!panel.isDocked());
    panel.pointerMoved(viewerPoint(), Qt::NoButton);
    QVERIFY(panel.isShown());
    QTRY_VERIFY_WITH_TIMEOUT(!panel.isShown(), kTryTimeoutMs);
    QVERIFY(panel.isAnimated());
  }

  // The crop mode turns the viewer input off.
  void lockedViewerKeepsTheFloatingPanelHidden() {
    ThumbnailListModel model;
    ThumbnailPanelController panel(model, panelSettings(false, Enums::PanelPosition::Bottom));
    panel.setWindowSize(kLargeWindow);
    panel.pointerMoved(bottomPanelPoint(), Qt::NoButton);
    QVERIFY(panel.isShown());
    panel.setInteractionEnabled(false);
    QVERIFY(!panel.isShown());
    QVERIFY(!panel.isAnimated());
    panel.pointerMoved(viewerPoint(), Qt::NoButton);
    panel.pointerMoved(bottomPanelPoint(), Qt::NoButton);
    QVERIFY(!panel.isShown());
    panel.setInteractionEnabled(true);
    panel.pointerMoved(bottomPanelPoint(), Qt::NoButton);
    QVERIFY(panel.isShown());
  }

  void pressInThePanelAreaKeepsItHidden() {
    ThumbnailListModel model;
    ThumbnailPanelController panel(model, panelSettings(false, Enums::PanelPosition::Bottom));
    panel.setWindowSize(kLargeWindow);
    panel.pointerMoved(bottomPanelPoint(), Qt::LeftButton);
    panel.pointerMoved(bottomPanelPoint(), Qt::NoButton);
    QVERIFY(!panel.isShown());
    // Until the pointer left the area.
    panel.pointerMoved(viewerPoint(), Qt::NoButton);
    panel.pointerMoved(bottomPanelPoint(), Qt::NoButton);
    QVERIFY(panel.isShown());
  }

  void leavingTheWindowHidesAfterTheGracePeriod() {
    static_assert(kShortHideDelayMs + kWaitMarginMs <
                  ThumbnailPanelController::kWindowExitMinimumHideDelayMs);
    ThumbnailListModel model;
    ThumbnailPanelController panel(model, panelSettings(false, Enums::PanelPosition::Bottom));
    panel.setWindowSize(kLargeWindow);
    panel.pointerMoved(bottomPanelPoint(), Qt::NoButton);
    panel.pointerLeftWindow();
    QTest::qWait(kShortHideDelayMs + kWaitMarginMs);
    QVERIFY(panel.isShown());
    QTRY_VERIFY_WITH_TIMEOUT(!panel.isShown(), kTryTimeoutMs);
  }

  void fullscreenOnlyPanelShowsInFullscreen() {
    ThumbnailListModel model;
    UiSettingsSnapshot settings = panelSettings(false, Enums::PanelPosition::Top);
    settings.panel.fullscreenOnly = true;
    ThumbnailPanelController panel(model, settings);
    panel.setWindowSize(kLargeWindow);
    panel.pointerMoved({10, 2}, Qt::NoButton);
    QVERIFY(!panel.isShown());
    QVERIFY(!panel.isExitButtonVisible());
    panel.setFullscreen(true);
    QVERIFY(panel.isExitButtonVisible());
    panel.pointerMoved({10, 2}, Qt::NoButton);
    QVERIFY(panel.isShown());
    // The folder view hides it at once.
    panel.setFolderViewActive(true);
    QVERIFY(!panel.isShown());
    QVERIFY(!panel.isAnimated());
  }

  void hiddenStripPrefetchesWhileThePanelCanShow() {
    ThumbnailListModel model;
    UiSettingsSnapshot settings = panelSettings(false, Enums::PanelPosition::Top);
    settings.panel.fullscreenOnly = true;
    ThumbnailPanelController panel(model, settings);
    prepare(model, 100, false);
    QSignalSpy requests(&model, &ThumbnailListModel::thumbnailsNeeded);
    // A fullscreen-only panel cannot appear in a window.
    panel.setWindowSize(kLargeWindow);
    QTest::qWait(ThumbnailListModel::kInactivePrefetchDelayMs + kWaitMarginMs);
    QCOMPARE(requests.count(), 0);
    panel.setFullscreen(true);
    QTRY_COMPARE(requests.count(), 1);
    QCOMPARE(requestedIndices(requests.first()).count(), kVisibleItems);
  }

  void pinButtonPublishesThePinnedState() {
    ThumbnailListModel model;
    ThumbnailPanelController panel(model, panelSettings(false, Enums::PanelPosition::Bottom));
    panel.setWindowSize(kLargeWindow);
    QSignalSpy pins(&panel, &ThumbnailPanelController::pinRequested);
    panel.togglePinned();
    QCOMPARE(pins.count(), 1);
    QCOMPARE(pins.first().at(0).toBool(), true);
    QVERIFY(panel.isPinned());
    QVERIFY(panel.isDocked());
  }

  void masonryButtonPublishesTheModeAndSwitchesTheModel() {
    ThumbnailListModel model;
    UiSettingsSnapshot settings = panelSettings(false, Enums::PanelPosition::Bottom);
    settings.panel.previewsSize = kDefaultPreviewSize;
    ThumbnailPanelController panel(model, settings);
    QSignalSpy requests(&panel, &ThumbnailPanelController::masonryRequested);
    QVERIFY(!panel.layout().masonry);
    QVERIFY(!model.imageSizesWanted());
    QVERIFY(!model.scrollConfig().rows);
    QCOMPARE(model.requestConfig().targetSize, QSize(256, 192));

    panel.toggleMasonry();
    QCOMPARE(requests.count(), 1);
    QCOMPARE(requests.first().at(0).toBool(), true);
    QVERIFY(panel.layout().masonry);
    QVERIFY(model.imageSizesWanted());
    // The thumbnails are made in the box of the lane: 4 tiles of the lane
    // wide, so that a 16:9 image is 256 x 144 and drawn without a resize.
    QCOMPARE(model.requestConfig().targetSize, QSize(576, 144));
    QCOMPARE(model.requestConfig().folderTargetSize, QSize(192, 144));
    QVERIFY(model.scrollConfig().rows);

    panel.toggleMasonry();
    QCOMPARE(requests.count(), 2);
    QCOMPARE(requests.last().at(0).toBool(), false);
    QVERIFY(!panel.layout().masonry);
    QVERIFY(!model.imageSizesWanted());
    QVERIFY(!model.scrollConfig().rows);
    QCOMPARE(model.requestConfig().targetSize, QSize(256, 192));
    QVERIFY(!model.requestConfig().folderTargetSize.isValid());
  }

  void previewSizeButtonsStepClampAndPublish() {
    constexpr int kBoundaryOffsetPx = SettingsScales::kPanelSizePixelsPerStep;
    ThumbnailListModel model;
    UiSettingsSnapshot settings = panelSettings(true, Enums::PanelPosition::Bottom);
    settings.panel.previewsSize = kDefaultPreviewSize;
    ThumbnailPanelController panel(model, settings);
    QSignalSpy changed(&panel, &ThumbnailPanelController::previewSizeChanged);
    QSignalSpy requested(&panel, &ThumbnailPanelController::previewSizeRequested);

    QCOMPARE(panel.previewSize(), kDefaultPreviewSize);
    QVERIFY(panel.canIncreasePreviewSize());
    QVERIFY(panel.canDecreasePreviewSize());

    panel.increasePreviewSize();
    const int steppedUp = kDefaultPreviewSize + ThumbnailPanelController::kPreviewSizeStepPx;
    QCOMPARE(panel.previewSize(), steppedUp);
    QCOMPARE(panel.layout().thumbnailSize, steppedUp);
    QCOMPARE(changed.count(), 1);
    QCOMPARE(requested.count(), 1);
    QCOMPARE(requested.first().at(0).toInt(), steppedUp);

    panel.decreasePreviewSize();
    QCOMPARE(panel.previewSize(), kDefaultPreviewSize);
    QCOMPARE(panel.layout().thumbnailSize, kDefaultPreviewSize);
    QCOMPARE(changed.count(), 2);
    QCOMPARE(requested.count(), 2);
    QCOMPARE(requested.last().at(0).toInt(), kDefaultPreviewSize);

    // applySettings updates previewSizeChanged without emitting previewSizeRequested.
    const int reqCountBeforeApply = requested.count();
    settings.panel.previewsSize = SettingsScales::kMinPanelSizePixels + kBoundaryOffsetPx;
    panel.applySettings(settings);
    QCOMPARE(changed.count(), 3);
    QCOMPARE(requested.count(), reqCountBeforeApply);
    QVERIFY(panel.canDecreasePreviewSize());
    panel.decreasePreviewSize();
    QCOMPARE(panel.previewSize(), SettingsScales::kMinPanelSizePixels);
    QVERIFY(!panel.canDecreasePreviewSize());
    const int reqCountAtMin = requested.count();
    panel.decreasePreviewSize();
    QCOMPARE(requested.count(), reqCountAtMin);
    QCOMPARE(panel.previewSize(), SettingsScales::kMinPanelSizePixels);

    // Sub-minimum legacy values cannot decrease, and increase onto the step grid.
    settings.panel.previewsSize = kThumbnailSize;
    panel.applySettings(settings);
    QVERIFY(!panel.canDecreasePreviewSize());
    panel.decreasePreviewSize();
    QCOMPARE(panel.previewSize(), kThumbnailSize);
    panel.increasePreviewSize();
    QCOMPARE(panel.previewSize(), SettingsScales::kMinPanelSizePixels);

    // Clamps at the maximum and disables further increase.
    settings.panel.previewsSize = SettingsScales::kMaxPanelSizePixels - kBoundaryOffsetPx;
    panel.applySettings(settings);
    QVERIFY(panel.canIncreasePreviewSize());
    panel.increasePreviewSize();
    QCOMPARE(panel.previewSize(), SettingsScales::kMaxPanelSizePixels);
    QVERIFY(!panel.canIncreasePreviewSize());
    const int reqCountAtMax = requested.count();
    panel.increasePreviewSize();
    QCOMPARE(requested.count(), reqCountAtMax);
  }

  void previewSizeButtonsUpdateMasonryLayout() {
    ThumbnailListModel model;
    ThumbnailPanelController panel(model, masonrySettings());
    model.populate(2, 0, {kWideImage, kClassicImage});
    const int initialExtent = panel.layout().panelExtent;
    const QSize initialTarget = model.requestConfig().targetSize;
    const int steppedUp = kDefaultPreviewSize + ThumbnailPanelController::kPreviewSizeStepPx;
    panel.increasePreviewSize();
    QCOMPARE(panel.previewSize(), steppedUp);
    QVERIFY(panel.layout().panelExtent > initialExtent);
    QVERIFY(panel.masonry()->layout() != nullptr);
    QCOMPARE(model.scrollConfig().thumbnailSize, steppedUp);
    QVERIFY(model.requestConfig().targetSize.width() > initialTarget.width());
    QVERIFY(model.requestConfig().targetSize.height() > initialTarget.height());
  }

  void masonryBoxFollowsTheDevicePixelRatio() {
    ThumbnailListModel model;
    ThumbnailPanelController panel(model, masonrySettings());
    panel.setDevicePixelRatio(1.25);
    QCOMPARE(model.requestConfig().targetSize, QSize(720, 180));
    QCOMPARE(model.requestConfig().folderTargetSize, QSize(240, 180));
  }

  void stripMasonryRequestsPerItemTileSizes() {
    ThumbnailListModel model;
    UiSettingsSnapshot settings = masonrySettings();
    settings.panel.autoThumbnailResolution = true;
    ThumbnailPanelController panel(model, settings);
    panel.setDevicePixelRatio(1.5);
    QSignalSpy requests(&model, &ThumbnailListModel::thumbnailsNeeded);
    model.populate(3, 1, {QSize(), kSquareImage, QSize(4000, 1000)});
    model.setViewport(0.0, kViewExtent);
    model.setActive(true);

    QCOMPARE(requests.count(), 1);
    const auto batch = requests.first().at(0).value<QList<ThumbnailItemRequest>>();
    QCOMPARE(batch.size(), 3);
    QCOMPARE(batch.at(0).targetSize, QSize(288, 216));
    QCOMPARE(batch.at(1).targetSize, QSize(216, 216));
    QCOMPARE(batch.at(2).targetSize, QSize(864, 216));
  }

  void stripMasonryUsesSharedBoxWhenAutoIsOff() {
    ThumbnailListModel model;
    UiSettingsSnapshot settings = masonrySettings();
    settings.panel.autoThumbnailResolution = false;
    ThumbnailPanelController panel(model, settings);
    panel.setDevicePixelRatio(1.5);
    QSignalSpy requests(&model, &ThumbnailListModel::thumbnailsNeeded);
    model.populate(3, 1, {QSize(), kSquareImage, QSize(4000, 1000)});
    QVERIFY(model.requestConfig().itemTargetSizes.isEmpty());
    model.setViewport(0.0, kViewExtent);
    model.setActive(true);

    QCOMPARE(requests.count(), 1);
    const auto batch = requests.first().at(0).value<QList<ThumbnailItemRequest>>();
    QCOMPARE(batch.size(), 3);
    QCOMPARE(batch.at(0).targetSize, QSize(288, 216));
    QCOMPARE(batch.at(1).targetSize, QSize(864, 216));
    QCOMPARE(batch.at(2).targetSize, QSize(864, 216));
  }

  void autoCacheResolutionToggleReloadsThumbnails() {
    ThumbnailListModel model;
    QSignalSpy requests(&model, &ThumbnailListModel::thumbnailsNeeded);
    prepare(model, 2);
    model.setThumbnail(0, entryOf(100, 75));
    QVERIFY(isLoaded(model, 0));
    requests.clear();
    model.configure({.targetSize = kRequestSize, .autoCacheResolution = true});
    QVERIFY(!isLoaded(model, 0));
    QCOMPARE(requests.count(), 1);
    QCOMPARE(requestedIndices(requests.first()), (QList<int>{0, 1}));
  }

  void masonryRowsFollowTheSizesOfTheImages() {
    ThumbnailListModel model;
    ThumbnailPanelController panel(model, masonrySettings());
    model.populate(3, 0, {kWideImage, kClassicImage, QSize()});

    auto layout = panel.masonry()->layout();
    QVERIFY(layout);
    QCOMPARE(layout->itemCount(), 3);
    // A 16:9 thumbnail is as wide as the preview size.
    QCOMPARE(layout->itemRect(0).size(), QSizeF(256, 144));
    QCOMPARE(layout->itemRect(1).size(), QSizeF(192, 144));
    // The size of the third image is not known yet.
    QCOMPARE(layout->itemRect(2).width(), qreal(192));
    // The model scrolls along the rows of the layout.
    QCOMPARE(static_cast<const ItemRows *>(layout.get()), model.scrollConfig().rows.get());

    // A size that arrives later changes the layout in a step.
    model.setSourceSize(2, QSize(2000, 1000));
    QTRY_VERIFY(panel.masonry()->layout() &&
                panel.masonry()->layout()->itemRect(2).width() == qreal(288));
    const auto updated = panel.masonry()->layout();
    QVERIFY(updated);
    QCOMPARE(static_cast<const ItemRows *>(updated.get()), model.scrollConfig().rows.get());
  }

  void masonryStripKeepsTheSelectedImageInPlaceWhenSizesArrive() {
    ThumbnailListModel model;
    ThumbnailPanelController panel(model, masonrySettings());
    constexpr int kCount = 40;
    constexpr int kSelected = 20;
    QList<QSize> sizes(kCount, kClassicImage);
    model.populate(kCount, 0, sizes);
    model.setViewport(0.0, kViewExtent);
    model.setActive(true);
    model.select(kSelected);
    model.focusOnSelection();
    QVERIFY(model.viewOffset() > 0.0);
    const auto before = panel.masonry()->layout();
    QVERIFY(before);
    const qreal startBefore = before->rowStart(kSelected) - model.viewOffset();

    // The images in front of the selection turn out to be wider.
    QList<QPair<int, QSize>> wider;
    for (int item = 0; item < kSelected; ++item)
      wider.append({item, kWideImage});
    model.setSourceSizes(wider);
    QTRY_VERIFY(panel.masonry()->layout() != before);
    const auto after = panel.masonry()->layout();
    QVERIFY(after);
    QVERIFY(after->rowStart(kSelected) > before->rowStart(kSelected));
    // The selected image stays where it was in the view.
    QVERIFY(qAbs((after->rowStart(kSelected) - model.viewOffset()) - startBefore) <=
            StripMasonryController::kAnchorTolerancePx);
  }

  void masonryItemAtFindsTheTileUnderThePointer() {
    ThumbnailListModel model;
    ThumbnailPanelController panel(model, masonrySettings());
    model.populate(2, 0, {kWideImage, kSquareImage});
    const auto layout = panel.masonry()->layout();
    QVERIFY(layout);
    const QRectF second = layout->itemRect(1);
    QCOMPARE(panel.masonry()->itemAt(second.center().x(), second.center().y()), 1);
    QCOMPARE(panel.masonry()->itemAt(-10.0, second.center().y()), -1);
  }

  void masonryViewStaysInsideTheContentWhenItShrinks() {
    ThumbnailListModel model;
    ThumbnailPanelController panel(model, masonrySettings());
    constexpr int kCount = 40;
    constexpr int kRemoved = 20;
    model.populate(kCount, 0, QList<QSize>(kCount, kClassicImage));
    model.setViewport(0.0, kViewExtent);
    model.setActive(true);
    // The view is at the end of the strip.
    model.restoreOffset(panel.masonry()->contentExtent());
    QVERIFY(model.viewOffset() > 0.0);
    // The last items go, one after the other: the view must not be left past
    // the end of what remains, whichever of them anchored it.
    for (int removed = 0; removed < kRemoved; ++removed) {
      model.removeItem(model.rowCount() - 1);
      const qreal maximum =
          qMax(0.0, panel.masonry()->contentExtent() - static_cast<qreal>(kViewExtent));
      QVERIFY2(model.viewOffset() <= maximum + StripMasonryController::kAnchorTolerancePx,
               qPrintable(QStringLiteral("offset %1, end %2 after %3 removed")
                              .arg(model.viewOffset()).arg(maximum).arg(removed + 1)));
    }
  }

  void masonryToggleBringsTheSelectionIntoView() {
    ThumbnailListModel model;
    UiSettingsSnapshot settings = panelSettings(false, Enums::PanelPosition::Bottom);
    settings.panel.previewsSize = kDefaultPreviewSize;
    ThumbnailPanelController panel(model, settings);
    constexpr int kCount = 60;
    constexpr int kSelected = 40;
    model.populate(kCount, 0, QList<QSize>(kCount, kWideImage));
    model.setViewport(0.0, kViewExtent);
    model.setActive(true);
    model.select(kSelected);
    model.focusOnSelection();
    QVERIFY(model.viewOffset() > 0.0);

    panel.toggleMasonry();
    const auto layout = panel.masonry()->layout();
    QVERIFY(layout);
    const qreal start = layout->rowStart(kSelected);
    QVERIFY2(start >= model.viewOffset() &&
                 start + layout->rowExtent(kSelected) <= model.viewOffset() + kViewExtent,
             qPrintable(QStringLiteral("selection at %1, view %2 - %3")
                            .arg(start).arg(model.viewOffset())
                            .arg(model.viewOffset() + kViewExtent)));

    panel.toggleMasonry();
    const qreal uniformStart = kSelected * panel.layout().itemExtent();
    QVERIFY(uniformStart >= model.viewOffset() &&
            uniformStart + panel.layout().itemExtent() <= model.viewOffset() + kViewExtent);
  }

  void populatingMakesTheLayoutOnce() {
    ThumbnailListModel model;
    ThumbnailPanelController panel(model, masonrySettings());
    QSignalSpy layouts(panel.masonry(), &StripMasonryController::layoutChanged);
    // The reset, the count and the folders of a population are one layout.
    model.populate(30, 2, QList<QSize>(30, kWideImage));
    QCOMPARE(layouts.count(), 1);
    // Items that come or go are another.
    model.removeItem(29);
    QCOMPARE(layouts.count(), 2);
  }

  void masonryModeIsOffWithoutTheSetting() {
    ThumbnailListModel model;
    ThumbnailPanelController panel(model, panelSettings(false, Enums::PanelPosition::Bottom));
    model.populate(2, 0, {kWideImage, kSquareImage});
    QVERIFY(!panel.masonry()->isEnabled());
    QVERIFY(!panel.masonry()->layout());
    QVERIFY(panel.masonry()->isReady());
  }

  void masonryWaitsForTheSizesAroundTheSelectionBeforeItIsReady() {
    ThumbnailListModel model;
    ThumbnailPanelController panel(model, masonrySettings());
    QSignalSpy ready(panel.masonry(), &StripMasonryController::readyChanged);
    model.populate(4, 0, {});
    QVERIFY(!panel.masonry()->isReady());
    model.setSourceSizes({{0, kWideImage}, {1, kWideImage}, {2, kWideImage}, {3, kWideImage}});
    QVERIFY(panel.masonry()->isReady());
    QVERIFY(ready.count() >= 2);
  }

  void masonryGivesUpWaitingForSizesAfterTheGracePeriod() {
    static_assert(kTryTimeoutMs > StripMasonryController::kGracePeriodMs);
    ThumbnailListModel model;
    ThumbnailPanelController panel(model, masonrySettings());
    model.populate(4, 0, {});
    QVERIFY(!panel.masonry()->isReady());
    QTRY_VERIFY_WITH_TIMEOUT(panel.masonry()->isReady(), kTryTimeoutMs);
  }

  void matchesThumbnailsByTheTargetBoxNotByTheirPixels() {
    ThumbnailListModel model;
    QSignalSpy ready(&model, &ThumbnailListModel::visibleThumbnailsReady);
    prepare(model, 4);
    // A thumbnail whose pixels are not the box (a square image, a folder
    // icon, a cropped image) is the answer to the request it is labelled
    // with.
    model.setThumbnail(0, entryOf(75, 75));
    model.setThumbnail(1, entryOf(100, 100));
    QVERIFY(isLoaded(model, 0));
    QVERIFY(isLoaded(model, 1));
    // The box of another request is rejected whatever its pixels are, also
    // when only one side or only the order of the sides differs.
    model.setThumbnail(2, entryOf(100, 75, QSize(100, 100)));
    model.setThumbnail(2, entryOf(100, 75, QSize(75, 100)));
    model.setThumbnail(2, entryOf(100, 75, QSize(100, 76)));
    QVERIFY(!isLoaded(model, 2));
    model.setThumbnailUnavailable(2, QSize(100, 100));
    QVERIFY(!model.data(model.index(2), ThumbnailListModel::UnavailableRole).toBool());
    QCOMPARE(ready.count(), 0);
    model.setThumbnail(2, entryOf(100, 75));
    model.setThumbnailUnavailable(3, kRequestSize);
    QCOMPARE(ready.count(), 1);
  }

  void changingTheTargetBoxKeepsThumbnailsAndRequestsAgain() {
    ThumbnailListModel model;
    QSignalSpy requests(&model, &ThumbnailListModel::thumbnailsNeeded);
    prepare(model, 2);
    QCOMPARE(requests.count(), 1);
    model.setThumbnail(0, entryOf(100, 75));
    QVERIFY(isLoaded(model, 0));
    // Same width, other height: the loaded thumbnail stays (the view scales
    // it) and the new box is requested after the load delay.
    const QSize taller(kRequestSize.width(), kRequestSize.height() + 1);
    model.configure({.targetSize = taller});
    QVERIFY(isLoaded(model, 0));
    QCOMPARE(requests.count(), 1);
    QTRY_COMPARE(requests.count(), 2);
    const auto updatedBatch = requests.last().at(0).value<QList<ThumbnailItemRequest>>();
    QCOMPARE(updatedBatch.first().targetSize, taller);
    QVERIFY(requestedIndices(requests.last()).contains(0));
    // The answer to the old request arrives late and is ignored.
    model.setThumbnail(1, entryOf(100, 75));
    QVERIFY(!isLoaded(model, 1));
    model.setThumbnail(1, entryOf(100, 75, taller));
    QVERIFY(isLoaded(model, 1));
  }


  void panelConfiguresTheModel() {
    ThumbnailListModel model;
    UiSettingsSnapshot settings = panelSettings(false, Enums::PanelPosition::Left);
    settings.panel.unloadThumbnails = true;
    settings.panel.centerSelection = true;
    ThumbnailPanelController panel(model, settings);
    panel.setDevicePixelRatio(1.5);
    // The image area of an 80 px cell is 80 x 60; the box is that at 1.5.
    QCOMPARE(model.requestConfig().targetSize, QSize(120, 90));
    // The strip never crops.
    QVERIFY(!model.requestConfig().crop);
    QVERIFY(model.requestConfig().unloadOffscreen);
    QVERIFY(!model.scrollConfig().horizontal);
    QCOMPARE(model.scrollConfig().itemExtent, panel.layout().cellHeight);
    QVERIFY(model.scrollConfig().centerSelection);
  }

  void slideAnimationBlocksThumbnailRequests() {
    ThumbnailListModel model;
    ThumbnailPanelController panel(model, panelSettings(false, Enums::PanelPosition::Left));
    prepare(model, 3, false);
    model.configure({.targetSize = kRequestSize});
    QSignalSpy requests(&model, &ThumbnailListModel::thumbnailsNeeded);
    panel.setAnimationRunning(true);
    model.setActive(true);
    QCOMPARE(requests.count(), 0);
    panel.setAnimationRunning(false);
    QCOMPARE(requests.count(), 1);
  }

  void labelTextContrastsWithItsBackground() {
    ThumbnailListModel model;
    const ThumbnailPanelController panel(model, panelSettings(false, Enums::PanelPosition::Top));
    QCOMPARE(panel.labelTextColor(QColor(Qt::white)), QColor(Qt::black));
    QCOMPARE(panel.labelTextColor(QColor(32, 32, 32)), QColor(Qt::white));
  }

  void targetSizeIsTheImageAreaTimesTheDevicePixelRatio() {
    const ThumbnailStripLayout layout = thumbnailStripLayoutFor(
        {.previewsSize = 120, .style = Enums::PanelStyle::Simple,
         .position = Enums::PanelPosition::Bottom, .textHeight = kTextHeight});
    QCOMPARE(layout.imageArea, QSize(120, 90));
    QCOMPARE(layout.imageTargetSize(1.0), QSize(120, 90));
    QCOMPARE(layout.imageTargetSize(2.0), QSize(240, 180));
    // Both sides are rounded, not truncated: 120 * 1.25 = 150, 90 * 1.25 =
    // 112.5.
    QCOMPARE(layout.imageTargetSize(1.25), QSize(150, 113));
    // The image area height is rounded first: 101 * 3 / 4 = 75.75.
    const ThumbnailStripLayout odd = thumbnailStripLayoutFor(
        {.previewsSize = 101, .style = Enums::PanelStyle::Simple,
         .position = Enums::PanelPosition::Bottom, .textHeight = kTextHeight});
    QCOMPARE(odd.imageArea, QSize(101, 76));
    QCOMPARE(odd.imageTargetSize(1.5), QSize(152, 114));
  }

  //--- adapter ----------------------------------------------------------------
  void adapterPublishesThumbnailsAndRequests() {
    DirectoryViewAdapter adapter;
    IDirectoryView &view = adapter;
    QSignalSpy requests(&adapter, &DirectoryViewAdapter::thumbnailsRequested);
    QSignalSpy activated(&adapter, &DirectoryViewAdapter::itemActivated);
    adapter.configure({.targetSize = kRequestSize});
    adapter.setScrollConfig({.itemExtent = kItemExtent, .thumbnailSize = kThumbnailSize});
    view.populate(3);
    view.setDirCount(1);
    adapter.setViewport(0.0, kViewExtent);
    adapter.setActive(true);
    QCOMPARE(requests.count(), 1);

    QImage image(40, 30, QImage::Format_RGB32);
    image.fill(Qt::blue);
    view.setThumbnail(2, std::make_shared<Thumbnail>(u"a.png"_s, u"40x30"_s, kRequestSize, image));
    QVERIFY(isLoaded(adapter, 2));
    QCOMPARE(adapter.data(adapter.index(2), ThumbnailListModel::NameRole).toString(), u"a.png"_s);
    const auto handle =
        adapter.data(adapter.index(2), ThumbnailListModel::ThumbnailRole).value<ThumbnailHandle>();
    QCOMPARE(handle.image.cacheKey(), image.cacheKey());
    QVERIFY(adapter.data(adapter.index(0), ThumbnailListModel::DirectoryRole).toBool());
    QVERIFY(!adapter.data(adapter.index(1), ThumbnailListModel::DirectoryRole).toBool());

    // A folder icon is square-ish in pixels but labelled with the box it was
    // requested for: the adapter hands that box to the model.
    QImage icon(60, 60, QImage::Format_ARGB32);
    icon.fill(Qt::gray);
    view.setThumbnail(0, std::make_shared<Thumbnail>(u"dir"_s, u"Folder"_s, kRequestSize, icon));
    QVERIFY(isLoaded(adapter, 0));
    view.setThumbnailUnavailable(1, kRequestSize);
    QVERIFY(adapter.data(adapter.index(1), ThumbnailListModel::UnavailableRole).toBool());

    adapter.press(1, Qt::LeftButton, Qt::NoModifier, {});
    QCOMPARE(activated.count(), 1);
    QCOMPARE(activated.first().at(0).toInt(), 1);
    view.select(QList<int>{2});
    QCOMPARE(view.selection(), QList<int>{2});
  }
};

int runThumbnailStripTests(int argc, char **argv) {
  ThumbnailStripTests tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "tst_thumbnailstrip.moc"
