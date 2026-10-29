#include "shortcuteditormodel.h"

#include <QCoreApplication>

#include <algorithm>

#include "settingsstores.h"
#include "shortcuttablemodel.h"

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

QStringList ShortcutEditorModel::actions() const {
    return mActions;
}

QStringList ShortcutEditorModel::actionTitles() const {
    return mActionTitles;
}

QStringList ShortcutEditorModel::scripts() const {
    return mScripts;
}

bool ShortcutEditorModel::isScriptSelected() const {
    return mScriptSelected;
}

void ShortcutEditorModel::setScriptSelected(bool selected) {
    if (mScriptSelected == selected)
        return;
    mScriptSelected = selected;
    emit selectionChanged();
    emit shortcutChanged();
}

int ShortcutEditorModel::actionIndex() const {
    return mActionIndex;
}

void ShortcutEditorModel::setActionIndex(int index) {
    if (index < 0 || index >= mActions.size() || index == mActionIndex)
        return;
    mActionIndex = index;
    emit selectionChanged();
}

int ShortcutEditorModel::scriptIndex() const {
    return mScriptIndex;
}

void ShortcutEditorModel::setScriptIndex(int index) {
    if (index < 0 || index >= mScripts.size() || index == mScriptIndex)
        return;
    mScriptIndex = index;
    emit selectionChanged();
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
    if (mShortcut.isEmpty())
        return false;
    return mScriptSelected ? !mScripts.isEmpty() : !mActions.isEmpty();
}

void ShortcutEditorModel::startAdd(const ShortcutList &table) {
    startRequest(QCoreApplication::translate(kCreatorContext, QT_TRANSLATE_NOOP("ShortcutCreatorDialog", "Add shortcut")), -1,
                 table);
    open();
}

void ShortcutEditorModel::startEdit(const ShortcutEntry &entry, int row, const ShortcutList &table) {
    startRequest(QCoreApplication::translate(kSettingsContext, QT_TRANSLATE_NOOP("SettingsDialog", "Edit shortcut")), row,
                 table);
    if (entry.action.startsWith(kScriptActionPrefix)) {
        mScriptSelected = true;
        const qsizetype index = mScripts.indexOf(entry.action.mid(kScriptActionPrefix.size()));
        if (index >= 0)
            mScriptIndex = static_cast<int>(index);
    } else {
        const qsizetype index = mActions.indexOf(entry.action);
        if (index >= 0)
            mActionIndex = static_cast<int>(index);
    }
    // The shortcut being edited is not a conflict.
    mShortcut = entry.shortcut;
    emit selectionChanged();
    emit shortcutChanged();
    open();
}

void ShortcutEditorModel::startRequest(const QString &title, int row, const ShortcutList &table) {
    mTable = table;
    mTitle = title;
    mTitles = mStore.actionTitles();
    mActions = mTitles.keys();
    std::ranges::sort(mActions, [this](const QString &a, const QString &b) {
        return mTitles.value(a).localeAwareCompare(mTitles.value(b)) < 0;
    });
    mActionTitles.clear();
    for (const QString &action : std::as_const(mActions))
        mActionTitles.append(mTitles.value(action));
    mScripts = mStore.scriptNames();
    mScriptSelected = false;
    mActionIndex = 0;
    mScriptIndex = 0;
    mShortcut.clear();
    mWarning.clear();
    mEditedRow = row;
    emit requestChanged();
    emit selectionChanged();
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
                       .arg(shortcutActionTitle(action, mTitles));
    }
    emit shortcutChanged();
}

QString ShortcutEditorModel::boundAction(const QString &shortcut) const {
    for (qsizetype row = 0; row < mTable.size(); ++row) {
        if (row != mEditedRow && mTable.at(row).shortcut == shortcut)
            return mTable.at(row).action;
    }
    return {};
}

ShortcutEntry ShortcutEditorModel::result() const {
    QString action;
    if (mScriptSelected)
        action = kScriptActionPrefix.toString() + mScripts.value(mScriptIndex);
    else
        action = mActions.value(mActionIndex);
    return {.action = action, .shortcut = mShortcut};
}

int ShortcutEditorModel::editedRow() const {
    return mEditedRow;
}
