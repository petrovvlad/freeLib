#include "settingsdlg.h"
#include "ui_settingsdlg.h"

#include <QStringBuilder>
#include <QMessageBox>
#include <QStandardItemModel>

#ifdef QUAZIP_STATIC
#include "quazip/quazip/quazipfile.h"
#else
#include <quazip/quazipfile.h>
#endif

#include "exportframe.h"
#include "conversionframe.h"
#include "config-freelib.h"
#include "utilites.h"

SettingsDlg::SettingsDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDlg)
{
    ui->setupUi(this);
    options_ = g::options;
    ui->stackedPages->setCurrentIndex(0);

    auto settings = GetSettings();
    if(settings->contains(u"SettingsWnd/geometry"_s))
        restoreGeometry(settings->value(u"SettingsWnd/geometry"_s).toByteArray());
    if(settings->contains(u"SettingsWndExportList/geometry"_s))
    {
        ui->ExportList->restoreGeometry(settings->value(u"SettingsWndExportList/geometry"_s).toByteArray());
        ui->ExportList->horizontalHeader()->restoreState(settings->value(u"SettingsWndExportList_headers/geometry"_s).toByteArray());
    }
    if(settings->contains(u"SettingsWndApplicationList/geometry"_s))
    {
        ui->ApplicationList->restoreGeometry(settings->value(u"SettingsWndApplicationList/geometry"_s).toByteArray());
        ui->ApplicationList->horizontalHeader()->restoreState(settings->value(u"SettingsWndApplicationList_headers/geometry"_s).toByteArray());
    }

    ui->AddExport->setIcon(themedIcon(u"list-add"_s));
    ui->DelExport->setIcon(themedIcon(u"list-remove"_s));
    ui->btnOpenExport->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    ui->btnSaveExport->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));

    ui->ApplicationList->setColumnWidth(0, 100);
    ui->ApplicationList->setColumnWidth(1, 400);
    ui->ExportList->setColumnWidth(0, 100);
    ui->ExportList->setColumnWidth(1, 250);
    ui->ExportList->setColumnWidth(2, 150);

    QStringList dirContent = QDir(QApplication::applicationDirPath() + u"/translations"_s).entryList({u"language_*.qm"_s}, QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
    if(dirContent.isEmpty())
        dirContent = QDir(FREELIB_DATA_DIR + u"/translations"_s).entryList({u"language_*.qm"_s}, QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
    ui->Language->clear();
    ui->Language->addItem(u"english"_s, "en_US");
    ui->Language->setCurrentIndex(0);
    for(const QString &str: std::as_const(dirContent))
    {
        QString lang = str.right(str.length()-9);
        lang = lang.left(lang.length()-3);
        QLocale loc(lang);
        ui->Language->addItem(loc.nativeLanguageName(), loc.name());
    }
    dirContent = QDir(u":/language"_s).entryList({u"abc_*.txt"_s}, QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
    ui->ABC->clear();
    ui->ABC->addItem(u"english"_s);
    ui->ABC->setCurrentIndex(0);

    for(const QString &str: std::as_const(dirContent))
    {
        QString lang = str.right(str.length() - 4);
        lang = lang.left(lang.length() - 4);
        QLocale loc(lang);
        ui->ABC->addItem(loc.nativeLanguageName(), lang);
    }

    ui->ApplicationList->setItemDelegateForColumn(1, new FileItemDelegate(this));
#if QT_VERSION > QT_VERSION_CHECK(6, 3, 0)
    ui->listFontComboBox->setSampleTextForSystem(QFontDatabase::Cyrillic, u" "_s);
    ui->annotationFontComboBox->setSampleTextForSystem(QFontDatabase::Cyrillic, u" "_s);
#endif

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &SettingsDlg::btnOK);
    connect(ui->DelExp, &QAbstractButton::clicked, this, &SettingsDlg::DelExt);
    connect(ui->AddExp, &QAbstractButton::clicked, this, &SettingsDlg::AddExt);
    connect(ui->DelApp, &QAbstractButton::clicked, this, &SettingsDlg::DelApp);
    connect(ui->AddApp, &QAbstractButton::clicked, this, &SettingsDlg::AddApp);
    connect(ui->AddExport, &QToolButton::clicked, this, &SettingsDlg::onAddExport);
    connect(ui->DelExport, &QPushButton::clicked, this, &SettingsDlg::onDelExport);
    connect(ui->ExportName, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SettingsDlg::onExportNameCurrentIndexChanged);
    connect(ui->ConversionName, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SettingsDlg::onExportNameCurrentIndexChanged/*[](){}*/);
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    connect(ui->DefaultExport, &QCheckBox::stateChanged, this, &SettingsDlg::onDefaultExportChanged);
#else
    connect(ui->DefaultExport, &QCheckBox::checkStateChanged, this, &SettingsDlg::onDefaultExportChanged);
#endif
    connect(ui->btnDefaultSettings, &QPushButton::clicked, this, &SettingsDlg::onBtnDefaultSettingsClicked);
    connect(ui->trayIcon, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SettingsDlg::onTrayIconCurrentIndexChanged);
    connect(ui->tray_color, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SettingsDlg::onTrayColorCurrentIndexChanged);
    connect(ui->btnSaveExport, &QToolButton::clicked, this, &SettingsDlg::onBtnSaveExportClicked);
    connect(ui->btnOpenExport, &QToolButton::clicked, this, &SettingsDlg::onBtnOpenExportClicked);
    connect(ui->ABC, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SettingsDlg::onChangeAlphabetCombobox);
    connect(ui->Language, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SettingsDlg::ChangeLanguage);
    connect(ui->ExportName->lineEdit(), &QLineEdit::editingFinished, this, &SettingsDlg::ExportNameChanged);
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    connect(ui->useSystemFonts, &QCheckBox::stateChanged, this, &SettingsDlg::onUseSystemFontsChanged);
#else
    connect(ui->useSystemFonts, &QCheckBox::checkStateChanged, this, &SettingsDlg::onUseSystemFontsChanged);
#endif
    connect(ui->sidebarFontComboBox, &QFontComboBox::currentFontChanged, this, &SettingsDlg::onSidebarFontChanged);
    connect(ui->listFontComboBox, &QFontComboBox::currentFontChanged, this, &SettingsDlg::onListFontChanged);
    connect(ui->annotationFontComboBox, &QFontComboBox::currentFontChanged, this, &SettingsDlg::onAnnotationFontChanged);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    connect(ui->sidebarFontSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsDlg::onSidebarSizeFontChanged);
    connect(ui->listFontSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsDlg::onListSizeFontChanged);
    connect(ui->annotationFontSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsDlg::onAnnotationSizeFontChanged);
#else
    connect(ui->sidebarFontSpinBox, &QSpinBox::valueChanged, this, &SettingsDlg::onSidebarSizeFontChanged);
    connect(ui->listFontSpinBox, &QSpinBox::valueChanged, this, &SettingsDlg::onListSizeFontChanged);
    connect(ui->annotationFontSpinBox, &QSpinBox::valueChanged, this, &SettingsDlg::onAnnotationSizeFontChanged);
#endif

    ui->treeWidget->setColumnWidth(0, 300);
    auto item = new QTreeWidgetItem(ui->treeWidget);
    item->setText(0, tr("Common"));
    item->setData(0, Qt::UserRole, 0);
    item = new QTreeWidgetItem(ui->treeWidget);
    item->setText(0, tr("Export"));
    item->setData(0, Qt::UserRole, 1);
    itemConversion = new QTreeWidgetItem(item);
    itemConversion->setText(0, tr("fb2 to MOBI/AZW3/EPUB conversion"));
    itemConversion->setData(0, Qt::UserRole, 2);
#ifdef USE_HTTSERVER
    item = new QTreeWidgetItem(ui->treeWidget);
    item->setText(0, tr("Network"));
    item->setData(0, Qt::UserRole, 3);
#endif
    item = new QTreeWidgetItem(ui->treeWidget);
    item->setText(0, tr("Program"));
    item->setData(0, Qt::UserRole, 4);

    item = new QTreeWidgetItem(ui->treeWidget);
    item->setText(0, tr("Postprocessing tools"));
    item->setData(0, Qt::UserRole, 5);

    item = new QTreeWidgetItem(ui->treeWidget);
    item->setText(0, tr("Fonts"));
    item->setData(0, Qt::UserRole, 6);

    ui->treeWidget->setCurrentItem(ui->treeWidget->topLevelItem(0));

    connect(ui->treeWidget, &QTreeWidget::itemSelectionChanged, this, &SettingsDlg::onChangePage);

    QToolButton* btnDBPath = new QToolButton(this);
    btnDBPath->setFocusPolicy(Qt::NoFocus);
    btnDBPath->setCursor(Qt::ArrowCursor);
    btnDBPath->setText(tr("Move to ..."));
    QHBoxLayout*  layout = new QHBoxLayout(ui->database_path);
    layout->addWidget(btnDBPath, 0, Qt::AlignRight);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    connect(btnDBPath, &QAbstractButton::clicked, this, &SettingsDlg::btnDBPath);


#ifdef USE_HTTSERVER
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    connect(ui->OPDS_enable, &QCheckBox::stateChanged, this, &SettingsDlg::onOpdsEnable);
    connect(ui->HTTP_need_pasword, &QCheckBox::stateChanged, this, &SettingsDlg::onHttpNeedPaswordChanged);
#else
    connect(ui->OPDS_enable, &QCheckBox::checkStateChanged, this, &SettingsDlg::onOpdsEnable);
    connect(ui->HTTP_need_pasword, &QCheckBox::checkStateChanged, this, &SettingsDlg::onHttpNeedPaswordChanged);
#endif //QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    connect(ui->proxy_type, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SettingsDlg::onProxyTypeCurrentIndexChanged);
#endif //USE_HTTSERVER

    LoadSettings();
#ifdef USE_HTTSERVER
    onProxyTypeCurrentIndexChanged(ui->proxy_type->currentIndex());
#endif //USE_HTTSERVER

}

SettingsDlg::~SettingsDlg()
{
    delete ui;
}

void SettingsDlg::showConversionNameItem(int nIndex, bool bShow)
{
    QStandardItemModel *model = qobject_cast<QStandardItemModel *>(ui->ConversionName->model());
    if (model) {
        QStandardItem *item = model->item(nIndex);
        if (item) {
            if(bShow)
                item->setFlags(item->flags() | Qt::ItemIsEnabled);
            else
                item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        }
    }
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
    if(g::bTray){
        ui->trayIcon->setCurrentIndex(2);
        ui->trayIcon->setEnabled(false);
    }else
        ui->trayIcon->setCurrentIndex(options_.nIconTray);
    ui->tray_color->setEnabled(options_.nIconTray > 0);
    ui->tray_color->setCurrentIndex(options_.bTrayColor ?0 :1);
    ui->database_path->setText(options_.sDatabasePath);

#ifdef Q_OS_MAC
//    ui->settings_to_file->setChecked(QFileInfo(app->applicationDirPath()+"/../../../freeLib/freeLib.cfg").exists());
    ui->settings_to_file->setChecked(QFileInfo(app->applicationDirPath()+"/freeLib.cfg").exists());
#else
    ui->settings_to_file->setChecked(QFileInfo::exists(QApplication::applicationDirPath() + u"/freeLib.cfg"_s));
#endif
    ui->CloseExpDlg->setChecked(options_.bCloseDlgAfterExport);
    ui->uncheck_export->setChecked(options_.bUncheckAfterExport);
#ifdef Q_OS_WINDOWS
    ui->extended_symbols->hide();
#endif

    ui->extended_symbols->setChecked(options_.bExtendedSymbols);
#ifdef USE_HTTSERVER
    ui->OPDS_enable->setChecked(options_.bOpdsEnable);
    ui->portHttp->setValue(options_.nHttpPort);
    ui->baseUrl->setText(options_.sBaseUrl);
    ui->HTTP_need_pasword->setChecked(options_.bOpdsNeedPassword);
    ui->HTTP_user->setText(options_.sOpdsUser);
    if(options_.baOpdsPasswordSalt.isEmpty())
        options_.baOpdsPasswordSalt = Options::generateSalt();
    ui->HTTP_password->setPassword(options_.baOpdsPasswordSalt, options_.baOpdsPasswordHash);
    ui->srv_annotation->setChecked(options_.bOpdsShowAnotation);
    ui->srv_covers->setChecked(options_.bOpdsShowCover);
    ui->books_per_page->setValue(options_.nOpdsBooksPerPage);
    ui->proxy_type->setCurrentIndex(options_.nProxyType);
    ui->proxy_port->setValue(options_.nProxyPort);
    ui->proxy_host->setText(options_.sProxyHost);
    ui->proxy_password->setText(options_.sProxyPassword);
    ui->proxy_user->setText(options_.sProxyUser);
#endif

    ui->ApplicationList->setRowCount(options_.applications.size());
    int index = 0;
    for(const auto &iApp :options_.applications){
        ui->ApplicationList->setItem(index, 0, new QTableWidgetItem(iApp.first));
        ui->ApplicationList->setItem(index, 1, new QTableWidgetItem(iApp.second));
        ++index;
    }

    ui->ExportList->setRowCount(options_.tools.size());
    index=0;
    for(const auto &iTool :options_.tools){
        ui->ExportList->setItem(index, 0, new QTableWidgetItem(iTool.first));
        ui->ExportList->setItem(index, 1, new QTableWidgetItem(iTool.second.sPath));
        ui->ExportList->setItem(index, 2, new QTableWidgetItem(iTool.second.sArgs));
        ++index;
    }

    ui->ExportName->clear();
    ui->ConversionName->clear();
    while(ui->stackedWidget->count() > 0){
        auto frame = ui->stackedWidget->widget(0);
        ui->stackedWidget->removeWidget(frame);
        frame->deleteLater();
    }
    int iDefault = 0;
    count = options_.vExportOptions.size();
    for(int i=0; i<count; i++)
    {
        ExportFrame* frame = new ExportFrame(this);
        ui->stackedWidget->addWidget(frame);
        const ExportOptions *pExportOptions = &options_.vExportOptions.at(i);
        frame->Load(pExportOptions);
        ui->ExportName->addItem(pExportOptions->sName, pExportOptions->bDefault);
        auto format = pExportOptions->format;
        ui->ConversionName->addItem(pExportOptions->sName, i);
        if(format != epub && format != azw3 && format != mobi && format != mobi7)
        {
            showConversionNameItem(i, false);
        }

        if(pExportOptions->bDefault)
            iDefault = i;
        connect(this, &SettingsDlg::NeedUpdateTools, frame, [=](){frame->UpdateToolComboBox();});
        connect(frame, &ExportFrame::OutputFormatChanged, this, &SettingsDlg::onExportFormatChanged);
#ifdef USE_HTTSERVER
        connect(frame, &ExportFrame::UseForHttpChanged, this, &SettingsDlg::onUseForHttpChanged);
#endif

        auto conversionFrame = new ConversionFrame(this);
        ui->stackedWidgetConversion->addWidget(conversionFrame);
        conversionFrame->Load(pExportOptions);
        connect(conversionFrame, &ConversionFrame::changeTabIndex, this, &SettingsDlg::onChangeConversionFrameTab);
        connect(this, &SettingsDlg::ChangingConversionFrameTab, conversionFrame, &ConversionFrame::setCurrentTab);
    }
    ui->DelExport->setEnabled(count > 1);
    ui->ExportName->setCurrentIndex(iDefault);
    updateKindelegenWarring(iDefault);
#ifdef USE_HTTSERVER
    onExportFormatChanged();
#endif

    ui->useSystemFonts->setChecked(options_.bUseSytemFonts);
    QFont font = QFont(options_.sSidebarFontFamaly);
    ui->sidebarFontComboBox->setCurrentFont(font);
    ui->sidebarFontSpinBox->setValue(options_.nSidebarFontSize);
    font = QFont(options_.sBooksListFontFamaly);
    ui->listFontComboBox->setCurrentFont(font);
    ui->listFontSpinBox->setValue(options_.nBooksListFontSize);
    font = QFont(options_.sAnnotationFontFamaly);
    ui->annotationFontComboBox->setCurrentFont(font);
    ui->annotationFontSpinBox->setValue(options_.nAnnotationFontSize);
}

void SettingsDlg::updateKindelegenWarring(int iExportOpton)
{
    if( iExportOpton >= 0 && iExportOpton < options_.vExportOptions.size() )
    {
        const ExportOptions *pExportOptions = &options_.vExportOptions.at(iExportOpton);
        bool bKindlgenLableVisible = !kindlegenInstalled() &&
                                     (pExportOptions->format == mobi ||
                                      pExportOptions->format == azw3 ||
                                      pExportOptions->format == mobi7);
        ui->label_kindlegen->setVisible(bKindlgenLableVisible);
    }
}

#ifdef USE_HTTSERVER
void SettingsDlg::onUseForHttpChanged(int state)
{
    auto currentFrame = qobject_cast<ExportFrame*>(ui->stackedWidget->currentWidget());
    bool bCurrentUseHttp = state == Qt::Checked; //currentFrame->getUseForHttp();
    if(bCurrentUseHttp){
        auto currentFormat = currentFrame->outputFormat();
        for(int i=0; i<ui->stackedWidget->count(); i++){
            auto frame = qobject_cast<ExportFrame*>(ui->stackedWidget->widget(i));
            auto format = frame->outputFormat();
            if(currentFormat == format && currentFrame != frame && frame->getUseForHttp())
                frame->setUseForHttp(false);
        }
    }
}
#endif

void SettingsDlg::reject()
{
    // Возврат к исхдному языку интерфеса.
    if(options_.sUiLanguageName != g::options.sUiLanguageName){
        ::setLocale(g::options.sUiLanguageName);
        emit ChangingLanguage();
    }
    // Возврат к исхдному алфавиту.
    if(options_.sAlphabetName != g::options.sAlphabetName){
        emit ChangeAlphabet(g::options.sAlphabetName);
    }

    if(options_.nIconTray != g::options.nIconTray || options_.bTrayColor != g::options.bTrayColor){
        emit ChangingTrayIcon(g::options.nIconTray, g::options.bTrayColor);
    }
    if(options_.bUseSytemFonts != g::options.bUseSytemFonts){
        QFont font(QGuiApplication::font());
        font.setFamily(g::options.sSidebarFontFamaly);
        font.setPointSize(g::options.nSidebarFontSize);
        emit ChangeSidebarFont(font);
        font.setFamily(g::options.sBooksListFontFamaly);
        font.setPointSize(g::options.nBooksListFontSize);
        emit ChangeListFont(font);
        font.setFamily(g::options.sAnnotationFontFamaly);
        font.setPointSize(g::options.nAnnotationFontSize);
        emit ChangeAnnotationFont(font);
    }
    if(options_.sSidebarFontFamaly != g::options.sSidebarFontFamaly || options_.nSidebarFontSize != g::options.nSidebarFontSize)  {
        QFont font(QGuiApplication::font());
        font.setFamily(g::options.sSidebarFontFamaly);
        font.setPointSize(g::options.nSidebarFontSize);
        emit ChangeSidebarFont(font);
    }
    if(options_.sAnnotationFontFamaly != g::options.sAnnotationFontFamaly || options_.nAnnotationFontSize != g::options.nAnnotationFontSize)  {
        QFont font(QGuiApplication::font());
        font.setFamily(g::options.sAnnotationFontFamaly);
        font.setPointSize(g::options.nAnnotationFontSize);
        emit ChangeAnnotationFont(font);
    }
    if(options_.sBooksListFontFamaly != g::options.sBooksListFontFamaly || options_.nBooksListFontSize != g::options.nBooksListFontSize)  {
        QFont font(QGuiApplication::font());
        font.setFamily(g::options.sBooksListFontFamaly);
        font.setPointSize(g::options.nBooksListFontSize);
        emit ChangeListFont(font);
    }
    QDialog::reject();
}

void SettingsDlg::btnDBPath()
{
    QDir::setCurrent(QFileInfo(ui->database_path->text()).absolutePath());
    QString dir=QFileDialog::getExistingDirectory(this, tr("Select database directory"));
    if(!dir.isEmpty() && dir != ui->database_path->text())
    {
        g::options.sDatabasePath = ui->database_path->text().trimmed();
        QSqlDatabase dbase = QSqlDatabase::database(u"libdb"_s, false);
        if (dbase.isOpen())
            dbase.close();
        if(QFile().rename(RelativeToAbsolutePath(ui->database_path->text()), dir + u"/freeLib.sqlite"_s))
        {
            QFile().remove(RelativeToAbsolutePath(ui->database_path->text()));
            ui->database_path->setText(dir + u"/freeLib.sqlite"_s);
            dbase.setDatabaseName(RelativeToAbsolutePath(ui->database_path->text()));
            if(!dbase.open())
            {
                QApplication::closingDown();
            }
        }
    }
}

void SettingsDlg::onChangePage()
{
    auto item = ui->treeWidget->currentItem();
    int page = item->data(0, Qt::UserRole).toInt();
    ui->stackedPages->setCurrentIndex(page);
}

void SettingsDlg::ChangeLanguage()
{
    options_.sUiLanguageName = ui->Language->currentData().toString();
    ::setLocale(options_.sUiLanguageName);
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
        QFile cfg(dir + u"/freeLib.cfg"_s);
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
        QFile().remove(QApplication::applicationDirPath() + u"/freeLib.cfg"_s);
#endif
    }
    auto settings = GetSettings(true);

    g::options.sUiLanguageName = options_.sUiLanguageName;
    g::options.sAlphabetName = options_.sAlphabetName;
    g::options.sDatabasePath = ui->database_path->text().trimmed();
    g::options.bShowDeleted = ui->ShowDeleted->isChecked();
    g::options.bUseTag = ui->use_tag->isChecked();
    g::options.bShowSplash = !ui->splash->isChecked();
    g::options.bStorePosition = ui->store_pos->isChecked();
    g::options.nIconTray = ui->trayIcon->currentIndex();
    g::options.bTrayColor = ui->tray_color->currentIndex() == 0;
    g::options.bCloseDlgAfterExport = ui->CloseExpDlg->isChecked();
    g::options.bUncheckAfterExport = ui->uncheck_export->isChecked();
    g::options.bExtendedSymbols = ui->extended_symbols->isChecked();
#ifdef USE_HTTSERVER
    g::options.bOpdsEnable = ui->OPDS_enable->isChecked();
    g::options.bOpdsNeedPassword = ui->HTTP_need_pasword->isChecked();
    g::options.bOpdsShowAnotation = ui->srv_annotation->isChecked();
    g::options.bOpdsShowCover = ui->srv_covers->isChecked();
    g::options.sOpdsUser = ui->HTTP_user->text();
    g::options.baOpdsPasswordHash = ui->HTTP_password->getPasswordHash();
    g::options.baOpdsPasswordSalt = ui->HTTP_password->getPasswordSalt();
    g::options.nHttpPort = ui->portHttp->value();
    g::options.sBaseUrl = ui->baseUrl->text();
    g::options.nOpdsBooksPerPage = ui->books_per_page->value();
    g::options.nProxyType = ui->proxy_type->currentIndex();
    g::options.nProxyPort = ui->proxy_port->value();
    g::options.sProxyHost = ui->proxy_host->text();
    g::options.sProxyUser = ui->proxy_user->text();
    g::options.sProxyPassword = ui->proxy_password->text();
#endif

    g::options.applications.clear();
    for (int i = 0; i < ui->ApplicationList->rowCount(); ++i)
    {
        QString sExt = ui->ApplicationList->item(i, 0)->text();
        QString sApp  = ui->ApplicationList->item(i, 1)->text();
        g::options.applications[sExt] = sApp;
    }
    SaveTools();

    g::options.vExportOptions.clear();
    g::options.vExportOptions.resize(ui->stackedWidget->count());
    for(int i=0; i<ui->stackedWidget->count(); i++)
    {
        ExportOptions* pExportOptions = &g::options.vExportOptions[i];
        qobject_cast<ExportFrame*>(ui->stackedWidget->widget(i))->Save(pExportOptions);
        qobject_cast<ConversionFrame*>(ui->stackedWidgetConversion->widget(i))->Save(pExportOptions);
        pExportOptions->sName = ui->ExportName->itemText(i);
        pExportOptions->bDefault = ui->ExportName->itemData(i).toBool();
    }

    g::options.bUseSytemFonts = options_.bUseSytemFonts;
    g::options.sSidebarFontFamaly = options_.sSidebarFontFamaly;
    g::options.nSidebarFontSize = options_.nSidebarFontSize;
    g::options.sBooksListFontFamaly = options_.sBooksListFontFamaly;
    g::options.nBooksListFontSize = options_.nBooksListFontSize;
    g::options.sAnnotationFontFamaly = options_.sAnnotationFontFamaly;
    g::options.nAnnotationFontSize = options_.nAnnotationFontSize;

    g::options.Save(settings);
    g::options.savePasswords();

    settings->setValue(u"SettingsWnd/geometry"_s, saveGeometry());
    settings->setValue(u"SettingsWndExportList/geometry"_s, ui->ExportList->saveGeometry());
    settings->setValue(u"SettingsWndExportList_headers/geometry"_s, ui->ExportList->horizontalHeader()->saveState());
    settings->setValue(u"SettingsWndApplicationList/geometry"_s, ui->ApplicationList->saveGeometry());
    settings->setValue(u"SettingsWndApplicationList_headers/geometry"_s, ui->ApplicationList->horizontalHeader()->saveState());
#ifdef USE_HTTSERVER
    setProxy();
#endif
}

