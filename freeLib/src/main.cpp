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


//SLib current_lib;
int idCurrentLib;
QTranslator* translator;
QTranslator* translator_qt;
QList<tag> tag_list;
QSettings *global_settings=0;
QMap<int,SLib> mLibs;
QMap <uint,SGenre> mGenre;
QCommandLineParser CMDparser;
QSplashScreen *splash;
QApplication *app;

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
    if(QDir(path).isRelative() && path.indexOf("%")<0)
    {
        return app->applicationDirPath()+"/"+path;
    }
    return path;
}

QSettings* GetSettings(bool need_copy,bool reopen)
{
    if(reopen && global_settings)
    {
        global_settings->sync();
        delete global_settings;
        global_settings=0;
    }
    if(global_settings && !need_copy)
    {
        global_settings->sync();
        return global_settings;
    }
    QFileInfo fi(app->applicationDirPath()+"/freeLib.cfg");
    QSettings* current_settings;
    if(fi.exists())
    {
        current_settings=new QSettings(fi.absoluteFilePath(),QSettings::IniFormat);
    }
    else
    {
        fi.setFile(app->applicationDirPath()+"/../../../freeLib/freeLib.cfg");
        if(fi.exists())
            current_settings=new QSettings(fi.absoluteFilePath(),QSettings::IniFormat);
        else
            current_settings=new QSettings(OrgName,AppName);
    }
    if(need_copy)
        return current_settings;
    global_settings=current_settings;
    return global_settings;
}

