/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.9.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QDate>
#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDateEdit>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "qwebengineview.h"

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionAddLibrary;
    QAction *actionPreference;
    QAction *actionAbout;
    QAction *actionHelp;
    QAction *actionMove_to_author;
    QAction *actionExit;
    QAction *actionLibraries;
    QAction *actionMove_to_series;
    QAction *actionNew_labrary_wizard;
    QAction *actionCheck_uncheck;
    QAction *actionSwitch_to_convert_mode;
    QAction *actionSwitch_to_library_mode;
    QAction *actionMinimize_window;
    QWidget *centralWidget;
    QVBoxLayout *verticalLayout_12;
    QStackedWidget *stackedWidget;
    QWidget *pageLabrary;
    QVBoxLayout *verticalLayout_10;
    QSplitter *splitter_2;
    QWidget *verticalLayoutWidget;
    QVBoxLayout *verticalLayout_2;
    QFrame *frame;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QToolButton *btnAuthor;
    QToolButton *btnSeries;
    QToolButton *btnJanre;
    QToolButton *btnSearch;
    QSpacerItem *horizontalSpacer_2;
    QFrame *SearchFrame;
    QHBoxLayout *horizontalLayout_3;
    QLabel *label;
    QLineEdit *searchString;
    QStackedWidget *tabWidget;
    QWidget *pageAuthor;
    QVBoxLayout *verticalLayout;
    QListWidget *AuthorList;
    QWidget *pageSeria;
    QVBoxLayout *verticalLayout_6;
    QListWidget *SeriaList;
    QWidget *pageJanre;
    QVBoxLayout *verticalLayout_7;
    QTreeWidget *JanreList;
    QWidget *page;
    QVBoxLayout *verticalLayout_8;
    QFrame *frame_5;
    QVBoxLayout *verticalLayout_5;
    QLabel *label_2;
    QLineEdit *s_name;
    QLabel *label_3;
    QLineEdit *s_author;
    QLabel *label_4;
    QComboBox *s_janre;
    QLabel *label_5;
    QLineEdit *s_seria;
    QLabel *label_6;
    QHBoxLayout *horizontalLayout_6;
    QLabel *label_7;
    QDateEdit *date_from;
    QLabel *label_8;
    QDateEdit *date_to;
    QLabel *label_10;
    QComboBox *findLanguage;
    QLabel *label_13;
    QSpinBox *maxBooks;
    QHBoxLayout *horizontalLayout_7;
    QSpacerItem *horizontalSpacer_5;
    QPushButton *do_search;
    QLabel *label_11;
    QLabel *find_books;
    QSpacerItem *verticalSpacer;
    QWidget *verticalLayoutWidget_2;
    QVBoxLayout *verticalLayout_4;
    QFrame *frame_4;
    QHBoxLayout *horizontalLayout_2;
    QToolButton *btnExport;
    QToolButton *btnOpenBook;
    QToolButton *btnEdit;
    QToolButton *btnCheck;
    QToolButton *btnLibrary;
    QToolButton *btnOption;
    QSpacerItem *horizontalSpacer_8;
    QLabel *label_9;
    QComboBox *language;
    QSpacerItem *horizontalSpacer_9;
    QLabel *tag_label;
    QHBoxLayout *TagLayout;
    QComboBox *TagFilter;
    QSpacerItem *horizontalSpacer_3;
    QFrame *frame_3;
    QVBoxLayout *verticalLayout_9;
    QFrame *abc;
    QFrame *frame_2;
    QVBoxLayout *verticalLayout_16;
    QSplitter *splitter;
    QStackedWidget *stacked_books;
    QWidget *page_books;
    QVBoxLayout *verticalLayout_14;
    QTreeWidget *Books;
    QWidget *page_bookslist;
    QVBoxLayout *verticalLayout_15;
    QTreeWidget *BooksList;
    QFrame *frame_7;
    QVBoxLayout *verticalLayout_3;
    QWebEngineView *Review;
    QWidget *pageConvert;
    QVBoxLayout *verticalLayout_11;
    QFrame *frame_6;
    QHBoxLayout *horizontalLayout_4;
    QPushButton *btnPreference;
    QPushButton *btnSwitchToLib;
    QFrame *frame_drop;
    QVBoxLayout *verticalLayout_13;
    QFrame *drop_buttons;
    QLabel *label_drop;
    QMenuBar *menuBar;
    QMenu *menuHelp;
    QMenu *menu;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QStringLiteral("MainWindow"));
        MainWindow->resize(1297, 733);
        MainWindow->setMinimumSize(QSize(800, 400));
        MainWindow->setAcceptDrops(true);
        MainWindow->setWindowTitle(QStringLiteral("freeLib"));
        actionAddLibrary = new QAction(MainWindow);
        actionAddLibrary->setObjectName(QStringLiteral("actionAddLibrary"));
#ifndef QT_NO_SHORTCUT
        actionAddLibrary->setShortcut(QStringLiteral("Ctrl+L"));
#endif // QT_NO_SHORTCUT
        actionPreference = new QAction(MainWindow);
        actionPreference->setObjectName(QStringLiteral("actionPreference"));
        actionPreference->setMenuRole(QAction::PreferencesRole);
        actionAbout = new QAction(MainWindow);
        actionAbout->setObjectName(QStringLiteral("actionAbout"));
        actionAbout->setMenuRole(QAction::AboutRole);
        actionHelp = new QAction(MainWindow);
        actionHelp->setObjectName(QStringLiteral("actionHelp"));
