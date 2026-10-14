#pragma once

#include <QMetaObject>
#include <QObject>
#include <QPointer>

class QQuickWindow;

// Reports the loss of a window's graphics device (a driver reset, a removed
// or updated GPU): deviceLost() is emitted on the GUI thread after the scene
// graph of the window was dropped because its QRhi reported the device lost.
//
// Qt recovers on its own: the render loop creates a new QRhi and schedules a
// full update, so every item renders again from its GUI-thread state. The
// monitor only makes the event visible. Other invalidations of the scene
// graph (window hidden, application exit) are not reported.
//
// The check runs on the render thread in the window's sceneGraphInvalidated
// signal, the last point at which the lost QRhi still exists.
//
// GUI thread only, except onSceneGraphInvalidated(), which the window's
// render thread calls through the connection made by setWindow().
class GraphicsDeviceLossMonitor final : public QObject {
    Q_OBJECT
public:
    explicit GraphicsDeviceLossMonitor(QObject *parent = nullptr);

    // Follows the scene graph of window (nullptr detaches).
    void setWindow(QQuickWindow *window);
    [[nodiscard]] QQuickWindow *window() const;

signals:
    void deviceLost();

private:
    // Render thread (or the GUI thread with the non-threaded render loop).
    void onSceneGraphInvalidated(const QQuickWindow *window);
    void disconnectWindow();

    QPointer<QQuickWindow> mWindow;
    QMetaObject::Connection mInvalidatedConnection;
};
