#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>

// Holds back image loads while the person steps through the folder faster
// than the images decode (a held arrow key): a step that follows the previous
// one by less than rapidStepIntervalMs is deferred, and settled() is emitted
// once no step has come for settleDelayMs. A single step loads at once.
//
// GUI thread only.
class NavigationThrottle final : public QObject {
    Q_OBJECT
public:
    // Steps of a held key come at the keyboard's repeat interval, give or
    // take the hardware and a busy event loop; twice the interval tells them
    // from a pause.
    static constexpr int kRepeatIntervalMargin = 2;
    // The interval of the fastest Windows repeat rate (30 per second).
    static constexpr int kFastestRepeatIntervalMs = 33;

    struct Timing {
        int rapidStepIntervalMs = kRepeatIntervalMargin * kFastestRepeatIntervalMs;
        int settleDelayMs = kRepeatIntervalMargin * kFastestRepeatIntervalMs;

        // Steps that come about as often as a key repeats at repeatIntervalMs
        // are rapid.
        [[nodiscard]] static Timing forRepeatInterval(int repeatIntervalMs);
    };

    explicit NavigationThrottle(QObject *parent = nullptr);
    explicit NavigationThrottle(Timing timing, QObject *parent = nullptr);

    // Takes effect from the next step; a pending settled() keeps its delay.
    void setTiming(Timing timing);

    // Records a step; true when its image should load now, false when the
    // load waits for settled().
    [[nodiscard]] bool admit();
    // Drops the pending settled() (the image was loaded some other way).
    void cancel();
    [[nodiscard]] bool isPending() const;

signals:
    void settled();

private:
    Timing timing;
    QElapsedTimer sinceLastStep;
    QTimer settleTimer;
};
