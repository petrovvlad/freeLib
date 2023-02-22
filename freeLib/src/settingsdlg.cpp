#define QT_USE_QSTRINGBUILDER
#include "settingsdlg.h"
#include "ui_settingsdlg.h"

#include <QToolButton>
#include <QStringBuilder>
#include <QMessageBox>

#include "quazip/quazip/quazip.h"
#include "quazip/quazip/quazipfile.h"

#include "fontframe.h"
#include "exportframe.h"
#include "config-freelib.h"



SettingsDlg::SettingsDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDlg)
{
    ui->setupUi(this);
    options_ = options;

    QSettings *settings = GetSettings();
    if(settings->contains(QStringLiteral("SettingsWnd/geometry")))
        restoreGeometry(settings->value(QStringLiteral("SettingsWnd/geometry")).toByteArray());
    if(settings->contains(QStringLiteral("SettingsWndExportList/geometry")))
    {
        ui->ExportList->restoreGeometry(settings->value(QStringLiteral("SettingsWndExportList/geometry")).toByteArray());
        ui->ExportList->horizontalHeader()->restoreState(settings->value(QStringLiteral("SettingsWndExportList_headers/geometry")).toByteArray());
    }
    if(settings->contains(QStringLiteral("SettingsWndApplicationList/geometry")))
    {
        ui->ApplicationList->restoreGeometry(settings->value(QStringLiteral("SettingsWndApplicationList/geometry")).toByteArray());
        ui->ApplicationList->horizontalHeader()->restoreState(settings->value(QStringLiteral("SettingsWndApplicationList_headers/geometry")).toByteArray());
    }

    ui->tabWidget->setCurrentIndex(0);

    bool darkTheme = palette().color(QPalette::Window).lightness() < 127;
    QString sIconsPath = QLatin1String(":/img/icons/") + (darkTheme ?QLatin1String("dark/") :QLatin1String("light/"));
    ui->AddExport->setIcon(QIcon::fromTheme(QStringLiteral("list-add"), QIcon(sIconsPath + QLatin1String("plus.svg"))));
    ui->DelExport->setIcon(QIcon::fromTheme(QStringLiteral("list-remove"), QIcon(sIconsPath + QLatin1String("minus.svg"))));
    ui->btnOpenExport->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    ui->btnSaveExport->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));

    ui->ApplicationList->setColumnWidth(0, 100);
    ui->ApplicationList->setColumnWidth(1, 400);
    ui->ExportList->setColumnWidth(0, 100);
    ui->ExportList->setColumnWidth(1, 250);
    ui->ExportList->setColumnWidth(2, 150);
    //Колонка «имя выходного файла» временно спрятана
    ui->ExportList->hideColumn(3);


    QStringList dirContent = QDir(QApplication::applicationDirPath() + QLatin1String("/translations")).entryList(QStringList() << QStringLiteral("language_*.qm"), QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
    if(dirContent.isEmpty())
        dirContent = QDir(FREELIB_DATA_DIR + QLatin1String("/translations")).entryList(QStringList() << QStringLiteral("language_*.qm"), QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
    ui->Language->clear();
    ui->Language->addItem(QStringLiteral("english"), "en_US");
    ui->Language->setCurrentIndex(0);
    for(const QString &str: qAsConst(dirContent))
    {
        QString lang = str.right(str.length()-9);
        lang = lang.left(lang.length()-3);
        QLocale loc(lang);
        ui->Language->addItem(loc.nativeLanguageName(), loc.name());
    }
    dirContent = QDir(QStringLiteral(":/language")).entryList(QStringList()<< QStringLiteral("abc_*.txt"), QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
    ui->ABC->clear();
    ui->ABC->addItem(QStringLiteral("english"));
    ui->ABC->setCurrentIndex(0);

    for(const QString &str: qAsConst(dirContent))
    {
        QString lang = str.right(str.length() - 4);
        lang = lang.left(lang.length() - 4);
        QLocale loc(lang);
        ui->ABC->addItem(loc.nativeLanguageName(), lang);
    }

    ui->ApplicationList->setItemDelegateForColumn(1, new FileItemDelegate(this));

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &SettingsDlg::btnOK);
    connect(ui->DelExp, &QAbstractButton::clicked, this, &SettingsDlg::DelExt);
    connect(ui->AddExp, &QAbstractButton::clicked, this, &SettingsDlg::AddExt);
    connect(ui->DelApp, &QAbstractButton::clicked, this, &SettingsDlg::DelApp);
    connect(ui->AddApp, &QAbstractButton::clicked, this, &SettingsDlg::AddApp);
    connect(ui->OPDS_port, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &SettingsDlg::ChangePort);
    connect(ui->OPDS_enable, &QCheckBox::stateChanged, this, &SettingsDlg::onOpdsEnable);
    connect(ui->AddExport, &QToolButton::clicked, this, &SettingsDlg::onAddExportClicked);
    connect(ui->DelExport, &QPushButton::clicked, this, &SettingsDlg::onDelExportClicked);
    connect(ui->ExportName, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SettingsDlg::onExportNameCurrentIndexChanged);
    connect(ui->DefaultExport, &QCheckBox::clicked, this, &SettingsDlg::onDefaultExportClicked);
    connect(ui->btnDefaultSettings, &QPushButton::clicked, this, &SettingsDlg::onBtnDefaultSettingsClicked);
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &SettingsDlg::onTabWidgetCurrentChanged);
    connect(ui->proxy_type, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SettingsDlg::onProxyTypeCurrentIndexChanged);
    connect(ui->browseDir, &QCheckBox::stateChanged, this, &SettingsDlg::onBrowseDirStateChanged);
    connect(ui->trayIcon, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SettingsDlg::onTrayIconCurrentIndexChanged);
    connect(ui->tray_color, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SettingsDlg::onTrayColorCurrentIndexChanged);
    connect(ui->HTTP_need_pasword, &QCheckBox::clicked, this, &SettingsDlg::onHTTPneedPaswordClicked);
    connect(ui->btnSaveExport, &QToolButton::clicked, this, &SettingsDlg::onBtnSaveExportClicked);
    connect(ui->btnOpenExport, &QToolButton::clicked, this, &SettingsDlg::onBtnOpenExportClicked);
    connect(ui->ABC, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SettingsDlg::onChangeAlphabetCombobox);
    connect(ui->Language, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SettingsDlg::ChangeLanguage);
    connect(ui->ExportName->lineEdit(), &QLineEdit::editingFinished, this, &SettingsDlg::ExportNameChanged);

    QToolButton* btnDBPath = new QToolButton(this);
    btnDBPath->setFocusPolicy(Qt::NoFocus);
    btnDBPath->setCursor(Qt::ArrowCursor);
    btnDBPath->setText(tr("Move to ..."));
    QHBoxLayout*  layout = new QHBoxLayout(ui->database_path);
    layout->addWidget(btnDBPath, 0, Qt::AlignRight);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    connect(btnDBPath, &QAbstractButton::clicked, this, &SettingsDlg::btnDBPath);

    QToolButton* btnDirPath = new QToolButton(this);
    btnDirPath->setFocusPolicy(Qt::NoFocus);
    btnDirPath->setCursor(Qt::ArrowCursor);
    btnDirPath->setText(QStringLiteral("..."));
    layout = new QHBoxLayout(ui->dirForBrowsing);
    layout->addWidget(btnDirPath, 0, Qt::AlignRight);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    connect(btnDirPath, &QAbstractButton::clicked, this, &SettingsDlg::btnDirPath);

    LoadSettings();
    UpdateWebExportList();
    onProxyTypeCurrentIndexChanged(ui->proxy_type->currentIndex());
    onHTTPneedPaswordClicked();

    ChangePort(ui->OPDS_port->value());
    onOpdsEnable(ui->OPDS_enable->checkState());
}

