import QtQuick
import wynpix.bridges
import wynpix.style

// Window controls in the top right corner in fullscreen (ControlsOverlay in
// the widget UI): folder view, settings and exit. They run their actions;
// FullscreenChromeController shows them on pointer moves and keeps them
// while hovered. Faded out, they let input through.
Item {
    id: root

    required property FullscreenChromeController chrome

    readonly property int buttonSize: 30
    readonly property int largeGlyphSize: 20
    readonly property int smallGlyphSize: 16
    readonly property real backgroundOpacity: 0.9
    // Glyphs and fills on the HUD panel, which is dark in both schemes.
    readonly property color glyphColor: Theme.colors.overlayText
    readonly property color hoverColor: Theme.colors.hudHover
    readonly property color pressedColor: Theme.colors.hudPressed

    Rectangle {
        id: bar

        anchors.top: parent.top
        anchors.right: parent.right
        width: buttons.implicitWidth
        height: buttons.implicitHeight
        bottomLeftRadius: StyleConstants.surfaceRadius
        color: Color.transparent(Theme.colors.overlay, root.backgroundOpacity)
        visible: opacity > 0

        states: State {
            name: "hidden"
            when: !root.chrome.controlsShown
            PropertyChanges {
                bar.opacity: 0
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
            onHoveredChanged: root.chrome.setControlsHovered(hovered)
        }

        Row {
            id: buttons

            OverlayHeaderButton {
                glyph: FluentIcons.Grid20
                glyphSize: root.largeGlyphSize
                glyphColor: root.glyphColor
                hoverColor: root.hoverColor
                pressedColor: root.pressedColor
                size: root.buttonSize
                surfaceInset: 0
                Accessible.name: qsTr("Folder view")
                onClicked: Actions.invoke("folderView")
            }
            OverlayHeaderButton {
                glyph: FluentIcons.Settings20
                glyphSize: root.largeGlyphSize
                glyphColor: root.glyphColor
                hoverColor: root.hoverColor
                pressedColor: root.pressedColor
                size: root.buttonSize
                surfaceInset: 0
                Accessible.name: qsTr("Settings")
                onClicked: Actions.invoke("openSettings")
            }
            OverlayHeaderButton {
                glyph: FluentIcons.Dismiss16
                glyphSize: root.smallGlyphSize
                glyphColor: root.glyphColor
                hoverColor: root.hoverColor
                pressedColor: root.pressedColor
                size: root.buttonSize
                surfaceInset: 0
                Accessible.name: qsTr("Exit")
                onClicked: Actions.invoke("exit")
            }
        }
    }
}