QNetworkProxy proxy;
void setProxy()
{
    QSettings* settings=GetSettings();
    proxy.setPort(settings->value("proxy_port",default_proxy_port).toInt());
    proxy.setHostName(settings->value("proxy_host").toString());
    proxy.setPassword(settings->value("proxy_password").toString());
    proxy.setUser(settings->value("proxy_user").toString());
    switch(settings->value("proxy_type",0).toInt())
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

void ResetToDefaultSettings()
{
    int size=100;
    int id_tag=0;

    foreach(tag i,tag_list)
    {
        if(i.font_name=="dropcaps_font")
        {
            size=i.font_size;
            break;
        }
        id_tag++;
    }

    QSettings* settings=GetSettings();
    settings->clear();
    settings->sync();
    settings->beginWriteArray("export");

    settings->setArrayIndex(0);
    settings->setValue("ExportName",QApplication::tr("Save as")+" ...");
    settings->setValue("OutputFormat","-");
    settings->beginWriteArray("fonts");
    settings->setArrayIndex(0);
    settings->setValue("use",true);
    settings->setValue("tag",id_tag);
    settings->setValue("font",default_dropcaps_font);
    settings->setValue("fontSize",size);
    settings->endArray();

    settings->setArrayIndex(1);
    settings->setValue("ExportName",QApplication::tr("Save as")+" MOBI");
    settings->setValue("OutputFormat","MOBI");
    settings->beginWriteArray("fonts");
    settings->setArrayIndex(0);
    settings->setValue("use",true);
    settings->setValue("tag",id_tag);
    settings->setValue("font",default_dropcaps_font);
    settings->setValue("fontSize",size);
    settings->endArray();

    settings->setArrayIndex(2);
    settings->setValue("ExportName",QApplication::tr("Save as")+" EPUB");
    settings->setValue("OutputFormat","EPUB");
    settings->beginWriteArray("fonts");
    settings->setArrayIndex(0);
    settings->setValue("use",true);
    settings->setValue("tag",id_tag);
    settings->setValue("font",default_dropcaps_font);
    settings->setValue("fontSize",size);
    settings->endArray();

    settings->setArrayIndex(3);
    settings->setValue("ExportName",QApplication::tr("Save as")+" AZW3");
    settings->setValue("OutputFormat","AZW3");
    settings->beginWriteArray("fonts");
    settings->setArrayIndex(0);
    settings->setValue("use",true);
    settings->setValue("tag",id_tag);
    settings->setValue("font",default_dropcaps_font);
    settings->setValue("fontSize",size);
    settings->endArray();

    settings->endArray();
}

QStringList fillParams(QStringList str, book_info &bi,QFileInfo book_file)
{
    QStringList result=str;
    QString abbr = "";
    foreach(QString str,bi.seria.split(" "))
    {
        abbr+=str.left(1);
    }
    result.replaceInStrings("%abbrs", abbr.toLower());

    result.replaceInStrings("%fn",book_file.completeBaseName()).
            replaceInStrings("%d",book_file.absoluteDir().path()).
            replaceInStrings("%app_dir",QApplication::applicationDirPath()+"/");
    result.removeOne("%no_point");

    qDebug()<<bi.authors[0].firstname;
    qDebug()<<bi.authors[0].author;
    qDebug()<<str;
    result.replaceInStrings("%fi",bi.authors[0].firstname.left(1)+".").
            replaceInStrings("%mi",bi.authors[0].middlename.left(1)+".").
            replaceInStrings("%li",bi.authors[0].lastname.left(1)+".").
            replaceInStrings("%nf",bi.authors[0].firstname.trimmed()).
            replaceInStrings("%nm",bi.authors[0].middlename.trimmed()).
            replaceInStrings("%nl",bi.authors[0].lastname.trimmed());

    result.replaceInStrings("%f",book_file.absoluteFilePath());

    result.replaceInStrings("%s",bi.seria.trimmed()).replaceInStrings("%b",bi.title.trimmed()).replaceInStrings("%a",bi.authors[0].author.replace(","," ").trimmed());
    QString num_in_seria=QString::number(bi.num_in_seria);
    for(int i=0;i<result.count();i++)
    {
        if(result[i].contains("%n"))
        {
            int len=result[i].mid(result[i].indexOf("%n")+2,1).toInt();
            QString zerro;
            if(bi.num_in_seria==0)
                result[i].replace("%n"+QString::number(len),"");
            else
                result[i].replace("%n"+(len>0?QString::number(len):""),(len>0?zerro.fill('0',len-num_in_seria.length()):"")+num_in_seria+" ");
        }
        result[i]=result[i].trimmed();
    }
    result.replaceInStrings("/ ","/");
    result.replaceInStrings("/.","/");
    result.replaceInStrings("////","/");
    result.replaceInStrings("///","/");
    result.replaceInStrings("//","/");
    return result;
}

QString fillParams(QString str, book_info &bi,QFileInfo book_file)
{
    QStringList result=fillParams(QStringList()<<str, bi,book_file);
    return result[0];
}

QStringList fillParams(QStringList str, QFileInfo book_file,QString seria_name,QString book_name,QString author,QString ser_num)
{
    book_info bi;
    bi.seria=seria_name;
    bi.title=book_name;
    bi.authors<<author_info(author,-1);
    bi.num_in_seria=ser_num.toInt();
    return fillParams(str,bi,book_file);
}

QString fillParams(QString str, QFileInfo book_file,QString seria_name,QString book_name,QString author,QString ser_num)
{
    QStringList result=fillParams(QStringList()<<str, book_file, seria_name, book_name, author, ser_num);
    return result[0];
}

SendType SetCurrentExportSettings(int index)
{
    QSettings *settings=GetSettings(true);
    int count=settings->beginReadArray("export");
    QMap<QString,QVariant> map;
    QStringList keys;
    if(index>=0 && index<count)
    {
        settings->setArrayIndex(index);
        keys=settings->allKeys();
        foreach (QString key, keys)
        {
            map.insert(key,settings->value(key));
        }
    }
    settings->endArray();
    foreach (QString key, keys)
    {
        settings->setValue(key,map.value(key,settings->value(key)));
    }
    settings->sync();
    delete settings;
    if(map.contains("sendTo"))
    {
        return (map.value("sendTo").toString()=="device"?ST_Device:ST_Mail);
    }
    return ST_Device;
}

QString FindLocaleFile(QString locale,QString name,QString suffics)
{
    QString FileName=QApplication::applicationDirPath()+"/language/"+name+"_";
    int name_len=FileName.length();
    FileName+=locale+".qm";
    bool qm=true;
    while(!QFile(FileName).exists())
    {
        if(qm)
        {
            FileName=FileName.left(FileName.length()-(suffics.length()+1));
            qm=false;
        }
        else
        {
            int pos=FileName.indexOf("_",name_len);
            if(pos<0)
                return "";
            FileName=FileName.left(pos)+"."+suffics;
            qm=true;
        }
    }
    return FileName;
}

void SetLocale()
{
    QSettings* settings=GetSettings();
    QString locale=settings->value("localeUI",QLocale::system().name()).toString();
    setlocale(LC_ALL, "rus");
    QLocale::setDefault(QLocale(QLocale::Russian,QLocale::RussianFederation));

    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

    if(translator)
    {
        app->removeTranslator(translator);
    }
    if(translator_qt)
    {
        app->removeTranslator(translator_qt);
    }

    translator=new QTranslator(app);
    if(translator->load(FindLocaleFile(locale,"language","qm")))
    {
        app->installTranslator(translator);
    }
    else
    {
        delete translator;
        translator=0;
    }

    translator_qt=new QTranslator(app);
    if(translator_qt->load(FindLocaleFile(locale,"qtbase","qm")))
    {
        app->installTranslator(translator_qt);
    }
    else
    {
        delete translator_qt;
        translator_qt=0;
    }
    settings->setValue("localeUI",locale);
    settings->sync();

    tag_list.clear();
    tag_list<<
               tag(app->translate("SettingsDlg","Top level captions"),".h0","top_caption_font",140)<<
               tag(app->translate("SettingsDlg","Captions"),".h1,.h2,.h3,.h4,.h5,.h6","caption_font",120)<<
               tag(app->translate("SettingsDlg","Dropcaps"),"span.dropcaps","dropcaps_font",300)<<
               tag(app->translate("SettingsDlg","Footnotes"),".inlinenote,.blocknote","footnotes_font",80)<<
               tag(app->translate("SettingsDlg","Annotation"),".annotation","annotation_font",100)<<
               tag(app->translate("SettingsDlg","Poems"),".poem","poem_font",100)<<
               tag(app->translate("SettingsDlg","Epigraph"),".epigraph","epigraph_font",100)<<
               tag(app->translate("SettingsDlg","Book"),"body","body_font",100);
}

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QString tmp_dir;
    if(QStandardPaths::standardLocations(QStandardPaths::TempLocation).count()>0)
        tmp_dir=QStandardPaths::standardLocations(QStandardPaths::TempLocation).at(0);
    tmp_dir+="/freeLib/log.txt";
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
    QString HomeDir="";
    if(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).count()>0)
        HomeDir=QStandardPaths::standardLocations(QStandardPaths::HomeLocation).at(0);
    QString db_file=HomeDir+"/freeLib/freeLib.sqlite";
    if(QFileInfo(HomeDir+"/freeLib/my_db.sqlite").exists())
        QFile::rename(HomeDir+"/freeLib/my_db.sqlite",db_file);
    QSettings *settings=GetSettings();

    QFileInfo fi(RelativeToAbsolutePath(settings->value("database_path").toString()));
    if(fi.exists() && fi.isFile())
    {
        db_file=fi.canonicalFilePath();
    }
    else
    {
        fi.setFile(app->applicationDirPath()+"/freeLib.sqlite");
        if(fi.exists() && fi.isFile())
        {
            db_file=fi.canonicalFilePath();
        }
        else
        {
            fi.setFile(app->applicationDirPath()+"/../../../freeLib/freeLib.sqlite");
            if(fi.exists() && fi.isFile())
                db_file=fi.canonicalFilePath();
        }
    }
    settings->setValue("database_path",db_file);
    settings->sync();
    QFile file(db_file);
    if(!file.exists() || replace)
    {
        if(replace)
        {
            QSqlDatabase dbase=QSqlDatabase::database("libdb",true);
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
        file.setFileName(":/freeLib.sqlite");
        file.open(QFile::ReadOnly);
        QByteArray data=file.readAll();
        file.close();
        file.setFileName(db_file);
        file.open(QFile::WriteOnly);
        file.write(data);
        file.close();
    }
    QSqlDatabase dbase=QSqlDatabase::database("libdb",false);
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
        QSettings* settings=GetSettings();
        idCurrentLib=settings->value("LibID",-1).toLongLong();
        QSqlQuery query(QSqlDatabase::database("libdb"));
        query.exec("SELECT id,name,path,inpx,firstauthor, woDeleted FROM lib ORDER BY name");
        mLibs.clear();
        while(query.next())
        {
            uint idLib = query.value(0).toUInt();
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
    a.setStyleSheet("QComboBox { combobox-popup: 0; }");

    QCommandLineOption trayOption("tray", "minimize to tray on start");
    CMDparser.addOption(trayOption);
    CMDparser.process(a);

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

    translator=0;
    translator_qt=0;
    app=&a;
    a.setOrganizationName("freeLib");
    app->setAttribute(Qt::AA_UseHighDpiPixmaps);
    QSqlDatabase::addDatabase("QSQLITE","libdb");

    SetLocale();

    QString HomeDir="";
    if(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).count()>0)
        HomeDir=QStandardPaths::standardLocations(QStandardPaths::HomeLocation).at(0);
    QDir::setCurrent(HomeDir);

    QPixmap pixmap(QString(":/splash%1.png").arg(app->devicePixelRatio()>=2?"@2x":""));
    pixmap.setDevicePixelRatio(app->devicePixelRatio());
    QPainter painter(&pixmap);
    painter.setFont(QFont(painter.font().family(),VERSION_FONT,QFont::Bold));
    painter.setPen(Qt::white);
    painter.drawText(QRect(30,140,261,111),Qt::AlignLeft|Qt::AlignVCenter,PROG_VERSION);
    splash=new QSplashScreen(pixmap);
    splash->resize(640,400);
    QRect screenRect=QDesktopWidget().screenGeometry();
    splash->move(
                (screenRect.width()-splash->width())/2,
                (screenRect.height()-splash->height())/2);
#ifdef Q_OS_LINUX
    splash->setWindowIcon(QIcon(":/library_128x128.png"));
#endif
    QSettings* settings=GetSettings();
    if(!settings->contains("ApplicationMode"))
    {
        ResetToDefaultSettings();
    }

    if(!settings->value("no_splash",false).toBool())
        splash->show();
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
        if(!CMDparser.isSet("tray") && settings->value("tray_icon",0).toInt()!=2)
            w.show();
    }
    else{
        return 1;
    }
    splash->finish(&w);
    //current_lib.UpdateLib();
    if(idCurrentLib<0 && settings->value("ApplicationMode",0).toInt()==0)
        w.newLibWizard(false);
    int result=a.exec();
    if(global_settings)
    {
        global_settings->sync();
        delete global_settings;
    }

    return result;
}
