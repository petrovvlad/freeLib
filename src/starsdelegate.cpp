#include "starsdelegate.h"

#include <QApplication>
#include "utilites.h"

StarsDelegate::StarsDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
    for(int i=0; i<=5; i++){
        iconStars[i] = QIcon(u":/img/icons/stars/%1star.svg"_s.arg(i));
    }

}

void StarsDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter, option, index);

    int nStars = index.data(Qt::UserRole).toInt();
    if(nStars>=0)
    {
        float width = option.font.pointSize()*8.0f;
        QSize size;
        float scale = width / 80.f;
        if(option.rect.width() < width)
            scale *= option.rect.width() / width;
        size = QSize(80.0f * scale, 16.0f * scale);

        option.widget->style()->drawItemPixmap(painter, option.rect, Qt::AlignCenter, iconStars[nStars].pixmap(size));
    }
}
