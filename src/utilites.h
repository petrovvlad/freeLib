#ifndef UTILITES_H
#define UTILITES_H

#include <ranges>
#include <QSvgRenderer>
#include <QString>
#include <QSqlDatabase>
#include <QSettings>
#include <QtConcurrent/QtConcurrentFilter>

#ifdef QUAZIP_STATIC
#include "quazip/quazip/quazip.h"
#else
#include <quazip/quazip.h>
#endif

#define LogWarning (qWarning()<<__FILE__<<__LINE__<<__PRETTY_FUNCTION__)

#if (QT_VERSION < QT_VERSION_CHECK(6, 4, 0))
// https://doc.qt.io/qt-6/qstring.html#operator-22-22_s
inline QString operator"" _s(const char16_t *str, const std::size_t size)
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    return QString::fromRawData(reinterpret_cast<const QChar *>(str), static_cast<int>(size));
#else
    return operator""_qs(str, size);
#endif
}
#else
using namespace Qt::Literals::StringLiterals;
#endif

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
    return std::find(v.begin(), v.end(), id) != v.end();
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
void ClearLib(const QSqlDatabase &dbase, qlonglong id_lib, bool delete_only);
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



#endif // UTILITES_H
