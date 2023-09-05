#include "genresortfilterproxymodel.h"

#include "library.h"

GenreSortFilterProxyModel::GenreSortFilterProxyModel(QObject *parent): QSortFilterProxyModel(parent)
{
}

bool GenreSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    ushort idLeft = sourceModel()->data(left, Qt::UserRole).toUInt();
    ushort idRight = sourceModel()->data(right, Qt::UserRole).toUInt();

    if(idRight == 0)
        return false;
    if(idLeft == 0)
        return true;

    if(genres[idLeft].idParrentGenre == 0 && genres[idRight].idParrentGenre == 0)
        return idLeft < idRight;

    if(genres[idLeft].idParrentGenre == 0 && genres[idRight].idParrentGenre > 0)
        return idLeft <= genres[idRight].idParrentGenre;

    if(genres[idLeft].idParrentGenre > 0 && genres[idRight].idParrentGenre == 0)
        return idRight>genres[idLeft].idParrentGenre;

    if(genres[idLeft].idParrentGenre > 0 && genres[idRight].idParrentGenre > 0){
        if(genres[idLeft].idParrentGenre == genres[idRight].idParrentGenre)
            return genres[idLeft].nSort < genres[idRight].nSort;
        return genres[idLeft].idParrentGenre < genres[idRight].idParrentGenre;
    }

    return idLeft < idRight;
}
