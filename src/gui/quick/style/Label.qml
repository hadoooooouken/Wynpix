import QtQuick
import QtQuick.Templates as T
import wynpix.bridges

// Text on the application's surfaces (overlays, panels, bars).
T.Label {
    color: Theme.colors.textHc
    // Links keep the text colour (light on dark, dark on light) and are told
    // apart by their underline.
    linkColor: color
}
