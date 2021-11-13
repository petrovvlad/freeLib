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

#include "quazip/quazip/quazip.h"

#include "mainwindow.h"
#include "aboutdialog.h"
#include "common.h"
#include "build_number.h"

uint idCurrentLib;
QTranslator* translator;
QTranslator* translator_qt;
QList<tag> tag_list;
QSettings *global_settings = nullptr;
QCommandLineParser CMDparser;
QSplashScreen *splash;
Options options;

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

QString RelativeToAbsolutePath(QString path)
{
    if(QDir(path).isRelative() && path.indexOf(QLatin1String("%"))<0 && !path.startsWith(QLatin1String("mtp:/")))
    {
        return QApplication::applicationDirPath() + QLatin1String("/") + path;
    }
    return path;
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
    if(translator->load(QStringLiteral(":/language/language_%1.qm").arg(sLocale)))
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

bool openDB(bool create, bool replace)
{
    QString sAppDir,db_file;
    QSettings settings;

    QFileInfo fi(RelativeToAbsolutePath(options.sDatabasePath));
    if(fi.exists() && fi.isFile())
    {
        db_file = fi.canonicalFilePath();
    }
    else
    {
        sAppDir = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).constFirst();
        db_file = sAppDir + QLatin1String("/freeLib.sqlite");
        options.sDatabasePath = db_file;
        settings.setValue(QStringLiteral("database_path"), db_file);
    }
    QFile file(db_file);
    if(!file.exists() || replace)
    {
        if(replace)
        {
            QSqlDatabase dbase = QSqlDatabase::database(QStringLiteral("libdb"), true);
            dbase.close();
            if(!file.remove())
            {
                qDebug()<<("Can't remove old database");
                db_is_open = false;
                return false;
            }
        }
        if(!create && !replace)
        {
            db_is_open = false;
            return true;
        }
        QDir dir;
        dir.mkpath(QFileInfo(db_file).absolutePath());
        file.setFileName(QStringLiteral(":/freeLib.sqlite"));
        file.open(QFile::ReadOnly);
        QByteArray data = file.readAll();
        file.close();
        file.setFileName(db_file);
        file.open(QFile::WriteOnly);
        file.write(data);
        file.close();
    }
    QSqlDatabase dbase = QSqlDatabase::database(QStringLiteral("libdb"), false);
    dbase.setDatabaseName(db_file);
    if (!dbase.open())
    {
        qDebug() << ("Error connect! ")<<db_file;
        db_is_open = false;
        return false;
    }
    db_is_open = true;
    qDebug()<<"Open DB OK. "<<db_file;
    return true;
}

void UpdateLibs()
{
    db_is_open = false;
    //error_quit=false;
    openDB(true,false);
//    if(!openDB(false,false))
//        error_quit=true;
    if(!db_is_open)
        idCurrentLib = 0;
    else{
        QSettings *settings = GetSettings();
        idCurrentLib = settings->value(QStringLiteral("LibID"), 0).toInt();
        QSqlQuery query(QSqlDatabase::database(QStringLiteral("libdb")));
        query.exec(QStringLiteral("SELECT id,name,path,inpx,firstauthor, woDeleted FROM lib ORDER BY name"));
        mLibs.clear();
        while(query.next())
        {
            int idLib = query.value(0).toUInt();
            mLibs[idLib].name = query.value(1).toString().trimmed();
            mLibs[idLib].path = query.value(2).toString().trimmed();
            mLibs[idLib].sInpx = query.value(3).toString().trimmed();
            mLibs[idLib].bFirstAuthor = query.value(4).toBool();
            mLibs[idLib].bWoDeleted = query.value(5).toBool();
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

#ifdef Q_OS_MACX
    if ( QSysInfo::MacintoshVersion > QSysInfo::MV_10_8 )
    {
        // fix Mac OS X 10.9 (mavericks) font issue
        // https://bugreports.qt-project.org/browse/QTBUG-32789
        //QFont::insertSubstitution(".Lucida Grande UI", "Lucida Grande");
    }
#endif

    QApplication a(argc, argv);
    a.setStyleSheet(QStringLiteral("QComboBox { combobox-popup: 0; }"));

    a.setOrganizationName(QStringLiteral("freeLib"));
    a.setApplicationName(QStringLiteral("freeLib"));

    QCommandLineOption trayOption(QStringLiteral("tray"), QStringLiteral("minimize to tray on start"));
    CMDparser.addOption(trayOption);
    CMDparser.process(a);

    QString HomeDir;
    if(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).count() > 0)
        HomeDir = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).at(0);

    translator = nullptr;
    translator_qt = nullptr;
    a.setAttribute(Qt::AA_UseHighDpiPixmaps);
    QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("libdb"));

    QSettings* settings = GetSettings();
    options.Load(settings);

    SetLocale(options.sUiLanguageName);

    QDir::setCurrent(HomeDir);
    QString sDirTmp = QStringLiteral("%1/freeLib").arg(QStandardPaths::standardLocations(QStandardPaths::TempLocation).constFirst());
    QDir dirTmp(sDirTmp);
    if(!dirTmp.exists())
        dirTmp.mkpath(sDirTmp);

    if(options.bShowSplash){
        QPixmap pixmap(QStringLiteral(":/splash%1.png").arg(a.devicePixelRatio()>=2? QStringLiteral("@2x") :QLatin1String("")));
        pixmap.setDevicePixelRatio(a.devicePixelRatio());
        QPainter painter(&pixmap);
        painter.setFont(QFont(painter.font().family(), VERSION_FONT, QFont::Bold));
        painter.setPen(Qt::white);
        painter.drawText(QRect(30, 140, 360, 111), Qt::AlignLeft|Qt::AlignVCenter, PROG_VERSION);
        splash = new QSplashScreen(pixmap);
        splash->resize(640,400);
#ifdef Q_OS_LINUX
        splash->setWindowIcon(QIcon(QStringLiteral(":/library_128x128.png")));
#endif
        splash->show();
    }
    a.processEvents();
    setProxy();
    UpdateLibs();
    MainWindow w;
#ifdef Q_OS_OSX
  //  w.setWindowFlags(w.windowFlags() & ~Qt::WindowFullscreenButtonHint);
#endif

    if(!w.error_quit)
    {
        if(!CMDparser.isSet(QStringLiteral("tray")) && options.nIconTray != 2)
            w.show();
    }
    else{
        return 1;
    }
    if(options.bShowSplash)
        splash->finish(&w);
    int result = a.exec();
    if(global_settings)
    {
        global_settings->sync();
        delete global_settings;
    }

    return result;
}
