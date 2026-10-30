pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
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
    readonly property int horizontalPadding: 8
    readonly property int verticalPadding: 4

    visible: navigation.visible

    TextMetrics {
        id: currentPageMetrics

        font: Theme.fonts.body
        text: {
            const count = Math.max(10, root.navigation.pageCount);
            return count.toString().replace(/./g, "8");
        }
    }

    Rectangle {
        id: pill

        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        width: controlsRow.implicitWidth + 2 * root.horizontalPadding
        height: controlsRow.implicitHeight + 2 * root.verticalPadding
        bottomLeftRadius: StyleConstants.surfaceRadius
        bottomRightRadius: StyleConstants.surfaceRadius
        topLeftRadius: 0
        topRightRadius: 0
        color: Theme.tokens.surfacePopup
        border.color: Theme.tokens.strokeDefault
        border.width: StyleConstants.separatorThickness
        visible: opacity > 0

        states: State {
            name: "hidden"
            when: !root.navigation.shown
            PropertyChanges {
                pill.opacity: 0
            }
        }

        transitions: [
            Transition {
                to: "hidden"
                NumberAnimation {
                    property: "opacity"
                    duration: StyleConstants.hudFadeOutDuration
                    easing.type: Easing.OutQuart
                }
            },
            Transition {
                from: "hidden"
                NumberAnimation {
                    property: "opacity"
                    duration: StyleConstants.hudFadeInDuration
                    easing.type: Easing.OutCubic
                }
            }
        ]

        HoverHandler {
            onHoveredChanged: root.navigation.setHovered(hovered)
        }

        RowLayout {
            id: controlsRow

            anchors.centerIn: parent
            spacing: 3

            // 1. First page (|◁)
            T.AbstractButton {
                id: firstPageButton

                implicitWidth: root.buttonSize
                implicitHeight: root.buttonSize
                enabled: root.navigation.hasPrevious
                opacity: enabled ? 1.0 : StyleConstants.disabledOpacity
                focusPolicy: Qt.NoFocus
                hoverEnabled: true

                Accessible.role: Accessible.Button
                Accessible.name: qsTr("First page")

                background: Rectangle {
                    radius: StyleConstants.controlRadius
                    color: firstPageButton.down ? Theme.tokens.fillPressed
                           : firstPageButton.hovered && firstPageButton.enabled ? Theme.tokens.fillHover
                           : "transparent"
                }

                contentItem: IconGlyph {
                    icon: FluentIcons.Next20
                    rotation: 180
                    size: root.iconSize
                    color: Theme.tokens.iconDefault
                }

                ToolTip {
                    text: qsTr("First page")
                    visible: firstPageButton.hovered
                }

                onClicked: root.navigation.firstPage()
            }

            // 2. Previous page (<)
            T.AbstractButton {
                id: prevPageButton

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
                    color: prevPageButton.down ? Theme.tokens.fillPressed
                           : prevPageButton.hovered && prevPageButton.enabled ? Theme.tokens.fillHover
                           : "transparent"
                }

                contentItem: IconGlyph {
                    icon: FluentIcons.ChevronLeft48
                    size: root.iconSize
                    color: Theme.tokens.iconDefault
                }

                ToolTip {
                    text: qsTr("Previous page")
                    visible: prevPageButton.hovered
                }

                onClicked: root.navigation.prevPage()
            }

            // 3. Next page (>)
            T.AbstractButton {
                id: nextPageButton

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
                    color: nextPageButton.down ? Theme.tokens.fillPressed
                           : nextPageButton.hovered && nextPageButton.enabled ? Theme.tokens.fillHover
                           : "transparent"
                }

                contentItem: IconGlyph {
                    icon: FluentIcons.ChevronRight48
                    size: root.iconSize
                    color: Theme.tokens.iconDefault
                }

                ToolTip {
                    text: qsTr("Next page")
                    visible: nextPageButton.hovered
                }

                onClicked: root.navigation.nextPage()
            }

            // 4. Last page (▷|)
            T.AbstractButton {
                id: lastPageButton

                implicitWidth: root.buttonSize
                implicitHeight: root.buttonSize
                enabled: root.navigation.hasNext
                opacity: enabled ? 1.0 : StyleConstants.disabledOpacity
                focusPolicy: Qt.NoFocus
                hoverEnabled: true

                Accessible.role: Accessible.Button
                Accessible.name: qsTr("Last page")

                background: Rectangle {
                    radius: StyleConstants.controlRadius
                    color: lastPageButton.down ? Theme.tokens.fillPressed
                           : lastPageButton.hovered && lastPageButton.enabled ? Theme.tokens.fillHover
                           : "transparent"
                }

                contentItem: IconGlyph {
                    icon: FluentIcons.Next20
                    size: root.iconSize
                    color: Theme.tokens.iconDefault
                }

                ToolTip {
                    text: qsTr("Last page")
                    visible: lastPageButton.hovered
                }

                onClicked: root.navigation.lastPage()
            }

            // 5. Page Slider
            Slider {
                id: pageSlider

                implicitWidth: 160
                Layout.alignment: Qt.AlignVCenter
                from: 0
                to: Math.max(0, root.navigation.pageCount - 1)
                stepSize: 1
                value: root.navigation.pageIndex

                onMoved: root.navigation.seekToPage(Math.round(value))
                onPressedChanged: {
                    if (pressed) {
                        root.navigation.onScrubbingStarted();
                    } else {
                        root.navigation.onScrubbingFinished();
                        value = Qt.binding(() => root.navigation.pageIndex);
                    }
                }
            }

            // 6. Page counter (e.g. "3 / 17")
            Row {
                Layout.alignment: Qt.AlignVCenter
                Layout.leftMargin: 4
                Layout.rightMargin: 4
                spacing: 0

                Text {
                    width: Math.ceil(currentPageMetrics.width)
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                    text: (pageSlider.pressed
                           ? Math.round(pageSlider.value) + 1
                           : root.navigation.pageIndex + 1).toString()
                    color: Theme.tokens.textSecondary
                    font: Theme.fonts.body
                }

                Text {
                    verticalAlignment: Text.AlignVCenter
                    text: " / " + root.navigation.pageCount
                    color: Theme.tokens.textSecondary
                    font: Theme.fonts.body
                }
            }
        }
    }
}
