#include "pagenavigationcontroller.h"

PageNavigationController::PageNavigationController(QObject *parent)
    : QObject(parent) {}

bool PageNavigationController::isVisible() const {
    return !mFolderViewActive && !mPanelAtTop && mPageCount > 1;
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
}

void PageNavigationController::setFolderViewActive(bool active) {
    if (mFolderViewActive == active)
        return;
    mFolderViewActive = active;
    emit stateChanged();
}

void PageNavigationController::setPanelAtTop(bool atTop) {
    if (mPanelAtTop == atTop)
        return;
    mPanelAtTop = atTop;
    emit stateChanged();
}
