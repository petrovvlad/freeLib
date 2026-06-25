#ifndef FILELINEEDIT_H
#define FILELINEEDIT_H

#include <QLineEdit>
#include <QToolButton>
#include <QFileDialog>


class FileLineEdit : public QLineEdit
{
    Q_OBJECT
    Q_PROPERTY(QString nameFilter READ nameFilter WRITE setNameFilter NOTIFY nameFilterChanged DESIGNABLE true)
    Q_PROPERTY(FileMode fileMode READ fileMode WRITE setFileMode NOTIFY fileModeChanged DESIGNABLE true)
    Q_PROPERTY(QString captionDialog READ captionDialog WRITE setCaptionDialog NOTIFY captionDialogChanged DESIGNABLE true)
    Q_PROPERTY(QString buttonText READ buttonText WRITE setButtonText NOTIFY textButtonChanged DESIGNABLE true)
    Q_PROPERTY(QString fileName READ fileName WRITE setFileName NOTIFY fileNameChaged DESIGNABLE true)

    using ValidateFunction = std::function<bool(const QString&)>;

public:
    enum FileMode {
        ExistingFile,
        Directory,
        FileInDirectory
    };
    Q_ENUM(FileMode)

    explicit FileLineEdit(QWidget *parent = nullptr);

    void setFileMode(FileMode mode);
    FileMode fileMode() const;

    void setNameFilter(const QString &filter);
    QString nameFilter() const;

    void setCaptionDialog(const QString &caption);
    QString captionDialog() const;    

    void setButtonText(const QString &text);
    QString buttonText() const;

    void setFileName(const QString &sFile);
    QString fileName() const;

    void setDefaultDirectory(const QString &dir);
    void setValidateFunction(ValidateFunction func);

signals:
    void nameFilterChanged(const QString &filter);
    void fileModeChanged(FileLineEdit::FileMode mode);
    void captionDialogChanged(const QString &title);
    void textButtonChanged(const QString &text);
    void fileNameChaged(const QString &sFile);

private slots:
    void browse();

private:
    void validate(const QString &sPath);
    ValidateFunction validateFunc;

    QToolButton *button_;
    FileMode fileMode_ = ExistingFile;
    QString sNameFilter_ = QStringLiteral("All files (*.*)");
    QString sDefaultDir_;
    QString sCaptionDialog_;
    QString sFileName_;
};

#endif // FILELINEEDIT_H
