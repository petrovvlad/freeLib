#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>

#ifdef Q_OS_WIN
    #define VERSION_FONT    45
#else
    #ifdef Q_OS_LINUX
        #define VERSION_FONT        55
    #else
        #define VERSION_FONT    65
    #endif
#endif

namespace Ui {
class AboutDialog;
}

class AboutDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit AboutDialog(QWidget *parent = 0);
    ~AboutDialog();
    
private:
    Ui::AboutDialog *ui;
    QString debugInfo();
};

#endif // ABOUTDIALOG_H
