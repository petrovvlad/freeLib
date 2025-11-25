#include "qarchive.h"

#ifdef USE_LIBARCHIVE
#include <archive.h>
#include <archive_entry.h>
#else //USE_LIBARCHIVE
#ifdef QUAZIP_STATIC
#include "quazip/quazip/quazipfile.h"
#else
#include <quazip/quazipfile.h>
#endif
#endif //USE_LIBARCHIVE


#include "utilites.h"

QArchive::QArchive()
#ifdef USE_LIBARCHIVE
    : archive_(nullptr),
      entry_(nullptr)
#endif //USE_LIBARCHIVE
{
}

QArchive::QArchive(const QString &sArchivePath)
#ifdef USE_LIBARCHIVE
    : archive_(nullptr),
      entry_(nullptr)
#endif //USE_LIBARCHIVE
{
    sArchivePath_ = sArchivePath;
}

QArchive::QArchive(const QByteArray data)
#ifdef USE_LIBARCHIVE
    : archive_(nullptr),
      entry_(nullptr)
#endif //USE_LIBARCHIVE

{
    data_ = data;
}

QArchive::~QArchive()
{
    close();
}

void QArchive::setData(const QByteArray data)
{
    close();
    sArchivePath_ = u""_s;
    data_ = data;
}

void QArchive::setPath(const QString &sArchivePath)
{
    close();
    sArchivePath_ = sArchivePath;
    data_.clear();
}

void QArchive::setCurrentFile(const QString &sFileName)
{
    qint64 timeStart, timeEnd;
    timeStart = QDateTime::currentMSecsSinceEpoch();

    if(!open())
        return;
#ifdef USE_LIBARCHIVE
    if( archive_ ){
        entry_ = archive_entry_new();
        bool bFound  = false;
        while (archive_read_next_header2(archive_, entry_) == ARCHIVE_OK) {
            const char *path = archive_entry_pathname(entry_);
            if (path) {
                QString filePath = QString::fromUtf8(path);
                if(filePath == sFileName){
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
        }
    }
    else
#endif //USE_LIBARCHIVE
        zip_.setCurrentFile(sFileName);

    timeEnd = QDateTime::currentMSecsSinceEpoch();
    qDebug() << "setCurrentFile " << timeEnd - timeStart << "msec " << sFileName;
}

bool QArchive::open()
{
    bool bSuccess = false;
#ifdef USE_LIBARCHIVE
    if(!sArchivePath_.endsWith(u".zip")){
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
    {
        if(zip_.isOpen())
            return true;
        if(data_.size() != 0)
        {
            buffer_.setData(data_);
            zip_.setIoDevice(&buffer_);
        }
        else if(!sArchivePath_.isEmpty())
        {
            zip_.setZipName(sArchivePath_);
        }

        if( !zip_.open(QuaZip::mdUnzip) ) [[unlikely]]
        {
            LogWarning << "Error open archive!" << sArchivePath_;
        }else
            bSuccess = true;
    }

    return bSuccess;
}

void QArchive::close()
{
#ifdef USE_LIBARCHIVE
    if (archive_) {
        archive_read_close(archive_);
        archive_read_free(archive_);
        archive_ = nullptr;
        if(entry_)
            archive_entry_free(entry_);
    }
    else
#endif //USE_LIBARCHIVE
    {
        zip_.close();
        buffer_.close();
    }
}

std::vector<QString> QArchive::fileList()
{
    std::vector<QString> vFilesList;
    if(!open())
        return vFilesList;
#ifdef USE_LIBARCHIVE
    if (archive_) {
        entry_ = archive_entry_new();

        while (archive_read_next_header2(archive_, entry_) == ARCHIVE_OK) {
            const char *path = archive_entry_pathname(entry_);
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
    {
        auto listFi = zip_.getFileInfoList64();
        for(const auto &fi :std::as_const(listFi)){
            if (!fi.name.endsWith('/'))
                vFilesList.push_back(fi.name);
        }
    }

    return vFilesList;
}

QByteArray QArchive::readFile(const QString &filePathInArchive)
{
    QByteArray result;
    if(!open())
        return result;
#ifdef USE_LIBARCHIVE
    if(archive_){
        entry_ = archive_entry_new();
        bool found = false;

        while (archive_read_next_header2(archive_, entry_) == ARCHIVE_OK) {
            QString currentPath = QString::fromUtf8(archive_entry_pathname(entry_));
            if (currentPath == filePathInArchive) {
                found = true;

                qint64 size = archive_entry_size(entry_);
                if (size <= 0 || size > 1024 * 1024 * 250) {  // Ограничение 250 МБ
                    archive_read_data_skip(archive_);
                    break;
                }

                result.resize(static_cast<int>(size));
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
            if(!filePathInArchive.endsWith(u".fbd"))
                LogWarning << "File not found in archive: " << filePathInArchive;
        }

        close();
    }else
#endif //USE_LIBARCHIVE

    {
        zip_.setCurrentFile(filePathInArchive);
        QuaZipFile zipFile(&zip_);
        if(!zipFile.open(QIODevice::ReadOnly)) [[unlikely]]
        {
            LogWarning << "Error open file:" << filePathInArchive;
        }else{
            result = zipFile.readAll();
            zipFile.close();
        }
    }

    return result;
}

QByteArray QArchive::readFile()
{
    QByteArray result;

#ifdef USE_LIBARCHIVE
    if(archive_){
        if(!entry_)
            return result;
        qint64 size = archive_entry_size(entry_);
        if (size <= 0 || size > 1024 * 1024 * 250) {  // Ограничение 250 МБ
            return result;
        }

        result.resize(static_cast<int>(size));
        la_ssize_t bytesRead = archive_read_data(archive_, result.data(), size);
        if (bytesRead != size) {
            LogWarning << "Failed to read file data: " << archive_error_string(archive_);
            result.clear();
        }
    }
    else
#endif //USE_LIBARCHIVE
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

    return result;

}

bool QArchive::extractFileTo(const QString sFileName, const QString &sDst)
{
    if(!open())
        return false;
    bool bSuccess = false;
#ifdef USE_LIBARCHIVE
    if(archive_){

        entry_ = archive_entry_new();
        bool found = false;

        while (archive_read_next_header2(archive_, entry_) == ARCHIVE_OK) {
            QString currentPath = QString::fromUtf8(archive_entry_pathname(entry_));
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
    {
        zip_.setCurrentFile(sFileName);
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

    return bSuccess;
}

QDateTime QArchive::getMTime(const QString sFileName)
{
    QDateTime time;
    if(!open())
        return time;
#ifdef USE_LIBARCHIVE
    if(archive_){
        entry_ = archive_entry_new();
        while (archive_read_next_header2(archive_, entry_) == ARCHIVE_OK) {
            const char *path = archive_entry_pathname(entry_);
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
    {
        zip_.setCurrentFile(sFileName);
        QuaZipFile zipFile(&zip_);
        if(!zipFile.open(QIODevice::ReadOnly)) [[unlikely]]
            LogWarning << "Error open file:" << sFileName;
        else{
            QuaZipFileInfo64 fiZip;
            if(zip_.getCurrentFileInfo(&fiZip)) [[likely]]
                time =  fiZip.dateTime;
        }
    }
    return time;
}

QDateTime QArchive::getMTime()
{
    QDateTime time;
#ifdef USE_LIBARCHIVE
    if(archive_){
        if(entry_){
            time_t bt = archive_entry_mtime(entry_);
            if (bt > 0)
                time = QDateTime::fromSecsSinceEpoch(bt);
        }
    }else
#endif //USE_LIBARCHIVE
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
    return time;

}
