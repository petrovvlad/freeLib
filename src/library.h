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
    std::unordered_map<uint, uint> series;
    uint idInLib;
    uint idFirstAuthor;
    uint nSize;
    uchar nStars;
    uchar idLanguage;
    bool bDeleted;
};

struct SSeries
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
    uint findSeries(const QString &sSeries) const;
    QString fillParams(const QString &str, uint idBook, bool bNestedBlock = false);
    QString fillParams(const QString &str, uint idBook, const QFileInfo &book_file);
    void deleteTag(uint idTag);
    static QString nameFromInpx(const QString &sInpx);
    void sortLanguages();

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
    std::unordered_map<uint, SSeries> series;
    std::vector<QString> vLaguages;
    std::vector<QString> vSortedLaguages;
    std::chrono::time_point<std::chrono::system_clock> timeHttp{};
};

void loadLibrary(uint idLibrary);
void loadGenres();

namespace g {
inline std::unordered_map<uint, SLib> libs;
inline std::unordered_map<ushort, SGenre> genres;
inline uint idCurrentLib;
inline std::unordered_map<QString, QString>  languges = {{u"ab"_s, u"Аԥсшәа"_s},
                                                        {u"ad"_s, u"Адыгэбзэ"_s},
                                                        {u"am"_s, u"አማርኛ"_s},
                                                        {u"ar"_s, u"العربية"_s},
                                                        {u"az"_s, u"Azərbaycanca"_s},
                                                        {u"ba"_s, u"Башҡортса"_s},
                                                        {u"be"_s, u"Беларуская"_s},
                                                        {u"bg"_s, u"Български"_s},
                                                        {u"bm"_s, u"ߓߡߊߣߊ߲ߞߊ߲"_s},
                                                        {u"bn"_s, u"বাংলা"_s},
                                                        {u"bo"_s, u"བོད་སྐད"_s},
                                                        {u"br"_s, u"Brezhoneg"_s},
                                                        {u"bs"_s, u"Bosanski"_s},
                                                        {u"ca"_s, u"Catala"_s},
                                                        {u"ce"_s, u"Нохчийн"_s},
                                                        {u"co"_s, u"Corsu"_s},
                                                        {u"cr"_s, u"Qırımtatar"_s},
                                                        {u"cs"_s, u"Čeština"_s},
                                                        {u"cu"_s, u"Словѣньскъ"_s},
                                                        {u"cv"_s, u"Чӑвашла"_s},
                                                        {u"da"_s, u"Dansk"_s},
                                                        {u"de"_s, u"Deutsch"_s},
                                                        {u"dv"_s, u"Dhivehi bas"_s},
                                                        {u"el"_s, u"Ελληνικά"_s},
                                                        {u"en"_s, u"English"_s},
                                                        {u"eo"_s, u"Esperanto"_s},
                                                        {u"es"_s, u"Español"_s},
                                                        {u"et"_s, u"Eesti"_s},
                                                        {u"ev"_s, u"Эвэды"_s},
                                                        {u"fa"_s, u"فارسی"_s},
                                                        {u"fi"_s, u"Suomi"_s},
                                                        {u"fr"_s, u"Français"_s},
                                                        {u"ga"_s, u"Gaeilge"_s},
                                                        {u"gu"_s, u"ગુજરાતી"_s},
                                                        {u"he"_s, u"עברית"_s},
                                                        {u"hi"_s, u"हिन्दी"_s},
                                                        {u"hr"_s, u"Hrvatski"_s},
                                                        {u"hu"_s, u"Magyar"_s},
                                                        {u"hy"_s, u"Հայերեն"_s},
                                                        {u"ia"_s, u"Interlingua"_s},
                                                        {u"id"_s, u"Bahasa Indonesia"_s},
                                                        {u"ie"_s, u"Interlingue"_s},
                                                        {u"is"_s, u"Íslenska"_s},
                                                        {u"it"_s, u"Italiano"_s},
                                                        {u"ja"_s, u"日本語"_s},
                                                        {u"ka"_s, u"ქართული"_s},
                                                        {u"kb"_s, u"Къарачай-малкъар"_s},
                                                        {u"kk"_s, u"Қазақша"_s},
                                                        {u"kl"_s, u"Kalaallisut"_s},
                                                        {u"km"_s, u"ភាសាខ្មែរ"_s},
                                                        {u"kn"_s, u"ಕನ್ನಡ"_s},
                                                        {u"ko"_s, u"한국어"_s},
                                                        //{u"kr"_s, u"Къарачай-Малкъар тил"_s},
                                                        {u"ks"_s, u"کٲشُر"_s},
                                                        {u"ku"_s, u"کوردی"_s},
                                                        {u"kv"_s, u"Коми"_s},
                                                        {u"ky"_s, u"Кыргызский"_s},
                                                        {u"la"_s, u"Latina"_s},
                                                        {u"le"_s, u"Лезги"_s},
                                                        {u"lt"_s, u"Lietuvių"_s},
                                                        {u"lv"_s, u"Latviešu"_s},
                                                        {u"mk"_s, u"Македонски"_s},
                                                        {u"ml"_s, u"മലയാളം"_s},
                                                        {u"mn"_s, u"Монгол"_s},
                                                        {u"mr"_s, u"मराठी"_s},
                                                        {u"my"_s, u"ဗမာစာ"_s},
                                                        {u"ne"_s, u"नेपाली"_s},
                                                        {u"nl"_s, u"Nederlands"_s},
                                                        {u"no"_s, u"Norsk"_s},
                                                        {u"oc"_s, u"Occitan"_s},
                                                        {u"os"_s, u"Ирон"_s},
                                                        {u"pl"_s, u"Polski"_s},
                                                        {u"ps"_s, u"پښتو"_s},
                                                        {u"pt"_s, u"Português"_s},
                                                        {u"ro"_s, u"Română"_s},
                                                        {u"ru"_s, u"Русский"_s},
                                                        {u"rw"_s, u"Kinyarwanda"_s},
                                                        {u"sa"_s, u"संस्कृत"_s},
                                                        {u"sd"_s, u"सिन्धी"_s},
                                                        {u"sk"_s, u"Slovenčina"_s},
                                                        {u"sl"_s, u"Slovenski"_s},
                                                        {u"sp"_s, u"Español"_s},
                                                        {u"sq"_s, u"Shqip"_s},
                                                        {u"sr"_s, u"Српски"_s},
                                                        {u"sv"_s, u"Svenska"_s},
                                                        {u"sw"_s, u"Kiswahili"_s},
                                                        {u"ta"_s, u"தமிழ்"_s},
                                                        {u"te"_s, u"తెలుగు"_s},
                                                        {u"tg"_s, u"Тоҷикӣ"_s},
                                                        {u"th"_s, u"ภาษาไทย"_s},
                                                        {u"tk"_s, u"Türkmençe"_s},
                                                        {u"tr"_s, u"Türkçe"_s},
                                                        {u"tt"_s, u"Татарча"_s},
                                                        {u"ug"_s, u"ئۇيغۇرچە"_s},
                                                        {u"uk"_s, u"Українська"_s},
                                                        {u"ur"_s, u"اردو"_s},
                                                        {u"uz"_s, u"Oʻzbekcha"_s},
                                                        {u"vi"_s, u"Tiếng Việt"_s},
                                                        {u"yi"_s, u"ייִדיש"_s},
                                                        {u"zh"_s, u"中文"_s}};

}
#endif // LIBRARY_H
