#include <QFont>
#include <QFontMetrics>
#include <QTest>

#include "gui/accentcontrast.h"
#include "gui/neutralfills.h"
#include "gui/selectionfills.h"
#include "gui/quick/adapters/themesnapshotbuilder.h"
#include "gui/uimetrics.h"
#include "testsuites.h"
#include "themestore.h"

namespace {
using namespace Qt::StringLiterals;

// Fonts whose text line is shorter and much taller than the reference
// height the context menu sizes were designed for. Point sizes, like the
// application font (the text styles derive point sizes from it).
constexpr int kSmallFontPointSize = 6;
constexpr int kLargeFontPointSize = 32;
// System fonts for the text styles: the Windows message font at the default
// text size, one enlarged by the Windows text size setting (4/3 of the
// default), and that enlarged font given in pixels.
constexpr int kDefaultFontPointSize = 9;
constexpr int kEnlargedFontPointSize = 12;
constexpr int kEnlargedFontPixelSize = 16;
constexpr qreal kDefaultTextScale = 1.0;
constexpr qreal kEnlargedTextScale =
    static_cast<qreal>(kEnlargedFontPointSize) / kDefaultFontPointSize;

QFont fontWithPointSize(int pointSize) {
  QFont font;
  font.setPointSize(pointSize);
  return font;
}

// Point size of a type ramp style at a text scale.
qreal pointSizeOf(const UiMetrics::TextStyle &style, qreal textScale) {
  return style.sizePx * textScale * UiMetrics::kPointsPerInch /
         UiMetrics::kLogicalDotsPerInch;
}

void verifyStyle(const QFont &font, const UiMetrics::TextStyle &style,
                 qreal textScale, const QFont &systemFont) {
  QCOMPARE(font.pointSizeF(), pointSizeOf(style, textScale));
  QCOMPARE(font.weight(), style.weight);
  QCOMPARE(font.families(),
           QStringList({QString(style.family), systemFont.family()}));
}

QColor grey(int value) { return QColor::fromRgb(value, value, value); }

// Accent colours the text colour is checked on: the Windows accent palette,
// and extremes that break fixed text colours (a nearly black and a nearly
// white accent, mid grey, and the dark dialog window colour).
const QStringList kAccentSamples = {
    u"#0078d4"_s, u"#107c10"_s, u"#e81123"_s, u"#8764b8"_s,
    u"#00b7c3"_s, u"#ff8c00"_s, u"#ffb900"_s, u"#4cc2ff"_s,
    u"#1a1a1a"_s, u"#808080"_s, u"#f0f0f0"_s, u"#252525"_s,
};
// Steps of the sweeps over the grey ramp and over the colour wheel.
constexpr int kGreyStep = 1;
constexpr int kHueStep = 15;
constexpr int kSaturationStep = 51;
constexpr int kValueStep = 17;
constexpr int kMaxChannel = 255;
// Contrast ratio of black on white.
constexpr qreal kMaxContrast = 21.0;
constexpr int kHueCount = 360;
// Luminance at which white and black have the same contrast (about 0.179).
constexpr qreal kEqualContrastLuminance = 0.179;
// White is kept up to the luminance where it still has the AA contrast
// (about 0.183).
constexpr qreal kWhiteMaxLuminance = 0.184;
} // namespace

// The control metrics and colours of the Qt Quick Theme bridge, and the
// theme snapshot built from them.
class UiMetricsTests : public QObject {
  Q_OBJECT

private slots:
  void dialogSurfacesKeepTheSettingsWindowColours_data() {
    QTest::addColumn<int>("scheme");
    QTest::addColumn<int>("window");
    QTest::addColumn<int>("card");
    // The values the stylesheet used for the settings window and its
    // groups: cards are lighter on the dark window, darker on the light one.
    QTest::newRow("dark") << static_cast<int>(COLORS_DARK) << 37 << 43;
    QTest::newRow("light") << static_cast<int>(COLORS_LIGHT) << 245 << 239;
  }

