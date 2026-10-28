import QtQuick
import QtQuick.Layouts
import wynpix.bridges
import wynpix.style

// One page of the settings window: its title above a scrolling column of
// sections, as the pages of the widget dialog.
ScrollView {
    id: root

    default property alias content: column.data
    required property string title

    readonly property int titleSpacing: 12
    readonly property int sectionSpacing: 18
    // Keeps the sections off the scroll bar while it is shown.
    readonly property int scrollBarGap: verticalBarShown ? StyleConstants.scrollBarContentSpacing : 0

    contentWidth: availableWidth
    clip: true

    ColumnLayout {
        id: column

        x: root.mirrored ? root.scrollBarGap : 0
        width: root.availableWidth - root.scrollBarGap
        spacing: root.sectionSpacing

        Label {
            Layout.fillWidth: true
            Layout.bottomMargin: root.titleSpacing - root.sectionSpacing
            text: root.title
            font: Theme.fonts.title
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: StyleConstants.separatorThickness
            color: Theme.tokens.strokeDefault
        }
    }
}
