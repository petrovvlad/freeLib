#ifndef STARSDELEGATE_H
#define STARSDELEGATE_H

#include <QStyledItemDelegate>

class StarsDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit StarsDelegate(QObject *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setAlinment(int align);
private:
    QIcon iconStars[6];
    int align_;
};

#endif // STARSDELEGATE_H
