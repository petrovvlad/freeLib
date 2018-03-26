/********************************************************************************
** Form generated from reading UI file 'libwizard.ui'
**
** Created by: Qt User Interface Compiler version 5.9.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_LIBWIZARD_H
#define UI_LIBWIZARD_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWizard>
#include <QtWidgets/QWizardPage>

QT_BEGIN_NAMESPACE

class Ui_LibWizard
{
public:
    QWizardPage *modePage;
    QVBoxLayout *verticalLayout_5;
    QGroupBox *groupBox;
    QVBoxLayout *verticalLayout_6;
    QRadioButton *mode_lib;
    QRadioButton *mode_convert;
    QWizardPage *wizardPage1;
    QVBoxLayout *verticalLayout;
    QLabel *label;
    QLineEdit *Dir;
    QWizardPage *wizardPage2;
    QVBoxLayout *verticalLayout_2;
    QLabel *label_2;
    QLineEdit *inpx;
    QWizardPage *wizardPage;
    QVBoxLayout *verticalLayout_3;
    QLabel *label_3;
    QLineEdit *lib_name;
    QWizardPage *wizardPage_2;
    QVBoxLayout *verticalLayout_4;
    QLabel *label_4;
    QLineEdit *r_dir;
    QLabel *label_5;
    QLineEdit *r_inpx;
    QLabel *label_6;
    QLineEdit *r_name;
    QCheckBox *update_lib;

    void setupUi(QWizard *LibWizard)
    {
        if (LibWizard->objectName().isEmpty())
            LibWizard->setObjectName(QStringLiteral("LibWizard"));
        LibWizard->setWindowModality(Qt::WindowModal);
        LibWizard->resize(642, 415);
        LibWizard->setModal(true);
        LibWizard->setWizardStyle(QWizard::MacStyle);
        LibWizard->setOptions(QWizard::NoDefaultButton);
        modePage = new QWizardPage();
        modePage->setObjectName(QStringLiteral("modePage"));
        verticalLayout_5 = new QVBoxLayout(modePage);
        verticalLayout_5->setObjectName(QStringLiteral("verticalLayout_5"));
        groupBox = new QGroupBox(modePage);
        groupBox->setObjectName(QStringLiteral("groupBox"));
        verticalLayout_6 = new QVBoxLayout(groupBox);
        verticalLayout_6->setSpacing(12);
        verticalLayout_6->setObjectName(QStringLiteral("verticalLayout_6"));
        mode_lib = new QRadioButton(groupBox);
        mode_lib->setObjectName(QStringLiteral("mode_lib"));
        mode_lib->setChecked(true);

        verticalLayout_6->addWidget(mode_lib);

        mode_convert = new QRadioButton(groupBox);
        mode_convert->setObjectName(QStringLiteral("mode_convert"));

        verticalLayout_6->addWidget(mode_convert);


        verticalLayout_5->addWidget(groupBox);

        LibWizard->addPage(modePage);
        wizardPage1 = new QWizardPage();
        wizardPage1->setObjectName(QStringLiteral("wizardPage1"));
        verticalLayout = new QVBoxLayout(wizardPage1);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        label = new QLabel(wizardPage1);
        label->setObjectName(QStringLiteral("label"));

        verticalLayout->addWidget(label);

        Dir = new QLineEdit(wizardPage1);
        Dir->setObjectName(QStringLiteral("Dir"));

        verticalLayout->addWidget(Dir);

        LibWizard->addPage(wizardPage1);
        wizardPage2 = new QWizardPage();
        wizardPage2->setObjectName(QStringLiteral("wizardPage2"));
        verticalLayout_2 = new QVBoxLayout(wizardPage2);
        verticalLayout_2->setObjectName(QStringLiteral("verticalLayout_2"));
        label_2 = new QLabel(wizardPage2);
        label_2->setObjectName(QStringLiteral("label_2"));

        verticalLayout_2->addWidget(label_2);

        inpx = new QLineEdit(wizardPage2);
        inpx->setObjectName(QStringLiteral("inpx"));

        verticalLayout_2->addWidget(inpx);

        LibWizard->addPage(wizardPage2);
        wizardPage = new QWizardPage();
        wizardPage->setObjectName(QStringLiteral("wizardPage"));
        verticalLayout_3 = new QVBoxLayout(wizardPage);
        verticalLayout_3->setObjectName(QStringLiteral("verticalLayout_3"));
        label_3 = new QLabel(wizardPage);
        label_3->setObjectName(QStringLiteral("label_3"));

        verticalLayout_3->addWidget(label_3);

        lib_name = new QLineEdit(wizardPage);
        lib_name->setObjectName(QStringLiteral("lib_name"));

        verticalLayout_3->addWidget(lib_name);

        LibWizard->addPage(wizardPage);
        wizardPage_2 = new QWizardPage();
        wizardPage_2->setObjectName(QStringLiteral("wizardPage_2"));
        verticalLayout_4 = new QVBoxLayout(wizardPage_2);
        verticalLayout_4->setObjectName(QStringLiteral("verticalLayout_4"));
        label_4 = new QLabel(wizardPage_2);
        label_4->setObjectName(QStringLiteral("label_4"));

        verticalLayout_4->addWidget(label_4);

        r_dir = new QLineEdit(wizardPage_2);
        r_dir->setObjectName(QStringLiteral("r_dir"));
        r_dir->setReadOnly(true);

        verticalLayout_4->addWidget(r_dir);

        label_5 = new QLabel(wizardPage_2);
        label_5->setObjectName(QStringLiteral("label_5"));

        verticalLayout_4->addWidget(label_5);

        r_inpx = new QLineEdit(wizardPage_2);
        r_inpx->setObjectName(QStringLiteral("r_inpx"));
        r_inpx->setReadOnly(true);

        verticalLayout_4->addWidget(r_inpx);

        label_6 = new QLabel(wizardPage_2);
        label_6->setObjectName(QStringLiteral("label_6"));

        verticalLayout_4->addWidget(label_6);

        r_name = new QLineEdit(wizardPage_2);
        r_name->setObjectName(QStringLiteral("r_name"));
        r_name->setReadOnly(true);

        verticalLayout_4->addWidget(r_name);

        update_lib = new QCheckBox(wizardPage_2);
        update_lib->setObjectName(QStringLiteral("update_lib"));
        update_lib->setChecked(true);

        verticalLayout_4->addWidget(update_lib);

        LibWizard->addPage(wizardPage_2);

        retranslateUi(LibWizard);

        QMetaObject::connectSlotsByName(LibWizard);
    } // setupUi

    void retranslateUi(QWizard *LibWizard)
    {
        LibWizard->setWindowTitle(QApplication::translate("LibWizard", "Library wizard", Q_NULLPTR));
        groupBox->setTitle(QApplication::translate("LibWizard", "Application mode", Q_NULLPTR));
        mode_lib->setText(QApplication::translate("LibWizard", "library mode", Q_NULLPTR));
        mode_convert->setText(QApplication::translate("LibWizard", "converter mode", Q_NULLPTR));
        label->setText(QApplication::translate("LibWizard", "Select book's directory:", Q_NULLPTR));
        label_2->setText(QApplication::translate("LibWizard", "Select INPX file (optionally):", Q_NULLPTR));
        label_3->setText(QApplication::translate("LibWizard", "Set library name:", Q_NULLPTR));
        label_4->setText(QApplication::translate("LibWizard", "Book's directory:", Q_NULLPTR));
        label_5->setText(QApplication::translate("LibWizard", "INPX file:", Q_NULLPTR));
        label_6->setText(QApplication::translate("LibWizard", "Library name:", Q_NULLPTR));
        update_lib->setText(QApplication::translate("LibWizard", "Update library", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class LibWizard: public Ui_LibWizard {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_LIBWIZARD_H
