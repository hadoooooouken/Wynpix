#include "shortcuteditormodel.h"

#include <QCoreApplication>

#include "actionshortcutmodel.h"
#include "settingsstores.h"

namespace {
// The texts keep the translation contexts of the widget dialogs.
constexpr char kCreatorContext[] = "ShortcutCreatorDialog";
constexpr char kSettingsContext[] = "SettingsDialog";
} // namespace

ShortcutEditorModel::ShortcutEditorModel(IShortcutScriptStore &store, QObject *parent)
    : EditorSession(parent), mStore(store) {}

QString ShortcutEditorModel::title() const {
    return mTitle;
}

QString ShortcutEditorModel::actionTitle() const {
    return shortcutActionInfo(mAction, mInfos).title;
}

QString ShortcutEditorModel::categoryTitle() const {
    return shortcutActionInfo(mAction, mInfos).category;
}

QString ShortcutEditorModel::shortcut() const {
    return mShortcut;
}

QString ShortcutEditorModel::shortcutText() const {
    if (mShortcut.isEmpty())
        return QCoreApplication::translate(kCreatorContext, QT_TRANSLATE_NOOP("ShortcutCreatorDialog", "[Enter shortcut]"));
    return mShortcut;
}

QString ShortcutEditorModel::warning() const {
    return mWarning;
}

bool ShortcutEditorModel::canAccept() const {
    return acceptable();
}

bool ShortcutEditorModel::acceptable() const {
    return !mAction.isEmpty() && !mShortcut.isEmpty();
}

void ShortcutEditorModel::startAdd(const QString &action, const ShortcutList &table) {
    startRequest(QCoreApplication::translate(kCreatorContext, QT_TRANSLATE_NOOP("ShortcutCreatorDialog", "Add shortcut")),
                 action, table);
    open();
}

void ShortcutEditorModel::startEdit(const ShortcutEntry &entry, const ShortcutList &table) {
    startRequest(QCoreApplication::translate(kSettingsContext, QT_TRANSLATE_NOOP("SettingsDialog", "Edit shortcut")),
                 entry.action, table);
    // The shortcut being edited is not a conflict.
    mEdited = entry;
    mShortcut = entry.shortcut;
    emit shortcutChanged();
    open();
}

void ShortcutEditorModel::startRequest(const QString &title, const QString &action, const ShortcutList &table) {
    mTable = table;
    mTitle = title;
    mInfos = mStore.actionInfos();
    mAction = action;
    mShortcut.clear();
    mWarning.clear();
    mEdited.reset();
    emit requestChanged();
    emit shortcutChanged();
}

void ShortcutEditorModel::setShortcut(const QString &shortcut) {
    if (shortcut.isEmpty())
        return;
    mShortcut = shortcut;
    const QString action = boundAction(shortcut);
    if (action.isEmpty()) {
        mWarning.clear();
    } else {
        // One string, so that a translation can place the action anywhere.
        mWarning = QCoreApplication::translate(kCreatorContext, QT_TRANSLATE_NOOP("ShortcutCreatorDialog",
                                                                                  "This shortcut is used for action: %1. Replace?"))
                       .arg(shortcutActionInfo(action, mInfos).title);
    }
    emit shortcutChanged();
}

QString ShortcutEditorModel::boundAction(const QString &shortcut) const {
    for (const ShortcutEntry &entry : mTable) {
        if (entry.shortcut == shortcut && entry != mEdited)
            return entry.action;
    }
    return {};
}

ShortcutEntry ShortcutEditorModel::result() const {
    return {.action = mAction, .shortcut = mShortcut};
}

std::optional<ShortcutEntry> ShortcutEditorModel::edited() const {
    return mEdited;
}
