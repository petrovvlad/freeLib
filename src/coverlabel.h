#ifndef COVERLABEL_H
#define COVERLABEL_H

#include <QLabel>
#include <QObject>

class CoverLabel : public QLabel
{
    Q_OBJECT
public:
    CoverLabel(QWidget *parent);
    void setImage(const QImage &image);
    QPixmap scaledPixmap() const;
    virtual QSize sizeHint() const;
protected:
    void resizeEvent(QResizeEvent *event);

private:
    QPixmap pix_;
};

#endif // COVERLABEL_H
