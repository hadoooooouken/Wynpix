#include "mainwindowshell.h"

#include <QDebug>

#include <algorithm>
#include <cmath>

bool MainWindowShell::folderViewActive() const {
    return mFolderViewActive;
}

void MainWindowShell::setFolderViewActive(bool active) {
    if (mFolderViewActive == active)
        return;
    const qreal previousOpacity = backgroundOpacity();
    mFolderViewActive = active;
    emit folderViewActiveChanged();
    notifyBackgroundOpacity(previousOpacity);
}

bool MainWindowShell::fullscreen() const {
    return mFullscreen;
}

void MainWindowShell::setFullscreen(bool fullscreen) {
    if (mFullscreen == fullscreen)
        return;
    const qreal previousOpacity = backgroundOpacity();
    mFullscreen = fullscreen;
    emit fullscreenChanged();
    notifyBackgroundOpacity(previousOpacity);
}

qreal MainWindowShell::backgroundOpacity() const {
    return mFullscreen || mFolderViewActive ? opaque : mWindowedBackgroundOpacity;
}

void MainWindowShell::setWindowedBackgroundOpacity(qreal opacity) {
    if (!std::isfinite(opacity)) {
        qWarning() << "MainWindowShell: the window opacity" << opacity
                   << "is not finite; the previous value is kept";
        return;
    }
    const qreal bounded = std::clamp(opacity, transparent, opaque);
    if (mWindowedBackgroundOpacity == bounded)
        return;
    const qreal previousOpacity = backgroundOpacity();
    mWindowedBackgroundOpacity = bounded;
    notifyBackgroundOpacity(previousOpacity);
}

bool MainWindowShell::translucentWindow() const {
    return mTranslucentWindow;
}

void MainWindowShell::setTranslucentWindow(bool translucent) {
    if (mTranslucentWindow == translucent)
        return;
    mTranslucentWindow = translucent;
    emit translucentWindowChanged();
}

void MainWindowShell::notifyBackgroundOpacity(qreal previous) {
    if (backgroundOpacity() != previous)
        emit backgroundOpacityChanged();
}

void MainWindowShell::dropUrls(const QList<QUrl> &urls, QObject *source) {
    if (urls.isEmpty()) {
        qWarning() << "MainWindowShell: a drop without URLs was ignored";
        return;
    }
    emit urlsDropped(urls, source);
}
