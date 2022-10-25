#define QT_USE_QSTRINGBUILDER
#include "exportframe.h"
#include "ui_exportframe.h"

#include <QToolButton>
#include <QStringBuilder>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>

ExportFrame::ExportFrame(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::ExportFrame)
{
    ui->setupUi(this);
    ui->btnDefaultCSS->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    ui->stackedWidget->setCurrentIndex(0);
    ui->tabWidget->setCurrentIndex(0);
    ui->toolBox->setCurrentIndex(0);
    onOutputFormatChanged(0);
    onConnectionTypeChanged(0);
    connect(ui->AddFont, &QPushButton::clicked, this, [=](){this->AddFont();});
    connect(ui->radioDevice, &QRadioButton::toggled, this, &ExportFrame::onRadioDeviceToggled);
    connect(ui->radioEmail, &QRadioButton::toggled, this, &ExportFrame::onRadioEmailToggled);
    connect(ui->OutputFormat, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ExportFrame::onOutputFormatChanged);
    connect(ui->ConnectionType, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ExportFrame::onConnectionTypeChanged);
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &ExportFrame::onTabWidgetCurrentChanged);
    connect(ui->addCoverLabel, &QCheckBox::clicked, this, &ExportFrame::onAddCoverLabelClicked);
    connect(ui->createCaverAlways, &QCheckBox::clicked, this, &ExportFrame::onCreateCaverAlwaysClicked);
    connect(ui->createCover, &QCheckBox::clicked, this, &ExportFrame::onCreateCoverClicked);
    connect(ui->originalFileName, &QCheckBox::clicked, this, &ExportFrame::onOriginalFileNameClicked);
    connect(ui->ml_toc, &QCheckBox::clicked, this, &ExportFrame::onMlTocClicked);
    connect(ui->PostprocessingCopy, &QCheckBox::clicked, this, &ExportFrame::onPostprocessingCopyClicked);
    connect(ui->userCSS, &QCheckBox::clicked, this, &ExportFrame::onUserCSSclicked);
    connect(ui->btnDefaultCSS, &QToolButton::clicked, this, &ExportFrame::onBtnDefaultCSSclicked);

    QToolButton* btnPath = new QToolButton(this);
    btnPath->setFocusPolicy(Qt::NoFocus);
    btnPath->setCursor(Qt::ArrowCursor);
    btnPath->setText(QStringLiteral("..."));
    QHBoxLayout*  layout = new QHBoxLayout(ui->Path);
    layout->addWidget(btnPath, 0, Qt::AlignRight);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    connect(btnPath, &QAbstractButton::clicked, this, &ExportFrame::btnPath);
    connect(ui->toolBox, &QToolBox::currentChanged, this, &ExportFrame::onTabWidgetCurrentChanged);
}

ExportFrame::~ExportFrame()
{
    delete ui;
}


void ExportFrame::onRadioDeviceToggled(bool checked)
{
    if(checked)
        ui->stackedWidget->setCurrentIndex(0);
}

void ExportFrame::onRadioEmailToggled(bool checked)
{
    if(checked)
        ui->stackedWidget->setCurrentIndex(1);
}

void ExportFrame::onOutputFormatChanged(int /*index*/)
{
    ui->tabFormat->setDisabled(ui->OutputFormat->currentText() == QStringLiteral("-"));
}

void ExportFrame::onConnectionTypeChanged(int /*index*/)
{
    if(ui->ConnectionType->currentText().toLower() == QStringLiteral("tcp"))
        ui->Port->setText(QStringLiteral("25"));
    else
        ui->Port->setText(QStringLiteral("465"));
}

