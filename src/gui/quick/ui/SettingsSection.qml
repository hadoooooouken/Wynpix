import QtQuick
import QtQuick.Layouts
import wynpix.bridges
import wynpix.style

// A group of options on a settings page under its title; no title for an
// untitled group. Drawn as a bordered, tinted card on the window (the SGroup
// frames of the widget dialog).
Rectangle {
    id: root

    default property alias content: body.data
    property string title

    readonly property int horizontalPadding: 13
    readonly property int verticalPadding: 10
    readonly property int cornerRadius: 6

    Layout.fillWidth: true
    implicitWidth: column.implicitWidth + 2 * root.horizontalPadding
    implicitHeight: column.implicitHeight + 2 * root.verticalPadding
    radius: root.cornerRadius
    color: Theme.surfaces.tintedLc2
    border.width: StyleConstants.separatorThickness
    border.color: Theme.surfaces.tintedLc

    ColumnLayout {
        id: column

        anchors.fill: parent
        anchors.leftMargin: root.horizontalPadding
        anchors.rightMargin: root.horizontalPadding
        anchors.topMargin: root.verticalPadding
        anchors.bottomMargin: root.verticalPadding
        spacing: StyleConstants.dialogSpacing

        Label {
            Layout.fillWidth: true
            visible: root.title.length > 0
            text: root.title
            font: Theme.fonts.bodyStrong
        }

        ColumnLayout {
            id: body

            Layout.fillWidth: true
            spacing: StyleConstants.dialogSpacing
        }
    }
}
