#include "treebookitem.h"

#include <QCollator>

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

    if(type() == ITEM_TYPE_BOOK && other.type() == ITEM_TYPE_SERIA && parent()->type() == ITEM_TYPE_AUTHOR)
        return false;
    if(type() == ITEM_TYPE_SERIA && other.type() == ITEM_TYPE_BOOK && other.parent()->type() == ITEM_TYPE_AUTHOR)
        return true;

    if(type() == ITEM_TYPE_BOOK && other.type() == ITEM_TYPE_BOOK){
        SBook& bookThis = mLibs[idCurrentLib].mBooks[data(0, Qt::UserRole).toUInt()];
        SBook& bookOther = mLibs[idCurrentLib].mBooks[other.data(0, Qt::UserRole).toUInt()];
        if(bookThis.idSerial > 0 && bookOther.idSerial)
            return bookThis.numInSerial > bookOther.numInSerial;
        return collator.compare(text(0), other.text(0)) > 0;
    }
    if(type() == ITEM_TYPE_AUTHOR && other.type() == ITEM_TYPE_AUTHOR){
        return collator.compare(text(0), other.text(0)) > 0;
    }
    if(type() == ITEM_TYPE_SERIA && other.type()== ITEM_TYPE_SERIA){
        return collator.compare(text(0), other.text(0)) > 0;
    }
    return true;
}
