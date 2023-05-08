#ifndef COMMON_H
#define COMMON_H

#include "quazip/quazip/quazip.h"

#define AppName  QStringLiteral("freeLib")
#define OrgName  QStringLiteral("freeLibOrg")

QPixmap GetTag(QColor color,int size);
bool SetCurrentZipFileName(QuaZip *zip,const QString &name);
QString RelativeToAbsolutePath(QString path);

#endif // COMMON_H
