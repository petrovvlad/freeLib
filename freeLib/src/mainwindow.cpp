
#include <QtSql>
#include <QDomDocument>
#include <QBuffer>
#include <QByteArray>
#include <QSqlDriver>
#include <QSystemTrayIcon>
#include <QCommandLineParser>
#include <QSplashScreen>
#include <QTextCodec>
#include <QMap>
#include <QButtonGroup>


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
#include "treebookitem.h"
#include "genresortfilterproxymodel.h"

extern QSplashScreen *splash;
extern QMap<int,SLib> mLibs;
extern QMap <uint,SGenre> mGenre;

extern int idCurrentLib;
bool db_is_open;


QString ToIndex(QString str)
{
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
    QString file,archive;
    QFileInfo fi;
    SBook &book = mLibs[idCurrentLib].mBooks[id_book];
    QString LibPath=mLibs[idCurrentLib].path;
    LibPath=RelativeToAbsolutePath(LibPath);
    if(book.sArchive.isEmpty()){
        file = QString("%1/%2.%3").arg(LibPath).arg(book.nFile).arg(book.sFormat);
    }else{
        file = QString("%1.%2").arg(book.nFile).arg(book.sFormat);
        archive = QString("%1/%2").arg(LibPath).arg(book.sArchive.replace(".inp",".zip"));
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

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    trIcon=0;
    pDropForm=nullptr;
    error_quit=false;
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

    setWindowTitle(AppName+(idCurrentLib<0||mLibs[idCurrentLib].name.isEmpty()?"":" - "+mLibs[idCurrentLib].name));

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

    connect(ui->searchString,SIGNAL(/*textEdited*/textChanged(QString)),this,SLOT(searchCanged(QString)));
    connect(tbClear,SIGNAL(clicked()),this,SLOT(searchClear()));
    connect(ui->actionAddLibrary,SIGNAL(triggered()),this,SLOT(ManageLibrary()));
    connect(ui->btnLibrary,SIGNAL(clicked()),this,SLOT(ManageLibrary()));
    connect(ui->btnOpenBook,SIGNAL(clicked()),this,SLOT(BookDblClick()));
    connect(ui->btnOption,SIGNAL(clicked()),this,SLOT(Settings()));
    connect(ui->actionPreference,SIGNAL(triggered()),this,SLOT(Settings()));
    connect(ui->actionCheck_uncheck,SIGNAL(triggered()),this,SLOT(CheckBooks()));
    connect(ui->btnCheck,SIGNAL(clicked()),this,SLOT(CheckBooks()));
    connect(ui->btnEdit,SIGNAL(clicked()),this,SLOT(EditBooks()));
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
    connect(ui->Books,SIGNAL(itemChanged(QTreeWidgetItem*,int)),this,SLOT(itemChanged(QTreeWidgetItem*,int)));


    ChangingLanguage(false);
    ExportBookListBtn(false);

    idCurrentLanguage_ = -1;
    bUseTag_=settings->value("use_tag",true).toBool();
    bShowDeleted_ =settings->value("ShowDeleted").toBool();
    int nCurrentTab;

    if(settings->value("store_position",true).toBool())
    {
        idCurrentAuthor_= settings->value("current_author_id",0).toUInt();
        idCurrentSerial_ = settings->value("current_serial_id",0).toUInt();
        idCurrentBook_ = settings->value("current_book_id",0).toLongLong();
        idCurrentGenre_ = settings->value("current_genre_id",0).toLongLong();
        nCurrentTab = settings->value("current_tab",0).toInt();
        ui->searchString->setText(settings->value("filter_set").toString());

    }
    else
    {
        idCurrentAuthor_ = 0;
        idCurrentSerial_ = 0;
        idCurrentBook_ = 0;
        idCurrentGenre_ = -1;
        nCurrentTab = 0;
//        FillAuthors();
//        FillSerials();
//        FillGenres();
//        FirstButton->click();
    }
    UpdateTags();
    UpdateSeria();
    UpdateAuthor();
    UpdateJanre();
    UpdateBooks();

    FillAuthors();
    FillSerials();
    FillGenres();
    switch(nCurrentTab)
    {
    case 0:
        FillListBooks();
        break;
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

    if(ui->searchString->text().trimmed().isEmpty())
        FirstButton->click();

    ui->date_to->setDate(QDate::currentDate());

    pHelpDlg=0;
    connect(ui->actionHelp,SIGNAL(triggered()),this,SLOT(HelpDlg()));
    ui->Books->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->Books,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(ContextMenu(QPoint)));
    ui->AuthorList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->AuthorList,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(ContextMenu(QPoint)));
    ui->SeriaList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->SeriaList,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(ContextMenu(QPoint)));
    connect(ui->TagFilter,SIGNAL(currentIndexChanged(int)),this,SLOT(tag_select(int)));

    opds.server_run();
    FillLibrariesMenu();
    UpdateExportMenu();

    setMouseTracking(true);
    centralWidget()->setMouseTracking(true);
    ui->label_drop->setMouseTracking(true);
    ui->frame_drop->setMouseTracking(true);
    ui->pageConvert->setMouseTracking(true);
    ui->stackedWidget->setMouseTracking(true);

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
    //ui->stacked_books->setCurrentWidget(ui->page_books);
    ChangingTrayIcon();

#ifdef Q_OS_OSX
    connect(MyPrivate::instance(), SIGNAL(dockClicked()), SLOT(dockClicked()));
#endif
    connect(ui->actionMinimize_window,SIGNAL(triggered(bool)),SLOT(MinimizeWindow()));

    GenreSortFilterProxyModel *MyProxySortModel = new GenreSortFilterProxyModel(ui->s_janre);
    MyProxySortModel->setSourceModel(ui->s_janre->model());
    ui->s_janre->model()->setParent(MyProxySortModel);
    ui->s_janre->setModel(MyProxySortModel);
    ui->s_janre->model()->sort(0);
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
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    QButtonGroup *group=new QButtonGroup(this);
    group->setExclusive(true);
//    disconnect(ui->TagFilter,SIGNAL(currentIndexChanged(int)),this,SLOT(tag_select(int)));
    const bool wasBlocked = ui->TagFilter->blockSignals(true);

    int size =ui->TagFilter->style()->pixelMetric(QStyle::PM_SmallIconSize)*app->devicePixelRatio();
    QSqlQuery query(QSqlDatabase::database("libdb"));
    query.exec("SELECT color,name,id from favorite");
    ui->TagFilter->clear();
    int con=1;
    ui->TagFilter->addItem("*",0);
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
    ui->TagFilter->setVisible(bUseTag_);
    ui->tag_label->setVisible(bUseTag_);

    while(query.next())
    {
        ui->TagFilter->addItem(query.value(1).toString().trimmed(),query.value(2).toInt());
        if(settings->value("current_tag").toInt()==ui->TagFilter->count()-1 && bUseTag_)
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
//    connect(ui->TagFilter,SIGNAL(currentIndexChanged(int)),this,SLOT(tag_select(int)));
    ui->TagFilter->blockSignals(wasBlocked);

    QApplication::restoreOverrideCursor();
}

