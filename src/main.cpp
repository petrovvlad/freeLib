#define QT_USE_QSTRINGBUILDER
#include <iostream>
#include <QApplication>
#include <QNetworkProxy>
#include <QSplashScreen>
#include <QPainter>
#include <QStringBuilder>
#include <QDir>
#include <QSqlError>
#include <QThread>
#include <QString>

#include "mainwindow.h"
#include "aboutdialog.h"
#include "utilites.h"
#include "config-freelib.h"
#ifdef USE_HTTSERVER
#include "opds_server.h"
#endif
#include "importthread.h"

void UpdateLibs()
{
    if(!QSqlDatabase::database(u"libdb"_s, false).isOpen())
        g::idCurrentLib = 0;
    else{
        auto settings = GetSettings();
        g::idCurrentLib = settings->value(u"LibID"_s, 0).toInt();
        QSqlQuery query(QSqlDatabase::database(u"libdb"_s));
        query.exec(u"SELECT id,name,path,inpx,version,firstauthor,woDeleted FROM lib ORDER BY name"_s);
        //                  0  1    2    3    4       5           6
        g::libs.clear();
        while(query.next())
        {
            int idLib = query.value(0).toUInt();
            g::libs[idLib].name = query.value(1).toString().trimmed();
            g::libs[idLib].path = query.value(2).toString().trimmed();
            g::libs[idLib].sInpx = query.value(3).toString().trimmed();
            g::libs[idLib].sVersion = query.value(4).toString().trimmed();
            g::libs[idLib].bFirstAuthor = query.value(5).toBool();
            g::libs[idLib].bWoDeleted = query.value(6).toBool();
        }
        if(g::libs.empty())
            g::idCurrentLib = 0;
        else{
            if(g::idCurrentLib == 0 || !g::libs.contains(g::idCurrentLib))
                g::idCurrentLib = g::libs.cbegin()->first;
        }
    }
}

QString parseOption(int argc, char* argv[], const char* option)
{
    QString sRet;
    if(argc >= 2){
        for(int j=0; j+1<argc; j++){
            if(!strcmp(argv[j], option)){
                sRet = QString::fromUtf8(argv[j+1]);
                break;
            }
        }
    }
    return sRet;
}

void cmdhelp(){

std::cout  << "freelib " << FREELIB_VERSION << "\n\nfreelib [Option [Parameters]\n"
             "Options:\n"
             "-t,\t--tray\t\tMinimize to tray on start\n"
#ifdef USE_HTTSERVER
             "-s,\t--server\tStart server\n"
                    "\t\t-lang [lang]\tLanguage filter\n"
            "\t--set\t Set server parameters\n"
                "\t\t-u [user]\n"
                "\t\t-p [password]\n"
                "\t\t-port [port]\n"
#endif
             "-v,\t--version\tShow version and exit\n"
             "\t--verbose\tVerbose mode\n"
             "\t--lib-ls\tShow libraries\n"
             "\t--lib-db [path]\tSet database path\n"
             "\t--lib-in [id]\tLibrary information\n"
             "\t--lib-sp\tSet paths for a library\n"
                    "\t\t-id [id]\n"
                    "\t\t-inpx [inpx path]\n"
                    "\t\t-path [library directory]\n"
             "\t--lib-ad\tAdd library\n"
                    "\t\t-name [name]\n"
                    "\t\t-inpx [inpx path]\n"
                    "\t\t-path [library directory]\n"
             "\t--lib-dl [id]\tDelete library\n"
             "\t--lib-up [id]\tUpdate library\n"
			 ;
}

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(resource);
#ifdef USE_HTTSERVER
    bool bServer = false;
    QString sLanguageFilter;
