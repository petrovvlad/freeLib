#include "libwizard.h"
#include "ui_libwizard.h"
#include <QDebug>
#include <QDir>
#include <QToolButton>
#include <QHBoxLayout>
#include <QFileDialog>
#include "quazip/quazip/quazip.h"
#include "quazip/quazip/quazipfile.h"
#include "common.h"
#include <QBuffer>

LibWizard::LibWizard(QWidget *parent) :
    QWizard(parent),
    ui(new Ui::LibWizard)
{
    ui->setupUi(this);
    connect(this,SIGNAL(currentIdChanged(int)),this,SLOT(SetDir(int)));
    QToolButton* tbInpx=new QToolButton(this);
    tbInpx->setFocusPolicy(Qt::NoFocus);
    tbInpx->setCursor(Qt::ArrowCursor);
    tbInpx->setText("...");
    QHBoxLayout* layout=new QHBoxLayout(ui->inpx);
    layout->addWidget(tbInpx,0,Qt::AlignRight);
    layout->setSpacing(0);
    layout->setMargin(0);
    QToolButton* tbBooksDir=new QToolButton(this);
    tbBooksDir->setFocusPolicy(Qt::NoFocus);
    tbBooksDir->setCursor(Qt::ArrowCursor);
    tbBooksDir->setText("...");
    //tbClear->setVisible(false);
    layout=new QHBoxLayout(ui->Dir);
    layout->addWidget(tbBooksDir,0,Qt::AlignRight);
    layout->setSpacing(0);
    layout->setMargin(0);
    connect(tbInpx,SIGNAL(clicked()),this,SLOT(InputINPX()));
    connect(tbBooksDir,SIGNAL(clicked()),this,SLOT(SelectBooksDir()));
//    AddLib=0;
    mode=-1;
}

LibWizard::~LibWizard()
{
    delete ui;
}

void LibWizard::InputINPX()
{
    QDir::setCurrent(QFileInfo(ui->inpx->text()).absolutePath());
    QString fileName = QFileDialog::getOpenFileName(this, tr("Add library"),"",tr("Library")+" (*.inpx)");
    if(!fileName.isEmpty())
    {
        ui->inpx->setText(fileName);
        ui->inpx->setCursorPosition(0);
    }
}
void LibWizard::SelectBooksDir()
{
    QDir::setCurrent(ui->Dir->text());
    QString dir=QFileDialog::getExistingDirectory(this,tr("Select books directory"));
    if(!dir.isEmpty())
    {
        ui->Dir->setText(dir);
        ui->Dir->setCursorPosition(0);
    }
}

bool LibWizard::validateCurrentPage()
{
    switch(this->currentId())
    {
    case 0:
    {
        mode=ui->mode_lib->isChecked()?MODE_LIBRARY:MODE_CONVERTER;
        if(mode==MODE_CONVERTER)
            reject();
        return true;
    }
    case 1:
    {
        QFileInfo fi(ui->Dir->text());
        if(!fi.isDir() || !fi.exists())
        {
            return false;
        }
        break;
    }
    case 2:
    {
        ui->inpx->setText(ui->inpx->text().trimmed());
        ui->inpx->setCursorPosition(0);
        QFileInfo fi(ui->inpx->text());
        if((!fi.isFile() || !fi.exists()) && !ui->inpx->text().isEmpty())
        {
            return false;
        }
        break;
    }
    case 3:
        ui->lib_name->setText(ui->lib_name->text().trimmed());
        ui->lib_name->setCursorPosition(0);
        return !ui->lib_name->text().isEmpty();
    }
    return true;
}

void LibWizard::SetDir(int page)
{
    //qDebug()<<page;
    switch(page)
    {
    case 1:
        if(mode==MODE_CONVERTER)
        {
    //        qDebug()<<"ww";

        }
        break;
    case 2:
    {
        QString inpx=find_inpx();
        if(!inpx.isEmpty())
        {
            ui->inpx->setText(inpx);
            ui->inpx->setCursorPosition(0);
        }
        break;
    }
    case 3:
        if(!ui->inpx->text().isEmpty())
        {

            QuaZip uz(ui->inpx->text());
            if(!uz.open(QuaZip::mdUnzip))
            {
                qDebug()<<"Error open INPX file: "<<ui->inpx->text();
                return;
            }
            QStringList list = uz.getFileNameList();
            foreach(QString str,list)
            {
                if(QString(str).toUpper()=="COLLECTION.INFO")
                {
                    QBuffer outbuff;
                    //if (!outbuff.open(QIODevice::ReadWrite))
                    //{
                    //    qDebug()<<("Error create buffer!");
                    //    return;
                    //}
                    SetCurrentZipFileName(&uz,str);
                    QuaZipFile zip_file(&uz);
                    zip_file.open(QIODevice::ReadOnly);
                    outbuff.setData(zip_file.readAll());
                    zip_file.close();
                    //uz.extractFile(str,&outbuff,UnZip::SkipPaths);
                    //qDebug()<<outbuff.size();
                    QStringList lines=(QString::fromUtf8(outbuff.data())).split('\n');
                    foreach(QString line,lines)
                    {
                        ui->lib_name->setText(line);
                        ui->lib_name->setCursorPosition(0);
                        //name=line.trimmed();
                        break;
                    }
                }
            }
        }
        break;
    case 4:
    {
        name=ui->lib_name->text().trimmed();
        dir=ui->Dir->text().trimmed();
        inpx=ui->inpx->text().trimmed();
        ui->r_dir->setText(dir);
        ui->r_dir->setCursorPosition(0);
        ui->r_inpx->setText(inpx);
        ui->r_inpx->setCursorPosition(0);
        ui->r_name->setText(name);
        ui->r_name->setCursorPosition(0);
    }
    }
}

QString LibWizard::find_inpx()
{
    QString path=ui->Dir->text();
    int i=100;
    while(!path.isEmpty() && i>0)
    {
        QStringList list=QDir(path).entryList(QStringList()<<"*.inpx",QDir::Files|QDir::Readable);
        foreach (QString str, list)
        {
            return QString(path+"/"+str).replace("//","/");
        }
        if(path==QFileInfo(path).absolutePath())
            return "";
        path=QFileInfo(path).absolutePath();
        i--;
    }
    return "";
}

int LibWizard::AddLibMode()
{
//    AddLib=1;
    this->removePage(0);
    return exec();
}
