#include "utilites.h"

#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QSettings>
#include <QSqlDatabase>

#include "options.h"

QString RelativeToAbsolutePath(QString path)
{
    if(QDir(path).isRelative() && path.indexOf(QLatin1String("%"))<0 && !path.startsWith(QLatin1String("mtp:/")))
    {
        return QApplication::applicationDirPath() + QLatin1String("/") + path;
    }
    return path;
}

bool openDB(const QString &sName, bool create, bool replace)
{
    if(!QSqlDatabase::contains(sName))
        QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), sName);

    QString sAppDir,db_file;
    QSettings settings;

    QFileInfo fi(RelativeToAbsolutePath(options.sDatabasePath));
    if(fi.exists() && fi.isFile())
    {
        db_file = fi.canonicalFilePath();
    }
    else
    {
        sAppDir = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).constFirst();
        db_file = sAppDir + QLatin1String("/freeLib.sqlite");
        options.sDatabasePath = db_file;
        settings.setValue(QStringLiteral("database_path"), db_file);
    }
    QFile file(db_file);
    if(!file.exists() || replace)
    {
        if(replace)
        {
            QSqlDatabase dbase = QSqlDatabase::database(sName, true);
            dbase.close();
            if(!file.remove())
            {
                qDebug()<<("Can't remove old database");
                return false;
            }
        }
        if(!create && !replace)
        {
            return true;
        }
        QDir dir;
        dir.mkpath(QFileInfo(db_file).absolutePath());
        file.setFileName(QStringLiteral(":/freeLib.sqlite"));
        file.open(QFile::ReadOnly);
        QByteArray data = file.readAll();
        file.close();
        file.setFileName(db_file);
        file.open(QFile::WriteOnly);
        file.write(data);
        file.close();
    }
    QSqlDatabase dbase = QSqlDatabase::database(sName, false);
    dbase.setDatabaseName(db_file);
    if (!dbase.open())
    {
        qDebug() << ("Error connect! ")<<db_file;
        return false;
    }
    qDebug()<<"Open DB OK. "<<db_file;
    return true;
}
