#ifndef MYAPPLICATIONCLASS_H
#define MYAPPLICATIONCLASS_H

#include <QWidget>

class MyPrivate: public QObject
{
    Q_OBJECT
private:
    static MyPrivate * p;
    MyPrivate();
public:
    static MyPrivate * instance();
    void emitClick();
signals:
    void dockClicked();
};

#endif // MYAPPLICATIONCLASS_H
