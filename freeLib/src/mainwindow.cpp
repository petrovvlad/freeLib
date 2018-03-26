#include <QtSql>
#include <QtXml/QDomDocument>
#include <QBuffer>
#include <QByteArray>
#include <QSqlDriver>
#include <QSystemTrayIcon>
#include <QCommandLineParser>
#include <QSplashScreen>
#include <QTextCodec>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "common.h"
#include "quazip/quazip/quazip.h"
#include "quazip/quazip/quazipfile.h"
#include "addlibrary.h"

#include "SmtpClient/smtpclient.h"
#include "settingsdlg.h"
#include "SmtpClient/mimefile.h"
#include "SmtpClient/mimetext.h"
#include "SmtpClient/mimeattachment.h"
#include "exportdlg.h"
#include "exportthread.h"
#include "aboutdialog.h"
#include "tagdialog.h"
#include "libwizard.h"
#include "bookeditdlg.h"
#include "webpage.h"

extern QSplashScreen *splash;

HelpDialog *Help_dlg;
bool db_is_open;


QString ToIndex(QString str)
{
    //return str.toUpper().replace("Ё","Ее");
    QString fn;
    int i, rU, rL;
    QString rusUpper = QString("АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ0123456789");
    QString rusLower = QString("абвгдеёжзийклмнопрстуфхцчшщъыьэюя");
    QStringList latUpper;
    latUpper <<"01-"<<"02-"<<"03-"<<"04-"<<"05-"<<"06-"<<"07-"<<"08-"<<"09-"<<"10-"<<"11-"<<"12-"<<"13-"<<"14-"<<"15-"
        <<"16-"<<"17-"<<"18-"<<"19-"<<"20-"<<"21-"<<"22-"<<"23-"<<"24-"<<"25-"<<"26-"<<"27-"<<"28-"<<"29-"<<"30-"<<"31-"<<"32-"<<"33-"
        <<"!0-"<<"!1-"<<"!2-"<<"!3-"<<"!4-"<<"!5-"<<"!6-"<<"!7-"<<"!8-"<<"!9-";
    for (i=0; i < str.size(); ++i)
    {
        if ( !rusUpper.contains(str[i]) && !rusLower.contains(str[i]))
        {
            fn = fn + "z"+str[i].toUpper()+"-";
        }
        else
        {
            rU = rusUpper.indexOf(str[i]);
            rL = rusLower.indexOf(str[i]);
            if (rU >= 0)
                fn = fn + latUpper[rU];
            else
                fn = fn + latUpper[rL];
        }
    }
    return fn;
}

QFileInfo GetBookFile(QBuffer &buffer,QBuffer &buffer_info, qlonglong id_book, bool caption, QDateTime *file_data)
{
    QSqlQuery query(QSqlDatabase::database("libdb"));
    query.exec("SELECT file,archive,format,id_lib,path from book left join lib ON lib.id=id_lib where book.id="+QString::number(id_book));
    QString file,archive;
    QFileInfo fi;
    while(query.next())
    {
        QString LibPath=current_lib.path;
        if(!query.value(4).toString().trimmed().isEmpty())
            LibPath=query.value(4).toString().trimmed();

        LibPath=RelativeToAbsolutePath(LibPath);
        if(query.value(1).toString().trimmed().isEmpty())
        {
            file=LibPath+"/"+query.value(0).toString().trimmed()+"."+query.value(2).toString().trimmed();
        }
        else
        {
            file=query.value(0).toString().trimmed()+"."+query.value(2).toString().trimmed();
            archive=LibPath+"/"+query.value(1).toString().trimmed().replace(".inp",".zip");
        }
    }
    archive=archive.replace("\\","/");
    if(archive.isEmpty())
    {
        QFile book_file(file);
        if(!book_file.open(QFile::ReadOnly))
        {
            qDebug()<<("Error open file!")<<" "<<file;
            return fi;
        }
        buffer.setData(book_file.readAll());
        fi.setFile(book_file);
        if(file_data)
        {
            *file_data=fi.created();
        }
        fi.setFile(file);
        QString fbd=fi.absolutePath()+"/"+fi.completeBaseName()+".fbd";
        QFile info_file(fbd);
        if(info_file.exists())
        {
            info_file.open(QFile::ReadOnly);
            buffer_info.setData(info_file.readAll());
        }
    }
    else
    {
        QuaZip uz(archive);
        if (!uz.open(QuaZip::mdUnzip))
        {
            qDebug()<<("Error open archive!")<<" "<<archive;
            return fi;
        }

        if(file_data)
        {
            SetCurrentZipFileName(&uz,file);
            QuaZipFileInfo64 zip_fi;
            if(uz.getCurrentFileInfo(&zip_fi))
            {
                *file_data=zip_fi.dateTime;
            }
        }
        QuaZipFile zip_file(&uz);
        SetCurrentZipFileName(&uz,file);
        if(!zip_file.open(QIODevice::ReadOnly))
        {
            qDebug()<<"Error open file: "<<file;
        }
        if(caption)
        {
            buffer.setData(zip_file.read(16*1024));
        }
        else
        {
            buffer.setData(zip_file.readAll());
        }
        zip_file.close();
        fi.setFile(file);
        QString fbd=fi.path()+"/"+fi.completeBaseName()+".fbd";

        if(SetCurrentZipFileName(&uz,fbd))
        {
            zip_file.open(QIODevice::ReadOnly);
            buffer.setData(zip_file.readAll());
            zip_file.close();
        }

        fi.setFile(archive+"/"+file);
    }
    return fi;
}

QPixmap GetTag(QColor color,int size)
{
    QPixmap pixmap(size,size-4);
    pixmap.fill(Qt::transparent);
    QPainter paint(&pixmap);
    paint.setBrush(QBrush(color));
    QPen pen=QPen(QColor(color.red()*0.5,color.green()*0.5,color.blue()*0.5,color.alpha()*0.5));
    paint.setPen(pen);
    paint.drawEllipse(2,0,size-5,size-5);
    return pixmap;
}
int sqliteLocaleAwareCompare(void* /*arg*/, int len1, const void* data1, int len2, const void* data2)
{
    QString string1 = QString::fromRawData( reinterpret_cast<const QChar*>( data1 ),
         len1 / sizeof( QChar ) );
     QString string2 = QString::fromRawData( reinterpret_cast<const QChar*>( data2 ),
         len2 / sizeof( QChar ) );
     return QString::localeAwareCompare( string1, string2 );
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


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    db_is_open=false;
    trIcon=0;
    dropForm=0;
    error_quit=false;
    if(!openDB(false,false))
        error_quit=true;
    if(db_is_open)
    {
        QSqlQuery query(QSqlDatabase::database("libdb"));
        query.exec("CREATE TABLE IF NOT EXISTS params (id INTEGER PRIMARY KEY, name TEXT, value TEXT)");
        query.exec(QString("SELECT value FROM params WHERE name='%1'").arg("version"));
        int version=0;
        if(query.next())
        {
            version=query.value(0).toInt();
        }
        switch(version)
        {
        case 0:
        {
            query.exec("INSERT INTO params (name,value) VALUES ('version','1')");
            query.exec("CREATE INDEX \"seria_name\" ON \"seria\" (\"name\" ASC, \"id_lib\" ASC)");
        }
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        {
            splash->hide();
            if(QMessageBox::question(0,tr("Database"),tr("This version needs new database version. All your old books data will be lost. Continue?"),QMessageBox::Yes|QMessageBox::No,QMessageBox::No)==QMessageBox::Yes)
            {
                if(!openDB(false,true))
                    error_quit=true;
            }
            else
            {
                error_quit=true;
            }
        }
        default:
            break;
        }
    }

    ui->setupUi(this);
    ui->btnEdit->setVisible(false);
    ui->searchString->setFocus();
    ui->tabWidget->setCurrentIndex(0);
    QSettings *settings=GetSettings();
    ui->Books->setColumnWidth(0,400);
    ui->Books->setColumnWidth(1,50);
    ui->Books->setColumnWidth(2,100);
    ui->Books->setColumnWidth(3,90);
    ui->Books->setColumnWidth(4,120);
    ui->Books->setColumnWidth(5,250);

    ui->Review->setPage(new WebPage());
    connect(ui->Review->page(),SIGNAL(linkClicked(QUrl)),this,SLOT(ReviewLink(QUrl)));

    current_lib.UpdateLib();
    setWindowTitle(AppName+(current_lib.name.isEmpty()||current_lib.id<0?"":" - "+current_lib.name));

    ui->searchString->setMaxLength(20);
    tbClear=new QToolButton(this);
    tbClear->setFocusPolicy(Qt::NoFocus);
    tbClear->setIcon(QIcon(":/icons/img/icons/clear.png"));
    tbClear->setStyleSheet("border: none;");
    tbClear->setCursor(Qt::ArrowCursor);
    tbClear->setVisible(false);
    QHBoxLayout* layout=new QHBoxLayout(ui->searchString);
    layout->addWidget(tbClear,0,Qt::AlignRight);
    layout->setSpacing(0);
    layout->setMargin(0);

    searchTimer.setSingleShot(true);
    connect(ui->searchString,SIGNAL(textEdited(QString)),this,SLOT(TimerResetSearch()));
    connect(tbClear,SIGNAL(clicked()),this,SLOT(searchClear()));
    connect(ui->actionAddLibrary,SIGNAL(triggered()),this,SLOT(Add_Library()));
    connect(ui->btnLibrary,SIGNAL(clicked()),this,SLOT(Add_Library()));
    connect(ui->btnOpenBook,SIGNAL(clicked()),this,SLOT(BookDblClick()));
    connect(ui->btnOption,SIGNAL(clicked()),this,SLOT(Settings()));
    connect(ui->actionPreference,SIGNAL(triggered()),this,SLOT(Settings()));
    connect(ui->actionCheck_uncheck,SIGNAL(triggered()),this,SLOT(CheckBooks()));
    connect(ui->btnCheck,SIGNAL(clicked()),this,SLOT(CheckBooks()));
    connect(ui->btnEdit,SIGNAL(clicked()),this,SLOT(EditBooks()));
    connect(&searchTimer,SIGNAL(timeout()),this,SLOT(TimerSearch()));
    //connect(ui->btnSendMail,SIGNAL(clicked()),this,SLOT(SendMail()));
    //connect(ui->btnExport,SIGNAL(clicked()),this,SLOT(SendToDevice()));
    connect(ui->actionExit,SIGNAL(triggered()),this,SLOT(close()));
    #ifdef Q_OS_MACX
        ui->actionExit->setShortcut(QKeySequence(Qt::CTRL|Qt::Key_Q));
    #endif
    #ifdef Q_OS_LINUX
        ui->actionExit->setShortcut(QKeySequence(Qt::CTRL|Qt::Key_Q));
        setWindowIcon(QIcon(":/library.png"));
    #endif
    #ifdef Q_OS_WIN
        ui->actionExit->setShortcut(QKeySequence(Qt::ALT|Qt::Key_F4));
    #endif
    #ifdef Q_OS_WIN32
        ui->actionExit->setShortcut(QKeySequence(Qt::ALT|Qt::Key_F4));
    #endif

    connect(ui->AuthorList,SIGNAL(itemSelectionChanged()),this,SLOT(SelectAuthor()));
    connect(ui->Books,SIGNAL(itemSelectionChanged()),this,SLOT(SelectBook()));
    connect(ui->Books,SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),this,SLOT(BookDblClick()));
    connect(ui->JanreList,SIGNAL(itemSelectionChanged()),this,SLOT(SelectJanre()));
    connect(ui->SeriaList,SIGNAL(itemSelectionChanged()),this,SLOT(SelectSeria()));

    connect(ui->btnAuthor,SIGNAL(clicked()),this,SLOT(btnAuthor()));
    connect(ui->btnJanre,SIGNAL(clicked()),this,SLOT(btnJanres()));
    connect(ui->btnSeries,SIGNAL(clicked()),this,SLOT(btnSeries()));
    connect(ui->btnSearch,SIGNAL(clicked()),this,SLOT(btnPageSearch()));
    connect(ui->do_search,SIGNAL(clicked()),this,SLOT(StartSearch()));
    connect(ui->s_author,SIGNAL(returnPressed()),this,SLOT(StartSearch()));
    connect(ui->s_seria,SIGNAL(returnPressed()),this,SLOT(StartSearch()));
    connect(ui->s_name,SIGNAL(returnPressed()),this,SLOT(StartSearch()));

    connect(ui->actionAbout,SIGNAL(triggered()),this,SLOT(About()));
    connect(ui->actionNew_labrary_wizard,SIGNAL(triggered()),this,SLOT(newLibWizard()));

    //ui->Review->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    //connect(ui->Review,SIGNAL(linkClicked(QUrl)),this,SLOT(ReviewLink(QUrl)));
   // ui->Review->page()->mainFrame()->setScrollBarPolicy(Qt::Vertical,Qt::ScrollBarAlwaysOn);

    ChangingLanguage(false);

    current_list_id=-1;
    stored_book=-1;

    ExportBookListBtn(false);
    UpdateTags();
    if(settings->value("store_position",false).toBool())
    {
        current_list_id=settings->value("current_list_id",0).toLongLong();
        stored_book=settings->value("current_book_id",0).toLongLong();
        UpdateJanre();
        switch(settings->value("current_tab",0).toInt())
            {
            case 1:
                ui->btnSeries->click();
                break;
            case 2:
                ui->btnJanre->click();
                break;
            case 3:
                ui->btnSearch->click();
                break;
            }
        if(settings->contains("filter_set"))
        {
            ui->searchString->setText(settings->value("filter_set").toString());
            searchCanged(settings->value("filter_set").toString());
        }
        else
        {
            FirstButton->click();
        }
    }
    else
    {
        UpdateJanre();
        FirstButton->click();
    }
    ui->date_to->setDate(QDate::currentDate());
    Help_dlg=0;
    connect(ui->actionHelp,SIGNAL(triggered()),this,SLOT(HelpDlg()));
    ui->Books->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->Books,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(ContextMenu(QPoint)));
    CurrentBookID=0;
    CurrentSeriaID=0;
    ui->AuthorList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->AuthorList,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(ContextMenu(QPoint)));
    ui->SeriaList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->SeriaList,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(ContextMenu(QPoint)));
    opds.server_run();
    FillLibrariesMenu();
    UpdateExportMenu();

    mode=(APP_MODE)settings->value("ApplicationMode",0).toInt();
    switch(mode)
    {
    case MODE_LIBRARY:
        on_actionSwitch_to_library_mode_triggered();
        break;
    case MODE_SHELF:
        //on_actionSwitch_to_shelf_mode_triggered();
        break;
    default:
        connect(this, SIGNAL(window_loaded()), this, SLOT(on_actionSwitch_to_convert_mode_triggered()));
        on_actionSwitch_to_convert_mode_triggered();
        break;
    }
    setMouseTracking(true);
    centralWidget()->setMouseTracking(true);
    ui->label_drop->setMouseTracking(true);
    ui->frame_drop->setMouseTracking(true);
    ui->pageConvert->setMouseTracking(true);
    ui->stackedWidget->setMouseTracking(true);

    ui->stacked_books->setCurrentWidget(ui->page_books);
    ChangingTrayIcon();

