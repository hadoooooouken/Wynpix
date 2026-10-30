#pragma once

#include <QEvent>
#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

class IActionDispatcher;

// Actions and shortcuts for QML (Actions singleton).
//
// QML runs actions by name and forwards unhandled input:
//   Keys.onPressed: (event) => event.accepted = Actions.handleKeyEvent(event)
//   MouseArea { onWheel: (wheel) => wheel.accepted = Actions.handleWheelEvent(wheel) }
//   MouseArea { onPressed: (mouse) => Actions.handleMousePress(mouse) }
// Each rebuilds the matching Qt event and passes it to the dispatcher, which
// is ActionManager::processEvent() in the application. Mouse buttons form
// shortcuts like the widget UI: presses of every button but the right one,
// the release of the right button, and left-button double clicks.
//
// GUI thread only.
class ActionBridge : public QObject {
  Q_OBJECT
  QML_NAMED_ELEMENT(Actions)
  QML_SINGLETON
  QML_UNCREATABLE("Provided by QuickUiHost via setExternalSingletonInstance()")

public:
  // dispatcher must outlive the bridge.
  explicit ActionBridge(IActionDispatcher &dispatcher,
                        QObject *parent = nullptr);

  // Runs the action called name; returns false when it is unknown.
  Q_INVOKABLE bool invoke(const QString &name);
  // Primary shortcut of the action called name, or an empty string.
  Q_INVOKABLE QString shortcutFor(const QString &name) const;
  // Forwards a QML KeyEvent; returns true when an action handled it.
  Q_INVOKABLE bool handleKeyEvent(QObject *event);
  // Forwards a QML WheelEvent; returns true when an action handled it.
  Q_INVOKABLE bool handleWheelEvent(QObject *event);
  // Forward a QML MouseEvent of a press, release or double click; return
  // true when an action handled it.
  Q_INVOKABLE bool handleMousePress(QObject *event);
  Q_INVOKABLE bool handleMouseRelease(QObject *event);
  Q_INVOKABLE bool handleMouseDoubleClick(QObject *event);
  // Shortcut text of a QML KeyEvent ("Ctrl+R"), as shortcuts are stored;
  // empty for events without a shortcut.
  Q_INVOKABLE QString shortcutText(QObject *event) const;
  // Layout-independent name of the key of a QML KeyEvent, without
  // modifiers ("3" for the key in the digit row).
  Q_INVOKABLE QString keyText(QObject *event) const;
  // Shortcut texts of a QML WheelEvent ("WheelUp") and of the press,
  // release or double click of a QML MouseEvent ("Ctrl+LMB",
  // "LMB_DoubleClick"), as shortcuts are stored; empty for input that forms
  // no shortcut (the press of the right button, the release of the others).
  Q_INVOKABLE QString wheelShortcutText(QObject *event) const;
  Q_INVOKABLE QString mousePressShortcutText(QObject *event) const;
  Q_INVOKABLE QString mouseReleaseShortcutText(QObject *event) const;
  Q_INVOKABLE QString mouseDoubleClickShortcutText(QObject *event) const;

private:
  [[nodiscard]] bool handleMouseEvent(QObject *event, QEvent::Type type);
  [[nodiscard]] QString mouseShortcutText(QObject *event, QEvent::Type type) const;

  IActionDispatcher &mDispatcher;
};
