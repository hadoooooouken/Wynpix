import QtQuick
import QtQuick.Effects
import wynpix.bridges

// Frame of popups, menus and tooltips: the widget surface with its border
// and a GPU drop shadow.
Item {
    id: root

    property real radius: StyleConstants.surfaceRadius
    property color color: Theme.tokens.surfacePopup
    property color borderColor: Theme.tokens.strokeDefault
    property int borderWidth: StyleConstants.separatorThickness

    RectangularShadow {
        anchors.fill: parent
        radius: root.radius
        blur: StyleConstants.shadowBlur
        offset.y: StyleConstants.shadowOffsetY
        color: StyleConstants.shadowColor
    }

    Rectangle {
        anchors.fill: parent
        radius: root.radius
        color: root.color
        border.color: root.borderColor
        border.width: root.borderWidth
    }
}
