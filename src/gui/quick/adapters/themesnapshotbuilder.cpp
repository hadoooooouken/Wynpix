#include "themesnapshotbuilder.h"

#include "gui/accentcontrast.h"
#include "gui/neutralfills.h"
#include "gui/selectionfills.h"
#include "gui/uimetrics.h"
#include "themestore.h"

namespace {

// Window of every dialog (decision D3: the settings window colour) and the
// cards and sidebar on it, 6 HSV values lighter in the dark scheme and
// darker in the light one.
constexpr QRgb kDarkSurfaceWindow = 0xff252525;
constexpr QRgb kDarkSurfaceCard = 0xff2b2b2b;
constexpr QRgb kLightSurfaceWindow = 0xfff5f5f5;
constexpr QRgb kLightSurfaceCard = 0xffefefef;

ThemeColors themeColors(const ColorScheme &scheme) {
  const bool dark = scheme.tid == COLORS_DARK;
  const QColor accentText = AccentContrast::textOn(scheme.accent);
  const SelectionFills::Colors selection =
      SelectionFills::colorsFor(scheme.accent);
  const NeutralFills::Colors fills = NeutralFills::colorsFor(dark);
  return {
      .background = scheme.background,
      .backgroundFullscreen = scheme.background_fullscreen,
      .text = scheme.text,
      .icons = scheme.icons,
      .folderIcons = scheme.folder_icons,
      .widget = scheme.widget,
      .accent = scheme.accent,
      .accentText = accentText,
      .accentHover = AccentContrast::stateFill(
          scheme.accent, accentText, AccentContrast::kHoverStepPercent),
      .accentPressed = AccentContrast::stateFill(
          scheme.accent, accentText, AccentContrast::kPressedStepPercent),
      .accentTint = selection.tint,
      .accentTintHover = selection.tintHover,
      .accentFrame = selection.frame,
      .folderView = scheme.folderview,
      .folderViewTopBar = scheme.folderview_topbar,
      .thumbPanel = scheme.thumbpanel,
      .scrollbar = scheme.scrollbar,
      .scrollbarHover = scheme.scrollbar_hover,
      .overlayText = scheme.overlay_text,
      .overlay = scheme.overlay,
      .statusPending = scheme.status_pending,
      .statusError = scheme.status_error,
      .statusProcessing = scheme.status_processing,
      .statusSuccess = scheme.status_success,
      .danger = scheme.danger,
      .trash = scheme.trash,
      .textHc2 = scheme.text_hc2,
      .textHc = scheme.text_hc,
      .textLc = scheme.text_lc,
      .folderViewHc = scheme.folderview_hc,
      .folderViewHc2 = scheme.folderview_hc2,
      .thumbPanelHc = scheme.thumbpanel_hc,
      .thumbPanelHc2 = scheme.thumbpanel_hc2,
      .thumbPanelText = scheme.thumbpanel_text,
      .comboField = scheme.combo_field,
      .comboFieldHover = scheme.combo_field_hover,
      .comboFieldPressed = scheme.combo_field_pressed,
      .comboFieldBorder = scheme.combo_field_border,
      .fillRest = fills.rest,
      .fillHover = fills.hover,
      .fillPressed = fills.pressed,
      .hudHover = fills.hudHover,
      .hudPressed = fills.hudPressed,
      .strokeSubtle = fills.strokeSubtle,
      .strokeDefault = fills.strokeDefault,
      .strokeStrong = fills.strokeStrong,
      .surfaceWindow =
          QColor::fromRgb(dark ? kDarkSurfaceWindow : kLightSurfaceWindow),
      .surfaceCard =
          QColor::fromRgb(dark ? kDarkSurfaceCard : kLightSurfaceCard),
  };
}

ThemeFonts themeFonts(const UiMetrics::Typography &typography) {
  return {
      .caption = typography.caption,
      .body = typography.body,
      .bodyStrong = typography.bodyStrong,
      .title = typography.title,
  };
}

} // namespace

//------------------------------------------------------------------------------
ThemeSnapshot buildThemeSnapshot(const ColorScheme &scheme,
                                 const QFont &systemFont,
                                 const QString &iconFontFamily) {
  const UiMetrics::Typography typography =
      UiMetrics::typographyFor(systemFont);
  const UiMetrics::ControlMetrics metrics =
      UiMetrics::controlMetricsFor(typography.body);
  return {
      .colors = themeColors(scheme),
      .fonts = themeFonts(typography),
      .metrics =
          {
              .buttonHeight = metrics.buttonHeight,
              .topPanelHeight = metrics.topPanelHeight,
              .overlayHeaderSize = metrics.overlayHeaderSize,
              .contextMenuWidth = metrics.contextMenuWidth,
              .contextMenuItemHeight = metrics.contextMenuItemHeight,
              .renameOverlayWidth = metrics.renameOverlayWidth,
              .tooltipBorderWidth = metrics.tooltipBorderWidth,
              .tooltipBorderRadius = metrics.tooltipBorderRadius,
              .contextMenuBorderRadius = metrics.contextMenuBorderRadius,
          },
      .dark = scheme.tid == COLORS_DARK,
      .iconFontFamily = iconFontFamily,
  };
}