void ExportFrame::Load(const ExportOptions *pExportOptions)
{
    disconnect(ui->ConnectionType, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ExportFrame::onConnectionTypeChanged);
    ui->Email->setText(pExportOptions->sEmail);
    ui->from_email->setText(pExportOptions->sEmailFrom);
    ui->mail_subject->setText(pExportOptions->sEmailSubject);
    ui->Server->setText(pExportOptions->sEmailServer);
    ui->Port->setText(QString::number(pExportOptions->nEmailServerPort));
    ui->User->setText(pExportOptions->sEmailUser);
    ui->Password->setText(pExportOptions->sEmailPassword);
    ui->PostprocessingCopy->setChecked(pExportOptions->bPostprocessingCopy);
    ui->Path->setText(pExportOptions->sDevicePath);
    ui->originalFileName->setChecked(pExportOptions->bOriginalFileName);
    ui->ExportFileName->setText(pExportOptions->sExportFileName);
    ui->PauseMail->setValue(pExportOptions->nEmailPause);
    ui->ConnectionType->setCurrentIndex(pExportOptions->nEmailConnectionType);
    ui->dropcaps->setChecked(pExportOptions->bDropCaps);
    ui->join_series->setChecked(pExportOptions->bJoinSeries);
    ui->hyphenate->setCurrentIndex(pExportOptions->nHyphenate);
    ui->Vignette->setCurrentIndex(pExportOptions->nVignette);
    ui->userCSS->setChecked(pExportOptions->bUserCSS);
    ui->UserCSStext->setPlainText(pExportOptions->sUserCSS);
    ui->split_file->setChecked(pExportOptions->bSplitFile);
    ui->OutputFormat->setCurrentText(pExportOptions->sOutputFormat);
    ui->break_after_cupture->setChecked(pExportOptions->bBreakAfterCupture);
    ui->annotation->setChecked(pExportOptions->bAnnotation);
    ui->footnotes->setCurrentIndex(pExportOptions->nFootNotes);
    ui->askPath->setChecked(pExportOptions->bAskPath);
    ui->transliteration->setChecked(pExportOptions->bTransliteration);
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
    if(pExportOptions->sSendTo == QStringLiteral("device"))
        ui->radioDevice->setChecked(true);
    else
        ui->radioEmail->setChecked(true);

    ui->content_placement->setCurrentIndex(pExportOptions->nContentPlacement);

    UpdateToolComboBox(pExportOptions->sCurrentTool);


    while(ui->fontLayout->count() > 2)
        delete ui->fontLayout->itemAt(0)->widget();
    int count = pExportOptions->vFontExportOptions.count();
    for(int i=0; i<count; i++)
    {
        AddFont(pExportOptions->vFontExportOptions.at(i).bUse,
                pExportOptions->vFontExportOptions.at(i).nTag,
                pExportOptions->vFontExportOptions.at(i).sFont,
                pExportOptions->vFontExportOptions.at(i).sFontB,
                pExportOptions->vFontExportOptions.at(i).sFontI,
                pExportOptions->vFontExportOptions.at(i).sFontBI,
                pExportOptions->vFontExportOptions.at(i).nFontSize);
    }

    onOriginalFileNameClicked();
    onPostprocessingCopyClicked();
    onMlTocClicked();
    set_userCSS_clicked();
    connect(ui->ConnectionType, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ExportFrame::onConnectionTypeChanged);
}

