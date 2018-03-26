/********************************************************************************
** Form generated from reading UI file 'bookeditdlg.ui'
**
** Created by: Qt User Interface Compiler version 5.9.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_BOOKEDITDLG_H
#define UI_BOOKEDITDLG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_BookEditDlg
{
public:
    QVBoxLayout *verticalLayout_5;
    QHBoxLayout *horizontalLayout_5;
    QVBoxLayout *verticalLayout_4;
    QHBoxLayout *horizontalLayout;
    QLabel *label;
    QLineEdit *book_name;
    QHBoxLayout *horizontalLayout_4;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_2;
    QComboBox *language;
    QHBoxLayout *horizontalLayout_3;
    QLabel *label_3;
    QLineEdit *series;
    QSpinBox *num_in_series;
    QGroupBox *groupBox;
    QHBoxLayout *horizontalLayout_6;
    QTableWidget *authors;
    QVBoxLayout *verticalLayout_7;
    QToolButton *toolButton_3;
    QToolButton *toolButton_2;
    QToolButton *toolButton;
    QSpacerItem *verticalSpacer;
    QGroupBox *groupBox_2;
    QHBoxLayout *horizontalLayout_7;
    QListWidget *ganres;
    QVBoxLayout *verticalLayout_2;
    QToolButton *toolButton_5;
    QToolButton *toolButton_6;
    QToolButton *toolButton_4;
    QSpacerItem *verticalSpacer_2;
    QFrame *frame;
    QVBoxLayout *verticalLayout;
    QLabel *cover;
    QGroupBox *groupBox_3;
    QVBoxLayout *verticalLayout_6;
    QPlainTextEdit *annotation;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *BookEditDlg)
    {
        if (BookEditDlg->objectName().isEmpty())
            BookEditDlg->setObjectName(QStringLiteral("BookEditDlg"));
        BookEditDlg->setWindowModality(Qt::WindowModal);
        BookEditDlg->resize(931, 647);
        BookEditDlg->setModal(true);
        verticalLayout_5 = new QVBoxLayout(BookEditDlg);
        verticalLayout_5->setObjectName(QStringLiteral("verticalLayout_5"));
        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setSpacing(4);
        horizontalLayout_5->setObjectName(QStringLiteral("horizontalLayout_5"));
        horizontalLayout_5->setContentsMargins(0, 0, 0, 0);
        verticalLayout_4 = new QVBoxLayout();
#ifndef Q_OS_MAC
        verticalLayout_4->setSpacing(-1);
#endif
        verticalLayout_4->setObjectName(QStringLiteral("verticalLayout_4"));
        verticalLayout_4->setContentsMargins(0, 0, 0, 0);
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(4);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        label = new QLabel(BookEditDlg);
        label->setObjectName(QStringLiteral("label"));

        horizontalLayout->addWidget(label);

        book_name = new QLineEdit(BookEditDlg);
        book_name->setObjectName(QStringLiteral("book_name"));

        horizontalLayout->addWidget(book_name);


        verticalLayout_4->addLayout(horizontalLayout);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setSpacing(4);
        horizontalLayout_4->setObjectName(QStringLiteral("horizontalLayout_4"));
        horizontalLayout_4->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setSpacing(4);
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        label_2 = new QLabel(BookEditDlg);
        label_2->setObjectName(QStringLiteral("label_2"));

        horizontalLayout_2->addWidget(label_2);

        language = new QComboBox(BookEditDlg);
        language->setObjectName(QStringLiteral("language"));
        language->setSizeAdjustPolicy(QComboBox::AdjustToContents);

        horizontalLayout_2->addWidget(language);


        horizontalLayout_4->addLayout(horizontalLayout_2);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setSpacing(4);
        horizontalLayout_3->setObjectName(QStringLiteral("horizontalLayout_3"));
        label_3 = new QLabel(BookEditDlg);
        label_3->setObjectName(QStringLiteral("label_3"));

        horizontalLayout_3->addWidget(label_3);

        series = new QLineEdit(BookEditDlg);
        series->setObjectName(QStringLiteral("series"));

        horizontalLayout_3->addWidget(series);

        num_in_series = new QSpinBox(BookEditDlg);
        num_in_series->setObjectName(QStringLiteral("num_in_series"));
        num_in_series->setMaximum(99999);

        horizontalLayout_3->addWidget(num_in_series);


        horizontalLayout_4->addLayout(horizontalLayout_3);


        verticalLayout_4->addLayout(horizontalLayout_4);

        groupBox = new QGroupBox(BookEditDlg);
        groupBox->setObjectName(QStringLiteral("groupBox"));
        horizontalLayout_6 = new QHBoxLayout(groupBox);
        horizontalLayout_6->setSpacing(4);
        horizontalLayout_6->setObjectName(QStringLiteral("horizontalLayout_6"));
        horizontalLayout_6->setContentsMargins(4, 4, 4, 4);
        authors = new QTableWidget(groupBox);
        if (authors->columnCount() < 3)
            authors->setColumnCount(3);
        QTableWidgetItem *__qtablewidgetitem = new QTableWidgetItem();
        authors->setHorizontalHeaderItem(0, __qtablewidgetitem);
        QTableWidgetItem *__qtablewidgetitem1 = new QTableWidgetItem();
        authors->setHorizontalHeaderItem(1, __qtablewidgetitem1);
        QTableWidgetItem *__qtablewidgetitem2 = new QTableWidgetItem();
        authors->setHorizontalHeaderItem(2, __qtablewidgetitem2);
        authors->setObjectName(QStringLiteral("authors"));

        horizontalLayout_6->addWidget(authors);

        verticalLayout_7 = new QVBoxLayout();
        verticalLayout_7->setObjectName(QStringLiteral("verticalLayout_7"));
        toolButton_3 = new QToolButton(groupBox);
        toolButton_3->setObjectName(QStringLiteral("toolButton_3"));

        verticalLayout_7->addWidget(toolButton_3);

        toolButton_2 = new QToolButton(groupBox);
        toolButton_2->setObjectName(QStringLiteral("toolButton_2"));

        verticalLayout_7->addWidget(toolButton_2);

        toolButton = new QToolButton(groupBox);
        toolButton->setObjectName(QStringLiteral("toolButton"));

        verticalLayout_7->addWidget(toolButton);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_7->addItem(verticalSpacer);


        horizontalLayout_6->addLayout(verticalLayout_7);


        verticalLayout_4->addWidget(groupBox);

        groupBox_2 = new QGroupBox(BookEditDlg);
        groupBox_2->setObjectName(QStringLiteral("groupBox_2"));
        horizontalLayout_7 = new QHBoxLayout(groupBox_2);
        horizontalLayout_7->setSpacing(4);
        horizontalLayout_7->setObjectName(QStringLiteral("horizontalLayout_7"));
        horizontalLayout_7->setContentsMargins(4, 4, 4, 4);
        ganres = new QListWidget(groupBox_2);
        ganres->setObjectName(QStringLiteral("ganres"));

        horizontalLayout_7->addWidget(ganres);

        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setObjectName(QStringLiteral("verticalLayout_2"));
        toolButton_5 = new QToolButton(groupBox_2);
        toolButton_5->setObjectName(QStringLiteral("toolButton_5"));

        verticalLayout_2->addWidget(toolButton_5);

        toolButton_6 = new QToolButton(groupBox_2);
        toolButton_6->setObjectName(QStringLiteral("toolButton_6"));

        verticalLayout_2->addWidget(toolButton_6);

        toolButton_4 = new QToolButton(groupBox_2);
        toolButton_4->setObjectName(QStringLiteral("toolButton_4"));

        verticalLayout_2->addWidget(toolButton_4);

        verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_2->addItem(verticalSpacer_2);


        horizontalLayout_7->addLayout(verticalLayout_2);


        verticalLayout_4->addWidget(groupBox_2);


        horizontalLayout_5->addLayout(verticalLayout_4);

        frame = new QFrame(BookEditDlg);
        frame->setObjectName(QStringLiteral("frame"));
        frame->setFrameShape(QFrame::StyledPanel);
        frame->setFrameShadow(QFrame::Raised);
        verticalLayout = new QVBoxLayout(frame);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        cover = new QLabel(frame);
        cover->setObjectName(QStringLiteral("cover"));
        cover->setAlignment(Qt::AlignCenter);

        verticalLayout->addWidget(cover);


        horizontalLayout_5->addWidget(frame);

        horizontalLayout_5->setStretch(0, 2);
        horizontalLayout_5->setStretch(1, 1);

        verticalLayout_5->addLayout(horizontalLayout_5);

        groupBox_3 = new QGroupBox(BookEditDlg);
        groupBox_3->setObjectName(QStringLiteral("groupBox_3"));
        verticalLayout_6 = new QVBoxLayout(groupBox_3);
        verticalLayout_6->setObjectName(QStringLiteral("verticalLayout_6"));
        verticalLayout_6->setContentsMargins(4, 4, 4, 4);
        annotation = new QPlainTextEdit(groupBox_3);
        annotation->setObjectName(QStringLiteral("annotation"));

        verticalLayout_6->addWidget(annotation);


        verticalLayout_5->addWidget(groupBox_3);

        buttonBox = new QDialogButtonBox(BookEditDlg);
        buttonBox->setObjectName(QStringLiteral("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout_5->addWidget(buttonBox);


        retranslateUi(BookEditDlg);
        QObject::connect(buttonBox, SIGNAL(accepted()), BookEditDlg, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), BookEditDlg, SLOT(reject()));

        QMetaObject::connectSlotsByName(BookEditDlg);
    } // setupUi

    void retranslateUi(QDialog *BookEditDlg)
    {
        BookEditDlg->setWindowTitle(QApplication::translate("BookEditDlg", "Dialog", Q_NULLPTR));
        label->setText(QApplication::translate("BookEditDlg", "Caption", Q_NULLPTR));
        label_2->setText(QApplication::translate("BookEditDlg", "Language", Q_NULLPTR));
        label_3->setText(QApplication::translate("BookEditDlg", "Seria", Q_NULLPTR));
        groupBox->setTitle(QApplication::translate("BookEditDlg", "Authors", Q_NULLPTR));
        QTableWidgetItem *___qtablewidgetitem = authors->horizontalHeaderItem(0);
        ___qtablewidgetitem->setText(QApplication::translate("BookEditDlg", "Secondname", Q_NULLPTR));
        QTableWidgetItem *___qtablewidgetitem1 = authors->horizontalHeaderItem(1);
        ___qtablewidgetitem1->setText(QApplication::translate("BookEditDlg", "Firstname", Q_NULLPTR));
        QTableWidgetItem *___qtablewidgetitem2 = authors->horizontalHeaderItem(2);
        ___qtablewidgetitem2->setText(QApplication::translate("BookEditDlg", "Middlename", Q_NULLPTR));
        toolButton_3->setText(QApplication::translate("BookEditDlg", "...", Q_NULLPTR));
        toolButton_2->setText(QApplication::translate("BookEditDlg", "...", Q_NULLPTR));
        toolButton->setText(QApplication::translate("BookEditDlg", "...", Q_NULLPTR));
        groupBox_2->setTitle(QApplication::translate("BookEditDlg", "Ganres", Q_NULLPTR));
        toolButton_5->setText(QApplication::translate("BookEditDlg", "...", Q_NULLPTR));
        toolButton_6->setText(QApplication::translate("BookEditDlg", "...", Q_NULLPTR));
        toolButton_4->setText(QApplication::translate("BookEditDlg", "...", Q_NULLPTR));
        cover->setText(QApplication::translate("BookEditDlg", "Cover", Q_NULLPTR));
        groupBox_3->setTitle(QApplication::translate("BookEditDlg", "Annotation", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class BookEditDlg: public Ui_BookEditDlg {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_BOOKEDITDLG_H
