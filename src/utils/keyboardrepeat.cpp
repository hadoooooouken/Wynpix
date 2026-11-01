#include "keyboardrepeat.h"

#include <algorithm>
#include <cmath>

#include <QDebug>

#include <windows.h>

namespace {
constexpr double kMillisecondsPerSecond = 1000.0;
} // namespace

namespace KeyboardRepeat {

int intervalMsForSpeedSetting(int speedSetting) {
    const int setting = std::clamp(speedSetting, kMinSpeedSetting, kMaxSpeedSetting);
    const double fraction = static_cast<double>(setting - kMinSpeedSetting) /
                            (kMaxSpeedSetting - kMinSpeedSetting);
    const double repeatsPerSecond =
        kMinRepeatsPerSecond + fraction * (kMaxRepeatsPerSecond - kMinRepeatsPerSecond);
    return static_cast<int>(std::lround(kMillisecondsPerSecond / repeatsPerSecond));
}

std::optional<int> systemSpeedSetting() {
    DWORD speed = 0;
    if (!SystemParametersInfoW(SPI_GETKEYBOARDSPEED, 0, &speed, 0)) {
        qWarning() << "KeyboardRepeat: SPI_GETKEYBOARDSPEED failed, error"
                   << GetLastError();
        return std::nullopt;
    }
    return static_cast<int>(speed);
}

int systemIntervalMs() {
    return intervalMsForSpeedSetting(systemSpeedSetting().value_or(kMaxSpeedSetting));
}

} // namespace KeyboardRepeat
