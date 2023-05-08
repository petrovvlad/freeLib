#ifndef COMMON_H
#define COMMON_H

#include <QPixmap>

#define AppName  QStringLiteral("freeLib")
#define OrgName  QStringLiteral("freeLibOrg")

QPixmap GetTag(QColor color,int size);
QString RelativeToAbsolutePath(QString path);

#endif // COMMON_H
