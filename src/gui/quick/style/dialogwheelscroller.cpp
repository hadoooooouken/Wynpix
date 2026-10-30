#include "dialogwheelscroller.h"

#include <QEasingCurve>
#include <QGuiApplication>
#include <QQuickWindow>
#include <QStyleHints>
#include <QWheelEvent>

#include <algorithm>
#include <chrono>
#include <cmath>

namespace {

using namespace std::chrono_literals;

// Pixels per wheel line, as Flickable scrolls.
constexpr qreal kWheelLineHeight = 24.0;
constexpr std::chrono::duration<double> kScrollDuration = 200ms;
constexpr QEasingCurve::Type kScrollEasing = QEasingCurve::OutCubic;
// Closer than this, two content positions are the same.
constexpr qreal kPositionTolerance = 0.5;

} // namespace

//------------------------------------------------------------------------------
DialogWheelScroller::DialogWheelScroller(QObject *parent) : QObject(parent) {}

//------------------------------------------------------------------------------
QQuickItem *DialogWheelScroller::flickable() const { return mFlickable; }

//------------------------------------------------------------------------------
void DialogWheelScroller::setFlickable(QQuickItem *flickable) {
  if (mFlickable == flickable)
    return;
  stop();
  if (mFlickable) {
    mFlickable->removeEventFilter(this);
    disconnect(mWindowConnection);
  }
  mFlickable = flickable;
  if (mFlickable) {
    mFlickable->installEventFilter(this);
    mWindowConnection = connect(mFlickable, &QQuickItem::windowChanged, this,
                                &DialogWheelScroller::stop);
  }
  emit flickableChanged();
}

//------------------------------------------------------------------------------
bool DialogWheelScroller::eventFilter(QObject *watched, QEvent *event) {
  if (watched == mFlickable && event->type() == QEvent::Wheel)
    return handleWheel(static_cast<QWheelEvent *>(event));
  return QObject::eventFilter(watched, event);
}

//------------------------------------------------------------------------------
bool DialogWheelScroller::handleWheel(QWheelEvent *event) {
  const QQuickWindow *window = mFlickable->window();
  if (!window || !window->transientParent() ||
      !mFlickable->property("interactive").toBool() ||
      mFlickable->property("dragging").toBool())
    return false;

  // As Flickable: the pixel delta only from a device that reports scroll
  // phases, the angle delta otherwise.
  const bool usesAngle = event->source() == Qt::MouseEventNotSynthesized ||
                         event->pixelDelta().isNull() ||
                         event->phase() == Qt::NoScrollPhase;
  const qreal notchHeight =
      QGuiApplication::styleHints()->wheelScrollLines() * kWheelLineHeight;
  const qreal delta =
      usesAngle ? -event->angleDelta().y() /
                      qreal(QWheelEvent::DefaultDeltasPerStep) * notchHeight
                : -event->pixelDelta().y();
  // A horizontal turn or a bare scroll phase is the Flickable's own.
  if (qFuzzyIsNull(delta))
    return false;

  const VerticalRange range = verticalRange();
  const qreal from = scrolling() ? mTarget : contentY();
  const qreal target = std::clamp(from + delta, range.top, range.bottom);
  if (std::abs(target - from) < kPositionTolerance) {
    event->ignore();
    return true;
  }
  if (usesAngle) {
    scrollTo(target);
  } else {
    stop();
    setContentY(target);
  }
  event->accept();
  return true;
}

//------------------------------------------------------------------------------
DialogWheelScroller::VerticalRange DialogWheelScroller::verticalRange() const {
  const qreal originY = mFlickable->property("originY").toReal();
  const qreal top = originY - mFlickable->property("topMargin").toReal();
  const qreal bottom = originY +
                       mFlickable->property("contentHeight").toReal() +
                       mFlickable->property("bottomMargin").toReal() -
                       mFlickable->height();
  return {top, std::max(top, bottom)};
}

//------------------------------------------------------------------------------
qreal DialogWheelScroller::contentY() const {
  return mFlickable->property("contentY").toReal();
}

//------------------------------------------------------------------------------
void DialogWheelScroller::setContentY(qreal y) {
  mFlickable->setProperty("contentY", y);
  mSetY = contentY();
}

//------------------------------------------------------------------------------
void DialogWheelScroller::scrollTo(qreal target) {
  QQuickWindow *window = mFlickable->window();
  mFrom = contentY();
  mSetY = mFrom;
  mTarget = target;
  mClock.start();
  if (!scrolling())
    mFrameConnection = connect(window, &QQuickWindow::afterAnimating, this,
                               &DialogWheelScroller::advance);
  window->update();
}

//------------------------------------------------------------------------------
void DialogWheelScroller::advance() {
  if (!mFlickable || !mFlickable->window() ||
      mFlickable->property("dragging").toBool() ||
      std::abs(contentY() - mSetY) >= kPositionTolerance) {
    stop();
    return;
  }
  // The content may have changed size since the turn.
  const VerticalRange range = verticalRange();
  mTarget = std::clamp(mTarget, range.top, range.bottom);
  const double progress = std::min(
      1.0, std::chrono::duration<double>(mClock.durationElapsed()) /
               kScrollDuration);
  const qreal eased = QEasingCurve(kScrollEasing).valueForProgress(progress);
  setContentY(mFrom + (mTarget - mFrom) * eased);
  if (progress < 1.0)
    mFlickable->window()->update();
  else
    stop();
}

//------------------------------------------------------------------------------
void DialogWheelScroller::stop() {
  disconnect(mFrameConnection);
  mFrameConnection = {};
}

//------------------------------------------------------------------------------
bool DialogWheelScroller::scrolling() const {
  return static_cast<bool>(mFrameConnection);
}
