#define QT_USE_QSTRINGBUILDER
#include "exportdlg.h"
#include "ui_exportdlg.h"

#include <QFileDialog>

ExportDlg::ExportDlg(QWidget *parent) :
    QDialog(parent,Qt::Window|Qt::WindowSystemMenuHint),
    ui(new Ui::ExportDlg)
{
    ui->setupUi(this);
    connect(ui->AbortButton, &QAbstractButton::clicked, this, &ExportDlg::BreakExport);
    itsOkToClose = false;
    worker = 0;
    pExportOptions_ = nullptr;
}

ExportDlg::~ExportDlg()
{
    if(worker)
        delete worker;
    delete ui;
}

void ExportDlg::reject()
{
    if (itsOkToClose)
    {
       QDialog::reject();
    }
    else
    {
        BreakExport();
    }
}

void ExportDlg::exec(const QList<uint> &list_books, SendType send, qlonglong id_author, const ExportOptions &exportOptions)
{
    pExportOptions_ = &exportOptions;
    ui->Exporting->setText(QStringLiteral("0"));
    ui->CloseAfter->setChecked(options.bCloseDlgAfterExport);
    ui->progressBar->setValue(0);
    ui->progressBar->setRange(0, 100);
    QString dir;
    if(send != ST_Mail && !exportOptions.bPostprocessingCopy)
    {
        dir = SelectDir();
        if(dir.isEmpty())
        {
            return;
        }
    }
    thread = new QThread;
    worker= new ExportThread(&exportOptions);
    worker->start(dir, list_books, send, id_author);
    worker->moveToThread(thread);
    connect(worker, &ExportThread::Progress, this, &ExportDlg::Process);
    connect(thread, &QThread::started, worker, &ExportThread::process);
    connect(worker, &ExportThread::End, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    connect(worker, &ExportThread::End, this, &ExportDlg::EndExport);
    connect(this, &ExportDlg::break_exp, worker, &ExportThread::break_exp);
    thread->start();

    QDialog::exec();
}

void ExportDlg::exec(qlonglong id_lib, const QString &path)
{
    ui->Exporting->setText(QStringLiteral("0"));
    ui->CloseAfter->setChecked(options.bCloseDlgAfterExport);
    ui->progressBar->setValue(0);
    ui->progressBar->setRange(0, 0);

    thread = new QThread;
    worker = new ExportThread;
    worker->start(id_lib,path);
    worker->moveToThread(thread);
    connect(worker, &ExportThread::Progress, this, &ExportDlg::Process);
    connect(thread, &QThread::started, worker, &ExportThread::process);
    connect(worker, &ExportThread::End, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    connect(worker, &ExportThread::End, this, &ExportDlg::EndExport);
    connect(this, &ExportDlg::break_exp, worker, &ExportThread::break_exp);
    thread->start();

    QDialog::exec();
}

int ExportDlg::exec()
{
    return 0;
}

QString ExportDlg::SelectDir()
{
    QString result = pExportOptions_->sDevicePath;
    if(pExportOptions_->bAskPath || result.isEmpty())
    {
        result=QFileDialog::getExistingDirectory(this, tr("Select device directory"), result);
    }
    return result;
}

void ExportDlg::EndExport()
{
    thread = 0;
    ui->AbortButton->setText(tr("Close"));
    succesfull_export_books = worker->successful_export_books;

    itsOkToClose = true;
    if(ui->CloseAfter->checkState() == Qt::Checked)
    {
        close();
    }
}

void ExportDlg::BreakExport()
{
    if(thread)
    {
        if(thread->isFinished())
        {
            itsOkToClose = true;
            close();
        }
        else
        {
            emit break_exp();
        }
    }
    else
        close();
}

void ExportDlg::Process(int Procent, int count)
{
    ui->progressBar->setValue(Procent);
    ui->Exporting->setText(QString::number(count));
}

