#include "foldergridcontroller.h"

#include <QDebug>
#include <QFontMetrics>
#include <QtGlobal>

#include <algorithm>
#include <cmath>

FolderGridController::FolderGridController(ThumbnailListModel &model,
                                           const UiSettingsSnapshot &settings, QObject *parent)
    : QObject(parent),
      mModel(model),
      mFolderView(settings.folderView),
      mPanel(settings.panel),
      mViewer(settings.viewer),
      mMasonry(settings.folderView.masonry),
      mIconSize(boundedFolderIconSize(settings.folderView.iconSize)) {
    mModel.setItemActivation(ThumbnailListModel::ItemActivation::OnDoubleClick);
    mModel.setImageSizesWanted(mMasonry);
    mTiles.setSource(&mModel);
    mLayoutTimer.setSingleShot(true);
    connect(&mLayoutTimer, &QTimer::timeout, this, &FolderGridController::updateLayout);
    connect(&mModel, &ThumbnailListModel::sourceSizesChanged, this,
            &FolderGridController::scheduleLayout);
    // The folders form the strip of the masonry layout.
    connect(&mModel, &ThumbnailListModel::dirCountChanged, this,
            &FolderGridController::updateLayout);
    connect(&mModel, &ThumbnailListModel::selectionChanged, this,
            &FolderGridController::updateSelectedImageCount);
    connect(&mModel, &ThumbnailListModel::dirCountChanged, this,
            &FolderGridController::updateSelectedImageCount);
    // The centring of a row depends on whether the directory fills one.
    connect(&mModel, &ThumbnailListModel::countChanged, this,
            &FolderGridController::updateLayout);
    // Any activation ends the column memory of Up / Down.
    connect(&mModel, &ThumbnailListModel::activated, this, [this]() { resetColumnMemory(); });
    updateLayout();
}

ThumbnailListModel *FolderGridController::model() const {
    return &mModel;
}

FolderGridLayout FolderGridController::layout() const {
    return mLayout;
}

int FolderGridController::iconSize() const {
    return mIconSize;
}

int FolderGridController::zoomSliderValue() const {
    return static_cast<int>(std::lround(std::log2(mIconSize) * kSliderScale));
}

int FolderGridController::zoomSliderMinimum() {
    return static_cast<int>(std::lround(std::log2(FolderGridLayout::kMinimumIconSize) * kSliderScale));
}

int FolderGridController::zoomSliderMaximum() {
    return static_cast<int>(std::lround(std::log2(FolderGridLayout::kMaximumIconSize) * kSliderScale));
}

QRectF FolderGridController::rubberBand() const {
    return mRubberBand;
}

int FolderGridController::selectedImageCount() const {
    return mSelectedImageCount;
}

bool FolderGridController::batchListPending() const {
    return mBatchListPending;
}

void FolderGridController::setBatchListPending(bool pending) {
    if (mBatchListPending == pending)
        return;
    mBatchListPending = pending;
    emit batchListPendingChanged();
}

bool FolderGridController::masonryEnabled() const {
    return mMasonry;
}

qreal FolderGridController::masonryContentHeight() const {
    return mMasonryLayout ? mMasonryLayout->contentExtent() : 0.0;
}

MasonryTilesModel *FolderGridController::tiles() {
    return &mTiles;
}

void FolderGridController::setMasonryEnabled(bool enabled) {
    if (enabled == mMasonry)
        return;
    mMasonry = enabled;
    mFolderView.masonry = enabled;
    // Only a view in the masonry layout needs the sizes of the images.
    mModel.setImageSizesWanted(enabled);
    resetColumnMemory();
    emit masonryEnabledChanged();
    updateLayout();
}

//------------------------------------------------------------------------------
// Configuration

void FolderGridController::applySettings(const UiSettingsSnapshot &settings) {
    mFolderView = settings.folderView;
    mPanel = settings.panel;
    mViewer = settings.viewer;
    setMasonryEnabled(settings.folderView.masonry);
    // FolderView::readSettings() applies the stored icon size.
    const int size = boundedFolderIconSize(settings.folderView.iconSize);
    if (size != mIconSize) {
        mIconSize = size;
        emit iconSizeChanged();
    }
    updateLayout();
}

void FolderGridController::setLabelFont(const QFont &font) {
    mLabelFont = font;
    updateLayout();
}

