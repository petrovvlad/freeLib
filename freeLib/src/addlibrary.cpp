#include <QMainWindow>
#include <QToolButton>

#include "addlibrary.h"
#include "ui_addlibrary.h"
#include "common.h"
#include "quazip/quazip/quazip.h"
#include "quazip/quazip/quazipfile.h"
#include "exportdlg.h"

AddLibrary::AddLibrary(QWidget *parent) :
    QDialog(parent,Qt::Dialog|Qt::WindowSystemMenuHint),
    ui(new Ui::AddLibrary)
{
    bLibChanged = false;
    ui->setupUi(this);

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
    layout=new QHBoxLayout(ui->BookDir);
    layout->addWidget(tbBooksDir,0,Qt::AlignRight);
    layout->setSpacing(0);
    layout->setMargin(0);

    ExistingID = -1;
    UpdateLibList();

    connect(tbInpx,SIGNAL(clicked()),this,SLOT(InputINPX()));
    connect(tbBooksDir,SIGNAL(clicked()),this,SLOT(SelectBooksDir()));
    connect(ui->btnUpdate,SIGNAL(clicked()),this,SLOT(StartImport()));
    //connect(ui->btnOK,SIGNAL(clicked()),this,SLOT(Ok()));
    connect(ui->btnCancel,SIGNAL(clicked()),this,SLOT(terminateImport()));
    connect(ui->btnExport,SIGNAL(clicked()),this,SLOT(ExportLib()));
    connect(ui->ExistingLibs,SIGNAL(currentIndexChanged(int)),this,SLOT(SelectLibrary()));
    connect(ui->Del,SIGNAL(clicked()),this,SLOT(DeleteLibrary()));
    connect(ui->Add,SIGNAL(clicked()),this,SLOT(Add_Library()));
    connect(ui->ExistingLibs->lineEdit(),SIGNAL(editingFinished()),this,SLOT(ExistingLibsChanged()));
    ui->add_new->setChecked(true);

    SelectLibrary();
}

AddLibrary::~AddLibrary()
{
    delete ui;
}

void AddLibrary::Add_Library()
{
    ExistingID=-1;
    ui->ExistingLibs->addItem(tr("new"),"");
    ui->ExistingLibs->setCurrentIndex(ui->ExistingLibs->count()-1);
    app->processEvents();
    SaveLibrary(ui->ExistingLibs->currentText().trimmed(),"","",false);
}

//int AddLibrary::exec()
///{
    //QString fileName = QFileDialog::getOpenFileName(this, tr("Add library"),"",tr("Library (*.inpx)"));
    // it(this);
    //imp_tr.start(fileName);
   // return QDialog::exec();
//}

void AddLibrary::LogMessage(QString msg)
{
    while(ui->Log->count()>100)
        ui->Log->takeItem(0);
    ui->Log->addItem(msg);
    ui->Log->setCurrentRow(ui->Log->count()-1);
}
void AddLibrary::InputINPX()
{
    QDir::setCurrent(QFileInfo(ui->inpx->text()).absolutePath());
    QString fileName = QFileDialog::getOpenFileName(this, tr("Add library"),"",tr("Library")+" (*.inpx)");
    if(!fileName.isEmpty())
    {
        ui->inpx->setText(fileName);
        ui->BookDir->setText(QFileInfo(fileName).absolutePath());
        QuaZip uz(fileName);
        if(!uz.open(QuaZip::mdUnzip))
        {
            qDebug()<<"Error open INPX file: "<<fileName;
            return;
        }
        if(SetCurrentZipFileName(&uz,"COLLECTION.INFO"))
        {
            QBuffer outbuff;
            QuaZipFile zip_file(&uz);
            zip_file.open(QIODevice::ReadOnly);
            outbuff.setData(zip_file.readAll());
            zip_file.close();
            QString sLib = QString::fromUtf8(outbuff.data().left(outbuff.data().indexOf('\n')));
            ui->ExistingLibs->setItemText(ui->ExistingLibs->currentIndex(),sLib);
//            QStringList lines=(QString::fromUtf8(outbuff.data())).split('\n');
//            foreach(QString line,lines)
//            {
//                ui->ExistingLibs->setItemText(ui->ExistingLibs->currentIndex(),line);
//                break;
//            }
        }
    }
}
void AddLibrary::SelectBooksDir()
{
    QDir::setCurrent(ui->BookDir->text());
    QString dir=QFileDialog::getExistingDirectory(this,tr("Select books directory"));
    if(!dir.isEmpty())
        ui->BookDir->setText(dir);
}

