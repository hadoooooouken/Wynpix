#include "themesnapshotbuilder.h"

#include "gui/semantictokens.h"
#include "gui/uimetrics.h"
#include "themestore.h"

namespace {

// Scheme entries; the colours derived from the accent and the neutral fills,
// strokes and dialog surfaces are the tokens'.
ThemeColors themeColors(const ColorScheme &scheme,
                        const SemanticTokens::Colors &tokens) {
  return {
      .background = scheme.background,
      .backgroundFullscreen = scheme.background_fullscreen,
      .text = scheme.text,
      .icons = scheme.icons,
      .folderIcons = scheme.folder_icons,
      .widget = scheme.widget,
      .accent = scheme.accent,
      .accentText = tokens.accentText,
      .accentHover = tokens.accentHover,
      .accentPressed = tokens.accentPressed,
      .accentTint = tokens.accentTint,
      .accentTintHover = tokens.accentTintHover,
      .accentFrame = tokens.accentFrame,
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
      .fillRest = tokens.fillRest,
      .fillHover = tokens.fillHover,
      .fillPressed = tokens.fillPressed,
      .hudHover = tokens.hudHover,
      .hudPressed = tokens.hudPressed,
      .strokeSubtle = tokens.strokeSubtle,
      .strokeDefault = tokens.strokeDefault,
      .strokeStrong = tokens.strokeStrong,
      .surfaceWindow = tokens.surfaceWindow,
      .surfaceCard = tokens.surfaceCard,
  };
}

ThemeTokens themeTokens(const SemanticTokens::Colors &colors) {
  return {
      .surfaceCanvas = colors.surfaceCanvas,
      .surfaceCanvasFullscreen = colors.surfaceCanvasFullscreen,
      .surfaceChrome = colors.surfaceChrome,
      .surfaceView = colors.surfaceView,
      .surfaceBar = colors.surfaceBar,
      .surfacePopup = colors.surfacePopup,
      .surfaceWindow = colors.surfaceWindow,
      .surfaceCard = colors.surfaceCard,
      .surfaceHud = colors.surfaceHud,
      .strokeSubtle = colors.strokeSubtle,
      .strokeDefault = colors.strokeDefault,
      .strokeStrong = colors.strokeStrong,
      .fillRest = colors.fillRest,
      .fillHover = colors.fillHover,
      .fillPressed = colors.fillPressed,
      .hudHover = colors.hudHover,
      .hudPressed = colors.hudPressed,
      .accent = colors.accent,
      .accentHover = colors.accentHover,
      .accentPressed = colors.accentPressed,
      .accentText = colors.accentText,
      .accentTint = colors.accentTint,
      .accentTintHover = colors.accentTintHover,
      .accentFrame = colors.accentFrame,
      .textPrimary = colors.textPrimary,
      .textSecondary = colors.textSecondary,
      .textOnHud = colors.textOnHud,
      .iconDefault = colors.iconDefault,
      .iconFolder = colors.iconFolder,
      .iconThumbFolder = colors.iconThumbFolder,
      .iconOnChrome = colors.iconOnChrome,
      .statusPending = colors.statusPending,
      .statusError = colors.statusError,
      .statusProcessing = colors.statusProcessing,
      .statusSuccess = colors.statusSuccess,
      .danger = colors.danger,
      .trash = colors.trash,
      .dropTarget = colors.dropTarget,
      .scrollbar = colors.scrollbar,
      .scrollbarHover = colors.scrollbarHover,
      .scrollbarMarker = colors.scrollbarMarker,
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
  const SemanticTokens::Colors tokens = SemanticTokens::colorsFor(scheme);
  return {
      .colors = themeColors(scheme, tokens),
      .tokens = themeTokens(tokens),
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