void FolderGridController::setDevicePixelRatio(qreal ratio) {
    if (ratio <= 0.0) {
        qWarning() << "FolderGridController ignores the device pixel ratio" << ratio;
        return;
    }
    mDevicePixelRatio = ratio;
    configureModel();
}

void FolderGridController::setViewWidth(qreal width) {
    const int viewWidth = qMax(0, static_cast<int>(width));
    if (viewWidth == mViewWidth)
        return;
    mViewWidth = viewWidth;
    updateLayout();
}

void FolderGridController::setIconSize(int size) {
    const int bounded = boundedFolderIconSize(size);
    if (bounded == mIconSize)
        return;
    mIconSize = bounded;
    emit iconSizeChanged();
    updateLayout();
    if (!mSliderPressed)
        emit iconSizeCommitted(mIconSize);
}

void FolderGridController::resetIconSize() {
    setIconSize(FolderGridLayout::kDefaultIconSize);
}

// FolderView::onZoomSliderValueChanged().
void FolderGridController::setZoomSliderValue(int value) {
    const int size = static_cast<int>(std::lround(std::pow(2.0, value / static_cast<double>(kSliderScale))));
    setIconSize(size);
}

void FolderGridController::setZoomSliderPressed(bool pressed) {
    if (mSliderPressed == pressed)
        return;
    mSliderPressed = pressed;
    if (!pressed)
        emit iconSizeCommitted(mIconSize);
}

QColor FolderGridController::labelTextColor(const QColor &background) {
    return thumbnailLabelColor(background);
}

void FolderGridController::updateLayout() {
    mLayoutTimer.stop();
    // The folder strip of the masonry layout centres by the folders alone.
    const FolderGridLayout layout = folderGridLayoutFor({
        .viewWidth = mViewWidth,
        .iconSize = mIconSize,
        .itemCount = mMasonry ? mModel.dirCount() : mModel.itemCount(),
        .textHeight = QFontMetrics(mLabelFont).height(),
    });
    if (layout != mLayout) {
        mLayout = layout;
        emit layoutChanged();
    }
    // The row at the top of the view, to put back where it was when the
    // rows before it change in size.
    const std::shared_ptr<const MasonryLayout> previous = mMasonryLayout;
    const qreal offset = mModel.viewOffset();
    const int anchorRow = previous && offset > 0.0 ? previous->rowAt(offset) : -1;
    const int anchorItem = anchorRow >= 0 ? previous->firstItem(anchorRow) : -1;
    const qreal anchorDistance = anchorRow >= 0 ? previous->rowStart(anchorRow) - offset : 0.0;

    rebuildMasonry();
    configureModel();

    if (anchorItem >= 0 && mMasonryLayout) {
        const qreal start = mMasonryLayout->rowStart(mMasonryLayout->rowOf(anchorItem));
        const qreal target = start - anchorDistance;
        if (std::abs(target - mModel.viewOffset()) > kAnchorToleranceDevicePx)
            mModel.restoreOffset(target);
    }
}

void FolderGridController::rebuildMasonry() {
    if (!mMasonry) {
        if (!mMasonryLayout)
            return;
        mMasonryLayout.reset();
        mTiles.setLayout(nullptr, mLayout.cell);
        emit masonryLayoutChanged();
        return;
    }
    mMasonryLayout = MasonryLayout::create({
        .viewWidth = mViewWidth,
        .targetRowHeight = MasonryLayout::targetRowHeightFor(mIconSize),
        .itemCount = mModel.itemCount(),
        .dirCount = mModel.dirCount(),
        .folders = mLayout,
        .sourceSizes = mModel.sourceSizes(),
    });
    mTiles.setLayout(mMasonryLayout, mLayout.cell);
    emit masonryLayoutChanged();
}

// Sizes arrive in a stream; the layout follows them in steps.
void FolderGridController::scheduleLayout() {
    if (!mMasonry || mLayoutTimer.isActive())
        return;
    mLayoutTimer.start(kLayoutDelayMs);
}

