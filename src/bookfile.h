#ifndef BOOKFILE_H
#define BOOKFILE_H

#include "library.h"

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

private:
    QImage coverFb2();
    void annotationFb2();
    QImage coverAnnotationEpub();
#ifdef USE_DEJVULIBRE
    QImage coverDjvu();
#endif
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
};

#endif // BOOKFILE_H
