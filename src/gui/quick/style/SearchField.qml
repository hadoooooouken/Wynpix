pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Templates as T
import wynpix.bridges

// Search field (the folder view name filter): the TextField look with a
// clear button, and a suggestion popup like the ComboBox list. The widget
// filter has no search icon, so there is no search indicator.
T.SearchField {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             clearIndicator.implicitIndicatorHeight + topPadding + bottomPadding)

    readonly property bool __clearIndicatorVisible: control.clearIndicator.indicator
                                                    && control.clearIndicator.indicator.visible

    topPadding: StyleConstants.fieldVerticalPadding + StyleConstants.fieldBorderWidth
    bottomPadding: StyleConstants.fieldVerticalPadding + StyleConstants.fieldBorderWidth
    leftPadding: StyleConstants.fieldHorizontalPadding + StyleConstants.fieldBorderWidth
                 + (control.mirrored && __clearIndicatorVisible
                    ? control.clearIndicator.indicator.width + spacing : 0)
    rightPadding: StyleConstants.fieldHorizontalPadding + StyleConstants.fieldBorderWidth
                  + (!control.mirrored && __clearIndicatorVisible
                     ? control.clearIndicator.indicator.width + spacing : 0)
    spacing: StyleConstants.indicatorSpacing
    opacity: enabled ? 1.0 : StyleConstants.disabledOpacity

    delegate: ItemDelegate {
        required property var model
        required property int index

        // Clear of the scroll bar, which lies over the list's right edge.
        width: ListView.view.width
               - (ListView.view.visibleArea.heightRatio < 1.0 ? StyleConstants.scrollBarThickness : 0)
        text: model[control.textRole]
        font.weight: control.currentIndex === index ? Theme.fonts.bodyStrong.weight
                                                    : Theme.fonts.body.weight
        highlighted: control.highlightedIndex === index
        hoverEnabled: control.hoverEnabled
    }

    searchIndicator.indicator: null

    clearIndicator.indicator: SpinStepIndicator {
        x: control.mirrored ? StyleConstants.fieldBorderWidth
                            : control.width - width - StyleConstants.fieldBorderWidth
        y: StyleConstants.fieldBorderWidth
        height: control.height - 2 * StyleConstants.fieldBorderWidth
        visible: control.text.length > 0
        icon: FluentIcons.Dismiss16
        pressed: control.clearIndicator.pressed
        hovered: control.clearIndicator.hovered
    }

    contentItem: T.TextField {
        id: field

        text: control.text
        placeholderText: control.placeholderText
        placeholderTextColor: Color.transparent(Theme.tokens.textSecondary, StyleConstants.placeholderTextOpacity)
        selectByMouse: control.selectTextByMouse

        font: control.font
        color: Theme.tokens.textPrimary
        selectionColor: Theme.tokens.accent
        selectedTextColor: Theme.tokens.accentText
        verticalAlignment: TextInput.AlignVCenter

        // The template draws no placeholder.
        Text {
            width: field.width
            height: field.height
            text: field.placeholderText
            font: field.font
            color: field.placeholderTextColor
            verticalAlignment: field.verticalAlignment
            visible: !field.length && !field.preeditText
                     && (!field.activeFocus || field.horizontalAlignment !== Qt.AlignHCenter)
            elide: Text.ElideRight
            renderType: field.renderType
        }
    }

    background: FieldBackground {
        implicitWidth: Theme.metrics.contextMenuWidth
        fieldEnabled: control.enabled
        hovered: control.hovered
        focused: control.activeFocus || control.contentItem.activeFocus
    }

    popup: T.Popup {
        y: control.height + StyleConstants.comboPopupGap
        width: control.width
        // An explicit height is never shrunk by the margins, so it is capped
        // to the window here; a longer list then scrolls inside the popup.
        height: Math.min(contentItem.implicitHeight + topPadding + bottomPadding,
                         control.Window.height - topMargin - bottomMargin)
        topMargin: StyleConstants.popupListPadding
        bottomMargin: StyleConstants.popupListPadding
        padding: StyleConstants.popupListPadding

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.delegateModel
            currentIndex: control.highlightedIndex
            highlightMoveDuration: 0

            T.ScrollBar.vertical: ScrollBar {}
        }

        background: PopupBackground {
            radius: Theme.metrics.contextMenuBorderRadius
        }
    }
}
