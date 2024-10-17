#include "conversionframe.h"
#include "ui_conversionframe.h"

ConversionFrame::ConversionFrame(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::ConversionFrame)
{
    ui->setupUi(this);
    ui->btnDefaultCSS->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    connect(ui->AddFont, &QPushButton::clicked, this, [this](){this->AddFont();});
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &ConversionFrame::onTabWidgetCurrentChanged);
    connect(ui->addCoverLabel, &QCheckBox::clicked, this, &ConversionFrame::onAddCoverLabelClicked);
    connect(ui->createCaverAlways, &QCheckBox::clicked, this, &ConversionFrame::onCreateCaverAlwaysClicked);
    connect(ui->createCover, &QCheckBox::clicked, this, &ConversionFrame::onCreateCoverClicked);
    connect(ui->ml_toc, &QCheckBox::clicked, this, &ConversionFrame::onMlTocClicked);
    connect(ui->userCSS, &QCheckBox::clicked, this, &ConversionFrame::onUserCSSclicked);
    connect(ui->btnDefaultCSS, &QToolButton::clicked, this, &ConversionFrame::onBtnDefaultCSSclicked);
}

ConversionFrame::~ConversionFrame()
{
    delete ui;
}

void ConversionFrame::onCreateCaverAlwaysClicked()
{
    onAddCoverLabelClicked();
    ui->addCoverLabel->setEnabled(!ui->createCaverAlways->isChecked());
}

void ConversionFrame::onCreateCoverClicked()
{
    ui->createCaverAlways->setEnabled(ui->createCover->isChecked());
    onCreateCaverAlwaysClicked();
}

void ConversionFrame::onMlTocClicked()
{
    ui->MAXcaptionLevel->setEnabled(ui->ml_toc->isChecked());
    ui->label_lavel->setEnabled(ui->ml_toc->isChecked());
}

void ConversionFrame::onUserCSSclicked()
{
    set_userCSS_clicked();
    if(ui->UserCSStext->toPlainText().isEmpty())
        onBtnDefaultCSSclicked();

}

void ConversionFrame::onBtnDefaultCSSclicked()
{
    if(!ui->UserCSStext->toPlainText().isEmpty())
    {
        if(QMessageBox::question(this, tr("Load CSS"), tr("Are you sure you want to load default CSS?"), QMessageBox::Yes|QMessageBox::No, QMessageBox::NoButton) != QMessageBox::Yes)
            return;
    }
    QFile file(u":/xsl/css/style.css"_s);
    file.open(QFile::ReadOnly);
    QTextStream in(&file);
    ui->UserCSStext->setPlainText(in.readAll());
    file.close();

}

void ConversionFrame::onTabWidgetCurrentChanged(int index)
{
    emit ChangeTabIndex(index);
}

void ConversionFrame::set_userCSS_clicked()
{
    ui->btnDefaultCSS->setEnabled(ui->userCSS->isChecked());
    ui->UserCSStext->setEnabled(ui->userCSS->isChecked());
}

void ConversionFrame::Load(const ExportOptions *pExportOptions)
{
    ui->dropcaps->setChecked(pExportOptions->bDropCaps);
    ui->join_series->setChecked(pExportOptions->bJoinSeries);
    ui->hyphenate->setCurrentIndex(pExportOptions->nHyphenate);
    ui->Vignette->setCurrentIndex(pExportOptions->nVignette);
    ui->userCSS->setChecked(pExportOptions->bUserCSS);
    ui->UserCSStext->setPlainText(pExportOptions->sUserCSS);
    ui->split_file->setChecked(pExportOptions->bSplitFile);

    ui->break_after_cupture->setChecked(pExportOptions->bBreakAfterCupture);
    ui->annotation->setChecked(pExportOptions->bAnnotation);
    ui->footnotes->setCurrentIndex(pExportOptions->nFootNotes);
    ui->removePersonal->setChecked(pExportOptions->bRemovePersonal);
    ui->repairCover->setChecked(pExportOptions->bRepairCover);
    ui->ml_toc->setChecked(pExportOptions->bMlToc);
    ui->MAXcaptionLevel->setValue(pExportOptions->nMaxCaptionLevel);
    ui->seriaTranslit->setChecked(pExportOptions->bSeriaTranslit);
    ui->authorTranslit->setChecked(pExportOptions->bAuthorTranslit);
    ui->seriastring->setText(pExportOptions->sBookSeriesTitle);
    ui->authorstring->setText(pExportOptions->sAuthorSring);
    ui->createCover->setChecked(pExportOptions->bCreateCover);
    ui->createCaverAlways->setChecked(pExportOptions->bCreateCoverAlways);
    ui->addCoverLabel->setChecked(pExportOptions->bAddCoverLabel);
    ui->coverLabel->setText(pExportOptions->sCoverLabel);
    ui->content_placement->setCurrentIndex(pExportOptions->nContentPlacement);

    while(ui->fontLayout->count() > 2)
        delete ui->fontLayout->itemAt(0)->widget();
    for(const auto &fontExportOptions :pExportOptions->vFontExportOptions)
    {
        AddFont(fontExportOptions.bUse,
                fontExportOptions.nTag,
                fontExportOptions.sFont,
                fontExportOptions.sFontB,
                fontExportOptions.sFontI,
                fontExportOptions.sFontBI,
                fontExportOptions.nFontSize);
    }
    if(pExportOptions->vFontExportOptions.empty())
    {
        int size=100;
        int id_tag=0;

        for(const tag &i :g::vTags)
        {
            if(i.font_name== u"dropcaps_font"_s)
            {
                size=i.font_size;
                break;
            }
            id_tag++;
        }
        AddFont(true, id_tag, ExportOptions::sDefaultDropcapsFont, u""_s, u""_s, u""_s, size);
    }
    onMlTocClicked();
    set_userCSS_clicked();
}

