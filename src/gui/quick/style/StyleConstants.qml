pragma Singleton

import QtQuick

// Fixed sizes and colour factors of the wynpix.style controls. They are the
// values of the former widget stylesheet and ProxyStyle; the sizes that
// scale with the font come from Theme.metrics.
QtObject {
    // Corner radius of buttons, fields, combo boxes and panel buttons.
    readonly property int controlRadius: 4
    // Floating panels and plain popups (FloatingWidget).
    readonly property int popupRadius: 8
    // Highlighted rows of menus and combo box lists.
    readonly property int itemRadius: 6

    // Push button padding.
    readonly property int buttonVerticalPadding: 7
    readonly property int buttonHorizontalPadding: 12
    // Panel buttons (ToolButton): inset from the panel edge and padding
    // around the icon.
    readonly property int toolButtonVerticalInset: 4
    readonly property int toolButtonHorizontalInset: 2
    readonly property int toolButtonPadding: 6

    // Text fields, spin boxes and search fields; also the width of the focus
    // ring of every entry control, combo boxes included.
    readonly property int fieldBorderWidth: 2
    readonly property int fieldVerticalPadding: 4
    readonly property int fieldHorizontalPadding: 6
    readonly property int spinBoxVerticalPadding: 2
    readonly property int spinBoxMinimumContentHeight: 20
    readonly property int spinStepIndicatorWidth: 18

    // Combo boxes (the flat panel ProxyStyle draws).
    readonly property int comboBorderWidth: 1
    readonly property int comboVerticalPadding: 4
    readonly property int comboHorizontalPadding: 8
    readonly property int comboPopupGap: 2
    readonly property int comboImplicitWidth: 140

    // Rows of combo box lists and other item delegates.
    readonly property int itemVerticalPadding: 4
    readonly property int itemHorizontalPadding: 5
    readonly property int itemMinimumHeight: 22

    // Menus and popup lists: padding inside the frame, row padding and
    // separator spacing.
    readonly property int popupListPadding: 5
    readonly property int popupPadding: 12
    // Dialog windows: margin around the content, spacing of its rows and of
    // the buttons (the layouts of the widget dialogs).
    readonly property int dialogPadding: 12
    readonly property int dialogSpacing: 6
    readonly property int dialogButtonSpacing: 6
    readonly property int menuItemHorizontalPadding: 8
    readonly property int menuItemSpacing: 8
    readonly property int menuSeparatorVerticalPadding: 4
    // Minimum space between a menu item's label and its shortcut.
    readonly property int menuShortcutGap: 24
    readonly property int separatorThickness: 1

    // Tooltips.
    readonly property int toolTipVerticalPadding: 4
    readonly property int toolTipHorizontalPadding: 6
    readonly property int toolTipMargins: 6
    readonly property int toolTipOffset: 3

    // Sliders.
    readonly property int sliderThickness: 20
    readonly property int sliderLength: 200
    readonly property int sliderGrooveThickness: 4
    readonly property int sliderHandleSize: 16
    readonly property int sliderHandleBorderWidth: 2

    // Progress bars.
    readonly property int progressBarRadius: 3
    readonly property int progressBarBorderWidth: 1
    readonly property int progressBarTextPadding: 2

    // Scroll bars.
    readonly property int scrollBarThickness: 13
    readonly property int scrollBarMinimumHandleLength: 60
    // Space around the handle inside the bar's thickness; the rounded handle
    // floats in the track, so the bar keeps its overall thickness.
    readonly property int scrollBarHandleInset: 2
    // Gap between a shown scroll bar and the content beside it.
    readonly property int scrollBarContentSpacing: 8

    // Glyph sizes: check and radio indicators, chevrons.
    readonly property int indicatorSize: 16
    readonly property int chevronSize: 12
    // Rotation that turns a downward chevron sideways.
    readonly property real quarterTurn: 90
    readonly property int indicatorSpacing: 6
    readonly property int indicatorPadding: 6

    // Colour factors.
    // Secondary text (shortcuts, disabled menu items): text at this opacity.
    readonly property real secondaryTextOpacity: 0.62
    // Disabled icons, indicators and control content.
    readonly property real disabledOpacity: 0.45
    // Hovered slider handle: accent lightened by this factor.
    readonly property real accentLightFactor: 1.3

    // HUD and toast overlay transitions (zoom indicator, fullscreen chrome, page navigation, animation HUD, floating messages).
    readonly property int hudFadeInDuration: 120
    readonly property int hudFadeOutDuration: 230

    // Drop shadow of popups, menus and tooltips.
    readonly property color shadowColor: Qt.rgba(0, 0, 0, 0.3)
    readonly property real shadowBlur: 12
    readonly property real shadowOffsetY: 3
    // Room around a popup window's frame for its drop shadow; without it the
    // window clips the shadow to a rectangle that shows past the frame's
    // rounded corners.
    readonly property real popupShadowMargin: shadowBlur + shadowOffsetY
    // Dimming behind modal and modeless popups.
    readonly property color modalDimColor: Qt.rgba(0, 0, 0, 0.5)
    readonly property color modelessDimColor: Qt.rgba(0, 0, 0, 0.12)
}
