/********************************************************************************
** Form generated from reading UI file 'exportdlg.ui'
**
** Created by: Qt User Interface Compiler version 5.9.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_EXPORTDLG_H
#define UI_EXPORTDLG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_ExportDlg
{
public:
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QLabel *label;
    QLabel *Exporting;
    QProgressBar *progressBar;
    QCheckBox *CloseAfter;
    QHBoxLayout *horizontalLayout_2;
    QSpacerItem *horizontalSpacer;
    QPushButton *AbortButton;

    void setupUi(QDialog *ExportDlg)
    {
        if (ExportDlg->objectName().isEmpty())
            ExportDlg->setObjectName(QStringLiteral("ExportDlg"));
        ExportDlg->setWindowModality(Qt::WindowModal);
        ExportDlg->resize(601, 133);
        ExportDlg->setModal(true);
        verticalLayout = new QVBoxLayout(ExportDlg);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        verticalLayout->setSizeConstraint(QLayout::SetFixedSize);
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        label = new QLabel(ExportDlg);
        label->setObjectName(QStringLiteral("label"));

        horizontalLayout->addWidget(label);

        Exporting = new QLabel(ExportDlg);
        Exporting->setObjectName(QStringLiteral("Exporting"));

        horizontalLayout->addWidget(Exporting);

        horizontalLayout->setStretch(1, 10);

        verticalLayout->addLayout(horizontalLayout);

        progressBar = new QProgressBar(ExportDlg);
        progressBar->setObjectName(QStringLiteral("progressBar"));
        progressBar->setMinimumSize(QSize(400, 0));
        progressBar->setValue(24);

        verticalLayout->addWidget(progressBar);

        CloseAfter = new QCheckBox(ExportDlg);
        CloseAfter->setObjectName(QStringLiteral("CloseAfter"));

        verticalLayout->addWidget(CloseAfter);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer);

        AbortButton = new QPushButton(ExportDlg);
        AbortButton->setObjectName(QStringLiteral("AbortButton"));

        horizontalLayout_2->addWidget(AbortButton);


        verticalLayout->addLayout(horizontalLayout_2);


        retranslateUi(ExportDlg);

        QMetaObject::connectSlotsByName(ExportDlg);
    } // setupUi

    void retranslateUi(QDialog *ExportDlg)
    {
        ExportDlg->setWindowTitle(QApplication::translate("ExportDlg", "Export dialog", Q_NULLPTR));
        label->setText(QApplication::translate("ExportDlg", "Export:", Q_NULLPTR));
        Exporting->setText(QApplication::translate("ExportDlg", "TextLabel", Q_NULLPTR));
        CloseAfter->setText(QApplication::translate("ExportDlg", "Close after finish", Q_NULLPTR));
        AbortButton->setText(QApplication::translate("ExportDlg", "Abort", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class ExportDlg: public Ui_ExportDlg {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_EXPORTDLG_H