#ifndef QT_NO_SHORTCUT
        actionHelp->setShortcut(QStringLiteral("F1"));
#endif // QT_NO_SHORTCUT
        actionMove_to_author = new QAction(MainWindow);
        actionMove_to_author->setObjectName(QStringLiteral("actionMove_to_author"));
        actionMove_to_author->setEnabled(false);
        actionExit = new QAction(MainWindow);
        actionExit->setObjectName(QStringLiteral("actionExit"));
        actionExit->setMenuRole(QAction::QuitRole);
        actionLibraries = new QAction(MainWindow);
        actionLibraries->setObjectName(QStringLiteral("actionLibraries"));
        actionMove_to_series = new QAction(MainWindow);
        actionMove_to_series->setObjectName(QStringLiteral("actionMove_to_series"));
        actionMove_to_series->setEnabled(false);
        actionNew_labrary_wizard = new QAction(MainWindow);
        actionNew_labrary_wizard->setObjectName(QStringLiteral("actionNew_labrary_wizard"));
        actionCheck_uncheck = new QAction(MainWindow);
        actionCheck_uncheck->setObjectName(QStringLiteral("actionCheck_uncheck"));
#ifndef QT_NO_SHORTCUT
        actionCheck_uncheck->setShortcut(QStringLiteral("Ctrl+A"));
#endif // QT_NO_SHORTCUT
        actionSwitch_to_convert_mode = new QAction(MainWindow);
        actionSwitch_to_convert_mode->setObjectName(QStringLiteral("actionSwitch_to_convert_mode"));
        actionSwitch_to_library_mode = new QAction(MainWindow);
        actionSwitch_to_library_mode->setObjectName(QStringLiteral("actionSwitch_to_library_mode"));
        actionMinimize_window = new QAction(MainWindow);
        actionMinimize_window->setObjectName(QStringLiteral("actionMinimize_window"));
#ifndef QT_NO_SHORTCUT
        actionMinimize_window->setShortcut(QStringLiteral("Ctrl+W"));