#ifdef Q_OS_OSX
    connect(MyPrivate::instance(), SIGNAL(dockClicked()), SLOT(dockClicked()));
#endif
    connect(ui->actionMinimize_window,SIGNAL(triggered(bool)),SLOT(MinimizeWindow()));

}

void MainWindow::showEvent(QShowEvent *ev)
{
    QMainWindow::showEvent(ev);
    emit window_loaded();
}

QPixmap MainWindow::GetTag(int id)
{
    foreach(Stag tag,tags_pic)
    {
        if(tag.id==id)
            return tag.pm;
    }

    return QPixmap();
}

void MainWindow::UpdateTags()
{
    if(!db_is_open)
        return;
    QButtonGroup *group=new QButtonGroup(this);
    group->setExclusive(true);
    disconnect(ui->TagFilter,SIGNAL(currentIndexChanged(int)),this,SLOT(tag_select(int)));
    int size =ui->TagFilter->style()->pixelMetric(QStyle::PM_SmallIconSize)*app->devicePixelRatio();
    QSqlQuery query(QSqlDatabase::database("libdb"));
    query.exec("SELECT color,name,id from favorite");
    ui->TagFilter->clear();
    int con=1;
    ui->TagFilter->addItem("*",-2);
    QSettings *settings=GetSettings();
    TagMenu.clear();
    QAction *ac=new QAction(tr("no tag"),&TagMenu);
    ac->setData(0);
    connect(ac,SIGNAL(triggered()),this,SLOT(set_tag()));
    TagMenu.addAction(ac);
    tags_pic.clear();
    QPixmap pix=::GetTag(QColor(0,0,0,0),size);
    pix.setDevicePixelRatio(app->devicePixelRatio());
    Stag new_tag={pix,0};
    tags_pic<<new_tag;
    bool use_tag=settings->value("use_tag",true).toBool();
    ui->TagFilter->setVisible(use_tag);
    ui->tag_label->setVisible(use_tag);

    while(query.next())
    {
        ui->TagFilter->addItem(query.value(1).toString().trimmed(),query.value(2).toInt());
        if(settings->value("current_tag").toInt()==ui->TagFilter->count()-1 && use_tag)
            ui->TagFilter->setCurrentIndex(ui->TagFilter->count()-1);
        pix=::GetTag(QColor(query.value(0).toString().trimmed()),size);
        Stag new_tag={pix,query.value(2).toInt()};
        tags_pic<<new_tag;
        ui->TagFilter->setItemData(con, pix, Qt::DecorationRole);//Добавляем изображение цвета в комбо
        con++;
        QAction *ac=new QAction(pix,query.value(1).toString().trimmed(),&TagMenu);
        ac->setData(query.value(2).toString());
        connect(ac,SIGNAL(triggered()),this,SLOT(set_tag()));
        TagMenu.addAction(ac);
    }

    ui->TagFilter->addItem(tr("setup ..."),-1);
    connect(ui->TagFilter,SIGNAL(currentIndexChanged(int)),this,SLOT(tag_select(int)));
}

void MainWindow::TimerSearch()
{
    searchCanged(ui->searchString->text());
}

void MainWindow::TimerResetSearch()
{
    searchTimer.start(400);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::CheckBooks()
{
    QList<book_info> book_list;
    FillCheckedBookList(book_list,0,false,true,true);

    ui->Books->blockSignals(true);
    Qt::CheckState cs=book_list.count()>0?Qt::Unchecked:Qt::Checked;
    for(int i=0;i<ui->Books->topLevelItemCount();i++)
    {
        ui->Books->topLevelItem(i)->setCheckState(0,cs);
        CheckChild(ui->Books->topLevelItem(i));
    } 
    ui->Books->blockSignals(false);
}

void MainWindow::EditBooks()
{
    BookEditDlg dlg(this);
    dlg.exec();
}

void MainWindow::newLibWizard(bool AddLibOnly)
{
    LibWizard wiz(this);
    if((AddLibOnly?wiz.AddLibMode():wiz.exec())==QDialog::Accepted || wiz.mode==MODE_CONVERTER)
    {
        if(wiz.mode==MODE_CONVERTER)
        {
            on_actionSwitch_to_convert_mode_triggered();
            return;
        }
        AddLibrary al(this);
        al.AddNewLibrary(wiz.name,wiz.dir,wiz.inpx);
        QSettings *settings=GetSettings();
        current_list_id=settings->value("current_list_id",0).toLongLong();
        stored_book=settings->value("current_book_id",0).toLongLong();
        UpdateJanre();
        UpdateTags();
        searchCanged(ui->searchString->text());
        setWindowTitle(AppName+(current_lib.name.isEmpty()||current_lib.id<0?"":" - "+current_lib.name));
        FillLibrariesMenu();
    }
}

void MainWindow::ReviewLink(QUrl url)
{
    if(url.toString().startsWith("file:///author_"))
    {
        MoveToAuthor(url.toString().right(url.toString().length()-8-8).toLongLong(),url.toString().mid(7+8,1).toUpper());
    }
    else if(url.toString().startsWith("file:///janre_"))
    {
        MoveToJanre(url.toString().right(url.toString().length()-7-8).toLongLong());
    }
    else if(url.toString().startsWith("file:///seria_"))
    {
        MoveToSeria(url.toString().right(url.toString().length()-7-8).toLongLong(),url.toString().mid(6+8,1).toUpper());
    }
    else if(url.toString().startsWith("file:///show_fileinfo"))
    {
        QSettings *settings=GetSettings();
        settings->setValue("show_fileinfo",!settings->value("show_fileinfo",false).toBool());
        settings->sync();
        SelectBook();
    }
}

void MainWindow::update_list_pix(qlonglong id, int list,int tag_id)
{
    switch(list)
    {
    case 1: //авторы
        for(int i=0;i<ui->AuthorList->count();i++)
        {
            if(ui->AuthorList->item(i)->data(Qt::UserRole).toLongLong()==id)
            {
                ui->AuthorList->item(i)->setIcon(GetTag(tag_id));
            }
        }
        break;
    case 2: //серии
        for(int i=0;i<ui->SeriaList->count();i++)
        {
            if(ui->SeriaList->item(i)->data(Qt::UserRole).toLongLong()==id)
            {
                ui->SeriaList->item(i)->setIcon(GetTag(tag_id));
            }
        }

        break;
    }
    for(int i=0;i<ui->Books->topLevelItemCount();i++)
    {
        if(list==1)
        {
            if(ui->Books->topLevelItem(i)->data(0,Qt::UserRole).toLongLong()==-id)
                ui->Books->topLevelItem(i)->setIcon(0,GetTag(tag_id));
        }
        else
        {
            for(int j=0;j<ui->Books->topLevelItem(i)->childCount();j++)
            {
                if(ui->Books->topLevelItem(i)->child(j)->data(0,Qt::UserRole).toLongLong()==-id)
                    ui->Books->topLevelItem(i)->child(j)->setIcon(0,GetTag(tag_id));
            }
        }
    }

}
void MainWindow::ChangingLanguage(bool change_language)
{
    QSettings *settings=GetSettings();
    QFile file(FindLocaleFile(settings->value("localeABC",QLocale::system().name()).toString(),"abc","txt"));
    QString abc_local="ABC";
    if(!file.fileName().isEmpty() && file.open(QFile::ReadOnly))
    {
        abc_local=QString::fromUtf8(file.readLine()).toUpper();
    }
    QVBoxLayout *layout_abc_all=new QVBoxLayout();

    if(ui->abc->layout())
    {

        while(!((QHBoxLayout *)ui->abc->layout()->itemAt(0))->isEmpty())
        {
            delete ((QHBoxLayout *)ui->abc->layout()->itemAt(0))->itemAt(0)->widget();
        }
        if(!((QHBoxLayout *)ui->abc->layout()->isEmpty()))
        {
            while(!((QHBoxLayout *)ui->abc->layout()->itemAt(1))->isEmpty())
            {
                delete ((QHBoxLayout *)ui->abc->layout()->itemAt(1))->itemAt(0)->widget();
            }
        }

        while(!ui->abc->layout()->isEmpty())
        {
            delete ui->abc->layout()->itemAt(0);
        }
        delete ui->abc->layout();
    }
   //
    FirstButton=0;
    if(abc_local!="ABC")
    {
        QHBoxLayout *layout_abc=new QHBoxLayout();
        for(int i=0;i<abc_local.length();i++)
        {
            QToolButton *btn=new QToolButton(this);
            btn->setText(abc_local.at(i));
            btn->setMaximumSize(QSize(22,22));
            btn->setMinimumSize(QSize(22,22));
            btn->setCheckable(true);
            btn->setAutoExclusive(true);
            btn->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
            layout_abc->addWidget(btn);
            connect(btn,SIGNAL(clicked()),this,SLOT(btnSearch()));
            if(!FirstButton)
                FirstButton=btn;
        }
        layout_abc->addStretch();
        layout_abc->setSpacing(1);
        layout_abc->setMargin(0);
        layout_abc_all->addItem(layout_abc);
    }
        QString abc="*#ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        {
            QHBoxLayout *layout_abc=new QHBoxLayout();
            for(int i=0;i<abc.length();i++)
            {
                QToolButton *btn=new QToolButton(this);
                btn->setText(abc.at(i));
                btn->setMaximumSize(QSize(22,22));
                btn->setMinimumSize(QSize(22,22));
                btn->setCheckable(true);
                btn->setAutoExclusive(true);
                btn->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
                layout_abc->addWidget(btn);
                connect(btn,SIGNAL(clicked()),this,SLOT(btnSearch()));
                if(!FirstButton && abc.at(i)=='A')
                    FirstButton=btn;
                if(abc.at(i)=='#')
                    btn_Hash=btn;
            }
            layout_abc->addStretch();
            layout_abc->setSpacing(1);
            layout_abc->setMargin(0);
#ifdef Q_OS_WIN
            layout_abc->setContentsMargins(0,abc_local!="ABC"?4:0,0,0);
#else
            layout_abc->setContentsMargins(0,abc_local!="ABC"?5:0,0,5);
#endif
            layout_abc_all->addItem(layout_abc);
        }

    ui->abc->setLayout(layout_abc_all);
    ui->abc->layout()->setSpacing(1);
    ui->abc->layout()->setMargin(0);
#ifdef Q_OS_WIN
    ui->abc->layout()->setContentsMargins(0,4,0,5);
#endif
    ui->retranslateUi(this);
    if(change_language)
        FirstButton->click();
}

