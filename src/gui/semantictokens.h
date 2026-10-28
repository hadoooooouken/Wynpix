#pragma once

#include <QColor>

#include "gui/accentcontrast.h"
#include "gui/neutralfills.h"
#include "gui/selectionfills.h"
#include "themestore.h"

// Colours of the UI named by role rather than by the scheme entry they come
// from: surfaces, strokes, fills, the accent and its states, text, icons and
// states. Built from a colour scheme; the fills, strokes and accent states
// are derived (NeutralFills, SelectionFills, AccentContrast), the icons on
// the chrome follow its lightness, the window and card surfaces, the drop
// target and the scroll bar marker are fixed. Used by the Qt Quick Theme
// bridge.
namespace SemanticTokens {

struct Colors {
  // Surfaces, from the image canvas to the HUD panels.
  QColor surfaceCanvas;
  QColor surfaceCanvasFullscreen;
  QColor surfaceChrome;
  // The chrome one step further from the canvas (its border, glyphs on it).
  QColor surfaceChromeRaised;
  QColor surfaceView;
  // The view one and two steps further from the canvas: hovered and
  // alternate rows, borders, the backdrop of the rename prompt.
  QColor surfaceViewRaised;
  QColor surfaceViewRaisedStrong;
  QColor surfaceBar;
  QColor surfacePopup;
  QColor surfaceWindow;
  QColor surfaceCard;
  QColor surfaceHud;
  QColor strokeSubtle;
  QColor strokeDefault;
  QColor strokeStrong;
  // Outline of entry fields (text fields, combo boxes, spin boxes).
  QColor strokeField;
  QColor fillRest;
  QColor fillHover;
  QColor fillPressed;
  // Opaque fills of entry fields, stronger than the button fills.
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
  // Text from the highest contrast to the lowest.
  QColor textPrimary;
  QColor textSecondary;
  QColor textTertiary;
  // Black or white, whichever stands out on the view or the chrome.
  QColor textOnView;
  QColor textOnChrome;
  QColor textOnHud;
  QColor iconDefault;
  QColor iconFolder;
  QColor iconThumbFolder;
  // Glyphs on the chrome surface (the click-zone arrows of the viewer).
  QColor iconOnChrome;
  // textOnChrome at the opacity of the chrome: hovered thumbnails.
  QColor contrastOnChrome;
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
// Icons on the chrome, picked by its HSL lightness rather than by the
// scheme: the black background option makes the chrome black in the light
// scheme too.
inline constexpr QRgb kIconOnLightChrome = 0xff646464;
inline constexpr QRgb kIconOnDarkChrome = 0xffa4a4a4;
inline constexpr qreal kLightChromeLightness = 0.5;
inline constexpr QRgb kDropTarget = 0xffbe3c19;
// "gray" of CSS and QML, not Qt::gray.
inline constexpr QRgb kScrollbarMarker = 0xff808080;
inline constexpr qreal kScrollbarMarkerAlpha = 0.30;

[[nodiscard]] inline Colors colorsFor(const ColorScheme &scheme) {
  const bool dark = scheme.tid == COLORS_DARK;
  const NeutralFills::Colors fills = NeutralFills::colorsFor(dark);
  const SelectionFills::Colors selection =
      SelectionFills::colorsFor(scheme.accent);
  const QColor accentText = AccentContrast::textOn(scheme.accent);
  QColor scrollbarMarker = QColor::fromRgb(kScrollbarMarker);
  scrollbarMarker.setAlphaF(kScrollbarMarkerAlpha);
  return {
      .surfaceCanvas = scheme.background,
      .surfaceCanvasFullscreen = scheme.background_fullscreen,
      .surfaceChrome = scheme.thumbpanel,
      .surfaceChromeRaised = scheme.thumbpanel_hc2,
      .surfaceView = scheme.folderview,
      .surfaceViewRaised = scheme.folderview_hc,
      .surfaceViewRaisedStrong = scheme.folderview_hc2,
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
      .strokeField = scheme.combo_field_border,
      .fillRest = fills.rest,
      .fillHover = fills.hover,
      .fillPressed = fills.pressed,
      .fieldRest = scheme.combo_field,
      .fieldHover = scheme.combo_field_hover,
      .fieldPressed = scheme.combo_field_pressed,
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
      .textPrimary = scheme.text_hc,
      .textSecondary = scheme.text,
      .textTertiary = scheme.text_lc,
      .textOnView = scheme.text_hc2,
      .textOnChrome = scheme.thumbpanel_text,
      .textOnHud = scheme.overlay_text,
      .iconDefault = scheme.icons,
      .iconFolder = scheme.folder_icons,
      .iconThumbFolder = scheme.thumbnail_folder_icons,
      .iconOnChrome = QColor::fromRgb(
          scheme.thumbpanel.lightnessF() > kLightChromeLightness
              ? kIconOnLightChrome
              : kIconOnDarkChrome),
      .contrastOnChrome = scheme.thumbpanel_hc,
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
