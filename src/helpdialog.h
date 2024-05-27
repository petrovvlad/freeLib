#ifndef HELPDIALOG_H
#define HELPDIALOG_H

#include <QDialog>
#include <QTextBrowser>

namespace Ui {
class HelpDialog;
}

class HelpDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit HelpDialog(QWidget *parent = 0);
    ~HelpDialog();
    
private:
    Ui::HelpDialog *ui;
    void loadPage(QTextBrowser* textBrowser, const QString& sFileName);
};

#endif // HELPDIALOG_H
