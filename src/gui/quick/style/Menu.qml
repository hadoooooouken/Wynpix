import QtQuick
import QtQuick.Templates as T
import wynpix.bridges

// Menu with the context menu frame of the widget UI: the widget surface,
// a thin border, rounded corners and a drop shadow. Runs of separators
// collapse (Menu.separatorsCollapsible) like QMenu's.
T.Menu {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    margins: 0
    overlap: 1
    padding: StyleConstants.popupListPadding
    // Negative insets make a Popup.Window menu's window large enough for the
    // shadow; the frame keeps the menu's own size.
    topInset: -StyleConstants.popupShadowMargin
    bottomInset: -StyleConstants.popupShadowMargin
    leftInset: -StyleConstants.popupShadowMargin
    rightInset: -StyleConstants.popupShadowMargin
    separatorsCollapsible: true

    delegate: MenuItem {}

    contentItem: ListView {
        implicitHeight: contentHeight
        model: control.contentModel
        interactive: Window.window
                     ? contentHeight + control.topPadding + control.bottomPadding > control.height
                     : false
        clip: true
        currentIndex: control.currentIndex

        T.ScrollBar.vertical: ScrollBar {}
    }

    background: Item {
        implicitWidth: Theme.metrics.contextMenuWidth - control.leftInset - control.rightInset

        PopupBackground {
            anchors.fill: parent
            anchors.leftMargin: -control.leftInset
            anchors.rightMargin: -control.rightInset
            anchors.topMargin: -control.topInset
            anchors.bottomMargin: -control.bottomInset
            radius: Theme.metrics.contextMenuBorderRadius
        }
    }

    T.Overlay.modal: Rectangle {
        color: StyleConstants.modalDimColor
    }

    T.Overlay.modeless: Rectangle {
        color: StyleConstants.modelessDimColor
    }
}
