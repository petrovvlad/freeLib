#include "qarchive.h"

#ifdef USE_LIBARCHIVE
#include <archive.h>
#include <archive_entry.h>
#endif //USE_LIBARCHIVE

#ifdef USE_QUAZIP
#ifdef QUAZIP_STATIC
#include "quazip/quazip/quazipfile.h"
#else
#include <quazip/quazipfile.h>
#include <quazip/quazipdir.h>
#endif
#endif //USE_QUAZIP

#ifdef USE_BIT7Z
#include <bit7z/bitarchivereader.hpp>
#include <bit7z/bitarchiveeditor.hpp>
#include <bit7z/bitfilecompressor.hpp>
#endif //USE_BIT7Z


#include "utilites.h"

#ifdef USE_BIT7Z
std::unique_ptr<bit7z::Bit7zLibrary> QArchive::b7zLib_ = nullptr;
#endif //USE_BIT7Z

QArchive::QArchive()
    : mode_(Mode::read)
#ifdef USE_LIBARCHIVE
    , archive_(nullptr)
    , entry_(nullptr)
#endif //USE_LIBARCHIVE
{
#ifdef USE_BIT7Z
    initLib();
#endif //USE_BIT7Z
}

QArchive::QArchive(const QString &sArchivePath)
    : mode_(Mode::read)
#ifdef USE_LIBARCHIVE
    , archive_(nullptr)
    , entry_(nullptr)
#endif //USE_LIBARCHIVE
{
    sArchivePath_ = sArchivePath;
#ifdef USE_BIT7Z
    initLib();
#endif //USE_BIT7Z
}

QArchive::QArchive(const QString &sArchivePath, Mode mode)
    : mode_(mode)
#ifdef USE_LIBARCHIVE
    , archive_(nullptr)
    , entry_(nullptr)
#endif //USE_LIBARCHIVE
{
    sArchivePath_ = sArchivePath;
#ifdef USE_BIT7Z
    initLib();
#endif //USE_BIT7Z
}

QArchive::QArchive(const QString &sArchivePath, const QString &sFormat, Mode mode)
    : mode_(mode)
#ifdef USE_LIBARCHIVE
    , archive_(nullptr)
    , entry_(nullptr)
#endif //USE_LIBARCHIVE
{
    sArchivePath_ = sArchivePath;
#ifdef USE_BIT7Z
    sFormat_ = sFormat;
    initLib();
#endif //USE_BIT7Z
}

QArchive::QArchive(const QByteArray data)
    : mode_(Mode::read)
#ifdef USE_LIBARCHIVE
    , archive_(nullptr)
    ,  entry_(nullptr)
#endif //USE_LIBARCHIVE
{
#ifdef USE_BIT7Z
    initLib();
    data_.reserve(data.size());
    std::ranges::transform(data, std::back_inserter(data_),
                           [](char b) { return static_cast<bit7z::byte_t>(b); });
#else //USE_BIT7Z
    data_ = data;
#endif //USE_BIT7Z
}

QArchive::QArchive(Mode mode)
    : mode_ (mode)
#ifdef USE_LIBARCHIVE
    , archive_(nullptr)
    , entry_(nullptr)
#endif //USE_LIBARCHIVE
{
#ifdef USE_BIT7Z
    initLib();
#endif //USE_BIT7Z
}

#ifdef USE_BIT7Z
void QArchive::initLib()
{
    static std::mutex m;
    std::lock_guard<std::mutex> guard(m);

    try {
        if(!b7zLib_)
            b7zLib_ = std::make_unique<bit7z::Bit7zLibrary>(find7zLibrary().toStdString());
    }catch (const bit7z::BitException& ex) {
        LogWarning  << "bit7z error:" << ex.what();
    }
}

QString QArchive::find7zLibrary() const
{
    QStringList candidates = {
        u"/usr/lib/p7zip/7z.so"_s,
        u"/usr/lib/x86_64-linux-gnu/p7zip/7z.so"_s,
        u"/usr/local/lib/7z.so"_s,
        u"/usr/lib/7z.so"_s,
        u"/usr/lib/7zip/7z.so"_s
    };
    for (const auto& p : candidates) {
        if (QFile::exists(p))
            return p;
    }
    return u"7z.so"_s;
}

