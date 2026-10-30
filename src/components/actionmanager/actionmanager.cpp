#include "actionmanager.h"
#include "components/scriptmanager/scriptmanager.h"
#include "settings.h"
#include "shortcutbuilder.h"

ActionManager *actionManager = nullptr;

ActionManager::ActionManager(QObject *parent) : QObject(parent) {}
//------------------------------------------------------------------------------
ActionManager::~ActionManager() {
  if (actionManager == this) {
    actionManager = nullptr;
  }
}
//------------------------------------------------------------------------------
ActionManager *ActionManager::getInstance() {
  if (!actionManager) {
    actionManager = new ActionManager();
    initDefaults();
    initShortcuts();
  }
  return actionManager;
}
//------------------------------------------------------------------------------
void ActionManager::initDefaults() {
  actionManager->defaults.insert("Right", "nextImage");
  actionManager->defaults.insert("Left", "prevImage");
  actionManager->defaults.insert("XButton2", "nextImage");
  actionManager->defaults.insert("XButton1", "prevImage");
  actionManager->defaults.insert("WheelDown", "zoomOut");
  actionManager->defaults.insert("WheelUp", "zoomIn");
  actionManager->defaults.insert("F", "toggleFullscreen");
  actionManager->defaults.insert("LMB_DoubleClick", "fitNormal");
  actionManager->defaults.insert("MiddleButton", "folderView");
  actionManager->defaults.insert("Backspace", "folderView");
  actionManager->defaults.insert("Esc", "folderView");
  actionManager->defaults.insert("Space", "toggleFitMode");
  actionManager->defaults.insert("1", "fitWindow");
  actionManager->defaults.insert("2", "fitWidth");
  actionManager->defaults.insert("3", "fitNormal");
  actionManager->defaults.insert("4", "fitHeight");
  actionManager->defaults.insert("R", "resize");
  actionManager->defaults.insert("H", "flipH");
  actionManager->defaults.insert("V", "flipV");
  actionManager->defaults.insert(InputMap::keyNameCtrl() + "+R", "rotateRight");
  actionManager->defaults.insert(InputMap::keyNameCtrl() + "+L", "rotateLeft");
  actionManager->defaults.insert(",", "prevPage");
  actionManager->defaults.insert(".", "nextPage");
  actionManager->defaults.insert(InputMap::keyNameCtrl() + "+,", "firstPage");
  actionManager->defaults.insert(InputMap::keyNameCtrl() + "+.", "lastPage");
  actionManager->defaults.insert(InputMap::keyNameCtrl() + "+WheelUp",
                                 "zoomInCursor");
  actionManager->defaults.insert(InputMap::keyNameCtrl() + "+WheelDown",
                                 "zoomOutCursor");
  actionManager->defaults.insert("=", "zoomIn");
  actionManager->defaults.insert("+", "zoomIn");
  actionManager->defaults.insert("-", "zoomOut");
  actionManager->defaults.insert(InputMap::keyNameCtrl() + "+Down", "zoomOut");
  actionManager->defaults.insert(InputMap::keyNameCtrl() + "+Up", "zoomIn");
  actionManager->defaults.insert("Up", "fitNormal");
  actionManager->defaults.insert("Down", "fitWindow");
  actionManager->defaults.insert(InputMap::keyNameCtrl() + "+S", "save");
  actionManager->defaults.insert(InputMap::keyNameCtrl() + "+" +
                                     InputMap::keyNameShift() + "+S",
                                 "saveAs");
  actionManager->defaults.insert(InputMap::keyNameCtrl() + "+W",
                                 "setWallpaper");
  actionManager->defaults.insert("X", "crop");
  actionManager->defaults.insert(InputMap::keyNameCtrl() + "+P", "print");
  actionManager->defaults.insert(InputMap::keyNameAlt() + "+X", "exit");
  actionManager->defaults.insert(InputMap::keyNameCtrl() + "+Q", "exit");
  actionManager->defaults.insert("Del", "moveToTrash");
  actionManager->defaults.insert(InputMap::keyNameShift() + "+Del",
                                 "removeFile");
  actionManager->defaults.insert("C", "copyFile");
  actionManager->defaults.insert("M", "moveFile");
  actionManager->defaults.insert("Home", "jumpToFirst");
  actionManager->defaults.insert("End", "jumpToLast");
  actionManager->defaults.insert("F5", "reloadImage");
  actionManager->defaults.insert(InputMap::keyNameCtrl() + "+C",
                                 "copyFileClipboard");
  actionManager->defaults.insert(InputMap::keyNameShift() + "+C",
                                 "copyViewportClipboard");
  actionManager->defaults.insert(InputMap::keyNameCtrl() + "+" +
                                     InputMap::keyNameShift() + "+C",
                                 "copyPathClipboard");
  actionManager->defaults.insert("F2", "renameFile");
  actionManager->defaults.insert("RMB", "contextMenu");
  actionManager->defaults.insert("Menu", "contextMenu");
  actionManager->defaults.insert("I", "toggleImageInfo");
  actionManager->defaults.insert(InputMap::keyNameCtrl() + "+`",
                                 "toggleShuffle");
  actionManager->defaults.insert(InputMap::keyNameCtrl() + "+D",
                                 "showInDirectory");
  actionManager->defaults.insert("`", "toggleSlideshow");
  actionManager->defaults.insert(InputMap::keyNameCtrl() + "+Z",
                                 "discardEdits");
  actionManager->defaults.insert(InputMap::keyNameShift() + "+Right",
                                 "nextDirectory");
  actionManager->defaults.insert(InputMap::keyNameShift() + "+Left",
                                 "prevDirectory");
  actionManager->defaults.insert(InputMap::keyNameShift() + "+F",
                                 "toggleFullscreenInfoBar");
  actionManager->defaults.insert(InputMap::keyNameCtrl() + "+V", "pasteFile");
  actionManager->defaults.insert("P", "openSettings");
  actionManager->defaults.insert("N", "toggleScalingFilter");
  actionManager->defaults.insert(InputMap::keyNameShift() + "+N", "cycleScalingFilter");
  actionManager->defaults.insert(InputMap::keyNameShift() + "+P",
                                 "togglePanorama");
  actionManager->defaults.insert(InputMap::keyNameAlt() + "+I", "toggleUpscayl");
  actionManager->defaults.insert(InputMap::keyNameAlt() + "+" + InputMap::keyNameShift() + "+I", "cycleUpscaylModel");
  actionManager->defaults.insert(InputMap::keyNameAlt() + "+H", "toggleHdrToneMapping");
  actionManager->defaults.insert(InputMap::keyNameCtrl() + "+F", "focusNameFilter");
  // The folder view; Shift with a selection move extends the selection.
  actionManager->defaults.insert("Enter", "folderOpenSelected");
  actionManager->defaults.insert("XButton1", "folderBack");
  actionManager->defaults.insert("XButton2", "folderForward");
  actionManager->defaults.insert("F", "folderToggleFullscreen");
  actionManager->defaults.insert("Backspace", "goUp");
  actionManager->defaults.insert(InputMap::keyNameCtrl() + "+A", "folderSelectAll");
  actionManager->defaults.insert("Left", "folderMoveLeft");
  actionManager->defaults.insert("Right", "folderMoveRight");
  actionManager->defaults.insert("Up", "folderMoveUp");
  actionManager->defaults.insert("Down", "folderMoveDown");
  actionManager->defaults.insert("PgUp", "folderPageUp");
  actionManager->defaults.insert("PgDown", "folderPageDown");
  actionManager->defaults.insert("Home", "folderFirstItem");
  actionManager->defaults.insert("End", "folderLastItem");
}
//------------------------------------------------------------------------------
void ActionManager::initShortcuts() {
  actionManager->readShortcuts();
}
//------------------------------------------------------------------------------
void ActionManager::addShortcut(const QString &keys, const QString &action) {
  ActionType type = validateAction(action);
  if (type != ActionType::ACTION_INVALID &&
      !actionManager->shortcuts.contains(keys, action)) {
    actionManager->shortcuts.insert(keys, action);
  }
}
//------------------------------------------------------------------------------
void ActionManager::removeShortcut(const QString &keys) {
  actionManager->shortcuts.remove(keys);
}
//------------------------------------------------------------------------------
QStringList ActionManager::actionList() { return appActions->getList(); }
//------------------------------------------------------------------------------
QString ActionManager::actionTitle(const QString &action) {
  return appActions->title(action);
}

