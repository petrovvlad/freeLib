/********************************************************************************
** Form generated from reading UI file 'exportframe.ui'
**
** Created by: Qt User Interface Compiler version 5.9.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_EXPORTFRAME_H
#define UI_EXPORTFRAME_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QToolBox>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ExportFrame
{
public:
    QVBoxLayout *verticalLayout_8;
    QTabWidget *tabWidget;
    QWidget *tab;
    QVBoxLayout *verticalLayout_4;
    QGroupBox *groupBox_3;
    QFormLayout *formLayout_2;
    QLabel *label_12;
    QComboBox *OutputFormat;
    QLabel *label_10;
    QComboBox *CurrentTools;
    QHBoxLayout *horizontalLayout_5;
    QRadioButton *radioDevice;
    QRadioButton *radioEmail;
    QSpacerItem *horizontalSpacer;
    QStackedWidget *stackedWidget;
    QWidget *page_3;
    QVBoxLayout *verticalLayout_3;
    QGroupBox *groupBox_2;
    QVBoxLayout *verticalLayout_6;
    QCheckBox *PostprocessingCopy;
    QCheckBox *askPath;
    QLabel *label_6;
    QLineEdit *Path;
    QCheckBox *originalFileName;
    QLabel *label_exportname;
    QHBoxLayout *horizontalLayout_9;
    QLineEdit *ExportFileName;
    QCheckBox *transliteration;
    QSpacerItem *verticalSpacer;
    QWidget *page_4;
    QVBoxLayout *verticalLayout_2;
    QGroupBox *groupBox;
    QVBoxLayout *verticalLayout_5;
    QFormLayout *formLayout;
    QLabel *label_16;
    QLineEdit *from_email;
    QLabel *label;
    QLineEdit *Email;
    QLabel *label_20;
    QLineEdit *mail_subject;
    QLabel *label_2;
    QHBoxLayout *horizontalLayout;
    QLineEdit *Server;
    QHBoxLayout *horizontalLayout_3;
    QLabel *label_11;
    QComboBox *ConnectionType;
    QLabel *label_4;
    QLineEdit *Port;
    QLabel *label_3;
    QHBoxLayout *horizontalLayout_2;
    QLineEdit *User;
    QHBoxLayout *horizontalLayout_6;
    QLabel *label_5;
    QLineEdit *Password;
    QLabel *label_9;
    QSpinBox *PauseMail;
    QSpacerItem *verticalSpacer_4;
    QWidget *tabFormat;
    QVBoxLayout *verticalLayout;
    QToolBox *toolBox;
    QWidget *page_2;
    QVBoxLayout *verticalLayout_9;
    QFrame *GB2;
    QVBoxLayout *verticalLayout_13;
    QLabel *label_13;
    QHBoxLayout *horizontalLayout_10;
    QLineEdit *authorstring;
    QCheckBox *authorTranslit;
    QLabel *Label22;
    QHBoxLayout *horizontalLayout_11;
    QLineEdit *seriastring;
    QCheckBox *seriaTranslit;
    QCheckBox *repairCover;
    QCheckBox *removePersonal;
    QCheckBox *createCover;
    QCheckBox *createCaverAlways;
    QCheckBox *addCoverLabel;
    QLabel *label_tmplate;
    QLineEdit *coverLabel;
    QSpacerItem *verticalSpacer_6;
    QWidget *page_5;
    QVBoxLayout *verticalLayout_10;
    QFrame *GB5;
    QHBoxLayout *horizontalLayout_14;
    QVBoxLayout *verticalLayout_16;
    QLabel *label_27;
    QHBoxLayout *horizontalLayout_4;
    QComboBox *footnotes;
    QSpacerItem *horizontalSpacer_2;
    QLabel *label_15;
    QHBoxLayout *horizontalLayout_7;
    QComboBox *hyphenate;
    QSpacerItem *horizontalSpacer_4;
    QLabel *label_18;
    QHBoxLayout *horizontalLayout_8;
    QComboBox *Vignette;
    QSpacerItem *horizontalSpacer_5;
    QCheckBox *break_after_cupture;
    QCheckBox *dropcaps;
    QLabel *label_7;
    QHBoxLayout *horizontalLayout_13;
    QComboBox *content_placement;
    QSpacerItem *horizontalSpacer_7;
    QSpacerItem *verticalSpacer_5;
    QVBoxLayout *verticalLayout_14;
    QHBoxLayout *horizontalLayout_15;
    QCheckBox *userCSS;
    QSpacerItem *horizontalSpacer_8;
    QToolButton *btnDefaultCSS;
    QPlainTextEdit *UserCSStext;
    QSpacerItem *verticalSpacer_7;
    QWidget *page;
    QVBoxLayout *verticalLayout_12;
    QFrame *frame;
    QVBoxLayout *fontLayout;
    QHBoxLayout *Layout;
    QSpacerItem *horizontalSpacer_3;
    QPushButton *AddFont;
    QSpacerItem *verticalSpacer_2;
    QWidget *page_6;
    QVBoxLayout *verticalLayout_11;
    QFrame *GB6;
    QVBoxLayout *verticalLayout_7;
    QCheckBox *annotation;
    QCheckBox *split_file;
    QCheckBox *join_series;
    QCheckBox *ml_toc;
    QHBoxLayout *horizontalLayout_12;
    QLabel *label_lavel;
    QSpinBox *MAXcaptionLevel;
    QSpacerItem *horizontalSpacer_6;
    QSpacerItem *verticalSpacer_3;

    void setupUi(QFrame *ExportFrame)
    {
        if (ExportFrame->objectName().isEmpty())
            ExportFrame->setObjectName(QStringLiteral("ExportFrame"));
        ExportFrame->resize(693, 425);
        ExportFrame->setWindowTitle(QStringLiteral("Frame"));
        ExportFrame->setFrameShape(QFrame::NoFrame);
        ExportFrame->setFrameShadow(QFrame::Raised);
        verticalLayout_8 = new QVBoxLayout(ExportFrame);
        verticalLayout_8->setSpacing(8);
        verticalLayout_8->setObjectName(QStringLiteral("verticalLayout_8"));
        verticalLayout_8->setContentsMargins(0, 0, 0, 0);
        tabWidget = new QTabWidget(ExportFrame);
        tabWidget->setObjectName(QStringLiteral("tabWidget"));
        tab = new QWidget();
        tab->setObjectName(QStringLiteral("tab"));
        verticalLayout_4 = new QVBoxLayout(tab);
        verticalLayout_4->setObjectName(QStringLiteral("verticalLayout_4"));
        verticalLayout_4->setContentsMargins(5, 5, 5, 5);
        groupBox_3 = new QGroupBox(tab);
        groupBox_3->setObjectName(QStringLiteral("groupBox_3"));
        formLayout_2 = new QFormLayout(groupBox_3);
        formLayout_2->setObjectName(QStringLiteral("formLayout_2"));
        formLayout_2->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
        formLayout_2->setFormAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);
        label_12 = new QLabel(groupBox_3);
        label_12->setObjectName(QStringLiteral("label_12"));

        formLayout_2->setWidget(0, QFormLayout::LabelRole, label_12);

        OutputFormat = new QComboBox(groupBox_3);
        OutputFormat->insertItems(0, QStringList()
         << QStringLiteral("-")
         << QStringLiteral("MOBI")
         << QStringLiteral("EPUB")
         << QStringLiteral("AZW3")
         << QStringLiteral("MOBI7")
        );
        OutputFormat->setObjectName(QStringLiteral("OutputFormat"));

        formLayout_2->setWidget(0, QFormLayout::FieldRole, OutputFormat);

        label_10 = new QLabel(groupBox_3);
        label_10->setObjectName(QStringLiteral("label_10"));

        formLayout_2->setWidget(1, QFormLayout::LabelRole, label_10);

        CurrentTools = new QComboBox(groupBox_3);
        CurrentTools->setObjectName(QStringLiteral("CurrentTools"));
        QSizePolicy sizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(CurrentTools->sizePolicy().hasHeightForWidth());
        CurrentTools->setSizePolicy(sizePolicy);

        formLayout_2->setWidget(1, QFormLayout::FieldRole, CurrentTools);


        verticalLayout_4->addWidget(groupBox_3);

        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setObjectName(QStringLiteral("horizontalLayout_5"));
        radioDevice = new QRadioButton(tab);
        radioDevice->setObjectName(QStringLiteral("radioDevice"));
        radioDevice->setChecked(true);

        horizontalLayout_5->addWidget(radioDevice);

        radioEmail = new QRadioButton(tab);
        radioEmail->setObjectName(QStringLiteral("radioEmail"));

        horizontalLayout_5->addWidget(radioEmail);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_5->addItem(horizontalSpacer);


        verticalLayout_4->addLayout(horizontalLayout_5);

        stackedWidget = new QStackedWidget(tab);
        stackedWidget->setObjectName(QStringLiteral("stackedWidget"));
        page_3 = new QWidget();
        page_3->setObjectName(QStringLiteral("page_3"));
        verticalLayout_3 = new QVBoxLayout(page_3);
#ifndef Q_OS_MAC
        verticalLayout_3->setSpacing(-1);
#endif
        verticalLayout_3->setObjectName(QStringLiteral("verticalLayout_3"));
        verticalLayout_3->setContentsMargins(0, 0, 0, 0);
        groupBox_2 = new QGroupBox(page_3);
        groupBox_2->setObjectName(QStringLiteral("groupBox_2"));
        verticalLayout_6 = new QVBoxLayout(groupBox_2);
        verticalLayout_6->setObjectName(QStringLiteral("verticalLayout_6"));
        PostprocessingCopy = new QCheckBox(groupBox_2);
        PostprocessingCopy->setObjectName(QStringLiteral("PostprocessingCopy"));

        verticalLayout_6->addWidget(PostprocessingCopy);

        askPath = new QCheckBox(groupBox_2);
        askPath->setObjectName(QStringLiteral("askPath"));

        verticalLayout_6->addWidget(askPath);

        label_6 = new QLabel(groupBox_2);
        label_6->setObjectName(QStringLiteral("label_6"));

        verticalLayout_6->addWidget(label_6);

        Path = new QLineEdit(groupBox_2);
        Path->setObjectName(QStringLiteral("Path"));

        verticalLayout_6->addWidget(Path);

        originalFileName = new QCheckBox(groupBox_2);
        originalFileName->setObjectName(QStringLiteral("originalFileName"));

        verticalLayout_6->addWidget(originalFileName);

        label_exportname = new QLabel(groupBox_2);
        label_exportname->setObjectName(QStringLiteral("label_exportname"));

        verticalLayout_6->addWidget(label_exportname);

        horizontalLayout_9 = new QHBoxLayout();
#ifndef Q_OS_MAC
        horizontalLayout_9->setSpacing(-1);
#endif
        horizontalLayout_9->setObjectName(QStringLiteral("horizontalLayout_9"));
        ExportFileName = new QLineEdit(groupBox_2);
        ExportFileName->setObjectName(QStringLiteral("ExportFileName"));
        ExportFileName->setPlaceholderText(QStringLiteral("%a/%s/%n3%b"));

        horizontalLayout_9->addWidget(ExportFileName);

        transliteration = new QCheckBox(groupBox_2);
        transliteration->setObjectName(QStringLiteral("transliteration"));

        horizontalLayout_9->addWidget(transliteration);


        verticalLayout_6->addLayout(horizontalLayout_9);


        verticalLayout_3->addWidget(groupBox_2);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_3->addItem(verticalSpacer);

        stackedWidget->addWidget(page_3);
        page_4 = new QWidget();
        page_4->setObjectName(QStringLiteral("page_4"));
        verticalLayout_2 = new QVBoxLayout(page_4);
        verticalLayout_2->setObjectName(QStringLiteral("verticalLayout_2"));
        verticalLayout_2->setContentsMargins(0, 0, 0, 0);
        groupBox = new QGroupBox(page_4);
        groupBox->setObjectName(QStringLiteral("groupBox"));
        verticalLayout_5 = new QVBoxLayout(groupBox);
        verticalLayout_5->setObjectName(QStringLiteral("verticalLayout_5"));
        formLayout = new QFormLayout();
        formLayout->setObjectName(QStringLiteral("formLayout"));
        formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
        label_16 = new QLabel(groupBox);
        label_16->setObjectName(QStringLiteral("label_16"));

        formLayout->setWidget(0, QFormLayout::LabelRole, label_16);

        from_email = new QLineEdit(groupBox);
        from_email->setObjectName(QStringLiteral("from_email"));

        formLayout->setWidget(0, QFormLayout::FieldRole, from_email);

        label = new QLabel(groupBox);
        label->setObjectName(QStringLiteral("label"));

        formLayout->setWidget(1, QFormLayout::LabelRole, label);

        Email = new QLineEdit(groupBox);
        Email->setObjectName(QStringLiteral("Email"));

        formLayout->setWidget(1, QFormLayout::FieldRole, Email);

        label_20 = new QLabel(groupBox);
        label_20->setObjectName(QStringLiteral("label_20"));

        formLayout->setWidget(2, QFormLayout::LabelRole, label_20);

        mail_subject = new QLineEdit(groupBox);
        mail_subject->setObjectName(QStringLiteral("mail_subject"));
        mail_subject->setPlaceholderText(QStringLiteral("freeLib"));

        formLayout->setWidget(2, QFormLayout::FieldRole, mail_subject);

        label_2 = new QLabel(groupBox);
        label_2->setObjectName(QStringLiteral("label_2"));

        formLayout->setWidget(3, QFormLayout::LabelRole, label_2);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        Server = new QLineEdit(groupBox);
        Server->setObjectName(QStringLiteral("Server"));

        horizontalLayout->addWidget(Server);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QStringLiteral("horizontalLayout_3"));
        label_11 = new QLabel(groupBox);
        label_11->setObjectName(QStringLiteral("label_11"));

        horizontalLayout_3->addWidget(label_11);

        ConnectionType = new QComboBox(groupBox);
        ConnectionType->insertItems(0, QStringList()
         << QStringLiteral("Tcp")
         << QStringLiteral("Ssl")
         << QStringLiteral("Tsl")
        );
        ConnectionType->setObjectName(QStringLiteral("ConnectionType"));

        horizontalLayout_3->addWidget(ConnectionType);

        label_4 = new QLabel(groupBox);
        label_4->setObjectName(QStringLiteral("label_4"));
        label_4->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout_3->addWidget(label_4);

        Port = new QLineEdit(groupBox);
        Port->setObjectName(QStringLiteral("Port"));

        horizontalLayout_3->addWidget(Port);


        horizontalLayout->addLayout(horizontalLayout_3);

        horizontalLayout->setStretch(1, 3);

        formLayout->setLayout(3, QFormLayout::FieldRole, horizontalLayout);

        label_3 = new QLabel(groupBox);
        label_3->setObjectName(QStringLiteral("label_3"));

        formLayout->setWidget(4, QFormLayout::LabelRole, label_3);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        User = new QLineEdit(groupBox);
        User->setObjectName(QStringLiteral("User"));

        horizontalLayout_2->addWidget(User);

        horizontalLayout_6 = new QHBoxLayout();
        horizontalLayout_6->setObjectName(QStringLiteral("horizontalLayout_6"));
        label_5 = new QLabel(groupBox);
        label_5->setObjectName(QStringLiteral("label_5"));
        label_5->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout_6->addWidget(label_5);

        Password = new QLineEdit(groupBox);
        Password->setObjectName(QStringLiteral("Password"));
        Password->setEchoMode(QLineEdit::Password);

        horizontalLayout_6->addWidget(Password);


        horizontalLayout_2->addLayout(horizontalLayout_6);

        horizontalLayout_2->setStretch(0, 2);
        horizontalLayout_2->setStretch(1, 3);

        formLayout->setLayout(4, QFormLayout::FieldRole, horizontalLayout_2);

        label_9 = new QLabel(groupBox);
        label_9->setObjectName(QStringLiteral("label_9"));

        formLayout->setWidget(5, QFormLayout::LabelRole, label_9);

        PauseMail = new QSpinBox(groupBox);
        PauseMail->setObjectName(QStringLiteral("PauseMail"));

        formLayout->setWidget(5, QFormLayout::FieldRole, PauseMail);


        verticalLayout_5->addLayout(formLayout);


        verticalLayout_2->addWidget(groupBox);

        verticalSpacer_4 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_2->addItem(verticalSpacer_4);

        stackedWidget->addWidget(page_4);

        verticalLayout_4->addWidget(stackedWidget);

        tabWidget->addTab(tab, QString());
        tabFormat = new QWidget();
        tabFormat->setObjectName(QStringLiteral("tabFormat"));
        verticalLayout = new QVBoxLayout(tabFormat);
        verticalLayout->setSpacing(4);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        verticalLayout->setContentsMargins(4, 4, 4, 4);
        toolBox = new QToolBox(tabFormat);
        toolBox->setObjectName(QStringLiteral("toolBox"));
        page_2 = new QWidget();
        page_2->setObjectName(QStringLiteral("page_2"));
        page_2->setGeometry(QRect(0, 0, 664, 296));
        verticalLayout_9 = new QVBoxLayout(page_2);
        verticalLayout_9->setSpacing(0);
        verticalLayout_9->setObjectName(QStringLiteral("verticalLayout_9"));
        verticalLayout_9->setContentsMargins(0, 0, 0, 0);
        GB2 = new QFrame(page_2);
        GB2->setObjectName(QStringLiteral("GB2"));
        GB2->setFrameShape(QFrame::StyledPanel);
        verticalLayout_13 = new QVBoxLayout(GB2);
        verticalLayout_13->setObjectName(QStringLiteral("verticalLayout_13"));
        verticalLayout_13->setContentsMargins(4, 4, 4, 4);
        label_13 = new QLabel(GB2);
        label_13->setObjectName(QStringLiteral("label_13"));

        verticalLayout_13->addWidget(label_13);

        horizontalLayout_10 = new QHBoxLayout();
        horizontalLayout_10->setObjectName(QStringLiteral("horizontalLayout_10"));
        authorstring = new QLineEdit(GB2);
        authorstring->setObjectName(QStringLiteral("authorstring"));
        authorstring->setPlaceholderText(QStringLiteral("%nf %nm %nl"));

        horizontalLayout_10->addWidget(authorstring);

        authorTranslit = new QCheckBox(GB2);
        authorTranslit->setObjectName(QStringLiteral("authorTranslit"));

        horizontalLayout_10->addWidget(authorTranslit);


        verticalLayout_13->addLayout(horizontalLayout_10);

        Label22 = new QLabel(GB2);
        Label22->setObjectName(QStringLiteral("Label22"));

        verticalLayout_13->addWidget(Label22);

        horizontalLayout_11 = new QHBoxLayout();
        horizontalLayout_11->setObjectName(QStringLiteral("horizontalLayout_11"));
        seriastring = new QLineEdit(GB2);
        seriastring->setObjectName(QStringLiteral("seriastring"));
        seriastring->setPlaceholderText(QStringLiteral("(%abbrs %n2) %b"));

        horizontalLayout_11->addWidget(seriastring);

        seriaTranslit = new QCheckBox(GB2);
        seriaTranslit->setObjectName(QStringLiteral("seriaTranslit"));

        horizontalLayout_11->addWidget(seriaTranslit);


        verticalLayout_13->addLayout(horizontalLayout_11);

        repairCover = new QCheckBox(GB2);
        repairCover->setObjectName(QStringLiteral("repairCover"));

        verticalLayout_13->addWidget(repairCover);

        removePersonal = new QCheckBox(GB2);
        removePersonal->setObjectName(QStringLiteral("removePersonal"));

        verticalLayout_13->addWidget(removePersonal);

        createCover = new QCheckBox(GB2);
        createCover->setObjectName(QStringLiteral("createCover"));

        verticalLayout_13->addWidget(createCover);

        createCaverAlways = new QCheckBox(GB2);
        createCaverAlways->setObjectName(QStringLiteral("createCaverAlways"));

        verticalLayout_13->addWidget(createCaverAlways);

        addCoverLabel = new QCheckBox(GB2);
        addCoverLabel->setObjectName(QStringLiteral("addCoverLabel"));

        verticalLayout_13->addWidget(addCoverLabel);

        label_tmplate = new QLabel(GB2);
        label_tmplate->setObjectName(QStringLiteral("label_tmplate"));

        verticalLayout_13->addWidget(label_tmplate);

        coverLabel = new QLineEdit(GB2);
        coverLabel->setObjectName(QStringLiteral("coverLabel"));
        coverLabel->setPlaceholderText(QStringLiteral("%abbrs - %n2"));

        verticalLayout_13->addWidget(coverLabel);

        verticalSpacer_6 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_13->addItem(verticalSpacer_6);


        verticalLayout_9->addWidget(GB2);

        toolBox->addItem(page_2, QStringLiteral("Title and cover"));
        page_5 = new QWidget();
        page_5->setObjectName(QStringLiteral("page_5"));
        page_5->setGeometry(QRect(0, 0, 386, 296));
        verticalLayout_10 = new QVBoxLayout(page_5);
        verticalLayout_10->setObjectName(QStringLiteral("verticalLayout_10"));
        verticalLayout_10->setContentsMargins(0, 0, 0, 0);
        GB5 = new QFrame(page_5);
        GB5->setObjectName(QStringLiteral("GB5"));
        GB5->setFrameShape(QFrame::StyledPanel);
        horizontalLayout_14 = new QHBoxLayout(GB5);
        horizontalLayout_14->setObjectName(QStringLiteral("horizontalLayout_14"));
        horizontalLayout_14->setContentsMargins(4, 4, 4, 4);
        verticalLayout_16 = new QVBoxLayout();
        verticalLayout_16->setObjectName(QStringLiteral("verticalLayout_16"));
        label_27 = new QLabel(GB5);
        label_27->setObjectName(QStringLiteral("label_27"));

        verticalLayout_16->addWidget(label_27);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName(QStringLiteral("horizontalLayout_4"));
        footnotes = new QComboBox(GB5);
        footnotes->setObjectName(QStringLiteral("footnotes"));

        horizontalLayout_4->addWidget(footnotes);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer_2);


        verticalLayout_16->addLayout(horizontalLayout_4);

        label_15 = new QLabel(GB5);
        label_15->setObjectName(QStringLiteral("label_15"));

        verticalLayout_16->addWidget(label_15);

        horizontalLayout_7 = new QHBoxLayout();
        horizontalLayout_7->setObjectName(QStringLiteral("horizontalLayout_7"));
        hyphenate = new QComboBox(GB5);
        hyphenate->setObjectName(QStringLiteral("hyphenate"));

        horizontalLayout_7->addWidget(hyphenate);

        horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_7->addItem(horizontalSpacer_4);


        verticalLayout_16->addLayout(horizontalLayout_7);

        label_18 = new QLabel(GB5);
        label_18->setObjectName(QStringLiteral("label_18"));

        verticalLayout_16->addWidget(label_18);

        horizontalLayout_8 = new QHBoxLayout();
        horizontalLayout_8->setObjectName(QStringLiteral("horizontalLayout_8"));
        Vignette = new QComboBox(GB5);
        Vignette->setObjectName(QStringLiteral("Vignette"));

        horizontalLayout_8->addWidget(Vignette);

        horizontalSpacer_5 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_8->addItem(horizontalSpacer_5);


        verticalLayout_16->addLayout(horizontalLayout_8);

        break_after_cupture = new QCheckBox(GB5);
        break_after_cupture->setObjectName(QStringLiteral("break_after_cupture"));

        verticalLayout_16->addWidget(break_after_cupture);

        dropcaps = new QCheckBox(GB5);
        dropcaps->setObjectName(QStringLiteral("dropcaps"));

        verticalLayout_16->addWidget(dropcaps);

        label_7 = new QLabel(GB5);
        label_7->setObjectName(QStringLiteral("label_7"));

        verticalLayout_16->addWidget(label_7);

        horizontalLayout_13 = new QHBoxLayout();
        horizontalLayout_13->setObjectName(QStringLiteral("horizontalLayout_13"));
        content_placement = new QComboBox(GB5);
        content_placement->setObjectName(QStringLiteral("content_placement"));

        horizontalLayout_13->addWidget(content_placement);

        horizontalSpacer_7 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_13->addItem(horizontalSpacer_7);


        verticalLayout_16->addLayout(horizontalLayout_13);

        verticalSpacer_5 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_16->addItem(verticalSpacer_5);


        horizontalLayout_14->addLayout(verticalLayout_16);

        verticalLayout_14 = new QVBoxLayout();
        verticalLayout_14->setSpacing(0);
        verticalLayout_14->setObjectName(QStringLiteral("verticalLayout_14"));
        horizontalLayout_15 = new QHBoxLayout();
        horizontalLayout_15->setObjectName(QStringLiteral("horizontalLayout_15"));
        userCSS = new QCheckBox(GB5);
        userCSS->setObjectName(QStringLiteral("userCSS"));

        horizontalLayout_15->addWidget(userCSS);

        horizontalSpacer_8 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_15->addItem(horizontalSpacer_8);

        btnDefaultCSS = new QToolButton(GB5);
        btnDefaultCSS->setObjectName(QStringLiteral("btnDefaultCSS"));
        btnDefaultCSS->setMaximumSize(QSize(20, 20));
        btnDefaultCSS->setText(QStringLiteral("..."));
        QIcon icon;
        icon.addFile(QStringLiteral(":/icons/img/icons/open.png"), QSize(), QIcon::Normal, QIcon::Off);
        btnDefaultCSS->setIcon(icon);

        horizontalLayout_15->addWidget(btnDefaultCSS);


        verticalLayout_14->addLayout(horizontalLayout_15);

        UserCSStext = new QPlainTextEdit(GB5);
        UserCSStext->setObjectName(QStringLiteral("UserCSStext"));

        verticalLayout_14->addWidget(UserCSStext);

        verticalSpacer_7 = new QSpacerItem(20, 5, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_14->addItem(verticalSpacer_7);

        verticalLayout_14->setStretch(1, 1);

        horizontalLayout_14->addLayout(verticalLayout_14);


        verticalLayout_10->addWidget(GB5);

        toolBox->addItem(page_5, QStringLiteral("Style"));
        page = new QWidget();
        page->setObjectName(QStringLiteral("page"));
        page->setGeometry(QRect(0, 0, 679, 252));
        verticalLayout_12 = new QVBoxLayout(page);
        verticalLayout_12->setObjectName(QStringLiteral("verticalLayout_12"));
        verticalLayout_12->setContentsMargins(0, 0, 0, 0);
        frame = new QFrame(page);
        frame->setObjectName(QStringLiteral("frame"));
        frame->setFrameShape(QFrame::StyledPanel);
        fontLayout = new QVBoxLayout(frame);
        fontLayout->setSpacing(2);
        fontLayout->setObjectName(QStringLiteral("fontLayout"));
        fontLayout->setContentsMargins(4, 4, 4, 4);
        Layout = new QHBoxLayout();
        Layout->setObjectName(QStringLiteral("Layout"));
        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        Layout->addItem(horizontalSpacer_3);

        AddFont = new QPushButton(frame);
        AddFont->setObjectName(QStringLiteral("AddFont"));

        Layout->addWidget(AddFont);


        fontLayout->addLayout(Layout);

        verticalSpacer_2 = new QSpacerItem(20, 195, QSizePolicy::Minimum, QSizePolicy::Expanding);

        fontLayout->addItem(verticalSpacer_2);


        verticalLayout_12->addWidget(frame);

        toolBox->addItem(page, QStringLiteral("Fonts"));
        page_6 = new QWidget();
        page_6->setObjectName(QStringLiteral("page_6"));
        page_6->setGeometry(QRect(0, 0, 228, 132));
        verticalLayout_11 = new QVBoxLayout(page_6);
        verticalLayout_11->setObjectName(QStringLiteral("verticalLayout_11"));
        verticalLayout_11->setContentsMargins(0, 0, 0, 0);
        GB6 = new QFrame(page_6);
        GB6->setObjectName(QStringLiteral("GB6"));
        GB6->setFrameShape(QFrame::StyledPanel);
        verticalLayout_7 = new QVBoxLayout(GB6);
        verticalLayout_7->setObjectName(QStringLiteral("verticalLayout_7"));
        verticalLayout_7->setContentsMargins(4, 4, 4, 4);
        annotation = new QCheckBox(GB6);
        annotation->setObjectName(QStringLiteral("annotation"));

        verticalLayout_7->addWidget(annotation);

        split_file = new QCheckBox(GB6);
        split_file->setObjectName(QStringLiteral("split_file"));

        verticalLayout_7->addWidget(split_file);

        join_series = new QCheckBox(GB6);
        join_series->setObjectName(QStringLiteral("join_series"));

        verticalLayout_7->addWidget(join_series);

        ml_toc = new QCheckBox(GB6);
        ml_toc->setObjectName(QStringLiteral("ml_toc"));

        verticalLayout_7->addWidget(ml_toc);

        horizontalLayout_12 = new QHBoxLayout();
        horizontalLayout_12->setObjectName(QStringLiteral("horizontalLayout_12"));
        label_lavel = new QLabel(GB6);
        label_lavel->setObjectName(QStringLiteral("label_lavel"));

        horizontalLayout_12->addWidget(label_lavel);

        MAXcaptionLevel = new QSpinBox(GB6);
        MAXcaptionLevel->setObjectName(QStringLiteral("MAXcaptionLevel"));
        MAXcaptionLevel->setMinimum(2);
        MAXcaptionLevel->setMaximum(10);

        horizontalLayout_12->addWidget(MAXcaptionLevel);

        horizontalSpacer_6 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_12->addItem(horizontalSpacer_6);

        horizontalLayout_12->setStretch(2, 1);

        verticalLayout_7->addLayout(horizontalLayout_12);

        verticalSpacer_3 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_7->addItem(verticalSpacer_3);


        verticalLayout_11->addWidget(GB6);

        toolBox->addItem(page_6, QStringLiteral("Advanced"));

        verticalLayout->addWidget(toolBox);

        tabWidget->addTab(tabFormat, QString());

        verticalLayout_8->addWidget(tabWidget);

#ifndef QT_NO_SHORTCUT
        label_12->setBuddy(GB2);
        label_27->setBuddy(GB5);
        label_lavel->setBuddy(GB6);
#endif // QT_NO_SHORTCUT
        QWidget::setTabOrder(OutputFormat, CurrentTools);
        QWidget::setTabOrder(CurrentTools, radioDevice);
        QWidget::setTabOrder(radioDevice, radioEmail);
        QWidget::setTabOrder(radioEmail, ExportFileName);
        QWidget::setTabOrder(ExportFileName, transliteration);
        QWidget::setTabOrder(transliteration, from_email);
        QWidget::setTabOrder(from_email, Email);
        QWidget::setTabOrder(Email, mail_subject);
        QWidget::setTabOrder(mail_subject, Server);
        QWidget::setTabOrder(Server, ConnectionType);
        QWidget::setTabOrder(ConnectionType, Port);
        QWidget::setTabOrder(Port, User);
        QWidget::setTabOrder(User, Password);
        QWidget::setTabOrder(Password, PauseMail);
        QWidget::setTabOrder(PauseMail, AddFont);
        QWidget::setTabOrder(AddFont, tabWidget);

        retranslateUi(ExportFrame);

        tabWidget->setCurrentIndex(1);
        stackedWidget->setCurrentIndex(1);
        toolBox->setCurrentIndex(2);


        QMetaObject::connectSlotsByName(ExportFrame);
    } // setupUi

    void retranslateUi(QFrame *ExportFrame)
    {
        groupBox_3->setTitle(QApplication::translate("ExportFrame", "Export parameters", Q_NULLPTR));
        label_12->setText(QApplication::translate("ExportFrame", "Convert to:", Q_NULLPTR));
        label_10->setText(QApplication::translate("ExportFrame", "Postprocessing:", Q_NULLPTR));
        CurrentTools->clear();
        CurrentTools->insertItems(0, QStringList()
         << QApplication::translate("ExportFrame", "nothing", Q_NULLPTR)
        );
        radioDevice->setText(QApplication::translate("ExportFrame", "Send to device", Q_NULLPTR));
        radioEmail->setText(QApplication::translate("ExportFrame", "Send to e-mail", Q_NULLPTR));
        groupBox_2->setTitle(QString());
        PostprocessingCopy->setText(QApplication::translate("ExportFrame", "Use postprocessing for copy", Q_NULLPTR));
        askPath->setText(QApplication::translate("ExportFrame", "Ask path before export", Q_NULLPTR));
        label_6->setText(QApplication::translate("ExportFrame", "Path:", Q_NULLPTR));
        originalFileName->setText(QApplication::translate("ExportFrame", "Use original file name", Q_NULLPTR));
        label_exportname->setText(QApplication::translate("ExportFrame", "File name:", Q_NULLPTR));
        transliteration->setText(QApplication::translate("ExportFrame", "transliteration", Q_NULLPTR));
        groupBox->setTitle(QString());
        label_16->setText(QApplication::translate("ExportFrame", "From:", Q_NULLPTR));
        from_email->setPlaceholderText(QApplication::translate("ExportFrame", "e-mail", Q_NULLPTR));
        label->setText(QApplication::translate("ExportFrame", "To:", Q_NULLPTR));
        Email->setPlaceholderText(QApplication::translate("ExportFrame", "e-mail", Q_NULLPTR));
        label_20->setText(QApplication::translate("ExportFrame", "Subject:", Q_NULLPTR));
        mail_subject->setText(QString());
        label_2->setText(QApplication::translate("ExportFrame", "Server:", Q_NULLPTR));
        Server->setPlaceholderText(QApplication::translate("ExportFrame", "server address", Q_NULLPTR));
        label_11->setText(QApplication::translate("ExportFrame", "Type:", Q_NULLPTR));
        label_4->setText(QApplication::translate("ExportFrame", "Port:", Q_NULLPTR));
        Port->setPlaceholderText(QApplication::translate("ExportFrame", "port", Q_NULLPTR));
        label_3->setText(QApplication::translate("ExportFrame", "User:", Q_NULLPTR));
        User->setPlaceholderText(QApplication::translate("ExportFrame", "user name", Q_NULLPTR));
        label_5->setText(QApplication::translate("ExportFrame", "Password:", Q_NULLPTR));
        Password->setPlaceholderText(QApplication::translate("ExportFrame", "password", Q_NULLPTR));
        label_9->setText(QApplication::translate("ExportFrame", "Pause:", Q_NULLPTR));
        tabWidget->setTabText(tabWidget->indexOf(tab), QApplication::translate("ExportFrame", "Export", Q_NULLPTR));
        label_13->setText(QApplication::translate("ExportFrame", "Author string:", Q_NULLPTR));
        authorTranslit->setText(QApplication::translate("ExportFrame", "Transliteration", Q_NULLPTR));
        Label22->setText(QApplication::translate("ExportFrame", "Book title:", Q_NULLPTR));
        seriaTranslit->setText(QApplication::translate("ExportFrame", "Transliteration", Q_NULLPTR));
        repairCover->setText(QApplication::translate("ExportFrame", "Repair cover", Q_NULLPTR));
        removePersonal->setText(QApplication::translate("ExportFrame", "Remove 'Personal' tag (AZW3 only)", Q_NULLPTR));
        createCover->setText(QApplication::translate("ExportFrame", "Create cover if not exists", Q_NULLPTR));
        createCaverAlways->setText(QApplication::translate("ExportFrame", "Always create cover", Q_NULLPTR));
        addCoverLabel->setText(QApplication::translate("ExportFrame", "Add additional label to existing cover", Q_NULLPTR));
        label_tmplate->setText(QApplication::translate("ExportFrame", "Additional label for existing cover:", Q_NULLPTR));
        toolBox->setItemText(toolBox->indexOf(page_2), QApplication::translate("ExportFrame", "Title and cover", Q_NULLPTR));
        label_27->setText(QApplication::translate("ExportFrame", "Show footnotes:", Q_NULLPTR));
        footnotes->clear();
        footnotes->insertItems(0, QStringList()
         << QApplication::translate("ExportFrame", "at the end of the book", Q_NULLPTR)
         << QApplication::translate("ExportFrame", "in text", Q_NULLPTR)
         << QApplication::translate("ExportFrame", "after paragraph", Q_NULLPTR)
         << QApplication::translate("ExportFrame", "popup", Q_NULLPTR)
        );
        label_15->setText(QApplication::translate("ExportFrame", "Hyphenate:", Q_NULLPTR));
        hyphenate->clear();
        hyphenate->insertItems(0, QStringList()
         << QApplication::translate("ExportFrame", "no", Q_NULLPTR)
         << QApplication::translate("ExportFrame", "soft", Q_NULLPTR)
         << QApplication::translate("ExportFrame", "children", Q_NULLPTR)
        );
        label_18->setText(QApplication::translate("ExportFrame", "Vignette:", Q_NULLPTR));
        Vignette->clear();
        Vignette->insertItems(0, QStringList()
         << QApplication::translate("ExportFrame", "no", Q_NULLPTR)
         << QApplication::translate("ExportFrame", "image", Q_NULLPTR)
         << QApplication::translate("ExportFrame", "text", Q_NULLPTR)
        );
        break_after_cupture->setText(QApplication::translate("ExportFrame", "Page break between chapters", Q_NULLPTR));
        dropcaps->setText(QApplication::translate("ExportFrame", "Insert dropcaps", Q_NULLPTR));
        label_7->setText(QApplication::translate("ExportFrame", "Content:", Q_NULLPTR));
        content_placement->clear();
        content_placement->insertItems(0, QStringList()
         << QApplication::translate("ExportFrame", "no content", Q_NULLPTR)
         << QApplication::translate("ExportFrame", "at the beginning of the book", Q_NULLPTR)
         << QApplication::translate("ExportFrame", "at the end of the book", Q_NULLPTR)
        );
        userCSS->setText(QApplication::translate("ExportFrame", "User CSS", Q_NULLPTR));
#ifndef QT_NO_TOOLTIP
        btnDefaultCSS->setToolTip(QApplication::translate("ExportFrame", "Load default CSS", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
        toolBox->setItemText(toolBox->indexOf(page_5), QApplication::translate("ExportFrame", "Style", Q_NULLPTR));
        AddFont->setText(QApplication::translate("ExportFrame", "Add", Q_NULLPTR));
        toolBox->setItemText(toolBox->indexOf(page), QApplication::translate("ExportFrame", "Fonts", Q_NULLPTR));
        annotation->setText(QApplication::translate("ExportFrame", "Don't add annotation", Q_NULLPTR));
        split_file->setText(QApplication::translate("ExportFrame", "Split into files by chapters", Q_NULLPTR));
        join_series->setText(QApplication::translate("ExportFrame", "Join a series to single book", Q_NULLPTR));
        ml_toc->setText(QApplication::translate("ExportFrame", "Use multi-level table of contents", Q_NULLPTR));
        label_lavel->setText(QApplication::translate("ExportFrame", "Maximum caption level:", Q_NULLPTR));
        toolBox->setItemText(toolBox->indexOf(page_6), QApplication::translate("ExportFrame", "Advanced", Q_NULLPTR));
        tabWidget->setTabText(tabWidget->indexOf(tabFormat), QApplication::translate("ExportFrame", "fb2 to MOBI/AZW3/EPUB conversion", Q_NULLPTR));
        Q_UNUSED(ExportFrame);
    } // retranslateUi

};

namespace Ui {
    class ExportFrame: public Ui_ExportFrame {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_EXPORTFRAME_H
