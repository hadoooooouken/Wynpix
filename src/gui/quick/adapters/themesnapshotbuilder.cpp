#include "themesnapshotbuilder.h"

#include "gui/semantictokens.h"
#include "gui/uimetrics.h"
#include "themestore.h"

namespace {

ThemeTokens themeTokens(const SemanticTokens::Colors &colors) {
  return {
      .surfaceCanvas = colors.surfaceCanvas,
      .surfaceCanvasFullscreen = colors.surfaceCanvasFullscreen,
      .surfaceChrome = colors.surfaceChrome,
      .surfaceChromeRaised = colors.surfaceChromeRaised,
      .surfaceView = colors.surfaceView,
      .surfaceViewRaised = colors.surfaceViewRaised,
      .surfaceViewRaisedStrong = colors.surfaceViewRaisedStrong,
      .surfaceBar = colors.surfaceBar,
      .surfacePopup = colors.surfacePopup,
      .surfaceWindow = colors.surfaceWindow,
      .surfaceCard = colors.surfaceCard,
      .surfaceHud = colors.surfaceHud,
      .strokeSubtle = colors.strokeSubtle,
      .strokeDefault = colors.strokeDefault,
      .strokeStrong = colors.strokeStrong,
      .strokeField = colors.strokeField,
      .fillRest = colors.fillRest,
      .fillHover = colors.fillHover,
      .fillPressed = colors.fillPressed,
      .fieldRest = colors.fieldRest,
      .fieldHover = colors.fieldHover,
      .fieldPressed = colors.fieldPressed,
      .hudHover = colors.hudHover,
      .hudPressed = colors.hudPressed,
      .rowHover = colors.rowHover,
      .rowSelected = colors.rowSelected,
      .rowSelectedHover = colors.rowSelectedHover,
      .rowPressed = colors.rowPressed,
      .accent = colors.accent,
      .accentHover = colors.accentHover,
      .accentPressed = colors.accentPressed,
      .accentText = colors.accentText,
      .accentTint = colors.accentTint,
      .accentTintHover = colors.accentTintHover,
      .accentFrame = colors.accentFrame,
      .textPrimary = colors.textPrimary,
      .textSecondary = colors.textSecondary,
      .textTertiary = colors.textTertiary,
      .textOnView = colors.textOnView,
      .textOnChrome = colors.textOnChrome,
      .textOnHud = colors.textOnHud,
      .iconDefault = colors.iconDefault,
      .iconFolder = colors.iconFolder,
      .iconOnChrome = colors.iconOnChrome,
      .contrastOnChrome = colors.contrastOnChrome,
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
      .iconFontFamily = iconFontFamily,
  };
}