// The grid requests thumbnails for the image area times the device pixel
// ratio and never crops; the masonry layout for a box that fits the usual
// proportions of its tiles. The rows of the masonry layout replace the
// uniform ones, which are left empty so that only the rows differ while the
// layout follows the sizes of the images.
void FolderGridController::configureModel() {
    const bool masonry = mMasonry && mMasonryLayout;
    const QSize target = masonry ? MasonryLayout::thumbnailBoxFor(
                                       MasonryLayout::targetRowHeightFor(mIconSize),
                                       mDevicePixelRatio)
                                 : mLayout.cell.imageTargetSize(mDevicePixelRatio);
    std::shared_ptr<const ItemRows> rows;
    if (masonry)
        rows = mMasonryLayout;
    mModel.configure({
        .targetSize = target,
        .crop = false,
        .unloadOffscreen = mPanel.unloadThumbnails,
        .cacheResolution = mPanel.thumbnailResolution,
    });
    mModel.setScrollConfig({
        .horizontal = false,
        .itemExtent = masonry ? 0 : mLayout.cellHeight(),
        .columns = masonry ? 1 : mLayout.columns,
        .leadingSpace = masonry ? 0 : mLayout.top,
        .rows = rows,
        .preloadDistance = kPreloadDistance,
        .focusShowsNeighbours = false,
        .thumbnailSize = mIconSize,
        .centerSelection = false,
        .smoothScroll = mViewer.smoothScroll,
        .wheelSpeed = mViewer.mouseScrollingSpeed,
        .trackpadDetection = mViewer.trackpadDetection,
    });
}

void FolderGridController::updateSelectedImageCount() {
    const int dirCount = mModel.dirCount();
    const QList<int> selection = mModel.selection();
    const int count = static_cast<int>(
        std::ranges::count_if(selection, [dirCount](int index) { return index >= dirCount; }));
    if (count == mSelectedImageCount)
        return;
    mSelectedImageCount = count;
    emit selectedImageCountChanged();
}

//------------------------------------------------------------------------------
// Pointer

void FolderGridController::press(int index, int button, int modifiers, QPointF position) {
    mNavigationX = -1.0;
    mModel.press(index, button, modifiers, position);
    const auto mouseButton = static_cast<Qt::MouseButton>(button);
    if (mouseButton != Qt::LeftButton || checkRange(index))
        return;
    // The background starts a rubber band from the selection the press left.
    mRubberBandActive = true;
    mRubberBandOrigin = QPointF(position.x(), position.y() + mModel.viewOffset());
    mRubberBandStartSelection = mModel.selection();
}

void FolderGridController::move(QPointF position, int buttons, int modifiers) {
    mModel.move(position, buttons);
    if (mRubberBandActive && Qt::MouseButtons::fromInt(buttons) & Qt::LeftButton)
        updateRubberBand(position, Qt::KeyboardModifiers::fromInt(modifiers));
}

void FolderGridController::release(int index, int button, QPointF position) {
    const auto mouseButton = static_cast<Qt::MouseButton>(button);
    const bool gesture = mModel.gestureActive();
    mModel.release(index, button, position);
    if (mouseButton == Qt::LeftButton)
        endRubberBand();
    else if (mouseButton == Qt::RightButton && !gesture)
        emit contextMenuRequested(position);
}

void FolderGridController::doubleClick(int index, int button) {
    mModel.doubleClick(index, button);
}

int FolderGridController::masonryItemAt(qreal x, qreal contentY) const {
    if (!mMasonryLayout)
        return -1;
    const int item = mMasonryLayout->itemAt(QPointF(x, contentY));
    return checkRange(item) ? item : -1;
}

// FolderGridView::wheelEvent().
void FolderGridController::wheel(QPoint angleDelta, QPoint pixelDelta, int modifiers) {
    if (!(Qt::KeyboardModifiers::fromInt(modifiers) & Qt::ControlModifier)) {
        mModel.wheelScrolled(angleDelta, pixelDelta);
        return;
    }
    if (pixelDelta.y() > 0 || angleDelta.y() > 0)
        setIconSize(mIconSize + FolderGridLayout::kZoomStep);
    else if (pixelDelta.y() < 0 || angleDelta.y() < 0)
        setIconSize(mIconSize - FolderGridLayout::kZoomStep);
}

// ThumbnailView::rubberBandChanged handler: items whose cell the band
// touches are added to the selection held at its start; with Ctrl they
// toggle against it.
void FolderGridController::updateRubberBand(QPointF position, Qt::KeyboardModifiers modifiers) {
    const qreal offset = mModel.viewOffset();
    const QRectF content =
        QRectF(mRubberBandOrigin, QPointF(position.x(), position.y() + offset)).normalized();
    mRubberBand = content.translated(0.0, -offset);
    emit rubberBandChanged();

    QList<int> selection = mRubberBandStartSelection;
    const bool toggle = modifiers & Qt::ControlModifier;
    for (const int index : rubberBandHits(content)) {
        if (toggle && mRubberBandStartSelection.contains(index))
            selection.removeAll(index);
        else if (!selection.contains(index))
            selection.append(index);
    }
    std::ranges::sort(selection);
    if (selection != mModel.selection())
        mModel.select(selection);
}

