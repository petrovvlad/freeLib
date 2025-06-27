#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <vector>
#include <ranges>
#include <QtSql>
#include <QSplashScreen>
#include <QButtonGroup>
#include <QMessageBox>
#include <QDesktopServices>
#include <QTextBrowser>
#include <QCloseEvent>
#include <QWidgetAction>
#include <QtConcurrent>
#include <QActionGroup>

#include "librariesdlg.h"
#include "settingsdlg.h"
#include "exportdlg.h"
#include "aboutdialog.h"
#include "tagdialog.h"
#include "bookeditdlg.h"
#include "treebookitem.h"
#include "genresortfilterproxymodel.h"
#include "library.h"
#include "starsdelegate.h"
#include "statisticsdialog.h"
#include "utilites.h"
#include "bookfile.h"

using namespace std::chrono_literals;
namespace views = std::ranges::views;

#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
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
#endif



void addShortcutToToolTip(QToolButton *btn, const QString &sShortCut)
{
    if(!sShortCut.isEmpty()){
        QString sToolTip = btn->toolTip();
        QColor shortcutTextColor = QApplication::palette().color(QPalette::ToolTipText);
        QString sShortCutTextColorName;
        if (shortcutTextColor.value() == 0) {
            sShortCutTextColorName = u"gray"_s;  // special handling for black because lighter() does not work there [QTBUG-9343].
        } else {
            int factor = (shortcutTextColor.value() < 128) ? 150 : 50;
            sShortCutTextColorName = shortcutTextColor.lighter(factor).name();
        }
        btn->setToolTip(u"<p style='white-space:pre'>%1&nbsp;&nbsp;<code style='color:%2; font-size:small'>%3</code></p>"_s
                            .arg(sToolTip, sShortCutTextColorName, sShortCut));
    }
}

void addShortcutToToolTip(QToolButton *btn, QAction *action)
{
    addShortcutToToolTip(btn, action->shortcut().toString(QKeySequence::NativeText));

}

void addShortcutToToolTip(QToolButton *btn)
{
    addShortcutToToolTip(btn, btn->shortcut().toString(QKeySequence::NativeText));
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
#ifdef USE_KStatusNotifier
    statusNotifierItem_ = nullptr;
#else
    pTrayIcon_ = nullptr;
#endif
    pTrayMenu_ = nullptr;
    pHideAction_ = nullptr;
    pShowAction_ = nullptr;
    error_quit = false;

    auto settings = GetSettings();

    ui->setupUi(this);
    ui->btnEdit->setVisible(false);
    bTreeView_ = true;
    bCollapsed_ = false;
    bTreeView_ = settings->value(u"TreeView"_s, true).toBool();
    bCollapsed_ = settings->value(u"collapseAll"_s, false).toBool();
    ui->btnTreeView->setChecked(bTreeView_);
    ui->btnListView->setChecked(!bTreeView_);
    ui->btnCollapseAll->setVisible(bTreeView_);
    ui->btnExpandAll->setVisible(bTreeView_);

    if(!g::options.bUseSytemFonts){
        QFont font(QGuiApplication::font());
        font.setFamily(g::options.sListFontFamaly);
        font.setPointSize(g::options.nListFontSize);
        onUpdateListFont(font);
        font.setFamily(g::options.sAnnotationFontFamaly);
        font.setPointSize(g::options.nAnnotationFontSize);
        onUpdateAnnotationFont(font);
    }

    ui->tabWidget->setCurrentIndex(0);
    ui->Books->setColumnWidth(0,400);
    ui->Books->setColumnWidth(3,50);
    ui->Books->setColumnWidth(4,100);
    ui->Books->setColumnWidth(5,90);
    ui->Books->setColumnWidth(6,120);
    ui->Books->setColumnWidth(7,250);
    ui->Books->setColumnWidth(8,50);
    ui->Books->setColumnWidth(9,50);

    pLibGroup_ = new QActionGroup(this);
    pCover = new CoverLabel(this);
    ui->horizontalLayout_3->addWidget(pCover);

    if(settings->contains(u"MainWnd/geometry"_s))
        restoreGeometry(settings->value(u"MainWnd/geometry"_s).toByteArray());
    if(settings->contains(u"MainWnd/windowState"_s))
        restoreState(settings->value(u"MainWnd/windowState"_s).toByteArray());
    if(settings->contains(u"MainWnd/VSplitterSizes"_s))
        ui->splitterV->restoreState(settings->value(u"MainWnd/VSplitterSizes"_s).toByteArray());
    if(settings->contains(u"MainWnd/HSplitterSizes"_s))
        ui->splitterH->restoreState(settings->value(u"MainWnd/HSplitterSizes"_s).toByteArray());

    settings->beginGroup(u"Columns"_s);
    QVariant varHeaders = settings->value(u"headersTree"_s);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if(varHeaders.type() == QVariant::ByteArray)
#else
    if(varHeaders.metaType().id() == QMetaType::QByteArray)
#endif
        aHeadersTree_ = varHeaders.toByteArray();
    else{
        ui->Books->setColumnHidden(1, true);
        ui->Books->setColumnHidden(2, true);
        ui->Books->setColumnHidden(9, true);
    }
    varHeaders = settings->value(u"headersList"_s);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if(varHeaders.type() == QVariant::ByteArray)
#else
    if(varHeaders.metaType().id() == QMetaType::QByteArray)
#endif
        aHeadersList_ = varHeaders.toByteArray();
    settings->endGroup();
    if(bTreeView_)
        ui->Books->header()->restoreState(aHeadersTree_);
    else
        ui->Books->header()->restoreState(aHeadersList_);
    ui->Books->header()->setSortIndicatorShown(true);

    ui->btnOpenBook->setIcon(themedIcon(u"book"_s));
    ui->btnCheck->setIcon(themedIcon(u"checkbox"_s));
    ui->btnLibrary->setIcon(themedIcon(u"library"_s));
    ui->btnOption->setIcon(themedIcon(u"settings-configure"_s));
    ui->btnTreeView->setIcon(themedIcon(u"view-list-tree"_s));
    ui->btnListView->setIcon(themedIcon(u"view-list-details"_s));
    ui->btnCollapseAll->setIcon(themedIcon(u"collapse-all"_s));
    ui->btnExpandAll->setIcon(themedIcon(u"expand-all"_s));

    GenreSortFilterProxyModel *MyProxySortModel = new GenreSortFilterProxyModel(ui->s_genre);
    MyProxySortModel->setSourceModel(ui->s_genre->model());
    ui->s_genre->model()->setParent(MyProxySortModel);
    ui->s_genre->setModel(MyProxySortModel);
    MyProxySortModel->setDynamicSortFilter(false);

    StarsDelegate* delegate = new StarsDelegate(this);
    delegate->setAlinment(Qt::AlignCenter);
    ui->Books->setItemDelegateForColumn(5, delegate);

    ui->comboMinimalRating->setCurrentIndex(0);
    idCurrentLanguage_ = -1;
    int nCurrentTab;

    QString sFilter;
    if(g::options.bStorePosition)
    {
        idCurrentAuthor_= settings->value(u"current_author_id"_s, 0).toUInt();
        idCurrentSerial_ = settings->value(u"current_serial_id"_s, 0).toUInt();
        idCurrentBook_ = settings->value(u"current_book_id"_s, 0).toUInt();
        idCurrentGenre_ = settings->value(u"current_genre_id"_s, 0).toUInt();
        nCurrentTab = settings->value(u"current_tab"_s, 0).toInt();
        sFilter = settings->value(u"filter_set"_s).toString();

        QString sTitle = settings->value(u"search.title"_s).toString();
        ui->s_name->setText(sTitle);
        QString sAuthor = settings->value(u"search.author"_s).toString();
        ui->s_author->setText(sAuthor);
        QString sSequence = settings->value(u"search.sequence"_s).toString();
        ui->s_seria->setText(sSequence);
        QString sFile = settings->value(u"search.file"_s).toString();
        ui->s_FileName->setText(sFile);
        int nMaxCount = settings->value(u"search.max.books"_s, 1000).toInt();
        ui->maxBooks->setValue(nMaxCount);
        uint nMinimalRating = settings->value(u"search.min.rating"_s).toUInt();
        ui->comboMinimalRating->setCurrentIndex(nMinimalRating);
    }
    else
    {
        idCurrentAuthor_ = 0;
        idCurrentSerial_ = 0;
        idCurrentBook_ = 0;
        idCurrentGenre_ = 0;
        nCurrentTab = TabAuthors;
    }

#ifdef USE_HTTSERVER
    if(g::options.bOpdsEnable){
        pOpds_ = std::unique_ptr<opds_server>( new opds_server(this) );
        pOpds_->server_run();
    }
#endif

    UpdateTags();
    loadGenres();
    loadLibrary(g::idCurrentLib);
    fillLanguages();

    FillGenres();
    ui->date_from->setDate( g::libs[g::idCurrentLib].earliestDate_ );

    connect(ui->searchAuthor, &QLineEdit::textChanged, this, &MainWindow::onSerachAuthorsChanded);
    connect(ui->searchSeries, &QLineEdit::textChanged, this, &MainWindow::onSerachSeriesChanded);
    connect(ui->actionAddLibrary, &QAction::triggered, this, &MainWindow::ManageLibrary);
    connect(ui->actionStatistics, &QAction::triggered, this, &MainWindow::onStatistics);
    connect(ui->actionAddBooks, &QAction::triggered, this, &MainWindow::onAddBooks);
    connect(ui->btnLibrary, &QAbstractButton::clicked, this, &MainWindow::ManageLibrary);
    connect(ui->btnOpenBook, &QAbstractButton::clicked, this, &MainWindow::BookDblClick);
    connect(ui->btnOption, &QAbstractButton::clicked, this, &MainWindow::Settings);
    connect(ui->actionPreference, &QAction::triggered, this, &MainWindow::Settings);
    connect(ui->actionCheck_uncheck, &QAction::triggered, this, &MainWindow::CheckBooks);
    connect(ui->btnCheck, &QAbstractButton::clicked, this, &MainWindow::CheckBooks);
    connect(ui->btnEdit, &QAbstractButton::clicked, this, &MainWindow::EditBooks);
    connect(ui->btnTreeView, &QAbstractButton::clicked, this, &MainWindow::onTreeView);
    connect(ui->btnListView, &QAbstractButton::clicked, this, &MainWindow::onListView);
    connect(ui->btnCollapseAll, &QAbstractButton::clicked, this, &MainWindow::onCollapseAll);
    connect(ui->btnExpandAll, &QAbstractButton::clicked, this, &MainWindow::onExpandAll);
    connect(ui->actionExit, &QAction::triggered, qApp, &QApplication::quit);
    #ifdef Q_OS_MACX
        ui->actionExit->setShortcut(QKeySequence(Qt::CTRL, Qt::Key_Q));
    #endif
    #ifdef Q_OS_LINUX
        setWindowIcon(QIcon(u":/library.png"_s));
    #endif
    #ifdef Q_OS_WIN
        ui->actionExit->setShortcut(QKeySequence(Qt::ALT, Qt::Key_F4));
    #endif
    #ifdef Q_OS_WIN32
        ui->actionExit->setShortcut(QKeySequence(Qt::ALT, Qt::Key_F4));
    #endif
    connect(ui->AuthorList, &QListWidget::itemSelectionChanged, this, &MainWindow::SelectAuthor);
    connect(ui->Books, &QTreeWidget::itemSelectionChanged, this, &MainWindow::selectBook);
    connect(ui->Books, &QTreeWidget::itemDoubleClicked, this, &MainWindow::BookDblClick);
    connect(ui->GenreList, &QTreeWidget::itemSelectionChanged, this, &MainWindow::SelectGenre);
    connect(ui->SeriaList, &QListWidget::itemSelectionChanged, this, &MainWindow::SelectSeria);
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabWidgetChanged);
    connect(ui->do_search, &QAbstractButton::clicked, this, &MainWindow::onStartSearch);
    connect(ui->buttonClear, &QAbstractButton::clicked, this, &MainWindow::onClearSearch);
    connect(ui->s_author, &QLineEdit::returnPressed, this, &MainWindow::onStartSearch);
    connect(ui->s_seria, &QLineEdit::returnPressed, this, &MainWindow::onStartSearch);
    connect(ui->s_name, &QLineEdit::returnPressed, this, &MainWindow::onStartSearch);
    connect(ui->s_FileName, &QLineEdit::returnPressed, this, &MainWindow::onStartSearch);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::About);
    connect(ui->Books, &QTreeWidget::itemChanged, this, &MainWindow::onItemChanged);
    connect(ui->language, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &MainWindow::onLanguageFilterChanged);
    connect(ui->Review, &QTextBrowser::anchorClicked, this, &MainWindow::ReviewLink );

    FillAlphabet(g::options.sAlphabetName);
    ExportBookListBtn(false);
    updateTitle();

    ui->tabWidget->setCurrentIndex(nCurrentTab);
    switch (nCurrentTab) {
    case TabAuthors:
        ui->searchAuthor->setText(sFilter);
        ui->searchAuthor->setFocus();
        if(sFilter.trimmed().isEmpty())
            FirstButton->click();
        SelectAuthor();
        break;
    case TabSeries:
        ui->searchSeries->setText(sFilter);
        ui->searchSeries->setFocus();
        SelectSeria();
        break;
    }

    ui->date_to->setDate(QDate::currentDate());

    pHelpDlg = nullptr;
    connect(ui->actionHelp, &QAction::triggered, this, &MainWindow::HelpDlg);
    ui->Books->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->Books, &QWidget::customContextMenuRequested, this, &MainWindow::ContextMenu);
    ui->AuthorList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->AuthorList, &QWidget::customContextMenuRequested, this, &MainWindow::ContextMenu);
    ui->SeriaList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->SeriaList, &QWidget::customContextMenuRequested, this, &MainWindow::ContextMenu);
    connect(ui->TagFilter, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &MainWindow::onTagFilterChanged);
    ui->Books->header()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->Books->header(), &QWidget::customContextMenuRequested, this, &MainWindow::HeaderContextMenu);
    FillLibrariesMenu();
    UpdateExportMenu();

    changingTrayIcon(g::options.nIconTray, g::options.bTrayColor);
#ifdef Q_OS_OSX
    connect(MyPrivate::instance(), SIGNAL(dockClicked()), SLOT(dockClicked()));
#endif
#ifndef USE_KStatusNotifier
    connect(ui->actionMinimize_window, &QAction::triggered, this, &MainWindow::MinimizeWindow);
#endif

    addShortcutToToolTip(ui->btnCheck, ui->actionCheck_uncheck);
    addShortcutToToolTip(ui->btnLibrary, ui->actionAddLibrary);
    addShortcutToToolTip(ui->btnCollapseAll);
    addShortcutToToolTip(ui->btnExpandAll);
    addShortcutToToolTip(ui->btnOption, ui->actionPreference);
    addShortcutToToolTip(ui->btnListView);
    addShortcutToToolTip(ui->btnTreeView);
}

