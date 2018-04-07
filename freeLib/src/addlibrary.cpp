#include <QMainWindow>
#include <QToolButton>

#include "addlibrary.h"
#include "ui_addlibrary.h"
#include "common.h"
#include "quazip/quazip/quazip.h"
#include "quazip/quazip/quazipfile.h"
#include "exportdlg.h"

extern int IdCurrentLib;

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

    IdCurrentLib_ = -1;
    UpdateLibList();

    connect(tbInpx,SIGNAL(clicked()),this,SLOT(InputINPX()));
    connect(tbBooksDir,SIGNAL(clicked()),this,SLOT(SelectBooksDir()));
    connect(ui->btnUpdate,SIGNAL(clicked()),this,SLOT(StartImport()));
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
    IdCurrentLib_ =-1;
    ui->ExistingLibs->addItem(tr("new"),"");
    ui->ExistingLibs->setCurrentIndex(ui->ExistingLibs->count()-1);
    app->processEvents();
    SaveLibrary(IdCurrentLib_,ui->ExistingLibs->currentText().trimmed(),"","",false,false);
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
        delete ui->Log->takeItem(0);
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
    query.exec("SELECT id,name,path,inpx,firstauthor, woDeleted FROM lib ORDER BY name");
    ui->ExistingLibs->clear();
    mapLib.clear();
    while(query.next())
    {
        uint idLib = query.value(0).toUInt();
        mapLib[idLib].id = idLib;
        mapLib[idLib].name = query.value(1).toString().trimmed();
        mapLib[idLib].path = query.value(2).toString().trimmed();
        mapLib[idLib].sInpx = query.value(3).toString().trimmed();
        mapLib[idLib].bFirstAuthor = query.value(4).toBool();
        mapLib[idLib].bWoDeleted = query.value(5).toBool();
        ui->ExistingLibs->addItem(mapLib[idLib].name,idLib);

        if(idLib==current_lib.id)
            ui->ExistingLibs->setCurrentIndex(ui->ExistingLibs->count()-1);
    }
}
void AddLibrary::StartImport()
{
    int update_type=(ui->add_new->isChecked()?UT_NEW:ui->del_old->isChecked()?UT_DEL_AND_NEW:UT_FULL);
    ui->btnUpdate->setDisabled(true);
    ui->BookDir->setDisabled(true);
    ui->inpx->setDisabled(true);
    ui->ExistingLibs->setDisabled(true);
    ui->Del->setDisabled(true);
    ui->Add->setDisabled(true);
    ui->btnCancel->setText(tr("Break"));
    ui->update_group->hide();
    QString sLibName = ui->ExistingLibs->currentText().trimmed();
    QString sPath = ui->BookDir->text().trimmed();
    QString sInpx = ui->inpx->text().trimmed();
    bool bFirstAuthor = ui->firstAuthorOnly->isChecked();
    bool bWoDeleted = ui->checkwoDeleted->isChecked();
    SaveLibrary(IdCurrentLib_,sLibName ,sPath,sInpx,bFirstAuthor,bWoDeleted);

    thread = new QThread;
    imp_tr=new ImportThread();
    imp_tr->start(sInpx,sLibName,sPath,IdCurrentLib_,update_type,false,
                  bFirstAuthor&&sInpx.isEmpty(),bWoDeleted);
    imp_tr->moveToThread(thread);
    connect(imp_tr, SIGNAL(Message(QString)), this, SLOT(LogMessage(QString)));
    connect(thread, SIGNAL(started()), imp_tr, SLOT(process()));
    connect(imp_tr, SIGNAL(End()), thread, SLOT(quit()));
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
    IdCurrentLib_ =-1;
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
    if(IdCurrentLib_>=0)
    {
        for(int i=0;i<ui->ExistingLibs->count();i++)
        {
            int idLib = ui->ExistingLibs->itemData(i).toInt();
            if(idLib==IdCurrentLib_)
            {
                //ui->ExistingLibs->setItemData(i,QString::number(idLib)+"$#$?#-"+ui->BookDir->text()+"$#$?#-"+ui->inpx->text()+"$#$?#-"+(ui->firstAuthorOnly->isChecked()?"1":"0"));
                SaveLibrary(idLib,ui->ExistingLibs->itemText(i),ui->BookDir->text(),ui->inpx->text(),ui->firstAuthorOnly->isChecked(),ui->checkwoDeleted->isChecked());
                break;
            }
        }
    }
    QString dir,inpx;
    bool firstAuthor=false;
    IdCurrentLib_ = ui->ExistingLibs->itemData(ui->ExistingLibs->currentIndex()).toInt(); //ExistingID;
    if(IdCurrentLib_>=0){
        dir = mapLib[IdCurrentLib_].path;
        inpx = mapLib[IdCurrentLib_].sInpx;
        firstAuthor = mapLib[IdCurrentLib_].bFirstAuthor;
    }

    ui->BookDir->setText(dir);
    ui->inpx->setText(inpx);
    ui->firstAuthorOnly->setChecked(firstAuthor);
    ui->checkwoDeleted->setChecked(mapLib[IdCurrentLib_].bWoDeleted);
    ui->Del->setDisabled(IdCurrentLib_<0);
    ui->ExistingLibs->setDisabled(IdCurrentLib_<0);
    ui->inpx->setDisabled(IdCurrentLib_<0);
    ui->BookDir->setDisabled(IdCurrentLib_<0);
    ui->btnUpdate->setDisabled(IdCurrentLib_<0);
    QSettings* settings=GetSettings();
    ui->OPDS->setText(IdCurrentLib_<0?"":QString("<a href=\"http://localhost:%2/opds_%1\">http://localhost:%2/opds_%1</a>").arg(IdCurrentLib_).arg(settings->value("OPDS_port",default_OPDS_port).toString()));
    ui->HTTP->setText(IdCurrentLib_<0?"":QString("<a href=\"http://localhost:%2/http_%1\">http://localhost:%2/http_%1</a>").arg(IdCurrentLib_).arg(settings->value("OPDS_port",default_OPDS_port).toString()));

    settings->setValue("LibID",IdCurrentLib_);
    settings->sync();
    current_lib.id=IdCurrentLib_;
    current_lib = mapLib[IdCurrentLib_];
    IdCurrentLib = IdCurrentLib_;
}

