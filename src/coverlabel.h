#ifndef COVERLABEL_H
#define COVERLABEL_H

#include <QLabel>
#include <QObject>

#include "library.h"

class CoverLabel : public QLabel
{
    Q_OBJECT
public:
    CoverLabel(QWidget *parent);
    void setBook(const SBook *pBook);
    void setImage(const QImage &image);
    QPixmap scaledPixmap() const;
    virtual QSize sizeHint() const;
protected:
    void resizeEvent(QResizeEvent *event);

private:
    QPixmap pix;
};

#endif // COVERLABEL_H
