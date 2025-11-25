#ifndef QARCHIVE_H
#define QARCHIVE_H

struct archive;
struct archive_entry;

class QArchive
{
public:
    QArchive();
    QArchive(const QString &sArchivePath);
    QArchive(const QByteArray data);
    ~QArchive();

    void setData(const QByteArray data);
    void setPath(const QString &sArchivePath);
    void setCurrentFile(const QString &sFileName);
    void close();

    std::vector<QString> fileList();
    QByteArray readFile(const QString &filePathInArchive);
    QByteArray readFile();

    bool extractFileTo(const QString sFileName, const QString &sDst);
    QDateTime getMTime(const QString sFileName);
    QDateTime getMTime();

private:
#ifdef USE_LIBARCHIVE
    struct archive *archive_;
    struct archive_entry *entry_;
#endif //USE_LIBARCHIVE

    QBuffer buffer_;
    QuaZip zip_;
    QByteArray data_;
    QString sArchivePath_;

    bool open();
};

#endif // QARCHIVE_H
