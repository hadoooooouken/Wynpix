import QtQuick
import QtQuick.Templates as T
import wynpix.bridges

// Progress bar (the batch converter's QProgressBar rules): a bordered bar in
// the folder view colour, filled with the accent colour, with the percentage
// centred on it: in the folder view text colour over the empty part and in
// the accent text colour over the filled part.
T.ProgressBar {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    readonly property real percentPerUnit: 100

    padding: StyleConstants.progressBarBorderWidth

    contentItem: Item {
        implicitWidth: StyleConstants.sliderLength
        implicitHeight: percentText.implicitHeight + 2 * StyleConstants.progressBarTextPadding

        Rectangle {
            id: fill

            width: control.indeterminate ? parent.width : control.visualPosition * parent.width
            height: parent.height
            radius: StyleConstants.progressBarRadius
            color: Theme.colors.accent
            visible: control.indeterminate || control.position > 0
        }

        Text {
            id: percentText

            anchors.centerIn: parent
            text: Math.round(control.position * control.percentPerUnit) + "%"
            visible: !control.indeterminate
            color: Theme.colors.textHc2
            font: Theme.fonts.body
        }

        // The same text again, cut to the filled part.
        Item {
            objectName: "filledPercentText"

            width: fill.width
            height: parent.height
            clip: true
            visible: percentText.visible && fill.visible

            Text {
                x: percentText.x
                y: percentText.y
                text: percentText.text
                color: Theme.colors.accentText
                font: percentText.font
            }
        }
    }

    background: Rectangle {
        implicitWidth: StyleConstants.sliderLength
        radius: StyleConstants.progressBarRadius
        color: Theme.colors.folderView
        border.width: StyleConstants.progressBarBorderWidth
        border.color: Theme.colors.strokeDefault
    }
}
