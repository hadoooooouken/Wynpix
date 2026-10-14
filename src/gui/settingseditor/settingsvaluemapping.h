#pragma once

#include <QColor>

#include "components/settingseditor/settingsvalues.h"

class Settings;

// Reads and stores the settings dialog's values in the application's
// Settings exactly as the former widget dialog did, so existing
// configurations keep their meaning. Depends on Settings only, so the stored
// keys can be checked without the rest of the application.
//
// GUI thread only.
class SettingsValueMapping {
public:
    // settings must outlive the mapping.
    explicit SettingsValueMapping(Settings &settings);

    [[nodiscard]] SettingsValues load() const;
    // Stores every value and saves the theme; sends no change notification.
    void store(const SettingsValues &values);
    // Sets the current colour scheme with customAccent and saves the theme.
    void storeCustomAccent(const QColor &customAccent);

private:
    void storeGeneral(const GeneralSettings &general);
    void storeView(const ViewSettings &view);
    void storeTheme(const ThemeSettings &theme);
    void storeControls(const ControlsSettings &controls);
    void storeAdvanced(const AdvancedSettings &advanced);
    void storeUpscale(const UpscaleSettings &upscale);

    Settings &mSettings;
};