  void dialogSurfacesKeepTheSettingsWindowColours() {
    QFETCH(int, scheme);
    QFETCH(int, window);
    QFETCH(int, card);
    const ThemeSnapshot snapshot = buildThemeSnapshot(
        ThemeStore::colorScheme(static_cast<ColorSchemes>(scheme)),
        fontWithPointSize(kDefaultFontPointSize), QString());

    QCOMPARE(snapshot.colors.surfaceWindow, grey(window));
    QCOMPARE(snapshot.colors.surfaceCard, grey(card));
  }

  void defaultSystemFontGivesTheDesktopRamp() {
    const QFont font = fontWithPointSize(kDefaultFontPointSize);
    const UiMetrics::Typography typography = UiMetrics::typographyFor(font);

    // 11, 12 and 24 px at 96 DPI; body is the system font's size.
    QCOMPARE(typography.caption.pointSizeF(), 8.25);
    QCOMPARE(typography.body.pointSizeF(), font.pointSizeF());
    QCOMPARE(typography.title.pointSizeF(), 18.0);
    QCOMPARE(UiMetrics::textScaleFor(font), kDefaultTextScale);
    verifyStyle(typography.caption, UiMetrics::kCaptionStyle,
                kDefaultTextScale, font);
    verifyStyle(typography.body, UiMetrics::kBodyStyle, kDefaultTextScale,
                font);
    verifyStyle(typography.bodyStrong, UiMetrics::kBodyStrongStyle,
                kDefaultTextScale, font);
    verifyStyle(typography.title, UiMetrics::kTitleStyle, kDefaultTextScale,
                font);
  }

  void enlargedSystemFontScalesTheRamp() {
    const QFont font = fontWithPointSize(kEnlargedFontPointSize);
    const UiMetrics::Typography typography = UiMetrics::typographyFor(font);

    QCOMPARE(UiMetrics::textScaleFor(font), kEnlargedTextScale);
    verifyStyle(typography.caption, UiMetrics::kCaptionStyle,
                kEnlargedTextScale, font);
    verifyStyle(typography.body, UiMetrics::kBodyStyle, kEnlargedTextScale,
                font);
    verifyStyle(typography.title, UiMetrics::kTitleStyle, kEnlargedTextScale,
                font);
  }

  void smallerSystemFontKeepsTheRampSizes() {
    const QFont font = fontWithPointSize(kSmallFontPointSize);
    const UiMetrics::Typography typography = UiMetrics::typographyFor(font);

    QCOMPARE(UiMetrics::textScaleFor(font), UiMetrics::kMinTextScale);
    verifyStyle(typography.caption, UiMetrics::kCaptionStyle,
                UiMetrics::kMinTextScale, font);
    verifyStyle(typography.body, UiMetrics::kBodyStyle,
                UiMetrics::kMinTextScale, font);
  }

  void pixelSizedSystemFontScalesLikeItsPointSize() {
    QFont font;
    font.setPixelSize(kEnlargedFontPixelSize);
    const UiMetrics::Typography typography = UiMetrics::typographyFor(font);

    QCOMPARE(UiMetrics::systemFontPointSize(font),
             static_cast<qreal>(kEnlargedFontPointSize));
    QCOMPARE(UiMetrics::textScaleFor(font), kEnlargedTextScale);
    QCOMPARE(typography.body.pointSizeF(),
             pointSizeOf(UiMetrics::kBodyStyle, kEnlargedTextScale));
    QCOMPARE(typography.body.pixelSize(), -1);
  }

  void smallFontsKeepTheDesignedSizes() {
    const UiMetrics::ControlMetrics metrics =
        UiMetrics::controlMetricsFor(fontWithPointSize(kSmallFontPointSize));
    QCOMPARE(metrics.contextMenuWidth, UiMetrics::kContextMenuWidthPx);
    QCOMPARE(metrics.contextMenuItemHeight, UiMetrics::kContextMenuItemHeightPx);
    QCOMPARE(metrics.renameOverlayWidth, UiMetrics::kRenameOverlayWidthPx);
    QCOMPARE(metrics.topPanelHeight, UiMetrics::kMinTopPanelHeightPx);
    QCOMPARE(metrics.overlayHeaderSize, UiMetrics::kMinOverlayHeaderSizePx);
  }