#endif //USE_BIT7Z

QArchive::~QArchive()
{
    close();
}

void QArchive::setData(const QByteArray &data)
{
    close();
    sArchivePath_ = u""_s;
#ifdef USE_BIT7Z
    data_.reserve(data.size());
    std::ranges::transform(data, std::back_inserter(data_),
                           [](char b) { return static_cast<bit7z::byte_t>(b); });
#else //USE_BIT7Z
    data_ = data;
#endif //USE_BIT7Z
}

void QArchive::setPath(const QString &sArchivePath)
{
    close();
    sArchivePath_ = sArchivePath;
    data_.clear();
}

bool QArchive::setCurrentFile(const QString &sFileName)
{
    if(!open())
        return false;

#ifdef USE_BIT7Z
    if (!reader_) return false;
    currentIndex_ = -1;
    for (const auto& item : *reader_) {
        if (QString::fromStdString(item.path()) == sFileName) {
            currentIndex_ = item.index();
            return true;
        }
    }
    return false;
#endif //USE_BIT7Z

#ifdef USE_LIBARCHIVE
    if( archive_ ){
        QString sFileNameLow = sFileName.toLower();

        entry_ = archive_entry_new();
        bool bFound  = false;
        while (archive_read_next_header2(archive_, entry_) == ARCHIVE_OK) {
            const char *path = archive_entry_pathname_utf8(entry_);
            if (path) {
                QString filePath = QString::fromUtf8(path).toLower();
                if(filePath == sFileNameLow){
                    qDebug() << "format" << archive_format_name(archive_);

                    bFound = true;
                    break;
                }
            }
            archive_entry_clear(entry_);
        }
        if (!bFound) {
            LogWarning << "File not found in archive: " << sFileName;
            archive_entry_free(entry_);
            entry_ = nullptr;
            return false;
        }
        return true;
    }
    // else
#endif //USE_LIBARCHIVE

#ifdef USE_QUAZIP
        return zip_.setCurrentFile(sFileName, QuaZip::csInsensitive);
#endif //USE_QUAZIP
}

bool QArchive::open()
{
#ifdef USE_BIT7Z
    if(mode_ == read){
        if (reader_) return true;

        try {
            if (!sArchivePath_.isEmpty()) {
                reader_ = std::make_unique<bit7z::BitArchiveReader>(*b7zLib_, sArchivePath_.toStdString());
            } else if (!data_.empty()) {
                reader_ = std::make_unique<bit7z::BitArchiveReader>(*b7zLib_, data_, bit7z::BitFormat::Auto);
            }
        } catch (const bit7z::BitException& ex) {
            LogWarning  << "bit7z error:" << ex.what();
            return false;
        }
        return true;
    }
#endif //USE_BIT7Z

    bool bSuccess = false;
#ifdef USE_LIBARCHIVE
    bool bZip = sArchivePath_.endsWith(u".zip") || data_.startsWith("PK\x03\x04");
    if(!bZip && mode_ != Mode::create){
        if(archive_)
            return true;
        archive_ = archive_read_new();
        if (!archive_) {
            LogWarning << "Failed to create archive_read object";
            return false;
        }

        archive_read_support_filter_all(archive_);
        archive_read_support_format_zip(archive_);
        archive_read_support_format_7zip(archive_);
        archive_read_support_format_rar(archive_);
        archive_read_support_format_rar5(archive_);
        // archive_read_support_format_all(archive_);


        if(data_.size() != 0)
        {
            bSuccess = archive_read_open_memory(archive_, data_.constData(), data_.size()) == ARCHIVE_OK;
        }
        else if(!sArchivePath_.isEmpty())
        {
            const std::size_t blockSize = 262144;  // 256 КиБ
            const QByteArray pathUtf8 = sArchivePath_.toUtf8();
            bSuccess = archive_read_open_filename(archive_, pathUtf8.constData(), blockSize) == ARCHIVE_OK;
        }else{
            LogWarning << "Cannot open archive";
        }
    }
    else
#endif //USE_LIBARCHIVE

#ifdef USE_QUAZIP
    {
        if(zip_.isOpen())
            return true;
        switch(mode_){
        case Mode::read:
            if(data_.size() != 0)
            {
                buffer_.setData(data_);
                zip_.setIoDevice(&buffer_);
            }
            else if(!sArchivePath_.isEmpty())
                zip_.setZipName(sArchivePath_);

            if( !zip_.open(QuaZip::mdUnzip) ) [[unlikely]]
                LogWarning << "Error open archive!" << sArchivePath_;
            else
                bSuccess = true;
            break;

        case Mode::create:
            zip_.setZipName(sArchivePath_);
            if( !zip_.open(QuaZip::mdCreate) ) [[unlikely]]
                LogWarning << "Error creat archive!" << sArchivePath_;
            else
                bSuccess = true;
            break;

        default:
            break;
        }
    }
#endif //USE_QUAZIP

    return bSuccess;
}

