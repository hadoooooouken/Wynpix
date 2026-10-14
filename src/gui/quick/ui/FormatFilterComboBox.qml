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
    readonly property int popupTopGap: 10
    // Distance kept between the popup and the screen's right edge.
    readonly property int popupRightGap: 20
    readonly property int controlRadius: 4
    readonly property int borderWidth: 2
    // Text and glyph on the accent field (Qt::white in the widget).
    readonly property color limitedContentColor: "white"

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

    Accessible.role: Accessible.ComboBox
    Accessible.name: filter.displayText

    FontMetrics {
        id: metrics

        font: Theme.fonts.body
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

        y: control.height + control.popupTopGap
        popupType: T.Popup.Window
        padding: control.popupMargin
        // Negative insets make the popup window large enough for the shadow;
        // the frame keeps the popup's own size.
        topInset: -StyleConstants.popupShadowMargin
        bottomInset: -StyleConstants.popupShadowMargin
        leftInset: -StyleConstants.popupShadowMargin
        rightInset: -StyleConstants.popupShadowMargin
        closePolicy: T.Popup.CloseOnEscape | T.Popup.CloseOnPressOutsideParent

        // Below the field, shifted left as far as needed to keep the gap to
        // the screen's right edge.
        onAboutToShow: {
            const fieldX = control.mapToGlobal(0, 0).x;
            const screenLeft = control.Screen.virtualX;
            const screenRight = screenLeft + control.Screen.width;
            const rightmostX = screenRight - control.popupRightGap - popup.width;
            popup.x = Math.max(screenLeft, Math.min(fieldX, rightmostX)) - fieldX;
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

        contentItem: Column {
            spacing: control.popupSpacing

            ModelCheckBox {
                text: control.filter.allFormatsLabel
                font: Theme.fonts.bodyStrong
                checked: control.filter.allFormats
                onClicked: control.filter.selectAllFormats()
            }

            Rectangle {
                width: parent.width
                height: 1
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

    // The model decides the state; a click does not toggle it by itself.
    component ModelCheckBox: CheckBox {
        id: box

        focusPolicy: Qt.NoFocus
        nextCheckState: function() {
            return box.checkState;
        }
    }
}
