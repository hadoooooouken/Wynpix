import QtQuick
import QtQuick.Layouts
import wynpix.bridges
import wynpix.style

// Frame of the floating overlay panels (FloatingWidget with an overlay
// header in the widget UI): the widget surface with a header of icon, title
// and close button above the content. Pointer input over the panel does not
// reach the viewer behind it. A draggable panel moves by its header, inside
// its parent; resetPosition() returns it to its placement bindings.
Item {
    id: panel

    // A FluentIcons value.
    required property int icon
    required property string title
    property bool draggable: false
    // Title in the emphasis style (Theme.fonts.bodyStrong) instead of body.
    property bool strongTitle: false
    // Space below the content.
    property int bottomPadding: 0
    // Placement while not dragged.
    property real placementX: 0
    property real placementY: 0
    property bool collapsible: false
    property bool collapsed: false
    default property alias content: body.data

    signal closeClicked()

    function resetPosition() {
        x = Qt.binding(() => panel.placementX);
        y = Qt.binding(() => panel.placementY);
    }

    x: placementX
    y: placementY
    implicitHeight: header.height + body.implicitHeight + bottomPadding

    onHeightChanged: {
        if (parent && y + height > parent.height)
            y = Math.max(0, parent.height - height);
    }

    PopupBackground {
        anchors.fill: parent
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.AllButtons
        hoverEnabled: true
        onWheel: wheel => wheel.accepted = true
    }

    Item {
        id: header

        width: parent.width
        height: Theme.metrics.overlayHeaderSize

        RowLayout {
            anchors.fill: parent
            spacing: 0

            IconGlyph {
                icon: panel.icon
                Layout.preferredWidth: Theme.metrics.overlayHeaderSize
                Layout.fillHeight: true
            }

            Label {
                text: panel.title
                color: Theme.colors.textHc
                font: panel.strongTitle ? Theme.fonts.bodyStrong : Theme.fonts.body
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            OverlayHeaderButton {
                id: collapseButton

                visible: panel.collapsible
                glyph: panel.collapsed ? FluentIcons.ChevronDown20 : FluentIcons.ChevronUp20
                glyphSize: Theme.standardIconSize
                Accessible.name: panel.collapsed ? qsTr("Expand") : qsTr("Collapse")
                onClicked: panel.collapsed = !panel.collapsed
            }

            OverlayHeaderButton {
                id: closeButton

                glyph: FluentIcons.Dismiss16
                onClicked: panel.closeClicked()
            }
        }

        DragHandler {
            enabled: panel.draggable
            target: panel
            cursorShape: active ? Qt.ClosedHandCursor : Qt.OpenHandCursor
            xAxis.minimum: 0
            xAxis.maximum: panel.parent ? Math.max(0, panel.parent.width - panel.width) : 0
            yAxis.minimum: 0
            yAxis.maximum: panel.parent ? Math.max(0, panel.parent.height - panel.height) : 0
        }

        TapHandler {
            enabled: panel.collapsible
            onDoubleTapped: eventPoint => {
                const buttonX = collapseButton.visible ? collapseButton.x : closeButton.x;
                if (eventPoint.position.x < buttonX)
                    panel.collapsed = !panel.collapsed;
            }
        }
    }

    Column {
        id: body

        anchors.top: header.bottom
        width: parent.width
    }
}
