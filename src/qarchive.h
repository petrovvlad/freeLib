#ifndef QARCHIVE_H
#define QARCHIVE_H

#include "utilites.h"

struct archive;
struct archive_entry;

struct AchiveFileInfo{
    QString sFileName;
    quint64 uncompressedSize;
};

class QArchive
{
public:
    QArchive();
    QArchive(const QString &sArchivePath);
    QArchive(const QByteArray data);
    ~QArchive();

    void setData(const QByteArray data);
    void setPath(const QString &sArchivePath);
    bool setCurrentFile(const QString &sFileName);
    void close();

    std::vector<QString> fileList();
    std::vector<AchiveFileInfo> fileInfoList();
    QByteArray readFile(const QString &sFileName);
    QByteArray readFile(const QString &sFileName, size_t size);
    QByteArray readFile(size_t size);
    QByteArray readFile();


    bool extractFileTo(const QString sFileName, const QString &sDst);
    QDateTime getMTime(const QString sFileName);
    QDateTime getMTime();
    QString getArcName();

    inline static std::vector<QString> formats ={
        u"zip"_s
#ifdef USE_LIBARCHIVE
        ,u"rar"_s, u"7z"_s
#endif //USE_LIBARCHIVE
    };


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
