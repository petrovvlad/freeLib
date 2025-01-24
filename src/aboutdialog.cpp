#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include <QStringBuilder>

#include "config-freelib.h"
#include "git-info.h"
#include "utilites.h"

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    ui->nameLabel->setText(ui->nameLabel->text().replace(QStringLiteral("${VERSION}"), QStringLiteral(FREELIB_VERSION)));
    ui->debugInfo->setText(debugInfo());
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &AboutDialog::close);
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

QString AboutDialog::debugInfo()
{
    QString debugInfo = u"freeLib - "_s % tr("Version %1").arg(QStringLiteral(FREELIB_VERSION)) % u"\n"_s;

    QString commitHash = QStringLiteral(GIT_HEAD);;
    if (!commitHash.isEmpty()) [[unlikely]]{
        debugInfo += tr("Revision: %1").arg(commitHash.left(7)) + u"\n"_s;
    }

    // Qt related debugging information.
    debugInfo += u"\nQt "_s % QString::fromLocal8Bit(qVersion()) % u"\n"_s;
#ifdef QT_NO_DEBUG
    debugInfo +=tr("Debugging mode is disabled.\n");
#else
    debugInfo += tr("Debugging mode is enabled.\n");
#endif
#ifdef USE_HTTSERVER
    debugInfo += tr("OPDS/Web server: available\n\n");
#else
    debugInfo += tr("OPDS/Web server: not available\n\n");
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
    debugInfo += tr("Operating system: %1\nCPU architecture: %2\nKernel: %3 %4\n")
                         .arg(QSysInfo::prettyProductName(),
                              QSysInfo::currentCpuArchitecture(),
                              QSysInfo::kernelType(),
                              QSysInfo::kernelVersion());

#endif

    return debugInfo;
}
