#ifndef COMMON_H
#define COMMON_H

#include <QSettings>
#include <QSqlQuery>
#include <QCommandLineParser>

#include "quazip/quazip/quazip.h"

#define AppName  QStringLiteral("freeLib")
#define OrgName  QStringLiteral("freeLibOrg")

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

QPixmap GetTag(QColor color,int size);
void SetLocale(const QString &sLocale);
QString decodeStr(const QString &str);
bool SetCurrentZipFileName(QuaZip *zip,const QString &name);
QString RelativeToAbsolutePath(QString path);

#endif // COMMON_H