ActionCategory ActionManager::actionCategory(const QString &action) {
  return appActions->category(action);
}

ActionGroup ActionManager::actionGroup(const QString &action) {
  return appActions->group(action);
}
//------------------------------------------------------------------------------
const ShortcutMap &ActionManager::allShortcuts() {
  return actionManager->shortcuts;
}

const ShortcutMap &ActionManager::defaultShortcuts() {
  return actionManager->defaults;
}

void ActionManager::setFolderViewActive(bool active) {
  mFolderViewActive = active;
}
//------------------------------------------------------------------------------
void ActionManager::removeAllShortcuts() { shortcuts.clear(); }
//------------------------------------------------------------------------------
void ActionManager::removeAllShortcuts(QString actionName) {
  if (validateAction(actionName) == ActionType::ACTION_INVALID)
    return;

  for (auto i = shortcuts.begin(); i != shortcuts.end();) {
    if (i.value() == actionName)
      i = shortcuts.erase(i);
    else
      ++i;
  }
}
//------------------------------------------------------------------------------
QString ActionManager::keyForNativeScancode(quint32 scanCode) {
  if (inputMap->keys().contains(scanCode)) {
    return inputMap->keys()[scanCode];
  }
  return "";
}
//------------------------------------------------------------------------------
void ActionManager::resetDefaults() {
  actionManager->shortcuts = actionManager->defaults;
}
//------------------------------------------------------------------------------
void ActionManager::saveShortcuts() {
  settings->saveShortcuts(actionManager->shortcuts);
  settings->setShortcutsVersion(kCurrentShortcutsVersion);
}
//------------------------------------------------------------------------------
const QString ActionManager::shortcutForAction(QString action) {
  return shortcuts.key(action, "");
}

