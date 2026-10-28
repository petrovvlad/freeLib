#ifndef QARCHIVE_H
#define QARCHIVE_H

#include "utilites.h"
#ifdef USE_QUAZIP
#ifdef QUAZIP_STATIC
#include "quazip/quazip/quazipfile.h"
#else
#include <quazip/quazipfile.h>
#endif
#endif //USE_QUAZIP

#ifdef USE_BIT7Z
// #include "bit7z/include/bit7z/bit7z.hpp"
#include "bit7z/bit7zlibrary.hpp"
#include "bit7z/bitarchivereader.hpp"
// #include "bit7z/include/bit7z/bitarchivewriter.hpp"
#endif //USE_BIT7Z


#ifdef USE_LIBARCHIVE
struct archive;
struct archive_entry;
#endif //USE_LIBARCHIVE

struct ArchiveFileInfo{
    QString sFileName;
    quint64 uncompressedSize;
    //QDateTime lastModified;
    //bool isDir = false;
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
    enum Method {
        Copy,       ///< No compression (the data is stored as-is).
        Deflate,    ///< The Deflate compression method.
        Deflate64,  ///< The Deflate64 compression method.
        BZip2,      ///< The BZip2 compression method.
        Lzma,       ///< The LZMA compression method.
        Lzma2,      ///< The LZMA2 compression method.
        Ppmd        ///< The PPMd compression method.
    };
    QArchive();
    QArchive(const QString &sArchivePath);
    QArchive(const QString &sArchivePath, Mode mode);
    QArchive(const QString &sArchivePath, const QString &sFormat, Mode mode);
    QArchive(const QByteArray data);
    QArchive(Mode mode);
    ~QArchive();

    void setData(const QByteArray &data);
    void setPath(const QString &sArchivePath);
    bool setCurrentFile(const QString &sFileName);
    void close();

    std::vector<QString> fileList();
    std::vector<QString> fileList(const QString &sDir);
    std::vector<ArchiveFileInfo> fileInfoList();
    QByteArray readFile(const QString &sFileName);
    QByteArray readFile(const QString &sFileName, size_t size);
    QByteArray readFile(size_t size);
    QByteArray readFile();

    void writeFile(const QString &sFileName, const QByteArray &data, int method = Method::Deflate, int level = 5);
    void writeFile(const QString &sFileName, QFile &file, int method = Method::Deflate, int level = 5);
    void writeFile(const QString &sSrcFile, const QString &sDstFile, int method = Method::Deflate, int level = 5);
    void writeDir(const QString &sSrcDir);

    bool extractFileTo(const QString &sFileName, const QString &sDst);
    bool extractFolderTo(const QString &sSrcDir, const QString &sDstDir);
    QDateTime getMTime(const QString &sFileName);
    QDateTime getMTime();
    QString getArcName();
    uint32_t filesCount();

    inline static std::vector<QString> formats ={
        u"zip"_s
#if defined(USE_LIBARCHIVE) || defined(USE_BIT7Z)
        ,u"rar"_s, u"7z"_s
#endif //USE_LIBARCHIVE
    };


private:
#ifdef USE_LIBARCHIVE
    struct archive *archive_;
    struct archive_entry *entry_;
#endif //USE_LIBARCHIVE


#ifdef USE_QUAZIP
    QuaZip zip_;
    QBuffer buffer_;
#endif //USE_QUAZIP
    QString sArchivePath_;
    Mode mode_;


#ifdef USE_BIT7Z
    void initLib();
    QString find7zLibrary() const;

    static std::unique_ptr<bit7z::Bit7zLibrary> b7zLib_;
    std::unique_ptr<bit7z::BitArchiveReader> reader_;

    int currentIndex_ = -1;
    bit7z::buffer_t data_;
    QString sFormat_;
#else //USE_BIT7Z
    QByteArray data_;
#endif //USE_BIT7Z

    bool open();
};

#endif // QARCHIVE_H
