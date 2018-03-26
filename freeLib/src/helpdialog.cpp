#include "helpdialog.h"
#include "ui_helpdialog.h"
//#include <QWebFrame>
#include <QDebug>

HelpDialog::HelpDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HelpDialog)
{
    ui->setupUi(this);
//    ui->webView->page()->mainFrame()->setScrollBarPolicy(Qt::Vertical,Qt::ScrollBarAlwaysOn);
//    ui->webView->load(QUrl::fromLocalFile(QApplication::applicationDirPath()+"/Help/index.html"));
}

HelpDialog::~HelpDialog()
{
    delete ui;
}