MainWindow::~MainWindow()
{
    delete ui;
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
        SLib lib;
        lib.name = wiz.name;
        lib.path = wiz.dir;
        lib.sInpx = wiz.inpx;
        lib.bWoDeleted = false;
        lib.bFirstAuthor = false;
        al.AddNewLibrary(lib);
        UpdateJanre();
        UpdateTags();
        UpdateSeria();
        UpdateAuthor();
        UpdateBooks();
        FillAuthors();
        FillSerials();
        FillGenres();
        searchCanged(ui->searchString->text());
        setWindowTitle(AppName+(idCurrentLib<0||mLibs[idCurrentLib].name.isEmpty()?"":" - "+mLibs[idCurrentLib].name));
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
            if(ui->Books->topLevelItem(i)->data(0,Qt::UserRole).toLongLong()==id)
                ui->Books->topLevelItem(i)->setIcon(0,GetTag(tag_id));
        }
        else
        {
            for(int j=0;j<ui->Books->topLevelItem(i)->childCount();j++)
            {
                if(ui->Books->topLevelItem(i)->child(j)->data(0,Qt::UserRole).toLongLong()==id)
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
    int tag_id=((QAction*)QObject::sender())->data().toInt();
    uint id;
    QSqlQuery query(QSqlDatabase::database("libdb"));

    if(current_list_for_tag==(QObject*)ui->Books)
    {
        QTreeWidgetItem* item=ui->Books->selectedItems()[0];
        id=item->data(0,Qt::UserRole).toUInt();
        switch (item->type()) {
        case ITEM_TYPE_BOOK:
            item->setIcon(0,GetTag(tag_id));
            query.prepare("UPDATE book set favorite=:favorite where id=:id");
            query.bindValue(":favorite",tag_id);
            query.bindValue(":id",id);
            query.exec();
            mLibs[idCurrentLib].mBooks[id].nTag = tag_id;
            break;

        case ITEM_TYPE_SERIA:
            update_list_pix(id,2,tag_id);
            query.prepare("UPDATE seria set favorite=:favorite where id=:id");
            query.bindValue(":favorite",tag_id);
            query.bindValue(":id",id);
            query.exec();
            mLibs[idCurrentLib].mSerials[id].nTag = tag_id;
            break;

        case ITEM_TYPE_AUTHOR:
            update_list_pix(id,1,tag_id);
            query.prepare("UPDATE author set favorite=:favorite where id=:id");
            query.bindValue(":favorite",tag_id);
            query.bindValue(":id",id);
            query.exec();
            mLibs[idCurrentLib].mAuthors[id].nTag = tag_id;
            break;

        default:
            break;
        }
    }
    else if(current_list_for_tag==(QObject*)ui->AuthorList)
    {
        id=ui->AuthorList->selectedItems()[0]->data(Qt::UserRole).toUInt();
        update_list_pix(id,1,tag_id);
        query.prepare("UPDATE author set favorite=:favorite where id=:id");
        query.bindValue(":favorite",tag_id);
        query.bindValue(":id",id);
        query.exec();
        mLibs[idCurrentLib].mAuthors[id].nTag = tag_id;
    }
    else if(current_list_for_tag==(QObject*)ui->SeriaList)
    {
        id=ui->SeriaList->selectedItems()[0]->data(Qt::UserRole).toUInt();
        update_list_pix(id,2 ,tag_id);
        query.prepare("UPDATE seria set favorite=:favorite where id=:id");
        query.bindValue(":favorite",tag_id);
        query.bindValue(":id",id);
        query.exec();
        mLibs[idCurrentLib].mSerials[id].nTag = tag_id;
    }
}

void MainWindow::tag_select(int index)
{
    QSettings *settings=GetSettings();
    if(ui->TagFilter->itemData(ui->TagFilter->currentIndex()).toInt()==-1)
    {
        //disconnect(ui->TagFilter,SIGNAL(currentIndexChanged(int)),this,SLOT(tag_select(int)));
        const bool wasBlocked = ui->TagFilter->blockSignals(true);
        ui->TagFilter->setCurrentIndex(settings->value("current_tag",0).toInt());
        ui->TagFilter->blockSignals(wasBlocked);
        //connect(ui->TagFilter,SIGNAL(currentIndexChanged(int)),this,SLOT(tag_select(int)));
        TagDialog td(this);
        if(td.exec())
            UpdateTags();
    }
    else if(index>=0)
    {
        settings->setValue("current_tag",index);
        FillListBooks();
        FillAuthors();
        FillSerials();
        FillGenres();
    }
}

void MainWindow::SaveLibPosition()
{
    QSettings *settings=GetSettings();
    settings->setValue("filter_set",ui->searchString->text());
    settings->setValue("current_tab",ui->tabWidget->currentIndex());
//    qlonglong id=-1;
//    switch (ui->tabWidget->currentIndex())
//    {
//    case 0:
//        if(ui->AuthorList->selectedItems().count()>0)
//        {
//            id=ui->AuthorList->selectedItems()[0]->data(Qt::UserRole).toLongLong();
//        }
//        break;
//    case 1:
//        if(ui->SeriaList->selectedItems().count()>0)
//        {
//            id=ui->SeriaList->selectedItems()[0]->data(Qt::UserRole).toLongLong();
//        }
//        break;
//    case 2:
//        if(ui->JanreList->selectedItems().count()>0)
//        {
//            id=ui->JanreList->selectedItems()[0]->data(0,Qt::UserRole).toLongLong();
//        }
//        break;
//    }
    //settings->setValue("current_list_id",id);
    //qlonglong book_id=-1;
//    if(ui->Books->selectedItems().count()>0)
//    {
//        book_id=ui->Books->selectedItems()[0]->data(0,Qt::UserRole).toLongLong();
//    }
    settings->setValue("current_book_id",idCurrentBook_);
    //current_list_id=id;
    //stored_book=book_id;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if(pHelpDlg!=0)
        delete pHelpDlg;
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
    //bool ShowDeleted=settings->value("ShowDeleted").toBool();
    //bool use_tag=settings->value("use_tag").toBool();
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
    settings->setValue("LibID",idCurrentLib);
    settings->sync();
    if(bShowDeleted_!=settings->value("ShowDeleted").toBool() || bUseTag_!=settings->value("use_tag").toBool())
    {
        bUseTag_ = settings->value("use_tag").toBool();
        bShowDeleted_ = settings->value("ShowDeleted").toBool();
        UpdateTags();
        SaveLibPosition();
        UpdateSeria();
        FillAuthors();
        FillGenres();
        FillListBooks();
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
    // проверит цикл
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

void MainWindow::CheckBooks()
{
    QList<book_info> book_list;
    FillCheckedBookList(book_list,0,false,true,true);

    const QSignalBlocker blocker( ui->Books);
    Qt::CheckState cs=book_list.count()>0?Qt::Unchecked:Qt::Checked;
    for(int i=0;i<ui->Books->topLevelItemCount();i++)
    {
        ui->Books->topLevelItem(i)->setCheckState(0,cs);
        CheckChild(ui->Books->topLevelItem(i));
    }
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
    //disconnect(ui->Books,SIGNAL(itemChanged(QTreeWidgetItem*,int)),this,SLOT(itemChanged(QTreeWidgetItem*,int)));
    const bool wasBlocked = ui->Books->blockSignals(true);
    CheckChild(item);
    QTreeWidgetItem* parent=item->parent();
    if(parent)
        CheckParent(parent);
    QList<book_info> book_list;
    FillCheckedBookList(book_list,0,false,true);
    ExportBookListBtn(book_list.count()!=0);

    ui->Books->blockSignals(wasBlocked);
    //connect(ui->Books,SIGNAL(itemChanged(QTreeWidgetItem*,int)),this,SLOT(itemChanged(QTreeWidgetItem*,int)));
}

//void MainWindow::FillBookList(QSqlQuery &query)
//{
//    disconnect(ui->Books,SIGNAL(itemChanged(QTreeWidgetItem*,int)),this,SLOT(itemChanged(QTreeWidgetItem*,int)));
//    QTreeWidgetItem* item;
//    QTreeWidgetItem* item_list;
//    QTreeWidgetItem* item_author=0;
//    QTreeWidgetItem* item_seria=0;
//    QFont bold_font(ui->Books->font());
//    bold_font.setBold(true);
//    qlonglong id_book=0;
//    qlonglong count=0;
//    QTreeWidgetItem* ScrollItem=0;
//    if(stored_book>0)
//    {
//        CurrentBookID=stored_book;
//        if(!(ui->tabWidget->currentIndex()==0 && current_list_for_tag!=(QObject*)ui->AuthorList))
//            stored_book=-1;
//    }
//    QSettings *settings=GetSettings();
//    bool use_tag=settings->value("use_tag",true).toBool();

//    QList<qlonglong> books_list;
//    while (query.next())
//    {
//        if(books_list.contains(query.value(0).toLongLong()))
//            continue;
//        books_list<<query.value(0).toLongLong();
//        if(id_book==query.value(0).toLongLong())
//        {
//            item->setText(5,item->text(5)+", "+query.value(8).toString().trimmed());
//            continue;
//        }
//        //item_list=new QTreeWidgetItem(ui->BooksList);
//        id_book=query.value(0).toLongLong();
//        if(item_author==0 || (item_author==0?true:item_author->text(0)!=query.value(2).toString().trimmed()))
//        {
//            item_author=new QTreeWidgetItem(ui->Books);
//            ui->Books->addTopLevelItem(item_author);
//            item_author->setText(0,query.value(2).toString().trimmed());
//            item_author->setExpanded(true);
//            item_author->setFont(0,bold_font);
//            item_author->setCheckState(0,Qt::Unchecked);
//            item_author->setData(0,Qt::UserRole,-query.value(10).toLongLong());
//            if(use_tag)
//                item_author->setIcon(0,GetTag(query.value(12).toInt()));
//            item_seria=0;
//        }
//        item_list->setText(0,query.value(2).toString().trimmed());
//        item_list->setCheckState(0,Qt::Unchecked);
//        item_list->setData(0,Qt::UserRole,-query.value(10).toLongLong());
//        if(use_tag)
//            item_list->setIcon(0,GetTag(query.value(12).toInt()));

//        if(query.value(3).toString().trimmed().isEmpty())
//        {
//            item_seria=0;
//        }
//        else
//        {
//            if(item_seria==0 || (item_seria==0?true:item_seria->text(0)!=query.value(3).toString().trimmed()))
//            {
//                item_seria=new QTreeWidgetItem(item_author);
//                item_seria->setText(0,query.value(3).toString().trimmed());
//                item_author->addChild(item_seria);
//                item_seria->setExpanded(true);
//                item_seria->setFont(0,bold_font);
//                item_seria->setCheckState(0,Qt::Unchecked);
//                item_seria->setData(0,Qt::UserRole,-query.value(11).toLongLong());
//                if(use_tag)
//                    item_seria->setIcon(0,GetTag(query.value(13).toInt()));
//                if(query.value(11).toLongLong()==CurrentSeriaID)
//                {
//                    ScrollItem=item_seria;
//                }
//           }
//        }
//        item=new QTreeWidgetItem(item_seria==0?item_author:item_seria);
//        item->setData(0,Qt::UserRole,query.value(0));
//        item->setText(0,query.value(1).toString().trimmed());
//        item->setText(1,(query.value(4).toString().trimmed()=="0"?"":query.value(4).toString().trimmed()));
//        item->setTextAlignment(1, Qt::AlignRight);
//        item->setText(2,(query.value(6).toString().trimmed()=="0"?"":query.value(6).toString().trimmed()));
//        item->setTextAlignment(2, Qt::AlignRight);
//        if(use_tag)
//            item->setIcon(0,GetTag(query.value(14).toInt()));
//        if(query.value(0).toLongLong()==CurrentBookID)
//        {
//            ScrollItem=item;
//        }

//        if(query.value(9).toBool())
//        {
//            item->setTextColor(0,QColor::fromRgb(96,96,96));
//            item->setTextColor(1,QColor::fromRgb(96,96,96));
//            item->setTextColor(2,QColor::fromRgb(96,96,96));
//            item->setTextColor(3,QColor::fromRgb(96,96,96));
//            item->setTextColor(4,QColor::fromRgb(96,96,96));
//            item->setTextColor(5,QColor::fromRgb(96,96,96));
//        }

//        QPixmap pix(":/icons/img/icons/stars/"+query.value(5).toString().trimmed()+QString("star%1.png").arg(app->devicePixelRatio()>=2?"@2x":""));
//        pix.setDevicePixelRatio(app->devicePixelRatio());
//        item->setData(3,Qt::DecorationRole,pix);

//        item->setTextAlignment(3,Qt::AlignHCenter);
//        item->setText(4,query.value(7).toDate().toString("dd.MM.yyyy"));
//        item->setTextAlignment(4, Qt::AlignCenter);
//        item->setText(5,query.value(8).toString().trimmed());
//        item->setCheckState(0,Qt::Unchecked);
//        (item_seria==0?item_author:item_seria)->addChild(item);

//        count++;
//        if(count==ui->maxBooks->value() && ui->tabWidget->currentWidget()==ui->page)
//            break;
//    }
//    ui->Books->scrollToTop();
//    ui->find_books->setText((query.next()?">":"")+QString::number(count));
//    CurrentBookID=0;
//    CurrentSeriaID=0;
//    if(ScrollItem)
//    {
//        ScrollItem->setSelected(true);
//        ui->Books->scrollToItem(ScrollItem);
//    }
//    connect(ui->Books,SIGNAL(itemChanged(QTreeWidgetItem*,int)),this,SLOT(itemChanged(QTreeWidgetItem*,int)));
//}

void MainWindow::ExportBookListBtn(bool Enable)
{
    ui->btnExport->setEnabled(Enable);
    ui->btnOpenBook->setEnabled(false);
}


void MainWindow::StartSearch()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    app->processEvents();
    ui->Books->clear();
    ExportBookListBtn(false);
    //QSqlQuery query(QSqlDatabase::database("libdb"));
    //QSettings *settings=GetSettings();
    //bool bShowDeleted=settings->value("ShowDeleted").toBool();
    //QStringList str_author=ui->s_author->text().split(' ');
    QString sName = ui->s_name->text().trimmed();
    QString sAuthor = ui->s_author->text().trimmed();
    QDate dateFrom = ui->date_from->date();
    QDate dateTo = ui->date_to->date();
    int nMaxCount = ui->maxBooks->value();
    int idGenre = ui->s_janre->currentData().toInt();

    QList<uint> listBooks;
    int nCount = 0;
    auto iBook = mLibs[idCurrentLib].mBooks.constBegin();
    while(iBook != mLibs[idCurrentLib].mBooks.constEnd()){
        if((bShowDeleted_ || !iBook->bDeleted)&&
                iBook->date>= dateFrom && iBook->date <= dateTo &&
                (sAuthor.isEmpty() || mLibs[idCurrentLib].mAuthors[iBook->idFirstAuthor].sName.contains(sAuthor,Qt::CaseInsensitive)) &&
                (sName.isEmpty() || iBook->sName.contains(sName,Qt::CaseInsensitive)))
        {
            if(idGenre==0){
                nCount++;
                listBooks << iBook.key();
            }else
            {
                foreach (int id,iBook->listIdGenres) {
                   if(id==idGenre){
                       nCount++;
                       listBooks << iBook.key();
                       break;
                   }
                }
            }
        }
        ++iBook;
        if(nCount==nMaxCount)
            break;
    }
    ui->find_books->setText(QString::number(nCount));
    FillListBooks(listBooks,0);

    QApplication::restoreOverrideCursor();
}

void MainWindow::SelectLibrary()
{
    QAction* action=(QAction*)sender();

    SaveLibPosition();
    QSettings *settings=GetSettings();
    //current_list_id=settings->value("current_list_id",0).toLongLong();
    //stored_book=settings->value("current_book_id",0).toLongLong();
    settings->setValue("LibID",action->data().toLongLong());
    settings->sync();
    idCurrentLib=action->data().toLongLong();
    //current_lib.UpdateLib();

    //UpdateJanre();
    UpdateAuthor();
    UpdateBooks();
    UpdateSeria();
    FillAuthors();
    FillSerials();
    FillGenres();
    searchCanged(ui->searchString->text());
    setWindowTitle(AppName+(idCurrentLib<0||mLibs[idCurrentLib].name.isEmpty()?"":" - "+mLibs[idCurrentLib].name));
    FillLibrariesMenu();
}

void MainWindow::SelectJanre()
{
    ui->Books->clear();
    ExportBookListBtn(false);
    if(ui->JanreList->selectedItems().count()==0)
        return;
    QTreeWidgetItem* cur_item=ui->JanreList->selectedItems()[0];
    uint idGenre = cur_item->data(0,Qt::UserRole).toUInt();
    QList<uint> listBooks;
    auto iBook = mLibs[idCurrentLib].mBooks.constBegin();
    while(iBook != mLibs[idCurrentLib].mBooks.constEnd()){
        if((idCurrentLanguage_==-1 || idCurrentLanguage_ == iBook->idLanguage)){
            foreach (uint iGenre, iBook->listIdGenres) {
                if(iGenre == idGenre){
                    listBooks << iBook.key();
                    break;
                }
            }
        }
        ++iBook;
    }
    idCurrentGenre_ = idGenre;
    FillListBooks(listBooks,0);
    QSettings *settings=GetSettings();
    if(settings->value("store_position",true).toBool()){
        settings->setValue("current_genre_id",idCurrentGenre_);
    }
}

void MainWindow::SelectSeria()
{
    ui->Books->clear();
    ExportBookListBtn(false);
    if(ui->SeriaList->selectedItems().count()==0)
        return;
    QListWidgetItem* cur_item=ui->SeriaList->selectedItems()[0];
    uint idSerial = cur_item->data(Qt::UserRole).toUInt();
    QList<uint> listBooks;
    auto iBook = mLibs[idCurrentLib].mBooks.constBegin();
    while(iBook != mLibs[idCurrentLib].mBooks.constEnd()){
        if(iBook->idSerial == idSerial && (idCurrentLanguage_==-1 || idCurrentLanguage_ == iBook->idLanguage)){
            listBooks << iBook.key();
        }
        ++iBook;
    }
    FillListBooks(listBooks,0);

    QSettings *settings=GetSettings();
    idCurrentSerial_= idSerial;
    if(settings->value("store_position",true).toBool()){
        settings->setValue("current_serial_id",idSerial);
    }
}

void MainWindow::SelectAuthor()
{
    ExportBookListBtn(false);
    if(ui->AuthorList->selectedItems().count()==0)
        return;
    QListWidgetItem* cur_item=ui->AuthorList->selectedItems()[0];

    QSettings *settings=GetSettings();

    idCurrentAuthor_ = cur_item->data(Qt::UserRole).toUInt();

    QList<uint> booksId = mLibs[idCurrentLib].mAuthorBooksLink.values(idCurrentAuthor_);
    FillListBooks(booksId,idCurrentAuthor_);
    if(settings->value("store_position",true).toBool()){
        settings->setValue("current_author_id",idCurrentAuthor_);
    }
}

void MainWindow::SelectBook()
{
    if(ui->Books->selectedItems().count()==0)
    {
        ExportBookListBtn(false);
        ui->Review->setHtml("");
        return;
    }
    QSettings *settings=GetSettings();
    ExportBookListBtn(true);
    QTreeWidgetItem* item=ui->Books->selectedItems()[0];
    if(item->type() != ITEM_TYPE_BOOK)
    {
        ui->btnOpenBook->setEnabled(false);
        ui->Review->setHtml("");
        return;
    }
    uint idBook = item->data(0,Qt::UserRole).toUInt();
    idCurrentBook_ = idBook;
    SBook &book = mLibs[idCurrentLib].mBooks[idBook];
    ui->btnOpenBook->setEnabled(true);
    //if(ui->splitter->sizes()[1]>0)
    //{
        QBuffer outbuff;
        QBuffer infobuff;
        QDateTime book_date;
        QFileInfo fi=GetBookFile(outbuff,infobuff,idBook,false,&book_date);
        book_info bi;
        if(fi.fileName().isEmpty())
        {
            GetBookInfo(bi,QByteArray(),"",true,idBook);
            QString file;
            QString LibPath=mLibs[idCurrentLib].path;
            if(book.sArchive.trimmed().isEmpty() )
            {
                file=QString("%1/%2.%3").arg(LibPath).arg(book.nFile).arg(book.sFormat);
            }
            else
            {
                file=LibPath+"/"+book.sArchive.trimmed().replace(".inp",".zip");
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
        //if(parent->parent()) //если это серия
        if(parent->type() == ITEM_TYPE_SERIA)
        {
            seria=QString("<a href=seria_%3%1>%2</a>").arg(QString::number(/*-*/parent->data(0,Qt::UserRole).toLongLong()),parent->text(0),parent->text(0).left(1).toUpper());
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
    //}
}

void MainWindow::UpdateAuthor()
{
    if(!db_is_open)
        return;

    qint64 t_start = QDateTime::currentMSecsSinceEpoch();

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    QSqlQuery query(QSqlDatabase::database("libdb"));
    query.prepare("SELECT author.id, name1||' '||name2||' '||name3, author.favorite FROM author WHERE id_lib=:id_lib;");
    query.bindValue(":id_lib",idCurrentLib);
    query.exec();
    while (query.next()) {
        quint64 idAuthor = query.value(0).toULongLong();
        QString sName = query.value(1).toString().trimmed();
        if(sName.isEmpty())
            sName = tr("unknown author");
        int nTag = query.value(2).toInt();
        mLibs[idCurrentLib].mAuthors[idAuthor].sName = sName;
        mLibs[idCurrentLib].mAuthors[idAuthor].nTag = nTag;
    }
    qint64 t_end = QDateTime::currentMSecsSinceEpoch();
        qDebug()<< "UpdateAuthor " << t_end-t_start << "msec";
    QApplication::restoreOverrideCursor();
}


void MainWindow::UpdateBooks()
{

    qint64 t_start = QDateTime::currentMSecsSinceEpoch();
    qint64 t_map = 0;
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    mLibs[idCurrentLib].mBooks.clear();
    QSqlQuery query(QSqlDatabase::database("libdb"));
    query.setForwardOnly(true);
    query.prepare("SELECT id, name, star, id_seria, num_in_seria, language, file, size, deleted, date, format, id_inlib, archive, first_author_id, favorite FROM book WHERE id_lib=:id_lib;");
    //                     0  1     2     3         4             5         6     7     8        9     10      11        12       13               14
    query.bindValue(":id_lib",idCurrentLib);
    if(!query.exec())
        qDebug() << query.lastError().text();
    while (query.next()) {
        QString sName = query.value(1).toString();
        if(sName.isEmpty())
            continue;
        uint id = query.value(0).toUInt();
        quint64 t_map_start =  QDateTime::currentMSecsSinceEpoch();
        SBook &book = mLibs[idCurrentLib].mBooks[id];
        book.sName = sName;
        book.nStars = query.value(2).toUInt();
        book.idSerial = query.value(3).toUInt();
        book.numInSerial = query.value(4).toUInt();
        QString sLaguage = query.value(5).toString().toLower();
        int idLaguage = mLibs[idCurrentLib].vLaguages.indexOf(sLaguage);
        if(idLaguage<0){
            idLaguage = mLibs[idCurrentLib].vLaguages.count();
            mLibs[idCurrentLib].vLaguages << sLaguage;
        }
        book.idLanguage = idLaguage;
        book.nFile = query.value(6).toUInt();
        book.nSize = query.value(7).toUInt();
        book.bDeleted = query.value(8).toBool();
        book.date = query.value(9).toDate();
        book.sFormat = query.value(10).toString();
        book.idInLib = query.value(11).toUInt();
        book.sArchive = query.value(12).toString();
        book.idFirstAuthor = query.value(13).toUInt();
        book.nTag = query.value(14).toInt();
        t_map += QDateTime::currentMSecsSinceEpoch()-t_map_start;
    }
    qDebug() << "mBooks insert " << t_map << "msec";
    qDebug()<< "UpdateBooks1 " << QDateTime::currentMSecsSinceEpoch()-t_start << "msec";

    mLibs[idCurrentLib].mAuthorBooksLink.clear();
    query.prepare("SELECT id_book, id_author FROM book_author WHERE id_lib=:id_lib;");
    //                     0       1
    query.bindValue(":id_lib",idCurrentLib);
    if(!query.exec())
        qDebug() << query.lastError().text();
    while (query.next()) {
        uint idBook = query.value(0).toUInt();
        uint idAuthor = query.value(1).toUInt();
        if(mLibs[idCurrentLib].mBooks.contains(idBook) && mLibs[idCurrentLib].mAuthors.contains(idAuthor)){
            mLibs[idCurrentLib].mAuthorBooksLink.insert(idAuthor,idBook);
            mLibs[idCurrentLib].mBooks[idBook].listIdAuthors << idAuthor;
        }
    }

    query.prepare("SELECT id_book, id_janre FROM book_janre WHERE id_lib=:id_lib;");
    //                     0       1
    query.bindValue(":id_lib",idCurrentLib);
    if(!query.exec())
        qDebug() << query.lastError().text();
    while (query.next()) {
        uint idBook = query.value(0).toUInt();
        uint idGenre = query.value(1).toUInt();
        if(idGenre==0) idGenre = 1112; // Прочие/Неотсортированное
        if(mLibs[idCurrentLib].mBooks.contains(idBook))
            mLibs[idCurrentLib].mBooks[idBook].listIdGenres << idGenre;
    }

    qint64 t_end = QDateTime::currentMSecsSinceEpoch();
    qDebug()<< "UpdateBooks " << t_end-t_start << "msec";

    ui->language->blockSignals(true);
    ui->findLanguage->blockSignals(true);
    ui->language->clear();
    ui->language->addItem("*",-1);
    ui->language->setCurrentIndex(0);
    ui->findLanguage->clear();
    ui->findLanguage->addItem("*",-1);
    ui->findLanguage->setCurrentIndex(0);

    QSettings *settings=GetSettings();
    QString sCurrentLanguage=settings->value("BookLanguage","*").toString();
    for(int iLang=0;iLang<mLibs[idCurrentLib].vLaguages.size();iLang++){
        QString sLanguage = mLibs[idCurrentLib].vLaguages[iLang].toUpper();
        if(!sLanguage.isEmpty()){
            ui->language->addItem(sLanguage,iLang);
            ui->findLanguage->addItem(sLanguage,iLang);
            if(sLanguage == sCurrentLanguage){
                ui->language->setCurrentIndex(ui->language->count()-1);
                idCurrentLanguage_ = iLang;
            }
        }
    }
    ui->language->model()->sort(0);
    settings->setValue("BookLanguage",ui->language->currentText());
    ui->language->blockSignals(false);
    ui->findLanguage->blockSignals(false);
    QApplication::restoreOverrideCursor();
}

void MainWindow::UpdateSeria()
{
    if(!db_is_open)
        return;
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    qint64 t_start = QDateTime::currentMSecsSinceEpoch();
    QSqlQuery query(QSqlDatabase::database("libdb"));

    qint64 t_end = QDateTime::currentMSecsSinceEpoch();
    qDebug()<< "UpdateSeria " << t_end-t_start << "msec";
    t_start = t_end;

    mLibs[idCurrentLib].mSerials.clear();
    query.prepare("SELECT id, name, favorite FROM seria WHERE id_lib=:id_lib;");
    //                    0   1     2
    query.bindValue(":id_lib",idCurrentLib);
    if(!query.exec())
        qDebug() << query.lastError().text();
    while (query.next()) {
        uint idSerial = query.value(0).toUInt();
        QString sName = query.value(1).toString();
        mLibs[idCurrentLib].mSerials[idSerial].sName = sName;
        mLibs[idCurrentLib].mSerials[idSerial].nTag = query.value(2).toUInt();
    }

    t_end = QDateTime::currentMSecsSinceEpoch();
    qDebug()<< "UpdateSeria2 " << t_end-t_start << "msec";
    QApplication::restoreOverrideCursor();
}

void MainWindow::UpdateJanre()
{
    if(!db_is_open)
        return;
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    qint64 t_start = QDateTime::currentMSecsSinceEpoch();

    QSqlQuery query(QSqlDatabase::database("libdb"));
    mGenre.clear();
    query.prepare("SELECT id, name, id_parent, sort_index FROM janre;");
    //                    0   1     2          3
    if(!query.exec())
        qDebug() << query.lastError().text();
    while (query.next()) {
        uint idGenre = query.value(0).toUInt();
        SGenre &genre = mGenre[idGenre];
        genre.sName = query.value(1).toString();
        genre.idParrentGenre = query.value(2).toUInt();
        genre.nSort = query.value(3).toUInt();
    }

    qint64 t_end = QDateTime::currentMSecsSinceEpoch();
    qDebug()<< "UpdateJanre " << t_end-t_start << "msec";

    QApplication::restoreOverrideCursor();
}

void MainWindow::ManageLibrary()
{
    SaveLibPosition();
    AddLibrary al(this);
    al.exec();
    if(al.bLibChanged){
        UpdateAuthor();
        UpdateJanre();
        UpdateTags();
        UpdateBooks();
        searchCanged(ui->searchString->text());
        setWindowTitle(AppName+(idCurrentLib<0||mLibs[idCurrentLib].name.isEmpty()?"":" - "+mLibs[idCurrentLib].name));
        FillLibrariesMenu();
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
void MainWindow::btnSeries()
{
    ui->tabWidget->setCurrentIndex(1);
    ui->SearchFrame->setEnabled(true);
    ui->frame_3->setEnabled(true);
    ui->language->setEnabled(true);
    SelectSeria();
}
void MainWindow::btnJanres()
{
    ui->tabWidget->setCurrentIndex(2);
    ui->SearchFrame->setEnabled(false);
    ui->frame_3->setEnabled(false);
    ui->language->setEnabled(true);
    SelectJanre();
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
        FillSerials();
        FillAuthors();
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
    if(pHelpDlg==0)
        pHelpDlg=new HelpDialog();
    pHelpDlg->show();
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
    //QSettings *settings=GetSettings();
    if(bUseTag_)
        menu.addActions(TagMenu.actions());
    if(menu.actions().count()>0)
        menu.exec(QCursor::pos());
}

void MainWindow::MoveToSeria(qlonglong id,QString FirstLetter)
{
    QTreeWidgetItem* Item=ui->Books->selectedItems()[0];
//    qlonglong CurrentID=Item->data(0,Qt::UserRole).toLongLong();
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
//                CurrentBookID=CurrentID;
            }
            else if(!IsAuthor)
            {
//                CurrentSeriaID=-CurrentID;
            }
            SelectSeria();
            return;
        }
    }
}
void MainWindow::MoveToJanre(qlonglong id)
{
    QTreeWidgetItem* Item=ui->Books->selectedItems()[0];
//    qlonglong CurrentID=Item->data(0,Qt::UserRole).toLongLong();
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
//                    CurrentBookID=CurrentID;
                }
                else if(!IsAuthor)
                {
//                    CurrentSeriaID=-CurrentID;
                }
                SelectJanre();
                return;
            }
        }
    }
}

