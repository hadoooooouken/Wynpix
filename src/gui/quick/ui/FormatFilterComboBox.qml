pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Templates as T
import wynpix.bridges
import wynpix.style

// The format filter of the folder view top bar (FormatFilterComboBox in the
// widget UI): a field showing the filter, accent coloured while it limits
// the formats, that opens a popup with "All formats" and the formats by
// category. FormatFilterModel decides; the check boxes only report clicks
// and show the model's state.
T.AbstractButton {
    id: control

    required property FormatFilterModel filter

    readonly property bool popupOpen: popup.opened
    readonly property bool limited: !filter.allFormats

    // FormatFilterComboBox constants.
    readonly property int textLeftPadding: 9
    readonly property int textIconSpacing: 4
    readonly property int iconRightPadding: 8
    readonly property int popupMargin: 10
    readonly property int popupSpacing: 8
    readonly property int formatColumnWidth: 128
    readonly property int formatColumns: 4
    readonly property int formatIndent: 20
    // Gap between the bottom of the control and the top of the popup frame.
    readonly property int popupTopGap: 10
    // Minimum gaps kept between the outer edge of the popup drop shadow and the window edges.
    readonly property int popupBottomGap: 10
    readonly property int popupLeftGap: 10
    readonly property int popupRightGap: 10
    readonly property int controlRadius: 4
    readonly property int borderWidth: 2
    // Text and glyph on the accent field.
    readonly property color limitedContentColor: Theme.colors.accentText

    readonly property int popupContentWidth: formatIndent + formatColumns * formatColumnWidth
                                             + (formatColumns - 1) * popupSpacing
    readonly property int popupBaseWidth: popupContentWidth + 2 * popupMargin

    readonly property real maxAllowedWidth: {
        if (!control.Window.window)
            return control.popupBaseWidth;
        return Math.max(0, control.Window.window.width - control.popupLeftGap - control.popupRightGap
                           - 2 * StyleConstants.popupShadowMargin);
    }
    readonly property real maxAllowedHeight: {
        if (!control.Window.window)
            return popup.implicitHeight;
        const fieldY = control.mapToItem(null, 0, 0).y;
        const popupWindowY = fieldY + control.height + control.popupTopGap;
        return Math.max(0, control.Window.window.height - popupWindowY
                           - control.popupBottomGap - StyleConstants.popupShadowMargin);
    }
    readonly property bool verticalScrollNeeded: popup.implicitHeight > control.maxAllowedHeight
    readonly property int targetPopupWidth: popupBaseWidth
                                            + (verticalScrollNeeded ? StyleConstants.scrollBarThickness : 0)

    readonly property real widestText: {
        let width = 0;
        for (const text of filter.displayTexts)
            width = Math.max(width, metrics.advanceWidth(text));
        return Math.ceil(width);
    }

    implicitWidth: textLeftPadding + widestText + textIconSpacing + Theme.compactIconSize
                   + iconRightPadding + 2 * borderWidth
    focusPolicy: Qt.NoFocus
    hoverEnabled: true
    onClicked: popup.opened ? popup.close() : popup.open()
    onXChanged: {
        if (popup.opened)
            repositionPopup();
    }

    Accessible.role: Accessible.ComboBox
    Accessible.name: filter.displayText

    FontMetrics {
        id: metrics

        font: Theme.fonts.body
    }

    function repositionPopup() {
        if (!control.Window.window)
            return;

        const win = control.Window.window;
        const fieldPos = control.mapToItem(null, 0, 0);
        const fieldX = fieldPos.x;

        const rightmostX = win.width - control.popupRightGap - StyleConstants.popupShadowMargin - popup.width;
        const leftmostX = control.popupLeftGap + StyleConstants.popupShadowMargin;
        const targetWindowX = Math.max(leftmostX, Math.min(fieldX, rightmostX));
        popup.x = targetWindowX - fieldX;
    }

    Connections {
        target: control.Window.window

        function onWidthChanged() {
            if (popup.opened)
                control.repositionPopup();
        }
    }

    background: Rectangle {
        radius: control.controlRadius
        color: control.limited ? Theme.colors.accent
             : control.hovered || control.popupOpen ? Theme.colors.panelButtonHover
             : Theme.colors.panelButton
        border.width: control.borderWidth
        border.color: color
    }

    contentItem: Item {
        Text {
            x: control.textLeftPadding
            width: parent.width - x - glyph.width - control.textIconSpacing - control.iconRightPadding
            height: parent.height
            text: control.filter.displayText
            color: control.limited ? control.limitedContentColor : Theme.colors.textHc
            font: Theme.fonts.body
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }

        IconGlyph {
            id: glyph

            x: parent.width - width - control.iconRightPadding
            anchors.verticalCenter: parent.verticalCenter
            icon: FluentIcons.Checkmark16
            size: Theme.compactIconSize
            color: control.limited ? control.limitedContentColor : Theme.colors.icons
        }
    }

    Popup {
        id: popup

        parent: control

        y: control.height + control.popupTopGap
        width: Math.min(control.targetPopupWidth, control.maxAllowedWidth)
        height: Math.min(implicitHeight, control.maxAllowedHeight)
        popupType: T.Popup.Item
        padding: control.popupMargin
        // Negative insets make the popup window large enough for the shadow;
        // the frame keeps the popup's own size.
        topInset: -StyleConstants.popupShadowMargin
        bottomInset: -StyleConstants.popupShadowMargin
        leftInset: -StyleConstants.popupShadowMargin
        rightInset: -StyleConstants.popupShadowMargin
        closePolicy: T.Popup.CloseOnEscape | T.Popup.CloseOnPressOutsideParent

        onAboutToShow: {
            if (scrollView.contentItem)
                scrollView.contentItem.contentY = 0;
            control.repositionPopup();
        }
        onWidthChanged: {
            if (popup.opened)
                control.repositionPopup();
        }

        background: Item {
            PopupBackground {
                anchors.fill: parent
                anchors.leftMargin: -popup.leftInset
                anchors.rightMargin: -popup.rightInset
                anchors.topMargin: -popup.topInset
                anchors.bottomMargin: -popup.bottomInset
                radius: Theme.metrics.contextMenuBorderRadius
            }
        }

        contentItem: ScrollView {
            id: scrollView

            clip: true
            contentWidth: contentColumn.width
            T.ScrollBar.horizontal.policy: popup.width < control.popupBaseWidth
                                         ? T.ScrollBar.AsNeeded
                                         : T.ScrollBar.AlwaysOff
            T.ScrollBar.vertical.policy: control.verticalScrollNeeded
                                         ? T.ScrollBar.AsNeeded
                                         : T.ScrollBar.AlwaysOff

            Column {
                id: contentColumn

                width: Math.max(control.popupContentWidth, scrollView.availableWidth)
                spacing: control.popupSpacing

                ModelCheckBox {
                    text: control.filter.allFormatsLabel
                    font: Theme.fonts.bodyStrong
                    checked: control.filter.allFormats
                    onClicked: control.filter.selectAllFormats()
                }

                Rectangle {
                    width: parent.width
                    height: StyleConstants.separatorThickness
                    color: Theme.colors.widgetBorder
                }

                Repeater {
                    model: control.filter

                    delegate: Column {
                        id: category

                        required property int index
                        required property string label
                        required property int checkState
                        required property var formats

                        spacing: control.popupSpacing

                        ModelCheckBox {
                            text: category.label
                            font: Theme.fonts.bodyStrong
                            tristate: true
                            checkState: category.checkState
                            onClicked: control.filter.setCategoryChecked(category.index,
                                                                         category.checkState !== Qt.Checked)
                        }

                        Grid {
                            leftPadding: control.formatIndent
                            columns: control.formatColumns
                            columnSpacing: control.popupSpacing
                            rowSpacing: control.popupSpacing

                            Repeater {
                                model: category.formats

                                delegate: ModelCheckBox {
                                    required property var modelData

                                    width: control.formatColumnWidth
                                    text: modelData.label
                                    checked: modelData.checked
                                    onClicked: control.filter.setFormatChecked(modelData.index,
                                                                               !modelData.checked)
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // The model decides the state; a click does not toggle it by itself.
    component ModelCheckBox: CheckBox {
        id: box

        focusPolicy: Qt.NoFocus
        nextCheckState: function() {
            return box.checkState;
        }
    }
}
