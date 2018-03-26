/********************************************************************************
** Form generated from reading UI file 'tagdialog.ui'
**
** Created by: Qt User Interface Compiler version 5.9.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TAGDIALOG_H
#define UI_TAGDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_TagDialog
{
public:
    QVBoxLayout *verticalLayout;
    QListWidget *listWidget;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *TagDialog)
    {
        if (TagDialog->objectName().isEmpty())
            TagDialog->setObjectName(QStringLiteral("TagDialog"));
        TagDialog->setWindowModality(Qt::WindowModal);
        TagDialog->resize(378, 360);
        TagDialog->setModal(true);
        verticalLayout = new QVBoxLayout(TagDialog);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        listWidget = new QListWidget(TagDialog);
        listWidget->setObjectName(QStringLiteral("listWidget"));

        verticalLayout->addWidget(listWidget);

        buttonBox = new QDialogButtonBox(TagDialog);
        buttonBox->setObjectName(QStringLiteral("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(TagDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), TagDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), TagDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(TagDialog);
    } // setupUi

    void retranslateUi(QDialog *TagDialog)
    {
        TagDialog->setWindowTitle(QApplication::translate("TagDialog", "Tag", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class TagDialog: public Ui_TagDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TAGDIALOG_H