  void largeFontsScaleTheSizes() {
    const QFont font = fontWithPointSize(kLargeFontPointSize);
    const int textHeight = QFontMetrics(font).height();
    QVERIFY(textHeight > UiMetrics::kReferenceTextHeightPx);
    const UiMetrics::ControlMetrics metrics = UiMetrics::controlMetricsFor(font);

    QCOMPARE(metrics.buttonHeight,
             textHeight + 2 * static_cast<int>(textHeight * 0.25f));
    const qreal scale =
        static_cast<qreal>(textHeight) / UiMetrics::kReferenceTextHeightPx;
    QCOMPARE(metrics.contextMenuWidth, static_cast<int>(212 * scale));
    QCOMPARE(metrics.contextMenuItemHeight, static_cast<int>(32 * scale));
    QCOMPARE(metrics.renameOverlayWidth, static_cast<int>(380 * scale));
    QVERIFY(metrics.topPanelHeight > UiMetrics::kMinTopPanelHeightPx);
    QVERIFY(metrics.overlayHeaderSize > UiMetrics::kMinOverlayHeaderSizePx);
  }

  void frameSizesAreFixed() {
    for (const int pointSize : {kSmallFontPointSize, kLargeFontPointSize}) {
      const UiMetrics::ControlMetrics metrics =
          UiMetrics::controlMetricsFor(fontWithPointSize(pointSize));
      QCOMPARE(metrics.tooltipBorderWidth, 1);
      QCOMPARE(metrics.tooltipBorderRadius, 6);
      QCOMPARE(metrics.contextMenuBorderRadius, 8);
    }
  }

  void contrastOfBlackAndWhiteIsTheMaximum() {
    QCOMPARE(AccentContrast::contrastRatio(Qt::black, Qt::white),
             kMaxContrast);
    QCOMPARE(AccentContrast::contrastRatio(Qt::white, Qt::white), 1.0);
    // The ratio does not depend on the order of the colours.
    QCOMPARE(AccentContrast::contrastRatio(grey(40), grey(200)),
             AccentContrast::contrastRatio(grey(200), grey(40)));
  }

  void accentTextKeepsTheAaContrastOnEveryAccentSample_data() {
    QTest::addColumn<QString>("accent");
    for (const QString &name : kAccentSamples) {
      QTest::newRow(qPrintable(name)) << name;
    }
  }

  void accentTextKeepsTheAaContrastOnEveryAccentSample() {
    QFETCH(QString, accent);
    const QColor fill(accent);
    const QColor text = AccentContrast::textOn(fill);
    QVERIFY(text == QColor(Qt::white) || text == QColor(Qt::black));
    QCOMPARE_GE(AccentContrast::contrastRatio(text, fill),
                AccentContrast::kMinTextContrast);
  }

  void accentTextKeepsTheAaContrastOnEveryGrey() {
    for (int value = 0; value <= kMaxChannel; value += kGreyStep) {
      const QColor fill = grey(value);
      QVERIFY2(AccentContrast::contrastRatio(AccentContrast::textOn(fill),
                                             fill) >=
                   AccentContrast::kMinTextContrast,
               qPrintable(fill.name()));
    }
  }

  void accentTextKeepsTheAaContrastOnTheColourWheel() {
    for (int hue = 0; hue < kHueCount; hue += kHueStep) {
      for (int saturation = 0; saturation <= kMaxChannel;
           saturation += kSaturationStep) {
        for (int value = 0; value <= kMaxChannel; value += kValueStep) {
          const QColor fill = QColor::fromHsv(hue, saturation, value);
          QVERIFY2(AccentContrast::contrastRatio(AccentContrast::textOn(fill),
                                                 fill) >=
                       AccentContrast::kMinTextContrast,
                   qPrintable(fill.name()));
        }
      }
    }
  }

