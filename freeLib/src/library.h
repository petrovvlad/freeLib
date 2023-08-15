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

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
inline bool operator==(const SAuthor &a1, const SAuthor &a2)
{
    return a1.sFirstName == a2.sFirstName && a1.sMiddleName == a2.sMiddleName && a1.sLastName == a2.sLastName;
}

inline size_t qHash(const SAuthor &key, size_t seed = 0)
{
    return qHashMulti(seed, key.sFirstName, key.sMiddleName, key.sLastName);
}
#endif

struct SBook
{
    QString sName;
    QString sAnnotation;
    QString sImg;
    QString sArchive;
    QString sIsbn;
    QDate date;
    QString sFormat;
    QString sFile;
    QString sKeywords;
    QList<uint> listIdGenres;
    QList<uint> listIdAuthors;
    QList<uint> listIdTags;
    uint idInLib;
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
    QStringList listKeys;
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
    static QString nameFromInpx(const QString &sInpx);

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
