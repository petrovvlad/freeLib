/********************************************************************************
** Form generated from reading UI file 'fontframe.ui'
**
** Created by: Qt User Interface Compiler version 5.9.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_FONTFRAME_H
#define UI_FONTFRAME_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QToolButton>

QT_BEGIN_NAMESPACE

class Ui_FontFrame
{
public:
    QHBoxLayout *horizontalLayout;
    QCheckBox *Use;
    QComboBox *tag;
    QHBoxLayout *horizontalLayout_3;
    QLabel *label_2;
    QSpinBox *fontSize;
    QLabel *label;
    QFrame *frame;
    QGridLayout *gridLayout_2;
    QLabel *label_6;
    QLabel *label_5;
    QComboBox *font_i;
    QComboBox *font;
    QLabel *label_3;
    QLabel *label_4;
    QComboBox *font_b;
    QComboBox *font_bi;
    QHBoxLayout *horizontalLayout_2;
    QToolButton *Up;
    QToolButton *Down;
    QToolButton *del;

    void setupUi(QFrame *FontFrame)
    {
        if (FontFrame->objectName().isEmpty())
            FontFrame->setObjectName(QStringLiteral("FontFrame"));
        FontFrame->resize(620, 83);
        FontFrame->setWindowTitle(QStringLiteral(""));
        FontFrame->setFrameShape(QFrame::StyledPanel);
        FontFrame->setFrameShadow(QFrame::Raised);
        horizontalLayout = new QHBoxLayout(FontFrame);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalLayout->setContentsMargins(4, 0, 4, 0);
        Use = new QCheckBox(FontFrame);
        Use->setObjectName(QStringLiteral("Use"));

        horizontalLayout->addWidget(Use);

        tag = new QComboBox(FontFrame);
        tag->setObjectName(QStringLiteral("tag"));
        QSizePolicy sizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(tag->sizePolicy().hasHeightForWidth());
        tag->setSizePolicy(sizePolicy);
        tag->setSizeAdjustPolicy(QComboBox::AdjustToContents);

        horizontalLayout->addWidget(tag);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setSpacing(1);
        horizontalLayout_3->setObjectName(QStringLiteral("horizontalLayout_3"));
        label_2 = new QLabel(FontFrame);
        label_2->setObjectName(QStringLiteral("label_2"));

        horizontalLayout_3->addWidget(label_2);

        fontSize = new QSpinBox(FontFrame);
        fontSize->setObjectName(QStringLiteral("fontSize"));
        fontSize->setMinimum(20);
        fontSize->setMaximum(1000);
        fontSize->setSingleStep(10);
        fontSize->setValue(100);

        horizontalLayout_3->addWidget(fontSize);

        label = new QLabel(FontFrame);
        label->setObjectName(QStringLiteral("label"));
        label->setText(QStringLiteral("%"));

        horizontalLayout_3->addWidget(label);


        horizontalLayout->addLayout(horizontalLayout_3);

        frame = new QFrame(FontFrame);
        frame->setObjectName(QStringLiteral("frame"));
        frame->setFrameShape(QFrame::StyledPanel);
        gridLayout_2 = new QGridLayout(frame);
        gridLayout_2->setSpacing(4);
        gridLayout_2->setObjectName(QStringLiteral("gridLayout_2"));
        gridLayout_2->setContentsMargins(4, 4, 4, 4);
        label_6 = new QLabel(frame);
        label_6->setObjectName(QStringLiteral("label_6"));

        gridLayout_2->addWidget(label_6, 2, 0, 1, 1);

        label_5 = new QLabel(frame);
        label_5->setObjectName(QStringLiteral("label_5"));

        gridLayout_2->addWidget(label_5, 1, 0, 1, 1);

        font_i = new QComboBox(frame);
        font_i->setObjectName(QStringLiteral("font_i"));

        gridLayout_2->addWidget(font_i, 2, 1, 1, 1);

        font = new QComboBox(frame);
        font->setObjectName(QStringLiteral("font"));

        gridLayout_2->addWidget(font, 1, 1, 1, 1);

        label_3 = new QLabel(frame);
        label_3->setObjectName(QStringLiteral("label_3"));
        label_3->setAlignment(Qt::AlignCenter);

        gridLayout_2->addWidget(label_3, 0, 1, 1, 1);

        label_4 = new QLabel(frame);
        label_4->setObjectName(QStringLiteral("label_4"));
        label_4->setAlignment(Qt::AlignCenter);

        gridLayout_2->addWidget(label_4, 0, 2, 1, 1);

        font_b = new QComboBox(frame);
        font_b->setObjectName(QStringLiteral("font_b"));

        gridLayout_2->addWidget(font_b, 1, 2, 1, 1);

        font_bi = new QComboBox(frame);
        font_bi->setObjectName(QStringLiteral("font_bi"));

        gridLayout_2->addWidget(font_bi, 2, 2, 1, 1);

        gridLayout_2->setColumnStretch(1, 1);
        gridLayout_2->setColumnStretch(2, 1);
        label_3->raise();
        label_6->raise();
        font->raise();
        font_i->raise();
        label_5->raise();
        label_4->raise();
        font_b->raise();
        font_bi->raise();

        horizontalLayout->addWidget(frame);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setSpacing(1);
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        Up = new QToolButton(FontFrame);
        Up->setObjectName(QStringLiteral("Up"));
        Up->setMaximumSize(QSize(22, 22));
        Up->setText(QStringLiteral("..."));
        QIcon icon;
        icon.addFile(QStringLiteral(":/icons/img/icons/arrow_sans_up.png"), QSize(), QIcon::Normal, QIcon::Off);
        Up->setIcon(icon);

        horizontalLayout_2->addWidget(Up);

        Down = new QToolButton(FontFrame);
        Down->setObjectName(QStringLiteral("Down"));
        Down->setMaximumSize(QSize(22, 22));
        Down->setText(QStringLiteral("..."));
        QIcon icon1;
        icon1.addFile(QStringLiteral(":/icons/img/icons/arrow_sans_down.png"), QSize(), QIcon::Normal, QIcon::Off);
        Down->setIcon(icon1);

        horizontalLayout_2->addWidget(Down);

        del = new QToolButton(FontFrame);
        del->setObjectName(QStringLiteral("del"));
        del->setMaximumSize(QSize(22, 22));
        del->setText(QStringLiteral("..."));
        QIcon icon2;
        icon2.addFile(QStringLiteral(":/icons/img/icons/close.png"), QSize(), QIcon::Normal, QIcon::Off);
        del->setIcon(icon2);

        horizontalLayout_2->addWidget(del);


        horizontalLayout->addLayout(horizontalLayout_2);

        horizontalLayout->setStretch(1, 1);

        retranslateUi(FontFrame);

        QMetaObject::connectSlotsByName(FontFrame);
    } // setupUi

    void retranslateUi(QFrame *FontFrame)
    {
        Use->setText(QString());
        label_2->setText(QApplication::translate("FontFrame", "size", Q_NULLPTR));
        label_6->setText(QApplication::translate("FontFrame", "Italic", Q_NULLPTR));
        label_5->setText(QApplication::translate("FontFrame", "Normal", Q_NULLPTR));
        label_3->setText(QApplication::translate("FontFrame", "Regular", Q_NULLPTR));
        label_4->setText(QApplication::translate("FontFrame", "Bold", Q_NULLPTR));
        Q_UNUSED(FontFrame);
    } // retranslateUi

};

namespace Ui {
    class FontFrame: public Ui_FontFrame {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_FONTFRAME_H
