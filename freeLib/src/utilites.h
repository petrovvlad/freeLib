#ifndef UTILITES_H
#define UTILITES_H

#include <QString>
#include <QSqlDatabase>

QString RelativeToAbsolutePath(QString path);
bool openDB(const QString &sName);
void ClearLib(const QSqlDatabase &dbase, qlonglong id_lib, bool delete_only);
QString Transliteration(QString str);
void setProxy();

#endif // UTILITES_H
