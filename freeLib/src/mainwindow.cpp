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
#include "SmtpClient/src/smtpclient.h"
#include "SmtpClient/src/mimefile.h"
#include "SmtpClient/src/mimetext.h"
#include "SmtpClient/src/mimeattachment.h"
#include "addlibrary.h"
#include "settingsdlg.h"
#include "exportdlg.h"
#include "exportthread.h"
#include "aboutdialog.h"
#include "tagdialog.h"
#include "libwizard.h"
#include "bookeditdlg.h"
#include "webpage.h"
#include "treebookitem.h"
#include "genresortfilterproxymodel.h"
#include "library.h"

extern QSplashScreen *splash;

bool db_is_open;

QPixmap GetTag(QColor color,int size)
{
    QPixmap pixmap(size,size-4);
    pixmap.fill(Qt::transparent);
    QPainter paint(&pixmap);
    paint.setBrush(QBrush(color));
    QPen pen=QPen(QColor(static_cast<int>(color.red()*0.5),static_cast<int>(color.green()*0.5),static_cast<int>(color.blue()*0.5),static_cast<int>(color.alpha()*0.5)));
    paint.setPen(pen);
    paint.drawEllipse(2,0,size-5,size-5);
    return pixmap;
}

QString sizeToString(uint size)
{
    QStringList mem;
    mem <<QCoreApplication::translate("MainWindow","B")<<QCoreApplication::translate("MainWindow","kB")<<QCoreApplication::translate("MainWindow","MB")<<QCoreApplication::translate("MainWindow","GB")<<QCoreApplication::translate("MainWindow","TB")<<QCoreApplication::translate("MainWindow","PB");
    uint rest=0;
    int mem_i=0;

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
    double size_d = (float)size + (float)rest / 1024.0;
    return QString("%L1 %2").arg(size_d,0,'f',mem_i>0?1:0).arg(mem[mem_i]);
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    trIcon=nullptr;
    pDropForm=nullptr;
    error_quit=false;
    QSettings settings;

    if(db_is_open)
    {
        QSqlQuery query(QSqlDatabase::database("libdb"));
        //query.exec("CREATE TABLE IF NOT EXISTS params (id INTEGER PRIMARY KEY, name TEXT, value TEXT)");
        query.exec(QString("SELECT value FROM params WHERE name='%1'").arg("version"));
        int version=0;
        if(query.next())
        {
            version=query.value(0).toInt();
        }
        if(version<6){
            splash->hide();
            if(QMessageBox::question(nullptr,tr("Database"),tr("This version needs new database version. All your old books data will be lost. Continue?"),QMessageBox::Yes|QMessageBox::No,QMessageBox::No)==QMessageBox::Yes)
            {
                if(!openDB(false,true))
                    error_quit=true;
            }
            else
            {
                error_quit=true;
            }
        }
    }


    ui->setupUi(this);
    ui->btnEdit->setVisible(false);
    ui->searchString->setFocus();
    ui->tabWidget->setCurrentIndex(0);
    ui->Books->setColumnWidth(0,400);
    ui->Books->setColumnWidth(1,50);
    ui->Books->setColumnWidth(2,100);
    ui->Books->setColumnWidth(3,90);
    ui->Books->setColumnWidth(4,120);
    ui->Books->setColumnWidth(5,250);
    ui->Books->setColumnWidth(6,50);

    QPalette palette = QApplication::style()->standardPalette();
    bool darkTheme = palette.color(QPalette::Window).lightness()<127;
    QString sIconsPath = QStringLiteral(":/img/icons/") + (darkTheme ?QStringLiteral("dark/") :QStringLiteral("light/"));
    ui->btnExport->setIcon(QIcon::fromTheme(QStringLiteral("tablet"),QIcon(sIconsPath + QStringLiteral("streamline.svg"))));
    ui->btnOpenBook->setIcon(QIcon(sIconsPath + QStringLiteral("book.svg")));
    ui->btnEdit->setIcon(QIcon::fromTheme(QStringLiteral("document-edit"),QIcon(sIconsPath + QStringLiteral("pen.svg"))));
    ui->btnCheck->setIcon(QIcon::fromTheme(QStringLiteral("checkbox"),QIcon(sIconsPath + QStringLiteral("checkbox.svg"))));
    ui->btnLibrary->setIcon(QIcon(sIconsPath + QStringLiteral("library.svg")));
    ui->btnOption->setIcon(QIcon::fromTheme(QStringLiteral("settings-configure"),QIcon(sIconsPath + QStringLiteral("settings.svg"))));

    GenreSortFilterProxyModel *MyProxySortModel = new GenreSortFilterProxyModel(ui->s_genre);
    MyProxySortModel->setSourceModel(ui->s_genre->model());
    ui->s_genre->model()->setParent(MyProxySortModel);
    ui->s_genre->setModel(MyProxySortModel);
    MyProxySortModel->setDynamicSortFilter(false);

    ui->Review->setPage(new WebPage());
    connect(static_cast<WebPage*>(ui->Review->page()), &WebPage::linkClicked, this, &MainWindow::ReviewLink);

    setWindowTitle(AppName+(idCurrentLib<0||mLibs[idCurrentLib].name.isEmpty()?"":" - "+mLibs[idCurrentLib].name));

    idCurrentLanguage_ = -1;
    bUseTag_=settings.value("use_tag",true).toBool();
    bShowDeleted_ =settings.value("ShowDeleted").toBool();
    int nCurrentTab;

    if(settings.value("store_position",true).toBool())
    {
        idCurrentAuthor_= settings.value("current_author_id",0).toUInt();
        idCurrentSerial_ = settings.value("current_serial_id",0).toUInt();
        idCurrentBook_ = settings.value("current_book_id",0).toUInt();
        idCurrentGenre_ = settings.value("current_genre_id",0).toUInt();
        nCurrentTab = settings.value("current_tab",0).toInt();
        QString sFilter = settings.value("filter_set").toString();
        ui->searchString->setText(sFilter);
        ui->searchString->setClearButtonEnabled(sFilter.length()>1);
    }
    else
    {
        idCurrentAuthor_ = 0;
        idCurrentSerial_ = 0;
        idCurrentBook_ = 0;
        idCurrentGenre_ = 0;
        nCurrentTab = TabAuthors;
    }

    UpdateTags();
    loadGenres();
    loadLibrary(idCurrentLib);
    UpdateBooks();

    FillAuthors();
    FillSerials();
    FillGenres();

    connect(ui->searchString, &QLineEdit::textChanged, this, &MainWindow::searchCanged);
    connect(ui->actionAddLibrary, &QAction::triggered, this, &MainWindow::ManageLibrary);
    connect(ui->btnLibrary, &QAbstractButton::clicked, this, &MainWindow::ManageLibrary);
    connect(ui->btnOpenBook, &QAbstractButton::clicked, this, &MainWindow::BookDblClick);
    connect(ui->btnOption, &QAbstractButton::clicked, this, &MainWindow::Settings);
    connect(ui->actionPreference, &QAction::triggered, this, &MainWindow::Settings);
    connect(ui->actionCheck_uncheck, &QAction::triggered, this, &MainWindow::CheckBooks);
    connect(ui->btnCheck, &QAbstractButton::clicked, this, &MainWindow::CheckBooks);
    connect(ui->btnEdit, &QAbstractButton::clicked, this, &MainWindow::EditBooks);
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
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
    connect(ui->AuthorList, &QListWidget::itemSelectionChanged, this, &MainWindow::SelectAuthor);
    connect(ui->Books, &QTreeWidget::itemSelectionChanged, this, &MainWindow::SelectBook);
    connect(ui->Books, &QTreeWidget::itemDoubleClicked, this, &MainWindow::BookDblClick);
    connect(ui->GenreList, &QTreeWidget::itemSelectionChanged, this, &MainWindow::SelectGenre);
    connect(ui->SeriaList, &QListWidget::itemSelectionChanged, this, &MainWindow::SelectSeria);
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabWidgetChanged);
    connect(ui->do_search, &QAbstractButton::clicked, this, &MainWindow::StartSearch);
    connect(ui->s_author, &QLineEdit::returnPressed, this, &MainWindow::StartSearch);
    connect(ui->s_seria, &QLineEdit::returnPressed, this, &MainWindow::StartSearch);
    connect(ui->s_name, &QLineEdit::returnPressed, this, &MainWindow::StartSearch);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::About);
    connect(ui->actionNew_labrary_wizard, &QAction::triggered, this, &MainWindow::newLibWizard);
    connect(ui->Books, &QTreeWidget::itemChanged, this, &MainWindow::itemChanged);
    connect(ui->language, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &MainWindow::onLanguageCurrentIndexChanged);
    QList<QAction*> actionList = ui->searchString->findChildren<QAction*>();
    if (!actionList.isEmpty()) {
        connect(actionList.first(), &QAction::triggered, this, &MainWindow::searchClear);
    }


    ChangingLanguage(false);
    ExportBookListBtn(false);


    mode=static_cast<APP_MODE>(settings.value("ApplicationMode",0).toInt());
    switch(mode)
    {
    case MODE_LIBRARY:
        on_actionSwitch_to_library_mode_triggered();
        break;
    case MODE_SHELF:
        //on_actionSwitch_to_shelf_mode_triggered();
        break;
    default:
        connect(this, &MainWindow::window_loaded, this, &MainWindow::on_actionSwitch_to_convert_mode_triggered);
        on_actionSwitch_to_convert_mode_triggered();
        break;
    }

    ui->tabWidget->setCurrentIndex(nCurrentTab);
    if(nCurrentTab == TabAuthors)
        SelectAuthor();

    if(ui->searchString->text().trimmed().isEmpty())
        FirstButton->click();

    ui->date_to->setDate(QDate::currentDate());

    pHelpDlg=nullptr;
    connect(ui->actionHelp,&QAction::triggered,this,&MainWindow::HelpDlg);
    ui->Books->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->Books, &QWidget::customContextMenuRequested, this, &MainWindow::ContextMenu);
    ui->AuthorList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->AuthorList, &QWidget::customContextMenuRequested, this, &MainWindow::ContextMenu);
    ui->SeriaList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->SeriaList, &QWidget::customContextMenuRequested, this, &MainWindow::ContextMenu);
    connect(ui->TagFilter, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &MainWindow::tag_select);
    ui->Books->header()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->Books->header(),&QWidget::customContextMenuRequested,this,&MainWindow::HeaderContextMenu);

    opds.server_run();
    FillLibrariesMenu();
    UpdateExportMenu();

    ChangingTrayIcon();