void AddLibrary::SaveLibrary(int idLib, QString _name, QString _path, QString _fileName, bool _firstAuthor, bool bWoDeleted)
{
    QSqlQuery query(QSqlDatabase::database("libdb"));
    if(idLib<0)
    {
        LogMessage(tr("Add library"));
        bool result = query.exec(QString("INSERT INTO lib(name,path,inpx,firstAuthor,woDeleted) values('%1','%2','%3',%4,%5)").arg(_name,_path,_fileName,_firstAuthor?"1":"0",bWoDeleted?"1":"0"));
        if(!result)
            qDebug()<<query.lastError().databaseText();
        IdCurrentLib_ = query.lastInsertId().toInt();
        QSettings* settings=GetSettings();
        settings->setValue("LibID",idLib);
        settings->sync();
        UpdateLibList();
    }
    else
    {
        LogMessage(tr("Update library"));
        bool result = query.exec(QString("UPDATE Lib SET name='%1',path='%2',inpx='%3' ,firstAuthor=%4, woDeleted=%5 WHERE ID=%6").arg(_name,_path,_fileName,_firstAuthor?"1":"0",bWoDeleted?"1":"0").arg(idLib));
        if(!result)
            qDebug()<<query.lastError().databaseText();

    }
    mapLib[IdCurrentLib_].name = _name;
    mapLib[IdCurrentLib_].path = _path;
    mapLib[IdCurrentLib_].sInpx = _fileName;
    mapLib[IdCurrentLib_].bFirstAuthor = _firstAuthor;
    mapLib[IdCurrentLib_].bWoDeleted = bWoDeleted;
    mapLib[IdCurrentLib_].id = IdCurrentLib_;
    current_lib = mapLib[IdCurrentLib_];
    bLibChanged = true;
 }
void AddLibrary::DeleteLibrary()
{
    if(IdCurrentLib_<0)
        return;
    if(QMessageBox::question(this,tr("Delete library"),tr("Delete library")+" \""+ui->ExistingLibs->currentText()+"\"",QMessageBox::Yes|QMessageBox::No,QMessageBox::No)==QMessageBox::No)
        return;
    ClearLib(IdCurrentLib_,false);
    QSqlQuery query(QSqlDatabase::database("libdb"));
    query.exec("DELETE FROM lib where ID="+QString::number(IdCurrentLib_));
    UpdateLibList();
    bLibChanged = true;
}
void AddLibrary::EndUpdate()
{
    LogMessage(tr("Ending"));
    UpdateLibList();
    ui->btnUpdate->setDisabled(false);
    ui->btnCancel->setText(tr("Close"));
    ui->BookDir->setDisabled(false);
    ui->inpx->setDisabled(false);
    ui->Del->setDisabled(false);
    ui->Add->setDisabled(false);
    ui->ExistingLibs->setDisabled(false);
    ui->update_group->show();
    bLibChanged = true;
}
void AddLibrary::terminateImport()
{
    if(IdCurrentLib_>0)
    {
        SaveLibrary(IdCurrentLib_,ui->ExistingLibs->currentText().trimmed(),ui->BookDir->text().trimmed(),ui->inpx->text().trimmed(),ui->firstAuthorOnly->isChecked(),ui->checkwoDeleted->isChecked());
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
    ed.exec(IdCurrentLib_,dirName);

}


