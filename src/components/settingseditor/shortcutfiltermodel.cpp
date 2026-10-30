#include "shortcutfiltermodel.h"

#include <algorithm>

#include "actionshortcutmodel.h"

ShortcutFilterModel::ShortcutFilterModel(QObject *parent) : QSortFilterProxyModel(parent) {}

QString ShortcutFilterModel::filterText() const {
    return mText;
}

void ShortcutFilterModel::setFilterText(const QString &text) {
    const QString trimmed = text.trimmed();
    if (trimmed == mText)
        return;
    beginFilterChange();
    mText = trimmed;
    endFilterChange(Direction::Rows);
}

bool ShortcutFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
    if (mText.isEmpty())
        return true;
    const QModelIndex row = sourceModel()->index(sourceRow, 0, sourceParent);
    const auto matches = [this](const QString &text) {
        return text.contains(mText, Qt::CaseInsensitive);
    };
    if (matches(row.data(ActionShortcutModel::TitleRole).toString())
        || matches(row.data(ActionShortcutModel::CategoryRole).toString()))
        return true;
    return std::ranges::any_of(row.data(ActionShortcutModel::ShortcutsRole).toStringList(), matches)
        || std::ranges::any_of(row.data(ActionShortcutModel::ShortcutTextsRole).toStringList(), matches);
}
