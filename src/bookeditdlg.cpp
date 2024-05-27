#include "bookeditdlg.h"
#include "ui_bookeditdlg.h"

BookEditDlg::BookEditDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BookEditDlg)
{
    ui->setupUi(this);
}

BookEditDlg::~BookEditDlg()
{
    delete ui;
}
