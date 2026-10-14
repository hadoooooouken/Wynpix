import QtQuick
import QtQuick.Layouts
import wynpix.bridges
import wynpix.style

// Yes / No question with a warning icon (the QMessageBox of the widget UI).
// Yes is the default button; Escape and the close button answer No.
DialogWindow {
    id: root

    required property ConfirmationDialogModel dialog

    readonly property int iconSize: 32
    // Longer messages wrap, as in a QMessageBox.
    readonly property int maximumTextWidth: 480

    title: root.dialog.title
    visible: root.dialog.open

    onAcceptRequested: root.dialog.accept()
    onDismissRequested: root.dialog.reject()

    RowLayout {
        Layout.fillWidth: true
        spacing: StyleConstants.popupPadding

        IconGlyph {
            Layout.alignment: Qt.AlignTop
            icon: FluentIcons.Warning20
            size: root.iconSize
        }

        Label {
            Layout.fillWidth: true
            Layout.maximumWidth: root.maximumTextWidth
            text: root.dialog.message
            wrapMode: Text.Wrap
            textFormat: Text.PlainText
        }
    }

    // Own buttons, not standardButtons: no Qt base translation is loaded.
    DialogButtonBox {
        Layout.fillWidth: true
        defaultButton: yesButton
        onAccepted: root.dialog.accept()
        onRejected: root.dialog.reject()

        Button {
            id: yesButton

            text: qsTr("Yes")
            DialogButtonBox.buttonRole: DialogButtonBox.YesRole
        }
        Button {
            text: qsTr("No")
            DialogButtonBox.buttonRole: DialogButtonBox.NoRole
        }
    }
}