SettingsDlg::~SettingsDlg()
{
    delete ui;
}

void SettingsDlg::LoadSettings()
{

    int count = ui->Language->count();
    for(int i = 0; i<count; ++i){
        if(ui->Language->itemData(i).toString() == options_.sUiLanguageName){
            ui->Language->setCurrentIndex(i);
            break;
        }
    }

    count = ui->ABC->count();
    for(int i = 0; i<count; ++i){
        if(ui->ABC->itemData(i).toString() == options_.sAlphabetName){
            ui->ABC->setCurrentIndex(i);
            break;
        }
    }

    ui->ShowDeleted->setChecked(options_.bShowDeleted);
    ui->use_tag->setChecked(options_.bUseTag);
    ui->splash->setChecked(!options_.bShowSplash);
    ui->store_pos->setChecked(options_.bStorePosition);
    ui->trayIcon->setCurrentIndex(options_.nIconTray);
    ui->tray_color->setEnabled(options_.nIconTray > 0);
    ui->tray_color->setCurrentIndex(options_.nTrayColor);
    ui->database_path->setText(options_.sDatabasePath);

#ifdef Q_OS_MAC
//    ui->settings_to_file->setChecked(QFileInfo(app->applicationDirPath()+"/../../../freeLib/freeLib.cfg").exists());
    ui->settings_to_file->setChecked(QFileInfo(app->applicationDirPath()+"/freeLib.cfg").exists());
#else
    ui->settings_to_file->setChecked(QFileInfo::exists(QApplication::applicationDirPath() + QLatin1String("/freeLib.cfg")));
#endif
    ui->CloseExpDlg->setChecked(options.bCloseDlgAfterExport);
    ui->uncheck_export->setChecked(options.bUncheckAfterExport);
#ifdef Q_OS_WINDOWS
    ui->extended_symbols->hide();
#endif

    ui->extended_symbols->setChecked(options_.bExtendedSymbols);
    ui->OPDS_enable->setChecked(options_.bOpdsEnable);
    ui->OPDS_port->setValue(options_.nOpdsPort);
    ui->browseDir->setChecked(options_.bBrowseDir);
    ui->dirForBrowsing->setText(options_.sDirForBrowsing);
    onBrowseDirStateChanged(options_.bBrowseDir);
    ui->HTTP_need_pasword->setChecked(options_.bOpdsNeedPassword);
    ui->HTTP_user->setText(options_.sOpdsUser);
    ui->HTTP_password->setText(options_.sOpdsPassword);
    ui->srv_annotation->setChecked(options_.bOpdsShowAnotation);
    ui->srv_covers->setChecked(options_.bOpdsShowCover);
    ui->books_per_page->setValue(options_.nOpdsBooksPerPage);
    ui->proxy_type->setCurrentIndex(options_.nProxyType);
    ui->proxy_port->setValue(options_.nProxyPort);
    ui->proxy_host->setText(options_.sProxyHost);
    ui->proxy_password->setText(options_.sProxyPassword);
    ui->proxy_user->setText(options_.sProxyUser);

    ui->ApplicationList->setRowCount(options_.applications.count());
    auto iApp = options_.applications.constBegin();
    int index = 0;
    while(iApp != options_.applications.constEnd()){
        ui->ApplicationList->setItem(index, 0, new QTableWidgetItem(iApp.key()));
        ui->ApplicationList->setItem(index, 1, new QTableWidgetItem(iApp.value()));
        ++iApp;
        ++index;
    }

    ui->ExportList->setRowCount(options_.tools.count());
    auto iTool = options_.tools.constBegin();
    index=0;
    while(iTool != options_.tools.constEnd()){
        ui->ExportList->setItem(index, 0, new QTableWidgetItem(iTool.key()));
        ui->ExportList->setItem(index, 1, new QTableWidgetItem(iTool->sPath));
        ui->ExportList->setItem(index, 2, new QTableWidgetItem(iTool->sArgs));
        ui->ExportList->setItem(index, 3, new QTableWidgetItem(iTool->sExt));
        ++index;
        ++iTool;
    }

    ui->ExportName->clear();
    while(ui->stackedWidget->count() > 0)
        ui->stackedWidget->removeWidget(ui->stackedWidget->widget(0));
    int iDefault = 0;
    count = options_.vExportOptions.count();
    for(int i=0; i<count; i++)
    {
        ExportFrame* frame = new ExportFrame(this);
        ui->stackedWidget->addWidget(frame);
        const ExportOptions *pExportOptions = &options_.vExportOptions.at(i);
        frame->Load(pExportOptions);
        ui->ExportName->addItem(pExportOptions->sName, pExportOptions->bDefault);
        if(pExportOptions->bDefault)
            iDefault = i;
        connect(frame, &ExportFrame::ChangeTabIndex, this, &SettingsDlg::onChangeExportFrameTab);
        connect(this, &SettingsDlg::ChangingExportFrameTab, frame, &ExportFrame::SetTabIndex);
        connect(this, &SettingsDlg::NeedUpdateTools, frame, [=](){frame->UpdateToolComboBox();});
    }
    ui->DelExport->setEnabled(count > 1);
    ui->ExportName->setCurrentIndex(iDefault);
}

