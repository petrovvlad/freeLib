#include "exportframe.h"
#include "ui_exportframe.h"
#include "QStandardPaths"
#include "common.h"
#include <QToolButton>

quint8 key[] = {1,65,245,245,235,2,34,61,0,32,54,12,66};
QString encodeStr(const QString& str)
{
    QByteArray arr(str.toUtf8());
    quint32 index=0;
    for(int i =0; i<arr.size(); i++)
    {
        arr[i] = arr[i] ^ key[index];
        index++;
        if(index>=sizeof(key)/sizeof(quint32))
            index=0;
    }

    return "#-#"+QString::fromLatin1(arr.toBase64());
}

QString decodeStr(const QString &str)
{
    if(str.left(3)!="#-#")
        return str;
    QByteArray arr = QByteArray::fromBase64(str.mid(3).toLatin1());
    quint32 index=0;
    for(int i =0; i<arr.size(); i++)
    {
        arr[i] =arr[i] ^ key[index];
        index++;
        if(index>=sizeof(key)/sizeof(quint32))
            index=0;
    }
    return QString::fromUtf8(arr);
}

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

    QToolButton* btnPath=new QToolButton(this);
    btnPath->setFocusPolicy(Qt::NoFocus);
    btnPath->setCursor(Qt::ArrowCursor);
    btnPath->setText("...");
    QHBoxLayout*  layout=new QHBoxLayout(ui->Path);
    layout->addWidget(btnPath,0,Qt::AlignRight);
    layout->setSpacing(0);
    layout->setMargin(0);
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
    ui->tabFormat->setDisabled(ui->OutputFormat->currentText()==QStringLiteral("-"));
}

void ExportFrame::onConnectionTypeChanged(int /*index*/)
{
    if(ui->ConnectionType->currentText().toLower()==QStringLiteral("tcp"))
        ui->Port->setText(QStringLiteral("25"));
    else
        ui->Port->setText(QStringLiteral("465"));
}

void ExportFrame::Load(QSettings *_settings)
{
    disconnect(ui->ConnectionType, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ExportFrame::onConnectionTypeChanged);
    QSettings *settings=_settings;
    if(_settings==0)
        settings=new QSettings();
    ui->Email->setText(settings->value("Email").toString());
    ui->from_email->setText(settings->value("from_email").toString());
    ui->mail_subject->setText(settings->value("mail_subject").toString());
    ui->Server->setText(settings->value("EmailServer").toString());
    ui->Port->setText(settings->value("EmailPort","25").toString());
    ui->User->setText(settings->value("EmailUser").toString());
    ui->Password->setText(decodeStr(settings->value("EmailPassword").toString()));
    QString HomeDir="";
    if(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).count()>0)
        HomeDir=QStandardPaths::standardLocations(QStandardPaths::HomeLocation).at(0);
    ui->PostprocessingCopy->setChecked(settings->value("PostprocessingCopy").toBool());
    ui->Path->setText(settings->value("DevicePath",HomeDir).toString());
    ui->originalFileName->setChecked(settings->value("originalFileName",false).toBool());
    ui->ExportFileName->setText(settings->value("ExportFileName",default_exp_file_name).toString());
    ui->PauseMail->setValue(settings->value("PauseMail",5).toInt());
    ui->ConnectionType->setCurrentIndex(settings->value("ConnectionType",0).toInt());
    ui->dropcaps->setChecked(settings->value("dropcaps").toBool());
    ui->join_series->setChecked(settings->value("join_series").toBool());
    ui->hyphenate->setCurrentIndex(settings->value("hyphenate").toInt());
    ui->Vignette->setCurrentIndex(settings->value("Vignette").toInt());
    ui->userCSS->setChecked(settings->value("userCSS",false).toBool());
    ui->UserCSStext->setPlainText(settings->value("UserCSStext","").toString());
    ui->split_file->setChecked(settings->value("split_file",true).toBool());
    ui->OutputFormat->setCurrentText(settings->value("OutputFormat").toString());
    ui->break_after_cupture->setChecked(settings->value("break_after_cupture",true).toBool());
    ui->annotation->setChecked(settings->value("annotation",false).toBool());
    ui->footnotes->setCurrentIndex(settings->value("footnotes",0).toInt());
    ui->askPath->setChecked(settings->value("askPath",true).toBool());
    ui->transliteration->setChecked(settings->value("transliteration",false).toBool());
    ui->removePersonal->setChecked(settings->value("removePersonal",false).toBool());
    ui->repairCover->setChecked(settings->value("repairCover",true).toBool());
    ui->ml_toc->setChecked(settings->value("ml_toc",true).toBool());
    ui->MAXcaptionLevel->setValue(settings->value("MAXcaptionLevel",2).toInt());
    ui->seriaTranslit->setChecked(settings->value("seriaTranslit",false).toBool());
    ui->authorTranslit->setChecked(settings->value("authorTranslit",false).toBool());

    ui->seriastring->setText(settings->value("bookseriestitle").toString());
    ui->authorstring->setText(settings->value("authorstring").toString());
    ui->createCover->setChecked(settings->value("createCover",false).toBool());
    ui->createCaverAlways->setChecked(settings->value("createCaverAlways",false).toBool());
    ui->addCoverLabel->setChecked(settings->value("addCoverLabel",false).toBool());
    ui->coverLabel->setText(settings->value("coverLabel").toString());
    if(settings->value("sendTo","device").toString()=="device")
        ui->radioDevice->setChecked(true);
    else
        ui->radioEmail->setChecked(true);

    ui->content_placement->setCurrentIndex(settings->value("content_placement",0).toInt());

    UpdateToolComboBox(settings);


    while(ui->fontLayout->count()>2)
        delete ui->fontLayout->itemAt(0)->widget();
    if(_settings==0)
    {
        int size=100;
        int id_tag=0;

        foreach(tag i,tag_list)
        {
            if(i.font_name=="dropcaps_font")
            {
                size=i.font_size;
                break;
            }
            id_tag++;
        }
        AddFont(true,id_tag,default_dropcaps_font,"","","",size);
    }
    else
    {
        int count=settings->beginReadArray("fonts");
        for(int i=0;i<count;i++)
        {
            //qDebug()<<settings->value("font").toString();
            settings->setArrayIndex(i);
            AddFont(settings->value("use").toBool(),
                    settings->value("tag").toInt(),
                    settings->value("font").toString(),
                    settings->value("font_b").toString(),
                    settings->value("font_i").toString(),
                    settings->value("font_bi").toString(),
                    settings->value("fontSize",100).toInt());
        }
        settings->endArray();
    }

    onOriginalFileNameClicked();
    onPostprocessingCopyClicked();
    onMlTocClicked();
    set_userCSS_clicked();
    connect(ui->ConnectionType, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ExportFrame::onConnectionTypeChanged);
    if(_settings==0)
        delete settings;
}