QStringList ExportFrame::Save(ExportOptions *pExportOptions)
{
    pExportOptions->sEmailFrom = ui->from_email->text().trimmed();
    pExportOptions->sEmail = ui->Email->text().trimmed();
    pExportOptions->sEmailSubject = ui->mail_subject->text().trimmed();
    pExportOptions->sEmailServer = ui->Server->text().trimmed();
    pExportOptions->nEmailServerPort = ui->Port->text().trimmed().toUInt();
    pExportOptions->sEmailUser = ui->User->text().trimmed();
    pExportOptions->sEmailPassword = ui->Password->text();
    pExportOptions->bPostprocessingCopy = ui->PostprocessingCopy->isChecked();
    pExportOptions->sDevicePath = ui->Path->text().trimmed();
    pExportOptions->nEmailPause = ui->PauseMail->value();
    pExportOptions->nEmailConnectionType = ui->ConnectionType->currentIndex();
    pExportOptions->sSendTo = ui->radioDevice->isChecked() ?QStringLiteral("device") :QStringLiteral("e-mail");
    pExportOptions->sCurrentTool = ui->CurrentTools->currentText();
    pExportOptions->bAskPath = ui->askPath->isChecked();
    pExportOptions->bOriginalFileName = ui->originalFileName->isChecked();
    pExportOptions->sExportFileName = ui->ExportFileName->text().trimmed();
    pExportOptions->sOutputFormat = ui->OutputFormat->currentText();
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
    pExportOptions->bTransliteration = ui->transliteration->isChecked();
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

void ExportFrame::UpdateToolComboBox(const QString &sCurrentTool)
{
    QString CurrentTool;
    if(sCurrentTool.isEmpty())
        CurrentTool = ui->CurrentTools->currentText();
    else
        CurrentTool = sCurrentTool;
    while(ui->CurrentTools->count()>1)
        ui->CurrentTools->removeItem(1);
    auto iTool = options.tools.constBegin();
    int index=0;
    while(iTool != options.tools.constEnd()){
        ui->CurrentTools->addItem(iTool.key());
        if(iTool.key() == CurrentTool)
        {
            ui->CurrentTools->setCurrentIndex(ui->CurrentTools->count()-1);
        }
        ++index;
        ++iTool;
    }
}

FontFrame* ExportFrame::AddFont(bool use, int tag, const QString &font, const QString &font_b, const QString &font_i, const QString &font_bi, int fontSize)
{
    FontFrame* frame = new FontFrame(use, tag, font, font_b, font_i, font_bi, fontSize, this);
    ui->fontLayout->insertWidget(ui->fontLayout->count()-2, frame);
    connect(frame, &FontFrame::remove_font, this, &ExportFrame::RemoveFont);
    connect(frame, &FontFrame::move_font, this, &ExportFrame::FontMove);
    return frame;
}

void ExportFrame::RemoveFont(QWidget *font_widget)
{
    delete font_widget;
}

void ExportFrame::FontMove(QWidget *font_widget, int direction)
{
    int index = ui->fontLayout->indexOf(font_widget);
    if(index == 0 && direction>0)
        return;
    if(index == ui->fontLayout->count()-3 && direction<0)
        return;
    ui->fontLayout->removeWidget(font_widget);
    ui->fontLayout->insertWidget(index-direction, font_widget);
}

void ExportFrame::btnPath()
{
    QDir::setCurrent(ui->Path->text());
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select device directory"));
    if(!dir.isEmpty())
        ui->Path->setText(dir);
}

void ExportFrame::SetTabIndex(int tab_id, int page_id)
{
    ui->tabWidget->setCurrentIndex(tab_id);
    ui->toolBox->setCurrentIndex(page_id);
}


void ExportFrame::onTabWidgetCurrentChanged(int )
{
    emit ChangeTabIndex(ui->tabWidget->currentIndex(), ui->toolBox->currentIndex());
}

void ExportFrame::onAddCoverLabelClicked()
{
    ui->coverLabel->setEnabled(ui->addCoverLabel->isChecked() && !ui->createCaverAlways->isChecked());
    ui->label_tmplate->setEnabled(ui->addCoverLabel->isChecked() && !ui->createCaverAlways->isChecked());
}

void ExportFrame::onCreateCaverAlwaysClicked()
{
    onAddCoverLabelClicked();
    ui->addCoverLabel->setEnabled(!ui->createCaverAlways->isChecked());
}

void ExportFrame::onCreateCoverClicked()
{
    ui->createCaverAlways->setEnabled(ui->createCover->isChecked());
    onCreateCaverAlwaysClicked();
}

void ExportFrame::onOriginalFileNameClicked()
{
    ui->ExportFileName->setEnabled(!ui->originalFileName->isChecked());
    ui->transliteration->setEnabled(!ui->originalFileName->isChecked());
    ui->label_exportname->setEnabled(!ui->originalFileName->isChecked());
}

void ExportFrame::onMlTocClicked()
{
    ui->MAXcaptionLevel->setEnabled(ui->ml_toc->isChecked());
    ui->label_lavel->setEnabled(ui->ml_toc->isChecked());
}

void ExportFrame::onPostprocessingCopyClicked()
{
    ui->askPath->setEnabled(!ui->PostprocessingCopy->isChecked());
    ui->Path->setEnabled(!ui->PostprocessingCopy->isChecked());
    ui->ExportFileName->setEnabled(!ui->PostprocessingCopy->isChecked());
    ui->transliteration->setEnabled(!ui->PostprocessingCopy->isChecked());
    ui->label_exportname->setEnabled(!ui->PostprocessingCopy->isChecked());
    ui->originalFileName->setEnabled(!ui->PostprocessingCopy->isChecked());
}

void ExportFrame::onUserCSSclicked()
{
    set_userCSS_clicked();
    if(ui->UserCSStext->toPlainText().isEmpty())
        onBtnDefaultCSSclicked();
}

void ExportFrame::set_userCSS_clicked()
{
    ui->btnDefaultCSS->setEnabled(ui->userCSS->isChecked());
    ui->UserCSStext->setEnabled(ui->userCSS->isChecked());
}

void ExportFrame::onBtnDefaultCSSclicked()
{
    if(!ui->UserCSStext->toPlainText().isEmpty())
    {
        if(QMessageBox::question(this, tr("Load CSS"), tr("Are you sure you want to load default CSS?"), QMessageBox::Yes|QMessageBox::No, QMessageBox::NoButton) != QMessageBox::Yes)
            return;
    }
    QFile file(QStringLiteral(":/xsl/css/style.css"));
    file.open(QFile::ReadOnly);
    QTextStream in(&file);
    ui->UserCSStext->setPlainText(in.readAll());
    file.close();
}
