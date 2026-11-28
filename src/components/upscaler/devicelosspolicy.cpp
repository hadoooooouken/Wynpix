#include "devicelosspolicy.h"

#include <algorithm>

bool DeviceLossPolicy::beginAttempt(qint64 nowMs) {
    if (mCooldownStartMs) {
        const qint64 elapsedMs = nowMs - *mCooldownStartMs;
        if (elapsedMs >= 0 && elapsedMs < kCooldownMs) {
            return false;
        }

        mCooldownStartMs.reset();
        // Allow a single probe attempt after cooldown without an automatic retry,
        // staying below Windows' TdrLimitCount threshold if the GPU still fails.
        mConsecutiveFailures = kMaxConsecutiveFailures - 1;
        return true;
    }

    if (mLastFailureMs && (nowMs - *mLastFailureMs >= kFailureResetWindowMs)) {
        mConsecutiveFailures = 0;
        mTileSizeCap.reset();
        mLastFailureMs.reset();
    }

    return true;
}

DeviceLossPolicy::RecoveryAction DeviceLossPolicy::recordFailure(qint64 nowMs, int currentTileSize) {
    ++mConsecutiveFailures;
    mLastFailureMs = nowMs;

    if (currentTileSize > kMinimumRetryTileSize) {
        const int reducedTileSize = (std::max)(currentTileSize / 2, kMinimumRetryTileSize);
        mTileSizeCap = mTileSizeCap ? (std::min)(*mTileSizeCap, reducedTileSize) : reducedTileSize;
    }

    if (mConsecutiveFailures < kMaxConsecutiveFailures) {
        return {.shouldRetry = true, .retryTileSize = mTileSizeCap, .cooldownStarted = false};
    }

    mCooldownStartMs = nowMs;
    return {.shouldRetry = false, .retryTileSize = std::nullopt, .cooldownStarted = true};
}

void DeviceLossPolicy::recordSuccess() {
    mConsecutiveFailures = 0;
    mTileSizeCap.reset();
    mLastFailureMs.reset();
    mCooldownStartMs.reset();
}