  void accentTextPrefersWhiteWhileItIsReadable() {
    // The Windows blue: black has the slightly higher contrast, but white
    // still passes AA and is what the UI shows on it.
    QCOMPARE(AccentContrast::textOn(QColor(u"#0078d4"_s)), QColor(Qt::white));
    // A light accent needs black.
    QCOMPARE(AccentContrast::textOn(QColor(u"#ffb900"_s)), QColor(Qt::black));
    // Both extremes of the grey ramp.
    QCOMPARE(AccentContrast::textOn(QColor(Qt::black)), QColor(Qt::white));
    QCOMPARE(AccentContrast::textOn(QColor(Qt::white)), QColor(Qt::black));
  }

  void accentTextSwitchesNearTheEqualContrastLuminance() {
    for (int value = 0; value <= kMaxChannel; value += kGreyStep) {
      const QColor fill = grey(value);
      const qreal luminance = AccentContrast::relativeLuminance(fill);
      if (AccentContrast::textOn(fill) == QColor(Qt::white)) {
        QCOMPARE_LE(luminance, kWhiteMaxLuminance);
      } else {
        QCOMPARE_GT(luminance, kEqualContrastLuminance);
      }
    }
  }

  void snapshotCarriesTheAccentText() {
    const QFont font = fontWithPointSize(kDefaultFontPointSize);
    for (const ColorSchemes scheme : {COLORS_DARK, COLORS_LIGHT}) {
      ColorScheme colors = ThemeStore::colorScheme(scheme);
      for (const QString &name : kAccentSamples) {
        colors.accent = QColor(name);
        const ThemeSnapshot snapshot = buildThemeSnapshot(colors, font, {});
        QCOMPARE(snapshot.colors.accentText,
                 AccentContrast::textOn(colors.accent));
      }
    }
  }

  void accentStatesKeepTheTextContrastOfTheAccent() {
    for (const QString &name : kAccentSamples) {
      const QColor accent(name);
      const QColor text = AccentContrast::textOn(accent);
      const qreal base = AccentContrast::contrastRatio(text, accent);
      for (const int step : {AccentContrast::kHoverStepPercent,
                             AccentContrast::kPressedStepPercent}) {
        const QColor fill = AccentContrast::stateFill(accent, text, step);
        QCOMPARE_GE(AccentContrast::contrastRatio(text, fill), base);
      }
    }
  }

  void accentStatesMoveAwayFromTheText() {
    // Under white text the states are darker, under black text lighter.
    const QColor blue(u"#0078d4"_s);
    QCOMPARE_LT(AccentContrast::relativeLuminance(AccentContrast::stateFill(
                    blue, QColor(Qt::white), AccentContrast::kHoverStepPercent)),
                AccentContrast::relativeLuminance(blue));
    const QColor teal(u"#00b7c3"_s);
    QCOMPARE_GT(AccentContrast::relativeLuminance(AccentContrast::stateFill(
                    teal, QColor(Qt::black), AccentContrast::kHoverStepPercent)),
                AccentContrast::relativeLuminance(teal));
  }

  void snapshotCarriesTheAccentStates() {
    const QFont font = fontWithPointSize(kDefaultFontPointSize);
    ColorScheme colors = ThemeStore::colorScheme(COLORS_DARK);
    for (const QString &name : kAccentSamples) {
      colors.accent = QColor(name);
      const ThemeSnapshot snapshot = buildThemeSnapshot(colors, font, {});
      const QColor text = snapshot.colors.accentText;
      QCOMPARE(snapshot.colors.accentHover,
               AccentContrast::stateFill(colors.accent, text,
                                         AccentContrast::kHoverStepPercent));
      QCOMPARE(snapshot.colors.accentPressed,
               AccentContrast::stateFill(colors.accent, text,
                                         AccentContrast::kPressedStepPercent));
    }
  }

  void neutralFillsAreInkOfTheScheme() {
    const NeutralFills::Colors dark = NeutralFills::colorsFor(true);
    QCOMPARE(dark.rest.rgb(), QColor(Qt::white).rgb());
    QCOMPARE(dark.rest.alphaF(), NeutralFills::kDarkRestAlpha);
    QCOMPARE(dark.hover.alphaF(), NeutralFills::kDarkHoverAlpha);
    QCOMPARE(dark.pressed.alphaF(), NeutralFills::kDarkPressedAlpha);
    const NeutralFills::Colors light = NeutralFills::colorsFor(false);
    QCOMPARE(light.rest.rgb(), QColor(Qt::black).rgb());
    QCOMPARE(light.rest.alphaF(), NeutralFills::kLightRestAlpha);
    QCOMPARE(light.hover.alphaF(), NeutralFills::kLightHoverAlpha);
    QCOMPARE(light.pressed.alphaF(), NeutralFills::kLightPressedAlpha);
  }

