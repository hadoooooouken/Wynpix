#pragma once

#include <QObject>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

#include "gui/ports/shellport.h"

// Presentation state of the document page navigation HUD (PageNavigationOverlay.qml)
// for multi-page static documents (PDF, multi-page TIFF, DjVu).
//
// Owned by OverlayCoordinator. GUI thread only.
class PageNavigationController final : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by OverlayCoordinator")
    Q_PROPERTY(bool visible READ isVisible NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool shown READ isShown NOTIFY shownChanged FINAL)
    Q_PROPERTY(bool hasPrevious READ hasPrevious NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool hasNext READ hasNext NOTIFY stateChanged FINAL)
    Q_PROPERTY(int pageIndex READ pageIndex NOTIFY stateChanged FINAL)
    Q_PROPERTY(int pageCount READ pageCount NOTIFY stateChanged FINAL)

public:
    static constexpr int kHideTimeoutMs = 2000;

    explicit PageNavigationController(QObject *parent = nullptr);

    [[nodiscard]] bool isVisible() const;
    [[nodiscard]] bool isShown() const;
    [[nodiscard]] bool hasPrevious() const;
    [[nodiscard]] bool hasNext() const;
    [[nodiscard]] int pageIndex() const;
    [[nodiscard]] int pageCount() const;
    [[nodiscard]] bool isScrubbing() const;
    [[nodiscard]] bool isHideTimerActive() const;

    void setFileInfo(const ShellFileInfo &info);
    void setFolderViewActive(bool active);
    void setPanelAtTop(bool atTop);
    void pointerMoved();
    Q_INVOKABLE void setHovered(bool hovered);
    Q_INVOKABLE void firstPage();
    Q_INVOKABLE void prevPage();
    Q_INVOKABLE void nextPage();
    Q_INVOKABLE void lastPage();
    Q_INVOKABLE void seekToPage(int index);
    Q_INVOKABLE void onScrubbingStarted();
    Q_INVOKABLE void onScrubbingFinished();

signals:
    void stateChanged();
    void shownChanged();
    void pageRequested(int pageIndex);

private:
    void setShown(bool shown);
    void show();
    void updateVisibilityState();
    void updateTimer();

    QTimer mHideTimer;
    int mPageIndex = 0;
    int mPageCount = 1;
    bool mFolderViewActive = false;
    bool mPanelAtTop = false;
    bool mShown = false;
    bool mHovered = false;
    bool mScrubbing = false;
};
