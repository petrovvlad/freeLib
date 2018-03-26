#include "exportdlg.h"
#include "ui_exportdlg.h"

ExportDlg::ExportDlg(QWidget *parent) :
    QDialog(parent,Qt::Window|Qt::WindowSystemMenuHint),
    ui(new Ui::ExportDlg)
{
    ui->setupUi(this);
    connect(ui->AbortButton,SIGNAL(clicked()),this,SLOT(BreakExport()));
    itsOkToClose=false;
    worker=0;
    //thread.pdf=ui->webView;
//    ui->webView->setUrl(QUrl::fromLocalFile("/home/user/freeLib/Temp/OEBPS/index.html"));
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
        //qDebug()<<"reject";
        BreakExport();
    }
}

void ExportDlg::exec(const QList<book_info> &list_books, SendType send, qlonglong id_author)
{
    ui->Exporting->setText("0");
    QSettings* settings=GetSettings();
    ui->CloseAfter->setChecked(settings->value("CloseExpDlg").toBool());
    ui->progressBar->setValue(0);
    ui->progressBar->setRange(0,100);
    QString dir;
    if(send!=ST_Mail)
    {
        dir=SelectDir();
        if(dir.isEmpty())
        {
            return;
        }
    }
    //qDebug()<<"ok";
    thread = new QThread;
    worker=new ExportThread;
    worker->start(dir,list_books,send,id_author);
    worker->moveToThread(thread);
    connect(worker, SIGNAL(Progress(int,int)), this, SLOT(Process(int,int)));
    connect(thread, SIGNAL(started()), worker, SLOT(process()));
    connect(worker, SIGNAL(End()), thread, SLOT(quit()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    connect(worker, SIGNAL(End()), this, SLOT(EndExport()));
    connect(this, SIGNAL(break_exp()), worker, SLOT(break_exp()));
    thread->start();

    QDialog::exec();
    settings->setValue("CloseExpDlg",ui->CloseAfter->checkState()==Qt::Checked);
    settings->sync();
}
void ExportDlg::exec(const QStringList &list_books, SendType send)
{
    ui->Exporting->setText("0");
    QSettings *settings=GetSettings();
    ui->CloseAfter->setChecked(settings->value("CloseExpDlg").toBool());
    ui->progressBar->setValue(0);
    ui->progressBar->setRange(0,100);
    QString dir;
    if(send!=ST_Mail)
    {
        dir=SelectDir();
        if(dir.isEmpty())
        {
            return;
        }
    }
    //qDebug()<<"ok";
    thread = new QThread;
    worker=new ExportThread;
    worker->start(dir,list_books,send);
    worker->moveToThread(thread);
    connect(worker, SIGNAL(Progress(int,int)), this, SLOT(Process(int,int)));
    connect(thread, SIGNAL(started()), worker, SLOT(process()));
    connect(worker, SIGNAL(End()), thread, SLOT(quit()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    connect(worker, SIGNAL(End()), this, SLOT(EndExport()));
    connect(this, SIGNAL(break_exp()), worker, SLOT(break_exp()));
    thread->start();

    QDialog::exec();
    settings->setValue("CloseExpDlg",ui->CloseAfter->checkState()==Qt::Checked);
    settings->sync();
    //delete settings;
}
void ExportDlg::exec(qlonglong id_lib, QString path)
{
    ui->Exporting->setText("0");
    QSettings *settings=GetSettings();
    ui->CloseAfter->setChecked(settings->value("CloseExpDlg").toBool());
    ui->progressBar->setValue(0);
    ui->progressBar->setRange(0,0);

    thread = new QThread;
    worker=new ExportThread;
    worker->start(id_lib,path);
    worker->moveToThread(thread);
    connect(worker, SIGNAL(Progress(int,int)), this, SLOT(Process(int,int)));
    connect(thread, SIGNAL(started()), worker, SLOT(process()));
    connect(worker, SIGNAL(End()), thread, SLOT(quit()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    connect(worker, SIGNAL(End()), this, SLOT(EndExport()));
    connect(this, SIGNAL(break_exp()), worker, SLOT(break_exp()));
    thread->start();

    QDialog::exec();
    settings->setValue("CloseExpDlg",ui->CloseAfter->checkState()==Qt::Checked);
    settings->sync();
}
int ExportDlg::exec()
{
    return 0;
}

QString ExportDlg::SelectDir()
{
    QString HomeDir="";
    if(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).count()>0)
        HomeDir=QStandardPaths::standardLocations(QStandardPaths::HomeLocation).at(0);
    QSettings *settings=GetSettings();
    QString result=settings->value("DevicePath",HomeDir).toString().trimmed();
    if(settings->value("askPath",true).toBool() || result.isEmpty())
    {
        if(!QDir::setCurrent(settings->value("DevicePath",HomeDir).toString()))
            QDir::setCurrent(HomeDir);
        result=QFileDialog::getExistingDirectory(this,tr("Select device directory"));
    }
    if(!result.isEmpty())
    {
        settings->setValue("DevicePath",result);
        settings->sync();
    }
    return result;
}

void ExportDlg::EndExport()
{
    thread=0;
    ui->AbortButton->setText(tr("Close"));
    succesfull_export_books=worker->successful_export_books;

    itsOkToClose=true;
    if(ui->CloseAfter->checkState()==Qt::Checked)
    {
        close();
    }
}
void ExportDlg::BreakExport()
{
   // thread.loop=false;
  //  worker->loop=false;
    //qDebug()<<"try close";
    if(thread)
    {
        if(thread->isFinished())
        {
            //qDebug()<<"close";
            itsOkToClose=true;
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
void ExportDlg::Process(int Procent,int count)
{
    ui->progressBar->setValue(Procent);
    ui->Exporting->setText(QString::number(count));
}

