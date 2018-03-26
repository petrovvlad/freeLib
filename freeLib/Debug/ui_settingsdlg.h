/********************************************************************************
** Form generated from reading UI file 'settingsdlg.ui'
**
** Created by: Qt User Interface Compiler version 5.9.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SETTINGSDLG_H
#define UI_SETTINGSDLG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_SettingsDlg
{
public:
    QVBoxLayout *verticalLayout_3;
    QTabWidget *tabWidget;
    QWidget *tab_common;
    QVBoxLayout *verticalLayout_6;
    QCheckBox *ShowDeleted;
    QCheckBox *store_pos;
    QCheckBox *use_tag;
    QCheckBox *splash;
    QHBoxLayout *horizontalLayout_8;
    QLabel *label_4;
    QComboBox *trayIcon;
    QComboBox *tray_color;
    QSpacerItem *horizontalSpacer_3;
    QHBoxLayout *horizontalLayout_6;
    QLabel *label_22;
    QComboBox *Language;
    QLabel *label_24;
    QComboBox *ABC;
    QHBoxLayout *horizontalLayout;
    QLabel *label;
    QLineEdit *database_path;
    QCheckBox *settings_to_file;
    QSpacerItem *verticalSpacer;
    QWidget *tab_export;
    QVBoxLayout *verticalLayout_8;
    QFrame *frame;
    QHBoxLayout *horizontalLayout_10;
    QLabel *label_30;
    QComboBox *ExportName;
    QCheckBox *DefaultExport;
    QSpacerItem *horizontalSpacer_4;
    QToolButton *AddExport;
    QToolButton *DelExport;
    QToolButton *btnOpenExport;
    QToolButton *btnSaveExport;
    QVBoxLayout *verticalLayout;
    QCheckBox *CloseExpDlg;
    QCheckBox *uncheck_export;
    QCheckBox *extended_symbols;
    QStackedWidget *stackedWidget;
    QWidget *tab;
    QVBoxLayout *verticalLayout_2;
    QGroupBox *groupBox_4;
    QFormLayout *formLayout_4;
    QCheckBox *OPDS_enable;
    QLabel *label_20;
    QSpinBox *OPDS_port;
    QLabel *label_21;
    QLabel *OPDS;
    QLabel *label_23;
    QLabel *HTTP;
    QLabel *label_2;
    QComboBox *httpExport;
    QLabel *label_3;
    QHBoxLayout *horizontalLayout_7;
    QCheckBox *browseDir;
    QLineEdit *dirForBrowsing;
    QCheckBox *HTTP_need_pasword;
    QLabel *p_user;
    QLabel *p_password;
    QLineEdit *HTTP_user;
    QLineEdit *HTTP_password;
    QCheckBox *srv_annotation;
    QCheckBox *srv_covers;
    QLabel *label_5;
    QSpinBox *books_per_page;
    QGroupBox *groupBox;
    QFormLayout *formLayout_2;
    QLabel *pl_2;
    QComboBox *proxy_type;
    QLabel *pl_1;
    QLineEdit *proxy_host;
    QLabel *pl_4;
    QSpinBox *proxy_port;
    QLabel *pl_5;
    QLineEdit *proxy_user;
    QLabel *pl_3;
    QLineEdit *proxy_password;
    QWidget *tab_4;
    QVBoxLayout *verticalLayout_7;
    QTableWidget *ApplicationList;
    QHBoxLayout *horizontalLayout_5;
    QSpacerItem *horizontalSpacer_2;
    QPushButton *AddApp;
    QPushButton *DelApp;
    QWidget *tab_2;
    QVBoxLayout *verticalLayout_5;
    QTableWidget *ExportList;
    QHBoxLayout *horizontalLayout_4;
    QSpacerItem *horizontalSpacer;
    QPushButton *AddExp;
    QPushButton *DelExp;
    QHBoxLayout *horizontalLayout_2;
    QPushButton *btnDefaultSettings;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *SettingsDlg)
    {
        if (SettingsDlg->objectName().isEmpty())
            SettingsDlg->setObjectName(QStringLiteral("SettingsDlg"));
        SettingsDlg->setWindowModality(Qt::WindowModal);
        SettingsDlg->resize(721, 682);
        SettingsDlg->setMinimumSize(QSize(0, 0));
        SettingsDlg->setMaximumSize(QSize(6666666, 6666666));
        SettingsDlg->setModal(true);
        verticalLayout_3 = new QVBoxLayout(SettingsDlg);
        verticalLayout_3->setObjectName(QStringLiteral("verticalLayout_3"));
        verticalLayout_3->setContentsMargins(12, 12, 12, 21);
        tabWidget = new QTabWidget(SettingsDlg);
        tabWidget->setObjectName(QStringLiteral("tabWidget"));
        tab_common = new QWidget();
        tab_common->setObjectName(QStringLiteral("tab_common"));
        verticalLayout_6 = new QVBoxLayout(tab_common);
        verticalLayout_6->setObjectName(QStringLiteral("verticalLayout_6"));
        ShowDeleted = new QCheckBox(tab_common);
        ShowDeleted->setObjectName(QStringLiteral("ShowDeleted"));

        verticalLayout_6->addWidget(ShowDeleted);

        store_pos = new QCheckBox(tab_common);
        store_pos->setObjectName(QStringLiteral("store_pos"));

        verticalLayout_6->addWidget(store_pos);

        use_tag = new QCheckBox(tab_common);
        use_tag->setObjectName(QStringLiteral("use_tag"));

        verticalLayout_6->addWidget(use_tag);

        splash = new QCheckBox(tab_common);
        splash->setObjectName(QStringLiteral("splash"));

        verticalLayout_6->addWidget(splash);

        horizontalLayout_8 = new QHBoxLayout();
        horizontalLayout_8->setObjectName(QStringLiteral("horizontalLayout_8"));
        label_4 = new QLabel(tab_common);
        label_4->setObjectName(QStringLiteral("label_4"));

        horizontalLayout_8->addWidget(label_4);

        trayIcon = new QComboBox(tab_common);
        trayIcon->setObjectName(QStringLiteral("trayIcon"));

        horizontalLayout_8->addWidget(trayIcon);

        tray_color = new QComboBox(tab_common);
        tray_color->setObjectName(QStringLiteral("tray_color"));

        horizontalLayout_8->addWidget(tray_color);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_8->addItem(horizontalSpacer_3);

        horizontalLayout_8->setStretch(3, 1);

        verticalLayout_6->addLayout(horizontalLayout_8);

        horizontalLayout_6 = new QHBoxLayout();
        horizontalLayout_6->setObjectName(QStringLiteral("horizontalLayout_6"));
        label_22 = new QLabel(tab_common);
        label_22->setObjectName(QStringLiteral("label_22"));

        horizontalLayout_6->addWidget(label_22);

        Language = new QComboBox(tab_common);
        Language->setObjectName(QStringLiteral("Language"));

        horizontalLayout_6->addWidget(Language);

        label_24 = new QLabel(tab_common);
        label_24->setObjectName(QStringLiteral("label_24"));

        horizontalLayout_6->addWidget(label_24);

        ABC = new QComboBox(tab_common);
        ABC->setObjectName(QStringLiteral("ABC"));

        horizontalLayout_6->addWidget(ABC);

        horizontalLayout_6->setStretch(1, 1);
        horizontalLayout_6->setStretch(3, 1);

        verticalLayout_6->addLayout(horizontalLayout_6);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        label = new QLabel(tab_common);
        label->setObjectName(QStringLiteral("label"));

        horizontalLayout->addWidget(label);

        database_path = new QLineEdit(tab_common);
        database_path->setObjectName(QStringLiteral("database_path"));
        database_path->setReadOnly(true);

        horizontalLayout->addWidget(database_path);


        verticalLayout_6->addLayout(horizontalLayout);

        settings_to_file = new QCheckBox(tab_common);
        settings_to_file->setObjectName(QStringLiteral("settings_to_file"));

        verticalLayout_6->addWidget(settings_to_file);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_6->addItem(verticalSpacer);

        tabWidget->addTab(tab_common, QString());
        tab_export = new QWidget();
        tab_export->setObjectName(QStringLiteral("tab_export"));
        verticalLayout_8 = new QVBoxLayout(tab_export);
        verticalLayout_8->setObjectName(QStringLiteral("verticalLayout_8"));
        frame = new QFrame(tab_export);
        frame->setObjectName(QStringLiteral("frame"));
        frame->setFrameShape(QFrame::NoFrame);
        frame->setFrameShadow(QFrame::Raised);
        horizontalLayout_10 = new QHBoxLayout(frame);
        horizontalLayout_10->setObjectName(QStringLiteral("horizontalLayout_10"));
        horizontalLayout_10->setContentsMargins(0, 0, 0, 0);
        label_30 = new QLabel(frame);
        label_30->setObjectName(QStringLiteral("label_30"));

        horizontalLayout_10->addWidget(label_30);

        ExportName = new QComboBox(frame);
        ExportName->setObjectName(QStringLiteral("ExportName"));
        ExportName->setEditable(true);

        horizontalLayout_10->addWidget(ExportName);

        DefaultExport = new QCheckBox(frame);
        DefaultExport->setObjectName(QStringLiteral("DefaultExport"));

        horizontalLayout_10->addWidget(DefaultExport);

        horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_10->addItem(horizontalSpacer_4);

        AddExport = new QToolButton(frame);
        AddExport->setObjectName(QStringLiteral("AddExport"));
        AddExport->setMaximumSize(QSize(22, 22));
        AddExport->setText(QStringLiteral("+"));
        QIcon icon;
        icon.addFile(QStringLiteral(":/icons/img/icons/plus.png"), QSize(), QIcon::Normal, QIcon::Off);
        AddExport->setIcon(icon);

        horizontalLayout_10->addWidget(AddExport);

        DelExport = new QToolButton(frame);
        DelExport->setObjectName(QStringLiteral("DelExport"));
        DelExport->setMaximumSize(QSize(22, 22));
        DelExport->setText(QStringLiteral("-"));
        QIcon icon1;
        icon1.addFile(QStringLiteral(":/icons/img/icons/minus.png"), QSize(), QIcon::Normal, QIcon::Off);
        DelExport->setIcon(icon1);

        horizontalLayout_10->addWidget(DelExport);

        btnOpenExport = new QToolButton(frame);
        btnOpenExport->setObjectName(QStringLiteral("btnOpenExport"));
        btnOpenExport->setMaximumSize(QSize(22, 22));
        QIcon icon2;
        icon2.addFile(QStringLiteral(":/icons/img/icons/open.png"), QSize(), QIcon::Normal, QIcon::Off);
        btnOpenExport->setIcon(icon2);

        horizontalLayout_10->addWidget(btnOpenExport);

        btnSaveExport = new QToolButton(frame);
        btnSaveExport->setObjectName(QStringLiteral("btnSaveExport"));
        btnSaveExport->setMaximumSize(QSize(22, 22));
        QIcon icon3;
        icon3.addFile(QStringLiteral(":/icons/img/icons/save.png"), QSize(), QIcon::Normal, QIcon::Off);
        btnSaveExport->setIcon(icon3);

        horizontalLayout_10->addWidget(btnSaveExport);

        horizontalLayout_10->setStretch(1, 1);

        verticalLayout_8->addWidget(frame);

        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        CloseExpDlg = new QCheckBox(tab_export);
        CloseExpDlg->setObjectName(QStringLiteral("CloseExpDlg"));

        verticalLayout->addWidget(CloseExpDlg);

        uncheck_export = new QCheckBox(tab_export);
        uncheck_export->setObjectName(QStringLiteral("uncheck_export"));

        verticalLayout->addWidget(uncheck_export);

        extended_symbols = new QCheckBox(tab_export);
        extended_symbols->setObjectName(QStringLiteral("extended_symbols"));

        verticalLayout->addWidget(extended_symbols);


        verticalLayout_8->addLayout(verticalLayout);

        stackedWidget = new QStackedWidget(tab_export);
        stackedWidget->setObjectName(QStringLiteral("stackedWidget"));

        verticalLayout_8->addWidget(stackedWidget);

        tabWidget->addTab(tab_export, QString());
        tab = new QWidget();
        tab->setObjectName(QStringLiteral("tab"));
        verticalLayout_2 = new QVBoxLayout(tab);
        verticalLayout_2->setObjectName(QStringLiteral("verticalLayout_2"));
        verticalLayout_2->setContentsMargins(4, 4, 4, 4);
        groupBox_4 = new QGroupBox(tab);
        groupBox_4->setObjectName(QStringLiteral("groupBox_4"));
        formLayout_4 = new QFormLayout(groupBox_4);
        formLayout_4->setObjectName(QStringLiteral("formLayout_4"));
        formLayout_4->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
        formLayout_4->setFormAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);
        OPDS_enable = new QCheckBox(groupBox_4);
        OPDS_enable->setObjectName(QStringLiteral("OPDS_enable"));

        formLayout_4->setWidget(0, QFormLayout::FieldRole, OPDS_enable);

        label_20 = new QLabel(groupBox_4);
        label_20->setObjectName(QStringLiteral("label_20"));

        formLayout_4->setWidget(1, QFormLayout::LabelRole, label_20);

        OPDS_port = new QSpinBox(groupBox_4);
        OPDS_port->setObjectName(QStringLiteral("OPDS_port"));
        OPDS_port->setMinimum(1);
        OPDS_port->setMaximum(99999);
        OPDS_port->setValue(3000);

        formLayout_4->setWidget(1, QFormLayout::FieldRole, OPDS_port);

        label_21 = new QLabel(groupBox_4);
        label_21->setObjectName(QStringLiteral("label_21"));

        formLayout_4->setWidget(2, QFormLayout::LabelRole, label_21);

        OPDS = new QLabel(groupBox_4);
        OPDS->setObjectName(QStringLiteral("OPDS"));
        OPDS->setText(QStringLiteral("<a href=\"localhost\">localhost</a>"));
        OPDS->setOpenExternalLinks(true);

        formLayout_4->setWidget(2, QFormLayout::FieldRole, OPDS);

        label_23 = new QLabel(groupBox_4);
        label_23->setObjectName(QStringLiteral("label_23"));

        formLayout_4->setWidget(3, QFormLayout::LabelRole, label_23);

        HTTP = new QLabel(groupBox_4);
        HTTP->setObjectName(QStringLiteral("HTTP"));
        HTTP->setText(QStringLiteral("<a href=\"localhost\">localhost</a>"));
        HTTP->setOpenExternalLinks(true);

        formLayout_4->setWidget(3, QFormLayout::FieldRole, HTTP);

        label_2 = new QLabel(groupBox_4);
        label_2->setObjectName(QStringLiteral("label_2"));

        formLayout_4->setWidget(4, QFormLayout::LabelRole, label_2);

        httpExport = new QComboBox(groupBox_4);
        httpExport->setObjectName(QStringLiteral("httpExport"));

        formLayout_4->setWidget(4, QFormLayout::FieldRole, httpExport);

        label_3 = new QLabel(groupBox_4);
        label_3->setObjectName(QStringLiteral("label_3"));

        formLayout_4->setWidget(5, QFormLayout::LabelRole, label_3);

        horizontalLayout_7 = new QHBoxLayout();
        horizontalLayout_7->setObjectName(QStringLiteral("horizontalLayout_7"));
        browseDir = new QCheckBox(groupBox_4);
        browseDir->setObjectName(QStringLiteral("browseDir"));

        horizontalLayout_7->addWidget(browseDir);

        dirForBrowsing = new QLineEdit(groupBox_4);
        dirForBrowsing->setObjectName(QStringLiteral("dirForBrowsing"));

        horizontalLayout_7->addWidget(dirForBrowsing);

        horizontalLayout_7->setStretch(1, 1);

        formLayout_4->setLayout(5, QFormLayout::FieldRole, horizontalLayout_7);

        HTTP_need_pasword = new QCheckBox(groupBox_4);
        HTTP_need_pasword->setObjectName(QStringLiteral("HTTP_need_pasword"));

        formLayout_4->setWidget(6, QFormLayout::FieldRole, HTTP_need_pasword);

        p_user = new QLabel(groupBox_4);
        p_user->setObjectName(QStringLiteral("p_user"));

        formLayout_4->setWidget(7, QFormLayout::LabelRole, p_user);

        p_password = new QLabel(groupBox_4);
        p_password->setObjectName(QStringLiteral("p_password"));

        formLayout_4->setWidget(8, QFormLayout::LabelRole, p_password);

        HTTP_user = new QLineEdit(groupBox_4);
        HTTP_user->setObjectName(QStringLiteral("HTTP_user"));

        formLayout_4->setWidget(7, QFormLayout::FieldRole, HTTP_user);

        HTTP_password = new QLineEdit(groupBox_4);
        HTTP_password->setObjectName(QStringLiteral("HTTP_password"));
        HTTP_password->setEchoMode(QLineEdit::Password);

        formLayout_4->setWidget(8, QFormLayout::FieldRole, HTTP_password);

        srv_annotation = new QCheckBox(groupBox_4);
        srv_annotation->setObjectName(QStringLiteral("srv_annotation"));

        formLayout_4->setWidget(10, QFormLayout::FieldRole, srv_annotation);

        srv_covers = new QCheckBox(groupBox_4);
        srv_covers->setObjectName(QStringLiteral("srv_covers"));

        formLayout_4->setWidget(9, QFormLayout::FieldRole, srv_covers);

        label_5 = new QLabel(groupBox_4);
        label_5->setObjectName(QStringLiteral("label_5"));

        formLayout_4->setWidget(11, QFormLayout::LabelRole, label_5);

        books_per_page = new QSpinBox(groupBox_4);
        books_per_page->setObjectName(QStringLiteral("books_per_page"));

        formLayout_4->setWidget(11, QFormLayout::FieldRole, books_per_page);


        verticalLayout_2->addWidget(groupBox_4);

        groupBox = new QGroupBox(tab);
        groupBox->setObjectName(QStringLiteral("groupBox"));
        formLayout_2 = new QFormLayout(groupBox);
        formLayout_2->setObjectName(QStringLiteral("formLayout_2"));
        formLayout_2->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
        formLayout_2->setFormAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);
        pl_2 = new QLabel(groupBox);
        pl_2->setObjectName(QStringLiteral("pl_2"));

        formLayout_2->setWidget(0, QFormLayout::LabelRole, pl_2);

        proxy_type = new QComboBox(groupBox);
        proxy_type->addItem(QString());
        proxy_type->addItem(QStringLiteral("SOCKS 5"));
        proxy_type->addItem(QStringLiteral("HTTP"));
        proxy_type->setObjectName(QStringLiteral("proxy_type"));

        formLayout_2->setWidget(0, QFormLayout::FieldRole, proxy_type);

        pl_1 = new QLabel(groupBox);
        pl_1->setObjectName(QStringLiteral("pl_1"));

        formLayout_2->setWidget(1, QFormLayout::LabelRole, pl_1);

        proxy_host = new QLineEdit(groupBox);
        proxy_host->setObjectName(QStringLiteral("proxy_host"));

        formLayout_2->setWidget(1, QFormLayout::FieldRole, proxy_host);

        pl_4 = new QLabel(groupBox);
        pl_4->setObjectName(QStringLiteral("pl_4"));

        formLayout_2->setWidget(2, QFormLayout::LabelRole, pl_4);

        proxy_port = new QSpinBox(groupBox);
        proxy_port->setObjectName(QStringLiteral("proxy_port"));
        proxy_port->setMinimum(1);
        proxy_port->setMaximum(99999);
        proxy_port->setValue(8080);

        formLayout_2->setWidget(2, QFormLayout::FieldRole, proxy_port);

        pl_5 = new QLabel(groupBox);
        pl_5->setObjectName(QStringLiteral("pl_5"));

        formLayout_2->setWidget(3, QFormLayout::LabelRole, pl_5);

        proxy_user = new QLineEdit(groupBox);
        proxy_user->setObjectName(QStringLiteral("proxy_user"));

        formLayout_2->setWidget(3, QFormLayout::FieldRole, proxy_user);

        pl_3 = new QLabel(groupBox);
        pl_3->setObjectName(QStringLiteral("pl_3"));

        formLayout_2->setWidget(4, QFormLayout::LabelRole, pl_3);

        proxy_password = new QLineEdit(groupBox);
        proxy_password->setObjectName(QStringLiteral("proxy_password"));
        proxy_password->setEchoMode(QLineEdit::Password);

        formLayout_2->setWidget(4, QFormLayout::FieldRole, proxy_password);


        verticalLayout_2->addWidget(groupBox);

        tabWidget->addTab(tab, QString());
        tab_4 = new QWidget();
        tab_4->setObjectName(QStringLiteral("tab_4"));
        verticalLayout_7 = new QVBoxLayout(tab_4);
        verticalLayout_7->setSpacing(5);
        verticalLayout_7->setObjectName(QStringLiteral("verticalLayout_7"));
        verticalLayout_7->setContentsMargins(5, 5, 5, 5);
        ApplicationList = new QTableWidget(tab_4);
        if (ApplicationList->columnCount() < 2)
            ApplicationList->setColumnCount(2);
        QTableWidgetItem *__qtablewidgetitem = new QTableWidgetItem();
        ApplicationList->setHorizontalHeaderItem(0, __qtablewidgetitem);
        QTableWidgetItem *__qtablewidgetitem1 = new QTableWidgetItem();
        ApplicationList->setHorizontalHeaderItem(1, __qtablewidgetitem1);
        ApplicationList->setObjectName(QStringLiteral("ApplicationList"));
        QSizePolicy sizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(ApplicationList->sizePolicy().hasHeightForWidth());
        ApplicationList->setSizePolicy(sizePolicy);
        ApplicationList->setProperty("showDropIndicator", QVariant(false));
        ApplicationList->setDragDropOverwriteMode(false);
        ApplicationList->setAlternatingRowColors(true);
        ApplicationList->setSelectionMode(QAbstractItemView::SingleSelection);
        ApplicationList->setSelectionBehavior(QAbstractItemView::SelectRows);
        ApplicationList->setWordWrap(false);
        ApplicationList->setRowCount(0);
        ApplicationList->setColumnCount(2);
        ApplicationList->horizontalHeader()->setHighlightSections(false);
        ApplicationList->verticalHeader()->setVisible(false);
        ApplicationList->verticalHeader()->setDefaultSectionSize(22);

        verticalLayout_7->addWidget(ApplicationList);

        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setSpacing(5);
        horizontalLayout_5->setObjectName(QStringLiteral("horizontalLayout_5"));
        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_5->addItem(horizontalSpacer_2);

        AddApp = new QPushButton(tab_4);
        AddApp->setObjectName(QStringLiteral("AddApp"));

        horizontalLayout_5->addWidget(AddApp);

        DelApp = new QPushButton(tab_4);
        DelApp->setObjectName(QStringLiteral("DelApp"));

        horizontalLayout_5->addWidget(DelApp);


        verticalLayout_7->addLayout(horizontalLayout_5);

        tabWidget->addTab(tab_4, QString());
        tab_2 = new QWidget();
        tab_2->setObjectName(QStringLiteral("tab_2"));
        verticalLayout_5 = new QVBoxLayout(tab_2);
        verticalLayout_5->setSpacing(5);
        verticalLayout_5->setObjectName(QStringLiteral("verticalLayout_5"));
        verticalLayout_5->setContentsMargins(5, 5, 5, 5);
        ExportList = new QTableWidget(tab_2);
        if (ExportList->columnCount() < 4)
            ExportList->setColumnCount(4);
        QTableWidgetItem *__qtablewidgetitem2 = new QTableWidgetItem();
        ExportList->setHorizontalHeaderItem(0, __qtablewidgetitem2);
        QTableWidgetItem *__qtablewidgetitem3 = new QTableWidgetItem();
        ExportList->setHorizontalHeaderItem(1, __qtablewidgetitem3);
        QTableWidgetItem *__qtablewidgetitem4 = new QTableWidgetItem();
        ExportList->setHorizontalHeaderItem(2, __qtablewidgetitem4);
        QTableWidgetItem *__qtablewidgetitem5 = new QTableWidgetItem();
        ExportList->setHorizontalHeaderItem(3, __qtablewidgetitem5);
        ExportList->setObjectName(QStringLiteral("ExportList"));
        ExportList->setProperty("showDropIndicator", QVariant(false));
        ExportList->setDragDropOverwriteMode(false);
        ExportList->setAlternatingRowColors(true);
        ExportList->setSelectionMode(QAbstractItemView::SingleSelection);
        ExportList->setSelectionBehavior(QAbstractItemView::SelectRows);
        ExportList->setWordWrap(false);
        ExportList->horizontalHeader()->setHighlightSections(false);
        ExportList->verticalHeader()->setVisible(false);
        ExportList->verticalHeader()->setDefaultSectionSize(22);

        verticalLayout_5->addWidget(ExportList);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setSpacing(5);
        horizontalLayout_4->setObjectName(QStringLiteral("horizontalLayout_4"));
        horizontalLayout_4->setContentsMargins(-1, -1, -1, 0);
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer);

        AddExp = new QPushButton(tab_2);
        AddExp->setObjectName(QStringLiteral("AddExp"));

        horizontalLayout_4->addWidget(AddExp);

        DelExp = new QPushButton(tab_2);
        DelExp->setObjectName(QStringLiteral("DelExp"));

        horizontalLayout_4->addWidget(DelExp);


        verticalLayout_5->addLayout(horizontalLayout_4);

        tabWidget->addTab(tab_2, QString());

        verticalLayout_3->addWidget(tabWidget);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        btnDefaultSettings = new QPushButton(SettingsDlg);
        btnDefaultSettings->setObjectName(QStringLiteral("btnDefaultSettings"));

        horizontalLayout_2->addWidget(btnDefaultSettings);

        buttonBox = new QDialogButtonBox(SettingsDlg);
        buttonBox->setObjectName(QStringLiteral("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        horizontalLayout_2->addWidget(buttonBox);


        verticalLayout_3->addLayout(horizontalLayout_2);


        retranslateUi(SettingsDlg);
        QObject::connect(buttonBox, SIGNAL(accepted()), SettingsDlg, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), SettingsDlg, SLOT(reject()));

        tabWidget->setCurrentIndex(1);


        QMetaObject::connectSlotsByName(SettingsDlg);
    } // setupUi

    void retranslateUi(QDialog *SettingsDlg)
    {
        SettingsDlg->setWindowTitle(QApplication::translate("SettingsDlg", "Settings dialog", Q_NULLPTR));
        ShowDeleted->setText(QApplication::translate("SettingsDlg", "Show deleted books", Q_NULLPTR));
        store_pos->setText(QApplication::translate("SettingsDlg", "Remember library position", Q_NULLPTR));
        use_tag->setText(QApplication::translate("SettingsDlg", "Use tag", Q_NULLPTR));
        splash->setText(QApplication::translate("SettingsDlg", "Don't show splash screen", Q_NULLPTR));
        label_4->setText(QApplication::translate("SettingsDlg", "Tray icon", Q_NULLPTR));
        trayIcon->clear();
        trayIcon->insertItems(0, QStringList()
         << QApplication::translate("SettingsDlg", "don't show", Q_NULLPTR)
         << QApplication::translate("SettingsDlg", "minimize to tray", Q_NULLPTR)
         << QApplication::translate("SettingsDlg", "minimize to tray on start", Q_NULLPTR)
        );
        tray_color->clear();
        tray_color->insertItems(0, QStringList()
         << QApplication::translate("SettingsDlg", "colored", Q_NULLPTR)
         << QApplication::translate("SettingsDlg", "dark", Q_NULLPTR)
         << QApplication::translate("SettingsDlg", "light", Q_NULLPTR)
        );
        label_22->setText(QApplication::translate("SettingsDlg", "Language UI:", Q_NULLPTR));
        label_24->setText(QApplication::translate("SettingsDlg", "ABC:", Q_NULLPTR));
        label->setText(QApplication::translate("SettingsDlg", "Database path:", Q_NULLPTR));
        settings_to_file->setText(QApplication::translate("SettingsDlg", "Save settings in application folder", Q_NULLPTR));
        tabWidget->setTabText(tabWidget->indexOf(tab_common), QApplication::translate("SettingsDlg", "Common", Q_NULLPTR));
        label_30->setText(QApplication::translate("SettingsDlg", "Profile:", Q_NULLPTR));
        DefaultExport->setText(QApplication::translate("SettingsDlg", "Default", Q_NULLPTR));
#ifndef QT_NO_TOOLTIP
        AddExport->setToolTip(QApplication::translate("SettingsDlg", "Add profile", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        DelExport->setToolTip(QApplication::translate("SettingsDlg", "Remove profile", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        btnOpenExport->setToolTip(QApplication::translate("SettingsDlg", "Open export settings", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
        btnOpenExport->setText(QApplication::translate("SettingsDlg", "Open", Q_NULLPTR));
#ifndef QT_NO_TOOLTIP
        btnSaveExport->setToolTip(QApplication::translate("SettingsDlg", "Save export settings", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
        btnSaveExport->setText(QApplication::translate("SettingsDlg", "Save", Q_NULLPTR));
        CloseExpDlg->setText(QApplication::translate("SettingsDlg", "Close dialog after export", Q_NULLPTR));
        uncheck_export->setText(QApplication::translate("SettingsDlg", "Uncheck successfully export", Q_NULLPTR));
        extended_symbols->setText(QApplication::translate("SettingsDlg", "Extended characters in the file name", Q_NULLPTR));
        tabWidget->setTabText(tabWidget->indexOf(tab_export), QApplication::translate("SettingsDlg", "Export", Q_NULLPTR));
        groupBox_4->setTitle(QApplication::translate("SettingsDlg", "HTTP/OPDS server", Q_NULLPTR));
        OPDS_enable->setText(QApplication::translate("SettingsDlg", "Enable", Q_NULLPTR));
        label_20->setText(QApplication::translate("SettingsDlg", "Port:", Q_NULLPTR));
        label_21->setText(QApplication::translate("SettingsDlg", "Common OPDS server", Q_NULLPTR));
        label_23->setText(QApplication::translate("SettingsDlg", "Common HTTP server", Q_NULLPTR));
        label_2->setText(QApplication::translate("SettingsDlg", "Export settings:", Q_NULLPTR));
        httpExport->clear();
        httpExport->insertItems(0, QStringList()
         << QApplication::translate("SettingsDlg", "default", Q_NULLPTR)
        );
        label_3->setText(QApplication::translate("SettingsDlg", "Browse dir:", Q_NULLPTR));
        browseDir->setText(QString());
        HTTP_need_pasword->setText(QApplication::translate("SettingsDlg", "Password protection", Q_NULLPTR));
        p_user->setText(QApplication::translate("SettingsDlg", "User:", Q_NULLPTR));
        p_password->setText(QApplication::translate("SettingsDlg", "Password:", Q_NULLPTR));
        srv_annotation->setText(QApplication::translate("SettingsDlg", "Show annotation", Q_NULLPTR));
        srv_covers->setText(QApplication::translate("SettingsDlg", "Show covers", Q_NULLPTR));
        label_5->setText(QApplication::translate("SettingsDlg", "Books per page:", Q_NULLPTR));
        groupBox->setTitle(QApplication::translate("SettingsDlg", "Proxy server", Q_NULLPTR));
        pl_2->setText(QApplication::translate("SettingsDlg", "Proxy type:", Q_NULLPTR));
        proxy_type->setItemText(0, QApplication::translate("SettingsDlg", "No proxy", Q_NULLPTR));

        pl_1->setText(QApplication::translate("SettingsDlg", "Host name:", Q_NULLPTR));
        pl_4->setText(QApplication::translate("SettingsDlg", "Port:", Q_NULLPTR));
        pl_5->setText(QApplication::translate("SettingsDlg", "User name:", Q_NULLPTR));
        proxy_user->setText(QString());
        pl_3->setText(QApplication::translate("SettingsDlg", "Password:", Q_NULLPTR));
        tabWidget->setTabText(tabWidget->indexOf(tab), QApplication::translate("SettingsDlg", "Network", Q_NULLPTR));
        QTableWidgetItem *___qtablewidgetitem = ApplicationList->horizontalHeaderItem(0);
        ___qtablewidgetitem->setText(QApplication::translate("SettingsDlg", "Ext", Q_NULLPTR));
        QTableWidgetItem *___qtablewidgetitem1 = ApplicationList->horizontalHeaderItem(1);
        ___qtablewidgetitem1->setText(QApplication::translate("SettingsDlg", "Application", Q_NULLPTR));
        AddApp->setText(QApplication::translate("SettingsDlg", "Add", Q_NULLPTR));
        DelApp->setText(QApplication::translate("SettingsDlg", "Del", Q_NULLPTR));
        tabWidget->setTabText(tabWidget->indexOf(tab_4), QApplication::translate("SettingsDlg", "Program", Q_NULLPTR));
        QTableWidgetItem *___qtablewidgetitem2 = ExportList->horizontalHeaderItem(0);
        ___qtablewidgetitem2->setText(QApplication::translate("SettingsDlg", "Name", Q_NULLPTR));
        QTableWidgetItem *___qtablewidgetitem3 = ExportList->horizontalHeaderItem(1);
        ___qtablewidgetitem3->setText(QApplication::translate("SettingsDlg", "Application", Q_NULLPTR));
        QTableWidgetItem *___qtablewidgetitem4 = ExportList->horizontalHeaderItem(2);
        ___qtablewidgetitem4->setText(QApplication::translate("SettingsDlg", "Params", Q_NULLPTR));
        QTableWidgetItem *___qtablewidgetitem5 = ExportList->horizontalHeaderItem(3);
        ___qtablewidgetitem5->setText(QApplication::translate("SettingsDlg", "Output file name", Q_NULLPTR));
#ifndef QT_NO_TOOLTIP
        ExportList->setToolTip(QApplication::translate("SettingsDlg", "%fn - file name without extension\n"
"%f - file name\n"
"%d - path to file", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
        AddExp->setText(QApplication::translate("SettingsDlg", "Add", Q_NULLPTR));
        DelExp->setText(QApplication::translate("SettingsDlg", "Del", Q_NULLPTR));
        tabWidget->setTabText(tabWidget->indexOf(tab_2), QApplication::translate("SettingsDlg", "Postprocessing tools", Q_NULLPTR));
        btnDefaultSettings->setText(QApplication::translate("SettingsDlg", "Load default", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class SettingsDlg: public Ui_SettingsDlg {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SETTINGSDLG_H
