#include "treebookitem.h"

#include <QCollator>
#include <QHeaderView>

#include "library.h"

TreeBookItem::TreeBookItem(QTreeWidget *parent, int type)
    :QTreeWidgetItem(parent, type)
{

}

TreeBookItem::TreeBookItem(QTreeWidgetItem *parent, int type)
    :QTreeWidgetItem(parent, type)
{

}

bool TreeBookItem::operator<(const QTreeWidgetItem &other) const
{
    if(type() == ITEM_TYPE_BOOK && other.type() == ITEM_TYPE_SERIA && parent()->type() == ITEM_TYPE_AUTHOR){
        auto header  = treeWidget()->header();
        auto sortOrder  = header->sortIndicatorOrder();
        return sortOrder == Qt::DescendingOrder;
    }
    if(type() == ITEM_TYPE_SERIA && other.type() == ITEM_TYPE_BOOK && other.parent()->type() == ITEM_TYPE_AUTHOR){
        auto header  = treeWidget()->header();
        auto sortOrder  = header->sortIndicatorOrder();
        return sortOrder == Qt::AscendingOrder;
    }
    auto sortColumn = treeWidget()->sortColumn();

    if(type() == ITEM_TYPE_BOOK && other.type() == ITEM_TYPE_BOOK){
        SBook& bookThis = libs[idCurrentLib].books[data(0, Qt::UserRole).toUInt()];
        SBook& bookOther = libs[idCurrentLib].books[other.data(0, Qt::UserRole).toUInt()];
        switch(sortColumn){
        case 0:  //Название книги
            return localeStringCompare(text(0), other.text(0));

        case 1: //Автор
            return localeStringCompare(libs[idCurrentLib].authors[bookThis.vIdAuthors.at(0)].getName(),
                                    libs[idCurrentLib].authors[bookOther.vIdAuthors.at(0)].getName());
        case 2: //Серия
            if(bookThis.idSerial == 0 && bookOther.idSerial != 0)
                return true;
            if(bookThis.idSerial != 0 && bookOther.idSerial == 0)
                return false;
            return localeStringCompare(libs[idCurrentLib].serials[bookThis.idSerial].sName,
                                    libs[idCurrentLib].serials[bookOther.idSerial].sName);
        case 3: //Номер в серии
            return bookThis.numInSerial < bookOther.numInSerial;
        case 4: //Размер
            return bookThis.nSize < bookOther.nSize;
        case 5: //рейтинг
            return bookThis.nStars < bookOther.nStars;
        case 6: //дата добавления
            return bookThis.date < bookOther.date;
        case 7: //Жанр
            return genres[bookThis.vIdGenres.at(0)].sName < genres[bookOther.vIdGenres.at(0)].sName;
        case 8: //Язык
            return libs[idCurrentLib].vLaguages[bookThis.idLanguage] < libs[idCurrentLib].vLaguages[bookOther.idLanguage];
        case 9: //Формат
            return bookThis.sFormat < bookOther.sFormat;
        }

    }
    if(type() == ITEM_TYPE_AUTHOR && other.type() == ITEM_TYPE_AUTHOR){
        auto header  = treeWidget()->header();
        auto sortOrder  = header->sortIndicatorOrder();
        switch(sortColumn){
        case 1: //Автор
            return localeStringCompare(text(0), other.text(0));
        default:
            return (localeStringCompare(text(0), other.text(0)) ) != (sortOrder == Qt::DescendingOrder);

        }
    }
    if(type() == ITEM_TYPE_SERIA && other.type()== ITEM_TYPE_SERIA){
        auto header  = treeWidget()->header();
        auto sortOrder  = header->sortIndicatorOrder();
        switch(sortColumn){
        case 2: //Серия
        {
            return localeStringCompare(text(0), other.text(0));
        }
        default:
            return localeStringCompare(text(0), other.text(0))  != (sortOrder == Qt::DescendingOrder);
        }
    }
    return true;
}