QStringList ExportFrame::Save(QSettings *settings,bool save_passwords)
{
    if(save_passwords)
    {
        settings->setValue("Email",ui->Email->text().trimmed());
        settings->setValue("from_email",ui->from_email->text().trimmed());
        settings->setValue("mail_subject",ui->mail_subject->text().trimmed());
        settings->setValue("EmailServer",ui->Server->text().trimmed());
        settings->setValue("EmailPort",ui->Port->text().trimmed());
        settings->setValue("EmailUser",ui->User->text().trimmed());
        settings->setValue("EmailPassword",encodeStr(ui->Password->text()));
        settings->setValue("PostprocessingCopy",ui->PostprocessingCopy->checkState()==Qt::Checked);
        settings->setValue("DevicePath",ui->Path->text().trimmed());
        settings->setValue("PauseMail",ui->PauseMail->value());
        settings->setValue("ConnectionType",ui->ConnectionType->currentIndex());
        settings->setValue("sendTo",(ui->radioDevice->isChecked()?"device":"e-mail"));
        settings->setValue("current_tool",ui->CurrentTools->currentText());
    }
    settings->setValue("askPath",ui->askPath->checkState()==Qt::Checked);
    settings->setValue("originalFileName",ui->originalFileName->checkState()==Qt::Checked);
    settings->setValue("ExportFileName",ui->ExportFileName->text().trimmed());
    settings->setValue("OutputFormat",ui->OutputFormat->currentText());
    settings->setValue("dropcaps",ui->dropcaps->checkState()==Qt::Checked);
    settings->setValue("join_series",ui->join_series->checkState()==Qt::Checked);
    settings->setValue("hyphenate",ui->hyphenate->currentIndex());
    settings->setValue("Vignette",ui->Vignette->currentIndex());
    settings->setValue("userCSS",ui->userCSS->checkState()==Qt::Checked);
    settings->setValue("UserCSStext",ui->UserCSStext->toPlainText());
    settings->setValue("split_file",ui->split_file->checkState()==Qt::Checked);
    settings->setValue("break_after_cupture",ui->break_after_cupture->checkState()==Qt::Checked);
    settings->setValue("annotation",ui->annotation->checkState()==Qt::Checked);
    settings->setValue("footnotes",ui->footnotes->currentIndex());
    settings->setValue("transliteration",ui->transliteration->checkState()==Qt::Checked);
    settings->setValue("removePersonal",ui->removePersonal->checkState()==Qt::Checked);
    settings->setValue("repairCover",ui->repairCover->checkState()==Qt::Checked);
    settings->setValue("ml_toc",ui->ml_toc->checkState()==Qt::Checked);
    settings->setValue("MAXcaptionLevel",ui->MAXcaptionLevel->value());
    settings->setValue("authorTranslit",ui->authorTranslit->checkState()==Qt::Checked);
    settings->setValue("seriaTranslit",ui->seriaTranslit->checkState()==Qt::Checked);
    settings->setValue("bookseriestitle",ui->seriastring->text().trimmed());
    settings->setValue("authorstring",ui->authorstring->text().trimmed());
    settings->setValue("createCover",ui->createCover->checkState()==Qt::Checked);
    settings->setValue("createCaverAlways",ui->createCaverAlways->checkState()==Qt::Checked);
    settings->setValue("addCoverLabel",ui->addCoverLabel->checkState()==Qt::Checked);
    settings->setValue("coverLabel",ui->coverLabel->text().trimmed());
    settings->setValue("content_placement",ui->content_placement->currentIndex());

    QStringList fonts_list;
    settings->beginWriteArray("fonts");
    for (int i = 0; i < ui->fontLayout->count()-2; ++i)
    {
        settings->setArrayIndex(i);
        settings->setValue("use",((FontFrame*)ui->fontLayout->itemAt(i)->widget())->use());
        settings->setValue("tag",((FontFrame*)ui->fontLayout->itemAt(i)->widget())->tag());
        fonts_list<<((FontFrame*)ui->fontLayout->itemAt(i)->widget())->font();
        fonts_list<<((FontFrame*)ui->fontLayout->itemAt(i)->widget())->font_b();
        fonts_list<<((FontFrame*)ui->fontLayout->itemAt(i)->widget())->font_i();
        fonts_list<<((FontFrame*)ui->fontLayout->itemAt(i)->widget())->font_bi();
        if(save_passwords)
        {
            settings->setValue("font",((FontFrame*)ui->fontLayout->itemAt(i)->widget())->font());
            settings->setValue("font_b",((FontFrame*)ui->fontLayout->itemAt(i)->widget())->font_b());
            settings->setValue("font_i",((FontFrame*)ui->fontLayout->itemAt(i)->widget())->font_i());
            settings->setValue("font_bi",((FontFrame*)ui->fontLayout->itemAt(i)->widget())->font_bi());
        }
        else
        {
            settings->setValue("font",QFileInfo(((FontFrame*)ui->fontLayout->itemAt(i)->widget())->font()).fileName());
            settings->setValue("font",QFileInfo(((FontFrame*)ui->fontLayout->itemAt(i)->widget())->font_b()).fileName());
            settings->setValue("font",QFileInfo(((FontFrame*)ui->fontLayout->itemAt(i)->widget())->font_i()).fileName());
            settings->setValue("font",QFileInfo(((FontFrame*)ui->fontLayout->itemAt(i)->widget())->font_bi()).fileName());
        }
        settings->setValue("fontSize",((FontFrame*)ui->fontLayout->itemAt(i)->widget())->fontSize());
    }
    settings->endArray();
    return fonts_list;
}

