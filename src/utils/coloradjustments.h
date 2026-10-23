#pragma once

#include <algorithm>
#include <cmath>
#include <numbers>

// Threshold below which a colour adjustment component counts as "unchanged".
inline constexpr float kColorAdjustmentEpsilon = 0.001f;

// Rec.709 luma weights: saturation and vibrance pivot around luma.
inline constexpr float kColorLumaR = 0.2126f;
inline constexpr float kColorLumaG = 0.7152f;
inline constexpr float kColorLumaB = 0.0722f;
// Positive vibrance scales the HSV saturation S = (peak - low) / peak around
// the peak channel (the peak itself stays put) by 1 + vibrance * boost(S),
// boost(S) = kVibrancePosBoost - kVibrancePosFalloff * t,
// t = clamp((S - kVibrancePosFalloffStart) / (1 - kVibrancePosFalloffStart), 0, 1).
// So +100 % is a roughly constant ~1.3x saturation lift that fades to exactly
// 1.0x for fully saturated colours. Fitted to Photoshop's Vibrance +100.
// Falloff == boost keeps S * factor <= 1 for every S and vibrance, so the low
// channel never hits the 0 clamp (which would shift the hue of near-pure colours).
inline constexpr float kVibrancePosBoost = 0.28f;
inline constexpr float kVibrancePosFalloff = 0.28f;
inline constexpr float kVibrancePosFalloffStart = 0.6f;
// Negative vibrance damps vivid colours into pastel around luma: weight(c) =
// base + slope * c, without inverting gradients (base + 2 * slope <= 1) or
// draining skin tones.
inline constexpr float kVibranceNegBase = 0.5f;
inline constexpr float kVibranceNegSlope = 0.25f;
// Guards the S = chroma / peak division for black pixels.
inline constexpr float kVibranceMinPeak = 1e-4f;

// Interactive colour adjustments applied by the viewer (GPU preview), by
// ImageLib::applyColorAdjustments() (CPU apply/export) and by the batch
// converter. Default-constructed values are the identity transform.
struct ColorAdjustments {
    static constexpr float kNeutralExposure = 0.0f;
    static constexpr float kNeutralContrast = 1.0f;
    static constexpr float kNeutralBrightness = 0.0f;
    static constexpr float kNeutralTemperature = 0.0f;
    static constexpr float kNeutralTint = 0.0f;
    static constexpr float kNeutralSaturation = 1.0f;
    static constexpr float kNeutralHue = 0.0f;
    static constexpr float kNeutralSmartEnhance = 0.0f;
    static constexpr float kNeutralVibrance = 0.0f;

    float smartEnhance = kNeutralSmartEnhance; // 0.0 to 1.0 (0% to 100%)
    float exposure = kNeutralExposure;       // stops
    float contrast = kNeutralContrast;       // multiplier
    float brightness = kNeutralBrightness;   // offset
    float temperature = kNeutralTemperature; // white balance blue <-> amber
    float tint = kNeutralTint;               // white balance green <-> magenta
    float saturation = kNeutralSaturation;   // multiplier
    float hue = kNeutralHue;                 // degrees
    float vibrance = kNeutralVibrance;       // -1.0 to 1.0, boosts muted colours most

    [[nodiscard]] bool hasMatrixAdjustments() const noexcept {
        return std::abs(exposure - kNeutralExposure) > kColorAdjustmentEpsilon ||
               std::abs(contrast - kNeutralContrast) > kColorAdjustmentEpsilon ||
               std::abs(brightness - kNeutralBrightness) > kColorAdjustmentEpsilon ||
               std::abs(temperature - kNeutralTemperature) > kColorAdjustmentEpsilon ||
               std::abs(tint - kNeutralTint) > kColorAdjustmentEpsilon ||
               std::abs(saturation - kNeutralSaturation) > kColorAdjustmentEpsilon ||
               std::abs(hue - kNeutralHue) > kColorAdjustmentEpsilon;
    }

    // True when at least one component differs from the identity transform.
    [[nodiscard]] bool hasAdjustments() const noexcept {
        return hasMatrixAdjustments() || hasSmartEnhance() || hasVibrance();
    }

    [[nodiscard]] bool hasSmartEnhance() const noexcept {
        return std::abs(smartEnhance - kNeutralSmartEnhance) > kColorAdjustmentEpsilon;
    }

    // Vibrance depends on the saturation of each pixel, so it is not part of
    // the colour matrix (see applyColorAdjustmentVibrance()).
    [[nodiscard]] bool hasVibrance() const noexcept {
        return std::abs(vibrance - kNeutralVibrance) > kColorAdjustmentEpsilon;
    }

    friend bool operator==(const ColorAdjustments &,
                           const ColorAdjustments &) = default;
};