void MainWindow::set_tag()
{
    QString tag_id=((QAction*)QObject::sender())->data().toString();
    qlonglong id;
    QSqlQuery query(QSqlDatabase::database("libdb"));
    if(current_list_for_tag==(QObject*)ui->Books)
    {
        QTreeWidgetItem* item=ui->Books->selectedItems()[0];
        id=item->data(0,Qt::UserRole).toLongLong();
        if(id<0)
        {
            if(item->parent())
            {
                update_list_pix(-id,2,tag_id.toInt());
                query.exec(QString("UPDATE seria set favorite=%1 where id=%2").arg(tag_id,QString::number(-id)));
            }
            else
            {
                update_list_pix(-id,1,tag_id.toInt());
                query.exec(QString("UPDATE author set favorite=%1 where id=%2").arg(tag_id,QString::number(-id)));
            }
        }
        else
        {
            //update_list_pix(id,3,tag_id.toInt());
            item->setIcon(0,GetTag(tag_id.toInt()));
            query.exec(QString("UPDATE book set favorite=%1 where id=%2").arg(tag_id,QString::number(id)));
        }

    }
    else if(current_list_for_tag==(QObject*)ui->AuthorList)
    {
        id=ui->AuthorList->selectedItems()[0]->data(Qt::UserRole).toLongLong();
        update_list_pix(id,1,tag_id.toInt());
        query.exec(QString("UPDATE author set favorite=%1 where id=%2").arg(tag_id,QString::number(id)));
    }
    else if(current_list_for_tag==(QObject*)ui->SeriaList)
    {
        id=ui->SeriaList->selectedItems()[0]->data(Qt::UserRole).toLongLong();
        update_list_pix(id,2 ,tag_id.toInt());
        query.exec(QString("UPDATE seria set favorite=%1 where id=%2").arg(tag_id,QString::number(id)));
    }
}

void MainWindow::tag_select(int index)
{
    QSettings *settings=GetSettings();
    if(ui->TagFilter->itemData(ui->TagFilter->currentIndex()).toInt()==-1)
    {
        disconnect(ui->TagFilter,SIGNAL(currentIndexChanged(int)),this,SLOT(tag_select(int)));
        ui->TagFilter->setCurrentIndex(settings->value("current_tag",0).toInt());
        connect(ui->TagFilter,SIGNAL(currentIndexChanged(int)),this,SLOT(tag_select(int)));
        TagDialog td(this);
        if(td.exec())
            UpdateTags();
    }
    else if(index>=0)
    {
        settings->setValue("current_tag",index);
        UpdateJanre();
        UpdateAuthor();
        UpdateSeria();
    }
    settings->sync();
}

void MainWindow::SaveLibPosition()
{
    QSettings *settings=GetSettings();
    settings->setValue("filter_set",ui->searchString->text());
    settings->setValue("current_tab",ui->tabWidget->currentIndex());
    qlonglong id=-1;
    switch (ui->tabWidget->currentIndex())
    {
    case 0:
        if(ui->AuthorList->selectedItems().count()>0)
        {
            id=ui->AuthorList->selectedItems()[0]->data(Qt::UserRole).toLongLong();
        }
        break;
    case 1:
        if(ui->SeriaList->selectedItems().count()>0)
        {
            id=ui->SeriaList->selectedItems()[0]->data(Qt::UserRole).toLongLong();
        }
        break;
    case 2:
        if(ui->JanreList->selectedItems().count()>0)
        {
            id=ui->JanreList->selectedItems()[0]->data(0,Qt::UserRole).toLongLong();
        }
        break;
    }
    settings->setValue("current_list_id",id);
    qlonglong book_id=-1;
    if(ui->Books->selectedItems().count()>0)
    {
        book_id=ui->Books->selectedItems()[0]->data(0,Qt::UserRole).toLongLong();
    }
    settings->setValue("current_book_id",book_id);
    settings->sync();
    current_list_id=id;
    stored_book=book_id;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if(Help_dlg!=0)
        delete Help_dlg;
    SaveLibPosition();
    QSettings *settings=GetSettings();
    settings->setValue("ApplicationMode", mode);
    if(mode==MODE_LIBRARY)
    {
        settings->setValue("MainWnd/geometry", saveGeometry());
        settings->setValue("MainWnd/windowState", saveState());
        settings->setValue("MainWnd/tab/geometry",ui->tabWidget->saveGeometry());
        settings->setValue("MainWnd/tab/geometry",ui->splitter->saveState());
        settings->setValue("MainWnd/books/geometry",ui->splitter_2->saveState());
        settings->setValue("MainWnd/books_head/geometry",ui->Books->header()->saveState());
    }
    else
    {
        settings->setValue("MainWndConvertMode/geometry", saveGeometry());
    }
    if(ui->btnExport->defaultAction())
        settings->setValue("DefaultExport",ui->btnExport->defaultAction()->data().toInt());
    settings->sync();
    QString TempDir="";
    if(QStandardPaths::standardLocations(QStandardPaths::TempLocation).count()>0)
        TempDir=QStandardPaths::standardLocations(QStandardPaths::TempLocation).at(0);
    QDir(TempDir+"/freeLib/").removeRecursively();
    QMainWindow::closeEvent(event);
    settings->sync();
}

void MainWindow::ChangingPort(int i)
{
    opds.server_run(i);
}

void MainWindow::Settings()
{
    QSettings *settings=GetSettings();
    bool ShowDeleted=settings->value("ShowDeleted").toBool();
    bool use_tag=settings->value("use_tag").toBool();
    if(ui->btnExport->defaultAction())
    {
        settings->setValue("DefaultExport",ui->btnExport->defaultAction()->data().toInt());
        settings->sync();
    }
    SettingsDlg dlg(this);
    connect(&dlg,SIGNAL(ChangingPort(int)),this,SLOT(ChangingPort(int)));
    connect(&dlg,SIGNAL(ChangingLanguage()),this,SLOT(ChangingLanguage()));
    connect(&dlg,SIGNAL(ChangingTrayIcon(int,int)),this,SLOT(ChangingTrayIcon(int,int)));
    dlg.exec();
    settings=GetSettings();
    settings->setValue("LibID",current_lib.id);
    settings->sync();
    if(ShowDeleted!=settings->value("ShowDeleted").toBool() || use_tag!=settings->value("use_tag").toBool())
    {
        UpdateTags();
        SaveLibPosition();
        UpdateJanre();
        UpdateAuthor();
        UpdateSeria();
    }
    opds.server_run();
    UpdateExportMenu();
    resizeEvent(0);
}
void MainWindow::FillCheckedBookList(QList<book_info> &list,QTreeWidgetItem* item,bool send_all,bool count_only,bool checked_only)
{
    FillCheckedItemsBookList(list,item,send_all,count_only);
    if(list.count()==0 && !checked_only)
    {
        if(ui->Books->selectedItems().count()>0)
        {
            if(ui->Books->selectedItems()[0]->childCount()>0)
                FillCheckedItemsBookList(list,ui->Books->selectedItems()[0],true,count_only);
            else
            {
                if(ui->Books->selectedItems()[0]->parent())
                {
                    qlonglong id_book=ui->Books->selectedItems()[0]->data(0,Qt::UserRole).toLongLong();
                    book_info bi;
                    if(!count_only)
                        bi.id=id_book;
                    list<<bi;
                }
            }
        }
    }
}

void MainWindow::FillCheckedItemsBookList(QList<book_info> &list,QTreeWidgetItem* item,bool send_all,bool count_only)
{
    QTreeWidgetItem* current;
    for(int i=0;i<(item?item->childCount():ui->Books->topLevelItemCount());i++)
    {
        current=item?item->child(i):ui->Books->topLevelItem(i);
        if(current->childCount()>0)
        {
            FillCheckedItemsBookList(list,current,send_all,count_only);
        }
        else
        {
            if(current->checkState(0)==Qt::Checked || send_all)
            {
                if(current->parent())
                {
                    qlonglong id_book=current->data(0,Qt::UserRole).toLongLong();
                    book_info bi;
                    if(!count_only)
                        bi.id=id_book;
                    list<<bi;
                }
            }
        }
    }
}

