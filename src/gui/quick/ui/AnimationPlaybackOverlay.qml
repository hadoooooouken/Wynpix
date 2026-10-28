pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import wynpix.bridges
import wynpix.style

// Visor/pill overlay flush with the top edge of the window for animation playback
// controls (GIF, animated WebP, APNG).
Item {
    id: root

    required property AnimationPlaybackController playback

    readonly property int buttonSize: Theme.metrics.contextMenuItemHeight
    readonly property int iconSize: Theme.standardIconSize
    readonly property int horizontalPadding: 8
    readonly property int verticalPadding: 4

    visible: playback.visible

    onVisibleChanged: {
        if (!visible && speedMenu.visible)
            speedMenu.close();
    }

    Connections {
        target: root.playback
        function onShownChanged() {
            if (!root.playback.shown && speedMenu.visible)
                speedMenu.close();
        }
    }

    TextMetrics {
        id: currentFrameMetrics

        font: Theme.fonts.body
        text: {
            const count = Math.max(10, root.playback.frameCount);
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
            when: !root.playback.shown
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
            id: pillHover
            onHoveredChanged: root.playback.setHovered(hovered || speedMenu.visible)
        }

        RowLayout {
            id: controlsRow

            anchors.centerIn: parent
            spacing: 3

            // 1. First frame (|◁)
            T.AbstractButton {
                id: firstFrameButton

                implicitWidth: root.buttonSize
                implicitHeight: root.buttonSize
                focusPolicy: Qt.NoFocus
                hoverEnabled: true

                Accessible.role: Accessible.Button
                Accessible.name: qsTr("First frame")

                background: Rectangle {
                    radius: StyleConstants.controlRadius
                    color: firstFrameButton.down ? Theme.tokens.fillPressed
                           : firstFrameButton.hovered ? Theme.tokens.fillHover
                           : "transparent"
                }

                contentItem: IconGlyph {
                    icon: FluentIcons.Next20
                    rotation: 180
                    size: root.iconSize
                    color: Theme.tokens.iconDefault
                }

                ToolTip {
                    text: qsTr("First frame")
                    visible: firstFrameButton.hovered
                }

                onClicked: root.playback.firstFrame()
            }

            // 2. Previous frame (<)
            T.AbstractButton {
                id: prevFrameButton

                implicitWidth: root.buttonSize
                implicitHeight: root.buttonSize
                focusPolicy: Qt.NoFocus
                hoverEnabled: true

                Accessible.role: Accessible.Button
                Accessible.name: qsTr("Previous frame")

                background: Rectangle {
                    radius: StyleConstants.controlRadius
                    color: prevFrameButton.down ? Theme.tokens.fillPressed
                           : prevFrameButton.hovered ? Theme.tokens.fillHover
                           : "transparent"
                }

                contentItem: IconGlyph {
                    icon: FluentIcons.ChevronDown20
                    rotation: 90
                    size: root.iconSize
                    color: Theme.tokens.iconDefault
                }

                ToolTip {
                    text: qsTr("Previous frame")
                    visible: prevFrameButton.hovered
                }

                onClicked: root.playback.prevFrame()
            }

            // 3. Play / Pause (▶ / ⏸)
            T.AbstractButton {
                id: playPauseButton

                implicitWidth: root.buttonSize
                implicitHeight: root.buttonSize
                focusPolicy: Qt.NoFocus
                hoverEnabled: true

                Accessible.role: Accessible.Button
                Accessible.name: root.playback.playing ? qsTr("Pause") : qsTr("Play")

                background: Rectangle {
                    radius: StyleConstants.controlRadius
                    color: playPauseButton.down ? Theme.tokens.accentPressed
                         : playPauseButton.hovered ? Theme.tokens.accentHover
                         : Theme.tokens.accent
                }

                contentItem: IconGlyph {
                    icon: root.playback.playing ? FluentIcons.Pause20 : FluentIcons.Play20
                    size: root.iconSize
                    color: Theme.tokens.accentText
                }

                ToolTip {
                    text: root.playback.playing ? qsTr("Pause") : qsTr("Play")
                    visible: playPauseButton.hovered
                }

                onClicked: root.playback.togglePlayPause()
            }

            // 4. Next frame (>)
            T.AbstractButton {
                id: nextFrameButton

                implicitWidth: root.buttonSize
                implicitHeight: root.buttonSize
                focusPolicy: Qt.NoFocus
                hoverEnabled: true

                Accessible.role: Accessible.Button
                Accessible.name: qsTr("Next frame")

                background: Rectangle {
                    radius: StyleConstants.controlRadius
                    color: nextFrameButton.down ? Theme.tokens.fillPressed
                           : nextFrameButton.hovered ? Theme.tokens.fillHover
                           : "transparent"
                }

                contentItem: IconGlyph {
                    icon: FluentIcons.ChevronDown20
                    rotation: -90
                    size: root.iconSize
                    color: Theme.tokens.iconDefault
                }

                ToolTip {
                    text: qsTr("Next frame")
                    visible: nextFrameButton.hovered
                }

                onClicked: root.playback.nextFrame()
            }

            // 5. Last frame (▷|)
            T.AbstractButton {
                id: lastFrameButton

                implicitWidth: root.buttonSize
                implicitHeight: root.buttonSize
                focusPolicy: Qt.NoFocus
                hoverEnabled: true

                Accessible.role: Accessible.Button
                Accessible.name: qsTr("Last frame")

                background: Rectangle {
                    radius: StyleConstants.controlRadius
                    color: lastFrameButton.down ? Theme.tokens.fillPressed
                           : lastFrameButton.hovered ? Theme.tokens.fillHover
                           : "transparent"
                }

                contentItem: IconGlyph {
                    icon: FluentIcons.Next20
                    size: root.iconSize
                    color: Theme.tokens.iconDefault
                }

                ToolTip {
                    text: qsTr("Last frame")
                    visible: lastFrameButton.hovered
                }

                onClicked: root.playback.lastFrame()
            }

            // 6. Reverse playback (↵)
            T.AbstractButton {
                id: reverseButton

                implicitWidth: root.buttonSize
                implicitHeight: root.buttonSize
                focusPolicy: Qt.NoFocus
                hoverEnabled: true
                checkable: true
                checked: root.playback.reversed

                Accessible.role: Accessible.Button
                Accessible.name: qsTr("Reverse playback")

                background: Rectangle {
                    radius: StyleConstants.controlRadius
                    color: reverseButton.checked
                           ? (reverseButton.down ? Theme.tokens.accentPressed
                              : reverseButton.hovered ? Theme.tokens.accentHover
                              : Theme.tokens.accent)
                           : (reverseButton.down ? Theme.tokens.fillPressed
                             : reverseButton.hovered ? Theme.tokens.fillHover
                             : "transparent")
                }

                contentItem: IconGlyph {
                    icon: FluentIcons.ArrowTurnDownLeft20
                    size: root.iconSize
                    color: reverseButton.checked ? Theme.tokens.accentText : Theme.tokens.iconDefault
                }

                ToolTip {
                    text: qsTr("Reverse playback")
                    visible: reverseButton.hovered
                }

                onClicked: root.playback.toggleReverse()
            }

            // 7. Loop (🔁)
            T.AbstractButton {
                id: loopButton

                implicitWidth: root.buttonSize
                implicitHeight: root.buttonSize
                focusPolicy: Qt.NoFocus
                hoverEnabled: true
                checkable: true
                checked: root.playback.loop

                Accessible.role: Accessible.Button
                Accessible.name: qsTr("Loop playback")

                background: Rectangle {
                    radius: StyleConstants.controlRadius
                    color: loopButton.checked
                           ? (loopButton.down ? Theme.tokens.accentPressed
                              : loopButton.hovered ? Theme.tokens.accentHover
                              : Theme.tokens.accent)
                           : (loopButton.down ? Theme.tokens.fillPressed
                             : loopButton.hovered ? Theme.tokens.fillHover
                             : "transparent")
                }

                contentItem: IconGlyph {
                    icon: FluentIcons.ArrowRepeatAll20
                    size: root.iconSize
                    color: loopButton.checked ? Theme.tokens.accentText : Theme.tokens.iconDefault
                }

                ToolTip {
                    text: qsTr("Loop playback")
                    visible: loopButton.hovered
                }

                onClicked: root.playback.toggleLoop()
            }

            // 8. Speed button (1x)
            T.AbstractButton {
                id: speedButton

                implicitWidth: root.buttonSize + 14
                implicitHeight: root.buttonSize
                focusPolicy: Qt.NoFocus
                hoverEnabled: true

                Accessible.role: Accessible.Button
                Accessible.name: qsTr("Playback speed")

                background: Rectangle {
                    radius: StyleConstants.controlRadius
                    color: speedButton.down ? Theme.tokens.fillPressed
                           : speedButton.hovered || speedMenu.visible ? Theme.tokens.fillHover
                           : "transparent"
                }

                contentItem: Text {
                    text: root.playback.speedText
                    color: Theme.tokens.textSecondary
                    font: Theme.fonts.body
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                ToolTip {
                    text: qsTr("Playback speed")
                    visible: speedButton.hovered && !speedMenu.visible
                }

                onClicked: {
                    if (speedMenu.visible)
                        speedMenu.close();
                    else
                        speedMenu.open();
                }

                Menu {
                    id: speedMenu

                    y: speedButton.height + 4
                    width: 100

                    onVisibleChanged: root.playback.setHovered(pillHover.hovered || visible)

                    MenuItem {
                        text: "0.25x"
                        checkable: true
                        checked: Math.abs(root.playback.speed - 0.25) < 0.01
                        onTriggered: root.playback.setSpeed(0.25)
                    }

                    MenuItem {
                        text: "0.5x"
                        checkable: true
                        checked: Math.abs(root.playback.speed - 0.5) < 0.01
                        onTriggered: root.playback.setSpeed(0.5)
                    }

                    MenuItem {
                        text: "1x"
                        checkable: true
                        checked: Math.abs(root.playback.speed - 1.0) < 0.01
                        onTriggered: root.playback.setSpeed(1.0)
                    }

                    MenuItem {
                        text: "2x"
                        checkable: true
                        checked: Math.abs(root.playback.speed - 2.0) < 0.01
                        onTriggered: root.playback.setSpeed(2.0)
                    }

                    MenuItem {
                        text: "3x"
                        checkable: true
                        checked: Math.abs(root.playback.speed - 3.0) < 0.01
                        onTriggered: root.playback.setSpeed(3.0)
                    }
                }
            }

            // 9. Timeline Slider
            Slider {
                id: timelineSlider

                implicitWidth: 160
                Layout.alignment: Qt.AlignVCenter
                from: 0
                to: Math.max(0, root.playback.frameCount - 1)
                stepSize: 1
                value: root.playback.frameIndex

                onMoved: root.playback.seekToFrame(Math.round(value))
                onPressedChanged: {
                    if (pressed)
                        root.playback.onScrubbingStarted();
                    else
                        root.playback.onScrubbingFinished();
                }
            }

            // 10. Frame counter (e.g. "17 / 88")
            Row {
                id: frameCounterRow

                Layout.alignment: Qt.AlignVCenter
                Layout.leftMargin: 4
                Layout.rightMargin: 4
                spacing: 0

                Text {
                    id: currentFrameText

                    width: Math.ceil(currentFrameMetrics.width)
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                    text: (root.playback.frameIndex + 1).toString()
                    color: Theme.tokens.textSecondary
                    font: Theme.fonts.body
                }

                Text {
                    id: totalFrameText

                    verticalAlignment: Text.AlignVCenter
                    text: " / " + root.playback.frameCount
                    color: Theme.tokens.textSecondary
                    font: Theme.fonts.body
                }
            }

            // 11. Remember settings across images
            CheckBox {
                id: rememberCheckBox

                Layout.alignment: Qt.AlignVCenter
                Layout.leftMargin: 6
                text: qsTr("Remember")
                font: Theme.fonts.caption
                checked: root.playback.rememberSettings
                focusPolicy: Qt.NoFocus

                Accessible.role: Accessible.CheckBox
                Accessible.name: qsTr("Remember playback settings")

                onToggled: root.playback.setRememberSettings(checked)

                ToolTip {
                    text: qsTr("Preserve playback settings across images")
                    visible: rememberCheckBox.hovered
                }
            }
        }
    }
}