void MainWindow::UpdateTags()
{
    if(!QSqlDatabase::database(u"libdb"_s, false).isOpen())
        return;
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    auto settings = GetSettings();
    bool darkTheme = palette().color(QPalette::Window).lightness() < 127;

    QButtonGroup *group = new QButtonGroup(this);
    group->setExclusive(true);

    iconsTags_.clear();
    iconsTags_[0] = themedIcon(u"multitags"_s);

    std::unordered_map<uint, QIcon> icons;
    QSqlQuery query(QSqlDatabase::database(u"libdb"_s));
    query.exec(u"SELECT id, light_theme FROM icon"_s);
    while(query.next())
    {
        uint idIcon = query.value(0).toUInt();
        QByteArray baLightIcon = query.value(1).toByteArray();
        QSvgRenderer render(baLightIcon);
        icons[idIcon] = renderSvg(render, darkTheme);
    }

    const bool wasBlocked = ui->TagFilter->blockSignals(true);

    query.exec(u"SELECT id,name,id_icon FROM tag"_s);
    //                                0  1    2
    ui->TagFilter->clear();
    int con = 1;
    ui->TagFilter->addItem(u"*"_s, 0);
    TagMenu.clear();
    ui->TagFilter->setVisible(g::options.bUseTag);
    ui->tag_label->setVisible(g::options.bUseTag);

    while(query.next())
    {
        uint idTag = query.value(0).toUInt();
        QString sTagName = query.value(1).toString().trimmed();
        bool bLatin1 = std::ranges::none_of(sTagName, [](QChar c) { return c.unicode() > 127; });
        if(bLatin1)
            sTagName = TagDialog::tr(sTagName.toLatin1().data());
        ui->TagFilter->addItem(sTagName, idTag);
        if(settings->value(u"current_tag"_s).toInt() == ui->TagFilter->count()-1 && g::options.bUseTag)
            ui->TagFilter->setCurrentIndex(ui->TagFilter->count()-1);
        QAction *ac;
        uint idIcon = query.value(2).toUInt();
        QIcon &iconTag = icons[idIcon];
        ui->TagFilter->setItemIcon(con, iconTag);
        ac = new QAction(iconTag, sTagName, &TagMenu);
        ac->setCheckable(true);
        iconsTags_[idTag] = iconTag;
        ac->setData(idTag);
        connect(ac, &QAction::triggered, this, &MainWindow::onSetTag);
        TagMenu.addAction(ac);
        con++;
    }

    updateIcons();

    ui->TagFilter->addItem(tr("setup ..."), -1);
    ui->TagFilter->blockSignals(wasBlocked);

    QApplication::restoreOverrideCursor();
}

void MainWindow::updateIcons()
{
    bool wasBlocked = ui->SeriaList->blockSignals(true);
    int nCount = ui->SeriaList->count();
    const auto &lib = g::libs[g::idCurrentLib];
    for(int i=0; i<nCount; i++){
        auto item = ui->SeriaList->item(i);
        uint idSerial = item->data(Qt::UserRole).toUInt();
        item->setIcon(getTagIcon(lib.serials.at(idSerial).idTags));
    }
    ui->SeriaList->blockSignals(wasBlocked);

    wasBlocked = ui->AuthorList->blockSignals(true);
    nCount = ui->AuthorList->count();
    for(int i=0; i<nCount; i++){
        auto item = ui->AuthorList->item(i);
        uint idAuthor = item->data(Qt::UserRole).toUInt();
        item->setIcon(getTagIcon(lib.authors.at(idAuthor).idTags));
    }
    ui->AuthorList->blockSignals(wasBlocked);

    wasBlocked = ui->Books->blockSignals(true);
    nCount  = ui->Books->topLevelItemCount();
    for(int i=0; i<nCount; i++){
        auto item1 = ui->Books->topLevelItem(i);
        updateItemIcon(item1);
        for(int j :views::iota(0, item1->childCount())){
            auto item2 = item1->child(j);
            updateItemIcon(item2);
            for(int k :views::iota(0, item2->childCount())){
                auto item3 = item2->child(k);
                updateItemIcon(item3);
            }
        }
    }
    ui->Books->blockSignals(wasBlocked);
}

void MainWindow::updateItemIcon(QTreeWidgetItem *item)
{
    uint id = item->data(0, Qt::UserRole).toUInt();
    switch(item->type()){
    case ITEM_TYPE_BOOK:
        item->setIcon(0, getTagIcon(g::libs[g::idCurrentLib].books[id].idTags));
        break;
    case ITEM_TYPE_SERIA:
        item->setIcon(0, getTagIcon(g::libs[g::idCurrentLib].serials[id].idTags));
        break;
    case ITEM_TYPE_AUTHOR:
        item->setIcon(0, getTagIcon(g::libs[g::idCurrentLib].authors[id].idTags));
        break;
    }
}

QIcon MainWindow::getTagIcon(const std::unordered_set<uint> &idTags)
{
    int size = idTags.size();
    if(size == 0)
        return QIcon();
    if(size == 1)
        return iconsTags_.at(*idTags.begin());
    return iconsTags_.at(0);
}

void MainWindow::updateTitle()
{
    QString sLibName;
    if(g::idCurrentLib != 0)
        sLibName = g::libs[g::idCurrentLib].name;
    setWindowTitle(u"freeLib"_s + (sLibName.isEmpty() ?u""_s : (u" — "_s + g::libs[g::idCurrentLib].name)));
#ifdef USE_KStatusNotifier
    if(statusNotifierItem_)
        statusNotifierItem_->setToolTipSubTitle(sLibName);
#else
    if(pTrayIcon_)
        pTrayIcon_->setToolTip(u"freeLib\n"_s + sLibName);
#endif
}

MainWindow::~MainWindow()
{
#ifndef USE_KStatusNotifier
    if(pTrayIcon_)
        delete pTrayIcon_;
    if(pTrayMenu_)
        delete pTrayMenu_;
#endif
    if(pHelpDlg != nullptr)
        delete pHelpDlg;

    if(g::options.bStorePosition)
        SaveLibPosition();
    delete ui;
}

void MainWindow::EditBooks()
{
    BookEditDlg dlg(this);
    dlg.exec();
}

/*
    обработчик клика мышкой на ссылках в описании Книги
*/
void MainWindow::ReviewLink(const QUrl &url)
{
    QString sPath = url.path();
    if(sPath.startsWith(u"author_"))
    {
        MoveToAuthor(sPath.right(sPath.length()-8).toLongLong(), sPath.mid(7,1).toUpper());
    }
    else if(sPath.startsWith(u"genre_"))
    {
        MoveToGenre(sPath.right(sPath.length()-7).toLongLong());
    }
    else if(sPath.startsWith(u"seria_"))
    {
        MoveToSeria(sPath.right(sPath.length()-7).toLongLong(), sPath.mid(6,1).toUpper());
    }
}

/*
    Изменение языка интерфейса
*/
void MainWindow::ChangingLanguage()
{
    ui->retranslateUi(this);
    FillListBooks();
    fillLanguages();
}

void MainWindow::FillAlphabet(const QString &sAlphabetName)
{
    QFile file(QStringLiteral(":/language/abc_%1.txt").arg(sAlphabetName));
    QString sAlphabet;
    if(!file.fileName().isEmpty() && file.open(QFile::ReadOnly))
    {
        sAlphabet = QString::fromUtf8(file.readLine()).toUpper();
    }
    QVBoxLayout *layout_abc_all = new QVBoxLayout();

    FirstButton = nullptr;
    if(!sAlphabet.isEmpty())
    {
        QHBoxLayout *layout_abc = new QHBoxLayout();
        int nCount = sAlphabet.length();
        for(int i=0; i<nCount; i++)
        {
            QToolButton *btn = new QToolButton(this);
            btn->setText(sAlphabet.at(i));
            btn->setMaximumSize(QSize(22, 22));
            btn->setMinimumSize(QSize(22, 22));
            btn->setFont(QFont(QStringLiteral("Noto Sans"), 10));
            btn->setCheckable(true);
            btn->setAutoExclusive(true);
            btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            btn->setFocusPolicy(Qt::NoFocus);
            layout_abc->addWidget(btn);
            connect(btn, &QAbstractButton::clicked, this, &MainWindow::btnSearch);
            if(!FirstButton)
                FirstButton = btn;
        }
        layout_abc->addStretch();
        layout_abc->setSpacing(1);
        layout_abc->setContentsMargins(0, 0, 0, 0);
        layout_abc_all->addItem(layout_abc);
    }
    const QString abc = u"*#ABCDEFGHIJKLMNOPQRSTUVWXYZ"_s;
    QHBoxLayout *layout_abc = new QHBoxLayout();
    for(int i :views::iota(0, abc.length()))
    {
        QToolButton *btn = new QToolButton(this);
        btn->setText(abc.at(i));
        btn->setMaximumSize(QSize(22, 22));
        btn->setMinimumSize(QSize(22, 22));
        btn->setCheckable(true);
        btn->setAutoExclusive(true);
        btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        btn->setFocusPolicy(Qt::NoFocus);
        layout_abc->addWidget(btn);
        connect(btn, &QAbstractButton::clicked, this, &MainWindow::btnSearch);
        if(!FirstButton && abc.at(i) == 'A')
            FirstButton = btn;
        if(abc.at(i) == '#')
            btn_Hash = btn;
    }
    layout_abc->addStretch();
    layout_abc->setSpacing(1);
#ifdef Q_OS_WIN
    layout_abc->setContentsMargins(0,!sAlphabet.isEmpty()?4:0,0,0);
#else
    layout_abc->setContentsMargins(0,!sAlphabet.isEmpty() ?5 :0, 0, 5);
#endif
    layout_abc_all->addItem(layout_abc);

    ui->abc->setLayout(layout_abc_all);
    ui->abc->layout()->setSpacing(1);
#ifdef Q_OS_WIN
    ui->abc->layout()->setContentsMargins(0,4,0,5);
#else
    ui->abc->layout()->setContentsMargins(0,0,0,0);
#endif
}

/*
    Установка метки на элемент
*/
void MainWindow::onSetTag()
{
    uint idTag = qobject_cast<QAction*>(QObject::sender())->data().toUInt();
    bool bSet = qobject_cast<QAction*>(QObject::sender())->isChecked();
    uint id;

    if(currentListForTag_ == ui->Books)
    {
        auto listItems = checkedItemsBookList();
        if(listItems.empty())
            listItems = ui->Books->selectedItems();
        for(auto item : std::as_const(listItems)){
            id = item->data(0, Qt::UserRole).toUInt();
            switch (item->type()) {
            case ITEM_TYPE_BOOK:
                setTagBook(idTag, id, bSet);
                break;

            case ITEM_TYPE_SERIA:
                setTagSequence(idTag, id, bSet);

                break;

            case ITEM_TYPE_AUTHOR:
                setTagAuthor(idTag, id, bSet);
                break;

            default:
                break;
            }
        }
    }
    else if(currentListForTag_ == ui->AuthorList)
    {
        id = ui->AuthorList->selectedItems().at(0)->data(Qt::UserRole).toUInt();
        setTagAuthor(idTag, id, bSet);
    }
    else if(currentListForTag_ == ui->SeriaList)
    {
        id = ui->SeriaList->selectedItems().at(0)->data(Qt::UserRole).toUInt();
        setTagSequence(idTag, id, bSet);
    }

    //TODO оптимизировать обновление иконок при большом количестве отображаемых авторов/книг
    updateIcons();
}

void MainWindow::setTagAuthor(uint idTag, uint idAuthor, bool bSet)
{
    QSqlQuery query(QSqlDatabase::database(u"libdb"_s));
    auto &author = g::libs[g::idCurrentLib].authors[idAuthor];
    auto &idTags = author.idTags;
    if(bSet){
        idTags.insert(idTag);
        query.prepare(u"INSERT INTO author_tag (id_author,id_tag) VALUES(:idAuthor, :idTag)"_s);
        query.bindValue(u":idAuthor"_s, idAuthor);
        query.bindValue(u":idTag"_s, idTag);
        if(!query.exec()) [[unlikely]]
            LogWarning << query.lastError().text();

        query.prepare(u"INSERT INTO author_tag (id_author,id_tag) "
                       "SELECT id, :idTag FROM author WHERE name1=:name1 AND name2=:name2 AND name3=:name3 AND id_lib!=:idLib"_s);
        query.bindValue(u":idTag"_s, idTag);
        query.bindValue(u":idLib"_s, g::idCurrentLib);
        query.bindValue(u":name1"_s, author.sLastName);
        query.bindValue(u":name2"_s, author.sFirstName);
        query.bindValue(u":name3"_s, author.sMiddleName);
        if(!query.exec()) [[unlikely]]
            LogWarning << query.lastError().text();
        for(auto &lib :g::libs){
            if(lib.first != g::idCurrentLib && lib.second.bLoaded){
                auto it = std::ranges::find_if(lib.second.authors, [&author](auto &&iAuthor)
                {
                    return  iAuthor.second.getName() == author.getName();
                });
                if(it != lib.second.authors.end()){
                    it->second.idTags.insert(idTag);
                }
            }
        }
    }else{
        idTags.erase(idTag);
        query.exec(u"PRAGMA foreign_keys = ON"_s);
        query.prepare(u"DELETE FROM author_tag WHERE (id_author=:idAuthor) AND (id_tag=:idTag)"_s);
        query.bindValue(u":idAuthor"_s, idAuthor);
        query.bindValue(u":idTag"_s, idTag);
        if(!query.exec()) [[unlikely]]
            LogWarning << query.lastError().text();

        query.prepare(u"DELETE FROM author_tag WHERE (id_tag=:idTag) AND id_author IN ("
                      "SELECT id FROM author WHERE name1=:name1 AND name2=:name2 AND name3=:name3 AND id_lib!=:idLib);"_s);
        query.bindValue(u":idTag"_s, idTag);
        query.bindValue(u":idLib"_s, g::idCurrentLib);
        query.bindValue(u":name1"_s, author.sLastName);
        query.bindValue(u":name2"_s, author.sFirstName);
        query.bindValue(u":name3"_s, author.sMiddleName);
        if(!query.exec()) [[unlikely]]
            LogWarning << query.lastError().text();
    }
}