void MainWindow::LanguageChange()
{
    QSettings *settings=GetSettings();
    settings->setValue("BookLanguage",ui->language->currentText());
    settings->sync();
    UpdateJanre();
    UpdateAuthor();
    UpdateSeria();
}

void MainWindow::uncheck_books(QList<qlonglong> list)
{
    QSettings *settings=GetSettings();
    if(!settings->value("uncheck_export",true).toBool())
    {
        return;
    }
    QList<QTreeWidgetItem*> items;
    if(ui->Books->topLevelItemCount()==0)
    {
        return;
    }
    foreach (qlonglong id, list)
    {
        for(int i=0;i<ui->Books->topLevelItemCount();i++)
        {
            items<<ui->Books->topLevelItem(i);
        }
        do
        {
            if(items[0]->childCount()>0)
            {
                for(int j=0;j<items[0]->childCount();j++)
                {
                    items<<items[0]->child(j);
                }
            }
            else
            {
                if(items[0]->data(0,Qt::UserRole).toLongLong()==id && items[0]->checkState(0)==Qt::Checked)
                {
                    items[0]->setCheckState(0,Qt::Unchecked);
                }
            }
            items.removeAt(0);
        }while(items.count()>0);
        items.clear();
    }
}

void MainWindow::SendToDevice()
{
    QList<book_info> book_list;
    FillCheckedBookList(book_list);
    if(book_list.count()==0)
        return;
    ExportDlg dlg(this);
    dlg.exec(book_list,ST_Device,(ui->btnAuthor->isChecked()?ui->AuthorList->selectedItems()[0]->data(Qt::UserRole).toLongLong():0));
    uncheck_books(dlg.succesfull_export_books);
}

void MainWindow::SendMail()
{
    QList<book_info> book_list;
    FillCheckedBookList(book_list);
    if(book_list.count()==0)
        return;
    ExportDlg dlg(this);
    dlg.exec(book_list,ST_Mail,(ui->btnAuthor->isChecked()?ui->AuthorList->selectedItems()[0]->data(Qt::UserRole).toLongLong():0));
    uncheck_books(dlg.succesfull_export_books);
}


void MainWindow::BookDblClick()
{
    if(ui->Books->selectedItems().count()==0)
        return;
    QTreeWidgetItem* item=ui->Books->selectedItems()[0];
    QBuffer outbuff;
    QBuffer infobuff;
    QFileInfo fi=GetBookFile(outbuff,infobuff,item->data(0,Qt::UserRole).toInt());
    if(fi.fileName().isEmpty())
        return;
    QString TempDir="";
    if(QStandardPaths::standardLocations(QStandardPaths::TempLocation).count()>0)
        TempDir=QStandardPaths::standardLocations(QStandardPaths::TempLocation).at(0);
    QDir dir(TempDir+"/freeLib");
    dir.mkpath(dir.path());
    QFile file(dir.path()+"/"+fi.fileName());
    file.open(QFile::WriteOnly);
    file.write(outbuff.data());
    file.close();

    QSettings *settings=GetSettings();
    int count=settings->beginReadArray("application");
    for(int i=0;i<count;i++)
    {
        settings->setArrayIndex(i);
        if((settings->value("ext").toString()+";").toLower().contains(fi.suffix().toLower()+";"))
        {
            if(
#ifdef Q_OS_MACX
            QProcess::startDetached("open",QStringList()<<settings->value("app").toString()<<"--args"<<file.fileName())&&
                    QFileInfo(settings->value("app").toString()).exists()
#else
            QProcess::startDetached(settings->value("app").toString(),QStringList()<<file.fileName())
#endif
            )
                settings->endArray();
                return;
        }
    }
    settings->endArray();

    QDesktopServices::openUrl(QUrl::fromLocalFile(file.fileName()));
    settings->sync();
}

void MainWindow::CheckParent(QTreeWidgetItem *parent)
{
    bool checked=false;
    bool unchecked=false;
    bool partially=false;
    for(int i=0;i<parent->childCount();i++)
    {
        switch(parent->child(i)->checkState(0))
        {
        case Qt::Checked:
            checked=true;
            break;
        case Qt::Unchecked:
            unchecked=true;
            break;
        case Qt::PartiallyChecked:
            partially=true;
            break;
        }
    }
    if(partially || (checked && unchecked))
        parent->setCheckState(0,Qt::PartiallyChecked);
    else if(checked)
        parent->setCheckState(0,Qt::Checked);
    else
        parent->setCheckState(0,Qt::Unchecked);
    if(parent->parent())
        CheckParent(parent->parent());

}
void MainWindow::CheckChild(QTreeWidgetItem *parent)
{
    if(parent->childCount()>0)
    {
        for(int i=0;i<parent->childCount();i++)
        {
            parent->child(i)->setCheckState(0,parent->checkState(0));
            if(parent->child(i)->childCount()>0)
                CheckChild(parent->child(i));
        }
    }
}
void MainWindow::itemChanged(QTreeWidgetItem *item, int)
{
    disconnect(ui->Books,SIGNAL(itemChanged(QTreeWidgetItem*,int)),this,SLOT(itemChanged(QTreeWidgetItem*,int)));
    CheckChild(item);
    QTreeWidgetItem* parent=item->parent();
    if(parent)
        CheckParent(parent);
    QList<book_info> book_list;
    FillCheckedBookList(book_list,0,false,true);
    ExportBookListBtn(book_list.count()!=0);

    connect(ui->Books,SIGNAL(itemChanged(QTreeWidgetItem*,int)),this,SLOT(itemChanged(QTreeWidgetItem*,int)));
}

void MainWindow::FillBookList(QSqlQuery &query)
{
    disconnect(ui->Books,SIGNAL(itemChanged(QTreeWidgetItem*,int)),this,SLOT(itemChanged(QTreeWidgetItem*,int)));
    QTreeWidgetItem* item;
    QTreeWidgetItem* item_list;
    QTreeWidgetItem* item_author=0;
    QTreeWidgetItem* item_seria=0;
    QFont bold_font(ui->Books->font());
    bold_font.setBold(true);
    qlonglong id_book=0;
    qlonglong count=0;
    QTreeWidgetItem* ScrollItem=0;
    if(stored_book>0)
    {
        CurrentBookID=stored_book;
        if(!(ui->tabWidget->currentIndex()==0 && current_list_for_tag!=(QObject*)ui->AuthorList))
            stored_book=-1;
    }
    QSettings *settings=GetSettings();
    bool use_tag=settings->value("use_tag",true).toBool();

    QList<qlonglong> books_list;
    while (query.next())
    {
        if(books_list.contains(query.value(0).toLongLong()))
            continue;
        books_list<<query.value(0).toLongLong();
        if(id_book==query.value(0).toLongLong())
        {
            item->setText(5,item->text(5)+", "+query.value(8).toString().trimmed());
            continue;
        }
        item_list=new QTreeWidgetItem(ui->BooksList);
        id_book=query.value(0).toLongLong();
        if(item_author==0 || (item_author==0?true:item_author->text(0)!=query.value(2).toString().trimmed()))
        {
            item_author=new QTreeWidgetItem(ui->Books);
            ui->Books->addTopLevelItem(item_author);
            item_author->setText(0,query.value(2).toString().trimmed());
            item_author->setExpanded(true);
            item_author->setFont(0,bold_font);
            item_author->setCheckState(0,Qt::Unchecked);
            item_author->setData(0,Qt::UserRole,-query.value(10).toLongLong());
            if(use_tag)
                item_author->setIcon(0,GetTag(query.value(12).toInt()));
            item_seria=0;
        }
        item_list->setText(0,query.value(2).toString().trimmed());
        item_list->setCheckState(0,Qt::Unchecked);
        item_list->setData(0,Qt::UserRole,-query.value(10).toLongLong());
        if(use_tag)
            item_list->setIcon(0,GetTag(query.value(12).toInt()));

        if(query.value(3).toString().trimmed().isEmpty())
        {
            item_seria=0;
        }
        else
        {
            if(item_seria==0 || (item_seria==0?true:item_seria->text(0)!=query.value(3).toString().trimmed()))
            {
                item_seria=new QTreeWidgetItem(item_author);
                item_seria->setText(0,query.value(3).toString().trimmed());
                item_author->addChild(item_seria);
                item_seria->setExpanded(true);
                item_seria->setFont(0,bold_font);
                item_seria->setCheckState(0,Qt::Unchecked);
                item_seria->setData(0,Qt::UserRole,-query.value(11).toLongLong());
                if(use_tag)
                    item_seria->setIcon(0,GetTag(query.value(13).toInt()));
                if(query.value(11).toLongLong()==CurrentSeriaID)
                {
                    ScrollItem=item_seria;
                }
           }
        }
        item=new QTreeWidgetItem(item_seria==0?item_author:item_seria);
        item->setData(0,Qt::UserRole,query.value(0));
        item->setText(0,query.value(1).toString().trimmed());
        item->setText(1,(query.value(4).toString().trimmed()=="0"?"":query.value(4).toString().trimmed()));
        item->setTextAlignment(1, Qt::AlignRight);
        item->setText(2,(query.value(6).toString().trimmed()=="0"?"":query.value(6).toString().trimmed()));
        item->setTextAlignment(2, Qt::AlignRight);
        if(use_tag)
            item->setIcon(0,GetTag(query.value(14).toInt()));
        if(query.value(0).toLongLong()==CurrentBookID)
        {
            ScrollItem=item;
        }

        if(query.value(9).toBool())
        {
            item->setTextColor(0,QColor::fromRgb(96,96,96));
            item->setTextColor(1,QColor::fromRgb(96,96,96));
            item->setTextColor(2,QColor::fromRgb(96,96,96));
            item->setTextColor(3,QColor::fromRgb(96,96,96));
            item->setTextColor(4,QColor::fromRgb(96,96,96));
            item->setTextColor(5,QColor::fromRgb(96,96,96));
        }

        QPixmap pix(":/icons/img/icons/stars/"+query.value(5).toString().trimmed()+QString("star%1.png").arg(app->devicePixelRatio()>=2?"@2x":""));
        pix.setDevicePixelRatio(app->devicePixelRatio());
        item->setData(3,Qt::DecorationRole,pix);

        item->setTextAlignment(3,Qt::AlignHCenter);
        item->setText(4,query.value(7).toDate().toString("dd.MM.yyyy"));
        item->setTextAlignment(4, Qt::AlignCenter);
        item->setText(5,query.value(8).toString().trimmed());
        item->setCheckState(0,Qt::Unchecked);
        (item_seria==0?item_author:item_seria)->addChild(item);

        count++;
        if(count==ui->maxBooks->value() && ui->tabWidget->currentWidget()==ui->page)
            break;
    }
    ui->Books->scrollToTop();
    ui->find_books->setText((query.next()?">":"")+QString::number(count));
    CurrentBookID=0;
    CurrentSeriaID=0;
    if(ScrollItem)
    {
        ScrollItem->setSelected(true);
        ui->Books->scrollToItem(ScrollItem);
    }
    connect(ui->Books,SIGNAL(itemChanged(QTreeWidgetItem*,int)),this,SLOT(itemChanged(QTreeWidgetItem*,int)));
}

void MainWindow::ExportBookListBtn(bool Enable)
{
    ui->btnExport->setEnabled(Enable);
    ui->btnOpenBook->setEnabled(false);
}


