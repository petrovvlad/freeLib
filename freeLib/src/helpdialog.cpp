#define QT_USE_QSTRINGBUILDER
#include "helpdialog.h"
#include "ui_helpdialog.h"

#include <QFile>

#include "config-freelib.h"

HelpDialog::HelpDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HelpDialog)
{
    ui->setupUi(this);

    loadPage(ui->textBrowserAbout, QStringLiteral("about.md"));
    loadPage(ui->textBrowserChangelog, QStringLiteral("changelog.md"));
    loadPage(ui->textBrowserParametrs, QStringLiteral("params.md"));
    loadPage(ui->textBrowserCmdLine, QStringLiteral("cmd_params.md"));
}

HelpDialog::~HelpDialog()
{
    delete ui;
}

void HelpDialog::loadPage(QTextBrowser *textBrowser, const QString &sFileName)
{
    QString sHelpFile = QApplication::applicationDirPath() + QLatin1String("/Help/") + sFileName;
    if(!QFile::exists(sHelpFile))
        sHelpFile = FREELIB_DATA_DIR + QLatin1String("/help/") + sFileName;
    textBrowser->setSource(sHelpFile, QTextDocument::MarkdownResource);
}
