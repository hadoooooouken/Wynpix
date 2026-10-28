pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import wynpix.bridges
import wynpix.style

// Labelled slider rows of an AdjustmentSliderModel (DraggableSliderOverlay's
// form): label, slider and value text. A double click on a slider restores
// its default value.
GridLayout {
    id: grid

    required property AdjustmentSliderModel sliders

    // Optional list of track gradient descriptors: { gradient: Gradient, baseColor: color }
    // or null for default sliders.
    property var trackGradients: null

    readonly property int valueTextMinimumWidth: 45
    readonly property int formRowSpacing: 10
    readonly property int formFieldSpacing: 6

    function adjustSliderByWheel(row, value, delta) {
        if (delta === 0)
            return;
        const steps = Math.max(1, Math.floor(Math.abs(delta) / 120));
        grid.sliders.setValue(row, value + Math.sign(delta) * steps);
    }

    columns: 3
    rowSpacing: formRowSpacing
    columnSpacing: formFieldSpacing

    Repeater {
        model: grid.sliders

        delegate: Label {
            required property int index
            required property string label
            required property int value

            objectName: "adjustmentLabel_" + index
            text: label
            color: Theme.tokens.textPrimary
            Layout.row: index
            Layout.column: 0

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                onDoubleClicked: mouse => {
                    mouse.accepted = true;
                    grid.sliders.resetValue(index);
                }
                onWheel: wheel => {
                    wheel.accepted = true;
                    grid.adjustSliderByWheel(index, value, wheel.angleDelta.y);
                }
            }
        }
    }

    Repeater {
        model: grid.sliders

        delegate: Slider {
            id: slider

            required property int index
            required property var model

            readonly property var trackSpec: grid.trackGradients && slider.index < grid.trackGradients.length
                                            ? grid.trackGradients[slider.index] : null

            from: slider.model.minimum
            to: slider.model.maximum
            value: slider.model.value
            stepSize: 1
            snapMode: Slider.SnapAlways
            focusPolicy: Qt.NoFocus
            trackGradient: slider.trackSpec ? slider.trackSpec.gradient : null
            trackBaseColor: slider.trackSpec && slider.trackSpec.baseColor ? slider.trackSpec.baseColor : "transparent"
            objectName: "adjustmentSlider_" + index
            Layout.row: index
            Layout.column: 1
            Layout.fillWidth: true

            function setValueAt(x) {
                const handleRadius = slider.handle.width / 2;
                const trackLength = slider.availableWidth - slider.handle.width;
                const position = Math.max(0, Math.min(1,
                    (x - handleRadius - slider.leftPadding) / trackLength));
                const logicalPosition = slider.mirrored ? 1 - position : position;
                grid.sliders.setValue(slider.index, Math.round(slider.valueAt(logicalPosition)));
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
                    grid.sliders.resetValue(slider.index);
                }
                onWheel: wheel => {
                    wheel.accepted = true;
                    grid.adjustSliderByWheel(slider.index, slider.model.value,
                                             wheel.angleDelta.y);
                }
            }
        }
    }

    Repeater {
        model: grid.sliders

        delegate: Label {
            required property int index
            required property string valueText
            required property int value

            objectName: "adjustmentValue_" + index
            text: valueText
            color: Theme.tokens.textPrimary
            horizontalAlignment: Text.AlignRight
            Layout.row: index
            Layout.column: 2
            Layout.minimumWidth: grid.valueTextMinimumWidth

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                onDoubleClicked: mouse => {
                    mouse.accepted = true;
                    grid.sliders.resetValue(index);
                }
                onWheel: wheel => {
                    wheel.accepted = true;
                    grid.adjustSliderByWheel(index, value, wheel.angleDelta.y);
                }
            }
        }
    }
}
