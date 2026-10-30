import QtQuick
import QtQuick.Layouts
import wynpix.bridges
import wynpix.style

// Accent colour picker of the settings window, in place of Qt's Quick
// ColorDialog: a saturation / value field for the hue picked on the slider
// below it, the hex code and the picked colour. The colour is kept as
// picked, so OK without a change returns the initial colour unrounded.
DialogWindow {
    id: root

    property color selectedColor
    // Hue, saturation and value of the picked colour, each 0..1. A grey or
    // black colour has no hue (and black no saturation): they keep theirs.
    property real hue: 0
    property real saturation: 0
    property real value: 0

    readonly property int bodyWidth: 288
    readonly property int fieldHeight: 180
    readonly property int markerSize: 12
    readonly property int markerBorderWidth: 2
    readonly property int markerOutlineWidth: 1
    readonly property int swatchWidth: 40
    // Set while the hex field applies its text, which it then keeps.
    property bool applyingHexText: false

    signal colorAccepted(color picked)

    title: qsTranslate("SettingsDialog", "Accent color")
    minimumContentWidth: root.bodyWidth

    onAcceptRequested: root.accept()
    onDismissRequested: root.hide()

    onSelectedColorChanged: {
        if (!root.applyingHexText)
            hexField.text = root.selectedColor.toString();
    }

    function openWith(picked) {
        root.pickColor(picked);
        hexField.text = root.selectedColor.toString();
        root.show();
    }

    function accept() {
        root.hide();
        root.colorAccepted(root.selectedColor);
    }

    function pickColor(picked) {
        if (picked.hsvHue >= 0)
            root.hue = picked.hsvHue;
        if (picked.hsvValue > 0)
            root.saturation = picked.hsvSaturation;
        root.value = picked.hsvValue;
        root.selectedColor = picked;
    }

    function pickHsv(hue, saturation, value) {
        root.hue = hue;
        root.saturation = saturation;
        root.value = value;
        root.selectedColor = Qt.hsva(hue, saturation, value, 1.0);
    }

    // Saturation from left to right, value from top to bottom: the hue
    // under a white layer fading out to the right, under a black layer
    // fading in to the bottom.
    Rectangle {
        id: field

        Layout.preferredWidth: root.bodyWidth
        Layout.preferredHeight: root.fieldHeight
        radius: StyleConstants.controlRadius
        color: Qt.hsva(root.hue, 1.0, 1.0, 1.0)

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "white" }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }
        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            gradient: Gradient {
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 1.0; color: "black" }
            }
        }

        // A white ring with a dark outline, seen on light and dark colours.
        Rectangle {
            x: root.saturation * field.width - width / 2
            y: (1.0 - root.value) * field.height - height / 2
            width: root.markerSize
            height: root.markerSize
            radius: width / 2
            color: "transparent"
            border.width: root.markerBorderWidth
            border.color: "white"

            Rectangle {
                anchors.fill: parent
                anchors.margins: -root.markerOutlineWidth
                radius: width / 2
                color: "transparent"
                border.width: root.markerOutlineWidth
                border.color: StyleConstants.shadowColor
            }
        }

        MouseArea {
            anchors.fill: parent
            preventStealing: true

            function pick(mouse) {
                root.pickHsv(root.hue,
                             Math.min(Math.max(mouse.x / width, 0.0), 1.0),
                             1.0 - Math.min(Math.max(mouse.y / height, 0.0), 1.0));
            }

            onPressed: mouse => pick(mouse)
            onPositionChanged: mouse => pick(mouse)
        }
    }

    Slider {
        Layout.fillWidth: true
        from: 0.0
        to: 1.0
        value: root.hue
        handleRingColor: Theme.tokens.surfaceWindow
        trackGradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: "#ff0000" }
            GradientStop { position: 1 / 6; color: "#ffff00" }
            GradientStop { position: 2 / 6; color: "#00ff00" }
            GradientStop { position: 3 / 6; color: "#00ffff" }
            GradientStop { position: 4 / 6; color: "#0000ff" }
            GradientStop { position: 5 / 6; color: "#ff00ff" }
            GradientStop { position: 1.0; color: "#ff0000" }
        }
        onMoved: root.pickHsv(value, root.saturation, root.value)
    }

    RowLayout {
        spacing: StyleConstants.dialogSpacing

        Label {
            text: qsTr("Hex:")
        }
        // A valid code is applied as typed; leaving the field puts back the
        // code of the picked colour.
        TextField {
            id: hexField

            Layout.fillWidth: true
            validator: RegularExpressionValidator {
                regularExpression: /#?[0-9A-Fa-f]{0,6}/
            }
            onTextEdited: {
                const digits = text.replace("#", "");
                if (digits.length !== 6)
                    return;
                root.applyingHexText = true;
                root.pickColor(Qt.color("#" + digits));
                root.applyingHexText = false;
            }
            onEditingFinished: text = root.selectedColor.toString()
        }
        Rectangle {
            Layout.preferredWidth: root.swatchWidth
            Layout.fillHeight: true
            radius: StyleConstants.controlRadius
            color: root.selectedColor
        }
    }

    DialogButtonBox {
        Layout.fillWidth: true
        Layout.topMargin: StyleConstants.dialogSpacing
        defaultButton: okButton
        onAccepted: root.accept()
        onRejected: root.hide()

        // Own buttons, not standardButtons: no Qt base translation is loaded.
        Button {
            id: okButton

            text: qsTr("OK")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        Button {
            text: qsTr("Cancel")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
    }
}
