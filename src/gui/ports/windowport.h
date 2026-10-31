#pragma once

#include <qwindowdefs.h>

// Outbound UI port for the top-level application window. Must only be
// called on the GUI thread.
class IWindowPort {
public:
    virtual ~IWindowPort() = default;

    // Shows the window in its configured state (fullscreen or windowed);
    // does nothing when it is already visible.
    virtual void showWindow() = 0;
    virtual void hideWindow() = 0;
    [[nodiscard]] virtual bool isWindowVisible() const = 0;
    // A concealed window stays mapped but is not shown on screen, so it can
    // be laid out and painted before it is revealed.
    virtual void setWindowConcealed(bool concealed) = 0;
    // Reveals a concealed window once the frame it has presented is on
    // screen, so the window never shows its previous frame.
    virtual void revealWindowAfterComposition() = 0;
    [[nodiscard]] virtual bool isWindowMinimized() const = 0;
    // Restores a minimized window and brings the window to the foreground
    // with keyboard focus.
    virtual void raiseAndActivateWindow() = 0;
    virtual void saveWindowGeometry() = 0;

protected:
    IWindowPort() = default;
    IWindowPort(const IWindowPort &) = default;
    IWindowPort &operator=(const IWindowPort &) = default;
};