void QArchive::close()
{
#ifdef USE_BIT7Z
    if(reader_){
        reader_ = nullptr/*release()*/;
    }
#endif //USE_BIT7Z
#ifdef USE_LIBARCHIVE
    if (archive_) {
        archive_read_close(archive_);
        archive_read_free(archive_);
        archive_ = nullptr;
        if(entry_){
            archive_entry_free(entry_);
            entry_ = nullptr;
        }
    }
    else
#endif //USE_LIBARCHIVE
#ifdef USE_QUAZIP
    {
        zip_.close();
        buffer_.close();
    }
#endif //USE_QUAZIP
}

std::vector<QString> QArchive::fileList()
{
#ifdef USE_BIT7Z
    return fileList("");
#else //USE_BIT7Z
    std::vector<QString> vFilesList;
    if(!open())
        return vFilesList;
#ifdef USE_LIBARCHIVE
    if (archive_) {
        entry_ = archive_entry_new();

        while (archive_read_next_header2(archive_, entry_) == ARCHIVE_OK) {
            const char *path = archive_entry_pathname_utf8(entry_);
            if (path) {
                QString filePath = QString::fromUtf8(path);
                // Пропускаем директории
                if (!filePath.endsWith('/')) {
                    vFilesList.push_back(filePath);
                }
            }
            archive_entry_clear(entry_);
        }
        close();
    }
    else
#endif //USE_LIBARCHIVE

#ifdef USE_QUAZIP
    {
        auto listFi = zip_.getFileInfoList64();
        for(const auto &fi :std::as_const(listFi)){
            if (!fi.name.endsWith('/'))
                vFilesList.push_back(fi.name);
        }
    }
#endif //USE_QUAZIP

    return vFilesList;
#endif //USE_BIT7Z
}

std::vector<QString> QArchive::fileList(const QString &sDir)
{
    std::vector<QString> vFilesList;
    if(!open())
        return vFilesList;
#ifdef USE_BIT7Z
    if (!reader_) return vFilesList;

    try {
        for (const auto& item : *reader_) {
            QString path = QString::fromStdString(item.path());
            if (path.isEmpty()) path = QString::fromStdString(item.name());

            if (sDir.isEmpty() || path.startsWith(sDir + "/") || path == sDir) {
                if (item.isDir())
                    path += "/";
                vFilesList.push_back(path);
            }
        }
    } catch (...) {}
#endif

#ifdef USE_LIBARCHIVE
    if (archive_) {
        entry_ = archive_entry_new();

        while (archive_read_next_header2(archive_, entry_) == ARCHIVE_OK) {
            const char *path = archive_entry_pathname_utf8(entry_);
            if (path) {
                QString filePath = QString::fromUtf8(path);
                // Пропускаем директории
                if (!filePath.endsWith('/')) {
                    vFilesList.push_back(filePath);
                }
            }
            archive_entry_clear(entry_);
        }
        close();
    }
    else
#endif //USE_LIBARCHIVE

#ifdef USE_QUAZIP
    {
        QuaZipDir dir(&zip_, sDir);
        if(dir.exists()){
            auto listFiles = dir.entryList(QDir::Files);
            for(const auto &sFile :std::as_const(listFiles))
                    vFilesList.push_back(sFile);
        }
    }
#endif //USE_QUAZIP

    return vFilesList;
}

