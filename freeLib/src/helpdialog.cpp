#define QT_USE_QSTRINGBUILDER
#include "helpdialog.h"
#include "ui_helpdialog.h"

#include <QWebEnginePage>
#include <QFile>

#include "config-freelib.h"

HelpDialog::HelpDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HelpDialog)
{
    ui->setupUi(this);
    QString sHelpFile = QApplication::applicationDirPath() + QLatin1String("/Help/index.html");
    if(!QFile::exists(sHelpFile))
        sHelpFile = FREELIB_DATA_DIR + QLatin1String("/help/index.html");
    ui->webView->load(QUrl::fromLocalFile(sHelpFile));

    //    ui->webView->load(QUrl::fromLocalFile(QApplication::applicationDirPath() + QLatin1String("/Help/index.html")));
}

HelpDialog::~HelpDialog()
{
    delete ui;
}
