#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include "config-freelib.h"
#include "git-info.h"


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
    QString debugInfo = QStringLiteral("freeLib - ");
    debugInfo.append(tr("Version %1").arg(QStringLiteral(FREELIB_VERSION)).append(QStringLiteral("\n")));

    QString commitHash = QStringLiteral(GIT_HEAD);;
    if (!commitHash.isEmpty()) {
        debugInfo.append(tr("Revision: %1").arg(commitHash.left(7)).append("\n"));
    }

    // Qt related debugging information.
    debugInfo.append("\n");
    debugInfo.append("Qt ").append(QString::fromLocal8Bit(qVersion())).append("\n");
#ifdef QT_NO_DEBUG
    debugInfo.append(tr("Debugging mode is disabled.").append("\n"));
#else
    debugInfo.append(tr("Debugging mode is enabled.").append("\n"));
#endif
    debugInfo.append("\n");

#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
    debugInfo.append(tr("Operating system: %1\nCPU architecture: %2\nKernel: %3 %4")
                         .arg(QSysInfo::prettyProductName(),
                              QSysInfo::currentCpuArchitecture(),
                              QSysInfo::kernelType(),
                              QSysInfo::kernelVersion()));

    debugInfo.append("\n\n");
#endif

    return debugInfo;
}
