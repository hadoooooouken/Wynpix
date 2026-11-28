#include <QTest>

#include "components/upscaler/devicelosspolicy.h"
#include "testsuites.h"

namespace {
constexpr qint64 kInitialTimeMs = 1000;
constexpr qint64 kShortElapsedMs = 5000;
constexpr qint64 kJustBeforeCooldownEndMs = DeviceLossPolicy::kCooldownMs - 1;
constexpr int kLargeTileSize = 512;
constexpr int kHalfLargeTileSize = 256;
constexpr int kSmallTileSize = 100;
} // namespace

class DeviceLossPolicyTests : public QObject {
  Q_OBJECT

private slots:
  void firstFailureRetriesWithHalvedTileSize() {
    DeviceLossPolicy policy;
    QVERIFY(policy.beginAttempt(kInitialTimeMs));
    QCOMPARE(policy.consecutiveFailures(), 0);
    QVERIFY(!policy.tileSizeCap().has_value());

    const DeviceLossPolicy::RecoveryAction action =
        policy.recordFailure(kInitialTimeMs, kLargeTileSize);
    QVERIFY(action.shouldRetry);
    QCOMPARE(action.retryTileSize, std::optional<int>(kHalfLargeTileSize));
    QCOMPARE(policy.tileSizeCap(), std::optional<int>(kHalfLargeTileSize));
    QVERIFY(!action.cooldownStarted);
    QVERIFY(!policy.inCooldown());
    QCOMPARE(policy.consecutiveFailures(), 1);
  }

  void retryTileSizeRespectsMinimumAndPersistsIfReinitFails() {
    DeviceLossPolicy policy;
    const DeviceLossPolicy::RecoveryAction clamped =
        policy.recordFailure(kInitialTimeMs, kSmallTileSize);
    QVERIFY(clamped.shouldRetry);
    QCOMPARE(clamped.retryTileSize,
             std::optional<int>(DeviceLossPolicy::kMinimumRetryTileSize));
    // Subsequent attempt before reset keeps the reduced tile cap even if reinit failed.
    QVERIFY(policy.beginAttempt(kInitialTimeMs + kShortElapsedMs));
    QCOMPARE(policy.tileSizeCap(),
             std::optional<int>(DeviceLossPolicy::kMinimumRetryTileSize));

    DeviceLossPolicy atMinimum;
    const DeviceLossPolicy::RecoveryAction noStepDown =
        atMinimum.recordFailure(kInitialTimeMs, DeviceLossPolicy::kMinimumRetryTileSize);
    QVERIFY(noStepDown.shouldRetry);
    QVERIFY(!noStepDown.retryTileSize.has_value());
    QVERIFY(!atMinimum.tileSizeCap().has_value());
  }

  void secondFailureEntersCooldownUntilExpiryThenAllowsSingleProbe() {
    DeviceLossPolicy policy;
    QVERIFY(policy.recordFailure(kInitialTimeMs, kLargeTileSize).shouldRetry);

    const qint64 cooldownStartMs = kInitialTimeMs + kShortElapsedMs;
    const DeviceLossPolicy::RecoveryAction second =
        policy.recordFailure(cooldownStartMs, kHalfLargeTileSize);
    QVERIFY(!second.shouldRetry);
    QVERIFY(!second.retryTileSize.has_value());
    QVERIFY(second.cooldownStarted);
    QVERIFY(policy.inCooldown());
    QVERIFY(!policy.beginAttempt(cooldownStartMs));
    QVERIFY(!policy.beginAttempt(cooldownStartMs + kJustBeforeCooldownEndMs));

    const qint64 cooldownEndMs = cooldownStartMs + DeviceLossPolicy::kCooldownMs;
    QVERIFY(policy.beginAttempt(cooldownEndMs));
    QVERIFY(!policy.inCooldown());
    QCOMPARE(policy.consecutiveFailures(), DeviceLossPolicy::kMaxConsecutiveFailures - 1);
    QCOMPARE(policy.tileSizeCap(), std::optional<int>(kHalfLargeTileSize / 2));

    // Probe failure immediately re-enters cooldown without a second retry.
    const DeviceLossPolicy::RecoveryAction probeFailure =
        policy.recordFailure(cooldownEndMs, kHalfLargeTileSize / 2);
    QVERIFY(!probeFailure.shouldRetry);
    QVERIFY(probeFailure.cooldownStarted);
    QVERIFY(!policy.beginAttempt(cooldownEndMs));
  }

  void oldSingleFailureAgesOutAfterResetWindow() {
    DeviceLossPolicy policy;
    QVERIFY(policy.recordFailure(kInitialTimeMs, kLargeTileSize).shouldRetry);
    QCOMPARE(policy.consecutiveFailures(), 1);
    QCOMPARE(policy.tileSizeCap(), std::optional<int>(kHalfLargeTileSize));

    const qint64 afterWindowMs = kInitialTimeMs + DeviceLossPolicy::kFailureResetWindowMs;
    QVERIFY(policy.beginAttempt(afterWindowMs));
    QCOMPARE(policy.consecutiveFailures(), 0);
    QVERIFY(!policy.tileSizeCap().has_value());
  }

  void successResetsFailureCountTileCapAndCooldown() {
    DeviceLossPolicy policy;
    QVERIFY(policy.recordFailure(kInitialTimeMs, kLargeTileSize).shouldRetry);
    QVERIFY(policy.recordFailure(kInitialTimeMs, kHalfLargeTileSize).cooldownStarted);
    QVERIFY(!policy.beginAttempt(kInitialTimeMs));

    policy.recordSuccess();
    QVERIFY(!policy.inCooldown());
    QVERIFY(policy.beginAttempt(kInitialTimeMs));
    QCOMPARE(policy.consecutiveFailures(), 0);
    QVERIFY(!policy.tileSizeCap().has_value());

    const DeviceLossPolicy::RecoveryAction afterReset =
        policy.recordFailure(kInitialTimeMs + kShortElapsedMs, kLargeTileSize);
    QVERIFY(afterReset.shouldRetry);
    QCOMPARE(afterReset.retryTileSize, std::optional<int>(kHalfLargeTileSize));
  }
};

int runDeviceLossPolicyTests(int argc, char **argv) {
  DeviceLossPolicyTests tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "tst_devicelosspolicy.moc"
