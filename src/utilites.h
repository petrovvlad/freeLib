#ifndef UTILITES_H
#define UTILITES_H

#include <ranges>
#include <QSvgRenderer>
#include <QString>
#include <QSqlDatabase>
#include <QSettings>
#include <QtConcurrent>

#ifdef QUAZIP_STATIC
#include "quazip/quazip/quazip.h"
#else
#include <quazip/quazip.h>
#endif

#if __has_include(<execution>) //checking to see if the <execution> header is there
#ifdef emit
#undef emit
#define NOQTEMIT
#endif
#include <execution>
#ifdef NOQTEMIT
#define emit
#endif
#endif //__has_include(<execution>)


#define LogWarning (qWarning()<<__FILE__<<__LINE__<<__PRETTY_FUNCTION__)

#if (QT_VERSION < QT_VERSION_CHECK(6, 4, 0))
// https://doc.qt.io/qt-6/qstring.html#operator-22-22_s
inline QString operator""_s(const char16_t *str, const std::size_t size)
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    return QString::fromRawData(reinterpret_cast<const QChar *>(str), static_cast<int>(size));
#else // Qt≥6.0
    return operator""_qs(str, size);

#endif // Qt<6.0
}

inline QByteArray operator""_ba(const char *str, std::size_t size)
{
    return QByteArray(str, static_cast<int>(size));
}
#else //Qt≥6.4
using namespace Qt::Literals::StringLiterals;
#endif //Qt<6.4

#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
template<>
struct std::hash<QString> {
    std::size_t operator()(const QString& s) const noexcept {
        return (size_t) qHash(s);
    }
};
#endif

template <typename T>
bool contains(const std::vector<T> &v, T id)
{
    return std::ranges::find(v, id) != v.end();
}

struct tag
{
    QString name;
    QString css;
    QString font_name;
    quint16 font_size;
    tag(const QString &name, const QString &css, const QString &font_name, quint16 font_size)
        :name(name), css(css), font_name(font_name), font_size(font_size)
    {
    }
};
namespace g {
inline std::vector<tag> vTags;
}

QString RelativeToAbsolutePath(QString path);
bool openDB(const QString &sName);
QString Transliteration(QString str);
bool localeStringCompare(const QString &str1, const QString &str2);
#ifdef USE_HTTSERVER
void setProxy();
#endif
QSharedPointer<QSettings> GetSettings(bool bReopen = false);
void setLocale(const QString &sLocale);
bool setCurrentZipFileName(QuaZip *zip, const QString &name);
bool kindlegenInstalled();
QIcon themedIcon(const QString &sIcon);
QPixmap renderSvg(QSvgRenderer &render, bool bDark);

template <typename T, typename SequenceType, typename KeepFunctor>
std::vector<T> blockingFiltered(const std::unordered_map<T, SequenceType> &sequence, KeepFunctor &&keep)
{
    std::vector<T> v;
    v.reserve(sequence.size());
    std::ranges::copy(sequence | std::views::keys,  std::back_inserter(v));

    return QtConcurrent::blockingFiltered(v, [&](T id) {
        const auto &a = sequence.at(id);
        return keep(a);
    });
}

template <typename T, typename SequenceType, typename MapFunctor>
void blockingMap(const std::unordered_map<T, SequenceType> &sequence, MapFunctor &&map)
{
    std::vector<T> v;
    v.reserve(sequence.size());
    std::ranges::copy(sequence | std::views::keys,  std::back_inserter(v));

    QtConcurrent::blockingMap(v, [&](T id) {
        const auto &a = sequence.at(id);
        map(a);
    });
}

namespace g {
#ifdef __cpp_lib_execution
#ifdef USE_TBB
inline constexpr auto executionpolicy = std::execution::par;
#else
inline constexpr auto executionpolicy = std::execution::seq;
#endif //USE_TBB
#endif //__cpp_lib_execution
}

template<typename T, typename Compare>
void sort(std::vector<T>& v, Compare comp) {
#ifdef USE_TBB
    if (v.size() > 1000)
        std::sort(std::execution::par, v.begin(), v.end(), comp);
    else
        std::sort(std::execution::seq, v.begin(), v.end(), comp);
#else
    std::sort(v.begin(), v.end(), comp);
#endif //USE_TBB
}

#endif // UTILITES_H