void MainWindow::StartSearch()
{
    app->processEvents();
    ui->Books->clear();
    ExportBookListBtn(false);
    QSqlQuery query(QSqlDatabase::database("libdb"));
    QSettings *settings=GetSettings();
    bool ShowDeleted=settings->value("ShowDeleted").toBool();
    QStringList str_author=ui->s_author->text().split(' ');
    QString where_author;
    foreach (QString str, str_author)
    {
        if(str.trimmed().isEmpty())
            continue;
        where_author+=QString(where_author.isEmpty()?"":" AND ")+" author.rus_index LIKE '%"+ToIndex(str.trimmed().toLower())+"%'";
    }

    query.exec("SELECT book.id,book.name,name1||' '||name2||' '||name3,seria.name,num_in_seria,star,size,date,janre.name,deleted,author.id,seria.id,author.favorite,seria.favorite,book.favorite from book_author join book on book.id=book_author.id_book "
               "join author on id_author=author.id "
               "left join seria on id_seria=seria.id "
               "left join book_janre on book_janre.id_book=book.id "
               "left join janre on book_janre.id_janre=janre.id "
               "where book_author.id_lib="+QString::number(current_lib.id)+(ui->findLanguage->currentText()=="*"?"":" and UPPER(book.language)='"+ui->findLanguage->currentText()+"'")+
               " and date>='"+ui->date_from->date().toString("yyyy/MM/dd")+"' and date<='"+ui->date_to->date().toString("yyyy/MM/dd")+"'"+
               (ui->s_name->text().trimmed().length()==0?"":" and name_index LIKE '%"+ToIndex(ui->s_name->text().toLower().trimmed())+"%'")+
               (ui->s_author->text().trimmed().length()==0?"":" and "+where_author)+
               (ui->s_seria->text().trimmed().length()==0?"":" and seria.rus_index LIKE '%"+ToIndex(ui->s_seria->text().toLower().trimmed())+"%'")+
               (ui->s_janre->currentText()=="*"?"":
                                                (QString(" and janre.")+(ui->s_janre->itemData(ui->s_janre->currentIndex()).toLongLong()>0?"id":"id_parent")+"="+QString::number(abs(ui->s_janre->itemData(ui->s_janre->currentIndex()).toLongLong())))
               )+
               (!ShowDeleted?" and not deleted":"")+
               " order by author.rus_index, seria.name,num_in_seria,book.name");
    FillBookList(query);
}

void MainWindow::SelectJanre()
{
    ui->Books->clear();
    ExportBookListBtn(false);
    if(ui->JanreList->selectedItems().count()==0)
        return;
    QTreeWidgetItem* cur_item=ui->JanreList->selectedItems()[0];
    QSqlQuery query(QSqlDatabase::database("libdb"));
    QSettings *settings=GetSettings();
    bool ShowDeleted=settings->value("ShowDeleted").toBool();
    int current_tag=ui->TagFilter->itemData(ui->TagFilter->currentIndex()).toInt();
    query.exec("SELECT book.id,book.name,name1||' '||name2||' '||name3,seria.name,num_in_seria,star,size,date,janre.name,deleted,author.id,seria.id,author.favorite,seria.favorite,book.favorite from book_janre join book on book.id=book_janre.id_book "
               "join author on first_author_id=author.id "
               "left join seria on id_seria=seria.id "
               "left join janre on book_janre.id_janre=janre.id "
               "where book_janre.id_lib="+QString::number(current_lib.id)+(ui->language->currentText()=="*"?"":" and UPPER(book.language)='"+ui->language->currentText()+"'")+" and id_janre="+cur_item->data(0,Qt::UserRole).toString()+
               (!ShowDeleted?" and not deleted":"")+
               (current_tag>0?" and (book.favorite="+QString::number(current_tag)+" or author.favorite="+QString::number(current_tag)+" or seria.favorite="+QString::number(current_tag)+")":"")+
               " order by author.rus_index, seria.name,num_in_seria,book.name");
    FillBookList(query);
}

void MainWindow::SelectSeria()
{
    ui->Books->clear();
    ExportBookListBtn(false);
    if(ui->SeriaList->selectedItems().count()==0)
        return;
    QListWidgetItem* cur_item=ui->SeriaList->selectedItems()[0];
    QSqlQuery query(QSqlDatabase::database("libdb"));
    QSettings *settings=GetSettings();
    bool ShowDeleted=settings->value("ShowDeleted").toBool();
    int current_tag=ui->TagFilter->itemData(ui->TagFilter->currentIndex()).toInt();
    query.exec("SELECT book.id,book.name,name1||' '||name2||' '||name3,seria.name,num_in_seria,star,size,date,janre.name,deleted,author.id,seria.id,author.favorite,seria.favorite,book.favorite from book "
               "join author on first_author_id=author.id "
               "left join seria on id_seria=seria.id "
               "left join book_janre on book_janre.id_book=book.id "
               "left join janre on book_janre.id_janre=janre.id "
               "where book.id_lib="+QString::number(current_lib.id)+(ui->language->currentText()=="*"?"":" and UPPER(book.language)='"+ui->language->currentText()+"'")+" and id_seria="+cur_item->data(Qt::UserRole).toString()+
               (!ShowDeleted?" and not deleted":"")+
               (current_tag>0?" and (book.favorite="+QString::number(current_tag)+" or author.favorite="+QString::number(current_tag)+" or seria.favorite="+QString::number(current_tag)+")":"")+
               " order by author.rus_index, seria.name,num_in_seria,book.name");
    FillBookList(query);
}

