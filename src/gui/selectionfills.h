#pragma once

#include <QColor>

// Fills of selected items (list rows, sidebar entries, thumbnails) as the
// accent at a fixed opacity over the surface under the item, so the text on
// them keeps its usual colour. Used by the Qt Quick Theme bridge.
namespace SelectionFills {

struct Colors {
  QColor tint;
  QColor tintHover;
  // Frame of a selected thumbnail.
  QColor frame;
};

inline constexpr qreal kTintAlpha = 0.40;
inline constexpr qreal kTintHoverAlpha = 0.50;
inline constexpr qreal kFrameAlpha = 0.70;

[[nodiscard]] inline QColor accentWithAlpha(const QColor &accent,
                                            qreal alpha) {
  QColor color = accent.toRgb();
  color.setAlphaF(alpha);
  return color;
}

[[nodiscard]] inline Colors colorsFor(const QColor &accent) {
  return {
      .tint = accentWithAlpha(accent, kTintAlpha),
      .tintHover = accentWithAlpha(accent, kTintHoverAlpha),
      .frame = accentWithAlpha(accent, kFrameAlpha),
  };
}

} // namespace SelectionFills
