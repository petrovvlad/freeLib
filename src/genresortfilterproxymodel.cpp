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

    const auto& genreLeft = g::genres[idLeft];
    const auto& genreRight = g::genres[idRight];
    if(g::genres[idLeft].idParrentGenre == 0 && g::genres[idRight].idParrentGenre == 0)
        return idLeft < idRight;

    if(genreLeft.idParrentGenre == 0 && genreRight.idParrentGenre > 0)
        return idLeft <= genreRight.idParrentGenre;

    if(genreLeft.idParrentGenre > 0 && genreRight.idParrentGenre == 0)
        return idRight>genreLeft.idParrentGenre;

    if(genreLeft.idParrentGenre > 0 && genreRight.idParrentGenre > 0){
        if(genreLeft.idParrentGenre == genreRight.idParrentGenre)
            return idLeft < idRight;
        return genreLeft.idParrentGenre < genreRight.idParrentGenre;
    }

    return idLeft < idRight;
}
