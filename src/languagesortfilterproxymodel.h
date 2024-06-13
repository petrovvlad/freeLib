#ifndef LANGUAGESORTFILTERPROXYMODEL_H
#define LANGUAGESORTFILTERPROXYMODEL_H

#include <QObject>
#include <QSortFilterProxyModel>

class LanguageSortFilterProxyModel : public QSortFilterProxyModel
{
public:
    explicit LanguageSortFilterProxyModel(QObject *parent = nullptr);
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
};

#endif // LANGUAGESORTFILTERPROXYMODEL_H
