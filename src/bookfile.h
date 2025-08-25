#ifndef BOOKFILE_H
#define BOOKFILE_H

#include "library.h"

#include "epubreader.h"

class BookFile : public QObject
{
public:
    BookFile(uint idLib, uint idBook);
    BookFile(const SLib &pLib, uint idBook);

    ~BookFile();
    void open();
    void openAsync();
    QImage cover();
    QString annotation();
    QDateTime birthTime() const;
    QString fileName() const;
    QString filePath() const;
    qint64 fileSize() const;
    QByteArray data();
    QByteArray dataZip(const QString &sSubFormat);

    QFuture<QString> annotationAsync();
    QFuture<QImage> coverAsync();
    bool isBusy();
    bool isCoverCashed();

private:
    QByteArray openZipInZip(const QString &sArchive, const QString &sFileName);
    QImage coverFb2();
    void annotationFb2();
    void annotationZip();
    QImage coverEpub();
    void annotationEpub();
#ifdef USE_POPPLER
    QImage coverPdf();
#endif //USE_POPPLER

#ifdef USE_DEJVULIBRE
    QImage coverDjvu();
#endif
    QImage coverZip();
    QImage createCover();
    void cleanCoversCache();
    void initializeEpub();

    uint idBook_;
    QFile file_;
    QDateTime timeBirth_;
    QByteArray data_;
    QString sLibPath_;
    QString sFile_;
    QString sFormat_;
    QString sArchive_;
    QString sAnnotation_;
    QString sBookName_;
    QString sAuthorName_;
    QString sSequence_;
    std::unique_ptr<EpubReader> pEpub_;
    QDomDocument doc_;
    bool bOpen_;
    QFuture<void> futureOpen_;
    QFuture<QImage> futureCover_;
    QFuture<QString> futureAnnotation_;
};

namespace g {
    inline std::unordered_map<uint, std::shared_ptr<BookFile>> activeBooks;
}

#endif // BOOKFILE_H
