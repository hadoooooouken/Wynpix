import QtQuick
import QtQuick.Templates as T
import wynpix.bridges

// Text field (QLineEdit of floating panels and dialog windows): a rounded
// field in the comboField colour with a comboFieldBorder outline that
// highlights in the accent colour while focused.
T.TextField {
    id: control

    implicitWidth: implicitBackgroundWidth + leftInset + rightInset
                   || Math.max(contentWidth, placeholder.implicitWidth) + leftPadding + rightPadding
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding,
                             placeholder.implicitHeight + topPadding + bottomPadding)

    topPadding: StyleConstants.fieldVerticalPadding + StyleConstants.fieldBorderWidth
    bottomPadding: StyleConstants.fieldVerticalPadding + StyleConstants.fieldBorderWidth
    leftPadding: StyleConstants.fieldHorizontalPadding + StyleConstants.fieldBorderWidth
    rightPadding: StyleConstants.fieldHorizontalPadding + StyleConstants.fieldBorderWidth

    color: Theme.colors.textHc
    selectionColor: Theme.colors.accent
    selectedTextColor: Theme.colors.textHc2
    placeholderTextColor: Color.transparent(Theme.colors.text, StyleConstants.secondaryTextOpacity)
    verticalAlignment: TextInput.AlignVCenter
    opacity: enabled ? 1.0 : StyleConstants.disabledOpacity

    Text {
        id: placeholder

        x: control.leftPadding
        y: control.topPadding
        width: control.width - (control.leftPadding + control.rightPadding)
        height: control.height - (control.topPadding + control.bottomPadding)

        text: control.placeholderText
        font: control.font
        color: control.placeholderTextColor
        verticalAlignment: control.verticalAlignment
        visible: !control.length && !control.preeditText
                 && (!control.activeFocus || control.horizontalAlignment !== Qt.AlignHCenter)
        elide: Text.ElideRight
        renderType: control.renderType
    }

    background: Rectangle {
        implicitWidth: Theme.metrics.contextMenuWidth
        radius: StyleConstants.controlRadius
        color: control.enabled && (control.hovered || control.activeFocus)
               ? Theme.colors.comboFieldHover : Theme.colors.comboField
        border.width: StyleConstants.fieldBorderWidth
        border.color: control.enabled && control.activeFocus
                      ? Theme.colors.accent
                      : Color.transparent(Theme.colors.comboFieldBorder,
                                          control.enabled ? 1.0 : StyleConstants.disabledOpacity)
    }
}