void MainWindow::MoveToAuthor(qlonglong id, QString FirstLetter)
{
    QTreeWidgetItem* Item=ui->Books->selectedItems()[0];
    if(Item->type() == ITEM_TYPE_BOOK)
        idCurrentBook_ = Item->data(0,Qt::UserRole).toUInt();
    ui->searchString->setText(id<0?Item->text(0).left(1).toUpper():FirstLetter);
    ui->btnAuthor->setChecked(true);
    searchCanged(FirstLetter);
    btnAuthor();
    ui->AuthorList->clearSelection();
    for (int i=0;i<ui->AuthorList->count();i++)
    {
        if(ui->AuthorList->item(i)->data(Qt::UserRole).toLongLong()==id)

        {
            ui->AuthorList->item(i)->setSelected(true);
            ui->AuthorList->scrollToItem(ui->AuthorList->item(i));
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
    auto i = mLibs.constBegin();
    while(i!=mLibs.constEnd()){
        QAction *action=new QAction(i->name, this);
        action->setData(i.key());
        action->setCheckable(true);
        lib_menu->insertAction(0,action);
        connect(action,SIGNAL(triggered()),this,SLOT(SelectLibrary()));
        action->setChecked(i.key()==idCurrentLib);
        ++i;
    }
    if(lib_menu->actions().count()>0)
    {
        ui->actionLibraries->setMenu(lib_menu);
        ui->actionLibraries->setEnabled(true);
    }
}

void MainWindow::FillAuthors()
{
    qint64 t_start = QDateTime::currentMSecsSinceEpoch();
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    const bool wasBlocked = ui->AuthorList->blockSignals(true);
    QListWidgetItem *item;
    ui->AuthorList->clear();
    SLib &currentLib = mLibs[idCurrentLib];
    QListWidgetItem *selectedItem = nullptr;
    QString sSearch = ui->searchString->text();
    auto i = currentLib.mAuthors.constBegin();

    while(i!=currentLib.mAuthors.constEnd()){
        if(sSearch == "*" || (sSearch=="#" && !i->sName.left(1).contains(QRegExp("[A-Za-zа-яА-ЯЁё]"))) || i->sName.startsWith(sSearch,Qt::CaseInsensitive)){
            QList<uint> booksId = currentLib.mAuthorBooksLink.values(i.key());
            int count =0;
            foreach( uint idBook, booksId) {
                SBook &book = currentLib.mBooks[idBook];
//                uint idSerial=book.idSerial;
//                if((idCurrentLanguage_==-1 || idCurrentLanguage_ == book.idLanguage)
//                        &&(bShowDeleted || !book.bDeleted) &&
//                        (!use_tag || current_tag==0 || current_tag== book.nTag ||
//                         (idSerial>0 && mLibs[idCurrentLib].mSerials[idSerial].nTag == current_tag)))
                if(IsBookInList(book))
                {
                    count++;
                }
            }
            if(count>0){
                item=new QListWidgetItem(QString("%1 (%2)").arg(i->sName).arg(count));
                item->setData(Qt::UserRole,i.key());
                if(bUseTag_)
                    item->setIcon(GetTag(i->nTag));
                ui->AuthorList->addItem(item);
                if(idCurrentAuthor_ == i.key()){
                    item->setSelected(true);
                    selectedItem = item;
                }
            }
        }

        ++i;
    }
    if(selectedItem!=nullptr)
        ui->AuthorList->scrollToItem(selectedItem);

    ui->AuthorList->blockSignals(wasBlocked);
    qint64 t_end = QDateTime::currentMSecsSinceEpoch();
    qDebug()<< "FillAuthors " << t_end-t_start << "msec";
    QApplication::restoreOverrideCursor();
}

void MainWindow::FillSerials()
{
    qint64 t_start = QDateTime::currentMSecsSinceEpoch();
    const bool wasBlocked = ui->SeriaList->blockSignals(true);
    ui->SeriaList->clear();
    QString sSearch = ui->searchString->text();

    QMap<uint,uint> mCounts;
    auto iBook = mLibs[idCurrentLib].mBooks.constBegin();
    while(iBook!=mLibs[idCurrentLib].mBooks.constEnd()){
        if(IsBookInList(*iBook) &&
                (sSearch == "*" || (sSearch=="#" && !mLibs[idCurrentLib].mSerials[iBook->idSerial].sName.left(1).contains(QRegExp("[A-Za-zа-яА-ЯЁё]"))) || mLibs[idCurrentLib].mSerials[iBook->idSerial].sName.startsWith(sSearch,Qt::CaseInsensitive)))
        {
            if(mCounts.contains(iBook->idSerial))
                mCounts[iBook->idSerial]++;
            else
                mCounts[iBook->idSerial] = 1;

        }
        ++iBook;
    }

    QListWidgetItem *item;
    auto iSerial = mCounts.constBegin();
    while(iSerial!=mCounts.constEnd()){
        item=new QListWidgetItem(QString("%1 (%2)").arg(mLibs[idCurrentLib].mSerials[iSerial.key()].sName).arg(iSerial.value()));
        item->setData(Qt::UserRole,iSerial.key());
        if(bUseTag_)
            item->setIcon(GetTag(mLibs[idCurrentLib].mSerials[iSerial.key()].nTag));
        ui->SeriaList->addItem(item);
        if(iSerial.key()==idCurrentSerial_)
        {
            item->setSelected(true);
            ui->SeriaList->scrollToItem(item);
         }

        ++iSerial;
    }
//    if(current_list_for_tag==(QObject*)ui->SeriaList)
//        current_list_id=-1;

    ui->SeriaList->blockSignals(wasBlocked);
    qint64 t_end = QDateTime::currentMSecsSinceEpoch();
    qDebug()<< "FillSerials " << t_end-t_start << "msec";
}

void MainWindow::FillGenres()
{
    qint64 t_start = QDateTime::currentMSecsSinceEpoch();
    const bool wasBlocked = ui->JanreList->blockSignals(true);
    ui->JanreList->clear();
    ui->s_janre->clear();
    ui->s_janre->addItem("*",0);
    QFont bold_font(ui->AuthorList->font());
    bold_font.setBold(true);


    QMap<uint,uint> mCounts;
    auto iBook = mLibs[idCurrentLib].mBooks.constBegin();
    while(iBook!=mLibs[idCurrentLib].mBooks.constEnd()){
        if(IsBookInList(*iBook))
        {
            foreach (uint iGenre, iBook->listIdGenres) {
                if(mCounts.contains(iGenre))
                    mCounts[iGenre]++;
                else
                    mCounts[iGenre] = 1;
            }
        }
        ++iBook;
    }

    QMap<uint,QTreeWidgetItem*> mTopGenresItem;
    auto iGenre = mGenre.constBegin();
    while(iGenre!=mGenre.constEnd()){
        QTreeWidgetItem *item;
        if(iGenre->idParrentGenre==0 && !mTopGenresItem.contains(iGenre.key())){
            item=new QTreeWidgetItem(ui->JanreList);
            item->setFont(0,bold_font);
            item->setText(0,iGenre->sName);
            item->setData(0,Qt::UserRole,iGenre.key());
            item->setExpanded(false);
            mTopGenresItem[iGenre.key()] = item;
            ui->s_janre->addItem(iGenre->sName,iGenre.key());
        }else{
            if(mCounts.contains(iGenre.key())){
                if(!mTopGenresItem.contains(iGenre->idParrentGenre)){
                    QTreeWidgetItem *itemTop = new QTreeWidgetItem(ui->JanreList);
                    itemTop->setFont(0,bold_font);
                    itemTop->setText(0,mGenre[iGenre->idParrentGenre].sName);
                    itemTop->setData(0,Qt::UserRole,iGenre->idParrentGenre);
                    itemTop->setExpanded(false);
                    mTopGenresItem[iGenre->idParrentGenre] = itemTop;
                }
                item=new QTreeWidgetItem(mTopGenresItem[iGenre->idParrentGenre]);
                item->setText(0,QString("%1 (%2)").arg(iGenre->sName).arg(mCounts[iGenre.key()]));
                item->setData(0,Qt::UserRole,iGenre.key());
                ui->s_janre->addItem("   "+iGenre->sName,iGenre.key());
                if(iGenre.key()==idCurrentGenre_)
                {
                    item->setSelected(true);
                    ui->JanreList->scrollToItem(item);
                }
            }
        }
        ++iGenre;
    }

    ui->JanreList->blockSignals(wasBlocked);
    qint64 t_end = QDateTime::currentMSecsSinceEpoch();
    qDebug()<< "FillGenres " << t_end-t_start << "msec";
}

void MainWindow::FillListBooks()
{
    switch(ui->tabWidget->currentIndex()){
        case 0:
            SelectAuthor();
        break;

        case 1:
            SelectSeria();
        break;

        case 2:
            SelectJanre();
        break;

    }
}

void MainWindow::FillListBooks(QList<uint> listBook,uint idCurrentAuthor)
{
    qint64 t_start = QDateTime::currentMSecsSinceEpoch();
    QFont bold_font(ui->Books->font());
    bold_font.setBold(true);
    TreeBookItem* ScrollItem=0;

    TreeBookItem* item_seria=0;
    TreeBookItem* item_book;
    TreeBookItem* item_author;
    QMap<uint,TreeBookItem*> mAuthors;

    QMultiMap<uint,TreeBookItem*> mSerias;

    const bool wasBlocked = ui->Books->blockSignals(true);
    ui->Books->clear();

//    if(idCurrentAuthor>0){
//        item_author = new TreeBookItem(ui->Books,ITEM_TYPE_AUTHOR);
//        item_author->setText(0,mLibs[idCurrentLib].mAuthors[idCurrentAuthor].sName);
//        item_author->setExpanded(true);
//        item_author->setFont(0,bold_font);
//        item_author->setCheckState(0,Qt::Unchecked);
//        item_author->setData(0,Qt::UserRole,idCurrentAuthor);
//        if(use_tag)
//            item_author->setIcon(0,GetTag(mLibs[idCurrentLib].mAuthors[idCurrentAuthor].ntag));
//        mAuthors[idCurrentAuthor] = item_author;

//    }

    foreach( uint idBook, listBook) {
        SBook &book = mLibs[idCurrentLib].mBooks[idBook];
        if(IsBookInList(book))
        {
            uint idSerial=book.idSerial;
            uint idAuthor;
            if(idCurrentAuthor>0)
                idAuthor = idCurrentAuthor;
            else{
                idAuthor = book.idFirstAuthor;
            }
            if(!mAuthors.contains(idAuthor)){
                item_author = new TreeBookItem(ui->Books,ITEM_TYPE_AUTHOR);
                item_author->setText(0,mLibs[idCurrentLib].mAuthors[idAuthor].sName);
                item_author->setExpanded(true);
                item_author->setFont(0,bold_font);
                item_author->setCheckState(0,Qt::Unchecked);
                item_author->setData(0,Qt::UserRole,idAuthor);
                if(bUseTag_)
                    item_author->setIcon(0,GetTag(mLibs[idCurrentLib].mAuthors[idAuthor].nTag));
                mAuthors[idAuthor] = item_author;
            }else
                item_author = mAuthors[idAuthor];

            if(idSerial>0){
                auto iSerial = mSerias.find(idSerial);
                while(iSerial != mSerias.constEnd()){
                    item_seria = iSerial.value();
                    if(item_seria->parent()->data(0,Qt::UserRole)==idAuthor)
                        break;
                    ++iSerial;
                }
                if(iSerial==mSerias.constEnd()){
                    item_seria = new TreeBookItem(mAuthors[idAuthor],ITEM_TYPE_SERIA);
                    item_seria->setText(0,mLibs[idCurrentLib].mSerials[idSerial].sName);
                    item_author->addChild(item_seria);
                    item_seria->setExpanded(true);
                    item_seria->setFont(0,bold_font);
                    item_seria->setCheckState(0,Qt::Unchecked);
                    item_seria->setData(0,Qt::UserRole,idSerial);
                    if(bUseTag_)
                        item_seria->setIcon(0,GetTag(mLibs[idCurrentLib].mSerials[idSerial].nTag));

                    mSerias.insert(idSerial,item_seria);

                }
                item_book = new TreeBookItem(item_seria,ITEM_TYPE_BOOK);
            }else
                item_book = new TreeBookItem(item_author,ITEM_TYPE_BOOK);

            item_book->setCheckState(0,Qt::Unchecked);
            item_book->setData(0,Qt::UserRole,idBook);
            if(bUseTag_)
                item_book->setIcon(0,GetTag(book.nTag));

            item_book->setText(0,book.sName);
            if(book.numInSerial>0){
                item_book->setText(1,QString::number(book.numInSerial));
                item_book->setTextAlignment(1, Qt::AlignRight);
            }

            if(book.nSize>0)
                item_book->setText(2,QString::number(book.nSize));
            item_book->setTextAlignment(2, Qt::AlignRight);

            QPixmap pix(":/icons/img/icons/stars/"+QString::number(book.nStars).trimmed()+QString("star%1.png").arg(app->devicePixelRatio()>=2?"@2x":""));
            pix.setDevicePixelRatio(app->devicePixelRatio());
            item_book->setData(3,Qt::DecorationRole,pix);

            item_book->setText(4,book.date.toString("dd.MM.yyyy"));
            item_book->setTextAlignment(4, Qt::AlignCenter);

            item_book->setText(5,mGenre[book.listIdGenres.first()].sName);
            item_book->setTextAlignment(5, Qt::AlignLeft);

            if(book.bDeleted)
            {
                item_book->setTextColor(0,QColor::fromRgb(96,96,96));
                item_book->setTextColor(1,QColor::fromRgb(96,96,96));
                item_book->setTextColor(2,QColor::fromRgb(96,96,96));
                item_book->setTextColor(3,QColor::fromRgb(96,96,96));
                item_book->setTextColor(4,QColor::fromRgb(96,96,96));
                item_book->setTextColor(5,QColor::fromRgb(96,96,96));
            }

            if(idBook==idCurrentBook_)
            {
                ScrollItem=item_book;
            }
        }
    }
    if(ScrollItem)
    {
        ScrollItem->setSelected(true);
        ui->Books->scrollToItem(ScrollItem);
    }
    SelectBook();

    ui->Books->blockSignals(wasBlocked);
    qint64 t_end = QDateTime::currentMSecsSinceEpoch();
    qDebug()<< "FillListBooks " << t_end-t_start << "msec";
}

bool MainWindow::IsBookInList(const SBook &book)
{
    int current_tag=ui->TagFilter->itemData(ui->TagFilter->currentIndex()).toInt();
    uint idSerial=book.idSerial;

    return (idCurrentLanguage_==-1 || idCurrentLanguage_ == book.idLanguage)
            &&(bShowDeleted_ || !book.bDeleted)&&
            (!bUseTag_ || current_tag==0 || current_tag==book.nTag
             ||(idSerial>0 && mLibs[idCurrentLib].mSerials[idSerial].nTag == current_tag)
             ||(mLibs[idCurrentLib].mAuthors[book.idFirstAuthor].nTag == current_tag));
}

void MainWindow::dropEvent(QDropEvent *ev)
{
    if(mode==MODE_LIBRARY)
        pDropForm->hide();
    QList<QUrl> urls = ev->mimeData()->urls();
    QStringList book_list;
    foreach(QUrl url, urls)
    {
        proc_path(url.path(),&book_list);
    }
    if(book_list.count())
    {
        ExportDlg dlg(this);
        int id=pDropForm->get_command(ev->pos());
        if(id<0)
        {
            pDropForm->get_command(QPoint(-1,-1));
            return;
        }
        dlg.exec(book_list,SetCurrentExportSettings(id));
    }
    pDropForm->get_command(QPoint(-1,-1));
}

void MainWindow::DeleteDropForm()
{
    if(pDropForm!=nullptr)
    {
        if(pDropForm->isHidden())
        {
            delete pDropForm;
            pDropForm=nullptr;
        }
    }
}

void MainWindow::ShowDropForm()
{
    if(pDropForm==0)
        pDropForm=new DropForm(this);
    if(mode==MODE_CONVERTER)
    {
        pDropForm->setFixedWidth(ui->drop_buttons->rect().width());
        pDropForm->setFixedHeight(ui->drop_buttons->rect().height());
        pDropForm->move(ui->drop_buttons->mapToGlobal(ui->drop_buttons->pos())-this->mapToGlobal(QPoint(0,0)));
    }
    else
    {
        pDropForm->setFixedWidth(rect().width()/10*9);
        pDropForm->setFixedHeight(rect().height()/10*9);
        pDropForm->move(QPoint(rect().width()/20,rect().height()/20));
    }
    QStringList cmd;
    foreach (QAction* action, ui->btnExport->menu()->actions())
    {
        cmd<<action->text();
    }
    pDropForm->AddCommand(cmd);
    pDropForm->setWindowFlags(Qt::WindowStaysOnTopHint);
    pDropForm->show();
    pDropForm->activateWindow();
    pDropForm->raise();
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
            pDropForm->hide();
    }
}
void MainWindow::dragMoveEvent(QDragMoveEvent *ev)
{
    pDropForm->switch_command(ev->pos());
}

void MainWindow::dragLeaveEvent(QDragLeaveEvent *)
{
    if(mode==MODE_LIBRARY)
        pDropForm->hide();
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
    if(pDropForm!=nullptr)
    {
        pDropForm->hide();
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
    if(pDropForm!=nullptr)
    {
        delete pDropForm;
        pDropForm=nullptr;
    }
    ui->stackedWidget->setCurrentWidget(ui->pageLabrary);
    ui->actionSwitch_to_library_mode->setVisible(false);
    ui->actionSwitch_to_convert_mode->setVisible(true);

    ui->actionCheck_uncheck->setVisible(true);
    ui->actionLibraries->setVisible(true);
    ui->actionAddLibrary->setVisible(true);
    ui->actionNew_labrary_wizard->setVisible(true);

    setWindowTitle(AppName+(idCurrentLib<0||mLibs[idCurrentLib].name.isEmpty()?"":" - "+mLibs[idCurrentLib].name));

    setMinimumSize(800,400);
    if(settings->contains("MainWnd/geometry"))
        restoreGeometry(settings->value("MainWnd/geometry").toByteArray());
    if(settings->contains("MainWnd/windowState"))
        restoreState(settings->value("MainWnd/windowState").toByteArray());
    if(settings->contains("MainWnd/tab/geometry"))
        ui->splitter->restoreState(settings->value("MainWnd/tab/geometry").toByteArray());
    //on_splitter_splitterMoved(0,0);
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

void MainWindow::resizeEvent(QResizeEvent */*e*/)
{

    if(pDropForm!=nullptr)
    {
        if(pDropForm->isVisible())
        {
            const QSignalBlocker blocker(this);
            //delete dropForm;
            pDropForm->deleteLater();
            pDropForm=nullptr;
            ShowDropForm();
        }
    }
}
void MainWindow::mouseMoveEvent(QMouseEvent *ev)
{
    if(mode==MODE_CONVERTER)
    {
        if(pDropForm==nullptr)
        {
            ShowDropForm();
        }
        pDropForm->switch_command(ev->pos());
    }
}

void MainWindow::leaveEvent(QEvent */*ev*/)
{
    if(pDropForm!=nullptr)
    {
        pDropForm->switch_command(QPoint(-1,-1));
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *ev)
{
    if(mode==MODE_CONVERTER)
    {
        if(pDropForm==nullptr)
        {
            ShowDropForm();
        }
        int id=pDropForm->get_command(ev->pos());
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

void MainWindow::on_language_currentIndexChanged(const QString &arg1)
{
    QSettings *settings=GetSettings();
    settings->setValue("BookLanguage",arg1);
    idCurrentLanguage_ = ui->language->currentData().toInt();

    FillSerials();
    FillAuthors();
    FillGenres();
    FillListBooks();
}