const QList<QString> ActionManager::shortcutsForAction(QString action) {
  return shortcuts.keys(action);
}
//------------------------------------------------------------------------------
bool ActionManager::invokeAction(const QString &actionName) {
  ActionType type = validateAction(actionName);
  if (type == ActionType::ACTION_NORMAL) {
    QMetaObject::invokeMethod(this, actionName.toLatin1().constData(),
                              Qt::DirectConnection);
    return true;
  } else if (type == ActionType::ACTION_SCRIPT) {
    QString scriptName = actionName;
    scriptName.remove(0, 2);
    emit runScript(scriptName);
    return true;
  }
  return false;
}
//------------------------------------------------------------------------------
bool ActionManager::invokeActionForShortcut(const QString &shortcut) {
  if (shortcut.isEmpty())
    return false;
  const QString action =
      ShortcutRules::actionFor(shortcuts, shortcut, mFolderViewActive);
  return !action.isEmpty() && invokeAction(action);
}
//------------------------------------------------------------------------------
void ActionManager::validateShortcuts() {
  for (auto i = shortcuts.begin(); i != shortcuts.end();) {
    if (validateAction(i.value()) == ActionType::ACTION_INVALID)
      i = shortcuts.erase(i);
    else
      ++i;
  }
}
//------------------------------------------------------------------------------
inline ActionType ActionManager::validateAction(const QString &actionName) {
  if (appActions->contains(actionName))
    return ActionType::ACTION_NORMAL;
  if (actionName.startsWith("s:")) {
    QString scriptName = actionName;
    scriptName.remove(0, 2);
    if (scriptManager->scriptExists(scriptName))
      return ActionType::ACTION_SCRIPT;
  }
  return ActionType::ACTION_INVALID;
}
//------------------------------------------------------------------------------
void ActionManager::readShortcuts() {
  settings->readShortcuts(shortcuts);
  const int storedVersion = settings->shortcutsVersion();

  if (shortcuts.isEmpty()) {
    resetDefaults();
    actionManager->saveShortcuts();
  } else if (storedVersion < kCurrentShortcutsVersion) {
    const bool removed = ShortcutRules::removeRetiredDefaults(storedVersion, shortcuts);
    if (ShortcutRules::mergeMissingDefaults(defaults, shortcuts) || removed) {
      actionManager->saveShortcuts();
    } else {
      settings->setShortcutsVersion(kCurrentShortcutsVersion);
    }
  }
  actionManager->validateShortcuts();
}
//------------------------------------------------------------------------------
bool ActionManager::processEvent(QInputEvent *event) {
  return actionManager->invokeActionForShortcut(
      ShortcutBuilder::fromEvent(event));
}
