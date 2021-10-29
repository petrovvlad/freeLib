#include <QApplication>
#include <QDesktopWidget>
#include <QNetworkProxy>
#include <QStyleFactory>
#include <quazip/quazip/quazip.h>
#include <QLocale>
#include <QTextCodec>
#include <QTranslator>
#include <QLibraryInfo>
#include <QSplashScreen>
#include <QPainter>
#include <QMap>


#include "mainwindow.h"
#include "aboutdialog.h"
#include "fb2mobi/hyphenations.h"
#include "common.h"
#include "build_number.h"

int idCurrentLib;
QTranslator* translator;
QTranslator* translator_qt;
QList<tag> tag_list;
QSettings *global_settings=nullptr;
QCommandLineParser CMDparser;
QSplashScreen *splash;
QApplication *app;
Options options;

bool SetCurrentZipFileName(QuaZip *zip,QString name)
{
    bool result=zip->setCurrentFile(name,QuaZip::csInsensitive);
    if(!result)
    {
        zip->setFileNameCodec(QTextCodec::codecForName("IBM 866"));
        result=zip->setCurrentFile(name,QuaZip::csInsensitive);
    }
    return result;
}

QString RelativeToAbsolutePath(QString path)
{
    if(QDir(path).isRelative() && path.indexOf(QLatin1String("%"))<0 && !path.startsWith(QLatin1String("mtp:/")))
    {
        return app->applicationDirPath()+"/"+path;
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
    if(global_settings==nullptr){
        QString sFile = app->applicationDirPath() + QStringLiteral("/freeLib.cfg");
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

QString fillParams(QString str, SBook& book)
{
    QString result=str;
    QString abbr = QLatin1String("");
    foreach(QString str,mLibs[idCurrentLib].mSerials[book.idSerial].sName.split(QStringLiteral(" ")))
    {
        if(!str.isEmpty())
                abbr += str.at(0);
    }
    result.replace(QLatin1String("%abbrs"), abbr.toLower());
    result.replace(QLatin1String("%app_dir"),QApplication::applicationDirPath()+"/");

    result
    //        .replace("%fn",book_file.completeBaseName()).
    //        replace("%d",book_file.absoluteDir().path()).
            .replace(QLatin1String("%app_dir"),QApplication::applicationDirPath()+"/");
    //result.removeOne("%no_point");
    SAuthor& sFirstAuthor = mLibs[idCurrentLib].mAuthors[book.idFirstAuthor];

    qDebug()<<sFirstAuthor.getName();
    qDebug()<<str;
    result.replace(QLatin1String("%fi"),sFirstAuthor.sFirstName.left(1)+(sFirstAuthor.sFirstName.isEmpty() ?"" :".")).
            replace(QLatin1String("%mi"),sFirstAuthor.sMiddleName.left(1)+(sFirstAuthor.sMiddleName.isEmpty() ?"" :".")).
            replace(QLatin1String("%li"),sFirstAuthor.sLastName.left(1)+(sFirstAuthor.sLastName.isEmpty() ?"" :".")).
            replace(QLatin1String("%nf"),sFirstAuthor.sFirstName.trimmed()).
            replace(QLatin1String("%nm"),sFirstAuthor.sMiddleName.trimmed()).
            replace(QLatin1String("%nl"),sFirstAuthor.sLastName.trimmed());

    //result.replace("%f",book_file.absoluteFilePath());

    result = result.replace(QLatin1String("%s"),mLibs[idCurrentLib].mSerials[book.idSerial].sName)
            .replace(QLatin1String("%b"),book.sName)
            .replace(QLatin1String("%a"),sFirstAuthor.getName())
            .replace(QLatin1String(","),QLatin1String(" ")).trimmed();
    QString num_in_seria=QString::number(book.numInSerial);
//    for(int i=0;i<result.count();i++)
//    {
        if(result.contains(QLatin1String("%n")))
        {
            int len=result.mid(result.indexOf(QLatin1String("%n"))+2,1).toInt();
            QString zerro;
            if(book.numInSerial==0)
                result.replace("%n"+QString::number(len),QLatin1String(""));
            else
                result.replace("%n"+(len>0?QString::number(len):QLatin1String("")),(len>0?zerro.fill('0',len-num_in_seria.length()):QLatin1String(""))+num_in_seria+" ");
        }
//        result[i]=result[i].trimmed();
    //}
    result.replace(QLatin1String("/ "),QLatin1String("/"));
    result.replace(QLatin1String("/."),QLatin1String("/"));
    result.replace(QLatin1String("////"),QLatin1String("/"));
    result.replace(QLatin1String("///"),QLatin1String("/"));
    result.replace(QLatin1String("//"),QLatin1String("/"));
    return result;
}

QString fillParams(QString str, SBook& book, QFileInfo book_file)
{
    QString result=str;
    result
            .replace(QLatin1String("%fn"),book_file.completeBaseName()).
            replace(QLatin1String("%f"),book_file.absoluteFilePath()).
            replace(QLatin1String("%d"),book_file.absoluteDir().path());
    result = fillParams(result,book);
    return result;
}

void SetLocale(QString sLocale)
{
    setlocale(LC_ALL, (sLocale+".UTF-8").toLatin1().data());
    QLocale::setDefault(QLocale(sLocale));

    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

    if(translator)
    {
        app->removeTranslator(translator);
    }
    if(translator_qt)
    {
        app->removeTranslator(translator_qt);
    }

    if(translator==nullptr)
        translator=new QTranslator(app);
    if(translator->load(QStringLiteral(":/language/language_%1.qm").arg(sLocale)))
    {
        app->installTranslator(translator);
    }
    else
    {
        delete translator;
        translator=nullptr;
    }

    if(translator_qt==nullptr)
        translator_qt=new QTranslator(app);
    if(translator_qt->load(QStringLiteral("qtbase_%1.qm").arg(sLocale),QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
    {
        app->installTranslator(translator_qt);
    }
    else
    {
        delete translator_qt;
        translator_qt=nullptr;
    }

    tag_list.clear();
    tag_list<<
               tag(app->translate("SettingsDlg","Top level captions"),QStringLiteral(".h0"),QStringLiteral("top_caption_font"),140)<<
               tag(app->translate("SettingsDlg","Captions"),QStringLiteral(".h1,.h2,.h3,.h4,.h5,.h6"),QStringLiteral("caption_font"),120)<<
               tag(app->translate("SettingsDlg","Dropcaps"),QStringLiteral("span.dropcaps"),QStringLiteral("dropcaps_font"),300)<<
               tag(app->translate("SettingsDlg","Footnotes"),QStringLiteral(".inlinenote,.blocknote"),QStringLiteral("footnotes_font"),80)<<
               tag(app->translate("SettingsDlg","Annotation"),QStringLiteral(".annotation"),QStringLiteral("annotation_font"),100)<<
               tag(app->translate("SettingsDlg","Poems"),QStringLiteral(".poem"),QStringLiteral("poem_font"),100)<<
               tag(app->translate("SettingsDlg","Epigraph"),QStringLiteral(".epigraph"),QStringLiteral("epigraph_font"),100)<<
               tag(app->translate("SettingsDlg","Book"),QStringLiteral("body"),QStringLiteral("body_font"),100);
}

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QString tmp_dir;
    if(QStandardPaths::standardLocations(QStandardPaths::TempLocation).count()>0)
        tmp_dir=QStandardPaths::standardLocations(QStandardPaths::TempLocation).at(0);
    tmp_dir+=QLatin1String("/freeLib/log.txt");
    QFile file(tmp_dir);
    file.open(QIODevice::Append|QIODevice::WriteOnly);
    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        file.write(localMsg.constData());
        file.write("\r\n");
        fprintf(stderr, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        abort();
    default:
        break;
    }
}

bool openDB(bool create, bool replace)
{
    QString sAppDir,db_file;
    QSettings settings;

    QFileInfo fi(RelativeToAbsolutePath(options.sDatabasePath));
    if(fi.exists() && fi.isFile())
    {
        db_file=fi.canonicalFilePath();
    }
    else
    {
        sAppDir=QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).constFirst();
        db_file=sAppDir+"/freeLib.sqlite";
        options.sDatabasePath = db_file;
        settings.setValue(QStringLiteral("database_path"),db_file);
    }
    QFile file(db_file);
    if(!file.exists() || replace)
    {
        if(replace)
        {
            QSqlDatabase dbase=QSqlDatabase::database(QStringLiteral("libdb"),true);
            dbase.close();
            if(!file.remove())
            {
                qDebug()<<("Can't remove old database");
                db_is_open=false;
                return false;
            }
        }
        if(!create && !replace)
        {
            db_is_open=false;
            return true;
        }
        QDir dir;
        dir.mkpath(QFileInfo(db_file).absolutePath());
        file.setFileName(QStringLiteral(":/freeLib.sqlite"));
        file.open(QFile::ReadOnly);
        QByteArray data=file.readAll();
        file.close();
        file.setFileName(db_file);
        file.open(QFile::WriteOnly);
        file.write(data);
        file.close();
    }
    QSqlDatabase dbase=QSqlDatabase::database(QStringLiteral("libdb"),false);
    dbase.setDatabaseName(db_file);
    if (!dbase.open())
    {
        qDebug() << ("Error connect! ")<<db_file;
        db_is_open=false;
        return false;
    }
    db_is_open=true;
    qDebug()<<"Open DB OK. "<<db_file;
    return true;
}

void UpdateLibs()
{
    db_is_open=false;
    //error_quit=false;
    openDB(true,false);
//    if(!openDB(false,false))
//        error_quit=true;
    if(!db_is_open)
        idCurrentLib=-1;
    else{
        QSettings settings/*=GetSettings()*/;
        idCurrentLib=settings.value(QStringLiteral("LibID"),-1).toInt();
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
            idCurrentLib = -1;
        else{
            if(idCurrentLib ==-1)
                idCurrentLib = mLibs.constBegin().key();
            if(!mLibs.contains(idCurrentLib))
                idCurrentLib = -1;
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

    QPixmap pixmap(QStringLiteral(":/splash%1.png").arg(app->devicePixelRatio()>=2? QStringLiteral("@2x") :QLatin1String("")));
    pixmap.setDevicePixelRatio(app->devicePixelRatio());
    QPainter painter(&pixmap);
    painter.setFont(QFont(painter.font().family(),VERSION_FONT,QFont::Bold));
    painter.setPen(Qt::white);
    painter.drawText(QRect(30,140,360,111),Qt::AlignLeft|Qt::AlignVCenter,PROG_VERSION);

    QString HomeDir=QLatin1String("");
    if(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).count()>0)
        HomeDir=QStandardPaths::standardLocations(QStandardPaths::HomeLocation).at(0);

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor("#E0E0E0")); //основной цвет интер
    darkPalette.setColor(QPalette::WindowText, Qt::black);
    darkPalette.setColor(QPalette::Base, QColor("#FfFfFf"));
    darkPalette.setColor(QPalette::AlternateBase, QColor("#F0F0F0"));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::black);
    darkPalette.setColor(QPalette::ToolTipText, Qt::black);
    darkPalette.setColor(QPalette::Text, Qt::black);
    darkPalette.setColor(QPalette::Button, QColor("#e4e4e4"));
    darkPalette.setColor(QPalette::ButtonText, Qt::black);
    //darkPalette.setColor(QPalette::BrightText, Qt::red);

    darkPalette.setColor(QPalette::Light, QColor("#c0c0c0"));
    darkPalette.setColor(QPalette::Midlight, QColor("#b0b0b0"));
    darkPalette.setColor(QPalette::Dark, QColor("#a0a0a0a"));
    darkPalette.setColor(QPalette::Mid, QColor("#909090"));
    darkPalette.setColor(QPalette::Shadow, QColor("#707070"));

    darkPalette.setColor(QPalette::Highlight, QColor("#0B61A4"));
    darkPalette.setColor(QPalette::HighlightedText, Qt::white);

    //darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    //darkPalette.setColor(QPalette::LinkVisited, QColor(42, 130, 218));

    //a.setPalette(darkPalette);

    translator=nullptr;
    translator_qt=nullptr;
    app=&a;
    app->setAttribute(Qt::AA_UseHighDpiPixmaps);
    QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),QStringLiteral("libdb"));

    QSettings* settings=GetSettings();
    options.Load(settings);

    SetLocale(options.sUiLanguageName);

    QDir::setCurrent(HomeDir);
    QString sDirTmp = QStringLiteral("%1/freeLib").arg(QStandardPaths::standardLocations(QStandardPaths::TempLocation).constFirst());
    QDir dirTmp(sDirTmp);
    if(!dirTmp.exists())
        dirTmp.mkpath(sDirTmp);

    if(options.bShowSplash){
        splash=new QSplashScreen(pixmap);
        splash->resize(640,400);
#ifdef Q_OS_LINUX
        splash->setWindowIcon(QIcon(QStringLiteral(":/library_128x128.png")));
#endif
        splash->show();
    }
    a.processEvents();
    setProxy();
    //idCurrentLib=settings->value("LibID",-1).toInt();
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
    //current_lib.UpdateLib();
    int result=a.exec();
    if(global_settings)
    {
        global_settings->sync();
        delete global_settings;
    }

    return result;
}
