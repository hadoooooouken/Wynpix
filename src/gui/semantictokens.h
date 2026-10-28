#pragma once

#include <QColor>

#include "gui/accentcontrast.h"
#include "gui/neutralfills.h"
#include "gui/selectionfills.h"
#include "themestore.h"

// Colours of the UI named by role rather than by the scheme entry they come
// from: surfaces, strokes, fills, the accent and its states, text, icons and
// states. Built from a colour scheme; the fills, strokes and accent states
// are derived (NeutralFills, SelectionFills, AccentContrast), the window and
// card surfaces, the drop target and the scroll bar marker are fixed. Used by the Qt Quick Theme
// bridge.
namespace SemanticTokens {

struct Colors {
  // Surfaces, from the image canvas to the HUD panels.
  QColor surfaceCanvas;
  QColor surfaceCanvasFullscreen;
  QColor surfaceChrome;
  QColor surfaceView;
  QColor surfaceBar;
  QColor surfacePopup;
  QColor surfaceWindow;
  QColor surfaceCard;
  QColor surfaceHud;
  QColor strokeSubtle;
  QColor strokeDefault;
  QColor strokeStrong;
  QColor fillRest;
  QColor fillHover;
  QColor fillPressed;
  QColor fillSubtle;
  QColor fieldRest;
  QColor fieldHover;
  QColor fieldPressed;
  QColor hudHover;
  QColor hudPressed;
  QColor accent;
  QColor accentHover;
  QColor accentPressed;
  QColor accentText;
  QColor accentTint;
  QColor accentTintHover;
  QColor accentFrame;
  // Keyboard focus ring; on a solid accent fill it is accentText.
  QColor focusRing;
  QColor textPrimary;
  QColor textSecondary;
  QColor textOnHud;
  QColor iconDefault;
  QColor iconFolder;
  QColor iconThumbFolder;
  QColor statusPending;
  QColor statusError;
  QColor statusProcessing;
  QColor statusSuccess;
  QColor danger;
  QColor trash;
  // Thumbnail under a drag that can be dropped on it.
  QColor dropTarget;
  QColor scrollbar;
  QColor scrollbarHover;
  // Mark of the current item on the thumbnail strip scroll bar.
  QColor scrollbarMarker;
};

// Window of every dialog (decision D3: the settings window colour) and the
// cards and sidebar on it, 6 HSV values lighter in the dark scheme and
// darker in the light one.
inline constexpr QRgb kDarkSurfaceWindow = 0xff252525;
inline constexpr QRgb kDarkSurfaceCard = 0xff2b2b2b;
inline constexpr QRgb kLightSurfaceWindow = 0xfff5f5f5;
inline constexpr QRgb kLightSurfaceCard = 0xffefefef;
inline constexpr QRgb kDropTarget = 0xffbe3c19;
// "gray" of CSS and QML, not Qt::gray.
inline constexpr QRgb kScrollbarMarker = 0xff808080;
inline constexpr qreal kScrollbarMarkerAlpha = 0.30;
// Opacity of secondary text (placeholders, shortcuts, notes) over the text
// colour.
inline constexpr qreal kSecondaryTextAlpha = 0.62;

[[nodiscard]] inline Colors colorsFor(const ColorScheme &scheme) {
  const bool dark = scheme.tid == COLORS_DARK;
  const NeutralFills::Colors fills = NeutralFills::colorsFor(dark);
  const SelectionFills::Colors selection =
      SelectionFills::colorsFor(scheme.accent);
  const QColor accentText = AccentContrast::textOn(scheme.accent);
  QColor textSecondary = scheme.text.toRgb();
  textSecondary.setAlphaF(kSecondaryTextAlpha);
  QColor scrollbarMarker = QColor::fromRgb(kScrollbarMarker);
  scrollbarMarker.setAlphaF(kScrollbarMarkerAlpha);
  return {
      .surfaceCanvas = scheme.background,
      .surfaceCanvasFullscreen = scheme.background_fullscreen,
      .surfaceChrome = scheme.thumbpanel,
      .surfaceView = scheme.folderview,
      .surfaceBar = scheme.folderview_topbar,
      .surfacePopup = scheme.widget,
      .surfaceWindow =
          QColor::fromRgb(dark ? kDarkSurfaceWindow : kLightSurfaceWindow),
      .surfaceCard =
          QColor::fromRgb(dark ? kDarkSurfaceCard : kLightSurfaceCard),
      .surfaceHud = scheme.overlay,
      .strokeSubtle = fills.strokeSubtle,
      .strokeDefault = fills.strokeDefault,
      .strokeStrong = fills.strokeStrong,
      .fillRest = fills.rest,
      .fillHover = fills.hover,
      .fillPressed = fills.pressed,
      .fillSubtle = fills.subtle,
      .fieldRest = fills.fieldRest,
      .fieldHover = fills.fieldHover,
      .fieldPressed = fills.fieldPressed,
      .hudHover = fills.hudHover,
      .hudPressed = fills.hudPressed,
      .accent = scheme.accent,
      .accentHover = AccentContrast::stateFill(
          scheme.accent, accentText, AccentContrast::kHoverStepPercent),
      .accentPressed = AccentContrast::stateFill(
          scheme.accent, accentText, AccentContrast::kPressedStepPercent),
      .accentText = accentText,
      .accentTint = selection.tint,
      .accentTintHover = selection.tintHover,
      .accentFrame = selection.frame,
      .focusRing = scheme.accent,
      .textPrimary = scheme.text_hc,
      .textSecondary = textSecondary,
      .textOnHud = scheme.overlay_text,
      .iconDefault = scheme.icons,
      .iconFolder = scheme.folder_icons,
      .iconThumbFolder = scheme.thumbnail_folder_icons,
      .statusPending = scheme.status_pending,
      .statusError = scheme.status_error,
      .statusProcessing = scheme.status_processing,
      .statusSuccess = scheme.status_success,
      .danger = scheme.danger,
      .trash = scheme.trash,
      .dropTarget = QColor::fromRgb(kDropTarget),
      .scrollbar = scheme.scrollbar,
      .scrollbarHover = scheme.scrollbar_hover,
      .scrollbarMarker = scrollbarMarker,
  };
}

} // namespace SemanticTokens
