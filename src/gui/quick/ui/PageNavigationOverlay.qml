pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Templates as T
import wynpix.bridges
import wynpix.style

// Visor/pill overlay flush with the top edge of the window for multi-page
// document page navigation (PDF, TIFF, DjVu).
Item {
    id: root

    required property PageNavigationController navigation

    readonly property int buttonSize: Theme.metrics.contextMenuItemHeight
    readonly property int iconSize: Theme.standardIconSize
    readonly property int cornerRadius: 8
    readonly property int horizontalPadding: 1
    readonly property int verticalPadding: 1
    readonly property int fadeOutDuration: 230

    visible: navigation.visible

    Rectangle {
        id: pill

        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        width: buttons.implicitWidth + 2 * root.horizontalPadding
        height: buttons.implicitHeight + 2 * root.verticalPadding
        bottomLeftRadius: root.cornerRadius
        bottomRightRadius: root.cornerRadius
        topLeftRadius: 0
        topRightRadius: 0
        color: Theme.colors.widget
        border.color: Theme.colors.widgetBorder
        border.width: StyleConstants.separatorThickness
        visible: opacity > 0

        states: State {
            name: "hidden"
            when: !root.navigation.shown
            PropertyChanges {
                pill.opacity: 0
            }
        }

        transitions: Transition {
            to: "hidden"
            NumberAnimation {
                property: "opacity"
                duration: root.fadeOutDuration
                easing.type: Easing.OutQuart
            }
        }

        HoverHandler {
            onHoveredChanged: root.navigation.setHovered(hovered)
        }

        Row {
            id: buttons

            anchors.centerIn: parent

            T.AbstractButton {
                id: prevButton

                implicitWidth: root.buttonSize
                implicitHeight: root.buttonSize
                enabled: root.navigation.hasPrevious
                opacity: enabled ? 1.0 : StyleConstants.disabledOpacity
                focusPolicy: Qt.NoFocus
                hoverEnabled: true

                Accessible.role: Accessible.Button
                Accessible.name: qsTr("Previous page")

                background: Rectangle {
                    radius: StyleConstants.controlRadius
                    color: prevButton.hovered && prevButton.enabled
                           ? Color.transparent(Theme.colors.accent, StyleConstants.accentHighlightOpacity)
                           : "transparent"
                }

                contentItem: IconGlyph {
                    icon: FluentIcons.ChevronLeft48
                    size: root.iconSize
                    color: Theme.colors.icons
                }

                onClicked: Actions.invoke("prevPage")
            }

            T.AbstractButton {
                id: nextButton

                implicitWidth: root.buttonSize
                implicitHeight: root.buttonSize
                enabled: root.navigation.hasNext
                opacity: enabled ? 1.0 : StyleConstants.disabledOpacity
                focusPolicy: Qt.NoFocus
                hoverEnabled: true

                Accessible.role: Accessible.Button
                Accessible.name: qsTr("Next page")

                background: Rectangle {
                    radius: StyleConstants.controlRadius
                    color: nextButton.hovered && nextButton.enabled
                           ? Color.transparent(Theme.colors.accent, StyleConstants.accentHighlightOpacity)
                           : "transparent"
                }

                contentItem: IconGlyph {
                    icon: FluentIcons.ChevronRight48
                    size: root.iconSize
                    color: Theme.colors.icons
                }

                onClicked: Actions.invoke("nextPage")
            }
        }
    }
}
