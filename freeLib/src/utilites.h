#ifndef UTILITES_H
#define UTILITES_H

#include <QString>
#include <QSqlDatabase>
#include <QSettings>

QString RelativeToAbsolutePath(QString path);
bool openDB(const QString &sName);
void ClearLib(const QSqlDatabase &dbase, qlonglong id_lib, bool delete_only);
QString Transliteration(QString str);
void setProxy();
QSharedPointer<QSettings> GetSettings(bool bReopen = false);

#endif // UTILITES_H