void MainWindow::SelectAuthor()
{
    ui->Books->clear();
    ExportBookListBtn(false);
    if(ui->AuthorList->selectedItems().count()==0)
        return;
    QListWidgetItem* cur_item=ui->AuthorList->selectedItems()[0];
    QSqlQuery query(QSqlDatabase::database("libdb"));
    QSettings *settings=GetSettings();
    bool ShowDeleted=settings->value("ShowDeleted").toBool();
    int current_tag=ui->TagFilter->itemData(ui->TagFilter->currentIndex()).toInt();
    query.exec("SELECT book.id,book.name,name1||' '||name2||' '||name3,seria.name,num_in_seria,star,size,date,janre.name,deleted,author.id,seria.id,author.favorite,seria.favorite,book.favorite from book_author join book on book.id=book_author.id_book "
               "join author on id_author=author.id "
               "left join seria on id_seria=seria.id "
               "left join book_janre on book_janre.id_book=book.id "
               "left join janre on book_janre.id_janre=janre.id "
               "where book_author.id_lib="+QString::number(current_lib.id)+(ui->language->currentText()=="*"?"":" and UPPER(book.language)='"+ui->language->currentText()+"'")+" and id_author="+cur_item->data(Qt::UserRole).toString()+
               (!ShowDeleted?" and not deleted":"")+
               (current_tag>0?" and (book.favorite="+QString::number(current_tag)+" or author.favorite="+QString::number(current_tag)+" or seria.favorite="+QString::number(current_tag)+")":"")+
               " order by author.rus_index, seria.name,num_in_seria,book.name");
    FillBookList(query);
}
void MainWindow::UpdateAuthor()
{
    if(!db_is_open)
        return;
    QSqlQuery query(QSqlDatabase::database("libdb"));
    QSettings *settings=GetSettings();
    bool ShowDeleted=settings->value("ShowDeleted").toBool();
    int current_tag=ui->TagFilter->itemData(ui->TagFilter->currentIndex()).toInt();
    query.exec(QString("SELECT author.id,name1||' '||name2||' '||name3,count(),author.favorite FROM book_author join author on book_author.id_author=author.id join book on book.id=id_book left join seria on id_seria=seria.id where author.id_lib="+QString::number(current_lib.id)+
                       (current_tag>0?" and (book.favorite="+QString::number(current_tag)+" or author.favorite="+QString::number(current_tag)+" or seria.favorite="+QString::number(current_tag)+")":"")+
                       (ui->searchString->text()=="*"?"":(ui->searchString->text()=="#"?" and not((name1>='a' and name1<='z') or (name1>='A' and name1<='Z') or (author.rus_index>='01-' and substr(author.rus_index, 1, 3)<='33-' and substr(author.rus_index, 3, 1)='-')) ":" and  author.rus_index LIKE ('"+ToIndex(ui->searchString->text())+"%')"))+
                       (ui->language->currentText()=="*"?"":" and UPPER(book_author.language)='"+ui->language->currentText()+"'")+(!ShowDeleted?" and not deleted":"")+" GROUP BY author.id ORDER BY author.rus_index"));
    QListWidgetItem *item;
    ui->AuthorList->clear();
    bool use_tag=settings->value("use_tag",true).toBool();
    while(query.next())
    {
        item=new QListWidgetItem(query.value(1).toString().trimmed()+" ("+query.value(2).toString()+")");
        item->setData(Qt::UserRole,query.value(0));
        if(use_tag)
            item->setIcon(GetTag(query.value(3).toInt()));
        ui->AuthorList->addItem(item);
        if(query.value(0).toLongLong()==current_list_id)
        {
            item->setSelected(true);
            ui->AuthorList->scrollToItem(item);
        }
    }
    if(current_list_for_tag==(QObject*)ui->AuthorList)
        current_list_id=-1;
}
void MainWindow::UpdateSeria()
{
    if(!db_is_open)
        return;
    QSqlQuery query(QSqlDatabase::database("libdb"));
    QSettings *settings=GetSettings();
    bool ShowDeleted=settings->value("ShowDeleted").toBool();
    int current_tag=ui->TagFilter->itemData(ui->TagFilter->currentIndex()).toInt();
    query.exec(QString("SELECT seria.id,seria.name,count(),seria.favorite FROM book join seria on id_seria=seria.id left join author on book.first_author_id=author.id where book.id_lib="+QString::number(current_lib.id)+
                       (current_tag>0?" and (book.favorite="+QString::number(current_tag)+" or author.favorite="+QString::number(current_tag)+" or seria.favorite="+QString::number(current_tag)+")":"")+
                       (ui->searchString->text()=="*"?"":(ui->searchString->text()=="#"?" and not((name>='a' and name<='z') or (name>='A' and name<='Z') or (seria.rus_index>='01-' and substr(seria.rus_index, 1, 3)<='33-' and substr(seria.rus_index, 3, 1)='-')) ":" and  seria.rus_index LIKE ('"+ToIndex(ui->searchString->text())+"%')"))+
                       (!ShowDeleted?" and not deleted":"")+
                       (ui->language->currentText()=="*"?"":" and UPPER(book.language)='"+ui->language->currentText()+"'")+" GROUP BY seria.id ORDER BY seria.rus_index"));
    QListWidgetItem *item;
    ui->SeriaList->clear();
    bool use_tag=settings->value("use_tag",true).toBool();

    while(query.next())
    {
        item=new QListWidgetItem(query.value(1).toString().trimmed()+" ("+query.value(2).toString()+")");
        item->setData(Qt::UserRole,query.value(0));
        if(use_tag)
            item->setIcon(GetTag(query.value(3).toInt()));
        ui->SeriaList->addItem(item);
        if(query.value(0).toLongLong()==current_list_id)
        {
            item->setSelected(true);
            ui->SeriaList->scrollToItem(item);
         }
    }
    if(current_list_for_tag==(QObject*)ui->SeriaList)
        current_list_id=-1;
}
void MainWindow::UpdateJanre()
{
    if(!db_is_open)
        return;
    QSqlQuery query_child(QSqlDatabase::database("libdb"));
    QSqlQuery query(QSqlDatabase::database("libdb"));

    disconnect(ui->language,SIGNAL(currentIndexChanged(int)),this,SLOT(LanguageChange()));
    query.exec("SELECT DISTINCT UPPER(language) from book where id_lib="+QString::number(current_lib.id)+" ORDER BY UPPER(language)");
    ui->language->clear();
    ui->language->addItem("*");
    ui->language->setCurrentIndex(0);
    ui->findLanguage->clear();
    ui->findLanguage->addItem("*");
    ui->findLanguage->setCurrentIndex(0);
    QSettings *settings=GetSettings();
    QString Lang=settings->value("BookLanguage","*").toString();
    bool ShowDeleted=settings->value("ShowDeleted").toBool();
    while(query.next())
    {
        if(!query.value(0).toString().isEmpty())
        {
            ui->language->addItem(query.value(0).toString());
            ui->findLanguage->addItem(query.value(0).toString());
            if(query.value(0).toString()==Lang)
                ui->language->setCurrentIndex(ui->language->count()-1);
        }
    }
    settings->setValue("BookLanguage",ui->language->currentText());
    settings->sync();

    connect(ui->language,SIGNAL(currentIndexChanged(int)),this,SLOT(LanguageChange()));

    query.exec(QString("SELECT id,name,id_parent FROM janre where ifnull(id_parent,0)=0 OR id_parent='' ORDER BY sort_index"));
    QTreeWidgetItem *item;
    ui->JanreList->clear();
    QFont bold_font(ui->AuthorList->font());
    bold_font.setBold(true);
    ui->s_janre->clear();
    ui->s_janre->addItem("*");
    int current_tag=ui->TagFilter->itemData(ui->TagFilter->currentIndex()).toInt();
    while(query.next())
    {
        item=new QTreeWidgetItem(ui->JanreList);
        item->setData(0,Qt::UserRole,query.value(0));
        item->setText(0,query.value(1).toString().trimmed());
        item->setFont(0,bold_font);
        ui->JanreList->addTopLevelItem(item);
        ui->s_janre->addItem(query.value(1).toString().trimmed(),-query.value(0).toLongLong());
        item->setExpanded(true);
        if(query.value(0).toLongLong()==current_list_id)
        {
            item->setSelected(true);
            ui->JanreList->scrollToItem(item);
        }
        query_child.exec(QString("SELECT janre.id,janre.name,id_parent,count() FROM janre  join book_janre on id_janre=janre.id join book on id_book=book.id left join author on book.first_author_id=author.id left join seria on seria.id=book.id_seria where book_janre.id_lib="+
                                 QString::number(current_lib.id)+
                                 (current_tag>0?" and (book.favorite="+QString::number(current_tag)+" or author.favorite="+QString::number(current_tag)+" or seria.favorite="+QString::number(current_tag)+")":"")+
                                 (ui->language->currentText()=="*"?"":" and UPPER(book_janre.language)='"+ui->language->currentText()+"'")+
                                 (!ShowDeleted?" and not deleted":"")+
                                 " and id_parent="+query.value(0).toString().trimmed()+" GROUP BY janre.id,janre.name,id_parent ORDER BY sort_index"));
        while(query_child.next())
        {
            QTreeWidgetItem* new_item=new QTreeWidgetItem(item);
            new_item->setText(0,query_child.value(1).toString().trimmed()+" ("+query_child.value(3).toString()+")");
            new_item->setData(0,Qt::UserRole,query_child.value(0));
            item->addChild(new_item);
            ui->s_janre->addItem("   "+query_child.value(1).toString().trimmed(),query_child.value(0));
            if(query_child.value(0).toLongLong()==current_list_id)
            {
                new_item->setSelected(true);
                ui->JanreList->scrollToItem(new_item);
            }
        }
    }

    if(current_list_for_tag==(QObject*)ui->JanreList)
        current_list_id=-1;

}
void MainWindow::SelectBook()
{
    ui->Review->setHtml("");
    if(ui->Books->selectedItems().count()==0)
    {
        ExportBookListBtn(false);
        return;
    }
    QSettings *settings=GetSettings();
    ExportBookListBtn(true);
    QTreeWidgetItem* item=ui->Books->selectedItems()[0];
    if(item->childCount()!=0)
    {
        ui->btnOpenBook->setEnabled(false);
        ui->Review->setHtml("");
        return;
    }


    ui->btnOpenBook->setEnabled(true);
    if(ui->splitter->sizes()[1]>0)
    {
        QBuffer outbuff;
        QBuffer infobuff;
        QDateTime book_date;
        QFileInfo fi=GetBookFile(outbuff,infobuff,item->data(0,Qt::UserRole).toLongLong(),false,&book_date);
        book_info bi;
        if(fi.fileName().isEmpty())
        {
            GetBookInfo(bi,QByteArray(),"",true,item->data(0,Qt::UserRole).toInt());
            QSqlQuery query(QSqlDatabase::database("libdb"));
            query.exec("SELECT file,archive,format,id_lib,path from book left join lib ON lib.id=id_lib where book.id="+QString::number(item->data(0,Qt::UserRole).toLongLong()));
            QString file;
            while(query.next())
            {
                QString LibPath=current_lib.path;
                if(!query.value(4).toString().trimmed().isEmpty())
                    LibPath=query.value(4).toString().trimmed();
                if(query.value(1).toString().trimmed().isEmpty())
                {
                    file=LibPath+"/"+query.value(0).toString().trimmed()+"."+query.value(2).toString().trimmed();
                }
                else
                {
                    file=LibPath+"/"+query.value(1).toString().trimmed().replace(".inp",".zip");
                }
            }
            file=file.replace("\\","/");
            bi.annotation="<font color=\"red\">"+tr("Can't find file: %1").arg(file)+"</font>";
        }
        else
        {
            if(fi.fileName().right(3).toLower()=="fb2" || infobuff.size()>0)
            {
                GetBookInfo(bi,infobuff.size()==0?outbuff.data():infobuff.data(),"fb2",false,item->data(0,Qt::UserRole).toLongLong());
            }
            else if(fi.fileName().right(4).toLower()=="epub")
            {
                GetBookInfo(bi,outbuff.data(),"epub",false,item->data(0,Qt::UserRole).toLongLong());
            }
            else
            {
                GetBookInfo(bi,outbuff.data(),fi.suffix(),false,item->data(0,Qt::UserRole).toLongLong());
            }
        }

        QString seria;
        QTreeWidgetItem *parent=item->parent();
        if(parent->parent()) //если это серия
        {
            seria=QString("<a href=seria_%3%1>%2</a>").arg(QString::number(-parent->data(0,Qt::UserRole).toLongLong()),parent->text(0),parent->text(0).left(1).toUpper());
        }
        QString img_width="220";
        if(!bi.img.isEmpty())
            bi.img=bi.img.arg(img_width);


        QString author;
        foreach (author_info auth, bi.authors)
        {
            author+=(author.isEmpty()?"":"; ")+QString("<a href='author_%3%1'>%2</a>").arg(QString::number(auth.id),auth.author.replace(","," "),auth.author.left(1));
        }
        QString janre;
        foreach (genre_info jan, bi.genres)
        {
            janre+=(janre.isEmpty()?"":"; ")+QString("<a href='janre_%3%1'>%2</a>").arg(QString::number(jan.id),jan.genre,jan.genre.left(1));
        }
        QFile file_html(":/preview.html");
        file_html.open(QIODevice::ReadOnly);
        QString content(file_html.readAll());
        qint64 size=0;
        qint64 rest=0;
        int mem_i=0;
        QStringList mem;
        QFileInfo arh;
        mem<<tr("B")<<tr("kB")<<tr("MB")<<tr("GB")<<tr("TB")<<tr("PB");
        if(!fi.fileName().isEmpty())
        {
            arh=fi;
            while(!arh.exists())
            {
                arh.setFile(arh.absolutePath());
                if(arh.fileName().isEmpty())
                    break;
            }
            size=arh.size();
            rest=0;
            if(!arh.isDir() && arh.suffix().toLower()!="zip")
                arh.setFile(arh.absolutePath());
            mem_i=0;
            while(size>1024)
            {
                mem_i++;
                if(mem_i==mem.count())
                {
                    mem_i--;
                    break;
                }
                rest=size%1024;
                size=size/1024;
             }
        }
        content.replace("#annotation#",bi.annotation).
                replace("#title#",bi.title).
                replace("#image#",bi.img).
                replace("#width#",(bi.img.isEmpty()?"0":img_width)).
                replace("#author#",author).
                replace("#janre#",janre).
                replace("#series#",seria).
                replace("#file_path#",arh.filePath()).
                replace("#file_size#",QString::number(size)+(mem_i>0?"."+QString::number((rest*10+5)/1024):"")+" "+mem[mem_i]).
                replace("#file_data#",book_date.toString("dd.MM.yyyy hh:mm:ss")).
                replace("#file_name#",fi.fileName()).
                replace("#file_info#",settings->value("show_fileinfo",true).toBool()?"block":"none");
        ui->Review->page()->setHtml(content,QUrl("file:")/*QUrl("file:/"+QStandardPaths::writableLocation(QStandardPaths::TempLocation))*/);
    }
}

void MainWindow::Add_Library()
{
    SaveLibPosition();
    AddLibrary al(this);
    al.exec();
    int result=al.result();
    QSettings *settings=GetSettings();
    current_list_id=settings->value("current_list_id",0).toLongLong();
    stored_book=settings->value("current_book_id",0).toLongLong();
    UpdateJanre();
    UpdateTags();
    searchCanged(ui->searchString->text());
    setWindowTitle(AppName+(current_lib.name.isEmpty()||current_lib.id<0?"":" - "+current_lib.name));
    FillLibrariesMenu();
    if(result==1) //нажата кнопка экспорт
    {
        QString dirName = QFileDialog::getExistingDirectory(this, tr("Select destination directory"));
        ExportDlg ed(this);
        ed.exec(current_lib.id,dirName);
    }
}
void MainWindow::btnAuthor()
{
    ui->tabWidget->setCurrentIndex(0);
    ui->SearchFrame->setEnabled(true);
    ui->frame_3->setEnabled(true);
    ui->language->setEnabled(true);
    SelectAuthor();
}
void MainWindow::btnJanres()
{
    ui->tabWidget->setCurrentIndex(2);
    ui->SearchFrame->setEnabled(false);
    ui->frame_3->setEnabled(false);
    ui->language->setEnabled(true);
    SelectJanre();
}
void MainWindow::btnSeries()
{
    ui->tabWidget->setCurrentIndex(1);
    ui->SearchFrame->setEnabled(true);
    ui->frame_3->setEnabled(true);
    ui->language->setEnabled(true);
    SelectSeria();
}
void MainWindow::btnPageSearch()
{
    ui->tabWidget->setCurrentIndex(3);
    ui->SearchFrame->setEnabled(false);
    ui->frame_3->setEnabled(false);
    ui->language->setEnabled(false);
    ui->Books->clear();
    ui->find_books->setText("0");
    ExportBookListBtn(false);
}

