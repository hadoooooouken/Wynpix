#pragma once

#include <QFont>
#include <QFontMetrics>
#include <QLatin1StringView>
#include <QString>
#include <QtGlobal>

namespace UiMetrics {

inline constexpr int kCompactIconSizePx = 16;
inline constexpr int kStandardIconSizePx = 20;

// Text styles of the UI, named after the Windows 11 (Fluent) type ramp
// styles they stand for, at desktop sizes: body is the Windows system font
// size (9 pt, 12 px) that Windows 10 and the Win32 surfaces of Windows 11
// use; the Fluent 14 px body read too large and thin at 100 % scaling
// (docs/TYPOGRAPHY_PLAN.md). Body is the application font. Used by the
// Qt Quick Theme bridge.
struct Typography {
  QFont caption;    // notes, hints, secondary info lines
  QFont body;       // default text and controls
  QFont bodyStrong; // emphasis, item names, section and group titles
  QFont title;      // page titles, zoom indicator
};

// One style of the ramp: the Segoe UI Variable optical size the ramp
// assigns to it, its size in effective pixels at the default Windows text
// size, and its weight (Regular for most text, Semibold for emphasis).
struct TextStyle {
  QLatin1StringView family;
  qreal sizePx = 0;
  QFont::Weight weight = QFont::Normal;
};

// Segoe UI Variable ships with Windows 11; Windows 10 lacks it, and the
// styles fall back to the family of the system font (Segoe UI).
inline constexpr QLatin1StringView kSmallFontFamily("Segoe UI Variable Small");
inline constexpr QLatin1StringView kTextFontFamily("Segoe UI Variable Text");
inline constexpr QLatin1StringView kDisplayFontFamily(
    "Segoe UI Variable Display");
inline constexpr QFont::Weight kRegularFontWeight = QFont::Normal;
inline constexpr QFont::Weight kEmphasisFontWeight = QFont::DemiBold;
inline constexpr TextStyle kCaptionStyle{kSmallFontFamily, 11.0,
                                         kRegularFontWeight};
inline constexpr TextStyle kBodyStyle{kTextFontFamily, 12.0,
                                      kRegularFontWeight};
inline constexpr TextStyle kBodyStrongStyle{kTextFontFamily, 12.0,
                                            kEmphasisFontWeight};
inline constexpr TextStyle kTitleStyle{kDisplayFontFamily, 24.0,
                                       kEmphasisFontWeight};

// Point size of the Windows system font at the default text size. A larger
// system font (the Windows text size setting) scales the ramp up; a smaller
// one keeps the ramp's sizes, below which the text gets hard to read.
inline constexpr qreal kDefaultSystemFontPointSize = 9.0;
inline constexpr qreal kMinTextScale = 1.0;
// Qt converts points to device-independent pixels at the Windows logical
// DPI; display scaling is applied on top of it separately.
inline constexpr qreal kPointsPerInch = 72.0;
inline constexpr qreal kLogicalDotsPerInch = 96.0;

// Point size of the system font, also when it was set in pixels.
[[nodiscard]] inline qreal systemFontPointSize(const QFont &systemFont) {
  if (systemFont.pointSizeF() > 0) {
    return systemFont.pointSizeF();
  }
  if (systemFont.pixelSize() > 0) {
    return systemFont.pixelSize() * kPointsPerInch / kLogicalDotsPerInch;
  }
  return kDefaultSystemFontPointSize;
}

[[nodiscard]] inline qreal textScaleFor(const QFont &systemFont) {
  return qMax(systemFontPointSize(systemFont) / kDefaultSystemFontPointSize,
              kMinTextScale);
}

[[nodiscard]] inline QFont fontFor(const QFont &systemFont, qreal textScale,
                                   const TextStyle &style) {
  QFont font = systemFont;
  font.setFamilies({QString(style.family), systemFont.family()});
  font.setPointSizeF(style.sizePx * textScale * kPointsPerInch /
                     kLogicalDotsPerInch);
  font.setWeight(style.weight);
  return font;
}

[[nodiscard]] inline Typography typographyFor(const QFont &systemFont) {
  const qreal textScale = textScaleFor(systemFont);
  return {
      .caption = fontFor(systemFont, textScale, kCaptionStyle),
      .body = fontFor(systemFont, textScale, kBodyStyle),
      .bodyStrong = fontFor(systemFont, textScale, kBodyStrongStyle),
      .title = fontFor(systemFont, textScale, kTitleStyle),
  };
}

// Control sizes derived from the height of a text line in the application
// font, plus the fixed frame sizes of tooltips and context menus. Used by
// the Qt Quick Theme bridge, like Typography.
struct ControlMetrics {
  int buttonHeight = 0;
  int topPanelHeight = 0;
  int overlayHeaderSize = 0;
  int contextMenuWidth = 0;
  int contextMenuItemHeight = 0;
  int renameOverlayWidth = 0;
  int tooltipBorderWidth = 0;
  int tooltipBorderRadius = 0;
  int contextMenuBorderRadius = 0;
};

// Vertical text padding as a fraction of the text line height.
inline constexpr float kTextPaddingScale = 0.10f;
inline constexpr float kLargeTextPaddingScale = 0.25f;
// Folder view top bar: vertical margin of its items, their minimum text
// padding and the minimum bar height.
inline constexpr int kTopPanelVerticalMarginPx = 4;
inline constexpr int kMinTopPanelTextPaddingPx = 4;
inline constexpr int kMinTopPanelHeightPx = 38;
inline constexpr int kMinOverlayHeaderSizePx = 30;
// Widths and heights designed for this text line height; larger fonts scale
// them up, smaller ones keep the designed size.
inline constexpr int kReferenceTextHeightPx = 22;
inline constexpr int kContextMenuWidthPx = 212;
inline constexpr int kContextMenuItemHeightPx = 32;
inline constexpr int kRenameOverlayWidthPx = 380;
inline constexpr int kTooltipBorderWidthPx = 1;
inline constexpr int kTooltipBorderRadiusPx = 6;
inline constexpr int kContextMenuBorderRadiusPx = 8;

[[nodiscard]] inline ControlMetrics controlMetricsFor(const QFont &baseFont) {
  const int textHeight = QFontMetrics(baseFont).height();
  const int textPadding = static_cast<int>(textHeight * kTextPaddingScale);
  const int largeTextPadding =
      static_cast<int>(textHeight * kLargeTextPaddingScale);
  const int topPanelTextPadding = qMax(textPadding, kMinTopPanelTextPaddingPx);
  const qreal referenceScale =
      qMax(static_cast<qreal>(textHeight) / kReferenceTextHeightPx, 1.0);
  return {
      .buttonHeight = textHeight + largeTextPadding * 2,
      .topPanelHeight = qMax(textHeight + topPanelTextPadding * 2 +
                                 kTopPanelVerticalMarginPx * 2,
                             kMinTopPanelHeightPx),
      .overlayHeaderSize =
          qMax(textHeight + textPadding * 2, kMinOverlayHeaderSizePx),
      .contextMenuWidth = static_cast<int>(kContextMenuWidthPx * referenceScale),
      .contextMenuItemHeight =
          static_cast<int>(kContextMenuItemHeightPx * referenceScale),
      .renameOverlayWidth =
          static_cast<int>(kRenameOverlayWidthPx * referenceScale),
      .tooltipBorderWidth = kTooltipBorderWidthPx,
      .tooltipBorderRadius = kTooltipBorderRadiusPx,
      .contextMenuBorderRadius = kContextMenuBorderRadiusPx,
  };
}

} // namespace UiMetrics