void MainWindow::setTagBook(uint idTag, uint idBook, bool bSet)
{
    QSqlQuery query(QSqlDatabase::database(u"libdb"_s));
    auto &book = g::libs[g::idCurrentLib].books.at(idBook);
    auto &idTags = book.idTags;
    if(bSet){
        idTags.insert(idTag);
        query.prepare(u"INSERT INTO book_tag (id_book,id_tag) VALUES(:idBook,:idTag)"_s);
        query.bindValue(u":idBook"_s, idBook);
        query.bindValue(u":idTag"_s, idTag);
        if(!query.exec()) [[unlikely]]
            LogWarning << query.lastError().text();

        query.prepare(u"INSERT INTO book_tag (id_book,id_tag) "
                      "SELECT id, :idTag FROM book WHERE id_inlib=:idInLib AND archive=:archive AND id_lib!=:idLib"_s);
        query.bindValue(u":idTag"_s, idTag);
        query.bindValue(u":idLib"_s, g::idCurrentLib);
        query.bindValue(u":idInLib"_s, book.idInLib);
        query.bindValue(u":archive"_s, book.sArchive);
        if(!query.exec()) [[unlikely]]
            LogWarning << query.lastError().text();
    }else{
        idTags.erase(idTag);
        query.exec(u"PRAGMA foreign_keys = ON"_s);
        query.prepare(u"DELETE FROM book_tag WHERE (id_book=:idBook) AND (id_tag=:idTag)"_s);
        query.bindValue(u":idBook"_s, idBook);
        query.bindValue(u":idTag"_s, idTag);
        if(!query.exec()) [[unlikely]]
            LogWarning << query.lastError().text();

        query.prepare(u"DELETE FROM book_tag WHERE (id_tag=:idTag) AND id_book IN ("
                      "SELECT id FROM book WHERE id_inlib=:idInLib AND archive=:archive AND id_lib!=:idLib);"_s);
        query.bindValue(u":idTag"_s, idTag);
        query.bindValue(u":idLib"_s, g::idCurrentLib);
        query.bindValue(u":idInLib"_s, book.idInLib);
        query.bindValue(u":archive"_s, book.sArchive);
        if(!query.exec()) [[unlikely]]
            LogWarning << query.lastError().text();
    }
}

void MainWindow::setTagSequence(uint idTag, uint idSequence, bool bSet)
{
    QSqlQuery query(QSqlDatabase::database(u"libdb"_s));
    auto &sequence = g::libs[g::idCurrentLib].serials.at(idSequence);
    auto &idTags = sequence.idTags;
    if(bSet){
        idTags.insert(idTag);
        query.prepare(u"INSERT INTO seria_tag (id_seria,id_tag) VALUES(:idSequence,:idTag)"_s);
        query.bindValue(u":idSequence"_s, idSequence);
        query.bindValue(u":idTag"_s, idTag);
        if(!query.exec()) [[unlikely]]
            LogWarning << query.lastError().text();

        query.prepare(u"INSERT INTO seria_tag (id_seria,id_tag) "
                      "SELECT id, :idTag FROM seria WHERE name=:name AND id_lib!=:idLib"_s);
        query.bindValue(u":idTag"_s, idTag);
        query.bindValue(u":idLib"_s, g::idCurrentLib);
        query.bindValue(u":name"_s, sequence.sName);
        if(!query.exec()) [[unlikely]]
            LogWarning << query.lastError().text();
    }else{
        idTags.erase(idTag);
        query.exec(u"PRAGMA foreign_keys = ON"_s);
        query.prepare(u"DELETE FROM seria_tag WHERE (id_seria=:idSequence) AND (id_tag=:idTag)"_s);
        query.bindValue(u":idSequence"_s, idSequence);
        query.bindValue(u":idTag"_s, idTag);
        if(!query.exec()) [[unlikely]]
            LogWarning << query.lastError().text();

        query.prepare(u"DELETE FROM seria_tag WHERE (id_tag=:idTag) AND id_seria IN ("
                      "SELECT id FROM seria WHERE name=:name AND id_lib!=:idLib);"_s);
        query.bindValue(u":idTag"_s, idTag);
        query.bindValue(u":idLib"_s, g::idCurrentLib);
        query.bindValue(u":name"_s, sequence.sName);
        if(!query.exec()) [[unlikely]]
            LogWarning << query.lastError().text();
    }
}

void MainWindow::onTagFilterChanged(int index)
{
    auto settings = GetSettings();
    if(ui->TagFilter->itemData(ui->TagFilter->currentIndex()).toInt() == -1)
    {
        const bool wasBlocked = ui->TagFilter->blockSignals(true);
        ui->TagFilter->setCurrentIndex(settings->value(u"current_tag"_s, 0).toInt());
        ui->TagFilter->blockSignals(wasBlocked);
        TagDialog td(this);
        if(td.exec())
            UpdateTags();
    }
    else if(index >= 0)
    {
        settings->setValue(u"current_tag"_s, index);
        FillAuthors();
        FillSerials();
        FillGenres();
        FillListBooks();
    }
}

void MainWindow::SaveLibPosition()
{
    auto settings = GetSettings();
    switch (ui->tabWidget->currentIndex()) {
    case TabAuthors:
        settings->setValue(QStringLiteral("filter_set"), ui->searchAuthor->text());
        break;
    case TabSeries:
        settings->setValue(QStringLiteral("filter_set"), ui->searchSeries->text());
        break;
    }
    settings->setValue(QStringLiteral("current_tab"), ui->tabWidget->currentIndex());
    settings->setValue(QStringLiteral("current_book_id"), idCurrentBook_);
}


void MainWindow::closeEvent(QCloseEvent *event)
{
    auto settings = GetSettings();
    settings->setValue(u"MainWnd/geometry"_s, saveGeometry());
    settings->setValue(u"MainWnd/windowState"_s, saveState());
    settings->setValue(u"MainWnd/VSplitterSizes"_s, ui->splitterV->saveState());
    settings->setValue(u"MainWnd/HSplitterSizes"_s, ui->splitterH->saveState());
    settings->beginGroup(u"Columns"_s);
    if(bTreeView_){
        settings->setValue(u"headersTree"_s, ui->Books->header()->saveState());
        settings->setValue(u"headersList"_s, aHeadersList_);
    }else{
        settings->setValue(u"headersTree"_s, aHeadersTree_);
        settings->setValue(u"headersList"_s, ui->Books->header()->saveState());
    }
    settings->endGroup();

#ifndef USE_KStatusNotifier
    if (pTrayIcon_ != nullptr && pTrayIcon_->isVisible()) {
        hide();
        event->ignore();
    }
#endif
}

/*
    Открытие окна настроек
*/
void MainWindow::Settings()
{
    SettingsDlg *pDlg = new SettingsDlg(this);
    connect(pDlg, &SettingsDlg::ChangingLanguage, this, [this](){this->ChangingLanguage();});
    connect(pDlg, &SettingsDlg::ChangeAlphabet, this, &MainWindow::onChangeAlpabet);
    connect(pDlg, &SettingsDlg::ChangingTrayIcon, this, &MainWindow::changingTrayIcon);
    connect(pDlg, &SettingsDlg::ChangeListFont, this, &MainWindow::onUpdateListFont);
    connect(pDlg, &SettingsDlg::ChangeAnnotationFont, this, &MainWindow::onUpdateAnnotationFont);

    if(pDlg->exec() == QDialog::Accepted){
        if(g::options.bShowDeleted != pDlg->options_.bShowDeleted || g::options.bUseTag != pDlg->options_.bUseTag)
        {
            UpdateTags();
            SaveLibPosition();
            FillAuthors();
            FillGenres();
            FillListBooks();
        }
#ifdef USE_HTTSERVER
        if(g::options.bOpdsEnable != pDlg->options_.bOpdsEnable || g::options.nHttpPort != pDlg->options_.nHttpPort || g::options.sBaseUrl != pDlg->options_.sBaseUrl ||
           g::options.bOpdsNeedPassword != pDlg->options_.bOpdsNeedPassword || g::options.sOpdsUser != pDlg->options_.sOpdsUser ||
           g::options.baOpdsPasswordHash != pDlg->options_.baOpdsPasswordHash)
        {
            if(pOpds_ == nullptr && g::options.bOpdsEnable)
                pOpds_ = std::unique_ptr<opds_server>( new opds_server(this) );
            pOpds_->server_run();
        }
#endif
        UpdateExportMenu();
        resizeEvent(nullptr);
    }
    pDlg->deleteLater();
}

/*
    Возвращает список отмеченных книг
*/
std::vector<uint> MainWindow::getCheckedBooks(bool bCheckedOnly)
{
    std::vector<uint> vBooks;
    FillCheckedItemsBookList(nullptr, false, &vBooks);
    if(vBooks.empty() && !bCheckedOnly)
    {
        if(ui->Books->selectedItems().count() > 0)
        {
            if(ui->Books->selectedItems()[0]->childCount() > 0)
                FillCheckedItemsBookList(ui->Books->selectedItems()[0], true, &vBooks);
            else
            {
                if(ui->Books->selectedItems()[0]->type() == ITEM_TYPE_BOOK)
                {
                    uint idBook = ui->Books->selectedItems()[0]->data(0, Qt::UserRole).toUInt();
                    vBooks.push_back(idBook);
                }
            }
        }
    }
    return vBooks;
}

void MainWindow::FillCheckedItemsBookList(const QTreeWidgetItem* item, bool send_all, std::vector<uint> *pList)
{
    QTreeWidgetItem* current;
    int count = item ?item->childCount() :ui->Books->topLevelItemCount();
    for(int i=0; i < count; i++)
    {
        current = item ?item->child(i) :ui->Books->topLevelItem(i);
        if(current->childCount()>0)
        {
            FillCheckedItemsBookList(current, send_all, pList);
        }
        else
        {
            if(current->checkState(0) == Qt::Checked || send_all)
            {
                if(current->type() == ITEM_TYPE_BOOK)
                {
                    uint idBook = current->data(0, Qt::UserRole).toUInt();
                    pList->push_back(idBook);
                }
            }
        }
    }
}

QList<QTreeWidgetItem *> MainWindow::checkedItemsBookList(const QTreeWidgetItem *item)
{
    QList<QTreeWidgetItem*> listItems;
    QTreeWidgetItem* current;
    int count = item ?item->childCount() :ui->Books->topLevelItemCount();
    for(int i=0; i < count; i++)
    {
        current = item ?item->child(i) :ui->Books->topLevelItem(i);
        if(current->childCount()>0)
        {
            listItems << checkedItemsBookList(current);
        }
        if(current->checkState(0) == Qt::Checked)
            listItems << current;
    }
    return listItems;
}

void MainWindow::uncheckBooks(const std::vector<uint> &vBooks)
{
    if(!g::options.bUncheckAfterExport)
        return;
    QList<QTreeWidgetItem*> items;
    if(ui->Books->topLevelItemCount() == 0)
        return;
    for(auto id: vBooks)
    {
        for(int i :views::iota(0, ui->Books->topLevelItemCount()))
            items << ui->Books->topLevelItem(i);
        do
        {
            if(items[0]->childCount() > 0)
            {
                for(int j :views::iota(0, items[0]->childCount()))
                    items << items[0]->child(j);
            }
            else
            {
                if(items[0]->data(0, Qt::UserRole).toLongLong() == id && items[0]->checkState(0) == Qt::Checked)
                    items[0]->setCheckState(0, Qt::Unchecked);
            }
            items.removeAt(0);
        }while(items.count() > 0);
        items.clear();
    }
}

void MainWindow::SendToDevice(const ExportOptions &exportOptions)
{
    std::vector<uint> vBooks = getCheckedBooks();
    if(vBooks.empty())
        return;
    ExportDlg dlg(this);
    dlg.exec(vBooks, ST_Device, ui->tabWidget->currentIndex() == TabAuthors ?ui->AuthorList->selectedItems()[0]->data(Qt::UserRole).toLongLong() :0, exportOptions);
    uncheckBooks(dlg.vSuccessfulExportBooks);
}

void MainWindow::SendMail(const ExportOptions &exportOptions)
{
    std::vector<uint> vBooks = getCheckedBooks();
    if(vBooks.empty())
        return;
    ExportDlg dlg(this);
    dlg.exec(vBooks, ST_Mail, ui->tabWidget->currentIndex() == TabAuthors ?ui->AuthorList->selectedItems()[0]->data(Qt::UserRole).toLongLong() :0, exportOptions);
    uncheckBooks(dlg.vSuccessfulExportBooks);
}


void MainWindow::BookDblClick()
{
    if(ui->Books->selectedItems().count() == 0)
        return;
    QTreeWidgetItem* item = ui->Books->selectedItems()[0];
    if(item->type() != ITEM_TYPE_BOOK)
        return;
    uint idBook = item->data(0, Qt::UserRole).toUInt();
    SLib& lib = g::libs[g::idCurrentLib];
    SBook &book = lib.books[idBook];
    QString sFileName;
    if(book.sArchive.isEmpty()){
        sFileName = lib.path % u"/"_s % book.sFile % u"."_s % book.sFormat;
    }else{
        BookFile fileBook(g::idCurrentLib, idBook);
        QByteArray baBook = fileBook.data();
        if(baBook.size() == 0) [[unlikely]]
            return;
        sFileName = QDir::tempPath() % u"/freeLib/"_s % fileBook.fileName();
        QFile file(sFileName);
        file.open(QFile::WriteOnly);
        file.write(baBook);
        file.close();
    }
    if(g::options.applications.contains(book.sFormat)){
        if(
#ifdef Q_OS_MACX
        QProcess::startDetached("open",QStringList()<<options.applications.value(fi.suffix().toLower())<<"--args"<<file.fileName())&&
                QFileInfo(g::options.applications.value(sExt)).exists()
#else
        QProcess::startDetached(g::options.applications.at(book.sFormat), QStringList() << sFileName)
#endif
        )
            return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(sFileName));
}

void MainWindow::CheckBooks()
{
    std::vector<uint> vBooks = getCheckedBooks(true);

    const QSignalBlocker blocker(ui->Books);
    Qt::CheckState cs = vBooks.empty() ?Qt::Checked :Qt::Unchecked;
    int nTopLevelItemCount = ui->Books->topLevelItemCount();
    for(int i = 0; i<nTopLevelItemCount; i++)
    {
        ui->Books->topLevelItem(i)->setCheckState(0, cs);
        CheckChild(ui->Books->topLevelItem(i));
    }
}

void MainWindow::CheckParent(QTreeWidgetItem *parent)
{
    bool checked = false;
    bool unchecked = false;
    bool partially = false;
    int nChildCount = parent->childCount();
    for(int i = 0; i<nChildCount; i++)
    {
        switch(parent->child(i)->checkState(0))
        {
        case Qt::Checked:
            checked = true;
            break;
        case Qt::Unchecked:
            unchecked = true;
            break;
        case Qt::PartiallyChecked:
            partially = true;
            break;
        }
    }
    if(partially || (checked && unchecked))
        parent->setCheckState(0, Qt::PartiallyChecked);
    else if(checked)
        parent->setCheckState(0, Qt::Checked);
    else
        parent->setCheckState(0, Qt::Unchecked);
    if(parent->parent())
        CheckParent(parent->parent());
}

