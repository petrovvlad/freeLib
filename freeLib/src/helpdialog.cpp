#include <QDebug>
#include <QWebEnginePage>

#include "helpdialog.h"
#include "ui_helpdialog.h"

HelpDialog::HelpDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HelpDialog)
{
    ui->setupUi(this);
    //ui->webView->page()->setScrollBarPolicy(Qt::Vertical,Qt::ScrollBarAlwaysOn);
    ui->webView->load(QUrl::fromLocalFile(QApplication::applicationDirPath()+"/Help/index.html"));
}

HelpDialog::~HelpDialog()
{
    delete ui;
}
