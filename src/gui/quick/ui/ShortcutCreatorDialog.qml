import QtQuick
import QtQuick.Layouts
import wynpix.bridges
import wynpix.style

// Shortcut creator of the settings window (ShortcutCreatorDialog in the
// widget UI): the action it was opened for, with its category, and the
// shortcut captured below. A shortcut the application already binds shows a
// warning; OK replaces it.
DialogWindow {
    id: root

    required property ShortcutEditorModel dialog

    readonly property int captureSpacing: 10
    readonly property int warningMinimumHeight: 36
    readonly property int bodyWidth: 316

    title: root.dialog.title
    minimumContentWidth: root.bodyWidth
    initialFocusItem: captureField
    visible: root.dialog.open

    onAcceptRequested: root.dialog.accept()
    onDismissRequested: root.dialog.reject()

    Label {
        Layout.fillWidth: true
        Layout.maximumWidth: root.bodyWidth
        text: root.dialog.actionTitle
        textFormat: Text.PlainText
        font: Theme.fonts.bodyStrong
        elide: Text.ElideRight
    }

    Label {
        Layout.fillWidth: true
        Layout.maximumWidth: root.bodyWidth
        text: root.dialog.categoryTitle
        textFormat: Text.PlainText
        font: Theme.fonts.caption
        color: Theme.tokens.textSecondary
        elide: Text.ElideRight
    }

    ShortcutCaptureField {
        id: captureField

        Layout.topMargin: root.captureSpacing
        Layout.fillWidth: true
        text: root.dialog.shortcutText
        onCaptured: shortcut => root.dialog.setShortcut(shortcut)
    }

    Label {
        Layout.fillWidth: true
        Layout.maximumWidth: root.bodyWidth
        text: qsTranslate("ShortcutCreatorDialog", "Press a key combination, click a mouse button or turn the wheel over the field.")
        textFormat: Text.PlainText
        font: Theme.fonts.caption
        color: Theme.tokens.textSecondary
        wrapMode: Text.WordWrap
    }

    // Keeps its height without a warning, so the dialog does not resize.
    RowLayout {
        Layout.fillWidth: true
        Layout.maximumWidth: root.bodyWidth
        Layout.minimumHeight: root.warningMinimumHeight
        spacing: StyleConstants.dialogSpacing

        IconGlyph {
            Layout.alignment: Qt.AlignTop
            visible: root.dialog.warning.length > 0
            icon: FluentIcons.Warning20
        }

        Label {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            text: root.dialog.warning
            textFormat: Text.PlainText
            wrapMode: Text.WordWrap
        }
    }

    DialogButtonBox {
        Layout.fillWidth: true
        defaultButton: okButton
        onAccepted: root.dialog.accept()
        onRejected: root.dialog.reject()

        // Own buttons, not standardButtons: no Qt base translation is loaded.
        Button {
            id: okButton

            text: qsTr("OK")
            enabled: root.dialog.canAccept
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        Button {
            text: qsTr("Cancel")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
    }
}
