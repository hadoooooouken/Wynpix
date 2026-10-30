#pragma once

#include "gui/quick/bridges/themedata.h"
#include "gui/quick/bridges/uisettings.h"

class Settings;

// Builds the UI settings passed to the controllers and the theme published by
// ThemeBridge from the application services. These are the only places that
// read Settings on behalf of the Qt Quick UI.
//
// GUI thread only (ThemeSnapshot reads the application font and the
// registered icon font).
namespace BridgeSnapshots {

[[nodiscard]] UiSettingsSnapshot readUiSettings(Settings &settings);
[[nodiscard]] ThemeSnapshot readTheme(Settings &settings);

} // namespace BridgeSnapshots
