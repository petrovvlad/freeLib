#define QT_USE_QSTRINGBUILDER
#include <iostream>
#include <QApplication>
#include <QDesktopWidget>
#include <QNetworkProxy>
#include <QLocale>
#include <QTranslator>
#include <QLibraryInfo>
#include <QSplashScreen>
#include <QPainter>
#include <QStringBuilder>
#include <QDir>
#include <QMessageBox>
#include <QSqlError>

#include "quazip/quazip/quazip.h"

#include "mainwindow.h"
#include "aboutdialog.h"
#include "common.h"
#include "utilites.h"
#include "config-freelib.h"
#include "opds_server.h"

uint idCurrentLib;
QTranslator* translator;
QTranslator* translator_qt;
QList<tag> tag_list;
QSettings *global_settings = nullptr;
bool bTray;
QSplashScreen *splash;
Options options;
opds_server *pOpds;

bool SetCurrentZipFileName(QuaZip *zip, const QString &name)
{
    bool result = zip->setCurrentFile(name, QuaZip::csInsensitive);
    if(!result)
    {
        zip->setFileNameCodec(QTextCodec::codecForName("IBM 866"));
        result = zip->setCurrentFile(name, QuaZip::csInsensitive);
    }
    return result;
}

QSettings* GetSettings(bool reopen)
{
    if(reopen && global_settings)
    {
        global_settings->sync();
        delete global_settings;
        global_settings = nullptr;
    }
    if(global_settings == nullptr){
        QString sFile = QApplication::applicationDirPath() + QLatin1String("/freeLib.cfg");
        if(QFile::exists(sFile))
            global_settings = new QSettings(sFile, QSettings::IniFormat);
        else
            global_settings = new QSettings();
        global_settings->setIniCodec("UTF-8");
    }
    return global_settings;
}

QNetworkProxy proxy;
void setProxy()
{
    proxy.setPort(options.nProxyPort);
    proxy.setHostName(options.sProxyHost);
    proxy.setPassword(options.sProxyPassword);
    proxy.setUser(options.sProxyUser);
    switch(options.nProxyType)
    {
    case 0:
        proxy.setType(QNetworkProxy::NoProxy);
        break;
    case 1:
        proxy.setType(QNetworkProxy::Socks5Proxy);
        break;
    case 2:
        proxy.setType(QNetworkProxy::HttpProxy);
        break;
    }
    QNetworkProxy::setApplicationProxy(proxy);
}