void SettingsDlg::UpdateWebExportList()
{
    int index = 0;
    if(ui->httpExport->count() > 1)
        index = ui->httpExport->currentIndex();
    while(ui->httpExport->count() > 1)
        ui->httpExport->removeItem(1);
    ui->httpExport->setCurrentIndex(0);

    if(index == 0)
        index = options_.nHttpExport;
    for(int i=0; i<ui->stackedWidget->count(); i++)
    {
        ui->httpExport->addItem(ui->ExportName->itemText(i));
        if((i+1) == index)
            ui->httpExport->setCurrentIndex(i + 1);
    }
}

void SettingsDlg::reject()
{
    // Возврат к исхдному языку интерфеса.
    if(options_.sUiLanguageName != options.sUiLanguageName){
        SetLocale(options.sUiLanguageName);
        emit ChangingLanguage();
    }
    // Возврат к исхдному алфавиту.
    if(options_.sAlphabetName != options.sAlphabetName){
        emit ChangeAlphabet(options.sAlphabetName);
    }

    if(options_.nIconTray != options.nIconTray || options_.nTrayColor != options.nTrayColor){
        emit ChangingTrayIcon(options.nIconTray, options.nTrayColor);
    }
    QDialog::reject();
}

void SettingsDlg::btnDirPath()
{
    QDir::setCurrent(QFileInfo(ui->database_path->text()).absolutePath());
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select book`s directory"));
    if(!dir.isEmpty())
    {
        ui->dirForBrowsing->setText(dir);
    }
}
void SettingsDlg::btnDBPath()
{
    QDir::setCurrent(QFileInfo(ui->database_path->text()).absolutePath());
    QString dir=QFileDialog::getExistingDirectory(this, tr("Select database directory"));
    if(!dir.isEmpty() && dir != ui->database_path->text())
    {
        options.sDatabasePath = ui->database_path->text().trimmed();
        QSqlDatabase dbase = QSqlDatabase::database(QStringLiteral("libdb"), false);
        if (dbase.isOpen())
            dbase.close();
        if(QFile().rename(RelativeToAbsolutePath(ui->database_path->text()), dir + QLatin1String("/freeLib.sqlite")))
        {
            QFile().remove(RelativeToAbsolutePath(ui->database_path->text()));
            ui->database_path->setText(dir + QLatin1String("/freeLib.sqlite"));
            dbase.setDatabaseName(RelativeToAbsolutePath(ui->database_path->text()));
            if(!dbase.open())
            {
                QApplication::closingDown();
            }
        }
    }
}

void SettingsDlg::ChangePort(int i)
{
    ui->OPDS->setText(QStringLiteral("<a href='http://localhost:%1/opds'>http://localhost:%1/opds</a>").arg(i));
    ui->HTTP->setText(QStringLiteral("<a href='http://localhost:%1'>http://localhost:%1</a>").arg(i));
}

void SettingsDlg::ChangeLanguage()
{
    options_.sUiLanguageName = ui->Language->currentData().toString();
    SetLocale(options_.sUiLanguageName);
    ui->retranslateUi(this);
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
        QString dir = QApplication::applicationDirPath();
#endif
        QDir().mkpath(dir);
        QFile cfg(dir + QLatin1String("/freeLib.cfg"));
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
        QFile().remove(QApplication::applicationDirPath() + QLatin1String("/freeLib.cfg"));
#endif
    }
    QSettings *settings = GetSettings(true);

    options.sUiLanguageName = options_.sUiLanguageName;
    options.sAlphabetName = options_.sAlphabetName;
    options.sDatabasePath = ui->database_path->text().trimmed();
    options.bShowDeleted = ui->ShowDeleted->isChecked();
    options.bUseTag = ui->use_tag->isChecked();
    options.bShowSplash = !ui->splash->isChecked();
    options.bStorePosition = ui->store_pos->isChecked();
    options.nIconTray = ui->trayIcon->currentIndex();
    options.bCloseDlgAfterExport = ui->CloseExpDlg->isChecked();
    options.bUncheckAfterExport = ui->uncheck_export->isChecked();
    options.bExtendedSymbols = ui->extended_symbols->isChecked();
    options.bOpdsEnable = ui->OPDS_enable->isChecked();
    options.bBrowseDir = ui->browseDir->isChecked();
    options.sDirForBrowsing = ui->dirForBrowsing->text().trimmed();
    options.nHttpExport = ui->httpExport->currentIndex();
    options.bOpdsNeedPassword = ui->HTTP_need_pasword->isChecked();
    options.bOpdsShowAnotation = ui->srv_annotation->isChecked();
    options.bOpdsShowCover = ui->srv_covers->isChecked();
    options.sOpdsUser = ui->HTTP_user->text();
    options.sOpdsPassword = ui->HTTP_password->text();
    options.nOpdsPort = ui->OPDS_port->value();
    options.nOpdsBooksPerPage = ui->books_per_page->value();
    options.nProxyType = ui->proxy_type->currentIndex();
    options.nProxyPort = ui->proxy_port->value();
    options.sProxyHost = ui->proxy_host->text();
    options.sProxyUser = ui->proxy_user->text();
    options.sProxyPassword = ui->proxy_password->text();

    options.applications.clear();
    for (int i = 0; i < ui->ApplicationList->rowCount(); ++i)
    {
        QString sExt = ui->ApplicationList->item(i, 0)->text();
        QString sApp  = ui->ApplicationList->item(i, 1)->text();
        options.applications[sExt] = sApp;
    }
    SaveTools();

    options.vExportOptions.clear();
    options.vExportOptions.resize(ui->stackedWidget->count());
    for(int i=0; i<ui->stackedWidget->count(); i++)
    {
        ExportOptions* pExportOptions = &options.vExportOptions[i];
        qobject_cast<ExportFrame*>(ui->stackedWidget->widget(i))->Save(pExportOptions);
        pExportOptions->sName = ui->ExportName->itemText(i);
        pExportOptions->bDefault = ui->ExportName->itemData(i).toBool();

    }
    options.Save(settings);

    settings->setValue(QStringLiteral("SettingsWnd/geometry"), saveGeometry());
    settings->setValue(QStringLiteral("SettingsWndExportList/geometry"), ui->ExportList->saveGeometry());
    settings->setValue(QStringLiteral("SettingsWndExportList_headers/geometry"), ui->ExportList->horizontalHeader()->saveState());
    settings->setValue(QStringLiteral("SettingsWndApplicationList/geometry"), ui->ApplicationList->saveGeometry());
    settings->setValue(QStringLiteral("SettingsWndApplicationList_headers/geometry"), ui->ApplicationList->horizontalHeader()->saveState());
    setProxy();
}

void SettingsDlg::SaveTools()
{
    options.tools.clear();
    for(int i = 0; i < ui->ExportList->rowCount(); ++i){
        QString sName = ui->ExportList->item(i, 0)->text();
        QString sPath = ui->ExportList->item(i, 1)->text();
        QString sArgs = ui->ExportList->item(i, 2)->text();
        QString sExt = ui->ExportList->item(i, 3)->text();
        options.tools[sName].sPath = sPath;
        options.tools[sName].sArgs = sArgs;
        options.tools[sName].sExt = sExt;
    }
}

void SettingsDlg::AddExt()
{
    ui->ExportList->setRowCount(ui->ExportList->rowCount()+1);
    ui->ExportList->setItem(ui->ExportList->rowCount()-1, 0, new QTableWidgetItem());
    ui->ExportList->setItem(ui->ExportList->rowCount()-1, 1, new QTableWidgetItem());
    ui->ExportList->setItem(ui->ExportList->rowCount()-1, 2, new QTableWidgetItem());
    ui->ExportList->setItem(ui->ExportList->rowCount()-1, 3, new QTableWidgetItem());
}

void SettingsDlg::DelExt()
{
    if(ui->ExportList->selectedItems().count() == 0)
        return;
    ui->ExportList->removeRow(ui->ExportList->selectedItems()[0]->row());
}

void SettingsDlg::AddApp()
{
    ui->ApplicationList->setRowCount(ui->ApplicationList->rowCount() + 1);
    ui->ApplicationList->setItem(ui->ApplicationList->rowCount()-1, 0, new QTableWidgetItem());
    ui->ApplicationList->setItem(ui->ApplicationList->rowCount()-1, 1, new QTableWidgetItem());
}

void SettingsDlg::DelApp()
{
    if(ui->ApplicationList->selectedItems().count() == 0)
        return;
    ui->ApplicationList->removeRow(ui->ApplicationList->selectedItems()[0]->row());
}


void SettingsDlg::onAddExportClicked()
{
    ExportOptions exportOptions;
    exportOptions.setDefault(tr("Send to ..."), QStringLiteral("-"), false);
    ExportFrame* frame=new ExportFrame(this);
    ui->stackedWidget->addWidget(frame);
    ui->ExportName->addItem(exportOptions.sName, false);
    ui->ExportName->setCurrentIndex(ui->ExportName->count() - 1);
    ui->DelExport->setDisabled(false);
    frame->Load(&exportOptions);

    connect(frame, &ExportFrame::ChangeTabIndex, this, &SettingsDlg::onChangeExportFrameTab);
    connect(this, &SettingsDlg::ChangingExportFrameTab, frame, &ExportFrame::SetTabIndex);
    UpdateWebExportList();
}

void SettingsDlg::ExportNameChanged()
{
    ui->ExportName->setItemText(ui->ExportName->currentIndex(), ui->ExportName->lineEdit()->text());
    UpdateWebExportList();
}

void SettingsDlg::onDelExportClicked()
{
    if(ui->ExportName->count() <= 1)
        return;
    if(QMessageBox::question(this, tr("Delete export settings"), tr("Are you sure you want to delete the current export settings?"),
                             QMessageBox::Yes | QMessageBox::No, QMessageBox::NoButton) != QMessageBox::Yes)
    {
        return;
    }
    ui->stackedWidget->removeWidget(ui->stackedWidget->widget(ui->ExportName->currentIndex()));
    ui->ExportName->removeItem(ui->ExportName->currentIndex());
    if(ui->ExportName->count() <= 1)
        ui->DelExport->setDisabled(true);
    UpdateWebExportList();
}

void SettingsDlg::onExportNameCurrentIndexChanged(int index)
{
    ui->stackedWidget->setCurrentIndex(index);
    ui->DefaultExport->setChecked(ui->ExportName->currentData().toBool());
}

void SettingsDlg::onChangeExportFrameTab(int tab_id, int page_id)
{
    emit ChangingExportFrameTab(tab_id, page_id);
}

void SettingsDlg::onDefaultExportClicked()
{
    for(int i=0; i<ui->ExportName->count(); i++)
        ui->ExportName->setItemData(i, false);
    ui->ExportName->setItemData(ui->ExportName->currentIndex(), ui->DefaultExport->isChecked());
}


void SettingsDlg::onBtnDefaultSettingsClicked()
{
    if(QMessageBox::question(this, tr("Load default"), tr("Are you sure you want to load the default settings?"),
                             QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
    {
        options_.setDefault();
        LoadSettings();
    }
}


void SettingsDlg::onTabWidgetCurrentChanged(int /*index*/)
{
    if(ui->tabWidget->currentWidget() == ui->tab_export)
    {
        SaveTools();
        emit NeedUpdateTools();
    }
}

void SettingsDlg::onProxyTypeCurrentIndexChanged(int index)
{
    bool bEnable = index > 0;
    ui->pl_1->setEnabled(bEnable);
    ui->pl_3->setEnabled(bEnable);
    ui->pl_4->setEnabled(bEnable);
    ui->pl_5->setEnabled(bEnable);
    ui->proxy_host->setEnabled(bEnable);
    ui->proxy_password->setEnabled(bEnable);
    ui->proxy_port->setEnabled(bEnable);
    ui->proxy_user->setEnabled(bEnable);
}

void SettingsDlg::onBrowseDirStateChanged(int checked)
{
    ui->dirForBrowsing->setEnabled(checked);
}

void SettingsDlg::onTrayIconCurrentIndexChanged(int index)
{
    options_.nIconTray = index;
    emit ChangingTrayIcon(options_.nIconTray, options_.nTrayColor);
    ui->tray_color->setEnabled(options_.nIconTray > 0);
}

void SettingsDlg::onTrayColorCurrentIndexChanged(int index)
{
    options_.nTrayColor = index;
    emit ChangingTrayIcon(options_.nIconTray, options_.nTrayColor);
}

void SettingsDlg::onHTTPneedPaswordClicked()
{
    ui->p_password->setEnabled(ui->HTTP_need_pasword->isChecked());
    ui->p_user->setEnabled(ui->HTTP_need_pasword->isChecked());
    ui->HTTP_password->setEnabled(ui->HTTP_need_pasword->isChecked());
    ui->HTTP_user->setEnabled(ui->HTTP_need_pasword->isChecked());
}

void SettingsDlg::onBtnSaveExportClicked()
{
    QString file_name = QFileDialog::getSaveFileName(this, tr("Save profile"),
        (ui->ExportName->currentText()), QStringLiteral("freeLib export (*.fle)"));
    if(file_name.isEmpty())
        return;
    QSettings set(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QLatin1String("/export.ini"), QSettings::IniFormat);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    set.setIniCodec("UTF-8");
#endif
    ExportOptions exportOptions;

    QStringList fonts_list = qobject_cast<ExportFrame*>(ui->stackedWidget->currentWidget())->Save(&exportOptions);
    exportOptions.sName = ui->ExportName->currentText();
    exportOptions.Save(&set, false);
    set.sync();
    QuaZip zip(file_name);
    zip.open(QuaZip::mdCreate);
    QuaZipFile zip_file(&zip);

    zip_file.open(QIODevice::WriteOnly, QuaZipNewInfo(QStringLiteral("export.ini"), set.fileName()));
    QFile file(set.fileName());
    file.open(QIODevice::ReadOnly);
    zip_file.write(file.readAll());
    file.close();
    zip_file.close();

    QString HomeDir;
    if(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).count() > 0)
        HomeDir = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).at(0);
    QString db_path = QFileInfo(options.sDatabasePath).absolutePath() + QLatin1String("/fonts");
    foreach (QString font, fonts_list)
    {
        QString font_file = font;
        if(QFile::exists(QApplication::applicationDirPath() + QLatin1String("/xsl/fonts/") + font_file))
        {
            font_file = QApplication::applicationDirPath() + QLatin1String("/xsl/fonts/") + font_file;
        }
        else
        {
            if(QFile::exists(db_path + QLatin1String("/") + font_file))
            {
                font_file = db_path + QLatin1String("/") + font_file;
            }
            else if(QFile::exists(FREELIB_DATA_DIR + QLatin1String("/fonts") + font_file))
            {
                font_file = FREELIB_DATA_DIR + QLatin1String("/fonts") + font_file;
            }
            else
            {
                if(!QFile::exists(font_file))
                    font_file = QLatin1String("");
            }
        }
        if(!font_file.isEmpty())
        {
            zip_file.open(QIODevice::WriteOnly, QuaZipNewInfo(QLatin1String("Fonts/") + QFileInfo(font_file).fileName(), font_file));
            file.setFileName(font_file);
            file.open(QIODevice::ReadOnly);
            zip_file.write(file.readAll());
            file.close();
            zip_file.close();
        }
    }
    zip.close();
}

void SettingsDlg::onBtnOpenExportClicked()
{
    QString file_name=QFileDialog::getOpenFileName(this, tr("Open profile"), QString() , QStringLiteral("freeLib export (*.fle)"));
    if(file_name.isEmpty())
        return;
    int result = QMessageBox::question(this, tr("Load profile"),
            tr("How to load profile?"),
            tr("Replace current"), tr("Load to new"), tr("Cancel"), 1, 2);
    if(result == 2)
        return;
    QuaZip zip(file_name);
    zip.open(QuaZip::mdUnzip);

    QString HomeDir;
    if(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).count() > 0)
        HomeDir = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).at(0);
    QString db_path = QFileInfo(options.sDatabasePath).absolutePath() + QLatin1String("/fonts");
    QDir().mkpath(db_path);
    QStringList files = zip.getFileNameList();

    foreach(const QString &ffile,files)
    {

        QFileInfo fi(ffile);
        if(fi.path() == QStringLiteral("Fonts"))
        {
            if(QFile::exists(QApplication::applicationDirPath() + QLatin1String("/xsl/fonts/") + fi.fileName()))
                continue;
            if(QFile::exists(db_path + QLatin1String("/") + fi.fileName()))
                continue;
            if(QFile::exists(FREELIB_DATA_DIR + QLatin1String("/fonts/") + fi.fileName()))
                continue;


            {
                QString font_name = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QLatin1String("/") + fi.fileName();
                SetCurrentZipFileName(&zip, ffile);
                QuaZipFile zip_file(&zip);
                zip_file.open(QIODevice::ReadOnly);
                QFile font_file(font_name);
                font_file.remove();
                font_file.open(QFile::WriteOnly);
                font_file.write(zip_file.readAll());
                font_file.close();

                QFile::copy(font_name,db_path + QLatin1String("/") + fi.fileName());
            }
        }
    }

    SetCurrentZipFileName(&zip, QStringLiteral("export.ini"));
    QuaZipFile zip_file(&zip);
    zip_file.open(QIODevice::ReadOnly);
    QString ini_name = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QLatin1String("/export.ini");
    QFile file(ini_name);
    file.remove();
    file.open(QFile::WriteOnly);
    file.write(zip_file.readAll());
    file.close();
    QSettings in_settings(ini_name, QSettings::IniFormat);

    if(result == 1)
    {

        ExportFrame* frame = new ExportFrame(this);
        ui->stackedWidget->addWidget(frame);
        ExportOptions exportOptions;
        exportOptions.Load(&in_settings);
        frame->Load(&exportOptions);
        ui->ExportName->addItem(QFileInfo(file_name).completeBaseName(),false);
        connect(frame, &ExportFrame::ChangeTabIndex, this, &SettingsDlg::onChangeExportFrameTab);
        connect(this, &SettingsDlg::ChangingExportFrameTab, frame, &ExportFrame::SetTabIndex);
        connect(this, &SettingsDlg::NeedUpdateTools, frame, [=](){frame->UpdateToolComboBox();});
        ui->ExportName->setCurrentIndex(ui->ExportName->count() - 1);
    }
    else
    {
        ExportOptions exportOptions;
        exportOptions.Load(&in_settings);
        qobject_cast<ExportFrame*>(ui->stackedWidget->currentWidget())->Load(&exportOptions);
    }

    zip_file.close();
}

void SettingsDlg::onChangeAlphabetCombobox(int /*index*/)
{
    options_.sAlphabetName = ui->ABC->currentData().toString();
    emit ChangeAlphabet(options_.sAlphabetName);
}

void SettingsDlg::onOpdsEnable(int state)
{
    bool bOpdsEnable = (state == Qt::Checked);
    ui->OPDS_port->setEnabled(bOpdsEnable);
    ui->label_20->setEnabled(bOpdsEnable);
    ui->OPDS->setEnabled(bOpdsEnable);
    ui->label_21->setEnabled(bOpdsEnable);
    ui->HTTP->setEnabled(bOpdsEnable);
    ui->label_23->setEnabled(bOpdsEnable);
    ui->httpExport->setEnabled(bOpdsEnable);
    ui->label_2->setEnabled(bOpdsEnable);
    ui->browseDir->setEnabled(bOpdsEnable);
    ui->dirForBrowsing->setEnabled(bOpdsEnable && ui->browseDir->isChecked());
    ui->label_3->setEnabled(bOpdsEnable);
    ui->HTTP_need_pasword->setEnabled(bOpdsEnable);
    ui->HTTP_user->setEnabled(bOpdsEnable && ui->HTTP_need_pasword->isChecked());
    ui->p_user->setEnabled(bOpdsEnable && ui->HTTP_need_pasword->isChecked());
    ui->HTTP_password->setEnabled(bOpdsEnable && ui->HTTP_need_pasword->isChecked());
    ui->p_password->setEnabled(bOpdsEnable && ui->HTTP_need_pasword->isChecked());
    ui->srv_covers->setEnabled(bOpdsEnable);
    ui->srv_annotation->setEnabled(bOpdsEnable);
    ui->books_per_page->setEnabled(bOpdsEnable);
    ui->label_5->setEnabled(bOpdsEnable);
}
