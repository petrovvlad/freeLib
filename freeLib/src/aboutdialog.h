#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>

#define MAJOR_VERSION 6
#define MINOR_VERSION 0
#define PROG_VERSION QStringLiteral("v %1.%2.%3").arg(MAJOR_VERSION).arg(MINOR_VERSION).arg(BUILD)

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
private slots:
    void CloseBtn();
};

#endif // ABOUTDIALOG_H
