#ifndef LIBRARY_H
#define LIBRARY_H

#include <QMultiMap>
#include <QList>
#include <QDateTime>

struct SAuthor
{
    int nTag;
    QString sName;
};

struct SBook
{
    QString sName;
    QString sAnnotation;
    QString sImg;
    QString sArchive;
    QDate date;
    QString sFormat;
    QList<uint> listIdGenres;
    QList<uint> listIdAuthors;
    uint idInLib;
    uint nFile;
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

struct SLib
{
    QString name;
    QString path;
    QString sInpx;
    bool bFirstAuthor;
    bool bWoDeleted;
    QHash<uint,SAuthor> mAuthors;
    QMultiHash<uint,uint> mAuthorBooksLink;
    QHash<uint,SBook> mBooks;
    QHash<uint,SSerial> mSerials;
    QVector<QString> vLaguages;
};

void loadLibrary(uint idLibrary);
void loadGenres();

extern bool db_is_open;

extern SLib current_lib;
extern QMap<int,SLib> mLibs;
extern QMap <uint,SGenre> mGenre;

#endif // LIBRARY_H