void AddLibrary::UpdateLibList()
{
    if(!db_is_open)
        return;
    QSqlQuery query(QSqlDatabase::database("libdb"));
    query.exec("SELECT id,name,path,inpx,firstauthor FROM lib ORDER BY name");
    qlonglong CurrentID=current_lib.id;
    ui->ExistingLibs->clear();
    while(query.next())
    {
        ui->ExistingLibs->addItem(query.value(1).toString().trimmed(),
                                  query.value(0).toString()+"$#$?#-"+
                                  query.value(2).toString().trimmed()+"$#$?#-"+
                                  query.value(3).toString().trimmed()+"$#$?#-"+
                                  query.value(4).toString().trimmed());
        if(query.value(0).toInt()==CurrentID)
            ui->ExistingLibs->setCurrentIndex(ui->ExistingLibs->count()-1);
    }
}
void AddLibrary::StartImport()
{
    //ui->btnOK->setDisabled(true);
    int update_type=(ui->add_new->isChecked()?UT_NEW:ui->del_old->isChecked()?UT_DEL_AND_NEW:UT_FULL);
    ui->btnUpdate->setDisabled(true);
    ui->BookDir->setDisabled(true);
    ui->inpx->setDisabled(true);
    //ui->LibName->setDisabled(true);
    ui->ExistingLibs->setDisabled(true);
    ui->Del->setDisabled(true);
    ui->Add->setDisabled(true);
    ui->btnCancel->setText(tr("Break"));
    ui->update_group->hide();
    //ui->author_group->hide();
    SaveLibrary(ui->ExistingLibs->currentText().trimmed(),ui->BookDir->text().trimmed(),ui->inpx->text().trimmed(),ui->firstAuthorOnly->isChecked());

    thread = new QThread;
    imp_tr=new ImportThread();
    imp_tr->start(ui->inpx->text().trimmed(),ui->ExistingLibs->currentText().trimmed(),ui->BookDir->text().trimmed(),ExistingID,update_type,false,
                  ui->firstAuthorOnly->isChecked()&&ui->inpx->text().isEmpty());
    imp_tr->moveToThread(thread);
    connect(imp_tr, SIGNAL(Message(QString)), this, SLOT(LogMessage(QString)));
    connect(thread, SIGNAL(started()), imp_tr, SLOT(process()));
    connect(imp_tr, SIGNAL(End()), thread, SLOT(quit()));
   // connect(worker, SIGNAL(End()), worker, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    connect(imp_tr, SIGNAL(End()), this, SLOT(EndUpdate()));
    connect(this, SIGNAL(break_import()), imp_tr, SLOT(break_import()));


    thread->start();
}
void AddLibrary::AddNewLibrary(QString _name, QString _path, QString _fileName)
{
    //if(!db_is_open)
    {
        db_is_open=openDB(true,false);
    }
    ExistingID=-1;
    ui->ExistingLibs->addItem(_name,"");
    ui->ExistingLibs->setCurrentIndex(ui->ExistingLibs->count()-1);
    app->processEvents();
    ui->BookDir->setText(_path);
    ui->inpx->setText(_fileName);
    ui->add_new->setChecked(true);
    StartImport();
    exec();
}

void AddLibrary::SelectLibrary()
{
    if(ExistingID>=0)
    {
        for(int i=0;i<ui->ExistingLibs->count();i++)
        {
            QStringList str=ui->ExistingLibs->itemData(i).toString().split("$#$?#-");
            if(str.count()<2)
                str.clear();
            if((str.count()==0?-1:str[0].toInt())==ExistingID)
            {
                ui->ExistingLibs->setItemData(i,QString::number(ExistingID)+"$#$?#-"+ui->BookDir->text()+"$#$?#-"+ui->inpx->text()+"$#$?#-"+(ui->firstAuthorOnly->isChecked()?"1":"0"));
                SaveLibrary(ui->ExistingLibs->itemText(i),ui->BookDir->text(),ui->inpx->text(),ui->firstAuthorOnly->isChecked());
                break;
            }
        }
    }
    QStringList str=ui->ExistingLibs->itemData(ui->ExistingLibs->currentIndex()).toString().split("$#$?#-");
    if(str.count()<2)
        str.clear();
    QString dir,inpx;
    ExistingID=(str.count()==0?-1:str[0].toInt());
    dir=(str.count()<2?"":str[1]);
    inpx=(str.count()<3?"":str[2]);
    bool firstAuthor=(str.count()<4?false:(str[3]=="1"));
    ui->BookDir->setText(dir);
    //ui->LibName->setText((str.count()==0?"":ui->ExistingLibs->currentText()));
    ui->inpx->setText(inpx);
    ui->firstAuthorOnly->setChecked(firstAuthor);
    ui->Del->setDisabled(ExistingID<0);
    ui->ExistingLibs->setDisabled(ExistingID<0);
    ui->inpx->setDisabled(ExistingID<0);
    ui->BookDir->setDisabled(ExistingID<0);
    ui->btnUpdate->setDisabled(ExistingID<0);
    //ui->ExistingLibs->setDisabled(ExistingID<0);
    QSettings* settings=GetSettings();
    ui->OPDS->setText(str.count()==0?"":QString("<a href=\"http://localhost:%2/opds_%1\">http://localhost:%2/opds_%1</a>").arg(str[0],settings->value("OPDS_port",default_OPDS_port).toString()));
    ui->HTTP->setText(str.count()==0?"":QString("<a href=\"http://localhost:%2/http_%1\">http://localhost:%2/http_%1</a>").arg(str[0],settings->value("OPDS_port",default_OPDS_port).toString()));

    settings->setValue("LibID",ExistingID/*(str.count()==0?-1:str[0].toInt())*/);
    settings->sync();
    current_lib.id=ExistingID;
    current_lib.UpdateLib();

}