QStringList ConversionFrame::Save(ExportOptions *pExportOptions)
{
    pExportOptions->bDropCaps = ui->dropcaps->isChecked();
    pExportOptions->bJoinSeries = ui->join_series->isChecked();
    pExportOptions->nHyphenate = ui->hyphenate->currentIndex();
    pExportOptions->nVignette = ui->Vignette->currentIndex();
    pExportOptions->bUserCSS = ui->userCSS->isChecked();
    pExportOptions->sUserCSS = ui->UserCSStext->toPlainText();
    pExportOptions->bSplitFile = ui->split_file->isChecked();
    pExportOptions->bBreakAfterCupture = ui->break_after_cupture->isChecked();
    pExportOptions->bAnnotation = ui->annotation->isChecked();
    pExportOptions->nFootNotes = ui->footnotes->currentIndex();

    pExportOptions->bRemovePersonal = ui->removePersonal->isChecked();
    pExportOptions->bRepairCover = ui->repairCover->isChecked();
    pExportOptions->bMlToc = ui->ml_toc->isChecked();
    pExportOptions->nMaxCaptionLevel = ui->MAXcaptionLevel->value();
    pExportOptions->bAuthorTranslit = ui->authorTranslit->isChecked();
    pExportOptions->bSeriaTranslit = ui->seriaTranslit->isChecked();
    pExportOptions->sBookSeriesTitle = ui->seriastring->text().trimmed();
    pExportOptions->sAuthorSring = ui->authorstring->text().trimmed();
    pExportOptions->bCreateCover = ui->createCover->isChecked();
    pExportOptions->bCreateCoverAlways = ui->createCaverAlways->isChecked();
    pExportOptions->bAddCoverLabel = ui->addCoverLabel->isChecked();
    pExportOptions->sCoverLabel = ui->coverLabel->text().trimmed();
    pExportOptions->nContentPlacement = ui->content_placement->currentIndex();

    QStringList fonts_list;
    int count = ui->fontLayout->count() - 2;
    pExportOptions->vFontExportOptions.resize(count);
    for (int i = 0; i < count; ++i)
    {
        FontExportOptions &fontExportOptions = pExportOptions->vFontExportOptions[i];
        FontFrame* pFontFrame = qobject_cast<FontFrame*>(ui->fontLayout->itemAt(i)->widget());
        fontExportOptions.bUse = pFontFrame->use();
        fontExportOptions.nTag = pFontFrame->tag();
        fontExportOptions.sFont = pFontFrame->font();
        fontExportOptions.sFontB = pFontFrame->font_b();
        fontExportOptions.sFontI = pFontFrame->font_i();
        fontExportOptions.sFontBI = pFontFrame->font_bi();
        fontExportOptions.nFontSize = pFontFrame->fontSize();

        fonts_list << fontExportOptions.sFont;
        fonts_list << fontExportOptions.sFontB;
        fonts_list << fontExportOptions.sFontI;
        fonts_list << fontExportOptions.sFontBI;
    }
    return fonts_list;

}

void ConversionFrame::setCurrentTab(int index)
{
    ui->tabWidget->setCurrentIndex(index);
}

void ConversionFrame::onAddCoverLabelClicked()
{
    ui->coverLabel->setEnabled(ui->addCoverLabel->isChecked() && !ui->createCaverAlways->isChecked());
    ui->label_tmplate->setEnabled(ui->addCoverLabel->isChecked() && !ui->createCaverAlways->isChecked());

}

void ConversionFrame::FontMove(QWidget *font_widget, int direction)
{
    int index = ui->fontLayout->indexOf(font_widget);
    if(index == 0 && direction>0)
        return;
    if(index == ui->fontLayout->count()-3 && direction<0)
        return;
    ui->fontLayout->removeWidget(font_widget);
    ui->fontLayout->insertWidget(index-direction, font_widget);

}

void ConversionFrame::RemoveFont(QWidget *font_widget)
{
    delete font_widget;

}

FontFrame *ConversionFrame::AddFont(bool use, int tag, const QString &font, const QString &font_b, const QString &font_i, const QString &font_bi, int fontSize)
{
    FontFrame* frame = new FontFrame(use, tag, font, font_b, font_i, font_bi, fontSize, this);
    ui->fontLayout->insertWidget(ui->fontLayout->count()-2, frame);
    connect(frame, &FontFrame::remove_font, this, &ConversionFrame::RemoveFont);
    connect(frame, &FontFrame::move_font, this, &ConversionFrame::FontMove);
    return frame;

}
