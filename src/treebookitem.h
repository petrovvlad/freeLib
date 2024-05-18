#ifndef TREEBOOKITEM_H
#define TREEBOOKITEM_H

#include <QTreeWidgetItem>

#define ITEM_TYPE_BOOK QTreeWidgetItem::UserType+1
#define ITEM_TYPE_SERIA QTreeWidgetItem::UserType+2
#define ITEM_TYPE_AUTHOR QTreeWidgetItem::UserType+3


class TreeBookItem : public QTreeWidgetItem
{
public:
    TreeBookItem(QTreeWidget *parent, int type = Type);
    TreeBookItem(QTreeWidgetItem *parent, int type = Type);

    bool operator<(const QTreeWidgetItem &other) const;

};

#endif // TREEBOOKITEM_H
