#include "pagenavigationcontroller.h"

#include <algorithm>

PageNavigationController::PageNavigationController(QObject *parent)
    : QObject(parent) {
    mHideTimer.setSingleShot(true);
    mHideTimer.setInterval(kHideTimeoutMs);
    connect(&mHideTimer, &QTimer::timeout, this, [this]() { setShown(false); });
}

bool PageNavigationController::isVisible() const {
    return !mFolderViewActive && !mPanelAtTop && mPageCount > 1;
}

bool PageNavigationController::isShown() const {
    return mShown;
}

bool PageNavigationController::hasPrevious() const {
    return mPageCount > 1 && mPageIndex > 0;
}

bool PageNavigationController::hasNext() const {
    return mPageCount > 1 && mPageIndex < mPageCount - 1;
}

int PageNavigationController::pageIndex() const {
    return mPageIndex;
}

int PageNavigationController::pageCount() const {
    return mPageCount;
}

bool PageNavigationController::isScrubbing() const {
    return mScrubbing;
}

bool PageNavigationController::isHideTimerActive() const {
    return mHideTimer.isActive();
}

void PageNavigationController::firstPage() {
    if (mPageIndex > 0)
        emit pageRequested(0);
}

void PageNavigationController::prevPage() {
    if (hasPrevious())
        emit pageRequested(mPageIndex - 1);
}

void PageNavigationController::nextPage() {
    if (hasNext())
        emit pageRequested(mPageIndex + 1);
}

void PageNavigationController::lastPage() {
    if (mPageIndex < mPageCount - 1)
        emit pageRequested(mPageCount - 1);
}

void PageNavigationController::seekToPage(int index) {
    if (mPageCount <= 1)
        return;
    const int target = std::clamp(index, 0, mPageCount - 1);
    if (target != mPageIndex)
        emit pageRequested(target);
}

void PageNavigationController::onScrubbingStarted() {
    mScrubbing = true;
    updateTimer();
}

void PageNavigationController::onScrubbingFinished() {
    mScrubbing = false;
    updateTimer();
}

void PageNavigationController::setFileInfo(const ShellFileInfo &info) {
    if (mPageIndex == info.pageIndex && mPageCount == info.pageCount)
        return;
    mPageIndex = info.pageIndex;
    mPageCount = info.pageCount;
    emit stateChanged();
    updateVisibilityState();
}

void PageNavigationController::setFolderViewActive(bool active) {
    if (mFolderViewActive == active)
        return;
    mFolderViewActive = active;
    emit stateChanged();
    updateVisibilityState();
}

void PageNavigationController::setPanelAtTop(bool atTop) {
    if (mPanelAtTop == atTop)
        return;
    mPanelAtTop = atTop;
    emit stateChanged();
    updateVisibilityState();
}

void PageNavigationController::pointerMoved() {
    if (!isVisible())
        return;
    show();
}

void PageNavigationController::setHovered(bool hovered) {
    if (mHovered == hovered)
        return;
    mHovered = hovered;
    updateTimer();
}

void PageNavigationController::setShown(bool shown) {
    if (mShown == shown)
        return;
    mShown = shown;
    emit shownChanged();
}

void PageNavigationController::show() {
    setShown(true);
    updateTimer();
}

void PageNavigationController::updateTimer() {
    if (!isVisible())
        return;
    if (mHovered || mScrubbing) {
        mHideTimer.stop();
        setShown(true);
    } else if (mShown) {
        mHideTimer.start();
    }
}

void PageNavigationController::updateVisibilityState() {
    if (isVisible()) {
        show();
    } else {
        mHovered = false;
        mScrubbing = false;
        mHideTimer.stop();
        setShown(false);
    }
}
