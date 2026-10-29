#include "shortcuttablemodel.h"

#include <QCoreApplication>

#include <algorithm>

namespace {
constexpr char kContext[] = "SettingsDialog";
} // namespace

QString shortcutActionTitle(const QString &action, const ActionTitleMap &titles) {
    if (action.startsWith(kScriptActionPrefix)) {
        return QCoreApplication::translate(kContext, QT_TRANSLATE_NOOP("SettingsDialog", "Script: %1"))
            .arg(action.mid(kScriptActionPrefix.size()));
    }
    return titles.value(action, action);
}

ShortcutTableModel::ShortcutTableModel(QObject *parent) : QAbstractTableModel(parent) {}

int ShortcutTableModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : static_cast<int>(mEntries.size());
}

int ShortcutTableModel::columnCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant ShortcutTableModel::data(const QModelIndex &index, int role) const {
    if (!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid))
        return {};
    const ShortcutEntry &entry = mEntries.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
        return index.column() == ActionColumn ? title(entry.action) : entry.shortcut;
    case Qt::TextAlignmentRole:
        return QVariant::fromValue(Qt::Alignment(Qt::AlignCenter));
    case ActionRole:
        return entry.action;
    case TitleRole:
        return title(entry.action);
    case ShortcutRole:
        return entry.shortcut;
    default:
        return {};
    }
}

QVariant ShortcutTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QAbstractTableModel::headerData(section, orientation, role);
    if (section == ActionColumn)
        return QCoreApplication::translate(kContext, QT_TRANSLATE_NOOP("SettingsDialog", "Action"));
    if (section == ShortcutColumn)
        return QCoreApplication::translate(kContext, QT_TRANSLATE_NOOP("SettingsDialog", "Shortcut"));
    return {};
}

QHash<int, QByteArray> ShortcutTableModel::roleNames() const {
    return {{ActionRole, "action"}, {TitleRole, "title"}, {ShortcutRole, "shortcut"}};
}

void ShortcutTableModel::setActionTitles(const ActionTitleMap &titles) {
    beginResetModel();
    mTitles = titles;
    sortEntries();
    endResetModel();
}

const ShortcutList &ShortcutTableModel::entries() const {
    return mEntries;
}

void ShortcutTableModel::setEntries(const ShortcutList &entries) {
    beginResetModel();
    mEntries = entries;
    sortEntries();
    endResetModel();
}

std::optional<ShortcutEntry> ShortcutTableModel::entryAt(int row) const {
    if (row < 0 || row >= mEntries.size())
        return std::nullopt;
    return mEntries.at(row);
}

int ShortcutTableModel::put(const ShortcutEntry &entry, int replacedRow) {
    if (entry.action.isEmpty() || entry.shortcut.isEmpty())
        return -1;
    removeAt(replacedRow);
    const auto bound = std::ranges::find(mEntries, entry.shortcut, &ShortcutEntry::shortcut);
    if (bound != mEntries.cend())
        removeAt(static_cast<int>(std::distance(mEntries.begin(), bound)));
    const int row = insertionRow(entry);
    beginInsertRows({}, row, row);
    mEntries.insert(row, entry);
    endInsertRows();
    return row;
}

bool ShortcutTableModel::removeAt(int row) {
    if (row < 0 || row >= mEntries.size())
        return false;
    beginRemoveRows({}, row, row);
    mEntries.removeAt(row);
    endRemoveRows();
    return true;
}

QString ShortcutTableModel::title(const QString &action) const {
    return shortcutActionTitle(action, mTitles);
}

bool ShortcutTableModel::titleLess(const ShortcutEntry &a, const ShortcutEntry &b) const {
    return title(a.action).localeAwareCompare(title(b.action)) < 0;
}

void ShortcutTableModel::sortEntries() {
    std::ranges::stable_sort(mEntries, [this](const ShortcutEntry &a, const ShortcutEntry &b) {
        return titleLess(a, b);
    });
}

int ShortcutTableModel::insertionRow(const ShortcutEntry &entry) const {
    const auto position = std::ranges::upper_bound(
        mEntries, entry, [this](const ShortcutEntry &a, const ShortcutEntry &b) { return titleLess(a, b); });
    return static_cast<int>(std::distance(mEntries.cbegin(), position));
}