#endif
    g::bTray = false;
    g::bVerbose = false;
    std::unique_ptr<QCoreApplication> a;
    QString cmdparam;

    for(int i=1; i<argc; i++){

        cmdparam = argv[i];

        if (cmdparam == u"--help" || cmdparam == u"-h"){
            cmdhelp();
            return 0;
        }

        if (cmdparam == u"--verbose")
            g::bVerbose = true;

#ifdef USE_HTTSERVER
        if (cmdparam == u"--server" || cmdparam == u"-s"){
            bServer = true;
            sLanguageFilter = parseOption(argc-(i), &argv[i], "-lang");
        }else
#endif
        if (cmdparam == u"--tray" || cmdparam == u"-t"){
            g::bTray = true;
        }else

        if (cmdparam == u"--version" || cmdparam == u"-v"){
            std::cout << "freelib " << FREELIB_VERSION << "\n";
            return 0;
        }

#ifdef USE_HTTSERVER
        if (cmdparam == u"--set"){
            g::bUseGui = false;
            auto settings = GetSettings();
            QString sUser = parseOption(argc-(i), &argv[i], "-u");
            if(!sUser.isEmpty()){
                settings->setValue(u"HTTP_user"_s, sUser);
            }
            QString sPassword = parseOption(argc-(i), &argv[i], "-p");
            if(!sPassword.isEmpty()){
                QByteArray baSalt = Options::generateSalt();
                QByteArray baHash =  Options::passwordToHash(sPassword, baSalt);
                settings->setValue(u"httpPassword"_s, QString(baSalt.toBase64()) + u":"_s + QString(baHash.toBase64()));
                settings->setValue(u"HTTP_need_pasword"_s, true);
            }
            QString sPort = parseOption(argc-(i), &argv[i], "-port");
            if(!sPort.isEmpty()){
                ushort nPort = sPort.toUShort();
                if(nPort>0){
                    settings->setValue(u"portHttp"_s, nPort);
                }
            }
            return 0;
        }
#endif

        // Edit libraries
        if (cmdparam.contains(u"--lib"_s)){
            a = std::unique_ptr<QCoreApplication>(new QCoreApplication(argc, argv));
            a->setApplicationName(u"freeLib"_s);

            g::bUseGui = false;
            auto settings = GetSettings();
            g::options.Load(settings);

            openDB(QStringLiteral("libdb"));
            UpdateLibs();

            setLocale(g::options.sUiLanguageName);

            uint nId = 0;

            // List libraries
            if(cmdparam == u"--lib-ls"){
                std::cout << "id\tlibrary\n"
                             "----------------------------------------------------------\n";
                for(const auto &iLib :g::libs){
                    std::cout << iLib.first << "\t" << iLib.second.name.toStdString() << "\n";
                }
            }

            // Set database path
            if(cmdparam == u"--lib-db"){
                QString sDbpath = parseOption(argc-(i), &argv[i], "--lib-db");
                sDbpath = QFileInfo{sDbpath}.absoluteFilePath();
                if(!sDbpath.isEmpty()){

                    if (QFile::exists(sDbpath)) {
                        g::options.sDatabasePath = sDbpath;
                        settings->setValue(QStringLiteral("database_path"), g::options.sDatabasePath);
                        std::cout <<  g::options.sDatabasePath.toStdString() + " - Ok! \n";
                    }
                    else{
                        std::cout << "The path " + sDbpath.toStdString() + " does not exist! \n";
                    }
                }
                else{
                    cmdhelp();
                }
            }

            // Get library information
            if(cmdparam == u"--lib-in"){
                nId = (parseOption(argc-(i), &argv[i], "--lib-in")).toUInt();
                if(g::libs.contains(nId)){

                    std::cout
                        << "Library:\t" << g::libs[nId].name.toStdString() << "\n"
                        << "Inpx file:\t" << g::libs[nId].sInpx.toStdString() << "\n"
                        << "Books dir:\t" << g::libs[nId].path.toStdString() << "\n"
                        << "Version:\t" << g::libs[nId].sVersion.toStdString() << "\n";
#ifdef USE_HTTSERVER
                    std::string sBaseUrl = g::options.sBaseUrl.isEmpty() ?"http://localhost:" + std::to_string(g::options.nHttpPort) :g::options.sBaseUrl.toStdString();
                    std::string sIdLib = std::to_string(nId);
                    std::cout
                        << QApplication::translate("LibrariesDlg", "OPDS server").toStdString() << " "
                        << "\t" << sBaseUrl <<"/opds/" << sIdLib << "\n"
                        << QApplication::translate("LibrariesDlg", "OPDS 2.0 server").toStdString()
                        << "\t" << sBaseUrl <<"/opds2/" << sIdLib << "\n"
                        << QApplication::translate("LibrariesDlg", "Web server").toStdString()
                        << "\t\t" << sBaseUrl << "/" << sIdLib << "\n";
#endif
                }
                else{
                    std::cout << QApplication::translate("main", "Library not found!").toStdString() << "\n\n";
                }
            }


            // Set library path
            if(cmdparam == u"--lib-sp"){
                QSqlQuery query(QSqlDatabase::database(u"libdb"_s));
                QString sId = parseOption(argc-(i), &argv[i], "-id");
                nId = sId.toUInt();
                if(g::libs.contains(nId)){

                    QString inpxPath = parseOption(argc-(i), &argv[i], "-inpx");
                    inpxPath = QFileInfo{inpxPath}.absoluteFilePath();
                    QString libPath = parseOption(argc-(i), &argv[i], "-path");
                    libPath = QFileInfo{libPath}.absoluteFilePath();

                    if(!libPath.isEmpty() && !sId.isEmpty()) {

                        std::cout <<  "Updating paths for library: " << sId.toStdString() + "\n";
                        if ( QFile::exists(libPath)){
                            bool result = query.exec(u"UPDATE lib SET path = '%1' WHERE id='%2'"_s.arg(libPath, sId));
                            if(!result)
                                std::cout << query.lastError().databaseText().toStdString() << "\n";
                            else{
                                std::cout <<libPath.toStdString() + " - Ok! \n";
                            }
                        }
                        else {
                            std::cout << "The lib path " + libPath.toStdString() + " does not exist\n";
                        }
                    }else
                    if(!inpxPath.isEmpty() && !sId.isEmpty()) {
                        if ( QFile::exists(inpxPath)){
                            bool result = query.exec(u"UPDATE lib SET inpx = '%1' WHERE id='%2'"_s.arg(inpxPath, sId));
                            if(!result)
                                std::cout << query.lastError().databaseText().toStdString() << "\n";
                            else{
                                std::cout << inpxPath.toStdString() + " - Ok! \n";
                            }
                        }
                        else
                            std::cout << "The inpx path " + inpxPath.toStdString() + " does not exist\n";
                    }
                    else{
                        cmdhelp();
                    }
                }else
                    std::cout << QApplication::translate("main", "Library not found!").toStdString() << "\n\n";
            }

            // Add library
            if(cmdparam == u"--lib-ad"){
                QString sPath = parseOption(argc-(i), &argv[i], "-path");
                sPath = QFileInfo{sPath}.absoluteFilePath();
                QString sInpx = parseOption(argc-(i), &argv[i], "-inpx");
                sInpx = QFileInfo{sInpx}.absoluteFilePath();
                QString sName = parseOption(argc-(i), &argv[i], "-name");

                if ( QFile::exists(sPath)){
                    if(sName.isEmpty()){
                        sName = sInpx.isEmpty() ? QApplication::translate("LibrariesDlg", "new") : SLib::nameFromInpx(sInpx);
                        if(sName.isEmpty())
                            std::cout <<  QApplication::translate("main", "Enter librarry name").toStdString() << ":";
                        else
                            std::cout << QApplication::translate("main", "Enter librarry name").toStdString() << " (" << sName.toStdString() << "):";
                        char newName[1024];
                        std::cin.get(newName, sizeof(newName));
                        QString sNewName = QString::fromUtf8(newName);
                        if(!sNewName.isEmpty())
                            sName = sNewName;
                    }

                    QSqlQuery query(QSqlDatabase::database(QStringLiteral("libdb")));
                    bool result = query.exec(QStringLiteral("INSERT INTO lib(name,path,inpx) values('%1','%2','%3')")
                                                 .arg(sName, sPath, sInpx));
                    if(!result){
                        std::cout << query.lastError().databaseText().toStdString() << "\n";
                    }
                    else{
                        std::cout << "Name: \t" << sName.toStdString() << "\tPath: \t" << sPath.toStdString() << "\t - Ok!\n";
                    }
                }
                else{
                    std::cout << "\nThe lib path " + sPath.toStdString() + " does not exist\n\n";
                    cmdhelp();
                }
            }


            // Delete library
            if(cmdparam == u"--lib-dl"){

                nId = (parseOption(argc-(i), &argv[i], "--lib-dl")).toUInt();
                if(g::libs.contains(nId)){
                    char ans;
                    std::cout << QApplication::translate("LibrariesDlg", "Delete library ").toStdString()
                              << "\"" << g::libs[nId].name.toStdString() << "\"? (y/N)";
                    std::cin.get(ans);
                    if(ans == 'y'){
                        QSqlQuery query(QSqlDatabase::database(QStringLiteral("libdb")));
                        query.exec(QStringLiteral("PRAGMA foreign_keys = ON;"));
                        query.exec(QStringLiteral("DELETE FROM lib where ID=") + QString::number(nId));
                        query.exec(QStringLiteral("VACUUM"));
                    }
                }
                else{
                    std::cout << QApplication::translate("main", "Library not found!").toStdString() << "\n\n";
                }
            }

            // Update libraries
            if(cmdparam == u"--lib-up"){
                nId = (parseOption(argc-(i), &argv[i], "--lib-up")).toUInt();

                if(g::libs.contains(nId)){
                    auto thread = new QThread;
                    auto imp_tr = new ImportThread();
                    SLib &lib = g::libs[nId];

                    imp_tr->init(nId, lib, UT_NEW);
                    imp_tr->moveToThread(thread);
                    QObject::connect(thread, &QThread::started, imp_tr, &ImportThread::process);
                    QObject::connect(imp_tr, &ImportThread::End, thread, &QThread::quit);
                    QObject::connect(imp_tr, &ImportThread::End, []()
                                     {
                                         std::cout << QApplication::translate("LibrariesDlg", "Ending").toStdString() << "\n";
                                     });
                    thread->start();
                    while(!thread->wait(500)){
                        QCoreApplication::processEvents();
                    }
                    thread->deleteLater();
                    imp_tr->deleteLater();

                }
                else{
                    std::cout << QApplication::translate("main", "Library not found!").toStdString() << "\n\n";
                }
            }
            return 0;
        }
    }
    Q_INIT_RESOURCE(resource);
    qputenv("QT_LOGGING_RULES", "qt.text.font.db=false");