std::vector<ArchiveFileInfo> QArchive::fileInfoList()
{
    std::vector<ArchiveFileInfo> vFilesList;
    if(!open())
        return vFilesList;
#ifdef USE_BIT7Z
    if (!reader_) return vFilesList;
    try {
        for (const auto& item : *reader_) {
            if(!item.isDir()){
                QString sFileName = QString::fromStdString(item.path());
                quint64 uncompressedSize = item.size();
                vFilesList.emplace_back(std::move(sFileName), uncompressedSize);
            }
        }
    } catch (...) {}
#endif //USE_BIT7Z
#ifdef USE_LIBARCHIVE
    if (archive_) {
        entry_ = archive_entry_new();

        while (archive_read_next_header2(archive_, entry_) == ARCHIVE_OK) {
            ArchiveFileInfo fi;
            const char *path = archive_entry_pathname_utf8(entry_);
            if (path) {
                QString filePath = QString::fromUtf8(path);
                fi.sFileName = filePath;
                fi.uncompressedSize = archive_entry_size(entry_);
                // Пропускаем директории
                if (!filePath.endsWith('/')) {
                    vFilesList.push_back(fi);
                }
            }
            archive_entry_clear(entry_);
        }
        close();
    }
    else
#endif //USE_LIBARCHIVE
#ifdef USE_QUAZIP
    {
        auto listFi = zip_.getFileInfoList64();
        for(const auto &quafi :std::as_const(listFi)){
            if (!quafi.name.endsWith('/')){
                ArchiveFileInfo fi;
                fi.sFileName = quafi.name;
                fi.uncompressedSize = quafi.uncompressedSize;
                vFilesList.push_back(fi);
            }
        }
    }
#endif //USE_QUAZIP
    return vFilesList;

}

QByteArray QArchive::readFile(const QString &sFileName)
{
    QByteArray result;
    if(!open())
        return result;
#ifdef USE_BIT7Z
    if (!reader_) return result;

    try {
        for (const auto& item : *reader_) {
            if (QString::fromStdString(item.path()) == sFileName) {
                auto size = item.size();
                result.resize(size);
                reader_->extractTo(reinterpret_cast< bit7z::byte_t *>(result.data()), size, item.index());
                break;
            }
        }
    } catch (const bit7z::BitException& ex) {
        LogWarning  << "bit7z error:" << ex.what();
    }
#endif //USE_BIT7Z

#ifdef USE_LIBARCHIVE
    if(archive_){
        entry_ = archive_entry_new();
        bool found = false;
        QString sFileNameLow = sFileName.toLower();

        while (archive_read_next_header2(archive_, entry_) == ARCHIVE_OK) {
            QString currentPath = QString::fromUtf8(archive_entry_pathname_utf8(entry_)).toLower();
            if (currentPath == sFileNameLow) {
                found = true;

                auto size = archive_entry_size(entry_);
                if (size <= 0 || size > 1024 * 1024 * 250) {  // Ограничение 250 МБ
                    archive_read_data_skip(archive_);
                    break;
                }

                result.resize(size);
                auto bytesRead = archive_read_data(archive_, result.data(), size);
                if (bytesRead != size) {
                    LogWarning << "Failed to read file data: " << archive_error_string(archive_);
                    result.clear();
                }
                break;
            }
            archive_entry_clear(entry_);
        }

        if (!found) {
            if(!sFileNameLow.endsWith(u".fbd"))
                LogWarning << "File not found in archive: " << sFileName;
        }

        close();
    }else
#endif //USE_LIBARCHIVE

#ifdef USE_QUAZIP
    {
        if(!zip_.setCurrentFile(sFileName, QuaZip::csInsensitive))
        {
            zip_.setFileNameCodec("IBM 866");
            if(!zip_.setCurrentFile(sFileName, QuaZip::csInsensitive))
            {
                LogWarning << "Failed to set file:" << sFileName;
                return result;
            }
        }
        QuaZipFile zipFile(&zip_);
        if(!zipFile.open(QIODevice::ReadOnly)) [[unlikely]]
        {
            LogWarning << "Error open file:" << sFileName;
        }else{
            result = zipFile.readAll();
            zipFile.close();
        }
    }
#endif //USE_QUAZIP
    return result;
}

