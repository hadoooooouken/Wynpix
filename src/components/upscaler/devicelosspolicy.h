#pragma once

#include <QtTypes>
#include <optional>

// Tracks consecutive Vulkan device-loss failures (for example after Windows TDR
// resets) and decides when to allow a stepped-down retry versus entering a
// cooldown that keeps Windows from blocking the process from graphics hardware.
class DeviceLossPolicy final {
public:
    static constexpr int kMaxConsecutiveFailures = 2;
    static constexpr qint64 kCooldownMs = 30000;
    static constexpr qint64 kFailureResetWindowMs = 60000;
    static constexpr int kMinimumRetryTileSize = 64;

    struct RecoveryAction final {
        bool shouldRetry = false;
        std::optional<int> retryTileSize;
        bool cooldownStarted = false;
    };

    // Checks whether an inference attempt may proceed at `nowMs`. Expires any
    // finished cooldown (allowing a single probe attempt) and ages out a prior
    // non-cooldown failure once `kFailureResetWindowMs` has elapsed.
    [[nodiscard]] bool beginAttempt(qint64 nowMs);
    RecoveryAction recordFailure(qint64 nowMs, int currentTileSize);
    void recordSuccess();

    [[nodiscard]] int consecutiveFailures() const noexcept { return mConsecutiveFailures; }
    [[nodiscard]] bool inCooldown() const noexcept { return mCooldownStartMs.has_value(); }
    [[nodiscard]] std::optional<int> tileSizeCap() const noexcept { return mTileSizeCap; }

private:
    int mConsecutiveFailures = 0;
    std::optional<int> mTileSizeCap;
    std::optional<qint64> mLastFailureMs;
    std::optional<qint64> mCooldownStartMs;
};
