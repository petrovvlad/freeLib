#ifndef BOOKFILE_H
#define BOOKFILE_H

#include "library.h"

#include <QDomDocument>

class BookFile
{
public:
    BookFile(uint idLib, uint idBook);
    BookFile(SLib *pLib, uint idBook);

    ~BookFile();
    void open();
    QImage cover();
    QString annotation();
    QDateTime birthTime() const;
    QString fileName() const;
    QString filePath() const;
    qint64 fileSize() const;
    QByteArray data();
    QByteArray dataZip(const QString &sSubFormat);


private:
    QByteArray openZipInZip(const QString &sArchive, const QString &sFileName);
    QImage coverFb2();
    QImage coverFb2(const QDomDocument &doc);
    void annotationFb2();
    void annotationFb2(const QDomDocument &doc);
    void annotationZip();
    QImage coverAnnotationEpub();
    QImage coverAnnotationEpub(QuaZip &zipEpub);

#ifdef USE_DEJVULIBRE
    QImage coverDjvu();
#endif
    QImage coverZip();
    QImage createCover();
    void cleanCoversCache();

    SLib *pLib_;
    uint idBook_;
    QFile file_;
    QDateTime timeBirth_;
    //QBuffer bufferFile_;
    QByteArray data_;
    QString sAnnotation_;
    bool bOpen_;
    bool bCoverNotFound_;
};

#endif // BOOKFILE_H