QByteArray QArchive::readFile(const QString &sFileName, size_t size)
{ 
#ifdef USE_BIT7Z
    return readFile(sFileName).left(size);
#endif //USE_BIT7Z

    QByteArray result;
    if(!open())
        return result;
#ifdef USE_LIBARCHIVE
    if(archive_){
        entry_ = archive_entry_new();
        bool found = false;
        QString sFileNameLow = sFileName.toLower();

        while (archive_read_next_header2(archive_, entry_) == ARCHIVE_OK) {
            QString currentPath = QString::fromUtf8(archive_entry_pathname_utf8(entry_)).toLower();
            if (currentPath == sFileNameLow) {
                found = true;

                size = std::min(static_cast<size_t>(archive_entry_size(entry_)), size);
                if (size == 0 ) {
                    archive_read_data_skip(archive_);
                    break;
                }

                result.resize(size);
                la_ssize_t bytesRead = archive_read_data(archive_, result.data(), size);
                if (bytesRead != size) {
                    LogWarning << "Failed to read file data: " << archive_error_string(archive_);
                    result.clear();
                }
                break;
            }
            archive_entry_clear(entry_);
        }

        if (!found) {
            if(!sFileNameLow.endsWith(u".fbd"))
                LogWarning << "File not found in archive: " << sFileName;
        }

        close();
    }else
#endif //USE_LIBARCHIVE

#ifdef USE_QUAZIP
    {
        zip_.setCurrentFile(sFileName, QuaZip::csInsensitive);
        QuaZipFile zipFile(&zip_);
        if(!zipFile.open(QIODevice::ReadOnly)) [[unlikely]]
        {
            LogWarning << "Error open file:" << sFileName;
        }else{
            result = zipFile.read(size);
            zipFile.close();
        }
    }
#endif //USE_QUAZIP
    return result;
}

QByteArray QArchive::readFile(size_t size)
{
    QByteArray result;

#ifdef USE_BIT7Z
    return readFile().left(size);
#endif //USE_BIT7Z

#ifdef USE_LIBARCHIVE
    if(archive_){
        if(!entry_)
            return result;
        size = std::min(static_cast<size_t>(archive_entry_size(entry_)), size);
        if (size == 0) {
            return result;
        }

        result.resize(size);
        la_ssize_t bytesRead = archive_read_data(archive_, result.data(), size);
        if (bytesRead != size) {
            LogWarning << "Failed to read file data: " << archive_error_string(archive_);
            result.clear();
        }
        close();
    }
    else
#endif //USE_LIBARCHIVE

#ifdef USE_QUAZIP
    {
        QuaZipFile zipFile(&zip_);
        if(!zipFile.open(QIODevice::ReadOnly)) [[unlikely]]
        {
            LogWarning << "Error open file";
        }else{
            result = zipFile.read(size);
            zipFile.close();
        }
    }
#endif //USE_QUAZIP
    return result;

}

QByteArray QArchive::readFile()
{
    QByteArray result;

#ifdef USE_BIT7Z
    if(currentIndex_ >= 0 ) {
        try {
            auto item = reader_->itemAt(currentIndex_);
            auto size = item.size();
            result.resize(size);

            reader_->extractTo(reinterpret_cast< bit7z::byte_t *>(result.data()), size, currentIndex_);
        } catch (const bit7z::BitException& ex) {
            LogWarning  << "bit7z error:" << ex.what();
        }
    }
#endif //USE_BIT7Z

#ifdef USE_LIBARCHIVE
    if(archive_){
        if(!entry_)
            return result;
        auto size = archive_entry_size(entry_);
        if (size <= 0 || size > 1024 * 1024 * 250) {  // Ограничение 250 МБ
            return result;
        }

        result.resize(static_cast<int>(size));
        la_ssize_t bytesRead = archive_read_data(archive_, result.data(), size);
        if (bytesRead != size) {
            LogWarning << "Failed to read file data: " << archive_error_string(archive_);
            result.clear();
        }
        close();
    }
    else
#endif //USE_LIBARCHIVE

#ifdef USE_QUAZIP
    {
        QuaZipFile zipFile(&zip_);
        if(!zipFile.open(QIODevice::ReadOnly)) [[unlikely]]
        {
            LogWarning << "Error open file";
        }else{
            result = zipFile.readAll();
            zipFile.close();
        }
    }
#endif //USE_QUAZIP
    return result;
}