void MainWindow::DoSearch()
{

}
void MainWindow::About()
{
    AboutDialog* dlg=new AboutDialog(this);
    dlg->exec();
    delete dlg;
}



void MainWindow::searchCanged(QString str)
{
    if(str.length()==0)
    {
        ui->searchString->setText(last_search_symbol);
        ui->searchString->selectAll();
    }
    else
    {
        last_search_symbol=ui->searchString->text().left(1);
        if((ui->searchString->text().left(1)=="*" || ui->searchString->text().left(1)=="#" ) && ui->searchString->text().length()>1)
        {
            ui->searchString->setText(ui->searchString->text().right(ui->searchString->text().length()-1));
        }
        QList<QToolButton *> allButtons = findChildren<QToolButton *>();
        bool find=false;
        foreach(QToolButton *tb,allButtons)
        {
            if(tb->text()==ui->searchString->text().left(1).toUpper())
            {
                find=true;
                tb->setChecked(true);
            }
        }
        if(!find)
            btn_Hash->setChecked(true);
        UpdateAuthor();
        UpdateSeria();
    }
    tbClear->setVisible(ui->searchString->text().length()>1);
}

void MainWindow::searchClear()
{
    ui->searchString->setText(ui->searchString->text().left(1));
    searchCanged(ui->searchString->text());
}

void MainWindow::btnSearch()
{
    QToolButton *button = (QToolButton *)sender();
    ui->searchString->setText(button->text());
    searchCanged(ui->searchString->text());
}

void MainWindow::HelpDlg()
{
    if(Help_dlg==0)
        Help_dlg=new HelpDialog();
    Help_dlg->show();
}

void MainWindow::ContextMenu(QPoint point)
{
    if(QObject::sender()==(QObject*)ui->Books && !ui->Books->itemAt(point))
        return;
    if(QObject::sender()==(QObject*)ui->AuthorList && !ui->AuthorList->itemAt(point))
        return;
    if(QObject::sender()==(QObject*)ui->SeriaList && !ui->SeriaList->itemAt(point))
        return;
    QMenu menu;
    current_list_for_tag=QObject::sender();
    if(QObject::sender()==(QObject*)ui->Books)
    {
        QMenu *save=menu.addMenu(tr("Save as"));
        foreach (QAction* i, ui->btnExport->menu()->actions())
        {
            QAction *action=new QAction(i->text(), this);
            action->setData(i->data().toInt());
            connect(action,SIGNAL(triggered()),this,SLOT(ExportAction()));
            save->addAction(action);
        }
    }
    if(menu.actions().count()>0)
        menu.addSeparator();
    QSettings *settings=GetSettings();
    if(settings->value("use_tag",true).toBool())
        menu.addActions(TagMenu.actions());
    if(menu.actions().count()>0)
        menu.exec(QCursor::pos());
}

void MainWindow::MoveToSeria(qlonglong id,QString FirstLetter)
{
    QTreeWidgetItem* Item=ui->Books->selectedItems()[0];
    qlonglong CurrentID=Item->data(0,Qt::UserRole).toLongLong();
    bool IsBook=(Item->childCount()==0);
    bool IsAuthor=(Item->parent()==0);
    if(IsAuthor)
        return;
    while(Item->parent()->parent())
        Item=Item->parent();
    if(Item->childCount()==0)
        return;
    ui->searchString->setText(id<0?Item->text(0).left(1).toUpper():FirstLetter);
    qlonglong id_seria=id<0?-Item->data(0,Qt::UserRole).toLongLong():-id;
    ui->btnSeries->setChecked(true);
    searchCanged(id<0?Item->text(0).left(1).toUpper():FirstLetter);
    btnSeries();
    ui->SeriaList->clearSelection();
    for (int i=0;i<ui->SeriaList->count();i++)
    {
        if(ui->SeriaList->item(i)->data(Qt::UserRole).toLongLong()==-id_seria)
        {
            ui->SeriaList->item(i)->setSelected(true);
            ui->SeriaList->scrollToItem(ui->SeriaList->item(i));
            if(IsBook)
            {
                CurrentBookID=CurrentID;
            }
            else if(!IsAuthor)
            {
                CurrentSeriaID=-CurrentID;
            }
            SelectSeria();
            return;
        }
    }
}
void MainWindow::MoveToJanre(qlonglong id)
{
    QTreeWidgetItem* Item=ui->Books->selectedItems()[0];
    qlonglong CurrentID=Item->data(0,Qt::UserRole).toLongLong();
    bool IsBook=(Item->childCount()==0);
    bool IsAuthor=(Item->parent()==0);
    while(Item->parent())
        Item=Item->parent();
    qlonglong id_janre=id;
    ui->btnJanre->setChecked(true);
    btnJanres();
    ui->JanreList->clearSelection();
    for (int i=0;i<ui->JanreList->topLevelItemCount();i++)
    {
        for (int j=0;j<ui->JanreList->topLevelItem(i)->childCount();j++)
        {
            if(ui->JanreList->topLevelItem(i)->child(j)->data(0,Qt::UserRole).toLongLong()==id_janre)
            {
                ui->JanreList->topLevelItem(i)->child(j)->setSelected(true);
                ui->JanreList->scrollToItem(ui->JanreList->topLevelItem(i)->child(j));
                if(IsBook)
                {
                    CurrentBookID=CurrentID;
                }
                else if(!IsAuthor)
                {
                    CurrentSeriaID=-CurrentID;
                }
                SelectJanre();
                return;
            }
        }
    }
}

void MainWindow::MoveToAuthor(qlonglong id, QString FirstLetter)
{
    //QTreeWidgetItem* Item=ui->Books->selectedItems()[0];
    QTreeWidgetItem* Item=ui->Books->selectedItems()[0];
    qlonglong CurrentID=Item->data(0,Qt::UserRole).toLongLong();
    bool IsBook=(Item->childCount()==0);
    bool IsAuthor=(Item->parent()==0);
    while(Item->parent())
        Item=Item->parent();
    ui->searchString->setText(id<0?Item->text(0).left(1).toUpper():FirstLetter);
    qlonglong id_author=id<0?Item->data(0,Qt::UserRole).toLongLong():-id;
    ui->btnAuthor->setChecked(true);
    searchCanged(id<0?Item->text(0).left(1).toUpper():FirstLetter);
    btnAuthor();
    ui->AuthorList->clearSelection();
    for (int i=0;i<ui->AuthorList->count();i++)
    {
        if(ui->AuthorList->item(i)->data(Qt::UserRole).toLongLong()==-id_author)
        {
            ui->AuthorList->item(i)->setSelected(true);
            ui->AuthorList->scrollToItem(ui->AuthorList->item(i));
            if(IsBook)
            {
                CurrentBookID=CurrentID;
            }
            else if(!IsAuthor)
            {
                CurrentSeriaID=-CurrentID;
            }
            SelectAuthor();
            break;
        }
    }
}

void MainWindow::proc_path(QString path,QStringList *book_list)
{
#ifdef Q_OS_WIN
    while(path.left(1)=="/")
        path=path.right(path.length()-1);
#endif
    QFileInfo fi(path);
    if(fi.isFile())
    {
        *book_list<<path;
    }
    else if(fi.isDir())
    {
        QDir dir(path);
        QFileInfoList info_list = dir.entryInfoList(QDir::NoSymLinks|QDir::NoDotAndDotDot|QDir::Readable|QDir::Files|QDir::Dirs|QDir::Readable);
        QList<QFileInfo>::iterator iter=info_list.begin();
        for(iter=info_list.begin();iter != info_list.end();iter++)
        {
            proc_path(iter->absoluteFilePath(),book_list);
        }
    }
}

void MainWindow::FillLibrariesMenu()
{
    if(!db_is_open)
        return;
    QMenu *lib_menu=new QMenu(this);
    QSqlQuery query(QSqlDatabase::database("libdb"));
    query.exec("SELECT id,name FROM lib ORDER BY name");
    while(query.next())
    {
        QAction *action=new QAction((query.value(1).toString().trimmed()), this);
        action->setData(query.value(0).toLongLong());
        action->setCheckable(true);
        lib_menu->insertAction(0,action);
        connect(action,SIGNAL(triggered()),this,SLOT(SelectLibrary()));
        action->setChecked(query.value(0).toLongLong()==current_lib.id);
    }
    if(lib_menu->actions().count()>0)
    {
        ui->actionLibraries->setMenu(lib_menu);
        ui->actionLibraries->setEnabled(true);
    }
}

void MainWindow::SelectLibrary()
{
    QAction* action=(QAction*)sender();

    SaveLibPosition();
    QSettings *settings=GetSettings();
    current_list_id=settings->value("current_list_id",0).toLongLong();
    stored_book=settings->value("current_book_id",0).toLongLong();
    settings->setValue("LibID",action->data().toLongLong());
    settings->sync();
    current_lib.id=action->data().toLongLong();
    current_lib.UpdateLib();

    UpdateJanre();
    searchCanged(ui->searchString->text());
    setWindowTitle(AppName+(current_lib.name.isEmpty()||current_lib.id<0?"":" - "+current_lib.name));
    FillLibrariesMenu();
}

void MainWindow::dropEvent(QDropEvent *ev)
{
    if(mode==MODE_LIBRARY)
        dropForm->hide();
    QList<QUrl> urls = ev->mimeData()->urls();
    QStringList book_list;
    foreach(QUrl url, urls)
    {
        proc_path(url.path(),&book_list);
    }
    if(book_list.count())
    {
        ExportDlg dlg(this);
        int id=dropForm->get_command(ev->pos());
        if(id<0)
        {
            dropForm->get_command(QPoint(-1,-1));
            return;
        }
        dlg.exec(book_list,SetCurrentExportSettings(id));
    }
    dropForm->get_command(QPoint(-1,-1));
}

void MainWindow::DeleteDropForm()
{
    if(dropForm!=0)
    {
        if(dropForm->isHidden())
        {
            delete dropForm;
            dropForm=0;
        }
    }
}

void MainWindow::ShowDropForm()
{
    if(dropForm==0)
        dropForm=new DropForm(this);
    if(mode==MODE_CONVERTER)
    {
        dropForm->setFixedWidth(ui->drop_buttons->rect().width());
        dropForm->setFixedHeight(ui->drop_buttons->rect().height());
        dropForm->move(ui->drop_buttons->mapToGlobal(ui->drop_buttons->pos())-this->mapToGlobal(QPoint(0,0)));
    }
    else
    {
        dropForm->setFixedWidth(rect().width()/10*9);
        dropForm->setFixedHeight(rect().height()/10*9);
        dropForm->move(QPoint(rect().width()/20,rect().height()/20));
    }
    QStringList cmd;
    foreach (QAction* action, ui->btnExport->menu()->actions())
    {
        cmd<<action->text();
    }
    dropForm->AddCommand(cmd);
    dropForm->setWindowFlags(Qt::WindowStaysOnTopHint);
    dropForm->show();
    dropForm->activateWindow();
    dropForm->raise();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *ev)
{
    if(ev->mimeData()->urls().count()>0)
    {
        ev->accept();
        if(mode==MODE_LIBRARY)
        {
            DeleteDropForm();
            ShowDropForm();
        }
    }
    else
    {
        ev->setAccepted(false);
        if(mode==MODE_LIBRARY)
            dropForm->hide();
    }
}
void MainWindow::dragMoveEvent(QDragMoveEvent *ev)
{
    dropForm->switch_command(ev->pos());
}