void AddLibrary::SaveLibrary(QString _name,QString _path,QString _fileName,bool _firstAuthor)
{
    QSqlQuery query(QSqlDatabase::database("libdb"));
    if(ExistingID<0)
    {
        LogMessage(tr("Add library"));
        query.exec("INSERT INTO Lib(name,path,inpx,firstAuthor) values('"+_name+"','"+_path+"','"+_fileName+"',"+(_firstAuthor?"1":"0")+")");
        query.exec("select last_insert_rowid()");
        query.next();
        ExistingID=query.value(0).toLongLong();
        QSettings* settings=GetSettings();
        settings->setValue("LibID",ExistingID);
        settings->sync();
        current_lib.UpdateLib();
        UpdateLibList();
    }
    else
    {
        LogMessage(tr("Update library"));
        query.exec("UPDATE Lib SET name='"+_name+"',path='"+_path+"',inpx='"+_fileName+"' ,firstAuthor="+(_firstAuthor?"1":"0")+" WHERE ID="+QString::number(ExistingID));
    }
    bLibChanged = true;
 }
void AddLibrary::DeleteLibrary()
{
    if(ExistingID<0)
        return;
    if(QMessageBox::question(this,tr("Delete library"),tr("Delete library")+" \""+ui->ExistingLibs->currentText()+"\"",QMessageBox::Yes|QMessageBox::No,QMessageBox::No)==QMessageBox::No)
        return;
    ClearLib(ExistingID,false);
    QSqlQuery query(QSqlDatabase::database("libdb"));
    query.exec("DELETE FROM lib where ID="+QString::number(ExistingID));
    UpdateLibList();
    bLibChanged = true;
}
void AddLibrary::EndUpdate()
{
    LogMessage(tr("Ending"));
    UpdateLibList();
    //ui->btnOK->setDisabled(false);
    ui->btnUpdate->setDisabled(false);
    ui->btnCancel->setText(tr("Close"));
    ui->BookDir->setDisabled(false);
    ui->inpx->setDisabled(false);
    ui->Del->setDisabled(false);
    ui->Add->setDisabled(false);
    ui->ExistingLibs->setDisabled(false);
    ui->update_group->show();
    //ui->author_group->show();
}
void AddLibrary::terminateImport()
{
    if(ExistingID>0)
    {
        SaveLibrary(ui->ExistingLibs->currentText().trimmed(),ui->BookDir->text().trimmed(),ui->inpx->text().trimmed(),ui->firstAuthorOnly->isChecked());
        current_lib.UpdateLib();
    }
    emit break_import();
    //imp_tr->loop=false;
    //imp_tr->wait();
}

void AddLibrary::reject()
{
    if (ui->btnCancel->text()==tr("Close"))
    {
       QDialog::reject();
    }
    else
    {
        terminateImport();
    }
}

void AddLibrary::ExistingLibsChanged()
{
    ui->ExistingLibs->setItemText(ui->ExistingLibs->currentIndex(),ui->ExistingLibs->lineEdit()->text());
}

void AddLibrary::ExportLib()
{
    //accept();
    QString dirName = QFileDialog::getExistingDirectory(this, tr("Select destination directory"));
    ExportDlg ed(this);
    ed.exec(current_lib.id,dirName);

}