void MainWindow::CheckChild(QTreeWidgetItem *parent)
{
    int nChildCount = parent->childCount();
    if(nChildCount > 0)
    {
        for(int i=0; i<nChildCount; i++)
        {
            parent->child(i)->setCheckState(0, parent->checkState(0));
            if(parent->child(i)->childCount() > 0)
                CheckChild(parent->child(i));
        }
    }
}

void MainWindow::onItemChanged(QTreeWidgetItem *item, int)
{
    const bool wasBlocked = ui->Books->blockSignals(true);

    CheckChild(item);
    QTreeWidgetItem* parent = item->parent();
    if(parent)
        CheckParent(parent);
    std::vector<uint> vBooks = getCheckedBooks();
    ExportBookListBtn(!vBooks.empty());

    ui->Books->blockSignals(wasBlocked);
}

void MainWindow::ExportBookListBtn(bool Enable)
{
    ui->btnExport->setEnabled(Enable);
    ui->btnOpenBook->setEnabled(false);
}

void MainWindow::onStartSearch()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    QApplication::processEvents();
    ui->Books->clear();
    ExportBookListBtn(false);
    QString sName = ui->s_name->text().trimmed();
    QString sAuthor = ui->s_author->text().trimmed();
    QString sSeria = ui->s_seria->text().trimmed();
    QString sFile = ui->s_FileName->text().trimmed();
    QDate dateFrom = ui->date_from->date();
    QDate dateTo = ui->date_to->date();
    int nMaxCount = ui->maxBooks->value();
    ushort idGenre = ui->s_genre->currentData().toUInt();
    bool bTopGenre = idGenre<100;
    int idLanguage = ui->findLanguage->currentData().toInt();
    uint nMinimalRating = ui->comboMinimalRating->currentData().toUInt();

    SLib& lib = g::libs[g::idCurrentLib];
    vFoundBooks_.clear();
    int nCount = 0;
    std::vector<uint> vPosFound;
    for(const auto &[idBook, book] :lib.books){
        if(!g::options.bShowDeleted && book.bDeleted)
            continue;

        if(idLanguage != -1 && book.idLanguage != idLanguage)
            continue;

        if(book.nStars < nMinimalRating)
            continue;

        bool bMatchAuthor = false;
        if(!sAuthor.isEmpty()){
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
            const QStringList listAuthor1 = sAuthor.split(' ', Qt::SkipEmptyParts);
#else
            const QStringList listAuthor1 = sAuthor.split(' ');
#endif
            for(auto idAuthor :book.vIdAuthors){
                vPosFound.clear();
                const QStringList listAuthor2 = lib.authors[idAuthor].getName().split(' ');
                bool bMatch = true;
                for(const auto &sAuthor1 : listAuthor1){
                    uint i2 = 0;
                    while(i2 < listAuthor2.size()){
                        if(sAuthor1.compare(listAuthor2.at(i2), Qt::CaseInsensitive) == 0){
                            if(!contains(vPosFound, i2)){
                                vPosFound.push_back(i2);
                                break;
                            }
                        }
                        ++i2;
                    }
                    if(i2 == listAuthor2.size()){
                        bMatch = false;
                        break;
                    }
                }
                if(bMatch){
                    bMatchAuthor = true;
                    break;
                }
            }
        }else
            bMatchAuthor = true;        
        if(!bMatchAuthor)
            continue;

        bool bMatchFile;
        if(sFile.isEmpty())
            bMatchFile = true;
        else{
            QString sBookFileName = book.sFile % u"."_s % book.sFormat;
            bMatchFile = sBookFileName.contains(sFile, Qt::CaseInsensitive);
        }
        if(!bMatchFile)
            continue;

        if(book.date < dateFrom || book.date > dateTo)
            continue;

        if(!sName.isEmpty() &&  !book.sName.contains(sName, Qt::CaseInsensitive))
            continue;

        bool bSequence = false;
        if(sSeria.isEmpty())
            bSequence = true;
        else{
            for(const auto &iSequence :book.mSequences){
                uint idSequence = iSequence.first;
                if(lib.serials.at(idSequence).sName.contains(sSeria, Qt::CaseInsensitive)){
                    bSequence = true;
                    break;
                }
            }
        }
        if(!bSequence)
            continue;

        if(idGenre == 0){
            nCount++;
            vFoundBooks_.push_back(idBook);
        }else{
            for(auto id: book.vIdGenres) {
                if((bTopGenre && g::genres.at(id).idParrentGenre == idGenre) || id == idGenre){
                    nCount++;
                    vFoundBooks_.push_back(idBook);
                    break;
                }
            }
        }
        if(nCount == nMaxCount) [[unlikely]]
            break;
    }
    QLocale locale;
    ui->find_books->setText(locale.toString(nCount));
    FillListBooks(vFoundBooks_ ,{} , 0);
    if(g::options.bStorePosition){
        auto settings = GetSettings();
        settings->setValue(u"search.title"_s, sName);
        settings->setValue(u"search.author"_s, sAuthor);
        settings->setValue(u"search.sequence"_s, sSeria);
        settings->setValue(u"search.id.genre"_s, idGenre);

        QString sLanguage = idLanguage>=0 ?lib.vLaguages[idLanguage] :u""_s;
        settings->setValue(u"search.language"_s, sLanguage);
        settings->setValue(u"search.min.rating"_s, nMinimalRating);

        settings->setValue(u"search.file"_s, sFile);
        // settings->setValue(u"search.date.from"_s, dateFrom);
        // settings->setValue(u"search.date.to"_s, dateTo);
        settings->setValue(u"search.max.books"_s, nMaxCount);
    }

    QApplication::restoreOverrideCursor();
}

void MainWindow::onClearSearch()
{
    ui->Books->clear();
    ui->s_name->clear();
    ui->s_author->clear();
    ui->s_seria->clear();
    ui->s_FileName->clear();
    ui->find_books->clear();
    ui->comboMinimalRating->setCurrentIndex(0);
    vFoundBooks_.clear();
    if(g::options.bStorePosition){
        auto settings = GetSettings();
        settings->remove(u"search.title"_s);
        settings->remove(u"search.author"_s);
        settings->remove(u"search.sequence"_s);
        settings->remove(u"search.id.genre"_s);
        settings->remove(u"search.language"_s);
        settings->remove(u"search.min.rating"_s);
        settings->remove(u"search.file"_s);
        settings->remove(u"search.max.books"_s);
    }
}

void MainWindow::SelectLibrary()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    QApplication::processEvents();

    QAction* action = qobject_cast<QAction*>(sender());
    SaveLibPosition();
    uint idOldLib = g::idCurrentLib;
    g::idCurrentLib = action->data().toUInt();
    if(g::idCurrentLib != idOldLib){
        auto &oldLib = g::libs[idOldLib];
        QFuture<void> future;
        if(oldLib.timeHttp + 24h < std::chrono::system_clock::now()){
            future = QtConcurrent::run([&oldLib]()
            {
                oldLib.bLoaded = false;
                oldLib.books.clear();
                oldLib.authors.clear();
                oldLib.serials.clear();
                oldLib.authorBooksLink.clear();
                oldLib.vLaguages.clear();
            });
        }

        auto settings = GetSettings();
        settings->setValue(QStringLiteral("LibID"), g::idCurrentLib);

        loadLibrary(g::idCurrentLib);
        fillLanguages();
        FillAuthors();
        FillSerials();
        FillGenres();
        switch(ui->tabWidget->currentIndex()){
        case TabAuthors:
            onSerachAuthorsChanded(ui->searchAuthor->text());
            break;
        case TabSeries:
            onSerachSeriesChanded(ui->searchSeries->text());
            break;
        case TabGenres:
            SelectGenre();
            break;
        case TabSearch:
            ui->Books->clear();
        }
        vFoundBooks_.clear();
        ui->find_books->clear();
        ui->date_from->setDate( g::libs[g::idCurrentLib].earliestDate_ );
        updateTitle();
        future.waitForFinished();
    }

    QApplication::restoreOverrideCursor();
}

/*
    выбор текущего жанра
*/
void MainWindow::SelectGenre()
{
    ui->Books->clear();
    ExportBookListBtn(false);
    if(ui->GenreList->selectedItems().count() == 0)
        return;
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    QTreeWidgetItem* cur_item = ui->GenreList->selectedItems()[0];
    ushort idGenre = cur_item->data(0, Qt::UserRole).toUInt();
    vBooks_.clear();
    SLib& lib = g::libs[g::idCurrentLib];
    for(const auto &[idBook, book] :lib.books){
        if(IsBookInList(book)){
            for(auto iGenre: book.vIdGenres) {
                if(iGenre == idGenre){
                    vBooks_.push_back(idBook);
                    break;
                }
            }
        }
    }
    idCurrentGenre_ = idGenre;
    FillListBooks(vBooks_, std::vector<uint>(), 0);
    auto settings = GetSettings();
    if(g::options.bStorePosition){
        settings->setValue(u"current_genre_id"_s, idCurrentGenre_);
    }
    QApplication::restoreOverrideCursor();
}

/*
    выбор Серии в списке Серий
*/
void MainWindow::SelectSeria()
{
    ui->Books->clear();
    ExportBookListBtn(false);
    if(ui->SeriaList->selectedItems().count() == 0){
        ui->Books->clear();
        return;
    }
    QListWidgetItem* cur_item = ui->SeriaList->selectedItems().at(0);
    idCurrentSerial_ = cur_item->data(Qt::UserRole).toUInt();
    vBooks_.clear();
    SLib& lib = g::libs[g::idCurrentLib];

    for(const auto &[idBook, book] :lib.books){
        if(book.mSequences.contains(idCurrentSerial_) && IsBookInList(book)){
            vBooks_.push_back(idBook);
        }
    }
    FillListBooks(vBooks_, std::vector<uint>(), 0);

    if(g::options.bStorePosition){
        auto settings = GetSettings();
        settings->setValue(u"current_serial_id"_s, idCurrentSerial_);
    }
}

/*
    выбор Автора в списке Авторов
*/
void MainWindow::SelectAuthor()
{
    ExportBookListBtn(false);
    if(ui->AuthorList->selectedItems().count() == 0){
        ui->Books->clear();
        return;
    }
    QListWidgetItem* pItem = ui->AuthorList->selectedItems().at(0);
    idCurrentAuthor_ = pItem->data(Qt::UserRole).toUInt();
    vBooks_.clear();
    SLib &currentLib = g::libs[g::idCurrentLib];

    for (auto it = g::libs[g::idCurrentLib].authorBooksLink.equal_range(idCurrentAuthor_); it.first != it.second; ++it.first) {
        uint id = it.first->second;
        const auto &book = currentLib.books.at(id);
        if(IsBookInList(book))
            vBooks_.push_back(it.first->second);
    }
    FillListBooks(vBooks_, {}, idCurrentAuthor_);

    if(g::options.bStorePosition){
        auto settings = GetSettings();
        settings->setValue(u"current_author_id"_s, idCurrentAuthor_);
    }
}

void MainWindow::selectBook()
{
    if(ui->Books->selectedItems().count() == 0)
    {
        ExportBookListBtn(false);
        ui->Review->setHtml(u""_s);
        pCover->setImage(QImage());;
        return;
    }
    ExportBookListBtn(true);
    QTreeWidgetItem* item = ui->Books->selectedItems().at(0);
    if(item->type() != ITEM_TYPE_BOOK)
    {
        ui->btnOpenBook->setEnabled(false);
        ui->Review->setHtml(u""_s);
        pCover->setImage(QImage());;
        return;
    }
    uint idBook = item->data(0, Qt::UserRole).toUInt();
    idCurrentBook_ = idBook;
    SLib& lib = g::libs[g::idCurrentLib];
    SBook &book = lib.books[idBook];
    BookFile file(g::idCurrentLib, idBook);
    file.open();
    ui->btnOpenBook->setEnabled(true);
    if(book.sAnnotation.isEmpty())
        book.sAnnotation = file.annotation();

    QString sSequence;
    if(!book.mSequences.empty()){
        sSequence = u"<tr><td><b>Серия:</b></td><td>"_s;
        bool bFirst = true;
        for(auto [idSequence, numInSequence] :book.mSequences){
            if(!bFirst)
                sSequence += u"; "_s;
            sSequence += u"<a href=seria_%3%1>%2</a>"_s.arg(QString::number(idSequence),lib.serials[idSequence].sName, lib.serials[idSequence].sName.at(0).toUpper());
            bFirst = false;
        }
    }

    QString sAuthors;
    for(auto idAuthor: book.vIdAuthors)
    {
        QString sAuthor = lib.authors[idAuthor].getName();
        sAuthors += (sAuthors.isEmpty() ?u""_s :u"; "_s) + u"<a href='author_%3%1'>%2</a>"_s
                .arg(QString::number(idAuthor), sAuthor.replace(',', ' '), sAuthor.at(0));
    }
    QString sGenres;
    for(auto idGenre: book.vIdGenres)
    {
        QString sGenre = g::genres[idGenre].sName;
        sGenres += (sGenres.isEmpty() ?u""_s :u"; "_s) + u"<a href='genre_%3%1'>%2</a>"_s
                .arg(QString::number(idGenre), sGenre, sGenre.at(0));
    }
    QFile file_html(u":/preview.html"_s);
    file_html.open(QIODevice::ReadOnly);
    QString content(file_html.readAll());
    auto size = file.fileSize();
    pCover->setImage(file.cover());

    QLocale locale;
    QColor colorBInfo = palette().color(QPalette::AlternateBase);
    content.replace(u"#annotation#"_s, book.sAnnotation).
            replace(u"#title#"_s, book.sName).
            replace(u"#author#"_s, sAuthors).
            replace(u"#genre#"_s, sGenres).
            replace(u"#sequence#"_s, sSequence).
            replace(u"#file_path#"_s, file.filePath()).
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
            replace(u"#file_size#"_s, size>0 ?locale.formattedDataSize(file.fileSize(), 1, QLocale::DataSizeTraditionalFormat) : u""_s).
#else
            replace(QLatin1String("#file_size#"), size>0 ?sizeToString(size) : QLatin1String("")).
#endif
            replace(u"#file_data#"_s, file.birthTime().toString(u"dd.MM.yyyy hh:mm:ss"_s)).
            replace(u"#file_name#"_s, /*file.fileName()*/book.sFile % u"."_s % book.sFormat).
            replace(u"#infobcolor"_s, colorBInfo.name());
    ui->Review->setHtml(content);
}