QList<int> FolderGridController::rubberBandHits(const QRectF &content) const {
    QList<int> hits;
    if (mMasonryLayout) {
        for (const int index : mMasonryLayout->itemsIn(content)) {
            if (checkRange(index))
                hits.append(index);
        }
        return hits;
    }
    const int columns = qMax(1, mLayout.columns);
    const int cellHeight = qMax(1, mLayout.cellHeight());
    const int firstRow =
        qMax(0, static_cast<int>(std::floor((content.top() - mLayout.top) / cellHeight)));
    const int lastRow = static_cast<int>(std::floor((content.bottom() - mLayout.top) / cellHeight));
    for (int row = firstRow; row <= lastRow; ++row) {
        for (int column = 0; column < columns; ++column) {
            const int index = row * columns + column;
            if (!checkRange(index))
                break;
            if (mLayout.itemRect(index).intersects(content))
                hits.append(index);
        }
    }
    return hits;
}

void FolderGridController::endRubberBand() {
    if (!mRubberBandActive)
        return;
    mRubberBandActive = false;
    mRubberBandStartSelection.clear();
    if (mRubberBand.isNull())
        return;
    mRubberBand = {};
    emit rubberBandChanged();
}

//------------------------------------------------------------------------------
// Keyboard (FolderGridView::keyPressEvent())

bool FolderGridController::keyPressed(int key, int modifiers) {
    const auto keyboardModifiers = Qt::KeyboardModifiers::fromInt(modifiers);
    // ThumbnailView::keyPressEvent(): Shift anchors a range.
    if (key == Qt::Key_Shift) {
        mModel.beginRangeSelection();
        return true;
    }
    if ((keyboardModifiers & Qt::ShiftModifier) && !mModel.rangeSelectionActive())
        mModel.beginRangeSelection();
    return false;
}

void FolderGridController::activateCurrent() {
    mModel.activate(current());
}

void FolderGridController::keyReleased(int key) {
    if (key == Qt::Key_Shift)
        mModel.endRangeSelection();
}

void FolderGridController::focusLost() {
    mModel.endRangeSelection();
    endRubberBand();
}

bool FolderGridController::checkRange(int index) const {
    return index >= 0 && index < mModel.itemCount();
}

int FolderGridController::current() const {
    return mModel.currentIndex();
}

int FolderGridController::itemAbove(int index) const {
    const int above = index - qMax(1, mLayout.columns);
    return checkRange(above) ? above : index;
}

int FolderGridController::itemBelow(int index) const {
    return qMin(mModel.itemCount() - 1, index + qMax(1, mLayout.columns));
}

int FolderGridController::columnOf(int index) const {
    return checkRange(index) ? index % qMax(1, mLayout.columns) : -1;
}

bool FolderGridController::sameRow(int one, int two) const {
    if (!checkRange(one) || !checkRange(two))
        return false;
    const int columns = qMax(1, mLayout.columns);
    return one / columns == two / columns;
}

void FolderGridController::resetColumnMemory() {
    mShiftedColumn = -1;
    mNavigationX = -1.0;
}

// The first move of a series remembers the horizontal centre of the tile it
// leaves, and each move lands on the tile of the target row nearest to it, so
// that a column of tiles that happen to line up is followed.
void FolderGridController::moveByRows(int rows) {
    const int from = current();
    if (!mMasonryLayout || !checkRange(from))
        return;
    const int row = mMasonryLayout->rowOf(from);
    const int target = qBound(0, row + rows, mMasonryLayout->rowCount() - 1);
    if (target == row)
        return;
    if (mNavigationX < 0.0)
        mNavigationX = mMasonryLayout->itemRect(from).center().x();
    const int item = mMasonryLayout->itemInRow(target, mNavigationX);
    if (checkRange(item))
        selectOrExtend(item);
}

void FolderGridController::selectOrExtend(int index) {
    if (mModel.rangeSelectionActive())
        mModel.selectRangeTo(index);
    else
        mModel.select(index);
    mModel.scrollToItem(current());
}

