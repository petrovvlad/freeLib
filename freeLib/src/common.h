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

#define AppName  QString("freeLib")
#define OrgName  QString("freeLibOrg")
#define default_exp_file_name "%a/%s/%n3%b"
#define default_book_title "(%abbrs %n2) %b"
#define default_author_name "%nf %nm %nl"
#define default_cover_label "%abbrs - %n2"
#define default_OPDS_port   8080
#define default_proxy_port 8080

#define default_dropcaps_font "sangha.ttf"

extern QApplication *app;
extern QTranslator* translator;
extern QTranslator* translator_qt;
extern bool db_is_open;
extern QCommandLineParser CMDparser;

enum SendType{ST_Device,ST_Mail};
QSettings* GetSettings(bool need_copy=false, bool reopen=false);
SendType SetCurrentExportSettings(int index);

enum APP_MODE{MODE_LIBRARY,MODE_CONVERTER,MODE_SHELF};

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

struct genre_info
{
    genre_info(QString genre,qlonglong id):genre(genre),id(id)
    {
    }
    QString genre;
    qlonglong id;
};
struct author_info
{
    author_info(QString author,qlonglong id):author(author),id(id)
    {
        QStringList sl=author.split(",");
        if(sl.count()>0)
            lastname=sl[0].trimmed();
        if(sl.count()>1)
            firstname=sl[0].trimmed();
        if(sl.count()>2)
            middlename=sl[0].trimmed();
    }
    QString author;
    qlonglong id;
    QString firstname;
    QString middlename;
    QString lastname;
};

struct book_info
{
    qlonglong id;
    QString title;
    QString annotation;
    QList<author_info> authors;
    QList<genre_info> genres;
    QString language;
    QString seria;
    QString isbn;
    qlonglong id_seria;
    int num_in_seria;
    QString img;
    int star;
    book_info()
    {
        star=0;
        num_in_seria=0;
  //      num_in_seria=-1;
    }
//    book_info(qlonglong id,qlonglong id_seria):id(id),id_seria(id_seria)
//    {
//        num_in_seria=0;
//    }
};
struct tag
{
    QString name;
    QString css;
    QString font_name;
    quint16 font_size;
    tag(QString name,QString css,QString font_name,quint16 font_size):name(name),css(css),font_name(font_name),font_size(font_size)
    {
    }
};
extern QList<tag> tag_list;

QFileInfo GetBookFile(QBuffer &buffer, QBuffer &buffer_info, uint id_book, bool caption=false, QDateTime *file_data=nullptr);
void GetBookInfo(book_info &bi, const QByteArray &data, QString type, bool info_only=false, uint id_book=0);
QPixmap GetTag(QColor color,int size);
void SetLocale();
QString FindLocaleFile(QString locale,QString name,QString suffics);
void DoDonate();
QString Transliteration(QString str);
QString BuildFileName(QString filename);
void ResetToDefaultSettings();
void setProxy();
bool openDB(bool create,bool replace);
QStringList fillParams(QStringList str, book_info &bi,QFileInfo book_file=QFileInfo());
QString fillParams(QString str, book_info &bi,QFileInfo book_file=QFileInfo());
QString fillParams(QString str, QFileInfo book_file,QString seria_name,QString book_name,QString author,QString ser_num);
QStringList fillParams(QStringList str, QFileInfo book_file, QString seria_name, QString book_name, QString author, QString ser_num);
QString decodeStr(const QString &str);
bool SetCurrentZipFileName(QuaZip *zip,QString name);
QString RelativeToAbsolutePath(QString path);

extern int idCurrentLib;
extern SLib current_lib;
extern QMap<int,SLib> mLibs;
extern QMap <uint,SGenre> mGenre;

#endif // COMMON_H
