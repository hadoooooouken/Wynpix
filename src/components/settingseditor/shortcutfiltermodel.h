#pragma once

#include <QSortFilterProxyModel>

// The rows of an ActionShortcutModel that match the search of the
// "Controls" page: the text, ignoring case, is part of the row's title, its
// category or one of its shortcuts. An empty text matches every row. The
// order of the rows is kept.
//
// GUI thread only.
class ShortcutFilterModel final : public QSortFilterProxyModel {
    Q_OBJECT

public:
    explicit ShortcutFilterModel(QObject *parent = nullptr);

    [[nodiscard]] QString filterText() const;
    void setFilterText(const QString &text);

protected:
    [[nodiscard]] bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QString mText;
};
