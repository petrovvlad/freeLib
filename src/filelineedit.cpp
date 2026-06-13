#include "filelineedit.h"

#include <QHBoxLayout>

FileLineEdit::FileLineEdit(QWidget *parent)
    :QLineEdit(parent)
{
    button_ = new QToolButton(this);
    button_->setFocusPolicy(Qt::NoFocus);
    button_->setCursor(Qt::ArrowCursor);
    button_->setText(QStringLiteral("…"));
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->addWidget(button_, 0, Qt::AlignRight);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    connect(button_, &QToolButton::clicked, this, &FileLineEdit::browse);
    connect(this, &QLineEdit::textChanged, this, &FileLineEdit::validate);
}

void FileLineEdit::setFileMode(FileMode mode)
{
    fileMode_ = mode;
}

FileLineEdit::FileMode FileLineEdit::fileMode() const
{
    return fileMode_;
}

void FileLineEdit::setNameFilter(const QString &filter)
{
    if( sNameFilter_!= filter){
        sNameFilter_= filter;
        emit nameFilterChanged(sNameFilter_);
    }
}

QString FileLineEdit::nameFilter() const
{
    return sNameFilter_;
}

void FileLineEdit::setCaptionDialog(const QString &caption)
{
    sCaptionDialog_ = caption;
}

QString FileLineEdit::captionDialog() const
{
    return sCaptionDialog_;
}

void FileLineEdit::setDefaultDirectory(const QString &dir)
{
    sDefaultDir_ = dir;
}

void FileLineEdit::setValidateFunction(ValidateFunction func)
{
    validateFunc = std::move(func);
}

void FileLineEdit::setButtonText(const QString &text)
{
    button_->setText(text);
}

QString FileLineEdit::buttonText() const
{
    return button_->text();
}

void FileLineEdit::setFileName(const QString &sFile)
{
    sFileName_ = sFile;
}

QString FileLineEdit::fileName() const
{
    return sFileName_;
}

void FileLineEdit::browse()
{
    QString sOldPath = text();
    QString sPath = sOldPath.isEmpty() ?(sDefaultDir_.isEmpty() ?QDir::homePath() :sDefaultDir_) :sOldPath;
    QString sNewPath;
    switch(fileMode_){
    case Directory:
        sNewPath = QFileDialog::getExistingDirectory(this, sCaptionDialog_, sPath);
        break;

    case ExistingFile:
        sNewPath = QFileDialog::getOpenFileName(this, sCaptionDialog_, sPath, sNameFilter_);
        break;

    case FileInDirectory:
        sNewPath = QFileDialog::getExistingDirectory(this, sCaptionDialog_, sPath);
        if(!sNewPath.isEmpty())
            sNewPath += u'/' + sFileName_;
        break;
    }

    if (!sNewPath.isEmpty() && sNewPath != sOldPath)
    {
        setText(sNewPath);
    }
}

void FileLineEdit::validate(const QString &sPath)
{
    if(!sPath.isEmpty()){
        bool bValid;
        switch(fileMode_){
        case Directory:
            bValid = QDir(sPath).exists();
            break;

        case ExistingFile:
            bValid = QFileInfo::exists(sPath);
            break;

        case FileInDirectory:
            bValid = true;
        }

        if(validateFunc)
            bValid &= validateFunc(sPath);

        QPalette palette = parentWidget()->palette();
        if(!bValid)
            palette.setColor(QPalette::Active, QPalette::Text,  palette.color(QPalette::Window).lightness() < 128 ?QColor(0xFF8A80) :QColor(0xB00020));
        setPalette(palette);
    }
}
