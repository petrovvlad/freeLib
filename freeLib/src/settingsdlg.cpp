#include "settingsdlg.h"
#include "ui_settingsdlg.h"
#include <QSettings>
#include "common.h"
#include <QToolButton>
#include "fontframe.h"
#include "exportframe.h"
#include "./quazip/quazip/quazip.h"
#include "./quazip/quazip/quazipfile.h"


SettingsDlg::SettingsDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDlg)
{
    ui->setupUi(this);
    ui->tabWidget->setCurrentIndex(0);

    connect(ui->buttonBox,SIGNAL(accepted()),this,SLOT(btnOK()));

    connect(ui->DelExp,SIGNAL(clicked()),this,SLOT(DelExt()));
    connect(ui->AddExp,SIGNAL(clicked()),this,SLOT(AddExt()));
    connect(ui->DelApp,SIGNAL(clicked()),this,SLOT(DelApp()));
    connect(ui->AddApp,SIGNAL(clicked()),this,SLOT(AddApp()));
    connect(ui->OPDS_port,SIGNAL(valueChanged(int)),this,SLOT(ChangePort(int)));
    connect(ui->OPDS_enable,SIGNAL(stateChanged(int)),this,SLOT(ChangePort()));

    QToolButton* btnDBPath=new QToolButton(this);
    btnDBPath->setFocusPolicy(Qt::NoFocus);
    btnDBPath->setCursor(Qt::ArrowCursor);
    btnDBPath->setText(tr("Move to ..."));
    QHBoxLayout*  layout=new QHBoxLayout(ui->database_path);
    layout->addWidget(btnDBPath,0,Qt::AlignRight);
    layout->setSpacing(0);
    layout->setMargin(0);
    connect(btnDBPath,SIGNAL(clicked()),this,SLOT(btnDBPath()));

    QToolButton* btnDirPath=new QToolButton(this);
    btnDirPath->setFocusPolicy(Qt::NoFocus);
    btnDirPath->setCursor(Qt::ArrowCursor);
    btnDirPath->setText("...");
    layout=new QHBoxLayout(ui->dirForBrowsing);
    layout->addWidget(btnDirPath,0,Qt::AlignRight);
    layout->setSpacing(0);
    layout->setMargin(0);
    connect(btnDirPath,SIGNAL(clicked()),this,SLOT(btnDirPath()));

    LoadSettings();

    ChangePort(ui->OPDS_port->value());
}

SettingsDlg::~SettingsDlg()
{
    delete ui;
}

