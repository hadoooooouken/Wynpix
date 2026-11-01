#pragma once

#include <optional>

// Keyboard auto-repeat of Windows ("Repeat rate" in the keyboard settings).
// SPI_GETKEYBOARDSPEED reports a setting from 0 (about 2.5 repeats per
// second) to 31 (about 30 per second); the rates in between are linear, give
// or take the hardware's 20%.
namespace KeyboardRepeat {
inline constexpr int kMinSpeedSetting = 0;
inline constexpr int kMaxSpeedSetting = 31;
inline constexpr double kMinRepeatsPerSecond = 2.5;
inline constexpr double kMaxRepeatsPerSecond = 30.0;

// Milliseconds between two repeats at the speed setting, clamped to its
// range.
[[nodiscard]] int intervalMsForSpeedSetting(int speedSetting);
// The speed setting of this session; nullopt when Windows does not report
// it.
[[nodiscard]] std::optional<int> systemSpeedSetting();
// Milliseconds between two repeats as Windows sends them; the fastest rate
// when the setting cannot be read.
[[nodiscard]] int systemIntervalMs();
} // namespace KeyboardRepeat