void MainWindow::dragLeaveEvent(QDragLeaveEvent *)
{
    if(mode==MODE_LIBRARY)
        dropForm->hide();
}
void MainWindow::UpdateExportMenu()
{
    QSettings *settings=GetSettings();
    int defaultID=-1;
    if(ui->btnExport->defaultAction())
        defaultID=ui->btnExport->defaultAction()->data().toInt();
    else
        defaultID=settings->value("DefaultExport",-1).toInt();
    QMenu* menu=ui->btnExport->menu();
    if(menu)
    {
        ui->btnExport->menu()->clear();
    }
    else
    {
        menu=new QMenu(this);
        ui->btnExport->setMenu(menu);
    }
    ui->btnExport->setDefaultAction(0);
    int count=settings->beginReadArray("export");
    for(int i=0;i<count;i++)
    {
        settings->setArrayIndex(i);
        QAction *action=new QAction(settings->value("ExportName").toString(),this);
        action->setData(i);
        menu->addAction(action);
        if(settings->value("Default").toBool() || (i==defaultID && !ui->btnExport->defaultAction()))
        {
            ui->btnExport->setDefaultAction(action);
        }
    }
    settings->endArray();
    if(count==0)
    {
       QAction *action=new QAction(tr("Send to ..."),this);
       action->setData(-1);
       menu->addAction(action);
       ui->btnExport->setDefaultAction(action);
    }
    if(menu->actions().count()==0)
    {
        return;
    }
    if(!ui->btnExport->defaultAction())
    {
        ui->btnExport->setDefaultAction(menu->actions()[0]);
    }
    foreach (QAction *action, menu->actions())
    {
        connect(action,SIGNAL(triggered()),this,SLOT(ExportAction()));
    }
    QFont font(ui->btnExport->defaultAction()->font());
    font.setBold(true);
    ui->btnExport->defaultAction()->setFont(font);
    ui->btnExport->setIcon(QIcon(":/icons/img/icons/Streamline.png"));
    ui->btnExport->setEnabled(ui->Books->selectedItems().count()>0);
    //delete settings;
}

void MainWindow::ExportAction()
{
    int id=((QAction*)sender())->data().toInt();
    QSettings *settings=GetSettings();
    int count=settings->beginReadArray("export");
    if(count>1 && ui->btnExport->defaultAction())
    {
        settings->setArrayIndex(ui->btnExport->defaultAction()->data().toInt());
        if(!settings->value("Default").toBool())
        {
            ui->btnExport->setDefaultAction((QAction*)sender());
            QList<QAction*> actions=ui->btnExport->menu()->actions();
            foreach (QAction* action, actions)
            {
                QFont font(action->font());
                font.setBold(action==ui->btnExport->defaultAction());
                action->setFont(font);
            }
            ui->btnExport->setIcon(QIcon(":/icons/img/icons/Streamline.png"));
        }
    }
    settings->endArray();
    SendType type=SetCurrentExportSettings(id);
   if(type==ST_Device)
       SendToDevice();
   else
       SendMail();
}

void MainWindow::on_actionSwitch_to_convert_mode_triggered()
{
    QSettings *settings=GetSettings();
    if(mode==MODE_LIBRARY)
    {
        settings->setValue("MainWnd/geometry", saveGeometry());
        settings->setValue("MainWnd/windowState", saveState());
        settings->setValue("MainWnd/tab/geometry",ui->tabWidget->saveGeometry());
        settings->setValue("MainWnd/tab/geometry",ui->splitter->saveState());
        settings->setValue("MainWnd/books/geometry",ui->splitter_2->saveState());
        settings->setValue("MainWnd/books_head/geometry",ui->Books->header()->saveState());
    }
    ui->stackedWidget->setCurrentWidget(ui->pageConvert);
    ui->actionSwitch_to_library_mode->setVisible(true);
    ui->actionSwitch_to_convert_mode->setVisible(false);

    ui->actionCheck_uncheck->setVisible(false);
    ui->actionLibraries->setVisible(false);
    ui->actionAddLibrary->setVisible(false);
    ui->actionNew_labrary_wizard->setVisible(false);

    setWindowTitle(AppName);
    mode=MODE_CONVERTER;

    setMinimumSize(200,200);
    if(settings->contains("MainWndConvertMode/geometry"))
        restoreGeometry(settings->value("MainWndConvertMode/geometry").toByteArray());

    settings->setValue("ApplicationMode", mode);
    settings->sync();
    if(dropForm!=0)
    {
        dropForm->hide();
        DeleteDropForm();
    }
    ShowDropForm();
}

void MainWindow::on_actionSwitch_to_library_mode_triggered()
{
    QSettings *settings=GetSettings();
    if(mode==MODE_CONVERTER)
    {
        settings->setValue("MainWndConvertMode/geometry", saveGeometry());
    }
    mode=MODE_LIBRARY;
    if(dropForm!=0)
    {
        delete dropForm;
        dropForm=0;
    }
    ui->stackedWidget->setCurrentWidget(ui->pageLabrary);
    ui->actionSwitch_to_library_mode->setVisible(false);
    ui->actionSwitch_to_convert_mode->setVisible(true);

    ui->actionCheck_uncheck->setVisible(true);
    ui->actionLibraries->setVisible(true);
    ui->actionAddLibrary->setVisible(true);
    ui->actionNew_labrary_wizard->setVisible(true);

    setWindowTitle(AppName+(current_lib.name.isEmpty()||current_lib.id<0?"":" - "+current_lib.name));

    setMinimumSize(800,400);
    if(settings->contains("MainWnd/geometry"))
        restoreGeometry(settings->value("MainWnd/geometry").toByteArray());
    if(settings->contains("MainWnd/windowState"))
        restoreState(settings->value("MainWnd/windowState").toByteArray());
    if(settings->contains("MainWnd/tab/geometry"))
        ui->splitter->restoreState(settings->value("MainWnd/tab/geometry").toByteArray());
    on_splitter_splitterMoved(0,0);
    if(settings->contains("MainWnd/books/geometry"))
        ui->splitter_2->restoreState(settings->value("MainWnd/books/geometry").toByteArray());
    settings->setValue("ApplicationMode", mode);
    settings->sync();
}

void MainWindow::on_btnSwitchToLib_clicked()
{
    on_actionSwitch_to_library_mode_triggered();
}

void MainWindow::on_btnPreference_clicked()
{
    Settings();
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
    if(dropForm!=0)
    {
        if(dropForm->isVisible())
        {
            delete dropForm;
            dropForm=0;
            ShowDropForm();
        }
    }
}
void MainWindow::mouseMoveEvent(QMouseEvent *ev)
{
    if(mode==MODE_CONVERTER)
    {
        if(dropForm==0)
        {
            ShowDropForm();
        }
        dropForm->switch_command(ev->pos());
    }
}

void MainWindow::leaveEvent(QEvent *ev)
{
    if(dropForm!=0)
    {
        dropForm->switch_command(QPoint(-1,-1));
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *ev)
{
    if(mode==MODE_CONVERTER)
    {
        if(dropForm==0)
        {
            ShowDropForm();
        }
        int id=dropForm->get_command(ev->pos());
        if(id<0)
            return;
        QFileDialog dialog(this);
        dialog.setFileMode(QFileDialog::ExistingFiles);
        dialog.setNameFilter(tr("Books")+" (*.fb2 *.epub *.zip)");
        dialog.setWindowTitle(tr("Book`s files"));
        if (dialog.exec())
        {
            if(dialog.selectedFiles().isEmpty())
                return;
            QStringList book_list;
            foreach (QString file, dialog.selectedFiles())
            {
                proc_path(file,&book_list);
            }
            ExportDlg dlg(this);
            dlg.exec(book_list,SetCurrentExportSettings(id));
        }
    }
}


void MainWindow::on_splitter_splitterMoved(int , int )
{
//    if(ui->Review->page()->mainFrame()->toPlainText().isEmpty())
//    {
//        SelectBook();
//    }
}

void MainWindow::ChangingTrayIcon(int index,int color)
{
    if(CMDparser.isSet("tray"))
        index=2;
    QSettings *settings=GetSettings();
    if(index<0)
    {
        index=settings->value("tray_icon",0).toInt();
    }
    if(color<0)
    {
        color=settings->value("tray_color",0).toInt();
    }
    if(index==0)
    {
        if(trIcon)
        {
            trIcon->hide();
            trIcon->deleteLater();
        }
        trIcon=0;
    }
    else
    {
        if(!trIcon)
        {
            trIcon = new QSystemTrayIcon(this);  //инициализируем объект
            connect(trIcon,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),this,SLOT(TrayMenuAction(QSystemTrayIcon::ActivationReason)));
        }
        QIcon icon(QString(":/img/tray%1.png").arg(QString::number(color)));
        trIcon->setIcon(icon);//.arg(app->devicePixelRatio()>=2?"@2x":"")));  //устанавливаем иконку
        trIcon->show();
    }
}

void MainWindow::TrayMenuAction(QSystemTrayIcon::ActivationReason reson)
{
    if(reson!=QSystemTrayIcon::Trigger && reson!=QSystemTrayIcon::Unknown)
        return;
    QSettings *settings=GetSettings();
#ifdef Q_OS_WIN
    if(this->isVisible())
    {
        this->setWindowState(this->windowState()|Qt::WindowMinimized);
        if(settings->value("tray_icon",0).toInt()!=0)
            this->hide();
    }
    else
    {
        this->show();
        this->setWindowState(this->windowState() & ~Qt::WindowMinimized);
        this->raise();
        this->activateWindow();
        this->setFocus(Qt::ActiveWindowFocusReason);
    }
#else
    #ifdef Q_OS_OSX
        if(reson==QSystemTrayIcon::Unknown)
            return;
        if(this->isActiveWindow() && this->isVisible())
        {
            this->setWindowState(this->windowState()|Qt::WindowMinimized);
            if(settings->value("tray_icon",0).toInt()!=0)
                this->hide();
        }
        else
        {
            this->show();
            this->setWindowState(this->windowState() & ~Qt::WindowMinimized);
            this->activateWindow();
            this->raise();
            this->setFocus(Qt::ActiveWindowFocusReason);
        }
    #else
        if(this->isActiveWindow() && this->isVisible())
        {
            this->setWindowState(this->windowState()|Qt::WindowMinimized);
            if(settings->value("tray_icon",0).toInt()!=0)
                this->hide();
        }
        else
        {
            this->show();
            this->setWindowState(this->windowState() & ~Qt::WindowMinimized);
            this->raise();
            this->activateWindow();
            this->setFocus(Qt::ActiveWindowFocusReason);
        }
    #endif
#endif
}

void MainWindow::dockClicked()
{
    //qDebug()<<"dock";
}

void MainWindow::MinimizeWindow()
{
    this->setWindowState(this->windowState()|Qt::WindowMinimized);
}

void MainWindow::changeEvent(QEvent *event)
{
    if(event->type() == QEvent::WindowStateChange)
    {
        if(isMinimized())
        {
            ChangingTrayIcon();
            TrayMenuAction(QSystemTrayIcon::Unknown);
            event->ignore();
        }
    }
}
