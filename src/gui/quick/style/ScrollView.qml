import QtQuick
import QtQuick.Templates as T

// Scroll view with the style's scroll bars (a style that customizes ScrollBar
// must also provide ScrollView, which otherwise creates the fallback
// style's scroll bars). The vertical bar overlays the view, so while it is
// shown its thickness is reserved beside the content, as QScrollArea does.
// In a dialog window the wheel scrolls on that window's frames.
T.ScrollView {
    id: control

    // A property, not a child: a child would become content of the view.
    readonly property DialogWheelScroller wheelScroller: DialogWheelScroller {
        flickable: control.contentItem
    }

    readonly property bool verticalBarShown: {
        const bar = control.T.ScrollBar.vertical
        return bar !== null && bar.policy !== T.ScrollBar.AlwaysOff
               && (bar.policy === T.ScrollBar.AlwaysOn || bar.size < 1.0)
    }
    readonly property real verticalBarReserve: verticalBarShown ? T.ScrollBar.vertical.width : 0

    leftPadding: mirrored ? verticalBarReserve : 0
    rightPadding: mirrored ? 0 : verticalBarReserve

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    T.ScrollBar.vertical: ScrollBar {
        parent: control
        x: control.mirrored ? 0 : control.width - width
        y: control.topPadding
        height: control.availableHeight
        active: control.T.ScrollBar.horizontal.active
    }

    T.ScrollBar.horizontal: ScrollBar {
        parent: control
        x: control.leftPadding
        y: control.height - height
        width: control.availableWidth
        active: control.T.ScrollBar.vertical.active
    }
}
