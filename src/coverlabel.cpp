#include "coverlabel.h"

#include <QResizeEvent>

CoverLabel::CoverLabel(QWidget *parent)
    :QLabel(parent)
{
    setMinimumSize(1,1);
    setScaledContents(false);
}

void CoverLabel::setImage(const QImage &image)
{
    if(!image.isNull()){
        pix_ = QPixmap::fromImage(image);
        pix_.setDevicePixelRatio(devicePixelRatioF());
        QLabel::setPixmap(scaledPixmap());
        show();
    }else
        hide();
}

QPixmap CoverLabel::scaledPixmap() const
{
    return pix_.scaledToHeight(this->size().height()*devicePixelRatioF(), Qt::SmoothTransformation);
}

QSize CoverLabel::sizeHint() const
{
    int h = height();
    return pix_.isNull() ?size() :QSize( (float)h * pix_.width() / pix_.height(), h );
}

void CoverLabel::resizeEvent(QResizeEvent */*event*/)
{
    if(!pix_.isNull()){
        pix_.setDevicePixelRatio(devicePixelRatioF());
        QLabel::setPixmap(scaledPixmap());
    }
}

