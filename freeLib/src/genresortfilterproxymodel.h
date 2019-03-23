#ifndef GENRESORTFILTERPROXYMODEL_H
#define GENRESORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>

class GenreSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    GenreSortFilterProxyModel(QObject *parent);
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
};

#endif // GENRESORTFILTERPROXYMODEL_H
