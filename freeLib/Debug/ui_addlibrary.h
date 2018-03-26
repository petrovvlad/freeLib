/********************************************************************************
** Form generated from reading UI file 'addlibrary.ui'
**
** Created by: Qt User Interface Compiler version 5.9.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ADDLIBRARY_H
#define UI_ADDLIBRARY_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_AddLibrary
{
public:
    QVBoxLayout *verticalLayout;
    QGridLayout *gridLayout;
    QLabel *label_3;
    QLabel *HTTP;
    QLabel *label_4;
    QLabel *label_5;
    QHBoxLayout *Name_layout;
    QComboBox *ExistingLibs;
    QToolButton *Add;
    QToolButton *Del;
    QLineEdit *inpx;
    QLineEdit *BookDir;
    QLabel *label_6;
    QLabel *label_2;
    QLabel *OPDS;
    QGroupBox *update_group;
    QVBoxLayout *verticalLayout_2;
    QRadioButton *add_new;
    QRadioButton *del_old;
    QRadioButton *update_all;
    QCheckBox *firstAuthorOnly;
    QListWidget *Log;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *btnExport;
    QPushButton *btnUpdate;
    QPushButton *btnCancel;

    void setupUi(QDialog *AddLibrary)
    {
        if (AddLibrary->objectName().isEmpty())
            AddLibrary->setObjectName(QStringLiteral("AddLibrary"));
        AddLibrary->setWindowModality(Qt::WindowModal);
        AddLibrary->resize(705, 530);
        AddLibrary->setModal(true);
        verticalLayout = new QVBoxLayout(AddLibrary);
        verticalLayout->setSpacing(8);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        verticalLayout->setContentsMargins(8, 8, 8, 8);
        gridLayout = new QGridLayout();
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        label_3 = new QLabel(AddLibrary);
        label_3->setObjectName(QStringLiteral("label_3"));

        gridLayout->addWidget(label_3, 4, 0, 1, 1);

        HTTP = new QLabel(AddLibrary);
        HTTP->setObjectName(QStringLiteral("HTTP"));
        HTTP->setText(QStringLiteral("<a href=\\\"localhost\\\">localhost</a>"));
        HTTP->setOpenExternalLinks(true);

        gridLayout->addWidget(HTTP, 2, 1, 1, 1);

        label_4 = new QLabel(AddLibrary);
        label_4->setObjectName(QStringLiteral("label_4"));

        gridLayout->addWidget(label_4, 0, 0, 1, 1);

        label_5 = new QLabel(AddLibrary);
        label_5->setObjectName(QStringLiteral("label_5"));

        gridLayout->addWidget(label_5, 1, 0, 1, 1);

        Name_layout = new QHBoxLayout();
        Name_layout->setObjectName(QStringLiteral("Name_layout"));
        ExistingLibs = new QComboBox(AddLibrary);
        ExistingLibs->setObjectName(QStringLiteral("ExistingLibs"));
        ExistingLibs->setEditable(true);

        Name_layout->addWidget(ExistingLibs);

        Add = new QToolButton(AddLibrary);
        Add->setObjectName(QStringLiteral("Add"));
        Add->setMaximumSize(QSize(22, 22));
        Add->setText(QStringLiteral("+"));
        QIcon icon;
        icon.addFile(QStringLiteral(":/icons/img/icons/plus.png"), QSize(), QIcon::Normal, QIcon::Off);
        Add->setIcon(icon);

        Name_layout->addWidget(Add);

        Del = new QToolButton(AddLibrary);
        Del->setObjectName(QStringLiteral("Del"));
        Del->setMaximumSize(QSize(22, 22));
        Del->setText(QStringLiteral("-"));
        QIcon icon1;
        icon1.addFile(QStringLiteral(":/icons/img/icons/minus.png"), QSize(), QIcon::Normal, QIcon::Off);
        Del->setIcon(icon1);

        Name_layout->addWidget(Del);


        gridLayout->addLayout(Name_layout, 0, 1, 1, 1);

        inpx = new QLineEdit(AddLibrary);
        inpx->setObjectName(QStringLiteral("inpx"));

        gridLayout->addWidget(inpx, 3, 1, 1, 1);

        BookDir = new QLineEdit(AddLibrary);
        BookDir->setObjectName(QStringLiteral("BookDir"));

        gridLayout->addWidget(BookDir, 4, 1, 1, 1);

        label_6 = new QLabel(AddLibrary);
        label_6->setObjectName(QStringLiteral("label_6"));

        gridLayout->addWidget(label_6, 2, 0, 1, 1);

        label_2 = new QLabel(AddLibrary);
        label_2->setObjectName(QStringLiteral("label_2"));

        gridLayout->addWidget(label_2, 3, 0, 1, 1);

        OPDS = new QLabel(AddLibrary);
        OPDS->setObjectName(QStringLiteral("OPDS"));
        OPDS->setText(QStringLiteral("<a href=\\\"localhost\\\">localhost</a>"));
        OPDS->setOpenExternalLinks(true);
        OPDS->setTextInteractionFlags(Qt::LinksAccessibleByKeyboard|Qt::LinksAccessibleByMouse);

        gridLayout->addWidget(OPDS, 1, 1, 1, 1);


        verticalLayout->addLayout(gridLayout);

        update_group = new QGroupBox(AddLibrary);
        update_group->setObjectName(QStringLiteral("update_group"));
        verticalLayout_2 = new QVBoxLayout(update_group);
        verticalLayout_2->setObjectName(QStringLiteral("verticalLayout_2"));
        add_new = new QRadioButton(update_group);
        add_new->setObjectName(QStringLiteral("add_new"));

        verticalLayout_2->addWidget(add_new);

        del_old = new QRadioButton(update_group);
        del_old->setObjectName(QStringLiteral("del_old"));

        verticalLayout_2->addWidget(del_old);

        update_all = new QRadioButton(update_group);
        update_all->setObjectName(QStringLiteral("update_all"));

        verticalLayout_2->addWidget(update_all);


        verticalLayout->addWidget(update_group);

        firstAuthorOnly = new QCheckBox(AddLibrary);
        firstAuthorOnly->setObjectName(QStringLiteral("firstAuthorOnly"));

        verticalLayout->addWidget(firstAuthorOnly);

        Log = new QListWidget(AddLibrary);
        Log->setObjectName(QStringLiteral("Log"));
        Log->setFocusPolicy(Qt::TabFocus);

        verticalLayout->addWidget(Log);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        btnExport = new QPushButton(AddLibrary);
        btnExport->setObjectName(QStringLiteral("btnExport"));
        btnExport->setMinimumSize(QSize(90, 0));

        horizontalLayout->addWidget(btnExport);

        btnUpdate = new QPushButton(AddLibrary);
        btnUpdate->setObjectName(QStringLiteral("btnUpdate"));
        btnUpdate->setMinimumSize(QSize(90, 0));

        horizontalLayout->addWidget(btnUpdate);

        btnCancel = new QPushButton(AddLibrary);
        btnCancel->setObjectName(QStringLiteral("btnCancel"));
        btnCancel->setMinimumSize(QSize(90, 0));
        btnCancel->setMaximumSize(QSize(16777215, 16777215));
        btnCancel->setAutoDefault(false);

        horizontalLayout->addWidget(btnCancel);


        verticalLayout->addLayout(horizontalLayout);

        QWidget::setTabOrder(inpx, BookDir);
        QWidget::setTabOrder(BookDir, Log);

        retranslateUi(AddLibrary);
        QObject::connect(btnCancel, SIGNAL(clicked()), AddLibrary, SLOT(reject()));

        btnCancel->setDefault(true);


        QMetaObject::connectSlotsByName(AddLibrary);
    } // setupUi

    void retranslateUi(QDialog *AddLibrary)
    {
        AddLibrary->setWindowTitle(QApplication::translate("AddLibrary", "Add/Edit library", Q_NULLPTR));
        label_3->setText(QApplication::translate("AddLibrary", "Books dir:", Q_NULLPTR));
        label_4->setText(QApplication::translate("AddLibrary", "Library name:", Q_NULLPTR));
        label_5->setText(QApplication::translate("AddLibrary", "OPDS server", Q_NULLPTR));
#ifndef QT_NO_TOOLTIP
        Add->setToolTip(QApplication::translate("AddLibrary", "Add new library", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        Del->setToolTip(QApplication::translate("AddLibrary", "Delete current library", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
        label_6->setText(QApplication::translate("AddLibrary", "HTTP server", Q_NULLPTR));
        label_2->setText(QApplication::translate("AddLibrary", "INPX file (optionally):", Q_NULLPTR));
        update_group->setTitle(QApplication::translate("AddLibrary", "Update type", Q_NULLPTR));
        add_new->setText(QApplication::translate("AddLibrary", "Add new", Q_NULLPTR));
        del_old->setText(QApplication::translate("AddLibrary", "Delete old and add new", Q_NULLPTR));
        update_all->setText(QApplication::translate("AddLibrary", "Recreate library", Q_NULLPTR));
        firstAuthorOnly->setText(QApplication::translate("AddLibrary", "import first author only", Q_NULLPTR));
        btnExport->setText(QApplication::translate("AddLibrary", "Export", Q_NULLPTR));
        btnUpdate->setText(QApplication::translate("AddLibrary", "Update", Q_NULLPTR));
        btnCancel->setText(QApplication::translate("AddLibrary", "Close", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class AddLibrary: public Ui_AddLibrary {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ADDLIBRARY_H