/*
    заполнение списка языков в основном фильтре языков и фильтре языков на вкладке поиска
*/
void MainWindow::fillLanguages()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    SLib &currentLib = g::libs[g::idCurrentLib];
    static std::unordered_map<QString, QString> mLanguges = {{u"ab"_s, u"Аԥсшәа"_s},
                                                             {u"ad"_s, u"Адыгэбзэ"_s},
                                                             {u"am"_s, u"አማርኛ"_s},
                                                             {u"ar"_s, u"العربية"_s},
                                                             {u"az"_s, u"Azərbaycanca"_s},
                                                             {u"ba"_s, u"Башҡортса"_s},
                                                             {u"be"_s, u"Беларуская"_s},
                                                             {u"bg"_s, u"Български"_s},
                                                             {u"bm"_s, u"ߓߡߊߣߊ߲ߞߊ߲"_s},
                                                             {u"bn"_s, u"বাংলা"_s},
                                                             {u"bo"_s, u"བོད་སྐད"_s},
                                                             {u"br"_s, u"Brezhoneg"_s},
                                                             {u"bs"_s, u"Bosanski"_s},
                                                             {u"ca"_s, u"Catala"_s},
                                                             {u"ce"_s, u"Нохчийн"_s},
                                                             {u"co"_s, u"Corsu"_s},
                                                             {u"cr"_s, u"Qırımtatar"_s},
                                                             {u"cs"_s, u"Čeština"_s},
                                                             {u"cu"_s, u"Словѣньскъ"_s},
                                                             {u"cv"_s, u"Чӑвашла"_s},
                                                             {u"da"_s, u"Dansk"_s},
                                                             {u"de"_s, u"Deutsch"_s},
                                                             {u"dv"_s, u"Dhivehi bas"_s},
                                                             {u"el"_s, u"Ελληνικά"_s},
                                                             {u"en"_s, u"English"_s},
                                                             {u"eo"_s, u"Esperanto"_s},
                                                             {u"es"_s, u"Español"_s},
                                                             {u"et"_s, u"Eesti"_s},
                                                             {u"ev"_s, u"Эвэды"_s},
                                                             {u"fa"_s, u"فارسی"_s},
                                                             {u"fi"_s, u"Suomi"_s},
                                                             {u"fr"_s, u"Français"_s},
                                                             {u"ga"_s, u"Gaeilge"_s},
                                                             {u"gu"_s, u"ગુજરાતી"_s},
                                                             {u"he"_s, u"עברית"_s},
                                                             {u"hi"_s, u"हिन्दी"_s},
                                                             {u"hr"_s, u"Hrvatski"_s},
                                                             {u"hu"_s, u"Magyar"_s},
                                                             {u"hy"_s, u"Հայերեն"_s},
                                                             {u"ia"_s, u"Interlingua"_s},
                                                             {u"id"_s, u"Bahasa Indonesia"_s},
                                                             {u"ie"_s, u"Interlingue"_s},
                                                             {u"is"_s, u"Íslenska"_s},
                                                             {u"it"_s, u"Italiano"_s},
                                                             {u"ja"_s, u"日本語"_s},
                                                             {u"ka"_s, u"ქართული"_s},
                                                             {u"kb"_s, u"Къарачай-малкъар"_s},
                                                             {u"kk"_s, u"Қазақша"_s},
                                                             {u"kl"_s, u"Kalaallisut"_s},
                                                             {u"km"_s, u"ភាសាខ្មែរ"_s},
                                                             {u"kn"_s, u"ಕನ್ನಡ"_s},
                                                             {u"ko"_s, u"한국어"_s},
                                                              //{u"kr"_s, u"Къарачай-Малкъар тил"_s},
                                                             {u"ks"_s, u"کٲشُر"_s},
                                                             {u"ku"_s, u"کوردی"_s},
                                                             {u"kv"_s, u"Коми"_s},
                                                             {u"ky"_s, u"Кыргызский"_s},
                                                             {u"la"_s, u"Latina"_s},
                                                             {u"le"_s, u"Лезги"_s},
                                                             {u"lt"_s, u"Lietuvių"_s},
                                                             {u"lv"_s, u"Latviešu"_s},
                                                             {u"mk"_s, u"Македонски"_s},
                                                             {u"ml"_s, u"മലയാളം"_s},
                                                             {u"mn"_s, u"Монгол"_s},
                                                             {u"mr"_s, u"मराठी"_s},
                                                             {u"my"_s, u"ဗမာစာ"_s},
                                                             {u"ne"_s, u"नेपाली"_s},
                                                             {u"nl"_s, u"Nederlands"_s},
                                                             {u"no"_s, u"Norsk"_s},
                                                             {u"oc"_s, u"Occitan"_s},
                                                             {u"os"_s, u"Ирон"_s},
                                                             {u"pl"_s, u"Polski"_s},
                                                             {u"ps"_s, u"پښتو"_s},
                                                             {u"pt"_s, u"Português"_s},
                                                             {u"ro"_s, u"Română"_s},
                                                             {u"ru"_s, u"Русский"_s},
                                                             {u"rw"_s, u"Kinyarwanda"_s},
                                                             {u"sa"_s, u"संस्कृत"_s},
                                                             {u"sd"_s, u"सिन्धी"_s},
                                                             {u"sk"_s, u"Slovenčina"_s},
                                                             {u"sl"_s, u"Slovenski"_s},
                                                             {u"sp"_s, u"Español"_s},
                                                             {u"sq"_s, u"Shqip"_s},
                                                             {u"sr"_s, u"Српски"_s},
                                                             {u"sv"_s, u"Svenska"_s},
                                                             {u"sw"_s, u"Kiswahili"_s},
                                                             {u"ta"_s, u"தமிழ்"_s},
                                                             {u"te"_s, u"తెలుగు"_s},
                                                             {u"tg"_s, u"Тоҷикӣ"_s},
                                                             {u"th"_s, u"ภาษาไทย"_s},
                                                             {u"tk"_s, u"Türkmençe"_s},
                                                             {u"tr"_s, u"Türkçe"_s},
                                                             {u"tt"_s, u"Татарча"_s},
                                                             {u"ug"_s, u"ئۇيغۇرچە"_s},
                                                             {u"uk"_s, u"Українська"_s},
                                                             {u"ur"_s, u"اردو"_s},
                                                             {u"uz"_s, u"Oʻzbekcha"_s},
                                                             {u"vi"_s, u"Tiếng Việt"_s},
                                                             {u"yi"_s, u"ייִדיש"_s},
                                                             {u"zh"_s, u"中文"_s}};

    ui->language->blockSignals(true);
    ui->findLanguage->blockSignals(true);
    ui->language->clear();
    ui->language->addItem(tr("All"), -1);
    ui->language->setCurrentIndex(0);
    ui->findLanguage->clear();
    ui->findLanguage->addItem(tr("All"), -1);
    ui->findLanguage->setCurrentIndex(0);

    auto settings = GetSettings();
    QString sCurrentLanguage = settings->value(u"BookLanguage"_s, u"*"_s).toString();
    QLocale locale;
    auto sLocale = locale.name().left(2);
    QString sSearchLanguage;
    if(g::options.bStorePosition)
        sSearchLanguage = settings->value(u"search.language"_s).toString();
    auto vLaguages = currentLib.vLaguages;
    std::ranges::sort(vLaguages, [&sLocale](auto i1, auto i2)
    {
        bool bFavorite1 = contains(g::options.vFavoriteLanguges, i1);
        bool bFavorite2 = contains(g::options.vFavoriteLanguges, i2);

        if(!(bFavorite1 & bFavorite2)){
            if(bFavorite1)
                return true;
            if(bFavorite2)
                return false;
        }

        if(i1 == sLocale)
            return true;
        if(i2 == sLocale)
            return false;

        QString sLanguage1;
        if(mLanguges.contains(i1))
            sLanguage1 = mLanguges.at(i1);
        else
            sLanguage1 = i1;
        QString sLanguage2;
        if(mLanguges.contains(i2))
            sLanguage2 = mLanguges.at(i2);
        else
            sLanguage2 = i2;

        return localeStringCompare(sLanguage1, sLanguage2);
    });

    int iCurrentLang = 0;
    int iSearchLang = 0;
    for(int iLang = 0; iLang<vLaguages.size(); iLang++){
        QString sAbbrLanguage = vLaguages[iLang];
        if(!sAbbrLanguage.isEmpty() && sAbbrLanguage != u"un"){
            QString sLanguage;
            if(mLanguges.contains(sAbbrLanguage))
                sLanguage = mLanguges.at(sAbbrLanguage);
            else
                sLanguage = sAbbrLanguage;

            auto it = std::ranges::find(currentLib.vLaguages, sAbbrLanguage);
            if(it != currentLib.vLaguages.end()){
                int id = it - currentLib.vLaguages.begin();
                ui->language->addItem(sLanguage, id);
                ui->findLanguage->addItem(sLanguage, id);

                if(sAbbrLanguage == sCurrentLanguage){
                    iCurrentLang = iLang + 1;
                    idCurrentLanguage_ = id;
                }

                if(sAbbrLanguage == sSearchLanguage)
                    iSearchLang = iLang + 1;
            }
        }
    }

    if(!g::options.vFavoriteLanguges.empty()){
        int iSeparator = std::ranges::count_if(g::options.vFavoriteLanguges, [&vLaguages](auto sLang)
            { return contains(vLaguages, sLang); }) + 1;
        ui->language->insertSeparator(iSeparator);
        ui->findLanguage->insertSeparator(iSeparator);
    }
    ui->language->setCurrentIndex(iCurrentLang);
    ui->findLanguage->setCurrentIndex(iSearchLang);

#ifdef USE_HTTSERVER
    if(pOpds_)
        pOpds_->setLanguageFilter(sCurrentLanguage);
#endif
    settings->setValue(u"BookLanguage"_s, sCurrentLanguage);
    ui->language->blockSignals(false);
    ui->findLanguage->blockSignals(false);
    QApplication::restoreOverrideCursor();
}

void MainWindow::ManageLibrary()
{
    SaveLibPosition();
    uint idOldLib = g::idCurrentLib;
    LibrariesDlg al(this);
    al.exec();
    if(al.bLibChanged){
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        QFuture<void> future;
        if(g::idCurrentLib != idOldLib){
            auto &oldLib = g::libs[idOldLib];
            if(oldLib.timeHttp + 24h < std::chrono::system_clock::now()){
                future = QtConcurrent::run([&oldLib]()
                {
                    oldLib.bLoaded = false;
                    oldLib.books.clear();
                    oldLib.authors.clear();
                    oldLib.serials.clear();
                    oldLib.authorBooksLink.clear();
                    oldLib.vLaguages.clear();
                });
            }
        }
        ui->date_from->setDate( g::libs[g::idCurrentLib].earliestDate_ );
        ui->find_books->clear();
        loadLibrary(g::idCurrentLib);
        vFoundBooks_.clear();

        fillLanguages();
        FillAuthors();
        FillSerials();
        FillGenres();
        switch(ui->tabWidget->currentIndex()){
        case TabAuthors:
            onSerachAuthorsChanded(ui->searchAuthor->text());
            SelectAuthor();
            break;
        case TabSeries:
            onSerachSeriesChanded(ui->searchSeries->text());
            SelectSeria();
            break;
        case TabGenres:
            SelectGenre();
            break;
        case TabSearch:
            ui->Books->clear();
        }
        future.waitForFinished();
        QGuiApplication::restoreOverrideCursor();
    }
    updateTitle();
    FillLibrariesMenu();
}

/*
    окно статистики библиотеки
*/
void MainWindow::onStatistics()
{
    StatisticsDialog dlg;
    dlg.exec();
}

void MainWindow::onAddBooks()
{
    SLib &lib = g::libs[g::idCurrentLib];

    QStringList listFiles = QFileDialog::getOpenFileNames(this, tr("Select books to add"), lib.path,
                                                          tr("Books") + u" (*.fb2 *.epub *.zip)"_s, nullptr, QFileDialog::ReadOnly);
    if(!listFiles.isEmpty()){
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

        for(auto &sFile :listFiles){
            if(!sFile.startsWith(lib.path)){
                //перенос файлов книг в папку библиотеки
                QFileInfo fiSrc = QFileInfo(sFile);
                QString baseName = fiSrc.baseName(); // получаем имя файла без расширения
                QString extension = fiSrc.completeSuffix(); // получаем расширение файла
                QFileInfo fiDst = QFileInfo(lib.path % "/" % baseName % "." % extension);
                uint j = 1;
                while(fiDst.exists()){
                    QString sNewName = QStringLiteral("%1 (%2).%3").arg(baseName, QString::number(j), extension);
                    fiDst = QFileInfo(lib.path % u"/"_s % sNewName);
                    j++;
                }
                QFile::copy(sFile, fiDst.absoluteFilePath());
                sFile = fiDst.absoluteFilePath();
            }
        }
        pThread_ = new QThread;
        pImportThread_ = new ImportThread();
        pImportThread_->init(g::idCurrentLib, lib, listFiles);
        pImportThread_->moveToThread(pThread_);
        connect(pThread_, &QThread::started, pImportThread_, &ImportThread::process);
        connect(pImportThread_, &ImportThread::End, pThread_, &QThread::quit);
        connect(pThread_, &QThread::finished, this, &MainWindow::addBooksFinished);
        pThread_->start();
    }
}

void MainWindow::addBooksFinished()
{
    pImportThread_->deleteLater();
    pThread_->deleteLater();
    loadLibrary(g::idCurrentLib);
    fillLanguages();
    FillAuthors();
    FillSerials();
    FillGenres();
    switch(ui->tabWidget->currentIndex()){
    case TabAuthors:
        onSerachAuthorsChanded(ui->searchAuthor->text());
        break;
    case TabSeries:
        onSerachSeriesChanded(ui->searchSeries->text());
        break;
    }
    QGuiApplication::restoreOverrideCursor();
}

void MainWindow::btnSearch()
{
    QToolButton *button = qobject_cast<QToolButton*>(sender());
    switch(ui->tabWidget->currentIndex()){
    case TabAuthors:
        ui->searchAuthor->setText(button->text());
        break;
    case TabSeries:
        ui->searchSeries->setText(button->text());
        break;
    }
}

void MainWindow::About()
{
    AboutDialog* dlg = new AboutDialog(this);
    dlg->exec();
    delete dlg;
}

void MainWindow::checkLetter(const QChar cLetter)
{
    const QList<QToolButton*> allButtons = findChildren<QToolButton *>();
    bool find = false;
    for(QToolButton *tb: allButtons)
    {
        QString sButtonLetter = tb->text();
        if(!sButtonLetter.isEmpty() && sButtonLetter.at(0) == cLetter)
        {
            find = true;
            tb->setChecked(true);
            break;
        }
    }
    if(!find)
        btn_Hash->setChecked(true);
}

