#pragma once

#include <QElapsedTimer>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QQuickItem>
#include <QtQml/qqmlregistration.h>

class QWheelEvent;

// Scrolls a Flickable in a dialog window (a window with a transient parent:
// the settings window and every dialog) by the mouse wheel, smoothly and on
// the frames of that window only.
//
// Flickable animates its wheel scroll on the global animation driver. While
// a second window is exposed, the threaded render loop advances that driver
// from a system timer instead of vsync, which steps unevenly against the
// display and renders the main window along. This helper takes the vertical
// wheel turns before the Flickable sees them and moves contentY towards an
// accumulated target from the window's afterAnimating(), requesting the next
// frame of that window only, so no animation of the QML engine runs.
//
// As Flickable with StopAtBounds: one notch scrolls
// QStyleHints::wheelScrollLines() lines of kWheelLineHeight pixels, a pixel
// delta scrolls at once, the target stays within the content, and a turn that
// cannot move the content goes on to the items under the view. Horizontal
// turns, and every turn in a window without a transient parent (the main
// window), reach the Flickable unchanged. Dragging the content, the scroll
// bar, the keys and any other change of contentY stop the scroll.
//
// GUI thread (created by the QML engine).
class DialogWheelScroller : public QObject {
  Q_OBJECT
  QML_ELEMENT
  // The Flickable to scroll: a ListView, or the contentItem of a ScrollView.
  Q_PROPERTY(QQuickItem *flickable READ flickable WRITE setFlickable NOTIFY
                 flickableChanged FINAL)

public:
  explicit DialogWheelScroller(QObject *parent = nullptr);

  QQuickItem *flickable() const;
  void setFlickable(QQuickItem *flickable);

signals:
  void flickableChanged();

protected:
  bool eventFilter(QObject *watched, QEvent *event) override;

private:
  // The contentY values the Flickable shows its content within.
  struct VerticalRange {
    qreal top = 0.0;
    qreal bottom = 0.0;
  };

  // Whether the turn was taken here; an ignored one goes on to the items
  // under the view.
  bool handleWheel(QWheelEvent *event);
  VerticalRange verticalRange() const;
  qreal contentY() const;
  void setContentY(qreal y);
  void scrollTo(qreal target);
  // One frame of the scroll.
  void advance();
  void stop();
  bool scrolling() const;

  QPointer<QQuickItem> mFlickable;
  QMetaObject::Connection mFrameConnection;
  QMetaObject::Connection mWindowConnection;
  QElapsedTimer mClock;
  qreal mFrom = 0.0;
  qreal mTarget = 0.0;
  // contentY as last set here: another value means something else moved
  // the content.
  qreal mSetY = 0.0;
};