#endif // QT_NO_SHORTCUT
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        verticalLayout_12 = new QVBoxLayout(centralWidget);
        verticalLayout_12->setSpacing(6);
        verticalLayout_12->setContentsMargins(11, 11, 11, 11);
        verticalLayout_12->setObjectName(QStringLiteral("verticalLayout_12"));
        stackedWidget = new QStackedWidget(centralWidget);
        stackedWidget->setObjectName(QStringLiteral("stackedWidget"));
        pageLabrary = new QWidget();
        pageLabrary->setObjectName(QStringLiteral("pageLabrary"));
        verticalLayout_10 = new QVBoxLayout(pageLabrary);
        verticalLayout_10->setSpacing(6);
        verticalLayout_10->setContentsMargins(11, 11, 11, 11);
        verticalLayout_10->setObjectName(QStringLiteral("verticalLayout_10"));
        verticalLayout_10->setContentsMargins(0, 0, 0, 0);
        splitter_2 = new QSplitter(pageLabrary);
        splitter_2->setObjectName(QStringLiteral("splitter_2"));
        splitter_2->setOrientation(Qt::Horizontal);
        verticalLayoutWidget = new QWidget(splitter_2);
        verticalLayoutWidget->setObjectName(QStringLiteral("verticalLayoutWidget"));
        verticalLayout_2 = new QVBoxLayout(verticalLayoutWidget);
        verticalLayout_2->setSpacing(6);
        verticalLayout_2->setContentsMargins(11, 11, 11, 11);
        verticalLayout_2->setObjectName(QStringLiteral("verticalLayout_2"));
        verticalLayout_2->setContentsMargins(0, 2, 0, 0);
        frame = new QFrame(verticalLayoutWidget);
        frame->setObjectName(QStringLiteral("frame"));
        frame->setMinimumSize(QSize(0, 30));
        frame->setFrameShape(QFrame::NoFrame);
        frame->setFrameShadow(QFrame::Raised);
        horizontalLayout = new QHBoxLayout(frame);
        horizontalLayout->setSpacing(3);
        horizontalLayout->setContentsMargins(11, 11, 11, 11);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        btnAuthor = new QToolButton(frame);
        btnAuthor->setObjectName(QStringLiteral("btnAuthor"));
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(btnAuthor->sizePolicy().hasHeightForWidth());
        btnAuthor->setSizePolicy(sizePolicy);
        btnAuthor->setMinimumSize(QSize(70, 26));
        btnAuthor->setMaximumSize(QSize(70, 26));
        btnAuthor->setCheckable(true);
        btnAuthor->setChecked(true);
        btnAuthor->setAutoExclusive(true);
        btnAuthor->setAutoRaise(false);

        horizontalLayout->addWidget(btnAuthor);

        btnSeries = new QToolButton(frame);
        btnSeries->setObjectName(QStringLiteral("btnSeries"));
        btnSeries->setMinimumSize(QSize(70, 26));
        btnSeries->setMaximumSize(QSize(70, 26));
        btnSeries->setCheckable(true);
        btnSeries->setAutoExclusive(true);

        horizontalLayout->addWidget(btnSeries);

        btnJanre = new QToolButton(frame);
        btnJanre->setObjectName(QStringLiteral("btnJanre"));
        btnJanre->setMinimumSize(QSize(70, 26));
        btnJanre->setMaximumSize(QSize(70, 26));
        btnJanre->setCheckable(true);
        btnJanre->setAutoExclusive(true);

        horizontalLayout->addWidget(btnJanre);

        btnSearch = new QToolButton(frame);
        btnSearch->setObjectName(QStringLiteral("btnSearch"));
        btnSearch->setMinimumSize(QSize(70, 26));
        btnSearch->setMaximumSize(QSize(16777215, 26));
        btnSearch->setCheckable(true);
        btnSearch->setAutoExclusive(true);

        horizontalLayout->addWidget(btnSearch);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_2);


        verticalLayout_2->addWidget(frame);

        SearchFrame = new QFrame(verticalLayoutWidget);
        SearchFrame->setObjectName(QStringLiteral("SearchFrame"));
        SearchFrame->setFrameShape(QFrame::NoFrame);
        SearchFrame->setFrameShadow(QFrame::Raised);
        horizontalLayout_3 = new QHBoxLayout(SearchFrame);
        horizontalLayout_3->setSpacing(6);
        horizontalLayout_3->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_3->setObjectName(QStringLiteral("horizontalLayout_3"));
        horizontalLayout_3->setContentsMargins(0, 0, 0, 0);
        label = new QLabel(SearchFrame);
        label->setObjectName(QStringLiteral("label"));
        QFont font;
        font.setBold(true);
        font.setWeight(75);
        label->setFont(font);

        horizontalLayout_3->addWidget(label);

        searchString = new QLineEdit(SearchFrame);
        searchString->setObjectName(QStringLiteral("searchString"));
        QSizePolicy sizePolicy1(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(searchString->sizePolicy().hasHeightForWidth());
        searchString->setSizePolicy(sizePolicy1);
        searchString->setMinimumSize(QSize(0, 22));
        searchString->setFocusPolicy(Qt::WheelFocus);

        horizontalLayout_3->addWidget(searchString);


        verticalLayout_2->addWidget(SearchFrame);

        tabWidget = new QStackedWidget(verticalLayoutWidget);
        tabWidget->setObjectName(QStringLiteral("tabWidget"));
        tabWidget->setMinimumSize(QSize(350, 0));
        tabWidget->setMaximumSize(QSize(16777215, 16777215));
        pageAuthor = new QWidget();
        pageAuthor->setObjectName(QStringLiteral("pageAuthor"));
        verticalLayout = new QVBoxLayout(pageAuthor);
        verticalLayout->setSpacing(4);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        AuthorList = new QListWidget(pageAuthor);
        AuthorList->setObjectName(QStringLiteral("AuthorList"));
        AuthorList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        AuthorList->setEditTriggers(QAbstractItemView::NoEditTriggers);

        verticalLayout->addWidget(AuthorList);

        tabWidget->addWidget(pageAuthor);
        pageSeria = new QWidget();
        pageSeria->setObjectName(QStringLiteral("pageSeria"));
        verticalLayout_6 = new QVBoxLayout(pageSeria);
        verticalLayout_6->setSpacing(6);
        verticalLayout_6->setContentsMargins(11, 11, 11, 11);
        verticalLayout_6->setObjectName(QStringLiteral("verticalLayout_6"));
        verticalLayout_6->setContentsMargins(0, 0, 0, 0);
        SeriaList = new QListWidget(pageSeria);
        SeriaList->setObjectName(QStringLiteral("SeriaList"));
        SeriaList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        SeriaList->setEditTriggers(QAbstractItemView::NoEditTriggers);

        verticalLayout_6->addWidget(SeriaList);

        tabWidget->addWidget(pageSeria);
        pageJanre = new QWidget();
        pageJanre->setObjectName(QStringLiteral("pageJanre"));
        verticalLayout_7 = new QVBoxLayout(pageJanre);
        verticalLayout_7->setSpacing(6);
        verticalLayout_7->setContentsMargins(11, 11, 11, 11);
        verticalLayout_7->setObjectName(QStringLiteral("verticalLayout_7"));
        verticalLayout_7->setContentsMargins(0, 0, 0, 0);
        JanreList = new QTreeWidget(pageJanre);
        QTreeWidgetItem *__qtreewidgetitem = new QTreeWidgetItem();
        __qtreewidgetitem->setText(0, QStringLiteral("1"));
        JanreList->setHeaderItem(__qtreewidgetitem);
        JanreList->setObjectName(QStringLiteral("JanreList"));
        JanreList->setFocusPolicy(Qt::WheelFocus);
        JanreList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        JanreList->setEditTriggers(QAbstractItemView::NoEditTriggers);
        JanreList->setHeaderHidden(true);
        JanreList->setColumnCount(1);

        verticalLayout_7->addWidget(JanreList);

        tabWidget->addWidget(pageJanre);
        page = new QWidget();
        page->setObjectName(QStringLiteral("page"));
        verticalLayout_8 = new QVBoxLayout(page);
        verticalLayout_8->setSpacing(0);
        verticalLayout_8->setContentsMargins(11, 11, 11, 11);
        verticalLayout_8->setObjectName(QStringLiteral("verticalLayout_8"));
        verticalLayout_8->setContentsMargins(0, 0, 0, 0);
        frame_5 = new QFrame(page);
        frame_5->setObjectName(QStringLiteral("frame_5"));
        frame_5->setFrameShape(QFrame::NoFrame);
        frame_5->setFrameShadow(QFrame::Raised);
        verticalLayout_5 = new QVBoxLayout(frame_5);
        verticalLayout_5->setSpacing(6);
        verticalLayout_5->setContentsMargins(11, 11, 11, 11);
        verticalLayout_5->setObjectName(QStringLiteral("verticalLayout_5"));
        label_2 = new QLabel(frame_5);
        label_2->setObjectName(QStringLiteral("label_2"));

        verticalLayout_5->addWidget(label_2);

        s_name = new QLineEdit(frame_5);
        s_name->setObjectName(QStringLiteral("s_name"));

        verticalLayout_5->addWidget(s_name);

        label_3 = new QLabel(frame_5);
        label_3->setObjectName(QStringLiteral("label_3"));

        verticalLayout_5->addWidget(label_3);

        s_author = new QLineEdit(frame_5);
        s_author->setObjectName(QStringLiteral("s_author"));

        verticalLayout_5->addWidget(s_author);

        label_4 = new QLabel(frame_5);
        label_4->setObjectName(QStringLiteral("label_4"));

        verticalLayout_5->addWidget(label_4);

        s_janre = new QComboBox(frame_5);
        s_janre->setObjectName(QStringLiteral("s_janre"));
        s_janre->setMaxVisibleItems(15);

        verticalLayout_5->addWidget(s_janre);

        label_5 = new QLabel(frame_5);
        label_5->setObjectName(QStringLiteral("label_5"));

        verticalLayout_5->addWidget(label_5);

        s_seria = new QLineEdit(frame_5);
        s_seria->setObjectName(QStringLiteral("s_seria"));

        verticalLayout_5->addWidget(s_seria);

        label_6 = new QLabel(frame_5);
        label_6->setObjectName(QStringLiteral("label_6"));

        verticalLayout_5->addWidget(label_6);

        horizontalLayout_6 = new QHBoxLayout();
        horizontalLayout_6->setSpacing(6);
        horizontalLayout_6->setObjectName(QStringLiteral("horizontalLayout_6"));
        horizontalLayout_6->setContentsMargins(-1, 0, 0, -1);
        label_7 = new QLabel(frame_5);
        label_7->setObjectName(QStringLiteral("label_7"));

        horizontalLayout_6->addWidget(label_7);

        date_from = new QDateEdit(frame_5);
        date_from->setObjectName(QStringLiteral("date_from"));
        date_from->setMaximumSize(QSize(16777215, 16777215));
        date_from->setDate(QDate(1900, 1, 1));

        horizontalLayout_6->addWidget(date_from);

        label_8 = new QLabel(frame_5);
        label_8->setObjectName(QStringLiteral("label_8"));

        horizontalLayout_6->addWidget(label_8);

        date_to = new QDateEdit(frame_5);
        date_to->setObjectName(QStringLiteral("date_to"));
        date_to->setMaximumSize(QSize(16777215, 16777215));
        date_to->setDate(QDate(1900, 1, 1));

        horizontalLayout_6->addWidget(date_to);

        horizontalLayout_6->setStretch(1, 1);
        horizontalLayout_6->setStretch(3, 1);

        verticalLayout_5->addLayout(horizontalLayout_6);

        label_10 = new QLabel(frame_5);
        label_10->setObjectName(QStringLiteral("label_10"));

        verticalLayout_5->addWidget(label_10);

        findLanguage = new QComboBox(frame_5);
        findLanguage->setObjectName(QStringLiteral("findLanguage"));

        verticalLayout_5->addWidget(findLanguage);

        label_13 = new QLabel(frame_5);
        label_13->setObjectName(QStringLiteral("label_13"));

        verticalLayout_5->addWidget(label_13);

        maxBooks = new QSpinBox(frame_5);
        maxBooks->setObjectName(QStringLiteral("maxBooks"));
        maxBooks->setMinimum(0);
        maxBooks->setMaximum(10000000);
        maxBooks->setSingleStep(1000);
        maxBooks->setValue(1000);

        verticalLayout_5->addWidget(maxBooks);

        horizontalLayout_7 = new QHBoxLayout();
        horizontalLayout_7->setSpacing(6);
        horizontalLayout_7->setObjectName(QStringLiteral("horizontalLayout_7"));
        horizontalSpacer_5 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_7->addItem(horizontalSpacer_5);

        do_search = new QPushButton(frame_5);
        do_search->setObjectName(QStringLiteral("do_search"));

        horizontalLayout_7->addWidget(do_search);


        verticalLayout_5->addLayout(horizontalLayout_7);

        label_11 = new QLabel(frame_5);
        label_11->setObjectName(QStringLiteral("label_11"));

        verticalLayout_5->addWidget(label_11);

        find_books = new QLabel(frame_5);
        find_books->setObjectName(QStringLiteral("find_books"));
        QFont font1;
        font1.setPointSize(26);
        font1.setBold(true);
        font1.setWeight(75);
        find_books->setFont(font1);
        find_books->setText(QStringLiteral("0"));
        find_books->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        verticalLayout_5->addWidget(find_books);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_5->addItem(verticalSpacer);


        verticalLayout_8->addWidget(frame_5);

        tabWidget->addWidget(page);

        verticalLayout_2->addWidget(tabWidget);

        splitter_2->addWidget(verticalLayoutWidget);
        verticalLayoutWidget_2 = new QWidget(splitter_2);
        verticalLayoutWidget_2->setObjectName(QStringLiteral("verticalLayoutWidget_2"));
        verticalLayout_4 = new QVBoxLayout(verticalLayoutWidget_2);
        verticalLayout_4->setSpacing(0);
        verticalLayout_4->setContentsMargins(11, 11, 11, 11);
        verticalLayout_4->setObjectName(QStringLiteral("verticalLayout_4"));
        verticalLayout_4->setContentsMargins(0, 0, 0, 0);
        frame_4 = new QFrame(verticalLayoutWidget_2);
        frame_4->setObjectName(QStringLiteral("frame_4"));
        frame_4->setMinimumSize(QSize(0, 0));
        frame_4->setFrameShape(QFrame::NoFrame);
        frame_4->setFrameShadow(QFrame::Raised);
        horizontalLayout_2 = new QHBoxLayout(frame_4);
        horizontalLayout_2->setSpacing(3);
        horizontalLayout_2->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        horizontalLayout_2->setContentsMargins(0, 4, 0, 8);
        btnExport = new QToolButton(frame_4);
        btnExport->setObjectName(QStringLiteral("btnExport"));
        btnExport->setText(QStringLiteral("..."));
        QIcon icon;
        icon.addFile(QStringLiteral(":/icons/img/icons/Streamline.png"), QSize(), QIcon::Normal, QIcon::Off);
        btnExport->setIcon(icon);
        btnExport->setPopupMode(QToolButton::MenuButtonPopup);
        btnExport->setToolButtonStyle(Qt::ToolButtonIconOnly);

        horizontalLayout_2->addWidget(btnExport);

        btnOpenBook = new QToolButton(frame_4);
        btnOpenBook->setObjectName(QStringLiteral("btnOpenBook"));
        btnOpenBook->setText(QStringLiteral("..."));
        QIcon icon1;
        icon1.addFile(QStringLiteral(":/icons/img/icons/book.png"), QSize(), QIcon::Normal, QIcon::Off);
        btnOpenBook->setIcon(icon1);

        horizontalLayout_2->addWidget(btnOpenBook);

        btnEdit = new QToolButton(frame_4);
        btnEdit->setObjectName(QStringLiteral("btnEdit"));
        QIcon icon2;
        icon2.addFile(QStringLiteral(":/icons/img/icons/pen_alt_fill@2x.png"), QSize(), QIcon::Normal, QIcon::Off);
        btnEdit->setIcon(icon2);

        horizontalLayout_2->addWidget(btnEdit);

        btnCheck = new QToolButton(frame_4);
        btnCheck->setObjectName(QStringLiteral("btnCheck"));
        btnCheck->setText(QStringLiteral("..."));
        QIcon icon3;
        icon3.addFile(QStringLiteral(":/icons/img/icons/checkbox.png"), QSize(), QIcon::Normal, QIcon::Off);
        btnCheck->setIcon(icon3);

        horizontalLayout_2->addWidget(btnCheck);

        btnLibrary = new QToolButton(frame_4);
        btnLibrary->setObjectName(QStringLiteral("btnLibrary"));
        btnLibrary->setText(QStringLiteral("..."));
        QIcon icon4;
        icon4.addFile(QStringLiteral(":/icons/img/icons/library.png"), QSize(), QIcon::Normal, QIcon::Off);
        btnLibrary->setIcon(icon4);

        horizontalLayout_2->addWidget(btnLibrary);

        btnOption = new QToolButton(frame_4);
        btnOption->setObjectName(QStringLiteral("btnOption"));
        btnOption->setText(QStringLiteral("..."));
        QIcon icon5;
        icon5.addFile(QStringLiteral(":/icons/img/icons/settings.png"), QSize(), QIcon::Normal, QIcon::Off);
        btnOption->setIcon(icon5);

        horizontalLayout_2->addWidget(btnOption);

        horizontalSpacer_8 = new QSpacerItem(20, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_8);

        label_9 = new QLabel(frame_4);
        label_9->setObjectName(QStringLiteral("label_9"));

        horizontalLayout_2->addWidget(label_9);

        language = new QComboBox(frame_4);
        language->setObjectName(QStringLiteral("language"));
        language->setMaxVisibleItems(20);
        language->setInsertPolicy(QComboBox::NoInsert);
        language->setSizeAdjustPolicy(QComboBox::AdjustToContents);

        horizontalLayout_2->addWidget(language);

        horizontalSpacer_9 = new QSpacerItem(20, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_9);

        tag_label = new QLabel(frame_4);
        tag_label->setObjectName(QStringLiteral("tag_label"));

        horizontalLayout_2->addWidget(tag_label);

        TagLayout = new QHBoxLayout();
        TagLayout->setSpacing(1);
        TagLayout->setObjectName(QStringLiteral("TagLayout"));

        horizontalLayout_2->addLayout(TagLayout);

        TagFilter = new QComboBox(frame_4);
        TagFilter->setObjectName(QStringLiteral("TagFilter"));
        TagFilter->setInsertPolicy(QComboBox::NoInsert);
        TagFilter->setSizeAdjustPolicy(QComboBox::AdjustToContents);

        horizontalLayout_2->addWidget(TagFilter);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_3);


        verticalLayout_4->addWidget(frame_4);

        frame_3 = new QFrame(verticalLayoutWidget_2);
        frame_3->setObjectName(QStringLiteral("frame_3"));
        frame_3->setEnabled(true);
        verticalLayout_9 = new QVBoxLayout(frame_3);
        verticalLayout_9->setSpacing(0);
        verticalLayout_9->setContentsMargins(11, 11, 11, 11);
        verticalLayout_9->setObjectName(QStringLiteral("verticalLayout_9"));
        verticalLayout_9->setContentsMargins(0, 0, 0, 1);
        abc = new QFrame(frame_3);
        abc->setObjectName(QStringLiteral("abc"));
        abc->setFrameShape(QFrame::NoFrame);
        abc->setFrameShadow(QFrame::Raised);

        verticalLayout_9->addWidget(abc);


        verticalLayout_4->addWidget(frame_3);

        frame_2 = new QFrame(verticalLayoutWidget_2);
        frame_2->setObjectName(QStringLiteral("frame_2"));
        frame_2->setFrameShape(QFrame::NoFrame);
        frame_2->setFrameShadow(QFrame::Sunken);
        verticalLayout_16 = new QVBoxLayout(frame_2);
        verticalLayout_16->setSpacing(6);
        verticalLayout_16->setContentsMargins(11, 11, 11, 11);
        verticalLayout_16->setObjectName(QStringLiteral("verticalLayout_16"));
        verticalLayout_16->setContentsMargins(0, 0, 0, 0);
        splitter = new QSplitter(frame_2);
        splitter->setObjectName(QStringLiteral("splitter"));
        splitter->setOrientation(Qt::Vertical);
        stacked_books = new QStackedWidget(splitter);
        stacked_books->setObjectName(QStringLiteral("stacked_books"));
        page_books = new QWidget();
        page_books->setObjectName(QStringLiteral("page_books"));
        verticalLayout_14 = new QVBoxLayout(page_books);
        verticalLayout_14->setSpacing(6);
        verticalLayout_14->setContentsMargins(11, 11, 11, 11);
        verticalLayout_14->setObjectName(QStringLiteral("verticalLayout_14"));
        verticalLayout_14->setContentsMargins(0, 0, 0, 0);
        Books = new QTreeWidget(page_books);
        QTreeWidgetItem *__qtreewidgetitem1 = new QTreeWidgetItem();
        __qtreewidgetitem1->setTextAlignment(4, Qt::AlignCenter);
        __qtreewidgetitem1->setTextAlignment(3, Qt::AlignCenter);
        __qtreewidgetitem1->setTextAlignment(2, Qt::AlignTrailing|Qt::AlignVCenter);
        __qtreewidgetitem1->setTextAlignment(1, Qt::AlignTrailing|Qt::AlignVCenter);
        Books->setHeaderItem(__qtreewidgetitem1);
        Books->setObjectName(QStringLiteral("Books"));
        Books->setFrameShape(QFrame::StyledPanel);
        Books->setEditTriggers(QAbstractItemView::NoEditTriggers);
        Books->setAlternatingRowColors(true);
        Books->setSelectionBehavior(QAbstractItemView::SelectRows);

        verticalLayout_14->addWidget(Books);

        stacked_books->addWidget(page_books);
        page_bookslist = new QWidget();
        page_bookslist->setObjectName(QStringLiteral("page_bookslist"));
        verticalLayout_15 = new QVBoxLayout(page_bookslist);
        verticalLayout_15->setSpacing(6);
        verticalLayout_15->setContentsMargins(11, 11, 11, 11);
        verticalLayout_15->setObjectName(QStringLiteral("verticalLayout_15"));
        verticalLayout_15->setContentsMargins(0, 0, 0, 0);
        BooksList = new QTreeWidget(page_bookslist);
        BooksList->setObjectName(QStringLiteral("BooksList"));
        BooksList->header()->setProperty("showSortIndicator", QVariant(true));

        verticalLayout_15->addWidget(BooksList);

        stacked_books->addWidget(page_bookslist);
        splitter->addWidget(stacked_books);
        frame_7 = new QFrame(splitter);
        frame_7->setObjectName(QStringLiteral("frame_7"));
        frame_7->setFrameShape(QFrame::Panel);
        frame_7->setFrameShadow(QFrame::Sunken);
        verticalLayout_3 = new QVBoxLayout(frame_7);
        verticalLayout_3->setSpacing(6);
        verticalLayout_3->setContentsMargins(11, 11, 11, 11);
        verticalLayout_3->setObjectName(QStringLiteral("verticalLayout_3"));
        verticalLayout_3->setContentsMargins(0, 0, 0, 0);
        Review = new QWebEngineView(frame_7);
        Review->setObjectName(QStringLiteral("Review"));
        Review->setMinimumSize(QSize(0, 20));
        Review->setFocusPolicy(Qt::NoFocus);
        Review->setAcceptDrops(false);
        Review->setProperty("url", QVariant(QUrl(QStringLiteral("about:blank"))));

        verticalLayout_3->addWidget(Review);

        splitter->addWidget(frame_7);

        verticalLayout_16->addWidget(splitter);


        verticalLayout_4->addWidget(frame_2);

        splitter_2->addWidget(verticalLayoutWidget_2);

        verticalLayout_10->addWidget(splitter_2);

        stackedWidget->addWidget(pageLabrary);
        pageConvert = new QWidget();
        pageConvert->setObjectName(QStringLiteral("pageConvert"));
        verticalLayout_11 = new QVBoxLayout(pageConvert);
        verticalLayout_11->setSpacing(6);
        verticalLayout_11->setContentsMargins(11, 11, 11, 11);
        verticalLayout_11->setObjectName(QStringLiteral("verticalLayout_11"));
        verticalLayout_11->setContentsMargins(0, 0, 0, 0);
        frame_6 = new QFrame(pageConvert);
        frame_6->setObjectName(QStringLiteral("frame_6"));
        frame_6->setFrameShape(QFrame::NoFrame);
        frame_6->setFrameShadow(QFrame::Raised);
        horizontalLayout_4 = new QHBoxLayout(frame_6);
        horizontalLayout_4->setSpacing(6);
        horizontalLayout_4->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_4->setObjectName(QStringLiteral("horizontalLayout_4"));
        horizontalLayout_4->setContentsMargins(0, 0, 0, 0);
        btnPreference = new QPushButton(frame_6);
        btnPreference->setObjectName(QStringLiteral("btnPreference"));
        btnPreference->setFocusPolicy(Qt::NoFocus);

        horizontalLayout_4->addWidget(btnPreference);

        btnSwitchToLib = new QPushButton(frame_6);
        btnSwitchToLib->setObjectName(QStringLiteral("btnSwitchToLib"));
        btnSwitchToLib->setFocusPolicy(Qt::NoFocus);

        horizontalLayout_4->addWidget(btnSwitchToLib);


        verticalLayout_11->addWidget(frame_6);

        frame_drop = new QFrame(pageConvert);
        frame_drop->setObjectName(QStringLiteral("frame_drop"));
        frame_drop->setFrameShape(QFrame::NoFrame);
        frame_drop->setFrameShadow(QFrame::Raised);
        verticalLayout_13 = new QVBoxLayout(frame_drop);
        verticalLayout_13->setSpacing(6);
        verticalLayout_13->setContentsMargins(11, 11, 11, 11);
        verticalLayout_13->setObjectName(QStringLiteral("verticalLayout_13"));
        verticalLayout_13->setContentsMargins(0, 0, 0, 0);
        drop_buttons = new QFrame(frame_drop);
        drop_buttons->setObjectName(QStringLiteral("drop_buttons"));
        drop_buttons->setFrameShape(QFrame::NoFrame);
        drop_buttons->setFrameShadow(QFrame::Raised);

        verticalLayout_13->addWidget(drop_buttons);

        label_drop = new QLabel(frame_drop);
        label_drop->setObjectName(QStringLiteral("label_drop"));
        QFont font2;
        font2.setPointSize(14);
        font2.setBold(true);
        font2.setWeight(75);
        label_drop->setFont(font2);
        label_drop->setAlignment(Qt::AlignBottom|Qt::AlignHCenter);
        label_drop->setWordWrap(true);

        verticalLayout_13->addWidget(label_drop);

        verticalLayout_13->setStretch(0, 1);

        verticalLayout_11->addWidget(frame_drop);

        verticalLayout_11->setStretch(1, 1);
        stackedWidget->addWidget(pageConvert);

        verticalLayout_12->addWidget(stackedWidget);

        MainWindow->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(MainWindow);
        menuBar->setObjectName(QStringLiteral("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 1297, 23));
        menuHelp = new QMenu(menuBar);
        menuHelp->setObjectName(QStringLiteral("menuHelp"));
        menu = new QMenu(menuBar);
        menu->setObjectName(QStringLiteral("menu"));
        MainWindow->setMenuBar(menuBar);

        menuBar->addAction(menu->menuAction());
        menuBar->addAction(menuHelp->menuAction());
        menuHelp->addAction(actionAbout);
        menuHelp->addAction(actionHelp);
        menuHelp->addSeparator();
        menu->addAction(actionLibraries);
        menu->addAction(actionNew_labrary_wizard);
        menu->addAction(actionAddLibrary);
        menu->addAction(actionCheck_uncheck);
        menu->addSeparator();
        menu->addAction(actionSwitch_to_convert_mode);
        menu->addAction(actionSwitch_to_library_mode);
        menu->addSeparator();
        menu->addAction(actionPreference);
        menu->addSeparator();
        menu->addAction(actionMinimize_window);
        menu->addAction(actionExit);

        retranslateUi(MainWindow);

        stackedWidget->setCurrentIndex(0);
        tabWidget->setCurrentIndex(3);
        stacked_books->setCurrentIndex(1);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        actionAddLibrary->setText(QApplication::translate("MainWindow", "Add/Edit library", Q_NULLPTR));
        actionPreference->setText(QApplication::translate("MainWindow", "Preferences", Q_NULLPTR));
        actionAbout->setText(QApplication::translate("MainWindow", "About", Q_NULLPTR));
        actionHelp->setText(QApplication::translate("MainWindow", "Help", Q_NULLPTR));
        actionMove_to_author->setText(QApplication::translate("MainWindow", "Move to author", Q_NULLPTR));
        actionExit->setText(QApplication::translate("MainWindow", "Quit", Q_NULLPTR));
        actionLibraries->setText(QApplication::translate("MainWindow", "Libraries", Q_NULLPTR));
        actionMove_to_series->setText(QApplication::translate("MainWindow", "Move to series", Q_NULLPTR));
        actionNew_labrary_wizard->setText(QApplication::translate("MainWindow", "New library wizard", Q_NULLPTR));
        actionCheck_uncheck->setText(QApplication::translate("MainWindow", "Check/uncheck books", Q_NULLPTR));
        actionSwitch_to_convert_mode->setText(QApplication::translate("MainWindow", "Switch to convert mode", Q_NULLPTR));
        actionSwitch_to_library_mode->setText(QApplication::translate("MainWindow", "Switch to library mode", Q_NULLPTR));
        actionMinimize_window->setText(QApplication::translate("MainWindow", "Minimize window", Q_NULLPTR));
        btnAuthor->setText(QApplication::translate("MainWindow", "Authors", Q_NULLPTR));
        btnSeries->setText(QApplication::translate("MainWindow", "Series", Q_NULLPTR));
        btnJanre->setText(QApplication::translate("MainWindow", "Genres", Q_NULLPTR));
        btnSearch->setText(QApplication::translate("MainWindow", "Search", Q_NULLPTR));
        label->setText(QApplication::translate("MainWindow", "Search:", Q_NULLPTR));
        label_2->setText(QApplication::translate("MainWindow", "Title:", Q_NULLPTR));
        label_3->setText(QApplication::translate("MainWindow", "Author:", Q_NULLPTR));
        label_4->setText(QApplication::translate("MainWindow", "Genre:", Q_NULLPTR));
        label_5->setText(QApplication::translate("MainWindow", "Series:", Q_NULLPTR));
        label_6->setText(QApplication::translate("MainWindow", "Import date:", Q_NULLPTR));
        label_7->setText(QApplication::translate("MainWindow", "from:", Q_NULLPTR));
        date_from->setDisplayFormat(QApplication::translate("MainWindow", "dd.MM.yyyy", Q_NULLPTR));
        label_8->setText(QApplication::translate("MainWindow", "to:", Q_NULLPTR));
        date_to->setDisplayFormat(QApplication::translate("MainWindow", "dd.MM.yyyy", Q_NULLPTR));
        label_10->setText(QApplication::translate("MainWindow", "Language:", Q_NULLPTR));
        label_13->setText(QApplication::translate("MainWindow", "Maximum number of books:", Q_NULLPTR));
        do_search->setText(QApplication::translate("MainWindow", "Find", Q_NULLPTR));
        label_11->setText(QApplication::translate("MainWindow", "Books found", Q_NULLPTR));
#ifndef QT_NO_TOOLTIP
        btnExport->setToolTip(QApplication::translate("MainWindow", "Send to device", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        btnOpenBook->setToolTip(QApplication::translate("MainWindow", "Open book", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        btnEdit->setToolTip(QApplication::translate("MainWindow", "Edit matadata", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        btnCheck->setToolTip(QApplication::translate("MainWindow", "Select/unselect books", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        btnLibrary->setToolTip(QApplication::translate("MainWindow", "Libraries", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        btnOption->setToolTip(QApplication::translate("MainWindow", "Options", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
        label_9->setText(QApplication::translate("MainWindow", "Book language:", Q_NULLPTR));
        tag_label->setText(QApplication::translate("MainWindow", "Tag:", Q_NULLPTR));
        QTreeWidgetItem *___qtreewidgetitem = Books->headerItem();
        ___qtreewidgetitem->setText(5, QApplication::translate("MainWindow", "Genre", Q_NULLPTR));
        ___qtreewidgetitem->setText(4, QApplication::translate("MainWindow", "Import date", Q_NULLPTR));
        ___qtreewidgetitem->setText(3, QApplication::translate("MainWindow", "Mark", Q_NULLPTR));
        ___qtreewidgetitem->setText(2, QApplication::translate("MainWindow", "Size", Q_NULLPTR));
        ___qtreewidgetitem->setText(1, QApplication::translate("MainWindow", "No.", Q_NULLPTR));
        ___qtreewidgetitem->setText(0, QApplication::translate("MainWindow", "Name", Q_NULLPTR));
        QTreeWidgetItem *___qtreewidgetitem1 = BooksList->headerItem();
        ___qtreewidgetitem1->setText(5, QApplication::translate("MainWindow", "\320\235\320\276\320\262\321\213\320\271 \321\201\321\202\320\276\320\273\320\261\320\265\321\206", Q_NULLPTR));
        ___qtreewidgetitem1->setText(4, QApplication::translate("MainWindow", "\320\235\320\276\320\262\321\213\320\271 \321\201\321\202\320\276\320\273\320\261\320\265\321\206", Q_NULLPTR));
        ___qtreewidgetitem1->setText(3, QApplication::translate("MainWindow", "\320\235\320\276\320\262\321\213\320\271 \321\201\321\202\320\276\320\273\320\261\320\265\321\206", Q_NULLPTR));
        ___qtreewidgetitem1->setText(2, QApplication::translate("MainWindow", "\320\235\320\276\320\262\321\213\320\271 \321\201\321\202\320\276\320\273\320\261\320\265\321\206", Q_NULLPTR));
        ___qtreewidgetitem1->setText(1, QApplication::translate("MainWindow", "\320\235\320\276\320\262\321\213\320\271 \321\201\321\202\320\276\320\273\320\261\320\265\321\206", Q_NULLPTR));
        ___qtreewidgetitem1->setText(0, QApplication::translate("MainWindow", "\320\235\320\276\320\262\321\213\320\271 \321\201\321\202\320\276\320\273\320\261\320\265\321\206", Q_NULLPTR));
        btnPreference->setText(QApplication::translate("MainWindow", "Preference", Q_NULLPTR));
        btnSwitchToLib->setText(QApplication::translate("MainWindow", "Library", Q_NULLPTR));
        label_drop->setText(QApplication::translate("MainWindow", "Drop files for convert!", Q_NULLPTR));
        menuHelp->setTitle(QApplication::translate("MainWindow", "Help", Q_NULLPTR));
        menu->setTitle(QApplication::translate("MainWindow", "Library", Q_NULLPTR));
        Q_UNUSED(MainWindow);
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