void ExportFrame::LoadDefault()
{

}

void ExportFrame::UpdateToolComboBox(QSettings *settings)
{
    QString CurrentTool;
    if(!settings)
        CurrentTool=ui->CurrentTools->currentText();
    else
        CurrentTool=settings->value("current_tool").toString();
    while(ui->CurrentTools->count()>1)
        ui->CurrentTools->removeItem(1);
    QSettings* set=GetSettings(true);
    int count=set->beginReadArray("tools");
    for(int i=0;i<count;i++)
    {
        set->setArrayIndex(i);
        ui->CurrentTools->addItem(set->value("name").toString());
        if(set->value("name").toString()==CurrentTool)
        {
            ui->CurrentTools->setCurrentIndex(ui->CurrentTools->count()-1);
        }
    }
    set->endArray();
    delete set;
}

FontFrame* ExportFrame::AddFont(bool use, int tag, QString font,QString font_b,QString font_i,QString font_bi,int fontSize)
{
    FontFrame* frame=new FontFrame(use,tag,font,font_b,font_i,font_bi,fontSize,this);
    ui->fontLayout->insertWidget(ui->fontLayout->count()-2,frame);
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
    int index=ui->fontLayout->indexOf(font_widget);
    if(index==0 && direction>0)
        return;
    if(index==ui->fontLayout->count()-3 && direction<0)
        return;
    ui->fontLayout->removeWidget(font_widget);
    ui->fontLayout->insertWidget(index-direction,font_widget);
}
void ExportFrame::btnPath()
{
    QDir::setCurrent(ui->Path->text());
    QString dir=QFileDialog::getExistingDirectory(this,tr("Select device directory"));
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
    emit ChangeTabIndex(ui->tabWidget->currentIndex(),ui->toolBox->currentIndex());
}

void ExportFrame::onAddCoverLabelClicked()
{
    ui->coverLabel->setEnabled(ui->addCoverLabel->isChecked()&&!ui->createCaverAlways->isChecked());
    ui->label_tmplate->setEnabled(ui->addCoverLabel->isChecked()&&!ui->createCaverAlways->isChecked());
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
        if(QMessageBox::question(this,tr("Load CSS"),tr("Are you sure you want to load default CSS?"),QMessageBox::Yes|QMessageBox::No,QMessageBox::NoButton)!=QMessageBox::Yes)
            return;
    }
    QFile file(QApplication::applicationDirPath()+"/xsl/css/style.css");
    file.open(QFile::ReadOnly);
    QTextStream in(&file);
    ui->UserCSStext->setPlainText(in.readAll());
    file.close();
}