void SettingsDlg::SaveTools()
{
    g::options.tools.clear();
    for(int i = 0; i < ui->ExportList->rowCount(); ++i){
        QString sName = ui->ExportList->item(i, 0)->text();
        QString sPath = ui->ExportList->item(i, 1)->text();
        QString sArgs = ui->ExportList->item(i, 2)->text();
        g::options.tools[sName].sPath = sPath;
        g::options.tools[sName].sArgs = sArgs;
    }
}

void SettingsDlg::AddExt()
{
    ui->ExportList->setRowCount(ui->ExportList->rowCount()+1);
    ui->ExportList->setItem(ui->ExportList->rowCount()-1, 0, new QTableWidgetItem());
    ui->ExportList->setItem(ui->ExportList->rowCount()-1, 1, new QTableWidgetItem());
    ui->ExportList->setItem(ui->ExportList->rowCount()-1, 2, new QTableWidgetItem());
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


void SettingsDlg::onAddExport()
{
    ExportOptions exportOptions;
    exportOptions.setDefault(tr("Send to ..."), asis, false);
#ifdef USE_HTTSERVER
    bool bUseForHttp = true;
    for(int i=0; i<ui->stackedWidget->count(); i++){
        auto frame = qobject_cast<ExportFrame*>(ui->stackedWidget->widget(i));
        if(frame->outputFormat() == asis && frame->getUseForHttp()){
            bUseForHttp = false;
            break;
        }
    }
    exportOptions.bUseForHttp = bUseForHttp;
#endif
    ExportFrame* frame = new ExportFrame(this);
    ui->stackedWidget->addWidget(frame);
    ui->ExportName->addItem(exportOptions.sName, false);
    ui->ExportName->setCurrentIndex(ui->ExportName->count() - 1);
    ui->DelExport->setDisabled(false);
    frame->Load(&exportOptions);
    connect(this, &SettingsDlg::NeedUpdateTools, frame, [=](){frame->UpdateToolComboBox();});
    connect(frame, &ExportFrame::OutputFormatChanged, this, &SettingsDlg::onExportFormatChanged);
#ifdef USE_HTTSERVER
    connect(frame, &ExportFrame::UseForHttpChanged, this, &SettingsDlg::onUseForHttpChanged);
#endif

    auto conversionFrame = new ConversionFrame(this);
    ui->stackedWidgetConversion->addWidget(conversionFrame);
    ui->ConversionName->addItem(exportOptions.sName);
    ui->ConversionName->setCurrentIndex(ui->ExportName->count() - 1);

    conversionFrame->Load(&exportOptions);
    connect(conversionFrame, &ConversionFrame::changeTabIndex, this, &SettingsDlg::onChangeConversionFrameTab);
    connect(this, &SettingsDlg::ChangingConversionFrameTab, conversionFrame, &ConversionFrame::setCurrentTab);
    itemConversion->setDisabled(true);
}

void SettingsDlg::ExportNameChanged()
{
    ui->ExportName->setItemText(ui->ExportName->currentIndex(), ui->ExportName->lineEdit()->text());
}

void SettingsDlg::onDelExport()
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
}

