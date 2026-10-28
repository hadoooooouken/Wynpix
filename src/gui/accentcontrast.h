#pragma once

#include <QColor>

#include <algorithm>
#include <cmath>

// Colour of text and glyphs drawn on a solid accent fill. The accent is the
// system colour or a user-chosen one, so the text colour cannot be fixed: it
// is picked from the accent so that the text stays readable on any of them.
//
// Contrast is the WCAG 2.x ratio of relative luminances. Text is white while
// white keeps the AA contrast for normal text, which is the Windows convention
// and what the UI used before; otherwise it is black. At least one of the two
// always reaches the AA contrast (the worst case, a fill of luminance ~0.18,
// gives about 4.6:1), so the result is readable on every opaque fill.
//
// Also the fills of the accent hovered and pressed, which move away from the
// text colour so that the text stays readable on them.
namespace AccentContrast {

// WCAG 2.x AA minimum contrast ratio for normal text.
inline constexpr qreal kMinTextContrast = 4.5;

inline constexpr Qt::GlobalColor kLightText = Qt::white;
inline constexpr Qt::GlobalColor kDarkText = Qt::black;

// sRGB to linear light conversion and luminance weights (WCAG 2.x).
inline constexpr qreal kSrgbLinearThreshold = 0.04045;
inline constexpr qreal kSrgbLinearDivisor = 12.92;
inline constexpr qreal kSrgbGammaOffset = 0.055;
inline constexpr qreal kSrgbGammaScale = 1.055;
inline constexpr qreal kSrgbGamma = 2.4;
inline constexpr qreal kLumaRed = 0.2126;
inline constexpr qreal kLumaGreen = 0.7152;
inline constexpr qreal kLumaBlue = 0.0722;
// Added to both luminances in the contrast ratio (ambient light flare).
inline constexpr qreal kContrastFlare = 0.05;

[[nodiscard]] inline qreal linearLight(qreal channel) {
  if (channel <= kSrgbLinearThreshold) {
    return channel / kSrgbLinearDivisor;
  }
  return std::pow((channel + kSrgbGammaOffset) / kSrgbGammaScale, kSrgbGamma);
}

// Relative luminance, 0 for black to 1 for white. The colour is taken as
// opaque: the alpha channel is ignored.
[[nodiscard]] inline qreal relativeLuminance(const QColor &color) {
  const QColor rgb = color.toRgb();
  return kLumaRed * linearLight(rgb.redF()) +
         kLumaGreen * linearLight(rgb.greenF()) +
         kLumaBlue * linearLight(rgb.blueF());
}

// Contrast ratio of two opaque colours, from 1 (identical) to 21.
[[nodiscard]] inline qreal contrastRatio(const QColor &first,
                                         const QColor &second) {
  const qreal a = relativeLuminance(first);
  const qreal b = relativeLuminance(second);
  return (std::max(a, b) + kContrastFlare) / (std::min(a, b) + kContrastFlare);
}

// Text colour for an opaque fill: white if it keeps the AA contrast on the
// fill, black otherwise. Reaches the AA contrast on any fill.
[[nodiscard]] inline QColor textOn(const QColor &fill) {
  const QColor light(kLightText);
  if (contrastRatio(light, fill) >= kMinTextContrast) {
    return light;
  }
  return QColor(kDarkText);
}

// Steps of the hovered and pressed fills from the accent, in percent
// (QColor::lighter and QColor::darker).
inline constexpr int kHoverStepPercent = 115;
inline constexpr int kPressedStepPercent = 130;

// The accent moved by a step away from the text colour: darker under white
// text, lighter under black text. The text keeps at least the contrast it has
// on the plain accent, unlike a hover that always lightens: white text on a
// lightened blue falls below 3:1.
[[nodiscard]] inline QColor stateFill(const QColor &accent,
                                      const QColor &text, int stepPercent) {
  if (text == QColor(kLightText)) {
    return accent.darker(stepPercent);
  }
  return accent.lighter(stepPercent);
}

} // namespace AccentContrast