void MainWindow::onSerachAuthorsChanded(const QString &str)
{
    if(str.length() == 0)
    {
        ui->searchAuthor->setText(last_search_symbol);
        ui->searchAuthor->selectAll();
    }
    else
    {
        last_search_symbol = str.left(1);
        if((last_search_symbol == u"*" || last_search_symbol == u"#" ) && str.length()>1)
        {
            ui->searchAuthor->setText(str.right(str.length()-1));
        }
        checkLetter(last_search_symbol.at(0).toUpper());
        FillAuthors();
    }
    if(ui->AuthorList->count() > 0){
        if(ui->AuthorList->selectedItems().count() == 0)
            ui->AuthorList->setCurrentRow(0);
        auto item = ui->AuthorList->selectedItems()[0];
        ui->AuthorList->scrollToItem(item, QAbstractItemView::EnsureVisible);
    }
    ui->searchAuthor->setClearButtonEnabled(ui->searchAuthor->text().length() > 1);
}

void MainWindow::onSerachSeriesChanded(const QString &str)
{
    if(str.length() == 0)
    {
        ui->searchSeries->setText(last_search_symbol);
        ui->searchSeries->selectAll();
    }
    else
    {
        last_search_symbol=str.left(1);
        if((last_search_symbol == u"*" || last_search_symbol == u"#") && str.length()>1)
        {
            ui->searchSeries->setText(str.right(str.length()-1));
        }
        checkLetter(last_search_symbol.at(0).toUpper());
        FillSerials();
    }
    if(ui->SeriaList->count() > 0){
        if(ui->SeriaList->selectedItems().count() == 0)
            ui->SeriaList->setCurrentRow(0);
        auto item = ui->SeriaList->selectedItems()[0];
        ui->SeriaList->scrollToItem(item, QAbstractItemView::EnsureVisible);
    }
    ui->searchSeries->setClearButtonEnabled(ui->searchSeries->text().length()>1);
}

void MainWindow::HelpDlg()
{
    if(pHelpDlg == nullptr)
        pHelpDlg = new HelpDialog();
    pHelpDlg->show();
}

void MainWindow::ContextMenu(QPoint point)
{
    auto sender = QObject::sender();
    if(sender == ui->Books && !ui->Books->itemAt(point))
        return;
    if(sender == ui->AuthorList && !ui->AuthorList->itemAt(point))
        return;
    if(sender == ui->SeriaList && !ui->SeriaList->itemAt(point))
        return;
    QMenu menu;
    currentListForTag_ = sender;
    QList<QTreeWidgetItem*> listItems;
    if(QObject::sender() == ui->Books)
    {
        QMenu *save = menu.addMenu(tr("Save as"));
        listItems  = checkedItemsBookList();
        if(listItems.isEmpty())
            listItems = ui->Books->selectedItems();
        const auto actions = ui->btnExport->menu()->actions();
        for( const QAction* i: actions )
        {
            QAction *action = new QAction(i->text(), save);
            action->setData(i->data().toInt());
            connect(action, &QAction::triggered, this, &MainWindow::ExportAction);
            save->addAction(action);
        }
        if(listItems.size() == 1){
            auto item = listItems.at(0);
            if(item->type() == ITEM_TYPE_BOOK){
                QMenu *rate = menu.addMenu(tr("Mark"));
                auto starSize = 16;//rate->fontInfo().pixelSize();
                QString sStyle = u"QWidget:hover {background: %1;}"_s.arg(palette().color(QPalette::QPalette::Highlight).name());
                for(int i=0; i<=5; i++){
                    QWidgetAction *labelAction = new QWidgetAction(rate);
                    QLabel *label = new QLabel(rate);
                    label->setStyleSheet(sStyle);
                    label->setMargin(starSize/3);
                    QPixmap mypix (QIcon(u":/img/icons/stars/%1star.svg"_s.arg(i)).pixmap(QSize(starSize*5, starSize)));
                    label->setPixmap(mypix);
                    labelAction->setDefaultWidget(label);
                    connect(labelAction, &QAction::triggered, this, [this, item, i](){onSetRating(item, i);});
                    rate->addAction(labelAction);
                }
            }
        }
    }
    if(menu.actions().count() > 0)
        menu.addSeparator();
    if(g::options.bUseTag){
        SLib &lib = g::libs[g::idCurrentLib];
        if(sender == ui->Books){
            std::unordered_map<uint, uint> countTags;
            std::unordered_set<uint> idTags;
            for(const auto item : std::as_const(listItems)){
                uint id = item->data(0, Qt::UserRole).toUInt();
                switch(item->type()){
                case ITEM_TYPE_BOOK:
                    idTags = lib.books[id].idTags;
                    break;
                case ITEM_TYPE_SERIA:
                    idTags = lib.serials[id].idTags;
                    break;
                case ITEM_TYPE_AUTHOR:
                    idTags = lib.authors[id].idTags;
                }
                for(auto idTag : idTags){
                    countTags[idTag]++;
                }
            }
            const auto actions = TagMenu.actions();
            for(const auto action :actions){
                uint idTag = action->data().toUInt();
                action->setChecked( countTags.contains(idTag) );
            }
        }
        else
        if(sender == ui->AuthorList){
            const auto actions = TagMenu.actions();
            for(const auto action :actions){
                uint idTag = action->data().toUInt();
                uint idAuthor = ui->AuthorList->itemAt(point)->data(Qt::UserRole).toUInt();
                action->setChecked( lib.authors[idAuthor].idTags.contains(idTag) );
            }
        }
        else
        if(sender == ui->SeriaList){
            const auto actions = TagMenu.actions();
            for(const auto action :actions){
                uint idTag = action->data().toUInt();
                uint idSerial = ui->SeriaList->itemAt(point)->data(Qt::UserRole).toUInt();
                action->setChecked( lib.serials[idSerial].idTags.contains(idTag) );
            }
        }

        menu.addActions(TagMenu.actions());
    }
    if(menu.actions().count() > 0)
        menu.exec(QCursor::pos());
}

void MainWindow::HeaderContextMenu(QPoint /*point*/)
{
    QMenu menu;
    QAction *action;

    action = new QAction(tr("Name"), this);
    action->setCheckable(true);
    action->setChecked(!ui->Books->isColumnHidden(0));
    connect(action, &QAction::triggered,this, [action, this]{ui->Books->setColumnHidden(0, !action->isChecked());});
    menu.addAction(action);

    action = new QAction(tr("Author"), this);
    action->setCheckable(true);
    action->setChecked(!ui->Books->isColumnHidden(1));
    connect(action, &QAction::triggered,this, [action, this]{ui->Books->setColumnHidden(1, !action->isChecked());});
    menu.addAction(action);

    action = new QAction(tr("Serial"), this);
    action->setCheckable(true);
    action->setChecked(!ui->Books->isColumnHidden(2));
    connect(action, &QAction::triggered,this, [action, this]{ui->Books->setColumnHidden(2, !action->isChecked());});
    menu.addAction(action);

    action = new QAction(tr("No."), this);
    action->setCheckable(true);
    action->setChecked(!ui->Books->isColumnHidden(3));
    connect(action, &QAction::triggered, this, [action, this]{ui->Books->setColumnHidden(3, !action->isChecked());});
    menu.addAction(action);

    action = new QAction(tr("Size"), this);
    action->setCheckable(true);
    action->setChecked(!ui->Books->isColumnHidden(4));
    connect(action, &QAction::triggered, this, [action, this]{ui->Books->setColumnHidden(4, !action->isChecked());});
    menu.addAction(action);

    action = new QAction(tr("Mark"), this);
    action->setCheckable(true);
    action->setChecked(!ui->Books->isColumnHidden(5));
    connect(action, &QAction::triggered, this, [action, this]{ui->Books->setColumnHidden(5, !action->isChecked());});
    menu.addAction(action);

    action = new QAction(tr("Import date"), this);
    action->setCheckable(true);
    action->setChecked(!ui->Books->isColumnHidden(6));
    connect(action, &QAction::triggered, this, [action, this]{ui->Books->setColumnHidden(6, !action->isChecked());});
    menu.addAction(action);

    action = new QAction(tr("Genre"), this);
    action->setCheckable(true);
    action->setChecked(!ui->Books->isColumnHidden(7));
    connect(action, &QAction::triggered, this, [action, this]{ui->Books->setColumnHidden(7, !action->isChecked());});
    menu.addAction(action);

    action = new QAction(tr("Language"), this);
    action->setCheckable(true);
    action->setChecked(!ui->Books->isColumnHidden(8));
    connect(action, &QAction::triggered, this, [action, this]{ui->Books->setColumnHidden(8, !action->isChecked());});
    menu.addAction(action);

    action = new QAction(tr("Format"), this);
    action->setCheckable(true);
    action->setChecked(!ui->Books->isColumnHidden(9));
    connect(action, &QAction::triggered, this, [action, this]{ui->Books->setColumnHidden(9, !action->isChecked());});
    menu.addAction(action);

    menu.exec(QCursor::pos());
}

void MainWindow::MoveToSeria(uint id, const QString &FirstLetter)
{
    ui->searchSeries->setText(FirstLetter);
    ui->tabWidget->setCurrentIndex(TabSeries);
    ui->SeriaList->clearSelection();
    for (int i=0; i<ui->SeriaList->count(); i++)
    {
        if(ui->SeriaList->item(i)->data(Qt::UserRole).toUInt() == id)
        {
            ui->SeriaList->item(i)->setSelected(true);
            ui->SeriaList->scrollToItem(ui->SeriaList->item(i));
            SelectSeria();
            return;
        }
    }
}

void MainWindow::MoveToGenre(uint id)
{
    ui->tabWidget->setCurrentIndex(TabGenres);
    ui->GenreList->clearSelection();
    for (int i=0; i<ui->GenreList->topLevelItemCount(); i++)
    {
        for (int j=0; j<ui->GenreList->topLevelItem(i)->childCount(); j++)
        {
            if(ui->GenreList->topLevelItem(i)->child(j)->data(0, Qt::UserRole).toUInt() == id)
            {
                ui->GenreList->topLevelItem(i)->child(j)->setSelected(true);
                ui->GenreList->scrollToItem(ui->GenreList->topLevelItem(i)->child(j));
                SelectGenre();
                return;
            }
        }
    }
}

void MainWindow::MoveToAuthor(uint id, const QString &FirstLetter)
{
    ui->searchAuthor->setText(FirstLetter);
    ui->tabWidget->setCurrentIndex(TabAuthors);
    ui->AuthorList->clearSelection();
    for (int i=0; i<ui->AuthorList->count(); i++)
    {
        if(ui->AuthorList->item(i)->data(Qt::UserRole).toUInt() == id)
        {
            ui->AuthorList->item(i)->setSelected(true);
            ui->AuthorList->scrollToItem(ui->AuthorList->item(i));
            SelectAuthor();
            break;
        }
    }
}

void MainWindow::FillLibrariesMenu()
{
    if(!QSqlDatabase::database(u"libdb"_s, false).isOpen())
        return;
    QMenu *menu = ui->actionLibraries->menu();
    if(menu){
        menu->clear();
    }else{
        menu = new QMenu(this);
        ui->actionLibraries->setMenu(menu);
        ui->btnLibrary->setMenu(menu);
    }

    for(const auto &iLib :g::libs){
        uint idLib = iLib.first;
        if(idLib>0){
            QAction *action = new QAction(iLib.second.name, menu);
            action->setData(idLib);
            action->setCheckable(true);
            menu->insertAction(nullptr, action);
            pLibGroup_->addAction(action);
            connect(action, &QAction::triggered, this, &MainWindow::SelectLibrary);
            if(idLib == g::idCurrentLib)
                action->setChecked(true);
        }
    }
    menu->setEnabled( menu->actions().count()>1 );
    ui->actionLibraries->setEnabled( menu->actions().count()>0 );
}

