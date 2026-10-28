import QtQuick
import wynpix.bridges

// One glyph of the application icon font (FluentIcons), centred in a square
// of size pixels: IconGlyph { icon: FluentIcons.Settings20 }.
Item {
    id: root

    // Glyph to draw (a FluentIcons value).
    required property int icon
    // Edge of the glyph square, in pixels.
    property int size: Theme.standardIconSize
    property alias color: glyph.color
    readonly property alias text: glyph.text
    readonly property alias font: glyph.font

    implicitWidth: size
    implicitHeight: size

    Text {
        id: glyph

        anchors.fill: parent
        text: Theme.glyph(root.icon)
        color: Theme.tokens.iconDefault
        font.family: Theme.iconFontFamily
        font.pixelSize: root.size
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        renderType: Text.QtRendering
    }
}
