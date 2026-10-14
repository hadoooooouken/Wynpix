#include "graphicsdevicelossmonitor.h"

#include <QQuickWindow>
#include <rhi/qrhi.h>

GraphicsDeviceLossMonitor::GraphicsDeviceLossMonitor(QObject *parent) : QObject(parent) {}

void GraphicsDeviceLossMonitor::setWindow(QQuickWindow *window) {
    if (mWindow == window)
        return;
    disconnectWindow();
    mWindow = window;
    if (!window)
        return;
    // Direct connection: the signal is emitted on the render thread while
    // the window still holds the QRhi that was lost; afterwards Qt destroys
    // it. The window outlives its own signal emission.
    mInvalidatedConnection = connect(
        window, &QQuickWindow::sceneGraphInvalidated, this,
        [this, window]() { onSceneGraphInvalidated(window); }, Qt::DirectConnection);
}

QQuickWindow *GraphicsDeviceLossMonitor::window() const {
    return mWindow;
}

void GraphicsDeviceLossMonitor::onSceneGraphInvalidated(const QQuickWindow *window) {
    const QRhi *rhi = window->rhi();
    // No QRhi: the scene graph never got one, so no device was lost.
    if (!rhi || !rhi->isDeviceLost())
        return;
    QMetaObject::invokeMethod(this, [this]() { emit deviceLost(); }, Qt::QueuedConnection);
}

void GraphicsDeviceLossMonitor::disconnectWindow() {
    disconnect(mInvalidatedConnection);
    mWindow = nullptr;
}