void SettingsDlg::onExportNameCurrentIndexChanged(int index)
{
    updateKindelegenWarring(index);
    ui->stackedWidget->setCurrentIndex(index);
    ui->stackedWidgetConversion->setCurrentIndex(index);
    ui->DefaultExport->setChecked(ui->ExportName->currentData().toBool());

    auto currentFrame = qobject_cast<ExportFrame*>(ui->stackedWidget->currentWidget());
    auto currentFormat = currentFrame->outputFormat();
    itemConversion->setDisabled(currentFormat != epub && currentFormat!=azw3 && currentFormat !=mobi && currentFormat !=mobi7);

    ui->ExportName->blockSignals(true);
    ui->ExportName->setCurrentIndex(index);
    ui->ExportName->blockSignals(false);
    ui->ConversionName->blockSignals(true);
    ui->ConversionName->setCurrentIndex(index);
    ui->ConversionName->blockSignals(false);
}

void SettingsDlg::onExportFormatChanged()
{
    auto currentFrame = qobject_cast<ExportFrame*>(ui->stackedWidget->currentWidget());
    auto index = ui->stackedWidget->currentIndex();
    auto currentFormat = currentFrame->outputFormat();
    bool bConversion  = currentFormat == epub || currentFormat == azw3 || currentFormat ==mobi || currentFormat == mobi7;
    itemConversion->setDisabled(!bConversion);
    showConversionNameItem(index, bConversion);

#ifdef USE_HTTSERVER
    bool bCurrentUseHttp = currentFrame->getUseForHttp();
    std::unordered_map<ExportFormat, uint> formatCount;
    for(int i=0; i<ui->stackedWidget->count(); i++){
        auto frame = qobject_cast<ExportFrame*>(ui->stackedWidget->widget(i));
        auto format = frame->outputFormat();
        formatCount[format]++;
        if(bCurrentUseHttp && frame != currentFrame && currentFormat == format && frame->getUseForHttp()){
            currentFrame->setUseForHttp(false);
            bCurrentUseHttp = false;
        }
    }
    if(formatCount[currentFormat] == 1)
        currentFrame->setUseForHttp(true);
#endif
}

