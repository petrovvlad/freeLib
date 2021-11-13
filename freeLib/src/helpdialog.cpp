#include "helpdialog.h"
#include "ui_helpdialog.h"

#include <QWebEnginePage>


HelpDialog::HelpDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HelpDialog)
{
    ui->setupUi(this);
    ui->webView->load(QUrl::fromLocalFile(QApplication::applicationDirPath() + QLatin1String("/Help/index.html")));
}

HelpDialog::~HelpDialog()
{
    delete ui;
}
