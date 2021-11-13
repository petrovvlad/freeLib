#ifndef LIBRARY_H
#define LIBRARY_H

#include <QMultiMap>
#include <QList>
#include <QDateTime>
#include <QFileInfo>
#include <QBuffer>
#include <QVariant>

class SAuthor
{
public:
    SAuthor();
    SAuthor(const QString &sName);
    QString getName() const;
    int nTag;
    QString sFirstName;
    QString sLastName;
    QString sMiddleName;
};

struct SBook
{
    QString sName;
    QString sAnnotation;
    QString sImg;
    QString sArchive;
    QString sIsbn;
    QDate date;
    QString sFormat;
    QList<uint> listIdGenres;
    QList<uint> listIdAuthors;
    uint idInLib;
    QString sFile;
    uint idSerial;
    uint idFirstAuthor;
    uint numInSerial;
    uint nSize;
    uchar nStars;
    uchar idLanguage;
    uchar nTag;
    bool bDeleted;
};

struct SSerial
{
    QString sName;
    uchar nTag;
};

struct SGenre
{
    QString sName;
    ushort idParrentGenre;
    ushort nSort;
};

class SLib
{
public:
    uint findAuthor(SAuthor& author);
    uint findSerial(const QString &sSerial);
    void loadAnnotation(uint idBook);
    QFileInfo getBookFile(uint idBook, QBuffer *pBuffer=nullptr, QBuffer *pBufferInfo=nullptr, QDateTime *fileData=nullptr);
    QString fillParams(const QString &str, uint idBook);
    QString fillParams(const QString &str, uint idBook, const QFileInfo &book_file);


    QString name;
    QString path;
    QString sInpx;
    bool bFirstAuthor;
    bool bWoDeleted;
    bool bLoaded = false;
    QHash<uint,SAuthor> mAuthors;
    QMultiHash<uint,uint> mAuthorBooksLink;
    QHash<uint,SBook> mBooks;
    QHash<uint,SSerial> mSerials;
    QVector<QString> vLaguages;
};

void loadLibrary(uint idLibrary);
void loadGenres();

extern bool db_is_open;

extern uint idCurrentLib;
extern QMap<uint, SLib> mLibs;
extern QMap <uint, SGenre> mGenre;

#endif // LIBRARY_H