void MainWindow::FillAuthors()
{
    if(g::idCurrentLib == 0)
        return;
    qint64 timeStart;
    if(g::bVerbose)
        timeStart = QDateTime::currentMSecsSinceEpoch();
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    const bool wasBlocked = ui->AuthorList->blockSignals(true);
    ui->AuthorList->clear();
    SLib &currentLib = g::libs[g::idCurrentLib];
    auto &authors = currentLib.authors;
    QListWidgetItem *selectedItem = nullptr;
    QString sSearch = ui->searchAuthor->text();
    static const QRegularExpression re(QStringLiteral("[A-Za-zа-яА-ЯЁё]"));
    std::vector<uint> vIdAuthors;

    vIdAuthors = blockingFiltered(authors, [&](auto &author){
        QString sName  = author.getName();
        return (sSearch == u"*" || (sSearch == u"#" && !sName.left(1).contains(re)) || sName.startsWith(sSearch, Qt::CaseInsensitive));
    });

    std::vector<QListWidgetItem*> vItems;
    std::mutex m;
    QtConcurrent::blockingMap(vIdAuthors, [&](uint idAuthor){
        auto &author = authors.at(idAuthor);
        int count = 0;
        for (auto it = currentLib.authorBooksLink.equal_range(idAuthor); it.first != it.second; ++it.first) {
            SBook &book = currentLib.books[it.first->second];
            if( IsBookInList(book))
                count++;
        }
        if(count>0){
            QListWidgetItem *item = new QListWidgetItem(author.getName() % u" ("_s % QString::number(count) % u")"_s);
            if(g::options.bUseTag)
                item->setIcon( getTagIcon(authors.at(idAuthor).idTags) );

            item->setData(Qt::UserRole, idAuthor);
            if(g::options.bUseTag)
                item->setIcon(getTagIcon(author.idTags));
            if(idCurrentAuthor_ == idAuthor)
                selectedItem = item;

            {
                std::lock_guard<std::mutex> guard(m);
                vItems.push_back(item);
            }
        }
    });

#ifdef __cpp_lib_execution
    std::sort(g::executionpolicy, vItems.begin(), vItems.end(), [](auto item1, auto item2){
#else
    std::sort(vItems.begin(), vItems.end(), [](auto item1, auto item2){
#endif
        return localeStringCompare(item1->text(), item2->text());
    });

    for(auto item :vItems){
        ui->AuthorList->addItem(item);
        if(item == selectedItem)
             item->setSelected(true);
    }

    if(selectedItem != nullptr){
        ui->AuthorList->setCurrentItem(selectedItem);
    }

    ui->AuthorList->blockSignals(wasBlocked);
    if(g::bVerbose){
        qint64 timeEnd = QDateTime::currentMSecsSinceEpoch();
        qDebug()<< "FillAuthors " << timeEnd - timeStart << "msec";
    }
    QApplication::restoreOverrideCursor();
}

void MainWindow::FillSerials()
{
    if(g::idCurrentLib == 0)
        return;
    qint64 timeStart;
    if(g::bVerbose)
        timeStart = QDateTime::currentMSecsSinceEpoch();
    const bool wasBlocked = ui->SeriaList->blockSignals(true);
    ui->SeriaList->clear();
    QString sSearch = ui->searchSeries->text();
    static const QRegularExpression re(u"[A-Za-zа-яА-ЯЁё]"_s);

    std::unordered_map<uint, uint> mCounts;
    const SLib& lib = g::libs[g::idCurrentLib];
    auto &squences = lib.serials;

    for(const auto &iBook :lib.books){
        auto &book = iBook.second;
        if(!book.mSequences.empty() && IsBookInList(book)){
            for(const auto &iSequence: book.mSequences){
                uint idSequence = iSequence.first;
                if((sSearch == u"*" || (sSearch == u"#" && !squences.at(idSequence).sName.left(1).contains(re)) ||
                     squences.at(idSequence).sName.startsWith(sSearch, Qt::CaseInsensitive)))
                {
                    mCounts[idSequence]++;
                }
            }
        }
    }

    std::vector<uint> vIdSequence(mCounts.size());
    std::ranges::copy(mCounts | std::views::keys, vIdSequence.begin());
#ifdef __cpp_lib_execution
    std::sort(g::executionpolicy, vIdSequence.begin(), vIdSequence.end(), [&squences](uint id1, uint id2)
#else
    std::sort(vIdSequence.begin(), vIdSequence.end(), [&squences](uint id1, uint id2)
#endif
    {
        return localeStringCompare(squences.at(id1).sName, squences.at(id2).sName);
    });

    QListWidgetItem *selectedItem = nullptr;

    for(auto idSequnce :vIdSequence)
    {
        QListWidgetItem *item = new QListWidgetItem(u"%1 (%2)"_s.arg(squences.at(idSequnce).sName).arg(mCounts.at(idSequnce)));
        item->setData(Qt::UserRole, idSequnce);
        if(g::options.bUseTag)
            item->setIcon(getTagIcon(squences.at(idSequnce).idTags));
        ui->SeriaList->addItem(item);
        if(idSequnce == idCurrentSerial_)
        {
            selectedItem = item;
            item->setSelected(true);
        }
    }
    if(selectedItem != nullptr)
        ui->SeriaList->setCurrentItem(selectedItem);

    ui->SeriaList->blockSignals(wasBlocked);
    if(g::bVerbose){
        qint64 timeEnd = QDateTime::currentMSecsSinceEpoch();
        qDebug()<< "FillSerials " << timeEnd - timeStart << "msec";
    }
}

void MainWindow::FillGenres()
{
    if(g::idCurrentLib == 0)
        return;
    qint64 timeStart;
    if(g::bVerbose)
        timeStart = QDateTime::currentMSecsSinceEpoch();
    const bool wasBlocked = ui->GenreList->blockSignals(true);
    ui->GenreList->clear();
    ui->s_genre->clear();
    ui->s_genre->addItem(u"*"_s, 0);
    QFont fontBold(ui->AuthorList->font());
    fontBold.setBold(true);

    std::unordered_map<ushort, uint> mCounts;
    SLib& lib = g::libs[g::idCurrentLib];

    for(const auto &book :lib.books){
        if(IsBookInList(book.second))
        {
            for(auto iGenre: book.second.vIdGenres)
                    mCounts[iGenre]++;
        }
    }

    uint idSearchGenre = 0;
    if(g::options.bStorePosition){
        auto settings = GetSettings();
        idSearchGenre = settings->value(u"search.id.genre"_s).toUInt();
    }
    std::unordered_map<ushort, QTreeWidgetItem*> mTopGenresItem;
    for(const auto &iGenre :mCounts){
        auto idGenre = iGenre.first;
        auto idParrentGenre = g::genres[idGenre].idParrentGenre;
        QTreeWidgetItem *pTopItem;

        if(!mTopGenresItem.contains(idParrentGenre)){
            const QString &sTopName = g::genres[idParrentGenre].sName;
            pTopItem = new QTreeWidgetItem(ui->GenreList);
            pTopItem->setFont(0, fontBold);
            pTopItem->setText(0, sTopName);
            pTopItem->setData(0, Qt::UserRole, idParrentGenre);
            pTopItem->setExpanded(false);
            mTopGenresItem[idParrentGenre] = pTopItem;
            ui->s_genre->addItem(sTopName, idParrentGenre);
        }else
            pTopItem = mTopGenresItem[idParrentGenre];

        QTreeWidgetItem *item = new QTreeWidgetItem(pTopItem);
        const QString &sName = g::genres[idGenre].sName;
        item->setText(0, u"%1 (%2)"_s.arg(sName).arg(mCounts[idGenre]));
        item->setData(0, Qt::UserRole, idGenre);
        ui->s_genre->addItem(u"   "_s + sName, idGenre);
        if(idGenre == idCurrentGenre_)
        {
            item->setSelected(true);
            ui->GenreList->scrollToItem(item);
        }
    }

    ui->s_genre->model()->sort(0);
    ui->GenreList->model()->sort(0);

    for(int i=0; i<ui->s_genre->count(); i++){
        if(ui->s_genre->itemData(i) == idSearchGenre){
            ui->s_genre->setCurrentIndex(i);
            break;
        }
    }

    ui->GenreList->blockSignals(wasBlocked);
    if(g::bVerbose){
        qint64 timeEnd = QDateTime::currentMSecsSinceEpoch();
        qDebug()<< "FillGenres " << timeEnd - timeStart << "msec";
    }
}

void MainWindow::FillListBooks()
{
    switch(ui->tabWidget->currentIndex()){
        case TabAuthors:
            SelectAuthor();
        break;

        case TabSeries:
            SelectSeria();
        break;

        case TabGenres:
            SelectGenre();
        break;
    }
}

void MainWindow::FillListBooks(const std::vector<uint> &vBooks, const std::vector<uint> &vCheckedBooks, uint idCurrentAuthor)
{
    if(g::idCurrentLib == 0)
        return;
    qint64 timeStart;
    if(g::bVerbose)
        timeStart = QDateTime::currentMSecsSinceEpoch();
    QFont bold_font(ui->Books->font());
    bold_font.setBold(true);
    TreeBookItem* ScrollItem = nullptr;
    QLocale locale;

    TreeBookItem* item_seria = nullptr;
    TreeBookItem* item_author;
    std::unordered_map<uint, TreeBookItem*> mAuthorItems;
    std::unordered_multimap<uint, TreeBookItem*> mSeriaItems;

    const bool wasBlocked = ui->Books->blockSignals(true);
    ui->Books->clear();
    SLib &lib = g::libs[g::idCurrentLib];

    bool bSequenceInTree;
    uint idCurrentSequence;
    if(ui->tabWidget->currentIndex() == 1){
        bSequenceInTree = false;
        QListWidgetItem* cur_item = ui->SeriaList->selectedItems().at(0);
        idCurrentSequence = cur_item->data(Qt::UserRole).toUInt();
    }else
        bSequenceInTree = true;

    for( uint idBook: vBooks) {
        SBook &book = lib.books[idBook];
        std::vector<TreeBookItem*> itemsBook;
        uint idAuthor;
        QString sAuthor;
        bool bBookChecked = contains(vCheckedBooks, idBook);
        if(idCurrentAuthor > 0)
            idAuthor = idCurrentAuthor;
        else
            idAuthor = book.idFirstAuthor;
        sAuthor = lib.authors[idAuthor].getName();

        if(bTreeView_){
            if(!mAuthorItems.contains(idAuthor)){
                item_author = new TreeBookItem(ui->Books, ITEM_TYPE_AUTHOR);
                item_author->setText(0, sAuthor);
                item_author->setExpanded(true);
                item_author->setFont(0, bold_font);
                item_author->setCheckState(0, bBookChecked ?Qt::Checked  :Qt::Unchecked);
                item_author->setData(0, Qt::UserRole, idAuthor);
                item_author->setData(3, Qt::UserRole, -1);
                item_author->setData(5, Qt::UserRole, -1);

                if(g::options.bUseTag){
                    item_author->setIcon(0, getTagIcon(lib.authors[idAuthor].idTags));
                }
                mAuthorItems[idAuthor] = item_author;
            }else{
                item_author = mAuthorItems[idAuthor];
                auto checkedState = item_author->checkState(0);
                if((checkedState == Qt::Checked && !bBookChecked) || (checkedState == Qt::Unchecked && bBookChecked))
                    item_author->setCheckState(0, Qt::PartiallyChecked);
            }

            if(!bSequenceInTree){
                itemsBook.push_back(new TreeBookItem(item_author, ITEM_TYPE_BOOK));
            }else{
                if(!book.mSequences.empty()){
                    for(auto [idSequence, numInSequenc] :book.mSequences){
                        bool bFound = false;
                        auto items = mSeriaItems | std::views::filter([idSequence](const auto& pair){return pair.first == idSequence;});
                        for (const auto& [id, item] :items) {
                            if(item->parent()->data(0, Qt::UserRole) == idAuthor){
                                auto checkedState = item->checkState(0);
                                if((checkedState == Qt::Checked && !bBookChecked) || (checkedState == Qt::Unchecked && bBookChecked))
                                    item->setCheckState(0, Qt::PartiallyChecked);
                                bFound = true;
                                item_seria = item;
                                break;
                            }
                        }
                        if(!bFound){
                            item_seria = new TreeBookItem(mAuthorItems[idAuthor], ITEM_TYPE_SERIA);
                            item_seria->setText(0, lib.serials[idSequence].sName);
                            item_author->addChild(item_seria);
                            item_seria->setExpanded(true);
                            item_seria->setFont(0, bold_font);
                            item_seria->setCheckState(0, bBookChecked ?Qt::Checked  :Qt::Unchecked);
                            item_seria->setData(0, Qt::UserRole, idSequence);
                            item_seria->setData(3, Qt::UserRole, -1);
                            item_seria->setData(5, Qt::UserRole, -1);
                            if(g::options.bUseTag)
                                item_seria->setIcon(0, getTagIcon(lib.serials[idSequence].idTags));
                            mSeriaItems.insert({idSequence, item_seria});
                        }
                        itemsBook.push_back(new TreeBookItem(item_seria, ITEM_TYPE_BOOK));
                    }
                }else{
                    itemsBook.push_back(new TreeBookItem(item_author, ITEM_TYPE_BOOK));
                }
            }
        }else
            itemsBook.push_back(new TreeBookItem(ui->Books, ITEM_TYPE_BOOK));

        for(auto itemBook :itemsBook){
            itemBook->setCheckState(0, bBookChecked ?Qt::Checked  :Qt::Unchecked);
            itemBook->setData(0, Qt::UserRole, idBook);
            if(g::options.bUseTag){
                itemBook->setIcon(0, getTagIcon(book.idTags));
            }

            itemBook->setText(0, book.sName);

            itemBook->setText(1, sAuthor);

            uint numInSequence = 0;
            QString sSequence;
            if(bTreeView_){
                if(!bSequenceInTree){
                    sSequence = lib.serials[idCurrentSequence].sName;
                    numInSequence = book.mSequences[idCurrentSequence];

                }else{
                    TreeBookItem* parrentItem = static_cast<TreeBookItem*>(itemBook->parent());
                    if(parrentItem && parrentItem->type() == ITEM_TYPE_SERIA){
                        uint idSequence = parrentItem->data(0, Qt::UserRole).toUInt();
                        sSequence = lib.serials[idSequence].sName;
                        numInSequence = book.mSequences[idSequence];
                    }
                }
            }else{
                if(book.mSequences.size() == 1)
                    numInSequence = book.mSequences.begin()->second;
                bool bFirst = true;
                for(auto [idSequence, numInSequence] :book.mSequences){
                    if(!bFirst)
                        sSequence += u"; "_s;
                    sSequence += lib.serials[idSequence].sName;
                    bFirst = false;
                }

            }
            if(!sSequence.isEmpty())
                itemBook->setText(2, sSequence);
            if(numInSequence>0){
                itemBook->setText(3, QString::number(numInSequence));
                itemBook->setTextAlignment(3, Qt::AlignRight|Qt::AlignVCenter);
            }

            if(book.nSize>0){
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
                itemBook->setText(4, locale.formattedDataSize(book.nSize, 1, QLocale::DataSizeTraditionalFormat));
#else
                itemBook->setText(4, sizeToString(book.nSize));

#endif
                itemBook->setTextAlignment(4, Qt::AlignRight|Qt::AlignVCenter);
            }

            itemBook->setData(5, Qt::UserRole, book.nStars);

            itemBook->setText(6, book.date.toString(u"dd.MM.yyyy"_s));
            itemBook->setTextAlignment(6, Qt::AlignCenter);

            if(!book.vIdGenres.empty()){
                itemBook->setText(7, g::genres[book.vIdGenres.at(0)].sName);
                itemBook->setTextAlignment(7, Qt::AlignLeft|Qt::AlignVCenter);
            }

            itemBook->setText(8, lib.vLaguages[book.idLanguage]);
            itemBook->setTextAlignment(8, Qt::AlignCenter);

            itemBook->setText(9, book.sFormat);
            itemBook->setTextAlignment(9, Qt::AlignCenter);

            if(book.bDeleted)
            {
                QBrush brush(QColor::fromRgb(96, 96, 96));
                for (int i = 0; i != 7; ++i)
                    itemBook->setForeground(i, brush);
            }

            if(idBook == idCurrentBook_)
                ScrollItem = itemBook;
        }
    }
    if(bCollapsed_ && bTreeView_){
        ui->Books->collapseAll();
        if(mAuthorItems.size()==1){
            auto item = mAuthorItems.begin()->second;
            item->setExpanded(true);
        }
    }
    if(ScrollItem)
    {
        ScrollItem->setSelected(true);
        ui->Books->scrollToItem(ScrollItem);
    }
    selectBook();

    ui->Books->blockSignals(wasBlocked);
    if(g::bVerbose){
        qint64 timeEnd = QDateTime::currentMSecsSinceEpoch();
        qDebug()<< "FillListBooks " << timeEnd - timeStart << "msec";
    }
}

bool MainWindow::IsBookInList(const SBook &book)
{

    uint idTagFilter = ui->TagFilter->itemData(ui->TagFilter->currentIndex()).toUInt();
    if(idCurrentLanguage_ != -1 && idCurrentLanguage_ != book.idLanguage)
        return false;
    if(!g::options.bShowDeleted && book.bDeleted)
        return false;
    if(g::options.bUseTag && idTagFilter != 0){
        if(book.idTags.contains(idTagFilter))
            return true;

        const auto& lib = g::libs[g::idCurrentLib];
        if(std::ranges::any_of(book.mSequences, [&lib, idTagFilter](const auto& seq)
                                { return lib.serials.at(seq.first).idTags.contains(idTagFilter); }))
            return true;

        return std::ranges::any_of(book.vIdAuthors, [&lib, idTagFilter](uint authorId)
                                   { return lib.authors.at(authorId).idTags.contains(idTagFilter); });
    }
    return true;
}

void MainWindow::UpdateExportMenu()
{
    bool bKindlegenInstalled = kindlegenInstalled();
    QMenu* menu = ui->btnExport->menu();
    if(menu)
    {
        ui->btnExport->menu()->clear();
    }
    else
    {
        menu = new QMenu(this);
        ui->btnExport->setMenu(menu);
    }
    //ui->btnExport->setDefaultAction(nullptr);
    int count = g::options.vExportOptions.size();
    for(int i = 0; i<count; i++)
    {
        const ExportOptions &exportOptions = g::options.vExportOptions.at(i);
        if(bKindlegenInstalled ||
            (exportOptions.format != mobi && exportOptions.format != azw3 && exportOptions.format != mobi7))
        {
            QAction *action = new QAction(exportOptions.sName, menu);
            action->setData(i);

            menu->addAction(action);
            if(exportOptions.bDefault)
            {
                ui->btnExport->setDefaultAction(action);
            }
        }
    }
    if(count == 0)
    {
       QAction *action = new QAction(tr("Send to ..."), this);
       action->setData(-1);
       menu->addAction(action);
       ui->btnExport->setDefaultAction(action);
    }
    if(menu->actions().count() == 0)
    {
        return;
    }
    if(!ui->btnExport->defaultAction())
    {
        ui->btnExport->setDefaultAction(menu->actions().constFirst());
    }
    const auto actions = menu->actions();
    for(const QAction *action: actions)
    {
        connect(action, &QAction::triggered, this, &MainWindow::ExportAction);
    }
    QFont font(ui->btnExport->defaultAction()->font());
    font.setBold(true);
    ui->btnExport->defaultAction()->setFont(font);
    ui->btnExport->setIcon(themedIcon(u"document-send"_s));

    //костыль предотвращающий изменение иконки
    connect(ui->btnExport->defaultAction(), &QAction::changed, this, [this](){
        ui->btnExport->setIcon(themedIcon(u"document-send"_s));
    });
    ui->btnExport->setEnabled(ui->Books->selectedItems().count() > 0);
}

void MainWindow::ExportAction()
{
    int id = qobject_cast<QAction*>(sender())->data().toInt();
    if(g::options.vExportOptions.at(id).sSendTo == QStringLiteral("device"))
       SendToDevice(g::options.vExportOptions.at(id));
   else
       SendMail(g::options.vExportOptions.at(id));
}

void MainWindow::onLanguageFilterChanged(int index)
{
    SLib &currentLib = g::libs[g::idCurrentLib];
    int iLang = ui->language->itemData(index).toInt();
    QString sLanguage;
    if(iLang == -1)
        sLanguage = u"*"_s;
    else
        sLanguage = currentLib.vLaguages[iLang];
    auto settings = GetSettings();
    settings->setValue(u"BookLanguage"_s, sLanguage);
    idCurrentLanguage_ = ui->language->itemData(index).toInt();

    FillSerials();
    FillAuthors();
    FillGenres();
    FillListBooks();
#ifdef USE_HTTSERVER
    if(pOpds_)
        pOpds_->setLanguageFilter(sLanguage);
#endif
}

void MainWindow::onChangeAlpabet(const QString &sAlphabetName)
{
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
    FillAlphabet(sAlphabetName);
    FirstButton->click();
}

void MainWindow::onTreeView()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    ui->btnTreeView->setChecked(true);
    ui->btnListView->setChecked(false);
    if(!bTreeView_){
        bTreeView_ = true;
        std::vector<uint> vCheckedBooks = getCheckedBooks(true);
        auto currentTab = ui->tabWidget->currentIndex();
        if(ui->tabWidget->currentIndex() != TabSearch)
            FillListBooks(vBooks_, vCheckedBooks, currentTab==TabAuthors ? idCurrentAuthor_ :0);
        else
            FillListBooks(vFoundBooks_, vCheckedBooks, 0);
        auto settings = GetSettings();
        settings->setValue(u"TreeView"_s, true);
        aHeadersList_ = ui->Books->header()->saveState();
        ui->Books->header()->restoreState(aHeadersTree_);
        ui->Books->header()->setSortIndicatorShown(true);
        ui->btnCollapseAll->show();
        ui->btnExpandAll->show();
    }
    QApplication::restoreOverrideCursor();
}

void MainWindow::onListView()
{
    ui->btnTreeView->setChecked(false);
    ui->btnListView->setChecked(true);
    if(bTreeView_){
        bTreeView_ = false;
        std::vector<uint> vCheckedBooks = getCheckedBooks(true);
        auto currentTab = ui->tabWidget->currentIndex();
        if(ui->tabWidget->currentIndex() != TabSearch)
            FillListBooks(vBooks_, vCheckedBooks, currentTab==TabAuthors ? idCurrentAuthor_ :0);
        else
            FillListBooks(vFoundBooks_, vCheckedBooks, 0);
        auto settings = GetSettings();
        settings->setValue(QStringLiteral("TreeView"), false);
        aHeadersTree_ = ui->Books->header()->saveState();
        ui->Books->header()->restoreState(aHeadersList_);
        ui->Books->header()->setSortIndicatorShown(true);
        ui->btnCollapseAll->hide();
        ui->btnExpandAll->hide();
    }
}

void MainWindow::onCollapseAll()
{
    ui->Books->collapseAll();
    if(ui->Books->topLevelItemCount()==1){
        auto item = ui->Books->topLevelItem(0);
        if(item)
            item->setExpanded(true);
    }
    auto settings = GetSettings();
    settings->setValue(u"collapseAll"_s, true);
    bCollapsed_ = true;
}

void MainWindow::onExpandAll()
{
    ui->Books->expandAll();
    bCollapsed_ = false;
    auto settings = GetSettings();
    settings->setValue(u"collapseAll"_s, false);
}

void MainWindow::onUpdateListFont(const QFont &font)
{
    ui->AuthorList->setFont(font);
    ui->searchAuthor->setFont(font);
    ui->SeriaList->setFont(font);
    ui->searchSeries->setFont(font);
    ui->GenreList->setFont(font);
    ui->Books->setFont(font);

    QFont boldFont(font);
    boldFont.setBold(true);
    auto count = ui->Books->topLevelItemCount();
    for (int i = 0; i < count; ++i) {
        auto topItem = ui->Books->topLevelItem(i);
        if(topItem->font(0).bold())
            topItem->setFont(0, boldFont);
        auto childCount = topItem->childCount();
        for (int j = 0; j < childCount; ++j) {
            auto item = topItem->child(j);
            if(item->font(0).bold())
                item->setFont(0, boldFont);
        }
    }

    count = ui->GenreList->topLevelItemCount();
    for(int i = 0; i < count; ++i)
        ui->GenreList->topLevelItem(i)->setFont(0, boldFont);
}

void MainWindow::onUpdateAnnotationFont(const QFont &font)
{
    ui->Review->setFont(font);
}

void MainWindow::onSetRating(QTreeWidgetItem *item, uchar nRating)
{
    auto dbase = QSqlDatabase::database(QStringLiteral("libdb"), false);
    if(!dbase.isOpen())
        return;
    uint idBook = item->data(0, Qt::UserRole).toUInt();
    QSqlQuery query(dbase);
    query.prepare(QStringLiteral("UPDATE book SET star=:nRating WHERE id=:idBook;"));
    query.bindValue(QStringLiteral(":nRating"), nRating);
    query.bindValue(QStringLiteral(":idBook"), idBook);
    if(!query.exec())
        LogWarning << query.lastError().text();
    else{
        item->setData(5, Qt::UserRole, nRating);
        g::libs[g::idCurrentLib].books[idBook].nStars = nRating;
    }
}

void MainWindow::onTabWidgetChanged(int index)
{
    switch(index){
    case TabAuthors: //Авторы
    {
        QString sFilter = ui->searchAuthor->text();
        if(sFilter.isEmpty()){
            ui->searchAuthor->setText(FirstButton->text());
        }else {
            checkLetter(sFilter.at(0).toUpper());
        }
        ui->frame_3->setEnabled(true);
        ui->language->setEnabled(true);
        SelectAuthor();
    }
        break;
    case TabSeries: //Серии
    {
        QString sFilter = ui->searchSeries->text();
        if(sFilter.isEmpty()){
            ui->searchSeries->setText(FirstButton->text());
        }else {
            checkLetter(sFilter.at(0).toUpper());
        }
        ui->frame_3->setEnabled(true);
        ui->language->setEnabled(true);
        SelectSeria();
    }
        break;
    case TabGenres: //Жанры
        ui->frame_3->setEnabled(false);
        ui->language->setEnabled(true);
        SelectGenre();
        break;
    case TabSearch: //Поиск
        ui->frame_3->setEnabled(false);
        ui->language->setEnabled(false);
        FillListBooks(vFoundBooks_ ,{} , 0);
        ExportBookListBtn(false);
        break;
    }
}

void MainWindow::changingTrayIcon(int index, bool bColor)
{
    if (!g::bTray && index == 0)
    {
#ifdef USE_KStatusNotifier
        if(statusNotifierItem_) {
            statusNotifierItem_->deleteLater();
            statusNotifierItem_ = nullptr;
            pTrayMenu_ = nullptr;
            pHideAction_ = nullptr;
            pShowAction_ = nullptr;
        }
#else //USE_KStatusNotifier
        if(pTrayIcon_) {
            pTrayIcon_->hide();
            pTrayIcon_->deleteLater();
            pTrayIcon_ = nullptr;
        }

        if(pTrayMenu_) {
            pTrayMenu_->deleteLater();
            pTrayMenu_ = nullptr;
            pHideAction_ = nullptr;
            pShowAction_ = nullptr;
        }

#endif //USE_KStatusNotifier
    }
    else
    {
        // создание меню и действий
        if(!pTrayMenu_) {
            pTrayMenu_ = new QMenu(this);
            pHideAction_ = new QAction(QIcon::fromTheme(u"window-minimize"_s), tr("Minimize"), pTrayMenu_);
            pTrayMenu_->addAction(pHideAction_);
            pShowAction_ = new QAction(QIcon::fromTheme(u"window-restore"_s), tr("Open freeLib"), pTrayMenu_);
            pTrayMenu_->addAction(pShowAction_);

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
            QAction *exitAction = new QAction(QIcon::fromTheme(QIcon::ThemeIcon::ApplicationExit), tr("Quit"), pTrayMenu_);
#else
            QAction *exitAction = new QAction(QIcon::fromTheme(u"application-exit"_s), tr("Quit"), pTrayMenu_);
#endif
            pTrayMenu_->addAction(exitAction);

            connect(pHideAction_, &QAction::triggered, this, &MainWindow::hide);
            connect(pShowAction_, &QAction::triggered, this, &MainWindow::show);
            connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);

            // Установка видимости действий
            if(isVisible())
                pShowAction_->setVisible(false);
            else
                pHideAction_->setVisible(false);
        }

        // установка иконки
        QIcon icon;
        if(bColor)
            icon = QIcon(u":/img/tray0.png"_s);
        else
            icon = themedIcon(u"tray"_s);

#ifdef USE_KStatusNotifier
        if(!statusNotifierItem_) {
            statusNotifierItem_ = new KStatusNotifierItem(this);
            statusNotifierItem_->setStatus(KStatusNotifierItem::Active);
            statusNotifierItem_->setCategory(KStatusNotifierItem::ApplicationStatus);
            statusNotifierItem_->setTitle(u"freeLib"_s);
            statusNotifierItem_->setToolTipTitle(u"freeLib"_s);
            if (g::idCurrentLib != 0)
                statusNotifierItem_->setToolTipSubTitle(g::libs[g::idCurrentLib].name);
            statusNotifierItem_->setStandardActionsEnabled(false);

            // Подключение обработчика активации иконки трея для переключения видимости окна при запуске в свёрнутом виде
            if (!statusNotifierItem_->associatedWindow()) {
                connect(statusNotifierItem_, &KStatusNotifierItem::activateRequested, this, &MainWindow::handleTrayActivation);
            }

            statusNotifierItem_->setContextMenu(pTrayMenu_);
        }
        statusNotifierItem_->setIconByPixmap(icon);
#else // USE_KStatusNotifier
        if(!pTrayIcon_) {
            pTrayIcon_ = new QSystemTrayIcon(this);
            QString sToolTip = u"freeLib\n"_s;
            if(g::idCurrentLib != 0)
                sToolTip += g::libs[g::idCurrentLib].name;
            pTrayIcon_->setToolTip(sToolTip);

            pTrayIcon_->setContextMenu(pTrayMenu_);
            pTrayIcon_->show();

            connect(pTrayIcon_, &QSystemTrayIcon::activated, this, &MainWindow::handleTrayActivation);
        }
        pTrayIcon_->setIcon(icon);
#endif
    }
}

#ifdef USE_KStatusNotifier
void MainWindow::hideEvent(QHideEvent *ev)
{
    if(pHideAction_)
        pHideAction_->setVisible(false);
    if(pShowAction_)
        pShowAction_->setVisible(true);

    QMainWindow::hideEvent(ev);
}

void MainWindow::showEvent(QShowEvent *ev)
{
    if(pHideAction_)
        pHideAction_->setVisible(true);
    if(pShowAction_)
        pShowAction_->setVisible(false);

    QMainWindow::showEvent(ev);
}

void MainWindow::handleTrayActivation(bool active, const QPoint &pos)
{
    Q_UNUSED(pos);
    if (active) {
        if (isVisible())
            hide();
        else
            show();
    }
}
#else

void MainWindow::handleTrayActivation(QSystemTrayIcon::ActivationReason reason)
{
    if(reason != QSystemTrayIcon::Trigger && reason != QSystemTrayIcon::Unknown)
        return;
#ifdef Q_OS_WIN
    if(this->isVisible())
    {
        this->setWindowState(this->windowState()|Qt::WindowMinimized);
        if(options.nIconTray !=0 )
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
            if(g::options.nIconTray!=0)
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
            if(g::options.nIconTray != 0)
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

void MainWindow::MinimizeWindow()
{
    this->setWindowState(this->windowState() | Qt::WindowMinimized);
}

void MainWindow::hide()
{
    if(pHideAction_)
        pHideAction_->setVisible(false);
    if(pShowAction_)
        pShowAction_->setVisible(true);
    QMainWindow::hide();
}

void MainWindow::show()
{
    if(pHideAction_)
        pHideAction_->setVisible(true);
    if(pShowAction_)
        pShowAction_->setVisible(false);
    QMainWindow::show();
}

void MainWindow::changeEvent(QEvent *event)
{
    if(event->type() == QEvent::WindowStateChange)
    {
        if(isMinimized())
            hide();
    }
    QMainWindow::changeEvent(event);
}
#endif
