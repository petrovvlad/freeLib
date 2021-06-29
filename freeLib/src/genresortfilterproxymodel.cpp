#include "common.h"
#include "genresortfilterproxymodel.h"
#include "library.h"

GenreSortFilterProxyModel::GenreSortFilterProxyModel(QObject *parent): QSortFilterProxyModel(parent)
{
}

bool GenreSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    uint idLeft = sourceModel()->data(left,Qt::UserRole).toUInt();
    uint idRight = sourceModel()->data(right,Qt::UserRole).toUInt();

    if(sourceModel()->data(right).toString()=="*")
        return false;
    if(sourceModel()->data(left).toString()=="*")
        return true;

    if(mGenre[idLeft].idParrentGenre==0 && mGenre[idRight].idParrentGenre==0)
        return idLeft<idRight;

    if(mGenre[idLeft].idParrentGenre==0 && mGenre[idRight].idParrentGenre>0)
        return idLeft<=mGenre[idRight].idParrentGenre;

    if(mGenre[idLeft].idParrentGenre>0 && mGenre[idRight].idParrentGenre==0)
        return idRight>mGenre[idLeft].idParrentGenre;

    if(mGenre[idLeft].idParrentGenre>0 && mGenre[idRight].idParrentGenre>0){
        if(mGenre[idLeft].idParrentGenre==mGenre[idRight].idParrentGenre)
            return mGenre[idLeft].nSort<mGenre[idRight].nSort;
        return mGenre[idLeft].idParrentGenre<mGenre[idRight].idParrentGenre;
    }

    return idLeft<idRight;
}
