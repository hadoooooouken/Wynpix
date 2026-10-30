#include "shortcutmap.h"

#include <QSet>

#include "utils/actions.h"
#include "utils/inputmap.h"

namespace {
// Whether shortcut runs an action of shortcuts in a mode that action works in.
bool shortcutTaken(const ShortcutMap &shortcuts, const QString &shortcut,
                   const QString &action) {
  const ActionCategory category = Actions::getInstance()->category(action);
  for (auto it = shortcuts.constFind(shortcut);
       it != shortcuts.cend() && it.key() == shortcut; ++it) {
    if (Actions::categoriesOverlap(Actions::getInstance()->category(it.value()),
                                   category))
      return true;
  }
  return false;
}

// The action of shortcuts bound to shortcut that works in the folder view
// (folderView) or in the viewer; a numpad key without one runs that of its
// main keyboard key.
QString boundAction(const ShortcutMap &shortcuts, const QString &shortcut,
                    bool folderView) {
  for (auto it = shortcuts.constFind(shortcut);
       it != shortcuts.cend() && it.key() == shortcut; ++it) {
    if (Actions::worksIn(Actions::getInstance()->category(it.value()), folderView))
      return it.value();
  }
  const QString mainKey = InputMap::mainKeyShortcut(shortcut);
  return mainKey.isEmpty() ? QString() : boundAction(shortcuts, mainKey, folderView);
}
} // namespace

namespace ShortcutRules {
bool mergeMissingDefaults(const ShortcutMap &defaults, ShortcutMap &shortcuts) {
  bool changed = false;
  QSet<QString> boundActions;
  boundActions.reserve(shortcuts.size());
  for (auto it = shortcuts.cbegin(); it != shortcuts.cend(); ++it)
    boundActions.insert(it.value());

  for (auto it = defaults.cbegin(); it != defaults.cend(); ++it) {
    if (!boundActions.contains(it.value()) &&
        !shortcutTaken(shortcuts, it.key(), it.value())) {
      shortcuts.insert(it.key(), it.value());
      changed = true;
    }
  }
  return changed;
}

bool removeRetiredDefaults(int version, ShortcutMap &shortcuts) {
  // Since then Enter opens the selected folder view item, not fullscreen.
  constexpr int kEnterOpensSelectionVersion = 3;
  if (version >= kEnterOpensSelectionVersion)
    return false;
  return shortcuts.remove("Enter", "toggleFullscreen") > 0;
}

QString actionFor(const ShortcutMap &shortcuts, const QString &shortcut, bool folderView) {
  const QString bound = boundAction(shortcuts, shortcut, folderView);
  if (!bound.isEmpty())
    return bound;
  const QString shiftPrefix = InputMap::keyNameShift() + "+";
  if (!folderView || !shortcut.contains(shiftPrefix))
    return {};
  const QString action =
      actionFor(shortcuts, QString(shortcut).remove(shiftPrefix), folderView);
  return Actions::getInstance()->extendsWithShift(action) ? action : QString();
}
} // namespace ShortcutRules
