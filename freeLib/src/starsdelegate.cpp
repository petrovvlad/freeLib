#include "starsdelegate.h"

#include <QApplication>

StarsDelegate::StarsDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
    for(int i=0; i<=5; i++){
        iconStars[i] = QIcon(QStringLiteral(":/icons/img/icons/stars/%1star.svg").arg(i));
    }

}

void StarsDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter, option, index);

    int nStars = index.data(Qt::UserRole).toInt();
    if(nStars>=0)
    {
        QSize size;
        if(option.rect.width()<80)
        {
            float scale = option.rect.width() / 80.0f;
            size = QSize(80.0f * scale, 16.0f * scale);
        }
        else
            size = QSize(80, 16);
        option.widget->style()->drawItemPixmap(painter, option.rect, Qt::AlignCenter, iconStars[nStars].pixmap(size));
    }
}
