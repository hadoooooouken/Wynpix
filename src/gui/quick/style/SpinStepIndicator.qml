import QtQuick
import wynpix.bridges

// Up or down step button of SpinBox and DoubleSpinBox: a chevron glyph on a
// translucent fill while hovered or pressed, which stays visible on the
// field's own hover colour.
Rectangle {
    id: root

    // Chevron glyph (a FluentIcons value).
    required property int icon
    property bool pressed: false
    property bool hovered: false

    implicitWidth: StyleConstants.spinStepIndicatorWidth
    implicitHeight: StyleConstants.spinBoxMinimumContentHeight
    radius: StyleConstants.controlRadius
    color: pressed ? Theme.colors.fillPressed
         : hovered ? Theme.colors.fillHover
         : "transparent"
    opacity: enabled ? 1.0 : StyleConstants.disabledOpacity

    IconGlyph {
        anchors.centerIn: parent
        icon: root.icon
        size: StyleConstants.chevronSize
    }
}