void SettingsDlg::LoadSettings()
{
#ifdef Q_OS_MAC
//    ui->settings_to_file->setChecked(QFileInfo(app->applicationDirPath()+"/../../../freeLib/freeLib.cfg").exists());
    ui->settings_to_file->setChecked(QFileInfo(app->applicationDirPath()+"/freeLib.cfg").exists());
#else
    ui->settings_to_file->setChecked(QFileInfo(app->applicationDirPath()+"/freeLib.cfg").exists());
#endif

    QSettings *settings=GetSettings();
    ui->ApplicationList->setColumnWidth(0,100);
    ui->ApplicationList->setColumnWidth(1,400);
    ui->ExportList->setColumnWidth(0,100);
    ui->ExportList->setColumnWidth(1,250);
    ui->ExportList->setColumnWidth(2,150);

    if(settings->contains("SettingsWnd/geometry"))
        restoreGeometry(settings->value("SettingsWnd/geometry").toByteArray());
    if(settings->contains("SettingsWndExportList/geometry"))
    {
        ui->ExportList->restoreGeometry(settings->value("SettingsWndExportList/geometry").toByteArray());
        ui->ExportList->horizontalHeader()->restoreState(settings->value("SettingsWndExportList_headers/geometry").toByteArray());
    }
    if(settings->contains("SettingsWndApplicationList/geometry"))
    {
        ui->ApplicationList->restoreGeometry(settings->value("SettingsWndApplicationList/geometry").toByteArray());
        ui->ApplicationList->horizontalHeader()->restoreState(settings->value("SettingsWndApplicationList_headers/geometry").toByteArray());
    }
    ui->browseDir->setChecked(settings->value("browseDir",false).toBool());
    ui->dirForBrowsing->setText(settings->value("dirForBrowsing").toString());
    on_browseDir_stateChanged(ui->browseDir->isChecked());
    ui->CloseExpDlg->setChecked(settings->value("CloseExpDlg").toBool());
    ui->ShowDeleted->setChecked(settings->value("ShowDeleted").toBool());
    ui->use_tag->setChecked(settings->value("use_tag",true).toBool());
    ui->uncheck_export->setChecked(settings->value("uncheck_export",true).toBool());
    ui->store_pos->setChecked(settings->value("store_position",true).toBool());
    ui->OPDS_enable->setChecked(settings->value("OPDS_enable",false).toBool());
    ui->OPDS_port->setValue(settings->value("OPDS_port",default_OPDS_port).toInt());
    ui->splash->setChecked(settings->value("no_splash",false).toBool());
    ui->trayIcon->setCurrentIndex(settings->value("tray_icon",0).toInt());
    ui->tray_color->setCurrentIndex(settings->value("tray_color",0).toInt());
    ui->HTTP_need_pasword->setChecked(settings->value("HTTP_need_pasword").toBool());
    ui->HTTP_user->setText(settings->value("HTTP_user").toString());
    ui->HTTP_password->setText(settings->value("HTTP_password").toString());
    ui->srv_annotation->setChecked(settings->value("srv_annotation",true).toBool());
    ui->srv_covers->setChecked(settings->value("srv_covers",true).toBool());
    ui->books_per_page->setValue(settings->value("books_per_page",15).toInt());
    ui->extended_symbols->setChecked(settings->value("extended_symbols",false).toBool());
#ifdef Q_OS_WINDOWS
    ui->extended_symbols->hide();
#endif
    ui->proxy_type->setCurrentIndex(settings->value("proxy_type",0).toInt());
    ui->proxy_port->setValue(settings->value("proxy_port",default_proxy_port).toInt());
    ui->proxy_host->setText(settings->value("proxy_host").toString());
    ui->proxy_password->setText(settings->value("proxy_password").toString());
    ui->proxy_user->setText(settings->value("proxy_user").toString());

    int count=settings->beginReadArray("tools");
    ui->ExportList->setRowCount(count);
    for(int i=0;i<count;i++)
    {
        settings->setArrayIndex(i);
        ui->ExportList->setItem(i,0,new QTableWidgetItem(settings->value("name").toString()));
        ui->ExportList->setItem(i,1,new QTableWidgetItem(settings->value("path").toString()));
        ui->ExportList->setItem(i,2,new QTableWidgetItem(settings->value("args").toString()));
        ui->ExportList->setItem(i,3,new QTableWidgetItem(settings->value("ext").toString()));
    }
    settings->endArray();

    count=settings->beginReadArray("application");
    ui->ApplicationList->setRowCount(count);
    ui->ApplicationList->setItemDelegateForColumn(1,new FileItemDelegate(this));
    for(int i=0;i<count;i++)
    {
        settings->setArrayIndex(i);
        ui->ApplicationList->setItem(i,0,new QTableWidgetItem(settings->value("ext").toString()));
        ui->ApplicationList->setItem(i,1,new QTableWidgetItem(settings->value("app").toString()));
    }
    settings->endArray();

    ui->ExportName->clear();
    while(ui->stackedWidget->count()>0)
        ui->stackedWidget->removeWidget(ui->stackedWidget->widget(0));
    count=settings->beginReadArray("export");
    for(int i=0;i<count;i++)
    {
        settings->setArrayIndex(i);
        ExportFrame* frame=new ExportFrame(this);
        ui->stackedWidget->addWidget(frame);
        frame->Load(settings);
        ui->ExportName->addItem(settings->value("ExportName").toString(),settings->value("Default").toBool());
        connect(frame,SIGNAL(ChangeTabIndex(int,int)),this,SLOT(on_ChangeExportFrameTab(int,int)));
        connect(this,SIGNAL(ChangingExportFrameTab(int,int)),frame,SLOT(SetTabIndex(int,int)));
        connect(this,SIGNAL(NeedUpdateTools()),frame,SLOT(UpdateToolComboBox()));
    }

    settings->endArray();
    if(count==0)
    {
        ExportFrame* frame=new ExportFrame(this);
        ui->stackedWidget->addWidget(frame);
        frame->Load(settings);
        ui->ExportName->addItem(tr("Send to ..."),false);
        connect(frame,SIGNAL(ChangeTabIndex(int,int)),this,SLOT(on_ChangeExportFrameTab(int,int)));
        connect(this,SIGNAL(ChangingExportFrameTab(int,int)),frame,SLOT(SetTabIndex(int,int)));
        connect(this,SIGNAL(NeedUpdateTools()),frame,SLOT(UpdateToolComboBox()));
    }
    ui->DelExport->setEnabled(ui->ExportName->count()>1);

    disconnect(ui->Language,SIGNAL(currentIndexChanged(int)),this,SLOT(ChangeLanguage()));
    disconnect(ui->ABC,SIGNAL(currentIndexChanged(int)),this,SLOT(ChangeLanguage()));
    QStringList dirContent = QDir(QApplication::applicationDirPath()+"/language").entryList(QStringList()<< "language_*.qm", QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
    QString locale=settings->value("localeUI",QLocale::system().name()).toString();
    ui->Language->clear();
    ui->Language->addItem("english");
    ui->Language->setCurrentIndex(0);
    foreach(QString str,dirContent)
    {
        QString lang=str.right(str.length()-9);
        lang=lang.left(lang.length()-3);
        QLocale loc(lang);
        ui->Language->addItem(loc.nativeLanguageName(),loc.name());
        if(loc.name()==locale)
        {
            ui->Language->setCurrentIndex(ui->Language->count()-1);
        }
    }
    dirContent = QDir(QApplication::applicationDirPath()+"/language").entryList(QStringList()<< "abc_*.txt", QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
    QString locale_abc=settings->value("localeABC",QLocale::system().name()).toString();
    ui->ABC->clear();
    ui->ABC->addItem("english");
    ui->ABC->setCurrentIndex(0);
    foreach(QString str,dirContent)
    {
        QString lang=str.right(str.length()-4);
        lang=lang.left(lang.length()-4);
        QLocale loc(lang);
        ui->ABC->addItem(loc.nativeLanguageName(),loc.name());
        if(loc.name()==locale_abc)
        {
            ui->ABC->setCurrentIndex(ui->ABC->count()-1);
        }
    }
    QString sAppDir=QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).first();
    ui->database_path->setText(settings->value("database_path",sAppDir+"/freeLib.sqlite").toString());

    {
        int id=settings->value("DefaultExport",0).toInt();
        if(id<ui->ExportName->count())
        {
            ui->ExportName->setCurrentIndex(id);
        }
    }

    connect(ui->Language,SIGNAL(currentIndexChanged(int)),this,SLOT(ChangeLanguage()));
    connect(ui->ABC,SIGNAL(currentIndexChanged(int)),this,SLOT(ChangeLanguage()));
    connect(ui->ExportName->lineEdit(),SIGNAL(editingFinished()),this,SLOT(ExportNameChanged()));
    UpdateWebExportList();
    on_proxy_type_currentIndexChanged(ui->proxy_type->currentIndex());
    on_HTTP_need_pasword_clicked();
}

void SettingsDlg::UpdateWebExportList()
{
    QSettings* settings=GetSettings();
    int index=0;
    if(ui->httpExport->count()>1)
        index=ui->httpExport->currentIndex();
    while(ui->httpExport->count()>1)
        ui->httpExport->removeItem(1);
    ui->httpExport->setCurrentIndex(0);

    if(index==0)
        index=settings->value("httpExport",0).toInt();
    for(int i=0;i<ui->stackedWidget->count();i++)
    {
        ui->httpExport->addItem(ui->ExportName->itemText(i));
        if((i+1)==index)
            ui->httpExport->setCurrentIndex(i+1);
    }
}

void SettingsDlg::btnDirPath()
{
    QDir::setCurrent(QFileInfo(ui->database_path->text()).absolutePath());
    QString dir=QFileDialog::getExistingDirectory(this,tr("Select book`s directory"));
    if(!dir.isEmpty())
    {
        ui->dirForBrowsing->setText(dir);
    }
}
void SettingsDlg::btnDBPath()
{
    QDir::setCurrent(QFileInfo(ui->database_path->text()).absolutePath());
    QString dir=QFileDialog::getExistingDirectory(this,tr("Select database directory"));
    if(!dir.isEmpty() && dir!=ui->database_path->text())
    {
        QSettings *settings=GetSettings();
        settings->setValue("database_path",ui->database_path->text());
        settings->sync();
        QSqlDatabase dbase=QSqlDatabase::database("libdb",false);
        if (dbase.isOpen())
            dbase.close();
        if(QFile().rename(RelativeToAbsolutePath(ui->database_path->text()),dir+"/freeLib.sqlite"))
        {
            QFile().remove(RelativeToAbsolutePath(ui->database_path->text()));
            ui->database_path->setText(dir+"/freeLib.sqlite");
            dbase.setDatabaseName(RelativeToAbsolutePath(ui->database_path->text()));
            if(!dbase.open())
            {
                app->closingDown();
            }
        }
    }
}

void SettingsDlg::ChangePort(int i)
{
    if(i>0)
    {
        ui->OPDS->setText(QString("<a href='http://localhost:%1/opds'>http://localhost:%1/opds</a>").arg(QString::number(i)));
        ui->HTTP->setText(QString("<a href='http://localhost:%1'>http://localhost:%1</a>").arg(QString::number(i)));
    }
    else
    {
        QSettings *settings=GetSettings();
        settings->setValue("OPDS_enable",ui->OPDS_enable->isChecked());
        settings->sync();
    }
    emit ChangingPort(i);
}
void SettingsDlg::ChangeLanguage()
{
    disconnect(ui->Language,SIGNAL(currentIndexChanged(int)),this,SLOT(ChangeLanguage()));
    disconnect(ui->ABC,SIGNAL(currentIndexChanged(int)),this,SLOT(ChangeLanguage()));
    QSettings *settings=GetSettings();
    settings->setValue("localeUI",ui->Language->currentData().toString());
    settings->setValue("localeABC",ui->ABC->currentData().toString());
    settings->sync();
    SetLocale();
    ui->retranslateUi(this);
    LoadSettings();
    emit ChangingLanguage();
}

void SettingsDlg::btnOK()
{
    if(ui->settings_to_file->isChecked())
    {
#ifdef Q_OS_MAC
//        QString dir=app->applicationDirPath()+"/../../../freeLib";
        QString dir=app->applicationDirPath();
#else
        QString dir=app->applicationDirPath();
#endif
        QDir().mkpath(dir);
        QFile cfg(dir+"/freeLib.cfg");
        if(!cfg.exists())
        {
            cfg.open(QFile::WriteOnly);
            cfg.close();
        }
    }
    else
    {
#ifdef Q_OS_MAC
        QFile().remove(app->applicationDirPath()+"/freeLib.cfg");
//        QFile().remove(app->applicationDirPath()+"/../../../freeLib/freeLib.cfg");
//        if(QDir(app->applicationDirPath()+"/../../../freeLib").entryList(QDir::Files).count()==0)
//            QDir(app->applicationDirPath()+"/../../../freeLib").removeRecursively();
#else
        QFile().remove(app->applicationDirPath()+"/freeLib.cfg");
#endif
    }


    QSettings* settings=GetSettings(false,true);


    settings->setValue("SettingsWnd/geometry",saveGeometry());
    settings->setValue("SettingsWndExportList/geometry",ui->ExportList->saveGeometry());
    settings->setValue("SettingsWndExportList_headers/geometry",ui->ExportList->horizontalHeader()->saveState());
    settings->setValue("SettingsWndApplicationList/geometry",ui->ApplicationList->saveGeometry());
    settings->setValue("SettingsWndApplicationList_headers/geometry",ui->ApplicationList->horizontalHeader()->saveState());
    settings->setValue("CloseExpDlg",ui->CloseExpDlg->checkState()==Qt::Checked);
    settings->setValue("ShowDeleted",ui->ShowDeleted->checkState()==Qt::Checked);
    settings->setValue("use_tag",ui->use_tag->checkState()==Qt::Checked);
    settings->setValue("uncheck_export",ui->uncheck_export->checkState()==Qt::Checked);
    settings->setValue("store_position",ui->store_pos->isChecked());
    settings->setValue("OPDS_enable",ui->OPDS_enable->isChecked());
    settings->setValue("OPDS_port",ui->OPDS_port->value());
    settings->setValue("httpExport",ui->httpExport->currentIndex());
    settings->setValue("browseDir",ui->browseDir->isChecked());
    settings->setValue("dirForBrowsing",ui->dirForBrowsing->text());
    settings->setValue("no_splash",ui->splash->checkState()==Qt::Checked);
    settings->setValue("tray_icon",ui->trayIcon->currentIndex());
    settings->setValue("tray_color",ui->tray_color->currentIndex());
    settings->setValue("HTTP_need_pasword",ui->HTTP_need_pasword->isChecked());
    settings->setValue("HTTP_user",ui->HTTP_user->text());
    settings->setValue("HTTP_password",ui->HTTP_password->text());
    settings->setValue("srv_annotation",ui->srv_annotation->isChecked());
    settings->setValue("srv_covers",ui->srv_covers->isChecked());
    settings->setValue("books_per_page",ui->books_per_page->value());
    settings->setValue("extended_symbols",ui->extended_symbols->isChecked());

    settings->setValue("proxy_type",ui->proxy_type->currentIndex());
    settings->setValue("proxy_port",ui->proxy_port->value());
    settings->setValue("proxy_user",ui->proxy_user->text());
    settings->setValue("proxy_host",ui->proxy_host->text());
    settings->setValue("proxy_password",ui->proxy_password->text());

    SaveTools(settings);
    settings->beginWriteArray("application");
    for (int i = 0; i < ui->ApplicationList->rowCount(); ++i)
    {
        settings->setArrayIndex(i);
        settings->setValue("ext", ui->ApplicationList->item(i,0)->text());
        settings->setValue("app", ui->ApplicationList->item(i,1)->text());
    }
    settings->endArray();

    settings->beginWriteArray("export");
    for(int i=0;i<ui->stackedWidget->count();i++)
    {
        settings->setArrayIndex(i);
        ((ExportFrame*)ui->stackedWidget->widget(i))->Save(settings);
        settings->setValue("ExportName",ui->ExportName->itemText(i));
        settings->setValue("Default",ui->ExportName->itemData(i).toBool());
    }
    settings->endArray();

    settings->setValue("database_path",ui->database_path->text());
    settings->sync();
    setProxy();
}

void SettingsDlg::SaveTools(QSettings *settings)
{
    bool need_sync=false;
    if(!settings)
    {
        settings=GetSettings();
        need_sync=true;
    }
    settings->beginWriteArray("tools");
    for (int i = 0; i < ui->ExportList->rowCount(); ++i)
    {
        settings->setArrayIndex(i);
        settings->setValue("name", ui->ExportList->item(i,0)->text());
        settings->setValue("path", ui->ExportList->item(i,1)->text());
        settings->setValue("args", ui->ExportList->item(i,2)->text());
        settings->setValue("ext", ui->ExportList->item(i,3)->text());
    }
    settings->endArray();
    if(need_sync)
        settings->sync();
}

void SettingsDlg::AddExt()
{
    ui->ExportList->setRowCount(ui->ExportList->rowCount()+1);
    ui->ExportList->setItem(ui->ExportList->rowCount()-1,0,new QTableWidgetItem(""));
    ui->ExportList->setItem(ui->ExportList->rowCount()-1,1,new QTableWidgetItem(""));
    ui->ExportList->setItem(ui->ExportList->rowCount()-1,2,new QTableWidgetItem(""));
    ui->ExportList->setItem(ui->ExportList->rowCount()-1,3,new QTableWidgetItem(""));
}

void SettingsDlg::DelExt()
{
    if(ui->ExportList->selectedItems().count()==0)
        return;
    ui->ExportList->removeRow(ui->ExportList->selectedItems()[0]->row());
}

void SettingsDlg::AddApp()
{
    ui->ApplicationList->setRowCount(ui->ApplicationList->rowCount()+1);
    ui->ApplicationList->setItem(ui->ApplicationList->rowCount()-1,0,new QTableWidgetItem(""));
    ui->ApplicationList->setItem(ui->ApplicationList->rowCount()-1,1,new QTableWidgetItem(""));
}

void SettingsDlg::DelApp()
{
    if(ui->ApplicationList->selectedItems().count()==0)
        return;
    ui->ApplicationList->removeRow(ui->ApplicationList->selectedItems()[0]->row());
}


void SettingsDlg::on_AddExport_clicked()
{
    ExportFrame* frame=new ExportFrame(this);
    ui->stackedWidget->addWidget(frame);
    ui->ExportName->addItem(tr("Send to ..."),false);
    ui->ExportName->setCurrentIndex(ui->ExportName->count()-1);
    ui->DelExport->setDisabled(false);

    frame->Load(0);

    connect(frame,SIGNAL(ChangeTabIndex(int,int)),this,SLOT(on_ChangeExportFrameTab(int,int)));
    connect(this,SIGNAL(ChangingExportFrameTab(int,int)),frame,SLOT(SetTabIndex(int,int)));
    UpdateWebExportList();
}

void SettingsDlg::ExportNameChanged()
{
    ui->ExportName->setItemText(ui->ExportName->currentIndex(),ui->ExportName->lineEdit()->text());
    UpdateWebExportList();
}

void SettingsDlg::on_DelExport_clicked()
{
    if(ui->ExportName->count()<=1)
        return;
    if(QMessageBox::question(this,tr("Delete export settings"),tr("Are you sure you want to delete the current export settings?"),QMessageBox::Yes|QMessageBox::No,QMessageBox::NoButton)!=QMessageBox::Yes)
        return;
    ui->stackedWidget->removeWidget(ui->stackedWidget->widget(ui->ExportName->currentIndex()));
    ui->ExportName->removeItem(ui->ExportName->currentIndex());
    if(ui->ExportName->count()<=1)
        ui->DelExport->setDisabled(true);
    UpdateWebExportList();
}

void SettingsDlg::on_ExportName_currentIndexChanged(int index)
{
    ui->stackedWidget->setCurrentIndex(index);
    ui->DefaultExport->setChecked(ui->ExportName->currentData().toBool());
}

void SettingsDlg::on_ChangeExportFrameTab(int tab_id,int page_id)
{
    emit ChangingExportFrameTab(tab_id,page_id);
}

void SettingsDlg::on_DefaultExport_clicked()
{
    for(int i=0;i<ui->ExportName->count();i++)
        ui->ExportName->setItemData(i,false);
    ui->ExportName->setItemData(ui->ExportName->currentIndex(),ui->DefaultExport->isChecked());
}


void SettingsDlg::on_btnDefaultSettings_clicked()
{
    if(QMessageBox::question(this,tr("Load default"),tr("Are you sure you want to load the default settings?"),QMessageBox::Yes|QMessageBox::No,QMessageBox::No)==QMessageBox::Yes)
    {
        ResetToDefaultSettings();
        LoadSettings();
    }
}


void SettingsDlg::on_tabWidget_currentChanged(int index)
{
    if(ui->tabWidget->currentWidget()==ui->tab_export)
    {
        SaveTools();
        emit NeedUpdateTools();
    }
}

void SettingsDlg::on_proxy_type_currentIndexChanged(int index)
{
    ui->pl_1->setEnabled(index>0);
    ui->pl_3->setEnabled(index>0);
    ui->pl_4->setEnabled(index>0);
    ui->pl_5->setEnabled(index>0);
    ui->proxy_host->setEnabled(index>0);
    ui->proxy_password->setEnabled(index>0);
    ui->proxy_port->setEnabled(index>0);
    ui->proxy_user->setEnabled(index>0);
}

void SettingsDlg::on_browseDir_stateChanged(int checked)
{
    ui->dirForBrowsing->setEnabled(checked);
}

void SettingsDlg::on_trayIcon_currentIndexChanged(int index)
{
    index=ui->trayIcon->currentIndex();
    int color=ui->tray_color->currentIndex();
    emit ChangingTrayIcon(index,color);
    ui->tray_color->setEnabled(index>0);
}

void SettingsDlg::on_tray_color_currentIndexChanged(int)
{
    on_trayIcon_currentIndexChanged(ui->trayIcon->currentIndex());
}

void SettingsDlg::on_HTTP_need_pasword_clicked()
{
    ui->p_password->setEnabled(ui->HTTP_need_pasword->isChecked());
    ui->p_user->setEnabled(ui->HTTP_need_pasword->isChecked());
    ui->HTTP_password->setEnabled(ui->HTTP_need_pasword->isChecked());
    ui->HTTP_user->setEnabled(ui->HTTP_need_pasword->isChecked());
}

void SettingsDlg::on_btnSaveExport_clicked()
{
    QString file_name=QFileDialog::getSaveFileName(this,tr("Save profile"),
        (ui->ExportName->currentText()),"freeLib export (*.fle)");
    if(file_name.isEmpty())
        return;
    QSettings set(QStandardPaths::writableLocation(QStandardPaths::TempLocation)+"/export.ini",QSettings::IniFormat);
    QStringList fonts_list=((ExportFrame*)ui->stackedWidget->currentWidget())->Save(&set,false);
    set.sync();
    QuaZip zip(file_name);
    zip.open(QuaZip::mdCreate);
    QuaZipFile zip_file(&zip);

    zip_file.open(QIODevice::WriteOnly, QuaZipNewInfo("export.ini", set.fileName()));
    QFile file(set.fileName());
    file.open(QIODevice::ReadOnly);
    zip_file.write(file.readAll());
    file.close();
    zip_file.close();

    QSettings *settings=GetSettings();
    QString HomeDir="";
    if(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).count()>0)
        HomeDir=QStandardPaths::standardLocations(QStandardPaths::HomeLocation).at(0);
    QString db_path=QFileInfo(settings->value("database_path",HomeDir+"/freeLib/freeLib.sqlite").toString()).absolutePath()+"/fonts";
    foreach (QString font, fonts_list)
    {
        QString font_file=font;//set.value("font").toString();
        if(QFile::exists(QApplication::applicationDirPath()+"/xsl/fonts/"+font_file))
        {
            font_file=QApplication::applicationDirPath()+"/xsl/fonts/"+font_file;
        }
        else
        {
            if(QFile::exists(db_path+"/"+font_file))
            {
                font_file=db_path+"/"+font_file;
            }
            else
            {
                if(!QFile::exists(font_file))
                    font_file="";
            }
        }
        if(!font_file.isEmpty())
        {
            zip_file.open(QIODevice::WriteOnly, QuaZipNewInfo("Fonts/"+QFileInfo(font_file).fileName(), font_file));
            file.setFileName(font_file);
            file.open(QIODevice::ReadOnly);
            zip_file.write(file.readAll());
            file.close();
            zip_file.close();
        }
    }
    zip.close();
}