#ifdef Q_OS_OSX
    connect(MyPrivate::instance(), SIGNAL(dockClicked()), SLOT(dockClicked()));
#endif
    connect(ui->actionMinimize_window, &QAction::triggered, this, &MainWindow::MinimizeWindow);

    settings.beginGroup("Columns");
    ui->Books->setColumnHidden(0,!settings.value("ShowName",true).toBool());
    ui->Books->setColumnHidden(1,!settings.value("ShowNumber",true).toBool());
    ui->Books->setColumnHidden(2,!settings.value("ShowSize",true).toBool());
    ui->Books->setColumnHidden(3,!settings.value("ShowMark",true).toBool());
    ui->Books->setColumnHidden(4,!settings.value("ShowImportDate",true).toBool());
    ui->Books->setColumnHidden(5,!settings.value("ShowGenre",true).toBool());
    ui->Books->setColumnHidden(6,!settings.value("ShowLanguage",false).toBool());
    QVariant varHeaders = settings.value("headers");
    if(varHeaders.type() == QVariant::ByteArray){
        ui->Books->header()->restoreState(varHeaders.toByteArray());
    }

    settings.endGroup();
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
    QSettings settings;

    QButtonGroup *group=new QButtonGroup(this);
    group->setExclusive(true);
    const bool wasBlocked = ui->TagFilter->blockSignals(true);

    int size =static_cast<int>(ui->TagFilter->style()->pixelMetric(QStyle::PM_SmallIconSize)*app->devicePixelRatio());
    QSqlQuery query(QSqlDatabase::database("libdb"));
    query.exec("SELECT color,name,id from favorite");
    ui->TagFilter->clear();
    int con=1;
    ui->TagFilter->addItem("*",0);
    TagMenu.clear();
    QAction *ac=new QAction(tr("no tag"),&TagMenu);
    ac->setData(0);
    connect(ac, &QAction::triggered, this, &MainWindow::set_tag);
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
        if(settings.value("current_tag").toInt()==ui->TagFilter->count()-1 && bUseTag_)
            ui->TagFilter->setCurrentIndex(ui->TagFilter->count()-1);
        pix=::GetTag(QColor(query.value(0).toString().trimmed()),size);
        Stag new_tag={pix,query.value(2).toInt()};
        tags_pic<<new_tag;
        ui->TagFilter->setItemData(con, pix, Qt::DecorationRole);//Добавляем изображение цвета в комбо
        con++;
        QAction *ac=new QAction(pix,query.value(1).toString().trimmed(),&TagMenu);
        ac->setData(query.value(2).toString());
        connect(ac, &QAction::triggered, this, &MainWindow::set_tag);
        TagMenu.addAction(ac);
    }

    ui->TagFilter->addItem(tr("setup ..."),-1);
    ui->TagFilter->blockSignals(wasBlocked);

    QApplication::restoreOverrideCursor();
}

