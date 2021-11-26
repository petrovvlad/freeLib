#ifndef UTILITES_H
#define UTILITES_H

#include <QString>

QString RelativeToAbsolutePath(QString path);
bool openDB(const QString &sName, bool create, bool replace);

#endif // UTILITES_H
