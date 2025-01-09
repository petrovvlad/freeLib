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
        const auto &lib = g::libs[g::idCurrentLib];
        const SBook& bookThis = lib.books.at(data(0, Qt::UserRole).toUInt());
        const SBook& bookOther = lib.books.at(other.data(0, Qt::UserRole).toUInt());
        switch(sortColumn){
        case 0:  //Название книги
            return localeStringCompare(text(0), other.text(0));

        case 1: //Автор
            return localeStringCompare(lib.authors.at(bookThis.vIdAuthors.at(0)).getName(),
                                    lib.authors.at(bookOther.vIdAuthors.at(0)).getName());
        case 2: //Серия
            if(bookThis.mSequences.empty()/*idSerial == 0*/)
                return true;
            if(bookOther.mSequences.empty()/*idSerial == 0*/)
                return false;
            return localeStringCompare(lib.serials.at(bookThis.mSequences.begin()->first).sName,
                                    lib.serials.at(bookOther.mSequences.begin()->first).sName);
        case 3: //Номер в серии
            if(bookThis.mSequences.empty())
                return true;
            if(bookOther.mSequences.empty())
                return false;
            if(parent() && other.parent() && parent()->type() == ITEM_TYPE_SERIA && other.parent()->type()){
                uint idSequenceThis = parent()->data(0, Qt::UserRole).toUInt();
                uint idSequenceOther = parent()->data(0, Qt::UserRole).toUInt();
                if(idSequenceThis == idSequenceOther){
                    uint numInSequenceThis = bookThis.mSequences.at(idSequenceThis);
                    uint numInSequenceOther = bookOther.mSequences.at(idSequenceThis);
                    return numInSequenceThis < numInSequenceOther;
                }

            }
            if(bookThis.mSequences.size()>1)
                return true;
            if(bookOther.mSequences.size()>1)
                return false;
            return bookThis.mSequences.begin()->second < bookOther.mSequences.begin()->second;

        case 4: //Размер
            return bookThis.nSize < bookOther.nSize;
        case 5: //рейтинг
            return bookThis.nStars < bookOther.nStars;
        case 6: //дата добавления
            return bookThis.date < bookOther.date;
        case 7: //Жанр
            return g::genres[bookThis.vIdGenres.at(0)].sName < g::genres[bookOther.vIdGenres.at(0)].sName;
        case 8: //Язык
            return lib.vLaguages[bookThis.idLanguage] < lib.vLaguages[bookOther.idLanguage];
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