void SettingsDlg::onChangeConversionFrameTab(int index)
{
    emit ChangingConversionFrameTab(index);
}

#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
void SettingsDlg::onDefaultExportChanged(int state)
#else
void SettingsDlg::onDefaultExportChanged(Qt::CheckState state)
#endif
{
    bool bChecked = (state == Qt::Checked);
    for(int i=0; i<ui->ExportName->count(); i++)
        ui->ExportName->setItemData(i, false);
    ui->ExportName->setItemData(ui->ExportName->currentIndex(), bChecked);
}

void SettingsDlg::onUseSystemFontsChanged(int state)
{
    bool bUnchecked = (state == Qt::Unchecked);
    ui->labelSideBar->setEnabled(bUnchecked);
    ui->sidebarFontComboBox->setEnabled(bUnchecked);
    ui->sidebarFontSpinBox->setEnabled(bUnchecked);
    ui->labelListFont->setEnabled(bUnchecked);
    ui->listFontComboBox->setEnabled(bUnchecked);
    ui->listFontSpinBox->setEnabled(bUnchecked);
    ui->labelAnnotationFont->setEnabled(bUnchecked);
    ui->annotationFontComboBox->setEnabled(bUnchecked);
    ui->annotationFontSpinBox->setEnabled(bUnchecked);

    QFont newFont = QGuiApplication::font();
    if(bUnchecked){
        newFont.setFamily(ui->sidebarFontComboBox->currentFont().family());
        newFont.setPointSize(ui->sidebarFontSpinBox->value());
    }
    emit ChangeSidebarFont(newFont);
    if(bUnchecked){
        newFont.setFamily(ui->listFontComboBox->currentFont().family());
        newFont.setPointSize(ui->listFontSpinBox->value());
    }
    emit ChangeListFont(newFont);
    if(bUnchecked){
        newFont.setFamily(ui->annotationFontComboBox->currentFont().family());
        newFont.setPointSize(ui->annotationFontSpinBox->value());
    }
    options_.bUseSytemFonts = !bUnchecked;
    emit ChangeAnnotationFont(newFont);
}

