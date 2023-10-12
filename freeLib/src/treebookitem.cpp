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
    static QCollator collator;
    auto sortColumn = treeWidget()->sortColumn();

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

    if(type() == ITEM_TYPE_BOOK && other.type() == ITEM_TYPE_BOOK){
        SBook& bookThis = libs[idCurrentLib].books[data(0, Qt::UserRole).toUInt()];
        SBook& bookOther = libs[idCurrentLib].books[other.data(0, Qt::UserRole).toUInt()];
        switch(sortColumn){
        case 0:  //Название книги
            return collator.compare(text(0), other.text(0)) < 0;
        case 1: //Автор
            return collator.compare(libs[idCurrentLib].authors[bookThis.listIdAuthors.first()].getName(),
                    libs[idCurrentLib].authors[bookOther.listIdAuthors.first()].getName()) < 0;
        case 2: //Серия
            if(bookThis.idSerial == 0 && bookOther.idSerial != 0)
                return true;
            if(bookThis.idSerial != 0 && bookOther.idSerial == 0)
                return false;
            return collator.compare(libs[idCurrentLib].serials[bookThis.idSerial].sName,
                    libs[idCurrentLib].serials[bookOther.idSerial].sName) < 0;
        case 3: //Номер в серии
            return bookThis.numInSerial < bookOther.numInSerial;
        case 4: //Размер
            return bookThis.nSize < bookOther.nSize;
        case 5: //рейтинг
            return bookThis.nStars < bookOther.nStars;
        case 6: //дата добавления
            return bookThis.date < bookOther.date;
        case 7: //Жанр
            return collator.compare(genres[bookThis.listIdGenres.first()].sName, genres[bookOther.listIdGenres.first()].sName) < 0;
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
        case 0:  //Название книги
            return collator.compare(text(0), other.text(0)) < 0;
        default:
            return (collator.compare(text(0), other.text(0)) < 0) != (sortOrder == Qt::DescendingOrder);
        }
    }
    if(type() == ITEM_TYPE_SERIA && other.type()== ITEM_TYPE_SERIA){
        auto header  = treeWidget()->header();
        auto sortOrder  = header->sortIndicatorOrder();
        switch(sortColumn){
        case 0:  //Название книги
            return collator.compare(text(0), other.text(0)) < 0;
        default:
            return (collator.compare(text(0), other.text(0)) < 0) != (sortOrder == Qt::DescendingOrder);
        }
    }
    return true;
}
