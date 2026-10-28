#pragma once

#include <QColor>

// Fills of neutral controls (buttons, tool buttons, step buttons) and the
// strokes (outlines, separators, grooves) as a translucent
// "ink" over whatever surface the control lies on: white on the
// dark scheme, black on the light one. One ladder from the rest fill to the
// hovered and the pressed one serves every surface, so a button on a card or
// on a panel needs no surface-specific colours. Used by the Qt Quick Theme
// bridge.
namespace NeutralFills {

struct Colors {
  QColor rest;
  QColor hover;
  QColor pressed;
  // Hovered and pressed fills on the HUD panels, which are dark in both
  // schemes.
  QColor hudHover;
  QColor hudPressed;
  // Outlines and separators, from the faintest to the strongest.
  QColor strokeSubtle;
  QColor strokeDefault;
  QColor strokeStrong;
};

// Opacities of the ink. In the dark scheme the pressed fill is the faintest,
// in the light scheme the strongest, as the opaque button colours were.
inline constexpr qreal kDarkRestAlpha = 0.09;
inline constexpr qreal kDarkHoverAlpha = 0.12;
inline constexpr qreal kDarkPressedAlpha = 0.07;
inline constexpr qreal kLightRestAlpha = 0.16;
inline constexpr qreal kLightHoverAlpha = 0.21;
inline constexpr qreal kLightPressedAlpha = 0.29;
inline constexpr qreal kHudHoverAlpha = 0.12;
inline constexpr qreal kHudPressedAlpha = 0.07;
inline constexpr qreal kDarkStrokeSubtleAlpha = 0.06;
inline constexpr qreal kDarkStrokeDefaultAlpha = 0.09;
inline constexpr qreal kDarkStrokeStrongAlpha = 0.27;
inline constexpr qreal kLightStrokeSubtleAlpha = 0.06;
inline constexpr qreal kLightStrokeDefaultAlpha = 0.24;
inline constexpr qreal kLightStrokeStrongAlpha = 0.24;

[[nodiscard]] inline QColor inkWithAlpha(Qt::GlobalColor ink, qreal alpha) {
  QColor color(ink);
  color.setAlphaF(alpha);
  return color;
}

[[nodiscard]] inline Colors colorsFor(bool dark) {
  const Qt::GlobalColor ink = dark ? Qt::white : Qt::black;
  return {
      .rest = inkWithAlpha(ink, dark ? kDarkRestAlpha : kLightRestAlpha),
      .hover = inkWithAlpha(ink, dark ? kDarkHoverAlpha : kLightHoverAlpha),
      .pressed =
          inkWithAlpha(ink, dark ? kDarkPressedAlpha : kLightPressedAlpha),
      .hudHover = inkWithAlpha(Qt::white, kHudHoverAlpha),
      .hudPressed = inkWithAlpha(Qt::white, kHudPressedAlpha),
      .strokeSubtle = inkWithAlpha(
          ink, dark ? kDarkStrokeSubtleAlpha : kLightStrokeSubtleAlpha),
      .strokeDefault = inkWithAlpha(
          ink, dark ? kDarkStrokeDefaultAlpha : kLightStrokeDefaultAlpha),
      .strokeStrong = inkWithAlpha(
          ink, dark ? kDarkStrokeStrongAlpha : kLightStrokeStrongAlpha),
  };
}

} // namespace NeutralFills
