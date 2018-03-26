#ifndef BOOKEDITDLG_H
#define BOOKEDITDLG_H

#include <QDialog>

namespace Ui {
class BookEditDlg;
}

class BookEditDlg : public QDialog
{
    Q_OBJECT

public:
    explicit BookEditDlg(QWidget *parent = 0);
    ~BookEditDlg();

private:
    Ui::BookEditDlg *ui;
};

#endif // BOOKEDITDLG_H