void QArchive::writeFile(const QString &sFileName, const QByteArray &data, int method, int level)
{
#ifdef USE_BIT7Z
    try {
        std::vector<bit7z::byte_t> v;
        v.reserve(data.size());
        std::ranges::transform(data, std::back_inserter(v),
                               [](char b) { return static_cast<bit7z::byte_t>(b); });
        if (QFile::exists(sArchivePath_)) {
            bit7z::BitArchiveEditor editor(*b7zLib_, sArchivePath_.toStdString(), bit7z::BitFormat::Zip);
            editor.addFile(v, sFileName.toStdString());
            editor.applyChanges();
        }else{

            bit7z::BitArchiveWriter writer(*b7zLib_, bit7z::BitFormat::Zip);

            writer.compressTo(sArchivePath_.toStdString());
         }

    } catch (const bit7z::BitException& ex) {
        LogWarning << "bit7z error:" << ex.what();
    }
#endif //USE_BIT7Z

#ifdef USE_QUAZIP
    if(!open())
        return;
    QuaZipFile zipFile(&zip_);
    int zipMethod;
    switch (method) {
    case Copy:
        zipMethod = 0;
        break;
    case BZip2:
        zipMethod = Z_BZIP2ED;
        break;
    default:
        zipMethod = Z_DEFLATED;
    }
    if(!zipFile.open(QIODevice::WriteOnly, QuaZipNewInfo(sFileName), nullptr, 0, zipMethod, level)) [[unlikely]]
    {
        LogWarning << "Error open file:" << sFileName;
    }else{
        zipFile.write(data);
    }
#endif //USE_QUAZIP
}


void QArchive::writeFile(const QString &sFileName, QFile &file, int method, int level)
{
    writeFile(sFileName, file.readAll(), method, level);
}

void QArchive::writeFile(const QString &sSrcFile, const QString &sDstFile, int method, int level)
{
#ifdef USE_BIT7Z
    try {
        bit7z::BitArchiveWriter writer(*b7zLib_,sArchivePath_.toStdString(), bit7z::BitFormat::Zip);
        writer.setUpdateMode(bit7z::UpdateMode::Update);
        writer.setCompressionMethod(static_cast<bit7z::BitCompressionMethod> (method));
        writer.setCompressionLevel(static_cast<bit7z::BitCompressionLevel> (level));
        writer.addFile(sSrcFile.toStdString(), sDstFile.toStdString());
        writer.compressTo(sArchivePath_.toStdString());
    } catch (const bit7z::BitException& ex) {
        LogWarning << "bit7z error:" << ex.what();
    }
#endif //USE_BIT7Z
#ifdef USE_QUAZIP
    QFile srcFile(sSrcFile);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if(srcFile.open(QIODeviceBase::ReadOnly)){
#else
    if(srcFile.open(QIODevice::ReadOnly)){
#endif //QT_VERSION
        writeFile(sDstFile, srcFile.readAll(), method, level);
            }else{
        LogWarning << "Error open file:" << sSrcFile;
    }
#endif //USE_QUAZIP

}

#ifdef USE_QUAZIP
void collectFiles(const QDir &d, std::vector<QString> &list)
{
    QStringList qsl = d.entryList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files | QDir::NoSymLinks);
    for(const QString &file: std::as_const(qsl))
    {
        QFileInfo finfo(u"%1/%2"_s.arg(d.path(), file));
        if (finfo.isDir())
        {
            QDir sd(finfo.filePath());
            collectFiles(sd, list);
        }
        else
            list.emplace_back(QDir::toNativeSeparators(finfo.filePath()));
    }
}
#endif //USE_QUAZIP

void QArchive::writeDir(const QString &sSrcDir)
{
#ifdef USE_BIT7Z
    try {
        bit7z::BitFileCompressor compressor (*b7zLib_, bit7z::BitFormat::Zip);
        compressor.setUpdateMode(bit7z::UpdateMode::Update);
        compressor.compressDirectoryContents(sSrcDir.toStdString(), sArchivePath_.toStdString());
    } catch (const bit7z::BitException& ex) {
        LogWarning << "bit7z error:" << ex.what();
    }
#endif //USE_BIT7Z
#ifdef USE_QUAZIP
    if(!open())
        return;

    const int basePathLength = sSrcDir.length();

    std::vector<QString> filePaths;
    collectFiles(QDir(sSrcDir), filePaths);
    for (const QString& fullPath : filePaths) {
        QFileInfo fileInfo(fullPath);
        if (!fileInfo.isFile())
            continue;
        QString sFile = fullPath.mid(basePathLength);
        writeFile(fullPath, sFile);
    }

#endif //USE_QUAZIP
}

