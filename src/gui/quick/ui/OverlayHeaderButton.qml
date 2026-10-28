import QtQuick
import QtQuick.Templates as T
import wynpix.bridges
import wynpix.style

// Icon button of overlay headers and of the fullscreen controls
// (OverlayHeaderButton / ActionButton in the widget UI): a glyph that is
// tinted while hovered or pressed. It never takes the keyboard focus.
T.AbstractButton {
    id: control

    // A FluentIcons value.
    required property int glyph
    property int glyphSize: Theme.compactIconSize
    property color glyphColor: Theme.colors.icons
    // Edge of the square button.
    property int size: Theme.metrics.overlayHeaderSize
    // Inset of the tinted surface from the button edge.
    property int surfaceInset: 3
    property real surfaceRadius: StyleConstants.controlRadius
    // Fills of the hovered and the pressed button; HUD panels override them.
    property color hoverColor: Theme.colors.fillHover
    property color pressedColor: Theme.colors.fillPressed

    implicitWidth: size
    implicitHeight: size
    focusPolicy: Qt.NoFocus
    hoverEnabled: true

    Accessible.role: Accessible.Button

    background: Rectangle {
        anchors.fill: parent
        anchors.margins: control.surfaceInset
        radius: control.surfaceRadius
        color: control.down ? control.pressedColor
             : control.hovered ? control.hoverColor
             : "transparent"
    }

    contentItem: IconGlyph {
        icon: control.glyph
        size: control.glyphSize
        color: control.glyphColor
    }
}
