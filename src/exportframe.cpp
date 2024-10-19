#include "exportframe.h"
#include "ui_exportframe.h"

#include <QToolButton>
#include <QStringBuilder>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>

#include "utilites.h"

ExportFrame::ExportFrame(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::ExportFrame)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(0);
    ui->OutputFormat->addItem(u"-"_s, ExportFormat::asis);
    ui->OutputFormat->addItem(u"EPUB"_s, ExportFormat::epub);
    ui->OutputFormat->addItem(u"AZW3"_s, ExportFormat::azw3);
    ui->OutputFormat->addItem(u"MOBI"_s, ExportFormat::mobi);
    ui->OutputFormat->addItem(u"MOBI7"_s, ExportFormat::mobi7);

    onOutputFormatChanged(0);
    onConnectionTypeChanged(0);
    connect(ui->radioDevice, &QRadioButton::toggled, this, &ExportFrame::onRadioDeviceToggled);
    connect(ui->radioEmail, &QRadioButton::toggled, this, &ExportFrame::onRadioEmailToggled);
    connect(ui->OutputFormat, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ExportFrame::onOutputFormatChanged);
    connect(ui->ConnectionType, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ExportFrame::onConnectionTypeChanged);
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    connect(ui->originalFileName, &QCheckBox::stateChanged, this, &ExportFrame::onOriginalFileNameChanged);
    connect(ui->PostprocessingCopy, &QCheckBox::stateChanged, this, &ExportFrame::onPostprocessingCopyChanged);
#ifdef USE_HTTSERVER
    connect(ui->checkBoxUseForHttp, &QCheckBox::stateChanged, this, &ExportFrame::onUseForHttpChanged);
#endif //USE_HTTSERVER
#else
    connect(ui->originalFileName, &QCheckBox::checkStateChanged, this, &ExportFrame::onOriginalFileNameChanged);
    connect(ui->PostprocessingCopy, &QCheckBox::checkStateChanged, this, &ExportFrame::onPostprocessingCopyChanged);
#ifdef USE_HTTSERVER
    connect(ui->checkBoxUseForHttp, &QCheckBox::checkStateChanged, this, &ExportFrame::onUseForHttpChanged);
#endif //USE_HTTSERVER
#endif //QT_VERSION < QT_VERSION_CHECK(6, 7, 0)

    QToolButton* btnPath = new QToolButton(this);
    btnPath->setFocusPolicy(Qt::NoFocus);
    btnPath->setCursor(Qt::ArrowCursor);
    btnPath->setText(u"..."_s);
    QHBoxLayout*  layout = new QHBoxLayout(ui->Path);
    layout->addWidget(btnPath, 0, Qt::AlignRight);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    connect(btnPath, &QAbstractButton::clicked, this, &ExportFrame::btnPath);

    QRegularExpression rx(u"\\b[^@\\s]+@[^@\\s]+\\.[^@\\s]+\\b"_s, QRegularExpression::CaseInsensitiveOption);
    validatorEMail.setRegularExpression(rx);

    ui->from_email->setValidator(&validatorEMail);
    ui->Email->setValidator(&validatorEMail);
    connect(ui->from_email, &QLineEdit::textChanged, this, [this](){validateEmail(this->ui->from_email);});
    connect(ui->Email, &QLineEdit::textChanged, this, [this](){validateEmail(this->ui->Email);});
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
    emit OutputFormatChanged();
}

void ExportFrame::onConnectionTypeChanged(int /*index*/)
{
    if(ui->ConnectionType->currentText().toLower() == u"tcp"_s)
        ui->Port->setText(u"25"_s);
    else
        ui->Port->setText(u"465"_s);
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
    auto count = ui->OutputFormat->count();
    for(int i=0; i<count; ++i){
        if(ui->OutputFormat->itemData(i) == pExportOptions->format){
            ui->OutputFormat->setCurrentIndex(i);
            break;
        }
    }
#ifdef USE_HTTSERVER
    ui->checkBoxUseForHttp->setChecked(pExportOptions->bUseForHttp);
#endif
    ui->askPath->setChecked(pExportOptions->bAskPath);
    ui->transliteration->setChecked(pExportOptions->bTransliteration);
    if(pExportOptions->sSendTo == u"device"_s)
        ui->radioDevice->setChecked(true);
    else
        ui->radioEmail->setChecked(true);

    UpdateToolComboBox(pExportOptions->sCurrentTool);
    connect(ui->ConnectionType, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ExportFrame::onConnectionTypeChanged);
}

ExportFormat ExportFrame::outputFormat()
{
    return ui->OutputFormat->currentData().value<ExportFormat>();
}

bool ExportFrame::getUseForHttp()
{
    return ui->checkBoxUseForHttp->isChecked();
}

void ExportFrame::setUseForHttp(bool bUse)
{
    ui->checkBoxUseForHttp->setChecked(bUse);
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
    pExportOptions->sSendTo = ui->radioDevice->isChecked() ?u"device"_s :u"e-mail"_s;
    pExportOptions->sCurrentTool = ui->CurrentTools->currentText();
    pExportOptions->bAskPath = ui->askPath->isChecked();
    pExportOptions->bOriginalFileName = ui->originalFileName->isChecked();
    pExportOptions->sExportFileName = ui->ExportFileName->text().trimmed();
    pExportOptions->format = ui->OutputFormat->currentData().value<ExportFormat>();
#ifdef USE_HTTSERVER
    pExportOptions->bUseForHttp = ui->checkBoxUseForHttp->isChecked();
#endif

    pExportOptions->bTransliteration = ui->transliteration->isChecked();

    QStringList fonts_list;
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
    int index=0;
    for(const auto &iTool :g::options.tools){
        ui->CurrentTools->addItem(iTool.first);
        if(iTool.first == CurrentTool)
        {
            ui->CurrentTools->setCurrentIndex(ui->CurrentTools->count()-1);
        }
        ++index;
    }
}

void ExportFrame::onUseForHttpChanged(int state)
{
    emit UseForHttpChanged(state);
}

void ExportFrame::btnPath()
{
    QDir::setCurrent(ui->Path->text());
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select device directory"));
    if(!dir.isEmpty())
        ui->Path->setText(dir);
}

void ExportFrame::onOriginalFileNameChanged(int state)
{
    bool bUnchecked = (state == Qt::Unchecked);
    ui->ExportFileName->setEnabled(bUnchecked);
    ui->transliteration->setEnabled(bUnchecked);
    ui->label_exportname->setEnabled(bUnchecked);
}

void ExportFrame::onPostprocessingCopyChanged(int state)
{
    bool bUnchecked = (state == Qt::Unchecked);
    ui->askPath->setEnabled(bUnchecked);
    ui->Path->setEnabled(bUnchecked);
    ui->ExportFileName->setEnabled(bUnchecked);
    ui->transliteration->setEnabled(bUnchecked);
    ui->label_path->setEnabled(bUnchecked);
    ui->label_exportname->setEnabled(bUnchecked);
    ui->originalFileName->setEnabled(bUnchecked);
}

void ExportFrame::validateEmail(QLineEdit *leEmail)
{
    if(!leEmail->hasAcceptableInput())
        leEmail->setStyleSheet(u"QLineEdit { color: red;}"_s);
    else{
        auto sColor = palette().color(QPalette::WindowText).name();
        leEmail->setStyleSheet(u"QLineEdit { color: %1;}"_s.arg(sColor));
    }
}