void FolderGridController::selectAll() {
    QList<int> all;
    all.reserve(mModel.itemCount());
    for (int index = 0; index < mModel.itemCount(); ++index)
        all.append(index);
    // The current item goes last, so that it stays current.
    const int currentIndex = current();
    if (currentIndex >= 0)
        all.move(currentIndex, all.size() - 1);
    mModel.select(all);
}

void FolderGridController::selectAbove() {
    if (mMasonryLayout) {
        moveByRows(-1);
        return;
    }
    const int last = current();
    if (mModel.itemCount() == 0 || last < 0 || sameRow(0, last))
        return;
    int target = itemAbove(last);
    if (mShiftedColumn >= 0) {
        target += mShiftedColumn - columnOf(last);
        resetColumnMemory();
    }
    if (!checkRange(target))
        target = 0;
    selectOrExtend(target);
}

void FolderGridController::selectBelow() {
    if (mMasonryLayout) {
        moveByRows(1);
        return;
    }
    const int last = current();
    const int count = mModel.itemCount();
    if (count == 0 || last < 0 || sameRow(last, count - 1))
        return;
    resetColumnMemory();
    int target = itemBelow(last);
    if (!checkRange(target))
        target = count - 1;
    if (columnOf(target) != columnOf(last))
        mShiftedColumn = columnOf(last);
    selectOrExtend(target);
}

void FolderGridController::selectNext() {
    const int count = mModel.itemCount();
    if (count == 0 || current() == count - 1)
        return;
    resetColumnMemory();
    selectOrExtend(qMin(current() + 1, count - 1));
}

void FolderGridController::selectPrevious() {
    if (mModel.itemCount() == 0 || current() == 0)
        return;
    resetColumnMemory();
    selectOrExtend(qMax(current() - 1, 0));
}

void FolderGridController::pageUp() {
    if (mMasonryLayout) {
        moveByRows(-kPageRows);
        return;
    }
    const int last = current();
    if (mModel.itemCount() == 0 || last < 0 || sameRow(0, last))
        return;
    int target = last;
    for (int row = 0; row < kPageRows; ++row)
        target = itemAbove(target);
    if (mShiftedColumn >= 0) {
        target += mShiftedColumn - columnOf(target);
        resetColumnMemory();
    }
    selectOrExtend(target);
}

void FolderGridController::pageDown() {
    if (mMasonryLayout) {
        moveByRows(kPageRows);
        return;
    }
    const int last = current();
    const int count = mModel.itemCount();
    if (count == 0 || last < 0 || sameRow(last, count - 1))
        return;
    resetColumnMemory();
    int target = last;
    for (int row = 0; row < kPageRows; ++row)
        target = itemBelow(target);
    if (columnOf(target) != columnOf(last))
        mShiftedColumn = columnOf(last);
    selectOrExtend(target);
}

void FolderGridController::selectFirst() {
    if (mModel.itemCount() == 0)
        return;
    resetColumnMemory();
    selectOrExtend(0);
}

void FolderGridController::selectLast() {
    if (mModel.itemCount() == 0)
        return;
    resetColumnMemory();
    selectOrExtend(mModel.itemCount() - 1);
}

//------------------------------------------------------------------------------
// Drops and the context menu

bool FolderGridController::acceptsDropAction(int action) {
    const auto dropAction = static_cast<Qt::DropAction>(action);
    return dropAction == Qt::CopyAction || dropAction == Qt::MoveAction;
}

// FolderGridView::dragMoveEvent(): a new target removes the frame of the
// previous one; the presenter frames the new one if it is a directory.
void FolderGridController::dragMoved(int index) {
    const int target = checkRange(index) ? index : -1;
    if (target != mDragTarget)
        mModel.setDragHover(-1);
    mDragTarget = target;
    emit draggedOver(target);
}

void FolderGridController::dragLeft() {
    mDragTarget = -1;
    mModel.setDragHover(-1);
}

void FolderGridController::drop(const QList<QUrl> &urls, QObject *source, int index, int action) {
    dragLeft();
    if (!acceptsDropAction(action)) {
        qWarning() << "FolderGridController ignores a drop with action" << action;
        return;
    }
    emit urlsDropped(urls, source, checkRange(index) ? index : -1,
                     static_cast<Qt::DropAction>(action));
}

void FolderGridController::openSelected() {
    emit openSelectedRequested();
}

void FolderGridController::requestBatchConversion() {
    emit batchConversionRequested();
}

void FolderGridController::requestAddToBatch() {
    emit batchAddRequested();
}