  void neutralFillsKeepTheDirectionOfThePressedFill() {
    // Pressed is fainter than hover on the dark scheme and stronger on the
    // light one, as the opaque button colours were.
    const NeutralFills::Colors dark = NeutralFills::colorsFor(true);
    QCOMPARE_LT(dark.pressed.alphaF(), dark.hover.alphaF());
    const NeutralFills::Colors light = NeutralFills::colorsFor(false);
    QCOMPARE_GT(light.pressed.alphaF(), light.hover.alphaF());
    QCOMPARE_GT(light.hover.alphaF(), light.rest.alphaF());
  }

  void hudFillsAreWhiteInBothSchemes() {
    for (const bool dark : {true, false}) {
      const NeutralFills::Colors fills = NeutralFills::colorsFor(dark);
      QCOMPARE(fills.hudHover.rgb(), QColor(Qt::white).rgb());
      QCOMPARE(fills.hudHover.alphaF(), NeutralFills::kHudHoverAlpha);
      QCOMPARE(fills.hudPressed.rgb(), QColor(Qt::white).rgb());
      QCOMPARE(fills.hudPressed.alphaF(), NeutralFills::kHudPressedAlpha);
    }
  }

  void strokesAreInkOfTheSchemeFromFaintToStrong() {
    for (const bool dark : {true, false}) {
      const NeutralFills::Colors fills = NeutralFills::colorsFor(dark);
      const QColor ink = QColor(dark ? Qt::white : Qt::black);
      QCOMPARE(fills.strokeSubtle.rgb(), ink.rgb());
      QCOMPARE(fills.strokeDefault.rgb(), ink.rgb());
      QCOMPARE(fills.strokeStrong.rgb(), ink.rgb());
      QCOMPARE_LE(fills.strokeSubtle.alphaF(), fills.strokeDefault.alphaF());
      QCOMPARE_LE(fills.strokeDefault.alphaF(), fills.strokeStrong.alphaF());
    }
    QCOMPARE(NeutralFills::colorsFor(true).strokeDefault.alphaF(),
             NeutralFills::kDarkStrokeDefaultAlpha);
    QCOMPARE(NeutralFills::colorsFor(false).strokeDefault.alphaF(),
             NeutralFills::kLightStrokeDefaultAlpha);
  }

  void snapshotCarriesTheNeutralFills() {
    const QFont font = fontWithPointSize(kDefaultFontPointSize);
    for (const ColorSchemes scheme : {COLORS_DARK, COLORS_LIGHT}) {
      const ThemeSnapshot snapshot =
          buildThemeSnapshot(ThemeStore::colorScheme(scheme), font, {});
      const NeutralFills::Colors fills =
          NeutralFills::colorsFor(scheme == COLORS_DARK);
      QCOMPARE(snapshot.colors.fillRest, fills.rest);
      QCOMPARE(snapshot.colors.fillHover, fills.hover);
      QCOMPARE(snapshot.colors.fillPressed, fills.pressed);
      QCOMPARE(snapshot.colors.hudHover, fills.hudHover);
      QCOMPARE(snapshot.colors.hudPressed, fills.hudPressed);
      QCOMPARE(snapshot.colors.strokeSubtle, fills.strokeSubtle);
      QCOMPARE(snapshot.colors.strokeDefault, fills.strokeDefault);
      QCOMPARE(snapshot.colors.strokeStrong, fills.strokeStrong);
    }
  }

