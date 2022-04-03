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
    QList<uint> listIdTags;
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
    QList<uint> listIdTags;
    uint idInLib;
    QString sFile;
    uint idSerial;
    uint idFirstAuthor;
    uint numInSerial;
    uint nSize;
    uchar nStars;
    uchar idLanguage;
    bool bDeleted;
};

struct SSerial
{
    QString sName;
    QList<uint> listIdTags;
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
    uint findAuthor(SAuthor& author) const;
    uint findSerial(const QString &sSerial) const;
    void loadAnnotation(uint idBook);
    QFileInfo getBookFile(uint idBook, QBuffer *pBuffer=nullptr, QBuffer *pBufferInfo=nullptr, QDateTime *fileData=nullptr);
    QString fillParams(const QString &str, uint idBook);
    QString fillParams(const QString &str, uint idBook, const QFileInfo &book_file);
    void deleteTag(uint idTag);


    QString name;
    QString path;
    QString sInpx;
    QString sVersion;
    bool bFirstAuthor;
    bool bWoDeleted;
    bool bLoaded = false;
    QHash<uint, SAuthor> mAuthors;
    QMultiHash<uint, uint> mAuthorBooksLink;
    QHash<uint, SBook> mBooks;
    QHash<uint, SSerial> mSerials;
    QVector<QString> vLaguages;
};

void loadLibrary(uint idLibrary);
void loadGenres();

extern uint idCurrentLib;
extern QMap<uint, SLib> mLibs;
extern QMap <uint, SGenre> mGenre;

#endif // LIBRARY_H