bool QArchive::extractFileTo(const QString &sFileName, const QString &sDst)
{
    if(!open())
        return false;
    bool bSuccess = false;
#ifdef USE_BIT7Z
    if (!reader_) return false;
    bool bFound{false};
    try {
        for (const auto& item : *reader_) {
            if (QString::fromStdString(item.path()) == sFileName) {
                std::vector<bit7z::byte_t> buffer;
                auto size = item.size();
                buffer.resize(size);
                reader_->extractTo(buffer.data(), size, item.index());
                QDir().mkpath(QFileInfo(sDst).path());
                QFile dstFile(sDst);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                if(!dstFile.open(QIODeviceBase::WriteOnly))
#else
                if(!dstFile.open(QIODevice::WriteOnly))
#endif //QT_VERSION
                    throw;
                dstFile.write(reinterpret_cast<char*>(buffer.data()), size);
                dstFile.close();
                bFound = true;
                break;
            }
        }
        if(bFound)
            return true;
        LogWarning << "File not found in archive: " << sFileName;
        return false;
    } catch (const bit7z::BitException& ex) {
        LogWarning << "bit7z error:" << ex.what();
        return false;
    }
#endif //USE_BIT7Z
#ifdef USE_LIBARCHIVE
    if(archive_){

        entry_  = archive_entry_new();
        bool found = false;

        while (archive_read_next_header2(archive_, entry_) == ARCHIVE_OK) {
            QString currentPath = QString::fromUtf8(archive_entry_pathname_utf8(entry_));
            if (currentPath == sFileName) {
                found = true;

                QFile outFile(sDst);
                if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                    LogWarning << "Error open file :" << sDst;
                    break;
                }

                int fd = outFile.handle();
                int r = archive_read_data_into_fd(archive_, fd);
                if (r == ARCHIVE_OK || r == ARCHIVE_EOF) {
                    bSuccess = true;
                } else {
                    LogWarning << "Ошибка записи в fd:" << archive_error_string(archive_);
                }
                outFile.close();
                break;
            }
            archive_entry_clear(entry_);
        }

        if(!found)
            LogWarning << "File not found in archive: " << sFileName;

        close();
    }
    else
#endif //USE_LIBARCHIVE

#ifdef USE_QUAZIP
    {
        zip_.setCurrentFile(sFileName, QuaZip::csInsensitive);
        QuaZipFile zipFile(&zip_);
        if(!zipFile.open(QIODevice::ReadOnly)) [[unlikely]]
        {
            LogWarning << "Error open file:" << sFileName;
        }else{
            auto ba = zipFile.readAll();
            zipFile.close();
            QFile outFile(sDst);
            if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                LogWarning << "Error open file :" << sDst;
            }else{
                outFile.write(ba);
                outFile.close();
                bSuccess = true;
            }
        }
    }
#endif
    return bSuccess;
}

bool QArchive::extractFolderTo(const QString &sSrcDir, const QString &sDstDir)
{
    if(!open())
        return false;
#ifdef USE_BIT7Z
    if (!reader_) return false;

    try {
        if(sSrcDir.isEmpty() || sSrcDir == u"/"_s)
            reader_->extractRootFolderContentTo(sDstDir.toStdString());
        else
            reader_->extractFolderTo(sDstDir.toStdString(), sSrcDir.toStdString());
    } catch (...) {
        return false;
    }
    return true;
#endif //USE_BIT7Z

#ifdef USE_LIBARCHIVE
    bool bSuccess = false;
    if(archive_){
        entry_ = archive_entry_new();

        while (archive_read_next_header2(archive_, entry_) == ARCHIVE_OK) {
            QString currentPath = QString::fromUtf8(archive_entry_pathname_utf8(entry_));
            if(currentPath.startsWith(sSrcDir)){
                QString sDistFile = sDstDir + currentPath.mid(sSrcDir.length());
                QDir().mkpath(QFileInfo(sDistFile).path());
                QFile outFile(sDistFile);
                if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                    LogWarning << "Error open file :" << sDistFile;
                    break;
                }

                int fd = outFile.handle();
                int r = archive_read_data_into_fd(archive_, fd);
                if (r == ARCHIVE_OK || r == ARCHIVE_EOF) {
                    bSuccess = true;
                } else {
                    LogWarning << "Ошибка записи в fd:" << archive_error_string(archive_);
                    break;
                }
                outFile.close();

            }

        }
        archive_entry_clear(entry_);

    }
    close();
    return bSuccess;
