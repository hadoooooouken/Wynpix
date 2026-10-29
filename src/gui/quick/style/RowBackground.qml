import QtQuick
import wynpix.bridges

// Background of a selectable row of a list, tree or navigation sidebar, as
// in Windows 11: a neutral fill while hovered or selected and an accent pill
// on the leading edge of the selected row. The pill, not the fill, tells the
// selected row from the hovered one; a selected row under the pointer turns
// lighter. The neutral fills are ink of the scheme, so the rows read the
// same on the light and the dark scheme and on any accent.
Rectangle {
    id: row

    property bool selected: false
    property bool hovered: false
    property bool pressed: false
    // Fill of a row that is neither selected nor hovered (alternate rows).
    property color restColor: "transparent"

    radius: StyleConstants.rowRadius
    color: pressed ? Theme.tokens.rowPressed
         : selected ? (hovered ? Theme.tokens.rowSelectedHover : Theme.tokens.rowSelected)
         : hovered ? Theme.tokens.rowHover
         : restColor

    Rectangle {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        width: StyleConstants.selectionIndicatorWidth
        height: StyleConstants.selectionIndicatorLength
        radius: width / 2
        color: Theme.tokens.accent
        visible: row.selected
    }
}
