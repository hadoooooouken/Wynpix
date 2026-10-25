#include "pagenavigationcontroller.h"

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
    if (!isVisible())
        return;
    if (hovered) {
        mHideTimer.stop();
        setShown(true);
    } else if (mShown) {
        mHideTimer.start();
    }
}

void PageNavigationController::setShown(bool shown) {
    if (mShown == shown)
        return;
    mShown = shown;
    emit shownChanged();
}

void PageNavigationController::show() {
    setShown(true);
    if (mHovered)
        mHideTimer.stop();
    else
        mHideTimer.start();
}

void PageNavigationController::updateVisibilityState() {
    if (isVisible()) {
        show();
    } else {
        mHideTimer.stop();
        setShown(false);
    }
}
