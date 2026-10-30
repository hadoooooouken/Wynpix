#include "themebridge.h"

#include "gui/quick/bridges/publish.h"

#include <QDebug>

#include <optional>

//------------------------------------------------------------------------------
ThemeBridge::ThemeBridge(const ThemeSnapshot &initial, QObject *parent)
    : QObject(parent), mSnapshot(initial) {}

//------------------------------------------------------------------------------
const ThemeTokens &ThemeBridge::tokens() const { return mSnapshot.tokens; }

const ThemeFonts &ThemeBridge::fonts() const { return mSnapshot.fonts; }

const ThemeMetrics &ThemeBridge::metrics() const { return mSnapshot.metrics; }

const QString &ThemeBridge::iconFontFamily() const {
  return mSnapshot.iconFontFamily;
}

//------------------------------------------------------------------------------
QString ThemeBridge::glyph(FluentIcons::FluentIcon icon) const {
  const std::optional<char32_t> codepoint = FluentIcons::codepoint(icon);
  if (!codepoint) {
    qWarning() << "ThemeBridge: no glyph registered for" << icon;
    return {};
  }
  return QString::fromUcs4(&*codepoint, 1);
}

//------------------------------------------------------------------------------
void ThemeBridge::apply(const ThemeSnapshot &snapshot) {
  publishIfChanged(mSnapshot.tokens, snapshot.tokens, *this,
                   &ThemeBridge::tokensChanged);
  publishIfChanged(mSnapshot.fonts, snapshot.fonts, *this,
                   &ThemeBridge::fontsChanged);
  publishIfChanged(mSnapshot.metrics, snapshot.metrics, *this,
                   &ThemeBridge::metricsChanged);
  publishIfChanged(mSnapshot.iconFontFamily, snapshot.iconFontFamily, *this,
                   &ThemeBridge::iconFontFamilyChanged);
}
