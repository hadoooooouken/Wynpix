import QtQuick
import QtQuick.Layouts
import wynpix.style

// One colour adjustment of the batch converter: label, slider and aligned
// value text. Double-clicking any part of the row restores its default.
RowLayout {
    id: root

    required property int rowIndex
    required property var spec
    required property real value

    readonly property int labelWidth: 80
    readonly property int valueTextWidth: 45
    readonly property int formFieldSpacing: 6

    readonly property string valueText: {
        const number = Number(root.value).toFixed(root.spec.decimals);
        const signedNumber = root.rowIndex === 0 && root.value >= 0 ? "+" + number : number;
        return signedNumber + root.spec.suffix;
    }

    signal edited(real value)

    spacing: root.formFieldSpacing

    Label {
        objectName: "batchColorLabel_" + root.rowIndex
        Layout.preferredWidth: root.labelWidth
        text: root.spec.label

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            onDoubleClicked: mouse => {
                mouse.accepted = true;
                root.edited(root.spec.defaultValue);
            }
        }
    }

    Slider {
        id: slider

        objectName: "batchColorSlider_" + root.rowIndex
        Layout.fillWidth: true
        from: root.spec.minimum
        to: root.spec.maximum
        stepSize: root.spec.step
        snapMode: Slider.SnapAlways
        value: root.value

        function setValueAt(x) {
            const handleRadius = slider.handle.width / 2;
            const trackLength = slider.availableWidth - slider.handle.width;
            if (trackLength <= 0)
                return;
            const position = Math.max(0, Math.min(1,
                (x - handleRadius - slider.leftPadding) / trackLength));
            const logicalPosition = slider.mirrored ? 1 - position : position;
            const rawValue = slider.valueAt(logicalPosition);
            const steppedValue = slider.from
                                + Math.round((rawValue - slider.from) / slider.stepSize)
                                  * slider.stepSize;
            root.edited(Number(steppedValue.toFixed(root.spec.decimals)));
        }

        MouseArea {
            anchors.fill: parent
            z: 1
            acceptedButtons: Qt.LeftButton
            onPressed: mouse => slider.setValueAt(mouse.x)
            onPositionChanged: mouse => {
                if (pressed)
                    slider.setValueAt(mouse.x);
            }
            onDoubleClicked: mouse => {
                mouse.accepted = true;
                root.edited(root.spec.defaultValue);
            }
        }
    }

    Label {
        objectName: "batchColorValue_" + root.rowIndex
        Layout.preferredWidth: root.valueTextWidth
        horizontalAlignment: Text.AlignRight
        text: root.valueText

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            onDoubleClicked: mouse => {
                mouse.accepted = true;
                root.edited(root.spec.defaultValue);
            }
        }
    }
}