void SetLocale(const QString &sLocale)
{
    setlocale(LC_ALL, (sLocale + QLatin1String(".UTF-8")).toLatin1().data());
    QLocale::setDefault(QLocale(sLocale));

    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

    if(translator)
    {
        QApplication::removeTranslator(translator);
    }
    if(translator_qt)
    {
        QApplication::removeTranslator(translator_qt);
    }

    if(translator == nullptr)
        translator = new QTranslator();
    QString sQmFile = QStringLiteral("/translations/language_%1.qm").arg(sLocale.leftRef(2));
    QString sQmFileFull = QApplication::applicationDirPath() + sQmFile;
    if(!QFile::exists(sQmFileFull))
            sQmFileFull = FREELIB_DATA_DIR + sQmFile;
    if(translator->load(sQmFileFull))
    {
        QApplication::installTranslator(translator);
    }
    else
    {
        delete translator;
        translator = nullptr;
    }

    if(translator_qt == nullptr)
        translator_qt = new QTranslator();
    if(translator_qt->load(QStringLiteral("qtbase_%1.qm").arg(sLocale), QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
    {
        QApplication::installTranslator(translator_qt);
    }
    else
    {
        delete translator_qt;
        translator_qt = nullptr;
    }

    tag_list.clear();
    tag_list<<
               tag(QApplication::translate("SettingsDlg", "Top level captions"), QStringLiteral(".h0"), QStringLiteral("top_caption_font"), 140)<<
               tag(QApplication::translate("SettingsDlg", "Captions"), QStringLiteral(".h1,.h2,.h3,.h4,.h5,.h6"), QStringLiteral("caption_font"), 120)<<
               tag(QApplication::translate("SettingsDlg", "Dropcaps"), QStringLiteral("span.dropcaps"), QStringLiteral("dropcaps_font"), 300)<<
               tag(QApplication::translate("SettingsDlg", "Footnotes"), QStringLiteral(".inlinenote,.blocknote"), QStringLiteral("footnotes_font"), 80)<<
               tag(QApplication::translate("SettingsDlg", "Annotation"), QStringLiteral(".annotation"), QStringLiteral("annotation_font"), 100)<<
               tag(QApplication::translate("SettingsDlg", "Poems"), QStringLiteral(".poem"), QStringLiteral("poem_font"), 100)<<
               tag(QApplication::translate("SettingsDlg", "Epigraph"), QStringLiteral(".epigraph"), QStringLiteral("epigraph_font"), 100)<<
               tag(QApplication::translate("SettingsDlg", "Book"), QStringLiteral("body"), QStringLiteral("body_font"), 100);
}

void UpdateLibs()
{
    if(!QSqlDatabase::database(QStringLiteral("libdb"), false).isOpen())
        idCurrentLib = 0;
    else{
        QSettings *settings = GetSettings();
        idCurrentLib = settings->value(QStringLiteral("LibID"), 0).toInt();
        QSqlQuery query(QSqlDatabase::database(QStringLiteral("libdb")));
        query.exec(QStringLiteral("SELECT id,name,path,inpx,version,firstauthor,woDeleted FROM lib ORDER BY name"));
        //                                0  1    2    3    4       5           6
        mLibs.clear();
        while(query.next())
        {
            int idLib = query.value(0).toUInt();
            mLibs[idLib].name = query.value(1).toString().trimmed();
            mLibs[idLib].path = query.value(2).toString().trimmed();
            mLibs[idLib].sInpx = query.value(3).toString().trimmed();
            mLibs[idLib].sVersion = query.value(4).toString().trimmed();
            mLibs[idLib].bFirstAuthor = query.value(5).toBool();
            mLibs[idLib].bWoDeleted = query.value(6).toBool();
        }
        if(mLibs.empty())
            idCurrentLib = 0;
        else{
            if(idCurrentLib == 0)
                idCurrentLib = mLibs.constBegin().key();
            if(!mLibs.contains(idCurrentLib))
                idCurrentLib = 0;
        }
    }
}

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(resource);
    bool bServer = false;
    bTray = false;
    translator = nullptr;
    translator_qt = nullptr;
    QCoreApplication *a;

    for(int i=1; i<argc; i++){
        if(!strcmp(argv[i], "--server") || !strcmp(argv[i], "-s")){
            bServer = true;
        }else
        if(!strcmp(argv[i], "--tray")){
            bTray = true;
        }else
        if(!strcmp(argv[i], "--version") || !strcmp(argv[i], "-v")){
            std::cout << " freelib " << FREELIB_VERSION << "\n";
            return 0;
        }
        if(!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")){
            std::cout << "freelib [парамаетр][команда][опции]\n"
                         "параметры:\n"
                         "\t--tray\t\tminimize to tray on start\n"
                         "-s,\t--server\tstart server\n"
                         "-v,\t--version\tверсия freeLib\n"
                         "\t--lib \n"
                         "команды:\n"
                         "\tlist\t\tсписок библиотек\n"
                         "\tinfo -id\tинформация о библиотеке\n"
                         "\tdelete -id\tудалить библиотеку\n"
                         "опции:\n"
                         "\t-id\tid библиотеки\n";
            return 0;
        }
        if(!strcmp(argv[i], "--lib")){
            a = new QCoreApplication(argc, argv);
            a->setOrganizationName(QStringLiteral("freeLib"));
            a->setApplicationName(QStringLiteral("freeLib"));
            openDB(QStringLiteral("libdb"));
            UpdateLibs();

            QSettings* settings = GetSettings();
            options.Load(settings);
            SetLocale(options.sUiLanguageName);

            uint nId = 0;
            if(i+2 < argc){
                for(int j=i+1; j+1<argc; j++){
                    if(!strcmp(argv[j], "-id")){
                        int id{std::atoi(argv[j+1])};
                        if(id>0)
                            nId = id;
                    }
                }
            }

            if(++i < argc){
                if(!strcmp(argv[i], "list")){
                    std::cout << "id\tlibrary\n"
                                 "----------------------------------------------------------\n";
                    auto iLib = mLibs.constBegin();
                    while(iLib != mLibs.constEnd()){
                        std::cout << iLib.key() << "\t" << iLib->name.toStdString() << "\n";
                        ++iLib;
                    }
                }else
                if(!strcmp(argv[i], "info") && nId>0){
                    if(mLibs.contains(nId)){
                        std::cout << "Library:\t" << mLibs[nId].name.toStdString() << "\n";
                        std::cout << "Inpx file:\t" << mLibs[nId].sInpx.toStdString() << "\n";
                        std::cout << "Books dir:\t" << mLibs[nId].path.toStdString() << "\n";
                        std::cout << "Version:\t" << mLibs[nId].sVersion.toStdString() << "\n";
                    }
                }
                if(!strcmp(argv[i], "delete") && nId>0){
                    if(mLibs.contains(nId)){
                        char a;
                        std::cout << QApplication::translate("LibrariesDlg", "Delete library ").toStdString()
                                  << "\"" << mLibs[nId].name.toStdString() << "\"? (y/N)";
                        std::cin.get(a);
                        if(a == 'y'){
                            QSqlQuery query(QSqlDatabase::database(QStringLiteral("libdb")));
                            query.exec(QStringLiteral("PRAGMA foreign_keys = ON;"));
                            query.exec(QLatin1String("DELETE FROM lib where ID=") + QString::number(nId));
                            query.exec(QStringLiteral("VACUUM"));
                        }
                    }
                }
            }
            delete a;
            return 0;
        }

    }

#ifdef Q_OS_MACX
    if ( QSysInfo::MacintoshVersion > QSysInfo::MV_10_8 )
    {
        // fix Mac OS X 10.9 (mavericks) font issue
        // https://bugreports.qt-project.org/browse/QTBUG-32789
        //QFont::insertSubstitution(".Lucida Grande UI", "Lucida Grande");
    }
#endif
    if(bServer)
        a = new QCoreApplication(argc, argv);
    else{
        a = new QApplication(argc, argv);
        static_cast<QApplication*>(a)->setStyleSheet(QStringLiteral("QComboBox { combobox-popup: 0; }"));
        a->setAttribute(Qt::AA_UseHighDpiPixmaps);
    }

    a->setOrganizationName(QStringLiteral("freeLib"));
    a->setApplicationName(QStringLiteral("freeLib"));

    QString HomeDir;
    if(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).count() > 0)
        HomeDir = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).at(0);

    QSettings* settings = GetSettings();
    options.Load(settings);
    SetLocale(options.sUiLanguageName);
    if(options.vExportOptions.isEmpty())
        options.setExportDefault();

    QDir::setCurrent(HomeDir);
    QString sDirTmp = QStringLiteral("%1/freeLib").arg(QStandardPaths::standardLocations(QStandardPaths::TempLocation).constFirst());
    QDir dirTmp(sDirTmp);
    if(!dirTmp.exists())
        dirTmp.mkpath(sDirTmp);

    if(!bServer && options.bShowSplash){
        QPixmap pixmap(QStringLiteral(":/splash%1.png").arg(static_cast<QApplication*>(a)->devicePixelRatio()>=2? QStringLiteral("@2x") :QLatin1String("")));
        QPainter painter(&pixmap);
        painter.setFont(QFont(painter.font().family(), VERSION_FONT, QFont::Bold));
        painter.setPen(Qt::white);
        painter.drawText(QRect(30, 140, 360, 111), Qt::AlignLeft|Qt::AlignVCenter, QStringLiteral(FREELIB_VERSION));
        splash = new QSplashScreen(pixmap);
#ifdef Q_OS_LINUX
        splash->setWindowIcon(QIcon(QStringLiteral(":/library_128x128.png")));
#endif
        splash->show();
    }

    if(!openDB(QStringLiteral("libdb")))
        return 1;

    a->processEvents();
    setProxy();
    UpdateLibs();

    MainWindow *pMainWindow = nullptr;

    pOpds = new opds_server;
    if(bServer){
        loadGenres();
        loadLibrary(idCurrentLib);
        pOpds->server_run();
    }else{

        pMainWindow = new MainWindow;
#ifdef Q_OS_OSX
        //  w.setWindowFlags(w.windowFlags() & ~Qt::WindowFullscreenButtonHint);
#endif

        if(!pMainWindow->error_quit)
        {
            if(!bTray && options.nIconTray != 2)
                pMainWindow->show();
        }
        else{
            return 1;
        }
        if(options.bShowSplash)
            splash->finish(pMainWindow);
    }

    int result = a->exec();
    if(global_settings)
    {
        global_settings->sync();
        delete global_settings;
    }

    if(pMainWindow)
        delete pMainWindow;
    delete pOpds;
    delete a;
    return result;
}
