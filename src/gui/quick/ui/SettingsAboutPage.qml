import QtQuick
import QtQuick.Layouts
import wynpix.style

// Settings page "About": the application description with its links.
SettingsPage {
    id: root

    required property SettingsEditorModel editor

    title: qsTranslate("SettingsDialog", "About Wynpix")

    Label {
        Layout.fillWidth: true
        text: root.editor.aboutText
        textFormat: Text.StyledText
        wrapMode: Text.WordWrap
        onLinkActivated: link => Qt.openUrlExternally(link)

        HoverHandler {
            cursorShape: parent.hoveredLink.length > 0 ? Qt.PointingHandCursor : Qt.ArrowCursor
        }
    }
}
