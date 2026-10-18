#include "batchsavedsettings.h"

#include <QAnyStringView>
#include <QSettings>
#include <QStringList>
#include <QVariant>

namespace BatchSavedSettings {
namespace {
using namespace Qt::StringLiterals;
using BatchJobRules::SavedSettings;

constexpr auto kGroup = "BatchConverter"_L1;

constexpr auto kFormatKey = "format"_L1;
constexpr auto kQualityKey = "quality"_L1;
constexpr auto kResizeEnabledKey = "resizeEnabled"_L1;
constexpr auto kResizeModeKey = "resizeMode"_L1;
constexpr auto kResizePercentKey = "resizePercent"_L1;
constexpr auto kTargetMegapixelsKey = "targetMegapixels"_L1;
constexpr auto kUseBinaryMegapixelKey = "useBinaryMegapixel"_L1;
constexpr auto kTargetWidthKey = "targetWidth"_L1;
constexpr auto kTargetHeightKey = "targetHeight"_L1;
constexpr auto kKeepAspectRatioKey = "keepAspectRatio"_L1;
constexpr auto kAspectFitModeKey = "aspectFitMode"_L1;
constexpr auto kScalingFilterKey = "scalingFilter"_L1;
constexpr auto kPatternKey = "pattern"_L1;
constexpr auto kCreateSubfolderKey = "createSubfolder"_L1;

// Enums are stored as integers; these are the largest values that are valid.
constexpr int kLastResizeMode = static_cast<int>(ResizeMode::ByMegapixels);
constexpr int kLastAspectFitMode = static_cast<int>(AspectFitMode::Height);
constexpr int kLastScalingFilter = static_cast<int>(QI_FILTER_MKS2021_GPU);

// Enters the section for the lifetime of the object.
class GroupScope {
public:
    explicit GroupScope(QSettings &settings) : mSettings(settings) {
        mSettings.beginGroup(kGroup);
    }
    ~GroupScope() {
        mSettings.endGroup();
    }
    GroupScope(const GroupScope &) = delete;
    GroupScope &operator=(const GroupScope &) = delete;

private:
    QSettings &mSettings;
};

[[nodiscard]] std::optional<int> readInt(const QSettings &settings, QAnyStringView key) {
    if (!settings.contains(key))
        return std::nullopt;
    bool ok = false;
    const int value = settings.value(key).toInt(&ok);
    return ok ? std::optional<int>(value) : std::nullopt;
}

[[nodiscard]] std::optional<double> readDouble(const QSettings &settings, QAnyStringView key) {
    if (!settings.contains(key))
        return std::nullopt;
    bool ok = false;
    const double value = settings.value(key).toDouble(&ok);
    return ok ? std::optional<double>(value) : std::nullopt;
}

template <typename Enum>
[[nodiscard]] std::optional<Enum> readEnum(const QSettings &settings, QAnyStringView key,
                                           int lastValue) {
    const std::optional<int> raw = readInt(settings, key);
    if (!raw || *raw < 0 || *raw > lastValue)
        return std::nullopt;
    return static_cast<Enum>(*raw);
}

template <typename T>
void writeOptional(QSettings &settings, QAnyStringView key, const std::optional<T> &value) {
    if (value)
        settings.setValue(key, *value);
    else
        settings.remove(key);
}
} // namespace

std::optional<SavedSettings> read(QSettings &settings) {
    const GroupScope group(settings);
    if (settings.childKeys().isEmpty())
        return std::nullopt;

    SavedSettings saved;
    saved.format = settings.value(kFormatKey).toString();
    saved.quality = readInt(settings, kQualityKey);
    saved.resizeEnabled = settings.value(kResizeEnabledKey, saved.resizeEnabled).toBool();
    saved.resizeMode =
        readEnum<ResizeMode>(settings, kResizeModeKey, kLastResizeMode).value_or(saved.resizeMode);
    saved.resizePercent = readDouble(settings, kResizePercentKey).value_or(saved.resizePercent);
    saved.targetMegapixels = readDouble(settings, kTargetMegapixelsKey);
    saved.useBinaryMegapixel =
        settings.value(kUseBinaryMegapixelKey, saved.useBinaryMegapixel).toBool();
    const std::optional<int> width = readInt(settings, kTargetWidthKey);
    const std::optional<int> height = readInt(settings, kTargetHeightKey);
    if (width && height)
        saved.targetSize = QSize(*width, *height);
    saved.keepAspectRatio = settings.value(kKeepAspectRatioKey, saved.keepAspectRatio).toBool();
    saved.aspectFitMode =
        readEnum<AspectFitMode>(settings, kAspectFitModeKey, kLastAspectFitMode)
            .value_or(saved.aspectFitMode);
    saved.scalingFilter = readEnum<ScalingFilter>(settings, kScalingFilterKey, kLastScalingFilter);
    saved.pattern = settings.value(kPatternKey).toString();
    saved.createSubfolder = settings.value(kCreateSubfolderKey, saved.createSubfolder).toBool();
    return saved;
}

void write(QSettings &settings, const SavedSettings &saved) {
    const GroupScope group(settings);
    settings.setValue(kFormatKey, saved.format);
    writeOptional(settings, kQualityKey, saved.quality);
    settings.setValue(kResizeEnabledKey, saved.resizeEnabled);
    settings.setValue(kResizeModeKey, static_cast<int>(saved.resizeMode));
    settings.setValue(kResizePercentKey, saved.resizePercent);
    writeOptional(settings, kTargetMegapixelsKey, saved.targetMegapixels);
    settings.setValue(kUseBinaryMegapixelKey, saved.useBinaryMegapixel);
    if (saved.targetSize.isValid()) {
        settings.setValue(kTargetWidthKey, saved.targetSize.width());
        settings.setValue(kTargetHeightKey, saved.targetSize.height());
    } else {
        settings.remove(kTargetWidthKey);
        settings.remove(kTargetHeightKey);
    }
    settings.setValue(kKeepAspectRatioKey, saved.keepAspectRatio);
    settings.setValue(kAspectFitModeKey, static_cast<int>(saved.aspectFitMode));
    writeOptional(settings, kScalingFilterKey,
                  saved.scalingFilter ? std::optional<int>(static_cast<int>(*saved.scalingFilter))
                                      : std::nullopt);
    settings.setValue(kPatternKey, saved.pattern);
    settings.setValue(kCreateSubfolderKey, saved.createSubfolder);
}

} // namespace BatchSavedSettings
