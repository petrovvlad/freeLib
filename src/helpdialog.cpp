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
    QString sHelpFile = QApplication::applicationDirPath() + QStringLiteral("/Help/") + sFileName;
    if(!QFile::exists(sHelpFile))
        sHelpFile = FREELIB_DATA_DIR + QStringLiteral("/help/") + sFileName;
    QFile file(sHelpFile);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        textBrowser->setMarkdown(file.readAll());
#else
        QString sText = QStringLiteral("<meta charset=\"utf-8\">\n") + file.readAll();
        sText.replace(QStringLiteral("  \n"), QStringLiteral("<br>"));
        sText.replace(QStringLiteral("\n\n"), QStringLiteral("<br>"));
        sText.replace(QStringLiteral("><b>"), QStringLiteral("&nbsp;&nbsp;&nbsp;&nbsp;<b>"));
        textBrowser->setHtml(sText);
#endif
    }
}
