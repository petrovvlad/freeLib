#ifndef STARSDELEGATE_H
#define STARSDELEGATE_H

#include <QStyledItemDelegate>

class StarsDelegate : public QStyledItemDelegate
{
public:
    explicit StarsDelegate(QObject *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
private:
    QIcon iconStars[6];

};

#endif // STARSDELEGATE_H
