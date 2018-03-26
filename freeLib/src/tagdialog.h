#ifndef TAGDIALOG_H
#define TAGDIALOG_H

#include <QDialog>


namespace Ui {
class TagDialog;
}

class TagDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TagDialog(QWidget *parent = 0);
    ~TagDialog();

private:
    Ui::TagDialog *ui;
private slots:
    void ok_btn();
};

#endif // TAGDIALOG_H
