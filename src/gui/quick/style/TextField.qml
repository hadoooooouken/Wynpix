import QtQuick
import QtQuick.Templates as T
import wynpix.bridges

// Text field (QLineEdit of floating panels and dialog windows): a field
// background with the shared focus ring (FieldBackground).
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

    color: Theme.tokens.textPrimary
    selectionColor: Theme.tokens.accent
    selectedTextColor: Theme.tokens.accentText
    placeholderTextColor: Color.transparent(Theme.tokens.textSecondary, StyleConstants.secondaryTextOpacity)
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

    background: FieldBackground {
        implicitWidth: Theme.metrics.contextMenuWidth
        fieldEnabled: control.enabled
        hovered: control.hovered
        focused: control.activeFocus
    }
}
