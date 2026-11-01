#include <QSignalSpy>
#include <QTest>

#include "components/navigationthrottle/navigationthrottle.h"
#include "testsuites.h"
#include "utils/keyboardrepeat.h"

namespace {
constexpr NavigationThrottle::Timing kTestTiming{
    .rapidStepIntervalMs = 100,
    .settleDelayMs = 30,
};
// Longer than both intervals of kTestTiming.
constexpr int kPauseMs = 200;
// 1000 ms / 2.5 and 1000 ms / 30 repeats per second.
constexpr int kSlowestRepeatIntervalMs = 400;
constexpr int kFastestRepeatIntervalMs = 33;
} // namespace

class NavigationThrottleTests : public QObject {
    Q_OBJECT

private slots:
    void aSingleStepLoadsAtOnce() {
        NavigationThrottle throttle(kTestTiming);
        QSignalSpy settled(&throttle, &NavigationThrottle::settled);

        QVERIFY(throttle.admit());

        QVERIFY(!throttle.isPending());
        QTest::qWait(kPauseMs);
        QCOMPARE(settled.size(), 0);
    }

    void rapidStepsAreDeferredUntilTheySettle() {
        NavigationThrottle throttle(kTestTiming);
        QSignalSpy settled(&throttle, &NavigationThrottle::settled);

        QVERIFY(throttle.admit());
        QVERIFY(!throttle.admit());
        QVERIFY(!throttle.admit());

        QVERIFY(throttle.isPending());
        QVERIFY(settled.wait(kPauseMs));
        QCOMPARE(settled.size(), 1);
        QVERIFY(!throttle.isPending());
    }

    void aStepAfterAPauseLoadsAtOnceAgain() {
        NavigationThrottle throttle(kTestTiming);
        QSignalSpy settled(&throttle, &NavigationThrottle::settled);
        QVERIFY(throttle.admit());
        QVERIFY(!throttle.admit());
        QVERIFY(settled.wait(kPauseMs));

        QTest::qWait(kPauseMs);

        QVERIFY(throttle.admit());
        QVERIFY(!throttle.isPending());
    }

    void cancelDropsThePendingSettle() {
        NavigationThrottle throttle(kTestTiming);
        QSignalSpy settled(&throttle, &NavigationThrottle::settled);
        QVERIFY(throttle.admit());
        QVERIFY(!throttle.admit());

        throttle.cancel();

        QVERIFY(!throttle.isPending());
        QTest::qWait(kPauseMs);
        QCOMPARE(settled.size(), 0);
    }

    void timingFollowsTheRepeatInterval() {
        const auto timing =
            NavigationThrottle::Timing::forRepeatInterval(kFastestRepeatIntervalMs);

        const int expected = NavigationThrottle::kRepeatIntervalMargin * kFastestRepeatIntervalMs;
        QCOMPARE(timing.rapidStepIntervalMs, expected);
        QCOMPARE(timing.settleDelayMs, expected);
    }

    void repeatIntervalSpansTheWindowsRange() {
        QCOMPARE(KeyboardRepeat::intervalMsForSpeedSetting(KeyboardRepeat::kMinSpeedSetting),
                 kSlowestRepeatIntervalMs);
        QCOMPARE(KeyboardRepeat::intervalMsForSpeedSetting(KeyboardRepeat::kMaxSpeedSetting),
                 kFastestRepeatIntervalMs);
        // Out-of-range settings are clamped.
        QCOMPARE(KeyboardRepeat::intervalMsForSpeedSetting(KeyboardRepeat::kMaxSpeedSetting + 1),
                 kFastestRepeatIntervalMs);
        QCOMPARE(KeyboardRepeat::intervalMsForSpeedSetting(KeyboardRepeat::kMinSpeedSetting - 1),
                 kSlowestRepeatIntervalMs);
    }
};

int runNavigationThrottleTests(int argc, char **argv) {
    NavigationThrottleTests tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "tst_navigationthrottle.moc"