  void selectionFillsAreTheAccentAtFixedOpacities() {
    for (const QString &name : kAccentSamples) {
      const QColor accent(name);
      const SelectionFills::Colors fills = SelectionFills::colorsFor(accent);
      QCOMPARE(fills.tint.rgb(), accent.rgb());
      QCOMPARE(fills.tintHover.rgb(), accent.rgb());
      QCOMPARE(fills.frame.rgb(), accent.rgb());
      QCOMPARE(fills.tint.alphaF(), SelectionFills::kTintAlpha);
      QCOMPARE(fills.tintHover.alphaF(), SelectionFills::kTintHoverAlpha);
      QCOMPARE(fills.frame.alphaF(), SelectionFills::kFrameAlpha);
    }
  }

  void selectionFillsGrowFromTintToFrame() {
    QCOMPARE_LT(SelectionFills::kTintAlpha, SelectionFills::kTintHoverAlpha);
    QCOMPARE_LT(SelectionFills::kTintHoverAlpha, SelectionFills::kFrameAlpha);
  }

  void snapshotCarriesTheSelectionFills() {
    const QFont font = fontWithPointSize(kDefaultFontPointSize);
    ColorScheme colors = ThemeStore::colorScheme(COLORS_LIGHT);
    for (const QString &name : kAccentSamples) {
      colors.accent = QColor(name);
      const ThemeSnapshot snapshot = buildThemeSnapshot(colors, font, {});
      const SelectionFills::Colors fills =
          SelectionFills::colorsFor(colors.accent);
      QCOMPARE(snapshot.colors.accentTint, fills.tint);
      QCOMPARE(snapshot.colors.accentTintHover, fills.tintHover);
      QCOMPARE(snapshot.colors.accentFrame, fills.frame);
    }
  }

  void snapshotCarriesTheSchemeAndMetrics_data() {
    QTest::addColumn<int>("scheme");
    QTest::addColumn<bool>("dark");
    QTest::newRow("dark") << static_cast<int>(COLORS_DARK) << true;
    QTest::newRow("light") << static_cast<int>(COLORS_LIGHT) << false;
  }

  void snapshotCarriesTheSchemeAndMetrics() {
    QFETCH(int, scheme);
    QFETCH(bool, dark);
    const ColorScheme colors =
        ThemeStore::colorScheme(static_cast<ColorSchemes>(scheme));
    const QFont font = fontWithPointSize(kLargeFontPointSize);
    const QString iconFamily = u"Icons"_s;

    const ThemeSnapshot snapshot = buildThemeSnapshot(colors, font, iconFamily);

    QCOMPARE(snapshot.dark, dark);
    QCOMPARE(snapshot.iconFontFamily, iconFamily);
    QCOMPARE(snapshot.colors.background, colors.background);
    QCOMPARE(snapshot.colors.folderViewHc, colors.folderview_hc);
    QCOMPARE(snapshot.colors.thumbPanelText, colors.thumbpanel_text);
    QCOMPARE(snapshot.colors.comboFieldBorder, colors.combo_field_border);
    QCOMPARE(snapshot.colors.scrollbarHover, colors.scrollbar_hover);

    const UiMetrics::Typography typography = UiMetrics::typographyFor(font);
    QCOMPARE(snapshot.fonts.caption, typography.caption);
    QCOMPARE(snapshot.fonts.body, typography.body);
    QCOMPARE(snapshot.fonts.bodyStrong, typography.bodyStrong);
    QCOMPARE(snapshot.fonts.title, typography.title);

    // Controls are sized for the body style, not for the system font.
    const UiMetrics::ControlMetrics metrics =
        UiMetrics::controlMetricsFor(typography.body);
    QCOMPARE(snapshot.metrics.buttonHeight, metrics.buttonHeight);
    QCOMPARE(snapshot.metrics.contextMenuWidth, metrics.contextMenuWidth);
    QCOMPARE(snapshot.metrics.contextMenuItemHeight, metrics.contextMenuItemHeight);
    QCOMPARE(snapshot.metrics.tooltipBorderRadius, metrics.tooltipBorderRadius);
    QCOMPARE(snapshot.metrics.contextMenuBorderRadius,
             metrics.contextMenuBorderRadius);
  }
};

int runUiMetricsTests(int argc, char **argv) {
  UiMetricsTests tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "tst_uimetrics.moc"
