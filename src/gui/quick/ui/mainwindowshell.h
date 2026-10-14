#pragma once

#include <QList>
#include <QObject>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

// State and intents of the Qt Quick main window (Main.qml) that do not belong
// to the image viewport: which page is shown, whether the window is in
// fullscreen, how opaque the window background is, and files dropped onto
// the window. Main.qml binds the properties and forwards drops; the Quick UI
// host decides what they mean.
//
// Owned by the Quick UI host and handed to QML as a required property.
// GUI thread only.
class MainWindowShell final : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by the Quick UI host")
    Q_PROPERTY(bool folderViewActive READ folderViewActive NOTIFY folderViewActiveChanged FINAL)
    Q_PROPERTY(bool fullscreen READ fullscreen NOTIFY fullscreenChanged FINAL)
    Q_PROPERTY(qreal backgroundOpacity READ backgroundOpacity NOTIFY backgroundOpacityChanged FINAL)
    Q_PROPERTY(bool translucentWindow READ translucentWindow NOTIFY translucentWindowChanged FINAL)

public:
    using QObject::QObject;

    [[nodiscard]] bool folderViewActive() const;
    void setFolderViewActive(bool active);
    [[nodiscard]] bool fullscreen() const;
    void setFullscreen(bool fullscreen);

    // Opacity of the window background behind the image viewer, in [0, 1]:
    // the windowed opacity in windowed document mode, opaque in fullscreen
    // and in folder view (like the widget UI).
    [[nodiscard]] qreal backgroundOpacity() const;
    // The opacity setting for windowed mode; values outside [0, 1] are
    // clamped, a value that is not finite is rejected.
    void setWindowedBackgroundOpacity(qreal opacity);
    // Whether the window is presented with an alpha channel. Set by the
    // Quick UI host before the window is created; Main.qml then keeps the
    // window colour transparent and paints the background with an item.
    [[nodiscard]] bool translucentWindow() const;
    void setTranslucentWindow(bool translucent);

    // Files dropped onto the window. source is the object that started the
    // drag (nullptr for drags from other applications).
    Q_INVOKABLE void dropUrls(const QList<QUrl> &urls, QObject *source);

signals:
    void folderViewActiveChanged();
    void fullscreenChanged();
    void backgroundOpacityChanged();
    void translucentWindowChanged();
    // Never emitted with an empty list.
    void urlsDropped(const QList<QUrl> &urls, QObject *source);

private:
    static constexpr qreal opaque = 1.0;
    static constexpr qreal transparent = 0.0;

    // Notifies backgroundOpacityChanged when a state change moved the
    // effective opacity away from previous.
    void notifyBackgroundOpacity(qreal previous);

    bool mFolderViewActive = false;
    bool mFullscreen = false;
    qreal mWindowedBackgroundOpacity = opaque;
    bool mTranslucentWindow = false;
};