MainWindow::~MainWindow()
{
    QSettings settings;
    settings.beginGroup("Columns");
    QByteArray baHeaders = ui->Books->header()->saveState();
    settings.setValue("headers",baHeaders);
    delete ui->Review->page();
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
        loadLibrary(idCurrentLib);
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
    QString sPath = url.path();
    if(sPath.startsWith("/author_"))
    {
        MoveToAuthor(sPath.right(sPath.length()-9).toLongLong(),sPath.mid(8,1).toUpper());
    }
    else if(sPath.startsWith("/genre_"))
    {
        MoveToGenre(sPath.right(sPath.length()-8).toLongLong());
    }
    else if(sPath.startsWith("/seria_"))
    {
        MoveToSeria(sPath.right(sPath.length()-8).toLongLong(),sPath.mid(7,1).toUpper());
    }
    else if(sPath.startsWith("/show_fileinfo"))
    {
        QSettings settings;
        settings.setValue("show_fileinfo",!settings.value("show_fileinfo",false).toBool());
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
    QSettings settings;
    QFile file(FindLocaleFile(settings.value("localeABC",QLocale::system().name()).toString(),"abc","txt"));
    QString abc_local="ABC";
    if(!file.fileName().isEmpty() && file.open(QFile::ReadOnly))
    {
        abc_local=QString::fromUtf8(file.readLine()).toUpper();
    }
    QVBoxLayout *layout_abc_all=new QVBoxLayout();

    if(ui->abc->layout())
    {

        while(!(qobject_cast<QBoxLayout*>(ui->abc->layout())->itemAt(0))->isEmpty())
        {
            delete dynamic_cast<QBoxLayout*>(ui->abc->layout()->itemAt(0))->itemAt(0)->widget();
        }
        if(!qobject_cast<QBoxLayout*>(ui->abc->layout())->isEmpty())
        {
            while(!(dynamic_cast<QBoxLayout*>(ui->abc->layout()->itemAt(1)))->isEmpty())
            {
                delete dynamic_cast<QBoxLayout*>(ui->abc->layout()->itemAt(1))->itemAt(0)->widget();
            }
        }

        while(!ui->abc->layout()->isEmpty())
        {
            delete ui->abc->layout()->itemAt(0);
        }
        delete ui->abc->layout();
    }

    FirstButton=nullptr;
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
            connect(btn, &QAbstractButton::clicked, this, &MainWindow::btnSearch);
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
                connect(btn, &QAbstractButton::clicked, this, &MainWindow::btnSearch);
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
    uchar tag_id=static_cast<uchar>(qobject_cast<QAction*>(QObject::sender())->data().toInt());
    uint id;
    QSqlQuery query(QSqlDatabase::database("libdb"));

    if(current_list_for_tag==qobject_cast<QObject*>(ui->Books))
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
    else if(current_list_for_tag==qobject_cast<QObject*>(ui->AuthorList))
    {
        id=ui->AuthorList->selectedItems()[0]->data(Qt::UserRole).toUInt();
        update_list_pix(id,1,tag_id);
        query.prepare("UPDATE author set favorite=:favorite where id=:id");
        query.bindValue(":favorite",tag_id);
        query.bindValue(":id",id);
        query.exec();
        mLibs[idCurrentLib].mAuthors[id].nTag = tag_id;
    }
    else if(current_list_for_tag==qobject_cast<QObject*>(ui->SeriaList))
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
    QSettings settings;
    if(ui->TagFilter->itemData(ui->TagFilter->currentIndex()).toInt()==-1)
    {
        const bool wasBlocked = ui->TagFilter->blockSignals(true);
        ui->TagFilter->setCurrentIndex(settings.value("current_tag",0).toInt());
        ui->TagFilter->blockSignals(wasBlocked);
        TagDialog td(this);
        if(td.exec())
            UpdateTags();
    }
    else if(index>=0)
    {
        settings.setValue("current_tag",index);
        FillListBooks();
        FillAuthors();
        FillSerials();
        FillGenres();
    }
}

void MainWindow::SaveLibPosition()
{
    QSettings settings;
    settings.setValue("filter_set",ui->searchString->text());
    settings.setValue("current_tab",ui->tabWidget->currentIndex());
    settings.setValue("current_book_id",idCurrentBook_);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if(pHelpDlg!=nullptr)
        delete pHelpDlg;
    SaveLibPosition();
    QSettings settings;
    settings.setValue("ApplicationMode", mode);
    if(mode==MODE_LIBRARY)
    {
        settings.setValue("MainWnd/geometry", saveGeometry());
        settings.setValue("MainWnd/windowState", saveState());
        settings.setValue("MainWnd/tab/geometry",ui->tabWidget->saveGeometry());
        settings.setValue("MainWnd/tab/geometry",ui->splitter->saveState());
        settings.setValue("MainWnd/books/geometry",ui->splitter_2->saveState());
        settings.setValue("MainWnd/books_head/geometry",ui->Books->header()->saveState());
    }
    else
    {
        settings.setValue("MainWndConvertMode/geometry", saveGeometry());
    }
    if(ui->btnExport->defaultAction())
        settings.setValue("DefaultExport",ui->btnExport->defaultAction()->data().toInt());
    QString TempDir="";
    if(QStandardPaths::standardLocations(QStandardPaths::TempLocation).count()>0)
        TempDir=QStandardPaths::standardLocations(QStandardPaths::TempLocation).at(0);
    QDir(TempDir+"/freeLib/").removeRecursively();
    QMainWindow::closeEvent(event);
}

void MainWindow::ChangingPort(int i)
{
    opds.server_run(i);
}

void MainWindow::Settings()
{
    QSettings settings;
    if(ui->btnExport->defaultAction())
    {
        settings.setValue("DefaultExport",ui->btnExport->defaultAction()->data().toInt());
    }
    SettingsDlg dlg(this);
    connect(&dlg, &SettingsDlg::ChangingPort, this, &MainWindow::ChangingPort);
    connect(&dlg, &SettingsDlg::ChangingLanguage, this, [=](){this->ChangingLanguage();});
    connect(&dlg, &SettingsDlg::ChangingTrayIcon, this, &MainWindow::ChangingTrayIcon);
    dlg.exec();
    settings.setValue("LibID",idCurrentLib);
    if(bShowDeleted_!=settings.value("ShowDeleted").toBool() || bUseTag_!=settings.value("use_tag").toBool())
    {
        bUseTag_ = settings.value("use_tag").toBool();
        bShowDeleted_ = settings.value("ShowDeleted").toBool();
        UpdateTags();
        SaveLibPosition();
        FillAuthors();
        FillGenres();
        FillListBooks();
    }
    opds.server_run();
    UpdateExportMenu();
    resizeEvent(nullptr);
}

void MainWindow::FillCheckedBookList(QList<uint> &list,QTreeWidgetItem* item,bool send_all,bool checked_only)
{
    FillCheckedItemsBookList(list,item,send_all);
    if(list.count()==0 && !checked_only)
    {
        if(ui->Books->selectedItems().count()>0)
        {
            if(ui->Books->selectedItems()[0]->childCount()>0)
                FillCheckedItemsBookList(list,ui->Books->selectedItems()[0],true);
            else
            {
                if(ui->Books->selectedItems()[0]->parent())
                {
                    qlonglong id_book=ui->Books->selectedItems()[0]->data(0,Qt::UserRole).toLongLong();
                    list<<id_book;
                }
            }
        }
    }
}

void MainWindow::FillCheckedItemsBookList(QList<uint> &list,QTreeWidgetItem* item,bool send_all)
{
    QTreeWidgetItem* current;
    for(int i=0;i<(item?item->childCount():ui->Books->topLevelItemCount());i++)
    {
        current=item?item->child(i):ui->Books->topLevelItem(i);
        if(current->childCount()>0)
        {
            FillCheckedItemsBookList(list,current,send_all);
        }
        else
        {
            if(current->checkState(0)==Qt::Checked || send_all)
            {
                if(current->parent())
                {
                    qlonglong id_book=current->data(0,Qt::UserRole).toLongLong();
                    list<<id_book;
                }
            }
        }
    }
}

void MainWindow::uncheck_books(QList<qlonglong> list)
{
    QSettings settings;
    if(!settings.value("uncheck_export",true).toBool())
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
//    QList<book_info> book_list;
    QList<uint> book_list;
    FillCheckedBookList(book_list);
    if(book_list.count()==0)
        return;
    ExportDlg dlg(this);
    dlg.exec(book_list,ST_Device,(ui->tabWidget->currentIndex()==TabAuthors ?ui->AuthorList->selectedItems()[0]->data(Qt::UserRole).toLongLong():0));
    uncheck_books(dlg.succesfull_export_books);
}

void MainWindow::SendMail()
{
    //QList<book_info> book_list;
    QList<uint> book_list;
    FillCheckedBookList(book_list);
    if(book_list.count()==0)
        return;
    ExportDlg dlg(this);
    dlg.exec(book_list,ST_Mail,(ui->tabWidget->currentIndex()==TabAuthors ?ui->AuthorList->selectedItems()[0]->data(Qt::UserRole).toLongLong():0));
    uncheck_books(dlg.succesfull_export_books);
}


void MainWindow::BookDblClick()
{
    if(ui->Books->selectedItems().count()==0)
        return;
    QTreeWidgetItem* item=ui->Books->selectedItems()[0];
    QBuffer outbuff;
    QBuffer infobuff;
    QFileInfo fi=mLibs[idCurrentLib].getBookFile(outbuff,infobuff,item->data(0,Qt::UserRole).toUInt());
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

    QSettings settings;
    int count=settings.beginReadArray("application");
    // проверит цикл
    for(int i=0;i<count;i++)
    {
        settings.setArrayIndex(i);
        if((settings.value("ext").toString()+";").toLower().contains(fi.suffix().toLower()+";"))
        {
            if(
#ifdef Q_OS_MACX
            QProcess::startDetached("open",QStringList()<<settings.value("app").toString()<<"--args"<<file.fileName())&&
                    QFileInfo(settings.value("app").toString()).exists()
#else
            QProcess::startDetached(settings.value("app").toString(),QStringList()<<file.fileName())
#endif
            )
                settings.endArray();
                return;
        }
    }
    settings.endArray();

    QDesktopServices::openUrl(QUrl::fromLocalFile(file.fileName()));
    settings.sync();
}

void MainWindow::CheckBooks()
{
    QList<uint> book_list;
    FillCheckedBookList(book_list,nullptr,false,true);

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
    const bool wasBlocked = ui->Books->blockSignals(true);
    CheckChild(item);
    QTreeWidgetItem* parent=item->parent();
    if(parent)
        CheckParent(parent);
    QList<uint> book_list;
    FillCheckedBookList(book_list,nullptr,false);
    ExportBookListBtn(book_list.count()!=0);

    ui->Books->blockSignals(wasBlocked);
}

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
    QString sName = ui->s_name->text().trimmed();
    QString sAuthor = ui->s_author->text().trimmed();
    QString sSeria = ui->s_seria->text().trimmed();
    QDate dateFrom = ui->date_from->date();
    QDate dateTo = ui->date_to->date();
    int nMaxCount = ui->maxBooks->value();
    uint idGenre = ui->s_genre->currentData().toUInt();
    int idLanguage = ui->findLanguage->currentData().toInt();

    QList<uint> listBooks;
    int nCount = 0;
    auto iBook = mLibs[idCurrentLib].mBooks.constBegin();
    while(iBook != mLibs[idCurrentLib].mBooks.constEnd()){
        if((bShowDeleted_ || !iBook->bDeleted)&&
                iBook->date>= dateFrom && iBook->date <= dateTo &&
                (sAuthor.isEmpty() || mLibs[idCurrentLib].mAuthors[iBook->idFirstAuthor].getName().contains(sAuthor,Qt::CaseInsensitive)) &&
                (sName.isEmpty() || iBook->sName.contains(sName,Qt::CaseInsensitive)) &&
                (sSeria.isEmpty() || (iBook->idSerial>0 && mLibs[idCurrentLib].mSerials[iBook->idSerial].sName.contains(sSeria,Qt::CaseInsensitive))) &&
                (idLanguage == -1 ||(iBook->idLanguage == idLanguage)))
        {
            if(idGenre==0){
                nCount++;
                listBooks << iBook.key();
            }else
            {
                foreach (uint id,iBook->listIdGenres) {
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
    QAction* action=qobject_cast<QAction*>(sender());

    SaveLibPosition();
    QSettings settings;
    settings.setValue("LibID",action->data().toLongLong());
    idCurrentLib=action->data().toInt();

    loadLibrary(idCurrentLib);
    UpdateBooks();
    FillAuthors();
    FillSerials();
    FillGenres();
    searchCanged(ui->searchString->text());
    setWindowTitle(AppName+(idCurrentLib<0||mLibs[idCurrentLib].name.isEmpty()?"":" - "+mLibs[idCurrentLib].name));
    FillLibrariesMenu();
}

void MainWindow::SelectGenre()
{
    ui->Books->clear();
    ExportBookListBtn(false);
    if(ui->GenreList->selectedItems().count()==0)
        return;
    QTreeWidgetItem* cur_item=ui->GenreList->selectedItems()[0];
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
    QSettings settings;
    if(settings.value("store_position",true).toBool()){
        settings.setValue("current_genre_id",idCurrentGenre_);
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

    QSettings settings;
    idCurrentSerial_= idSerial;
    if(settings.value("store_position",true).toBool()){
        settings.setValue("current_serial_id",idSerial);
    }
}

void MainWindow::SelectAuthor()
{
    ExportBookListBtn(false);
    if(ui->AuthorList->selectedItems().count()==0)
        return;
    QListWidgetItem* cur_item=ui->AuthorList->selectedItems()[0];

    QSettings settings;

    idCurrentAuthor_ = cur_item->data(Qt::UserRole).toUInt();

    QList<uint> booksId = mLibs[idCurrentLib].mAuthorBooksLink.values(idCurrentAuthor_);
    FillListBooks(booksId,idCurrentAuthor_);
    if(settings.value("store_position",true).toBool()){
        settings.setValue("current_author_id",idCurrentAuthor_);
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
    if(ui->splitter->sizes().at(1)>0)
    {
        QBuffer outbuff;
        QBuffer infobuff;
        QDateTime book_date;
        QFileInfo fi=mLibs[idCurrentLib].getBookFile(outbuff,infobuff,idBook,false,&book_date);
        if(book.sAnnotation.isEmpty() && book.sImg.isEmpty())
            mLibs[idCurrentLib].loadAnnotation(idBook);
        if(fi.fileName().isEmpty())
        {
            QString file;
            QString LibPath=mLibs[idCurrentLib].path;
            if(book.sArchive.trimmed().isEmpty() )
            {
                file=QString("%1/%2.%3").arg(LibPath,book.sFile,book.sFormat);
            }
            else
            {
                file=LibPath+"/"+book.sArchive.trimmed().replace(".inp",".zip");
            }
            file=file.replace("\\","/");
        }

        QString seria;
        QTreeWidgetItem *parent=item->parent();
        if(parent->type() == ITEM_TYPE_SERIA) //если это серия
        {
            seria=QString("<a href=seria_%3%1>%2</a>").arg(QString::number(/*-*/parent->data(0,Qt::UserRole).toLongLong()),parent->text(0),parent->text(0).left(1).toUpper());
        }

        QString sAuthors;
        foreach (auto idAuthor, book.listIdAuthors)
        {
            QString sAuthor = mLibs[idCurrentLib].mAuthors[idAuthor].getName();
            sAuthors+=(sAuthors.isEmpty()?"":"; ")+QString("<a href='author_%3%1'>%2</a>").arg(QString::number(idAuthor),sAuthor.replace(","," "),sAuthor.left(1));
        }
        QString sGenres;
        foreach (auto idGenre, book.listIdGenres)
        {
            QString sGenre = mGenre[idGenre].sName;
            sGenres+=(sGenres.isEmpty()?"":"; ")+QString("<a href='genre_%3%1'>%2</a>").arg(QString::number(idGenre),sGenre,sGenre.left(1));
        }
        QFile file_html(":/preview.html");
        file_html.open(QIODevice::ReadOnly);
        QString content(file_html.readAll());
        qint64 size=0;
        QFileInfo arh;
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
        }

        QString img_width="220";
        content.replace("#annotation#",book.sAnnotation).
                replace("#title#",book.sName).
                replace("#width#",(book.sImg.isEmpty()?"0":img_width)).
                replace("#author#",sAuthors).
                replace("#genre#",sGenres).
                replace("#series#",seria).
                replace("#file_path#",arh.filePath()).
                replace("#file_size#",sizeToString(size)/*QString::number(size)+(mem_i>0?"."+QString::number((rest*10+5)/1024):"")+" "+mem[mem_i]*/).
                replace("#file_data#",book_date.toString("dd.MM.yyyy hh:mm:ss")).
                replace("#file_name#",fi.fileName()).
                replace("#image#",book.sImg).
                replace("#file_info#",settings->value("show_fileinfo",true).toBool()?"block":"none");
        dynamic_cast<WebPage*>(ui->Review->page())->setHtml(content);
    }
}

void MainWindow::UpdateBooks()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    SLib &currentLib = mLibs[idCurrentLib];

    ui->language->blockSignals(true);
    ui->findLanguage->blockSignals(true);
    ui->language->clear();
    ui->language->addItem("*",-1);
    ui->language->setCurrentIndex(0);
    ui->findLanguage->clear();
    ui->findLanguage->addItem("*",-1);
    ui->findLanguage->setCurrentIndex(0);

    QSettings settings;
    QString sCurrentLanguage=settings.value("BookLanguage","*").toString();
    for(int iLang=0;iLang<currentLib.vLaguages.size();iLang++){
        QString sLanguage = currentLib.vLaguages[iLang].toUpper();
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
    settings.setValue("BookLanguage",ui->language->currentText());
    ui->language->blockSignals(false);
    ui->findLanguage->blockSignals(false);
    QApplication::restoreOverrideCursor();
}

void MainWindow::ManageLibrary()
{
    SaveLibPosition();
    AddLibrary al(this);
    al.exec();
    if(al.bLibChanged){
        loadLibrary(idCurrentLib);
        UpdateTags();
        UpdateBooks();
        searchCanged(ui->searchString->text());
        setWindowTitle(AppName+(idCurrentLib<0||mLibs[idCurrentLib].name.isEmpty()?"":" - "+mLibs[idCurrentLib].name));
        FillLibrariesMenu();
    }
}

void MainWindow::btnSearch()
{
    QToolButton *button = qobject_cast<QToolButton*>(sender());
    ui->searchString->setText(button->text());
    searchCanged(ui->searchString->text());
}

void MainWindow::About()
{
    AboutDialog* dlg=new AboutDialog(this);
    dlg->exec();
    delete dlg;
}

void MainWindow::DoSearch()
{

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
    ui->searchString->setClearButtonEnabled(ui->searchString->text().length()>1);
}

void MainWindow::searchClear()
{
    ui->searchString->setText(ui->searchString->text().left(1));
    searchCanged(ui->searchString->text());
}


void MainWindow::HelpDlg()
{
    if(pHelpDlg==nullptr)
        pHelpDlg=new HelpDialog();
    pHelpDlg->show();
}

void MainWindow::ContextMenu(QPoint point)
{
    if(QObject::sender()==qobject_cast<QObject*>(ui->Books) && !ui->Books->itemAt(point))
        return;
    if(QObject::sender()==qobject_cast<QObject*>(ui->AuthorList) && !ui->AuthorList->itemAt(point))
        return;
    if(QObject::sender()==qobject_cast<QObject*>(ui->SeriaList) && !ui->SeriaList->itemAt(point))
        return;
    QMenu menu;
    current_list_for_tag=QObject::sender();
    if(QObject::sender()==qobject_cast<QObject*>(ui->Books))
    {
        QMenu *save=menu.addMenu(tr("Save as"));
        foreach (QAction* i, ui->btnExport->menu()->actions())
        {
            QAction *action=new QAction(i->text(), this);
            action->setData(i->data().toInt());
            connect(action, &QAction::triggered, this, &MainWindow::ExportAction);
            save->addAction(action);
        }
    }
    if(menu.actions().count()>0)
        menu.addSeparator();
    if(bUseTag_)
        menu.addActions(TagMenu.actions());
    if(menu.actions().count()>0)
        menu.exec(QCursor::pos());
}

void MainWindow::HeaderContextMenu(QPoint /*point*/)
{
    QMenu menu;
    QAction *action;

    action=new QAction(tr("Name"), this);
    action->setCheckable(true);
    action->setChecked(!ui->Books->isColumnHidden(0));
    connect(action, &QAction::triggered,this, [action, this]{ui->Books->setColumnHidden(0,!action->isChecked());});
    menu.addAction(action);

    action=new QAction(tr("No."), this);
    action->setCheckable(true);
    action->setChecked(!ui->Books->isColumnHidden(1));
    connect(action, &QAction::triggered, this, [action, this]{ShowHeaderCoulmn(1,"ShowName",!action->isChecked());});
    menu.addAction(action);

    action=new QAction(tr("Size"), this);
    action->setCheckable(true);
    action->setChecked(!ui->Books->isColumnHidden(2));
    connect(action, &QAction::triggered, this, [action, this]{ShowHeaderCoulmn(2,"ShowSize",!action->isChecked());});
    menu.addAction(action);

    action=new QAction(tr("Mark"), this);
    action->setCheckable(true);
    action->setChecked(!ui->Books->isColumnHidden(3));
    connect(action, &QAction::triggered, this, [action, this]{ShowHeaderCoulmn(3,"ShowMark",!action->isChecked());});
    menu.addAction(action);

    action=new QAction(tr("Import date"), this);
    action->setCheckable(true);
    action->setChecked(!ui->Books->isColumnHidden(4));
    connect(action, &QAction::triggered, this, [action, this]{ShowHeaderCoulmn(4,"ShowImportDate",!action->isChecked());});
    menu.addAction(action);

    action=new QAction(tr("Genre"), this);
    action->setCheckable(true);
    action->setChecked(!ui->Books->isColumnHidden(5));
    connect(action, &QAction::triggered, this, [action, this]{ShowHeaderCoulmn(5,"ShowGenre",!action->isChecked());});
    menu.addAction(action);

    action=new QAction(tr("Language"), this);
    action->setCheckable(true);
    action->setChecked(!ui->Books->isColumnHidden(6));
    connect(action, &QAction::triggered, this, [action, this]{ShowHeaderCoulmn(6,"ShowLanguage",!action->isChecked());});
    menu.addAction(action);

    menu.exec(QCursor::pos());
}

void MainWindow::ShowHeaderCoulmn(int nColumn,QString sSetting,bool bHide)
{
    ui->Books->setColumnHidden(nColumn,bHide);
    QSettings settings;
    settings.beginGroup("Columns");
    settings.setValue(sSetting,!bHide);
}

void MainWindow::MoveToSeria(qlonglong id,QString FirstLetter)
{
    ui->searchString->setText(FirstLetter);
    ui->tabWidget->setCurrentIndex(TabSeries);
    ui->SeriaList->clearSelection();
    for (int i=0;i<ui->SeriaList->count();i++)
    {
        if(ui->SeriaList->item(i)->data(Qt::UserRole).toLongLong()==id)
        {
            ui->SeriaList->item(i)->setSelected(true);
            ui->SeriaList->scrollToItem(ui->SeriaList->item(i));
            SelectSeria();
            return;
        }
    }
}

void MainWindow::MoveToGenre(qlonglong id)
{
    ui->tabWidget->setCurrentIndex(TabGenres);
    ui->GenreList->clearSelection();
    for (int i=0;i<ui->GenreList->topLevelItemCount();i++)
    {
        for (int j=0;j<ui->GenreList->topLevelItem(i)->childCount();j++)
        {
            if(ui->GenreList->topLevelItem(i)->child(j)->data(0,Qt::UserRole).toLongLong()==id)
            {
                ui->GenreList->topLevelItem(i)->child(j)->setSelected(true);
                ui->GenreList->scrollToItem(ui->GenreList->topLevelItem(i)->child(j));
                SelectGenre();
                return;
            }
        }
    }
}

void MainWindow::MoveToAuthor(qlonglong id, QString FirstLetter)
{
    ui->searchString->setText(/*id<0?Item->text(0).left(1).toUpper():*/FirstLetter);
    ui->tabWidget->setCurrentIndex(TabAuthors);
    searchCanged(FirstLetter);
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
        lib_menu->insertAction(nullptr,action);
        connect(action, &QAction::triggered, this, &MainWindow::SelectLibrary);
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
        if(sSearch == "*" || (sSearch=="#" && !i->getName().left(1).contains(QRegExp("[A-Za-zа-яА-ЯЁё]"))) || i->getName().startsWith(sSearch,Qt::CaseInsensitive)){
            QList<uint> booksId = currentLib.mAuthorBooksLink.values(i.key());
            int count =0;
            foreach( uint idBook, booksId) {
                SBook &book = currentLib.mBooks[idBook];
                if(IsBookInList(book))
                {
                    count++;
                }
            }
            if(count>0){
                item=new QListWidgetItem(QString("%1 (%2)").arg(i->getName()).arg(count));
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
    const bool wasBlocked = ui->GenreList->blockSignals(true);
    ui->GenreList->clear();
    ui->s_genre->clear();
    ui->s_genre->addItem("*",0);
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
            item=new QTreeWidgetItem(ui->GenreList);
            item->setFont(0,bold_font);
            item->setText(0,iGenre->sName);
            item->setData(0,Qt::UserRole,iGenre.key());
            item->setExpanded(false);
            mTopGenresItem[iGenre.key()] = item;
            ui->s_genre->addItem(iGenre->sName,iGenre.key());
        }else{
            if(mCounts.contains(iGenre.key())){
                if(!mTopGenresItem.contains(iGenre->idParrentGenre)){
                    QTreeWidgetItem *itemTop = new QTreeWidgetItem(ui->GenreList);
                    itemTop->setFont(0,bold_font);
                    itemTop->setText(0,mGenre[iGenre->idParrentGenre].sName);
                    itemTop->setData(0,Qt::UserRole,iGenre->idParrentGenre);
                    itemTop->setExpanded(false);
                    mTopGenresItem[iGenre->idParrentGenre] = itemTop;
                }
                item=new QTreeWidgetItem(mTopGenresItem[iGenre->idParrentGenre]);
                item->setText(0,QString("%1 (%2)").arg(iGenre->sName).arg(mCounts[iGenre.key()]));
                item->setData(0,Qt::UserRole,iGenre.key());
                ui->s_genre->addItem("   "+iGenre->sName,iGenre.key());
                if(iGenre.key()==idCurrentGenre_)
                {
                    item->setSelected(true);
                    ui->GenreList->scrollToItem(item);
                }
            }
        }
        ++iGenre;
    }

    ui->s_genre->model()->sort(0);

    ui->GenreList->blockSignals(wasBlocked);
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
            SelectGenre();
        break;

    }
}

void MainWindow::FillListBooks(QList<uint> listBook,uint idCurrentAuthor)
{
    qint64 t_start = QDateTime::currentMSecsSinceEpoch();
    QFont bold_font(ui->Books->font());
    bold_font.setBold(true);
    TreeBookItem* ScrollItem=nullptr;

    TreeBookItem* item_seria=nullptr;
    TreeBookItem* item_book;
    TreeBookItem* item_author;
    QMap<uint,TreeBookItem*> mAuthors;

    QMultiMap<uint,TreeBookItem*> mSerias;

    const bool wasBlocked = ui->Books->blockSignals(true);
    ui->Books->clear();

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
                item_author->setText(0,mLibs[idCurrentLib].mAuthors[idAuthor].getName());
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
                auto iSerial = mSerias.constFind(idSerial);
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
                item_book->setText(2,sizeToString(book.nSize));
            item_book->setTextAlignment(2, Qt::AlignRight);

            QPixmap pix(":/icons/img/icons/stars/"+QString::number(book.nStars).trimmed()+QString("star%1.png").arg(app->devicePixelRatio()>=2?"@2x":""));
            pix.setDevicePixelRatio(app->devicePixelRatio());
            item_book->setData(3,Qt::DecorationRole,pix);

            item_book->setText(4,book.date.toString("dd.MM.yyyy"));
            item_book->setTextAlignment(4, Qt::AlignCenter);

            item_book->setText(5,mGenre[book.listIdGenres.first()].sName);
            item_book->setTextAlignment(5, Qt::AlignLeft);

            item_book->setText(6,mLibs[idCurrentLib].vLaguages[book.idLanguage]);
            item_book->setTextAlignment(6, Qt::AlignCenter);

            if(book.bDeleted)
            {
                QBrush brush(QColor::fromRgb(96,96,96));
                for (int i = 0; i != 7; ++i)
                     item_book->setForeground(i, brush);
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
    if(pDropForm==nullptr)
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
    QSettings settings;
    int defaultID=-1;
    if(ui->btnExport->defaultAction())
        defaultID=ui->btnExport->defaultAction()->data().toInt();
    else
        defaultID=settings.value("DefaultExport",-1).toInt();
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
    ui->btnExport->setDefaultAction(nullptr);
    int count=settings.beginReadArray("export");
    for(int i=0;i<count;i++)
    {
        settings.setArrayIndex(i);
        QAction *action=new QAction(settings.value("ExportName").toString(),this);
        action->setData(i);
        menu->addAction(action);
        if(settings.value("Default").toBool() || (i==defaultID && !ui->btnExport->defaultAction()))
        {
            ui->btnExport->setDefaultAction(action);
        }
    }
    settings.endArray();
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
        ui->btnExport->setDefaultAction(menu->actions().constFirst());
    }
    foreach (QAction *action, menu->actions())
    {
        connect(action, &QAction::triggered, this, &MainWindow::ExportAction);
    }
    QFont font(ui->btnExport->defaultAction()->font());
    font.setBold(true);
    ui->btnExport->defaultAction()->setFont(font);
    QPalette palette = QApplication::style()->standardPalette();
    bool darkTheme = palette.color(QPalette::Window).lightness()<127;
    QString sIconsPath = QStringLiteral(":/img/icons/") + (darkTheme ?"dark/" :"light/");
    ui->btnExport->setIcon(QIcon::fromTheme(QStringLiteral("tablet"),QIcon(sIconsPath + QStringLiteral("streamline.svg"))));
    ui->btnExport->setEnabled(ui->Books->selectedItems().count()>0);
}

void MainWindow::ExportAction()
{
    int id=qobject_cast<QAction*>(sender())->data().toInt();
    QSettings settings;
    int count=settings.beginReadArray("export");
    if(count>1 && ui->btnExport->defaultAction())
    {
        settings.setArrayIndex(ui->btnExport->defaultAction()->data().toInt());
        if(!settings.value("Default").toBool())
        {
            ui->btnExport->setDefaultAction(qobject_cast<QAction*>(sender()));
            QList<QAction*> actions=ui->btnExport->menu()->actions();
            foreach (QAction* action, actions)
            {
                QFont font(action->font());
                font.setBold(action==ui->btnExport->defaultAction());
                action->setFont(font);
            }
            QPalette palette = QApplication::style()->standardPalette();
            bool darkTheme = palette.color(QPalette::Window).lightness()<127;
            QString sIconsPath = QStringLiteral(":/img/icons/") + (darkTheme ?QStringLiteral("dark/") :QStringLiteral("light/"));
            ui->btnExport->setIcon(QIcon::fromTheme(QStringLiteral("tablet"),QIcon(sIconsPath + QStringLiteral("streamline.svg"))));
        }
    }
    settings.endArray();
    SendType type=SetCurrentExportSettings(id);
   if(type==ST_Device)
       SendToDevice();
   else
       SendMail();
}

void MainWindow::on_actionSwitch_to_convert_mode_triggered()
{
    QSettings settings;
    if(mode==MODE_LIBRARY)
    {
        settings.setValue("MainWnd/geometry", saveGeometry());
        settings.setValue("MainWnd/windowState", saveState());
        settings.setValue("MainWnd/tab/geometry",ui->tabWidget->saveGeometry());
        settings.setValue("MainWnd/tab/geometry",ui->splitter->saveState());
        settings.setValue("MainWnd/books/geometry",ui->splitter_2->saveState());
        settings.setValue("MainWnd/books_head/geometry",ui->Books->header()->saveState());
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
    if(settings.contains("MainWndConvertMode/geometry"))
        restoreGeometry(settings.value("MainWndConvertMode/geometry").toByteArray());

    settings.setValue("ApplicationMode", mode);
    if(pDropForm!=nullptr)
    {
        pDropForm->hide();
        DeleteDropForm();
    }
    ShowDropForm();
}

void MainWindow::on_actionSwitch_to_library_mode_triggered()
{
    QSettings settings;
    if(mode==MODE_CONVERTER)
    {
        settings.setValue("MainWndConvertMode/geometry", saveGeometry());
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
    if(settings.contains("MainWnd/geometry"))
        restoreGeometry(settings.value("MainWnd/geometry").toByteArray());
    if(settings.contains("MainWnd/windowState"))
        restoreState(settings.value("MainWnd/windowState").toByteArray());
    if(settings.contains("MainWnd/tab/geometry"))
        ui->splitter->restoreState(settings.value("MainWnd/tab/geometry").toByteArray());
    //on_splitter_splitterMoved(0,0);
    if(settings.contains("MainWnd/books/geometry"))
        ui->splitter_2->restoreState(settings.value("MainWnd/books/geometry").toByteArray());
    settings.setValue("ApplicationMode", mode);
}

void MainWindow::onLanguageCurrentIndexChanged(int index)
{
    QString sLanguage = ui->language->itemText(index);
    QSettings settings;
    settings.setValue(QStringLiteral("BookLanguage"),sLanguage);
    idCurrentLanguage_ = ui->language->itemData(index).toInt();

    FillSerials();
    FillAuthors();
    FillGenres();
    FillListBooks();
}

void MainWindow::on_btnSwitchToLib_clicked()
{
    on_actionSwitch_to_library_mode_triggered();
}

void MainWindow::on_btnPreference_clicked()
{
    Settings();
}

void MainWindow::onTabWidgetChanged(int index)
{
    switch(index){
    case TabAuthors: //Авторы
        ui->SearchFrame->setEnabled(true);
        ui->frame_3->setEnabled(true);
        ui->language->setEnabled(true);
        SelectAuthor();
        break;
    case TabSeries: //Серии
        ui->SearchFrame->setEnabled(true);
        ui->frame_3->setEnabled(true);
        ui->language->setEnabled(true);
        SelectSeria();
        break;
    case TabGenres: //Жанры
        ui->SearchFrame->setEnabled(false);
        ui->frame_3->setEnabled(false);
        ui->language->setEnabled(true);
        SelectGenre();
        break;
    case TabSearch: //Поиск
        ui->SearchFrame->setEnabled(false);
        ui->frame_3->setEnabled(false);
        ui->language->setEnabled(false);
        ui->Books->clear();
        ui->find_books->setText(QStringLiteral("0"));
        ExportBookListBtn(false);
        break;
    }
}

void MainWindow::resizeEvent(QResizeEvent */*e*/)
{

    if(pDropForm!=nullptr)
    {
        if(pDropForm->isVisible())
        {
            const QSignalBlocker blocker(this);
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

void MainWindow::ChangingTrayIcon(int index,int color)
{
    if(CMDparser.isSet("tray"))
        index=2;
    QSettings settings;
    if(index<0)
    {
        index=settings.value("tray_icon",0).toInt();
    }
    if(color<0)
    {
        color=settings.value("tray_color",0).toInt();
    }
    if(index==0)
    {
        if(trIcon)
        {
            trIcon->hide();
            trIcon->deleteLater();
        }
        trIcon=nullptr;
    }
    else
    {
        if(!trIcon)
        {
            trIcon = new QSystemTrayIcon(this);  //инициализируем объект
            connect(trIcon, &QSystemTrayIcon::activated, this, &MainWindow::TrayMenuAction);
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
    QSettings settings;
#ifdef Q_OS_WIN
    if(this->isVisible())
    {
        this->setWindowState(this->windowState()|Qt::WindowMinimized);
        if(settings.value("tray_icon",0).toInt()!=0)
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
            if(settings.value("tray_icon",0).toInt()!=0)
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
            if(settings.value("tray_icon",0).toInt()!=0)
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

