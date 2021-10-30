#ifndef COMMON_H
#define COMMON_H

#include <QSettings>
#include <QtWidgets/QFileDialog>
#include <QDesktopServices>
#include <QSqlQuery>
#include <QDebug>
#include <QtWidgets/QLabel>
#include <QPixmap>
#include <QSqlError>
#include <QtWidgets/QMessageBox>
#include <QBuffer>
#include <QPixmap>
#include <QDateTime>
#include <QTranslator>
#include <QApplication>
#include "quazip/quazip/quazip.h"
#include <QCommandLineParser>
#include <QMultiMap>
#include <QList>

#define AppName  QStringLiteral("freeLib")
#define OrgName  QStringLiteral("freeLibOrg")
#define default_exp_file_name QStringLiteral("%a/%s/%n3%b")
#define default_book_title "(%abbrs %n2) %b"
#define default_author_name QStringLiteral("%nf %nm %nl")
#define default_cover_label QStringLiteral("%abbrs - %n2")
#define default_OPDS_port   8080
#define default_proxy_port 8080

#define default_dropcaps_font "sangha.ttf"

extern QApplication *app;
extern QTranslator* translator;
extern QTranslator* translator_qt;
extern bool db_is_open;
extern QCommandLineParser CMDparser;

enum SendType{ST_Device,ST_Mail};
QSettings* GetSettings(bool reopen=false);

enum APP_MODE{MODE_LIBRARY,MODE_CONVERTER,MODE_SHELF};

struct tag
{
    QString name;
    QString css;
    QString font_name;
    quint16 font_size;
    tag(const QString &name,const QString &css,const QString &font_name,quint16 font_size):name(name),css(css),font_name(font_name),font_size(font_size)
    {
    }
};
extern QList<tag> tag_list;

QPixmap GetTag(QColor color,int size);
void SetLocale(const QString &sLocale);
void DoDonate();
QString Transliteration(QString str);
QString BuildFileName(QString filename);
void setProxy();
bool openDB(bool create,bool replace);
QString decodeStr(const QString &str);
bool SetCurrentZipFileName(QuaZip *zip,const QString &name);
QString RelativeToAbsolutePath(QString path);
QString sizeToString(uint size);

extern int idCurrentLib;



#endif // COMMON_H
