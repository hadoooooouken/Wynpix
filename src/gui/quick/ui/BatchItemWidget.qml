import QtQuick
import QtQuick.Layouts
import wynpix.bridges
import wynpix.render
import wynpix.style

// One file of the batch converter's queue (BatchItemWidget in the widget
// dialog): the check box selecting it for the batch, its thumbnail, name and
// source info on the left, its state and what happened to it on the right.
Item {
    id: root

    required property int index
    required property string name
    required property string sourceInfo
    required property bool checked
    required property int itemState
    required property string stateText
    required property string details
    required property thumbnailHandle thumbnail
    required property bool animated

    required property BatchQueueModel queue
    required property int thumbnailSize

    readonly property int margin: 6
    readonly property int spacing: 10
    readonly property int textSpacing: 2
    readonly property int stateMinimumWidth: 80

    readonly property color stateColor: {
        switch (root.itemState) {
        case BatchQueueModel.Failed:
        case BatchQueueModel.Stopped:
            return Theme.tokens.statusError;
        case BatchQueueModel.Processing:
            return Theme.tokens.statusProcessing;
        case BatchQueueModel.Done:
            return Theme.tokens.statusSuccess;
        default:
            return Theme.tokens.statusPending;
        }
    }

    implicitHeight: row.implicitHeight + 2 * root.margin

    RowLayout {
        id: row

        anchors.fill: parent
        anchors.margins: root.margin
        spacing: root.spacing

        CheckBox {
            checked: root.checked
            onToggled: root.queue.setChecked(root.index, checked)
        }

        Rectangle {
            Layout.preferredWidth: root.thumbnailSize
            Layout.preferredHeight: root.thumbnailSize
            radius: StyleConstants.itemRadius
            color: Theme.tokens.surfacePopup
            border.color: Theme.tokens.strokeDefault

            ThumbnailItem {
                anchors.fill: parent
                thumbnail: root.thumbnail
                maximumSize: Qt.size(root.thumbnailSize, root.thumbnailSize)
                cornerRadius: StyleConstants.itemRadius
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: root.textSpacing

            Text {
                Layout.fillWidth: true
                text: root.name
                elide: Text.ElideRight
                color: Theme.tokens.textPrimary
                font: Theme.fonts.bodyStrong
            }
            Text {
                Layout.fillWidth: true
                text: root.sourceInfo
                elide: Text.ElideRight
                color: Theme.tokens.textTertiary
                font: Theme.fonts.caption
            }
            Text {
                Layout.fillWidth: true
                visible: root.animated
                text: qsTr("Animated image: only the first frame is converted")
                elide: Text.ElideRight
                color: Theme.tokens.statusPending
                font: Theme.fonts.caption
            }
        }

        ColumnLayout {
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            Layout.maximumWidth: row.width / 2
            spacing: root.textSpacing

            Text {
                Layout.alignment: Qt.AlignRight
                Layout.minimumWidth: root.stateMinimumWidth
                horizontalAlignment: Text.AlignRight
                text: root.stateText
                color: root.stateColor
                font: Theme.fonts.bodyStrong
            }
            TextEdit {
                Layout.alignment: Qt.AlignRight
                Layout.fillWidth: true
                visible: root.details.length > 0
                horizontalAlignment: Text.AlignRight
                text: root.details
                readOnly: true
                selectByMouse: true
                wrapMode: Text.Wrap
                color: Theme.tokens.textTertiary
                font: Theme.fonts.caption
            }
        }
    }
}