#endif //USE_LIBARCHIVE

    return false;
}

QDateTime QArchive::getMTime(const QString &sFileName)
{
    QDateTime time;
    if(!open())
        return time;
#ifdef USE_BIT7Z
    if(reader_){
        for (const auto& item : *reader_) {
            if (QString::fromStdString(item.path()) == sFileName) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
                auto tp = std::chrono::time_point_cast<std::chrono::milliseconds>(item.creationTime());
                time = QDateTime::fromStdTimePoint(tp);
#else
                auto durationInMs = std::chrono::duration_cast<std::chrono::milliseconds>(item.creationTime().time_since_epoch());
                time = QDateTime::fromMSecsSinceEpoch(durationInMs.count());
#endif //QT_VERSION
                break;
            }
        }
    }
#endif //USE_BIT7Z

#ifdef USE_LIBARCHIVE
    if(archive_){
        entry_ = archive_entry_new();
        while (archive_read_next_header2(archive_, entry_) == ARCHIVE_OK) {
            const char *path = archive_entry_pathname_utf8(entry_);
            if (path) {
                QString filePath = QString::fromUtf8(path);
                if(filePath == sFileName){
                    time_t bt = archive_entry_mtime(entry_);
                    if (bt > 0)
                        time = QDateTime::fromSecsSinceEpoch(bt);
                }
            }
            archive_entry_clear(entry_);
        }
        close();
    }
    else
#endif //USE_LIBARCHIVE
#ifdef USE_QUAZIP
    {
        zip_.setCurrentFile(sFileName, QuaZip::csInsensitive);
        QuaZipFile zipFile(&zip_);
        if(!zipFile.open(QIODevice::ReadOnly)) [[unlikely]]
            LogWarning << "Error open file:" << sFileName;
        else{
            QuaZipFileInfo64 fiZip;
            if(zip_.getCurrentFileInfo(&fiZip)) [[likely]]
                time =  fiZip.dateTime;
        }
    }
#endif //USE_QUAZIP
    return time;
}

QDateTime QArchive::getMTime()
{
    QDateTime time;
#ifdef USE_BIT7Z
    if(currentIndex_ >= 0){
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
        auto tp = std::chrono::time_point_cast<std::chrono::milliseconds>(reader_->itemAt(currentIndex_).creationTime());
        time = QDateTime::fromStdTimePoint(tp);
#else
        auto durationInMs = std::chrono::duration_cast<std::chrono::milliseconds>(reader_->itemAt(currentIndex_).creationTime().time_since_epoch());
        time = QDateTime::fromMSecsSinceEpoch(durationInMs.count());
#endif //QT_VERSION
    }
#endif //USE_BIT7Z

#ifdef USE_LIBARCHIVE
    if(archive_){
        if(entry_){
            time_t bt = archive_entry_mtime(entry_);
            if (bt > 0)
                time = QDateTime::fromSecsSinceEpoch(bt);
        }
    }else
#endif //USE_LIBARCHIVE
#ifdef USE_QUAZIP
    {
        QuaZipFile zipFile(&zip_);
        if(!zipFile.open(QIODevice::ReadOnly)) [[unlikely]]
            LogWarning << "Error open file";
        else{
            QuaZipFileInfo64 fiZip;
            if(zip_.getCurrentFileInfo(&fiZip)) [[likely]]
                time =  fiZip.dateTime;
        }
    }
#endif
    return time;

}

QString QArchive::getArcName()
{
    return sArchivePath_;
}

uint32_t QArchive::filesCount()
{
#ifdef USE_BIT7Z
    if(!open() || !reader_)
        return 0;
    return reader_->filesCount();
#endif //USE_BIT7Z
#ifdef USE_QUAZIP
    if(!open()) return 0;
    return zip_.getEntriesCount();
#endif
    return 0;
}



