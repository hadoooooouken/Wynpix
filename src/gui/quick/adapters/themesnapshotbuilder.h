#pragma once

#include <QFont>
#include <QString>

#include "gui/quick/bridges/themedata.h"

class ColorScheme;

// Builds the snapshot published by ThemeBridge from a colour scheme, the
// system font and the registered icon font family: every scheme colour, the
// text styles derived from the system font (UiMetrics::Typography), the
// control metrics of the body style (UiMetrics::ControlMetrics) and the
// dialog surfaces (DialogSurfaces).
//
// Pure: reads no global state, so the QML tests build the same snapshots
// from ThemeStore's schemes as the application does from Settings.
[[nodiscard]] ThemeSnapshot buildThemeSnapshot(const ColorScheme &scheme,
                                               const QFont &systemFont,
                                               const QString &iconFontFamily);