void SettingsDlg::onSidebarFontChanged(const QFont &font)
{
    QFont newFont = QGuiApplication::font();
    newFont.setFamily(font.family());
    newFont.setPointSize(ui->sidebarFontSpinBox->value());
    options_.sSidebarFontFamaly = font.family();
    emit ChangeSidebarFont(newFont);
}

void SettingsDlg::onSidebarSizeFontChanged(int i)
{
    QFont newFont = QGuiApplication::font();
    newFont.setFamily(ui->sidebarFontComboBox->currentFont().family());
    newFont.setPointSize(i);
    options_.nSidebarFontSize = i;
    emit ChangeSidebarFont(newFont);
}

void SettingsDlg::onListFontChanged(const QFont &font)
{
    QFont newFont = QGuiApplication::font();
    newFont.setFamily(font.family());
    newFont.setPointSize(ui->listFontSpinBox->value());
    options_.sBooksListFontFamaly = font.family();
    emit ChangeListFont(newFont);
}

void SettingsDlg::onListSizeFontChanged(int i)
{
    QFont newFont = QGuiApplication::font();
    newFont.setFamily(ui->listFontComboBox->currentFont().family());
    newFont.setPointSize(i);
    options_.nBooksListFontSize = i;
    emit ChangeListFont(newFont);
}