#ifdef Q_OS_MACX
    if ( QSysInfo::MacintoshVersion > QSysInfo::MV_10_8 )
    {
        // fix Mac OS X 10.9 (mavericks) font issue
        // https://bugreports.qt-project.org/browse/QTBUG-32789
        //QFont::insertSubstitution(".Lucida Grande UI", "Lucida Grande");
    }
#endif
#ifdef USE_HTTSERVER
    if(bServer){
#ifndef Q_OS_WINDOWS
        setenv("QT_QPA_PLATFORM", "offscreen", 1);
#endif //Q_OS_WINDOWS
        a = std::unique_ptr<QCoreApplication>(new QGuiApplication(argc, argv));
        g::bUseGui = false;
    }else
#endif //USE_HTTSERVER
    {
        a = std::unique_ptr<QCoreApplication>(new QApplication(argc, argv));
        static_cast<QApplication*>(a.get())->setStyleSheet(u"QComboBox { combobox-popup: 0; }"_s);
        g::bUseGui = true;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        a->setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
    }

    a->setApplicationName(u"freeLib"_s);
    QGuiApplication::setDesktopFileName(u"freelib"_s);

    QString HomeDir;
    if(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).count() > 0)
        HomeDir = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).at(0);

    auto  settings = GetSettings();
    bool bDefault = !QFile::exists(settings->fileName());
    if(bDefault){
        g::options.setDefault();
    }else{
        g::options.Load(settings);
        g::options.readPasswords();
        setLocale(g::options.sUiLanguageName);
    }
    if(g::options.vExportOptions.empty())
        g::options.setExportDefault();

    std::unique_ptr<QSplashScreen> splash;
    if(g::bUseGui && g::options.bShowSplash)
    {
        QPixmap pixmap(u":/splash%1.png"_s.arg(static_cast<QApplication*>(a.get())->devicePixelRatio()>=2? u"@2x"_s :u""_s));
        QPainter painter(&pixmap);
        painter.setFont(QFont(painter.font().family(), VERSION_FONT, QFont::Bold));
        painter.setPen(Qt::white);
        painter.drawText(QRect(30, 140, 360, 111), Qt::AlignLeft|Qt::AlignVCenter, QStringLiteral(FREELIB_VERSION));
        splash = std::unique_ptr<QSplashScreen>(new QSplashScreen(pixmap));
#ifdef Q_OS_LINUX
        splash->setWindowIcon(QIcon(u":/library_128x128.png"_s));
#endif
        splash->show();
    }

    if(!openDB(u"libdb"_s))
        return 1;

    QDir::setCurrent(HomeDir);
    QString sDirTmp = QDir::tempPath() + u"/freeLib"_s;
    QDir dirTmp(sDirTmp);
    if(!dirTmp.exists())
        dirTmp.mkpath(sDirTmp);
    QString sDirCovers = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + u"/covers"_s;
    QDir dirCovers(sDirCovers);
    if(!dirCovers.exists())
        dirCovers.mkpath(sDirCovers);

    a->processEvents();
    UpdateLibs();

    MainWindow *pMainWindow = nullptr;

#ifdef USE_HTTSERVER
    setProxy();
    std::unique_ptr<opds_server> pOpds;
    if(bServer){
        pOpds = std::unique_ptr<opds_server>( new opds_server(a.get()) );
        loadGenres();
        loadLibrary(g::idCurrentLib);
        g::options.bOpdsEnable = true;
        if(!sLanguageFilter.isEmpty())
            pOpds->setLanguageFilter(sLanguageFilter);
        pOpds->server_run();
    }else
#endif
    {
        pMainWindow = new MainWindow;
#ifdef Q_OS_OSX
        //  w.setWindowFlags(w.windowFlags() & ~Qt::WindowFullscreenButtonHint);
#endif

        if(!pMainWindow->error_quit)
        {
            if(!g::bTray && g::options.nIconTray != 2)
                pMainWindow->show();
        }
        else{
            delete pMainWindow;
            return 1;
        }
        if(g::options.bShowSplash && splash && pMainWindow)
            splash->finish(pMainWindow);
    }

    int result = a->exec();
    if(pMainWindow)
        delete pMainWindow;

    QDir(QDir::tempPath() + u"/freeLib/"_s).removeRecursively();
    return result;
}
