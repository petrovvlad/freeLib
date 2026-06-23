#ifndef QARCHIVE_H
#define QARCHIVE_H

#include "utilites.h"
#ifdef QUAZIP_STATIC
#include "quazip/quazip/quazipfile.h"
#else
#include <quazip/quazipfile.h>
#endif


struct archive;
struct archive_entry;

struct AchiveFileInfo{
    QString sFileName;
    quint64 uncompressedSize;
};

class QArchive
{
public:
    enum Mode {
        //notOpen,
        read,
        create
        //,append
        //,add
    };
    QArchive();
    QArchive(const QString &sArchivePath);
    QArchive(const QString &sArchivePath, Mode mode);
    QArchive(const QByteArray data);
    QArchive(Mode mode);
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

    void writeFile(const QString &sFileName, const QByteArray &data, int method = Z_DEFLATED, int level = Z_DEFAULT_COMPRESSION);
    void writeFile(const QString &sFileName, QFile &file, int method = Z_DEFLATED, int level = Z_DEFAULT_COMPRESSION);



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
    Mode mode_;

    bool open();
};

#endif // QARCHIVE_H
