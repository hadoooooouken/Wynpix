#include "coloradjustmentseditor.h"

#include <QCoreApplication>

namespace {
// Slider units per adjustment unit (exposure in stops, multipliers and
// offsets); hue is in whole degrees.
constexpr float kSliderUnitsPerUnit = 100.0f;

// Ranges and defaults of ColorAdjustmentsOverlay. The labels use its
// translation context, so the existing translations apply.
QList<SliderSpec> colorAdjustmentSpecs() {
    const auto label = [](const char *text) {
        return QCoreApplication::translate("ColorAdjustmentsOverlay", text);
    };
    // Defaults: the identity transform (ColorAdjustments{}) in slider units.
    constexpr int kNeutral = 0;
    constexpr int kNeutralMultiplier = 100;
    // Exposure: +-3 stops.
    constexpr int kExposureLimit = 300;
    // Contrast: x0 - x3.
    constexpr int kContrastMax = 300;
    // Brightness: +-100%.
    constexpr int kBrightnessLimit = 100;
    // Temperature and tint shifts.
    constexpr int kColorShiftLimit = 50;
    // Vibrance: +-100%.
    constexpr int kVibranceLimit = 100;
    // Saturation: x0 - x2.
    constexpr int kSaturationMax = 200;
    // Hue rotation: a half turn either way.
    constexpr int kHueLimit = 180;
    // Smart Enhance: 0% to 100%.
    constexpr int kSmartEnhanceMax = 100;
    constexpr int kMultiplierMin = 0;
    return {
        {label(QT_TRANSLATE_NOOP("ColorAdjustmentsOverlay", "Smart Enhance")), kMultiplierMin, kSmartEnhanceMax, kNeutral, SliderValueFormat::Percent},
        {label(QT_TRANSLATE_NOOP("ColorAdjustmentsOverlay", "Exposure")), -kExposureLimit, kExposureLimit, kNeutral, SliderValueFormat::SignedHundredths},
        {label(QT_TRANSLATE_NOOP("ColorAdjustmentsOverlay", "Contrast")), kMultiplierMin, kContrastMax, kNeutralMultiplier, SliderValueFormat::Percent},
        {label(QT_TRANSLATE_NOOP("ColorAdjustmentsOverlay", "Brightness")), -kBrightnessLimit, kBrightnessLimit, kNeutral, SliderValueFormat::Percent},
        {label(QT_TRANSLATE_NOOP("ColorAdjustmentsOverlay", "Temperature")), -kColorShiftLimit, kColorShiftLimit, kNeutral, SliderValueFormat::Integer},
        {label(QT_TRANSLATE_NOOP("ColorAdjustmentsOverlay", "Tint")), -kColorShiftLimit, kColorShiftLimit, kNeutral, SliderValueFormat::Integer},
        {label(QT_TRANSLATE_NOOP("ColorAdjustmentsOverlay", "Hue")), -kHueLimit, kHueLimit, kNeutral, SliderValueFormat::Degrees},
        {label(QT_TRANSLATE_NOOP("ColorAdjustmentsOverlay", "Vibrance")), -kVibranceLimit, kVibranceLimit, kNeutral, SliderValueFormat::Percent},
        {label(QT_TRANSLATE_NOOP("ColorAdjustmentsOverlay", "Saturation")), kMultiplierMin, kSaturationMax, kNeutralMultiplier, SliderValueFormat::Percent},
    };
}
} // namespace

ColorAdjustmentsEditor::ColorAdjustmentsEditor(QObject *parent)
    : AdjustmentSliderModel(colorAdjustmentSpecs(), parent) {}

ColorAdjustments ColorAdjustmentsEditor::adjustments() const {
    const auto scaled = [this](Row row) { return value(row) / kSliderUnitsPerUnit; };
    return {
        .smartEnhance = scaled(SmartEnhance),
        .exposure = scaled(Exposure),
        .contrast = scaled(Contrast),
        .brightness = scaled(Brightness),
        .temperature = scaled(Temperature),
        .tint = scaled(Tint),
        .saturation = scaled(Saturation),
        .hue = static_cast<float>(value(Hue)),
        .vibrance = scaled(Vibrance),
    };
}

void ColorAdjustmentsEditor::setComparing(bool comparing) {
    if (mComparing == comparing)
        return;
    mComparing = comparing;
    emit previewChanged(mComparing ? ColorAdjustments{} : adjustments());
}

void ColorAdjustmentsEditor::apply() {
    emit applyRequested(adjustments());
    resetAll();
}

void ColorAdjustmentsEditor::valuesEdited() {
    if (!mComparing)
        emit previewChanged(adjustments());
}
