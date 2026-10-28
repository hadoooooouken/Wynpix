import QtQuick
import wynpix.bridges

// Background of the entry controls (TextField, SearchField, SpinBox,
// DoubleSpinBox, ComboBox): a rounded field in the fieldRest colour with a
// strokeField outline. One focus ring serves all of them: while the
// control is focused the outline becomes an accent ring of fieldBorderWidth,
// and the fill takes the hover colour.
Rectangle {
    id: root

    property bool fieldEnabled: true
    property bool hovered: false
    property bool pressed: false
    property bool focused: false
    // Width of the outline while the control is not focused.
    property int restBorderWidth: StyleConstants.fieldBorderWidth

    readonly property bool ringVisible: root.fieldEnabled && root.focused

    radius: StyleConstants.controlRadius
    color: root.fieldEnabled && root.pressed ? Theme.tokens.fieldPressed
         : root.fieldEnabled && (root.hovered || root.focused) ? Theme.tokens.fieldHover
         : Theme.tokens.fieldRest
    border.width: root.ringVisible ? StyleConstants.fieldBorderWidth : root.restBorderWidth
    border.color: root.ringVisible
                  ? Theme.tokens.accent
                  : Color.transparent(Theme.tokens.strokeField,
                                      root.fieldEnabled ? 1.0 : StyleConstants.disabledOpacity)
}
