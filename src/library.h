#ifndef LIBRARY_H
#define LIBRARY_H

#include <QDateTime>
#include <QFileInfo>
#include <QBuffer>
#include <QVariant>
#include <chrono>
#include <unordered_set>

#include "utilites.h"
class SAuthor
{
public:
    SAuthor();
    SAuthor(const QString &sName);
    QString getName() const;
    std::unordered_set<uint> idTags;
    QString sFirstName;
    QString sLastName;
    QString sMiddleName;

    bool operator==(const SAuthor &other) const
    {
        return sFirstName == other.sFirstName && sMiddleName == other.sMiddleName && sLastName == other.sLastName;
    }

};

template <>
struct std::hash<SAuthor>
{
    std::size_t operator()(const SAuthor& k, size_t seed = 0) const noexcept
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        return qHashMulti(seed, k.sFirstName, k.sMiddleName, k.sLastName);
#else
        return static_cast<std::size_t>(qHash(k.sFirstName, seed)) << 32 ^
               static_cast<std::size_t>(qHash(k.sMiddleName, seed)) << 16 ^
               static_cast<std::size_t>(qHash(k.sLastName, seed));
#endif
    }
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
    QString sFile;
    QString sKeywords;
    std::vector<ushort> vIdGenres;
    std::vector<uint> vIdAuthors;
    std::unordered_set<uint> idTags;
    std::unordered_map<uint, uint> mSequences;
    uint idInLib;
    uint idFirstAuthor;
    uint nSize;
    uchar nStars;
    uchar idLanguage;
    bool bDeleted;
};

struct SSerial
{
    QString sName;
    std::unordered_set<uint> idTags;
};

struct SGenre
{
    QString sName;
    QStringList listKeys;
    ushort idParrentGenre;
};

class SLib
{
public:
    uint findAuthor(SAuthor& author) const;
    uint findSerial(const QString &sSerial) const;
    QString fillParams(const QString &str, uint idBook, bool bNestedBlock = false);
    QString fillParams(const QString &str, uint idBook, const QFileInfo &book_file);
    void deleteTag(uint idTag);
    static QString nameFromInpx(const QString &sInpx);

    QString name;
    QString path;
    QString sInpx;
    QString sVersion;
    QDate earliestDate_; //Самая ранняя дата добавления книги
    bool bFirstAuthor;
    bool bWoDeleted;
    bool bLoaded = false;
    std::unordered_map<uint, SAuthor> authors;
    std::unordered_multimap<uint, uint> authorBooksLink;
    std::unordered_map<uint, SBook> books;
    std::unordered_map<uint, SSerial> serials;
    std::vector<QString> vLaguages;
    std::chrono::time_point<std::chrono::system_clock> timeHttp{};
};

void loadLibrary(uint idLibrary);
void loadGenres();

namespace g {
inline std::unordered_map<uint, SLib> libs;
inline std::unordered_map<ushort, SGenre> genres;
inline uint idCurrentLib;
}
#endif // LIBRARY_H
