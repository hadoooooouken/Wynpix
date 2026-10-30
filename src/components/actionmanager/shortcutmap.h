#pragma once

#include <QMultiMap>
#include <QString>

// <shortcut, action>. A shortcut has at most one action per mode (see
// Actions::categoriesOverlap()): one of the folder view and one of the viewer,
// or one common to both.
using ShortcutMap = QMultiMap<QString, QString>;

// The rules of the shortcut table, apart from ActionManager so that they need
// only the action list (Actions).
namespace ShortcutRules {
// Adds to shortcuts the defaults of the actions without a shortcut whose
// shortcut has no action of a common mode yet; true when one was added.
bool mergeMissingDefaults(const ShortcutMap &defaults, ShortcutMap &shortcuts);
// Removes from shortcuts stored before version the defaults that later
// versions dropped; true when one was removed.
bool removeRetiredDefaults(int version, ShortcutMap &shortcuts);
// The action that shortcut runs in the folder view (folderView) or in the
// viewer; empty for none. A numpad key without an action runs that of the
// main keyboard key of the same meaning ("Num7" that of "7", "NumHome" that
// of "Home"). In the folder view a shortcut with Shift runs the selection
// move of the shortcut without it (Actions::extendsWithShift()).
QString actionFor(const ShortcutMap &shortcuts, const QString &shortcut, bool folderView);
} // namespace ShortcutRules
