#include "navigationthrottle.h"

NavigationThrottle::Timing
NavigationThrottle::Timing::forRepeatInterval(int repeatIntervalMs) {
    const int interval = kRepeatIntervalMargin * repeatIntervalMs;
    return Timing{.rapidStepIntervalMs = interval, .settleDelayMs = interval};
}

NavigationThrottle::NavigationThrottle(QObject *parent)
    : NavigationThrottle(Timing{}, parent) {}

NavigationThrottle::NavigationThrottle(Timing timing, QObject *parent)
    : QObject(parent) {
    settleTimer.setSingleShot(true);
    setTiming(timing);
    connect(&settleTimer, &QTimer::timeout, this, &NavigationThrottle::settled);
}

void NavigationThrottle::setTiming(Timing value) {
    timing = value;
    settleTimer.setInterval(timing.settleDelayMs);
}

bool NavigationThrottle::admit() {
    const bool rapid = sinceLastStep.isValid() &&
                       sinceLastStep.elapsed() < timing.rapidStepIntervalMs;
    sinceLastStep.start();
    if (!rapid && !settleTimer.isActive())
        return true;
    settleTimer.start();
    return false;
}

void NavigationThrottle::cancel() {
    settleTimer.stop();
}

bool NavigationThrottle::isPending() const {
    return settleTimer.isActive();
}
