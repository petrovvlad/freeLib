#include "languagesortfilterproxymodel.h"

#include "library.h"

LanguageSortFilterProxyModel::LanguageSortFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel{parent}
{}

bool LanguageSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    int idLeft = sourceModel()->data(left, Qt::UserRole).toUInt();
    int idRight = sourceModel()->data(right, Qt::UserRole).toUInt();
    if(idLeft == -1)
        return true;
    if(idRight == -1)
        return false;
    const auto &vLaguages = g::libs[g::idCurrentLib].vLaguages;
    QLocale locale;
    auto sCurrentLanguage = locale.name().left(2);
    if(vLaguages[idLeft] == sCurrentLanguage)
        return true;
    if(vLaguages[idRight] == sCurrentLanguage)
        return false;

    QString sLeft = sourceModel()->data(left, Qt::DisplayRole).toString();
    QString sRight = sourceModel()->data(right, Qt::DisplayRole).toString();
    return QString::localeAwareCompare(sLeft, sRight) <=0 ;

}
