#ifndef UTILITES_H
#define UTILITES_H

#include <QString>
#include <QSqlDatabase>
#include <QSettings>

#include "quazip/quazip/quazip.h"

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
extern QList<tag> tag_list;

QString RelativeToAbsolutePath(QString path);
bool openDB(const QString &sName);
void ClearLib(const QSqlDatabase &dbase, qlonglong id_lib, bool delete_only);
QString Transliteration(QString str);
void setProxy();
QSharedPointer<QSettings> GetSettings(bool bReopen = false);
void setLocale(const QString &sLocale);
bool setCurrentZipFileName(QuaZip *zip, const QString &name);


#endif // UTILITES_H
