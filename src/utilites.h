#ifndef UTILITES_H
#define UTILITES_H

#include <QString>
#include <QSqlDatabase>
#include <QSettings>

#ifdef QUAZIP_STATIC
#include "quazip/quazip/quazip.h"
#else
#include <quazip/quazip.h>
#endif


#define MyDBG (qDebug()<<__FILE__<<__LINE__<<__PRETTY_FUNCTION__)

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
extern std::vector<tag> vTags;

QString RelativeToAbsolutePath(QString path);
bool openDB(const QString &sName);
void ClearLib(const QSqlDatabase &dbase, qlonglong id_lib, bool delete_only);
QString Transliteration(QString str);
bool localeStringCompare(const QString &str1, const QString &str2);
void setProxy();
QSharedPointer<QSettings> GetSettings(bool bReopen = false);
void setLocale(const QString &sLocale);
bool setCurrentZipFileName(QuaZip *zip, const QString &name);
bool kindlegenInstalled();


#endif // UTILITES_H