// Affine colour transform of straight (non-premultiplied) RGB in [0, 1]:
// out[i] = sum_j m[i][j] * in[j] + offset, clamped to [0, 1] by the consumer.
struct ColorMatrix {
    // Default-constructed value is the identity transform.
    float m[3][3] = {{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};
    float offset = 0.0f;

    friend bool operator==(const ColorMatrix &, const ColorMatrix &) = default;
};

// The one definition of the ColorAdjustments -> ColorMatrix mapping, shared
// by the CPU path (ImageLib) and the QRhi renderer.
// Pure arithmetic, safe on any thread.
[[nodiscard]] inline ColorMatrix colorAdjustmentMatrix(const ColorAdjustments &adjustments) {
    // Rec.709 luma weights (saturation pivots around luma).
    constexpr float kLumaR = kColorLumaR;
    constexpr float kLumaG = kColorLumaG;
    constexpr float kLumaB = kColorLumaB;
    // Unit grey axis component 1 / sqrt(3): hue rotates around (1, 1, 1).
    constexpr float kGreyAxis = 0.57735f;
    constexpr float kExposureBase = 2.0f;
    // Tint moves green against the mean of red and blue.
    constexpr float kTintRedBlueShare = 0.5f;
    // Contrast pivots around mid grey.
    constexpr float kContrastPivot = 0.5f;
    constexpr float kDegreesPerHalfTurn = 180.0f;

    const auto multiply = [](const float a[3][3], const float b[3][3], float c[3][3]) {
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                c[i][j] = a[i][0] * b[0][j] + a[i][1] * b[1][j] + a[i][2] * b[2][j];
            }
        }
    };
    const auto assign = [](float target[3][3], const float source[3][3]) {
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) target[i][j] = source[i][j];
        }
    };

    // 1 & 2. White balance (temperature & tint) and exposure.
    const float temperature = adjustments.temperature;
    const float tint = adjustments.tint;
    const float factor = std::pow(kExposureBase, adjustments.exposure);
    const float wR = (1.0f + temperature + tint * kTintRedBlueShare) * factor;
    const float wG = (1.0f - tint) * factor;
    const float wB = (1.0f - temperature + tint * kTintRedBlueShare) * factor;

    float current[3][3] = {
        {wR,   0.0f, 0.0f},
        {0.0f, wG,   0.0f},
        {0.0f, 0.0f, wB  }
    };

    // 3. Hue rotation around the grey axis.
    if (std::abs(adjustments.hue) > kColorAdjustmentEpsilon) {
        const float hueRad = adjustments.hue * std::numbers::pi_v<float> / kDegreesPerHalfTurn;
        const float cosAngle = std::cos(hueRad);
        const float sinAngle = std::sin(hueRad);
        const float k = kGreyAxis;
        const float cosInv = 1.0f - cosAngle;

        const float hueMatrix[3][3] = {
            { cosAngle + k * k * cosInv,      -k * sinAngle + k * k * cosInv,  k * sinAngle + k * k * cosInv },
            { k * sinAngle + k * k * cosInv,  cosAngle + k * k * cosInv,       -k * sinAngle + k * k * cosInv },
            { -k * sinAngle + k * k * cosInv, k * sinAngle + k * k * cosInv,   cosAngle + k * k * cosInv }
        };

        float product[3][3];
        multiply(hueMatrix, current, product);
        assign(current, product);
    }

    // 4. Saturation.
    const float saturation = adjustments.saturation;
    if (std::abs(saturation - ColorAdjustments::kNeutralSaturation) > kColorAdjustmentEpsilon) {
        const float rWeight = kLumaR * (1.0f - saturation);
        const float gWeight = kLumaG * (1.0f - saturation);
        const float bWeight = kLumaB * (1.0f - saturation);

        const float saturationMatrix[3][3] = {
            { saturation + rWeight, gWeight,              bWeight },
            { rWeight,              saturation + gWeight, bWeight },
            { rWeight,              gWeight,              saturation + bWeight }
        };

        float product[3][3];
        multiply(saturationMatrix, current, product);
        assign(current, product);
    }

    // 5 & 6. Contrast and brightness.
    const float contrast = adjustments.contrast;
    ColorMatrix result;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            result.m[i][j] = current[i][j] * contrast;
        }
    }
    result.offset = adjustments.brightness * contrast + kContrastPivot * (1.0f - contrast);
    return result;
}

// Vibrance on straight RGB in [0, 1], applied after the colour matrix.
// Mirrored by applyVibrance() in res/shaders/rhi/image.frag and panorama.frag
// and by the AVX2 loop of ImageLib::applyColorAdjustmentsWithMode().
// Pure arithmetic, safe on any thread.
inline void applyColorAdjustmentVibrance(float (&rgb)[3], float vibrance) {
    const float peak = std::max({rgb[0], rgb[1], rgb[2]});
    const float low = std::min({rgb[0], rgb[1], rgb[2]});
    const float chroma = peak - low;
    if (vibrance >= 0.0f) {
        // Pivot on the peak channel: only the weaker channels move.
        const float saturation = chroma / std::max(peak, kVibranceMinPeak);
        const float t = std::clamp((saturation - kVibrancePosFalloffStart) /
                                       (1.0f - kVibrancePosFalloffStart),
                                   0.0f, 1.0f);
        const float factor = 1.0f + vibrance * (kVibrancePosBoost - kVibrancePosFalloff * t);
        for (float &channel : rgb) {
            channel = std::clamp(peak + (channel - peak) * factor, 0.0f, 1.0f);
        }
        return;
    }
    const float luma = kColorLumaR * rgb[0] + kColorLumaG * rgb[1] + kColorLumaB * rgb[2];
    const float weight = kVibranceNegBase + kVibranceNegSlope * chroma;
    const float factor = std::max(0.0f, 1.0f + vibrance * weight);
    for (float &channel : rgb) {
        channel = std::clamp(luma + (channel - luma) * factor, 0.0f, 1.0f);
    }
}
