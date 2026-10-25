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

public:
    static constexpr int kHideTimeoutMs = 2000;

    explicit PageNavigationController(QObject *parent = nullptr);

    [[nodiscard]] bool isVisible() const;
    [[nodiscard]] bool isShown() const;
    [[nodiscard]] bool hasPrevious() const;
    [[nodiscard]] bool hasNext() const;

    void setFileInfo(const ShellFileInfo &info);
    void setFolderViewActive(bool active);
    void setPanelAtTop(bool atTop);
    void pointerMoved();
    Q_INVOKABLE void setHovered(bool hovered);

signals:
    void stateChanged();
    void shownChanged();

private:
    void setShown(bool shown);
    void show();
    void updateVisibilityState();

    QTimer mHideTimer;
    int mPageIndex = 0;
    int mPageCount = 1;
    bool mFolderViewActive = false;
    bool mPanelAtTop = false;
    bool mShown = false;
    bool mHovered = false;
};
