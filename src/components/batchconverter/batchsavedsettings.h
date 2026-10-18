#pragma once

#include <optional>

#include "batchjobrules.h"

class QSettings;

// Storage of the batch converter dialog's SavedSettings in the
// [BatchConverter] section of a settings file (wynpix.ini). Knows the key
// names and the stored types, nothing about Settings or the dialog; the
// values read are not validated beyond their type and enum range, see
// BatchJobRules::sanitized().
namespace BatchSavedSettings {

// Absent while the section holds no keys (nothing was saved yet). A key that
// is missing or unreadable keeps the default of SavedSettings.
[[nodiscard]] std::optional<BatchJobRules::SavedSettings> read(QSettings &settings);

// Replaces the section's keys; an absent optional value removes its key.
void write(QSettings &settings, const BatchJobRules::SavedSettings &saved);

} // namespace BatchSavedSettings