void SettingsDlg::onAnnotationFontChanged(const QFont &font)
{
    QFont newFont = QGuiApplication::font();
    newFont.setFamily(font.family());
    newFont.setPointSize(ui->annotationFontSpinBox->value());
    options_.sAnnotationFontFamaly = font.family();
    emit ChangeAnnotationFont(newFont);
}

void SettingsDlg::onAnnotationSizeFontChanged(int i)
{
    QFont newFont = QGuiApplication::font();
    newFont.setFamily(ui->annotationFontComboBox->currentFont().family());
    newFont.setPointSize(i);
    options_.nAnnotationFontSize = i;
    emit ChangeAnnotationFont(newFont);
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

#ifdef USE_HTTSERVER
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
#endif

void SettingsDlg::onTrayIconCurrentIndexChanged(int index)
{
    options_.nIconTray = index;
    emit ChangingTrayIcon(options_.nIconTray, options_.bTrayColor);
    ui->tray_color->setEnabled(options_.nIconTray > 0);
}

void SettingsDlg::onTrayColorCurrentIndexChanged(int index)
{
    options_.bTrayColor = index == 0;
    emit ChangingTrayIcon(options_.nIconTray, options_.bTrayColor);
}

#ifdef USE_HTTSERVER
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
void SettingsDlg::onHttpNeedPaswordChanged(int state)
#else
void SettingsDlg::onHttpNeedPaswordChanged(Qt::CheckState state)
#endif //QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
{
    bool bChecked = (state == Qt::Checked);
    ui->p_password->setEnabled(bChecked);
    ui->p_user->setEnabled(bChecked);
    ui->HTTP_password->setEnabled(bChecked);
    ui->HTTP_user->setEnabled(bChecked);
}
#endif //USE_HTTSERVER

void SettingsDlg::onBtnSaveExportClicked()
{
    QString file_name = QFileDialog::getSaveFileName(this, tr("Save profile"),
        (ui->ExportName->currentText()), u"freeLib export (*.fle)"_s);
    if(file_name.isEmpty()) [[unlikely]]
        return;
    auto settings = QSharedPointer<QSettings> (new QSettings(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + u"/export.ini"_s, QSettings::IniFormat));
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    settings->setIniCodec("UTF-8");
#endif
    ExportOptions exportOptions;

    const QStringList fonts_list = qobject_cast<ExportFrame*>(ui->stackedWidget->currentWidget())->Save(&exportOptions);
    exportOptions.sName = ui->ExportName->currentText();
    exportOptions.Save(settings, false);
    settings->sync();
    QuaZip zip(file_name);
    zip.open(QuaZip::mdCreate);
    QuaZipFile zip_file(&zip);

    zip_file.open(QIODevice::WriteOnly, QuaZipNewInfo(u"export.ini"_s, settings->fileName()));
    QFile file(settings->fileName());
    file.open(QIODevice::ReadOnly);
    zip_file.write(file.readAll());
    file.close();
    zip_file.close();

    QString db_path = QFileInfo(g::options.sDatabasePath).absolutePath() + u"/fonts"_s;
    for(const QString &font: fonts_list)
    {
        QString font_file = font;
        if(QFile::exists(QApplication::applicationDirPath() % u"/xsl/fonts/"_s % font_file))
        {
            font_file = QApplication::applicationDirPath() % u"/xsl/fonts/"_s % font_file;
        }
        else
        {
            if(QFile::exists(db_path % u"/"_s % font_file))
            {
                font_file = db_path % u"/"_s % font_file;
            }
            else if(QFile::exists(FREELIB_DATA_DIR % u"/fonts"_s % font_file))
            {
                font_file = FREELIB_DATA_DIR % u"/fonts"_s % font_file;
            }
            else
            {
                if(!QFile::exists(font_file))
                    font_file = u""_s;
            }
        }
        if(!font_file.isEmpty())
        {
            zip_file.open(QIODevice::WriteOnly, QuaZipNewInfo(u"Fonts/"_s + QFileInfo(font_file).fileName(), font_file));
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
    QString file_name = QFileDialog::getOpenFileName(this, tr("Open profile"), QString() , u"freeLib export (*.fle)"_s);
    if(file_name.isEmpty())
        return;
    QMessageBox msgBox = QMessageBox(QMessageBox::Question, tr("Load profile"), tr("How to load profile?"), QMessageBox::Cancel, this);
    msgBox.addButton(tr("Replace current"), QMessageBox::ActionRole);
    QPushButton *pButtonLoadNew = msgBox.addButton(tr("Load to new"), QMessageBox::ActionRole);
    if(msgBox.exec() == QMessageBox::Cancel)
        return;

    QuaZip zip(file_name);
    zip.open(QuaZip::mdUnzip);
    QString HomeDir;
    if(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).count() > 0)
        HomeDir = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).at(0);
    QString db_path = QFileInfo(g::options.sDatabasePath).absolutePath() + u"/fonts"_s;
    QDir().mkpath(db_path);
    const QStringList files = zip.getFileNameList();

    for(const QString &file: files)
    {

        QFileInfo fi(file);
        if(fi.path() == u"Fonts"_s)
        {
            if(QFile::exists(QApplication::applicationDirPath() % u"/xsl/fonts/"_s % fi.fileName()))
                continue;
            if(QFile::exists(db_path % u"/"_s % fi.fileName()))
                continue;
            if(QFile::exists(FREELIB_DATA_DIR % u"/fonts/"_s % fi.fileName()))
                continue;


            {
                QString font_name = QStandardPaths::writableLocation(QStandardPaths::TempLocation) % u"/"_s % fi.fileName();
                setCurrentZipFileName(&zip, file);
                QuaZipFile zip_file(&zip);
                zip_file.open(QIODevice::ReadOnly);
                QFile font_file(font_name);
                font_file.remove();
                font_file.open(QFile::WriteOnly);
                font_file.write(zip_file.readAll());
                font_file.close();

                QFile::copy(font_name, db_path % u"/"_s % fi.fileName());
            }
        }
    }

    setCurrentZipFileName(&zip, u"export.ini"_s);
    QuaZipFile zip_file(&zip);
    zip_file.open(QIODevice::ReadOnly);
    QString ini_name = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + u"/export.ini"_s;
    QFile file(ini_name);
    file.remove();
    file.open(QFile::WriteOnly);
    file.write(zip_file.readAll());
    file.close();
    auto settings = QSharedPointer<QSettings> (new QSettings(ini_name, QSettings::IniFormat));

    if(msgBox.clickedButton() == pButtonLoadNew)
    {
        ExportFrame* frame = new ExportFrame(this);
        ui->stackedWidget->addWidget(frame);
        ExportOptions exportOptions;
        exportOptions.Load(settings);
        frame->Load(&exportOptions);
        ui->ExportName->addItem(QFileInfo(file_name).completeBaseName(), false);
        connect(this, &SettingsDlg::NeedUpdateTools, frame, [=](){frame->UpdateToolComboBox();});
        ui->ExportName->setCurrentIndex(ui->ExportName->count() - 1);

        auto conversionFrame = new ConversionFrame(this);
        ui->stackedWidgetConversion->addWidget(conversionFrame);
        conversionFrame->Load(&exportOptions);
        connect(conversionFrame, &ConversionFrame::changeTabIndex, this, &SettingsDlg::onChangeConversionFrameTab);
        connect(this, &SettingsDlg::ChangingConversionFrameTab, conversionFrame, &ConversionFrame::setCurrentTab);
    }
    else
    {
        ExportOptions exportOptions;
        exportOptions.Load(settings);
        qobject_cast<ExportFrame*>(ui->stackedWidget->currentWidget())->Load(&exportOptions);
    }

    zip_file.close();
}

void SettingsDlg::onChangeAlphabetCombobox(int /*index*/)
{
    options_.sAlphabetName = ui->ABC->currentData().toString();
    emit ChangeAlphabet(options_.sAlphabetName);
}

#ifdef USE_HTTSERVER
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
void SettingsDlg::onOpdsEnable(int state)
#else
void SettingsDlg::onOpdsEnable(Qt::CheckState state)
#endif
{
    bool bOpdsEnable = (state == Qt::Checked);
    ui->portHttp->setEnabled(bOpdsEnable);
    ui->baseUrl->setEnabled(bOpdsEnable);
    ui->label_20->setEnabled(bOpdsEnable);
    ui->label_23->setEnabled(bOpdsEnable);
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
#endif
