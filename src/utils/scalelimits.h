#pragma once

#include <QSize>
#include <QtGlobal>

#include <cstdint>

enum class ResolutionLimitMode : uint8_t {
    Standard, // 12288 px / 100 MP
    Upscayl   // 16384 px / 256 MiPx (268,435,456 px)
};

struct ResolutionLimits {
    int maximumSide = 0;
    qint64 maximumPixels = 0;
};

namespace ScaleLimits {

inline constexpr ResolutionLimits kStandard{.maximumSide = 12288,
                                            .maximumPixels = 100'000'000};
// Upscayl output is larger than what the regular resize path is allowed to
// produce; capped to GPU max texture size / GDI memory safety (16384 px) and
// 256 MiPx (~1.07 GB RGBA8) to prevent allocation crashes.
inline constexpr ResolutionLimits kUpscayl{.maximumSide = 16384,
                                           .maximumPixels = 268'435'456};

[[nodiscard]] constexpr ResolutionLimits limitsFor(ResolutionLimitMode mode) noexcept {
    return mode == ResolutionLimitMode::Upscayl ? kUpscayl : kStandard;
}

[[nodiscard]] inline bool exceedsLimits(QSize target, ResolutionLimitMode mode) noexcept {
    const ResolutionLimits limits = limitsFor(mode);
    return target.width() > limits.maximumSide || target.height() > limits.maximumSide ||
           static_cast<qint64>(target.width()) * target.height() > limits.maximumPixels;
}

} // namespace ScaleLimits
