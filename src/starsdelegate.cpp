#include "starsdelegate.h"

#include <QApplication>
#include "utilites.h"

StarsDelegate::StarsDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
    for(int i=0; i<=5; i++){
        iconStars[i] = QIcon(u":/img/icons/stars/%1star.svg"_s.arg(i));
    }
    align_ = Qt::AlignCenter;
}

void StarsDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter, option, index);

    int nStars = index.data(Qt::UserRole).toInt();
    if(nStars>=0 && nStars<=5)
    {
        auto rect = option.rect;
        if((align_ & Qt::AlignHorizontal_Mask) == Qt::AlignLeft){
            rect.setLeft(option.font.pointSize());
        }
        float width = option.font.pointSizeF()*8.0f;
        QSize size;
        float scale = width / 80.f;
        if(option.rect.width() < width)
            scale *= option.rect.width() / width;
        size = QSize(80.0f * scale, 16.0f * scale);

        option.widget->style()->drawItemPixmap(painter, rect, align_, iconStars[nStars].pixmap(size));
    }
}

void StarsDelegate::setAlinment(int align)
{
    align_ = align;
}

