#include <QFont>
#include <QFontMetrics>
#include <QTest>

#include "gui/accentcontrast.h"
#include "gui/dialogsurfaces.h"
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

// The control metrics and dialog surfaces of the Qt Quick Theme bridge, and
// the theme snapshot built from them.
class UiMetricsTests : public QObject {
  Q_OBJECT

private slots:
  void darkDialogSurfacesAreLighterTints() {
    const DialogSurfaces::Colors colors = DialogSurfaces::colorsFor(true);
    // The values the stylesheet used for the dark scheme.
    QCOMPARE(colors.window, grey(37));
    QCOMPARE(colors.text, grey(220));
    QCOMPARE(colors.tintedLc2, grey(43));
    QCOMPARE(colors.tintedLc, grey(51));
    QCOMPARE(colors.tinted, grey(57));
    QCOMPARE(colors.tintedHc, grey(72));
    QCOMPARE(colors.tintedHc2, grey(87));
  }

  void lightDialogSurfacesAreDarkerTints() {
    const DialogSurfaces::Colors colors = DialogSurfaces::colorsFor(false);
    QCOMPARE(colors.window, grey(245));
    QCOMPARE(colors.text, grey(30));
    QCOMPARE(colors.tintedLc2, grey(239));
    QCOMPARE(colors.tintedLc, grey(231));
    QCOMPARE(colors.tinted, grey(225));
    QCOMPARE(colors.tintedHc, grey(210));
    QCOMPARE(colors.tintedHc2, grey(195));
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

  void snapshotCarriesTheSchemeMetricsAndSurfaces_data() {
    QTest::addColumn<int>("scheme");
    QTest::addColumn<bool>("dark");
    QTest::newRow("dark") << static_cast<int>(COLORS_DARK) << true;
    QTest::newRow("light") << static_cast<int>(COLORS_LIGHT) << false;
  }

  void snapshotCarriesTheSchemeMetricsAndSurfaces() {
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
    QCOMPARE(snapshot.colors.button, colors.button);
    QCOMPARE(snapshot.colors.panelButtonHover, colors.panel_button_hover);
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

    const DialogSurfaces::Colors surfaces = DialogSurfaces::colorsFor(dark);
    QCOMPARE(snapshot.surfaces.window, surfaces.window);
    QCOMPARE(snapshot.surfaces.tintedHc2, surfaces.tintedHc2);
  }
};

int runUiMetricsTests(int argc, char **argv) {
  UiMetricsTests tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "tst_uimetrics.moc"
