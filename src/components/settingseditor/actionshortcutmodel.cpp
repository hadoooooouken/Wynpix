#include "actionshortcutmodel.h"

#include <QCoreApplication>

#include <algorithm>
#include <limits>

namespace {
constexpr char kContext[] = "SettingsDialog";
// The script category follows every action category; unknown actions come
// last.
constexpr int kScriptCategoryOrder = std::numeric_limits<int>::max() - 1;
constexpr int kUnknownCategoryOrder = std::numeric_limits<int>::max();
} // namespace

ActionInfo shortcutActionInfo(const QString &action, const ActionInfoMap &actions) {
    if (action.startsWith(kScriptActionPrefix)) {
        return {.title = QCoreApplication::translate(kContext, QT_TRANSLATE_NOOP("SettingsDialog", "Script: %1"))
                             .arg(action.mid(kScriptActionPrefix.size())),
                .category = QCoreApplication::translate(kContext, QT_TRANSLATE_NOOP("SettingsDialog", "Scripts")),
                .categoryOrder = kScriptCategoryOrder};
    }
    const auto info = actions.constFind(action);
    if (info != actions.cend())
        return *info;
    return {.title = action,
            .category = QCoreApplication::translate(kContext, QT_TRANSLATE_NOOP("SettingsDialog", "Other")),
            .categoryOrder = kUnknownCategoryOrder};
}

ActionShortcutModel::ActionShortcutModel(QObject *parent) : QAbstractListModel(parent) {}

int ActionShortcutModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : static_cast<int>(mRows.size());
}

QVariant ActionShortcutModel::data(const QModelIndex &index, int role) const {
    if (!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid))
        return {};
    const Row &row = mRows.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case TitleRole:
        // The category already says that a script row is a script.
        if (row.action.startsWith(kScriptActionPrefix))
            return row.action.mid(kScriptActionPrefix.size());
        return row.info.title;
    case ActionRole:
        return row.action;
    case CategoryRole:
        return row.info.category;
    case ShortcutsRole:
        return row.shortcuts;
    default:
        return {};
    }
}

QHash<int, QByteArray> ActionShortcutModel::roleNames() const {
    return {{ActionRole, "action"},
            {TitleRole, "title"},
            {CategoryRole, "category"},
            {ShortcutsRole, "shortcuts"}};
}

void ActionShortcutModel::setActions(const ActionInfoMap &actions, const QStringList &scripts) {
    const ShortcutList bound = entries();
    mActions = actions;
    mScripts = scripts;
    rebuild(bound);
}

ShortcutList ActionShortcutModel::entries() const {
    ShortcutList list;
    for (const Row &row : mRows) {
        for (const QString &shortcut : row.shortcuts)
            list.append({.action = row.action, .shortcut = shortcut});
    }
    return list;
}

void ActionShortcutModel::setEntries(const ShortcutList &entries) {
    rebuild(entries);
}

int ActionShortcutModel::put(const ShortcutEntry &entry, const std::optional<ShortcutEntry> &replaced) {
    if (entry.action.isEmpty() || entry.shortcut.isEmpty())
        return -1;
    const int row = rowOf(entry.action);
    if (row < 0) {
        // An action without a row: rebuilt with it.
        ShortcutList list = entries();
        list.removeIf([&](const ShortcutEntry &e) {
            return e.shortcut == entry.shortcut || (replaced && e == *replaced);
        });
        list.append(entry);
        rebuild(list);
        return rowOf(entry.action);
    }

    qsizetype position = -1;
    if (replaced) {
        const int replacedRow = rowOf(replaced->action);
        const qsizetype taken = replacedRow >= 0 ? takeShortcut(replacedRow, replaced->shortcut) : -1;
        if (replacedRow == row)
            position = taken;
    }
    for (int other = 0; other < mRows.size(); ++other) {
        const qsizetype taken = takeShortcut(other, entry.shortcut);
        if (other == row && taken >= 0 && taken < position)
            --position;
    }
    QStringList &shortcuts = mRows[row].shortcuts;
    if (position >= 0)
        shortcuts.insert(position, entry.shortcut);
    else
        shortcuts.append(entry.shortcut);
    const QModelIndex changed = index(row);
    emit dataChanged(changed, changed, {ShortcutsRole});
    return row;
}

bool ActionShortcutModel::remove(const ShortcutEntry &entry) {
    const int row = rowOf(entry.action);
    return row >= 0 && takeShortcut(row, entry.shortcut) >= 0;
}

int ActionShortcutModel::rowOf(const QString &action) const {
    const auto found = std::ranges::find(mRows, action, &Row::action);
    return found == mRows.cend() ? -1 : static_cast<int>(std::distance(mRows.cbegin(), found));
}

void ActionShortcutModel::rebuild(const ShortcutList &entries) {
    beginResetModel();
    mRows.clear();
    QHash<QString, qsizetype> rowOfAction;
    const auto addRow = [&](const QString &action) {
        if (rowOfAction.contains(action))
            return;
        rowOfAction.insert(action, mRows.size());
        mRows.append({.action = action, .info = shortcutActionInfo(action, mActions), .shortcuts = {}});
    };
    for (auto it = mActions.cbegin(); it != mActions.cend(); ++it)
        addRow(it.key());
    for (const QString &script : std::as_const(mScripts))
        addRow(kScriptActionPrefix.toString() + script);
    for (const ShortcutEntry &entry : entries)
        addRow(entry.action);
    for (const ShortcutEntry &entry : entries)
        mRows[rowOfAction.value(entry.action)].shortcuts.append(entry.shortcut);

    std::ranges::sort(mRows, [](const Row &a, const Row &b) {
        if (a.info.categoryOrder != b.info.categoryOrder)
            return a.info.categoryOrder < b.info.categoryOrder;
        if (const int order = a.info.category.localeAwareCompare(b.info.category); order != 0)
            return order < 0;
        if (const int order = a.info.title.localeAwareCompare(b.info.title); order != 0)
            return order < 0;
        return a.action < b.action;
    });
    endResetModel();
}

qsizetype ActionShortcutModel::takeShortcut(int row, const QString &shortcut) {
    const qsizetype position = mRows.at(row).shortcuts.indexOf(shortcut);
    if (position < 0)
        return -1;
    mRows[row].shortcuts.removeAt(position);
    const QModelIndex changed = index(row);
    emit dataChanged(changed, changed, {ShortcutsRole});
    return position;
}