void SettingsDlg::on_btnOpenExport_clicked()
{
    QString file_name=QFileDialog::getOpenFileName(this,tr("Open profile"),QString(),"freeLib export (*.fle)");
    if(file_name.isEmpty())
        return;
    int result=QMessageBox::question(this,tr("Load profile"),
            tr("How to load profile?"),
            tr("Replace current"),tr("Load to new"),tr("Cancel"),1,2);
    if(result==2)
        return;
    QuaZip zip(file_name);
    zip.open(QuaZip::mdUnzip);

    QSettings *settings=GetSettings();
    QString HomeDir="";
    if(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).count()>0)
        HomeDir=QStandardPaths::standardLocations(QStandardPaths::HomeLocation).at(0);
    QString db_path=QFileInfo(settings->value("database_path",HomeDir+"/freeLib/freeLib.sqlite").toString()).absolutePath()+"/fonts";
    QDir().mkpath(db_path);
    QStringList files=zip.getFileNameList();

    foreach(QString ffile,files)
    {

        QFileInfo fi(ffile);
        if(fi.path()=="Fonts")
        {
            if(QFile::exists(QApplication::applicationDirPath()+"/xsl/fonts/"+fi.fileName()))
                continue;
            if(QFile::exists(db_path+"/"+fi.fileName()))
                continue;

            {
                QString font_name=QStandardPaths::writableLocation(QStandardPaths::TempLocation)+"/"+fi.fileName();
                SetCurrentZipFileName(&zip,ffile);
                QuaZipFile zip_file(&zip);
                zip_file.open(QIODevice::ReadOnly);
                QFile font_file(font_name);
                font_file.remove();
                font_file.open(QFile::WriteOnly);
                font_file.write(zip_file.readAll());
                font_file.close();

                QFile::copy(font_name,db_path+"/"+fi.fileName());
            }
        }
    }

    SetCurrentZipFileName(&zip,"export.ini");
    QuaZipFile zip_file(&zip);
    zip_file.open(QIODevice::ReadOnly);
    QString ini_name=QStandardPaths::writableLocation(QStandardPaths::TempLocation)+"/export.ini";
    QFile file(ini_name);
    file.remove();
    file.open(QFile::WriteOnly);
    file.write(zip_file.readAll());
    file.close();
    QSettings in_settings(ini_name,QSettings::IniFormat);

    if(result==1)
    {
        ExportFrame* frame=new ExportFrame(this);
        ui->stackedWidget->addWidget(frame);
        frame->Load(&in_settings);
        ui->ExportName->addItem(QFileInfo(file_name).completeBaseName(),false);
        connect(frame,SIGNAL(ChangeTabIndex(int,int)),this,SLOT(on_ChangeExportFrameTab(int,int)));
        connect(this,SIGNAL(ChangingExportFrameTab(int,int)),frame,SLOT(SetTabIndex(int,int)));
        connect(this,SIGNAL(NeedUpdateTools()),frame,SLOT(UpdateToolComboBox()));
        ui->ExportName->setCurrentIndex(ui->ExportName->count()-1);
    }
    else
    {
        ((ExportFrame*)ui->stackedWidget->currentWidget())->Load(&in_settings);
    }


    zip_file.close();
}
