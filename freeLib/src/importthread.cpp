#define QT_USE_QSTRINGBUILDER
#include "importthread.h"

#include <QDomDocument>
#include <QByteArray>
#include <QBuffer>
#include <QStringBuilder>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDir>

#include "quazip/quazip/quazip.h"
#include "quazip/quazip/quazipfile.h"
#include "options.h"

QString RelativeToAbsolutePath(QString path);
bool SetCurrentZipFileName(QuaZip *zip,const QString &name);

void ClearLib(const QSqlDatabase &dbase, qlonglong id_lib, bool delete_only)
{
    QSqlQuery query(dbase);
    if(delete_only)
    {
        query.exec(QLatin1String("update book set deleted=1 where id_lib=") + QString::number(id_lib));
    }
    else
    {
        query.exec(QLatin1String("delete from book where id_lib=") + QString::number(id_lib));
        query.exec(QLatin1String("delete from author where id_lib=") + QString::number(id_lib));
        query.exec(QLatin1String("delete from seria where id_lib=") + QString::number(id_lib));
        query.exec(QLatin1String("delete from book_author where id_lib=") + QString::number(id_lib));
        query.exec(QLatin1String("delete from book_genre where id_lib=") + QString::number(id_lib));
        query.exec(QStringLiteral("VACUUM"));
    }
}

ImportThread::ImportThread(QObject *parent) :
    QObject(parent)
{
}

//1-  Авторы через двоеточие
//2-  жанр
//3-  Название
//4-  Серия
//5-  Номер книги в серии
//6-  файл
//7-  размер
//8-  иденификатор
//9-  удален
//10- формат файла
//11- дата добавления
//12- язык
//13- рейтинг (0-5)
//14- ключевые слова


qlonglong ImportThread::AddSeria(const QString &str, qlonglong libID, int tag)
{
    if(str.trimmed().isEmpty())
        return -1;
    QString name = str.trimmed();
    query->exec(QLatin1String("SELECT id FROM seria WHERE name='") + name + QLatin1String("' and id_lib=") + QString::number(libID));
    if(query->next())
    {
        qlonglong id = query->value(0).toLongLong();
        return id;
    }
    query->prepare(QStringLiteral("INSERT INTO seria(name,id_lib) values(:name,:id_lib)"));
    query->bindValue(QStringLiteral(":name"), name);
    query->bindValue(QStringLiteral(":id_lib"), libID);
    if(!query->exec())
        qDebug() << query->lastError().text();
    qlonglong id = query->lastInsertId().toLongLong();
    return id;
}

qlonglong ImportThread::addAuthor(const SAuthor &author, uint libID, uint idBook, bool first_author, int tag)
{
    QString sQuery = QStringLiteral("SELECT id FROM author WHERE id_lib=%1 and %2 and %3 and %4").arg(
                QString::number(libID),
                (author.sLastName.isEmpty() ?QStringLiteral("(name1 is null or name1=\"\")") :(QLatin1String("name1=\"") + author.sLastName + QLatin1String("\""))),
                (author.sFirstName.isEmpty() ?QStringLiteral("(name2 is null or name2=\"\")") :(QLatin1String("name2=\"") + author.sFirstName + QLatin1String("\""))),
                (author.sMiddleName.isEmpty() ?QStringLiteral("(name3 is null or name3=\"\")") :(QLatin1String("name3=\"") + author.sMiddleName + QLatin1String("\""))));
    query->prepare(sQuery);
    query->exec();
    qlonglong id = 0;
    if(query->next())
    {
        id = query->value(0).toLongLong();
    }
    if(id == 0)
    {
        query->prepare(QStringLiteral("INSERT INTO author(name1,name2,name3,id_lib) values(:name1,:name2,:name3,:id_lib)"));
        query->bindValue(QStringLiteral(":name1"), author.sLastName);
        query->bindValue(QStringLiteral(":name2"), author.sFirstName);
        query->bindValue(QStringLiteral(":name3"), author.sMiddleName);
        query->bindValue(QStringLiteral(":id_lib"), libID);
        if(!query->exec())
            qDebug() << query->lastError().text();
        id = query->lastInsertId().toLongLong();
    }
    else
    {
    }
    if(first_author)
        query->exec(QLatin1String("UPDATE book SET first_author_id=") + QString::number(id) + QLatin1String(" WHERE id=") + QString::number(idBook));
    if(!query->exec(QLatin1String("INSERT INTO book_author(id_book,id_author,id_lib) values(") + QString::number(idBook) +
                    QLatin1String(",") + QString::number(id) + QLatin1String(",") + QString::number(libID) + QLatin1String(")")))
        qDebug() << query->lastError().text();
    return id;
}

uint ImportThread::AddBook(qlonglong star, const QString &name, qlonglong id_seria, int num_in_seria, const QString &file,
             int size, int IDinLib, bool deleted, const QString &format, QDate date, const QString &language, const QString &keys, qlonglong id_lib, const QString &archive, int tag)
{
    query->prepare(QStringLiteral("INSERT INTO book(name,star,id_seria,num_in_seria,language,file,size,'deleted',date,keys,id_inlib,id_lib,format,archive) "
                   "values(:name,:star,:id_seria,:num_in_seria,:language,:file,:size,:deleted,:date,:keys,:id_inlib,:id_lib,:format,:archive)"));

    query->bindValue(QStringLiteral(":name"), name);
    query->bindValue(QStringLiteral(":star"), star);
    query->bindValue(QStringLiteral(":id_seria"), id_seria);
    query->bindValue(QStringLiteral(":num_in_seria"), num_in_seria);
    query->bindValue(QStringLiteral(":language"), language);
    query->bindValue(QStringLiteral(":file"), file);
    query->bindValue(QStringLiteral(":size"), size);
    query->bindValue(QStringLiteral(":deleted"), deleted);
    query->bindValue(QStringLiteral(":date"), date);
    query->bindValue(QStringLiteral(":keys"), keys);
    query->bindValue(QStringLiteral(":id_inlib"), IDinLib);
    query->bindValue(QStringLiteral(":id_lib"), id_lib);
    query->bindValue(QStringLiteral(":format"), format);
    query->bindValue(QStringLiteral(":archive"), archive);
    if(!query->exec())
        qDebug() << query->lastError().text();
    uint id = query->lastInsertId().toUInt();

    return id;
}

qlonglong ImportThread::AddGenre(qlonglong idBook, QString sGenre, qlonglong id_lib)
{
    qlonglong idGenre = 0;
    sGenre.replace(' ', '_');
    query->exec(QLatin1String("SELECT id FROM genre where keys LIKE '%") + sGenre.toLower() + QLatin1String(";%'"));
    if(query->next())
    {
        idGenre = query->value(0).toLongLong();
    }
    else
        qDebug() << "Неизвестный жанр: " + sGenre;
    query->exec(QStringLiteral("INSERT INTO book_genre(id_book,id_genre,id_lib) values(") +  QString::number(idBook) + QLatin1String(",") +
                QString::number(idGenre) + QLatin1String(",") + QString::number(id_lib) + QLatin1String(")"));
    query->exec(QStringLiteral("select last_insert_rowid()"));
    query->next();
    qlonglong id = query->value(0).toLongLong();
    return id;
}

void ImportThread::start(const QString &fileName, const QString &name, const QString &path, long ID, int update_type, bool save_only, bool firstAuthor, bool bWoDeleted)
{
    _fileName = RelativeToAbsolutePath(fileName);
    if(!QFileInfo::exists(_fileName) || !QFileInfo(_fileName).isFile())
    {
        _fileName = fileName;
    }
    _path = RelativeToAbsolutePath(path);
    _name = name;
    _update_type = update_type;
    existingID = ID;
    loop = true;
    _save_only = save_only;
    _firstAuthorOnly = firstAuthor;
    bWoDeleted_ = bWoDeleted;
}

void ImportThread::readFB2(const QByteArray& ba, QString file_name, QString arh_name, qint32 file_size)
{
    if(arh_name.isEmpty())
    {
        file_name = file_name.right(file_name.length()-_path.length());
        if(!file_name.isEmpty() && (file_name.at(0) == '/' || file_name.at(0) == '\\'))
                file_name = file_name.right(file_name.length()-1);
    }
    else
    {
        arh_name = arh_name.right(arh_name.length()-_path.length());
        if(!arh_name.isEmpty() && (arh_name.at(0) == '/' || arh_name.at(0) == '\\'))
                arh_name = arh_name.right(arh_name.length()-1);
    }
    QFileInfo fi(file_name);
    file_name = file_name.left(file_name.length()-fi.suffix().length()-1);
    query->exec(QStringLiteral("SELECT id FROM book where id_lib=%1 and file='%2' and archive='%3'").arg(QString::number(existingID), file_name, arh_name));
    if(query->next()) //если книга найдена, то просто снимаем пометку удаления
    {
        query->exec(QLatin1String("update book set deleted=0 where id=") + query->value(0).toString());
        return;
    }
    emit Message(tr("Book add:") + QLatin1String(" ") + file_name);

    QDomDocument doc;
    doc.setContent(ba);
    QDomElement title_info = doc.elementsByTagName(QStringLiteral("title-info")).at(0).toElement();
    QString sTitle = title_info.elementsByTagName(QStringLiteral("book-title")).at(0).toElement().text();
    QString sLanguage = title_info.elementsByTagName(QStringLiteral("lang")).at(0).toElement().text();
    QString sSeria = title_info.elementsByTagName(QStringLiteral("sequence")).at(0).attributes().namedItem(QStringLiteral("name")).toAttr().value().trimmed();
    uint numInSerial = title_info.elementsByTagName(QStringLiteral("sequence")).at(0).attributes().namedItem(QStringLiteral("number")).toAttr().value().trimmed().toUInt();

    QList<SAuthor> listAuthors;
    QDomNodeList authors = title_info.elementsByTagName(QStringLiteral("author"));
    for(int i=0; i<authors.count(); i++)
    {
        SAuthor author;
        author.sFirstName = authors.at(i).toElement().elementsByTagName(QStringLiteral("first-name")).at(0).toElement().text().trimmed();
        author.sLastName = authors.at(i).toElement().elementsByTagName(QStringLiteral("last-name")).at(0).toElement().text().trimmed();
        author.sMiddleName = authors.at(i).toElement().elementsByTagName(QStringLiteral("middle-name")).at(0).toElement().text().trimmed();
        listAuthors << author;
    }
    QStringList listGenres;
    QDomNodeList genre = title_info.elementsByTagName(QStringLiteral("genre"));
    for(int i=0; i<genre.count(); i++)
    {
        listGenres << genre.at(i).toElement().text().trimmed();
    }
    QDomElement publish_info = doc.elementsByTagName(QStringLiteral("publish-info")).at(0).toElement();
    QString sIsbn = publish_info.elementsByTagName(QStringLiteral("isbn")).at(0).toElement().text();

    qlonglong id_seria = AddSeria(sSeria, existingID, 0);
    qlonglong id_book = AddBook(0, sTitle, id_seria, numInSerial, file_name, (file_size == 0 ?ba.size() :file_size), 0, false, fi.suffix(), QDate::currentDate(),
                                sLanguage, QLatin1String(""), existingID, arh_name, 0);

    bool first_author = true;
    foreach(const SAuthor &author, listAuthors)
    {
        addAuthor(author, existingID, id_book, first_author, 0);
        first_author = false;
        if(_firstAuthorOnly)
            break;
    }
    foreach(const auto sGenre, listGenres)
        AddGenre(id_book, sGenre, existingID);
}

void ImportThread::readEPUB(const QByteArray &ba, QString file_name, QString arh_name, qint32 file_size)
{
    if(arh_name.isEmpty())
    {
        file_name = file_name.right(file_name.length() - _path.length());
        if(file_name.at(0) == '/' || file_name.at(0) == '\\')
                file_name=file_name.right(file_name.length()-1);
    }
    else
    {
        arh_name = arh_name.right(arh_name.length() - _path.length());
        if(arh_name.at(0) == '/' || arh_name.at(0) == '\\')
                arh_name=arh_name.right(arh_name.length()-1);
    }
    file_name = file_name.left(file_name.length()-5);
    query->exec(QStringLiteral("SELECT id FROM book where id_lib=%1 and file='%2' and archive='%3'").arg(QString::number(existingID), file_name, arh_name));
    if(query->next()) //если книга найдена, то просто снимаем пометку удаления
    {
        query->exec(QStringLiteral("update book set deleted=0 where id=") + query->value(0).toString());
        return;
    }
    emit Message(tr("Book add: ") + file_name);

    QBuffer buf;
    buf.setData(ba);
    QuaZip zip(&buf);
    zip.open(QuaZip::mdUnzip);
    QBuffer info;
    SetCurrentZipFileName(&zip, QStringLiteral("META-INF/container.xml"));
    QuaZipFile zip_file(&zip);
    zip_file.open(QIODevice::ReadOnly);
    info.setData(zip_file.readAll());
    zip_file.close();
    QDomDocument doc;
    doc.setContent(info.data());
    QDomNode root = doc.documentElement();
    bool need_loop = true;
    QString rel_path;
    QString sTitle, sLanguage;
    QList<SAuthor> listAuthors;
    QStringList listGenres;

    for(int i=0; i<root.childNodes().count() && need_loop; i++)
    {
        if(root.childNodes().at(i).nodeName().toLower() == QLatin1String("rootfiles"))
        {
            QDomNode roots = root.childNodes().at(i);
            for(int j=0; j<roots.childNodes().count() && need_loop; j++)
            {
                if(roots.childNodes().at(j).nodeName().toLower() == QLatin1String("rootfile"))
                {
                    QString path = roots.childNodes().at(j).attributes().namedItem(QStringLiteral("full-path")).toAttr().value();
                    QBuffer opf_buf;
                    QFileInfo fi(path);
                    rel_path = fi.path();
                    SetCurrentZipFileName(&zip,path);
                    zip_file.open(QIODevice::ReadOnly);
                    opf_buf.setData(zip_file.readAll());
                    zip_file.close();

                    QDomDocument opf;
                    opf.setContent(opf_buf.data());
                    QDomNode meta = opf.documentElement().namedItem(QStringLiteral("metadata"));
                    for(int m=0; m<meta.childNodes().count(); m++)
                    {
                        if(meta.childNodes().at(m).nodeName().right(5) == QLatin1String("title"))
                        {
                            sTitle = meta.childNodes().at(m).toElement().text().trimmed();
                        }
                        else if(meta.childNodes().at(m).nodeName().right(8) == QLatin1String("language"))
                        {
                            sLanguage = meta.childNodes().at(m).toElement().text().trimmed();
                        }
                        else if(meta.childNodes().at(m).nodeName().right(7) == QLatin1String("creator"))
                        {
                            SAuthor author;
                            QStringList names = meta.childNodes().at(m).toElement().text().trimmed().split(QStringLiteral(" "));
                            if(names.count() > 0)
                                author.sFirstName = names.at(0);
                            if(names.count() > 1)
                                author.sMiddleName = names.at(1);
                            if(names.count() > 2)
                                author.sLastName = names.at(2);
                            listAuthors << author;
                        }
                        else if(meta.childNodes().at(m).nodeName().right(7) == QLatin1String("subject"))
                        {
                            listGenres << meta.childNodes().at(m).toElement().text().trimmed();
                        }
                    }
                    need_loop = false;
                }
            }
        }
    }
    zip.close();
    buf.close();
    info.close();

    if(sTitle.isEmpty() || listAuthors.count() == 0)
    {
        return;
    }

    qlonglong id_book = AddBook(0, sTitle, 0, 0, file_name, (file_size == 0 ?ba.size() :file_size), 0, false, QStringLiteral("epub"), QDate::currentDate(),
                                sLanguage, QLatin1String(""), existingID, arh_name, 0);

    bool first_author = true;
    foreach(const SAuthor &author, listAuthors)
    {
        addAuthor(author, existingID, id_book, first_author, 0);
        first_author = false;
    }
    foreach(const auto sGenre, listGenres)
        AddGenre(id_book, sGenre, existingID);
}

void ImportThread::importFB2_main(const QString &path)
{
    int count = 0;
    importFB2(path, count);
    if(count>0)
        query->exec(QStringLiteral("COMMIT;"));
}

void ImportThread::importFB2(const QString &path, int &count)
{
    QDir dir(path);
    QFileInfoList info_list = dir.entryInfoList(QDir::NoSymLinks|QDir::NoDotAndDotDot|QDir::Readable|QDir::Files|QDir::Dirs);
    QList<QFileInfo>::iterator iter = info_list.begin();
    QString file_name;
    for(iter=info_list.begin();iter != info_list.end() && loop;iter++)
    {
        QCoreApplication::processEvents();
        file_name = iter->absoluteFilePath();
        if(iter->isDir())
        {
            importFB2(file_name, count);
        }
        else
        {
            if(iter->suffix().toLower() != QLatin1String("fbd") &&
                    !(iter->suffix().toLower() == QLatin1String("zip") ||
                     iter->suffix().toLower() == QLatin1String("fb2") ||
                     iter->suffix().toLower() == QLatin1String("epub")))
            {
                QString fbd = iter->absolutePath() + QLatin1String("/") + iter->completeBaseName() + QLatin1String(".fbd");
                QFile file(fbd);
                if(file.exists())
                {
                    file.open(QFile::ReadOnly);
                    readFB2(file.readAll(), file_name, QLatin1String(""), iter->size());
                    count++;
                }
            }
            else if(iter->suffix().toLower() == QLatin1String("fb2") || iter->suffix().toLower() == QLatin1String("epub"))
            {
                if(count == 0)
                    query->exec(QStringLiteral("BEGIN;"));
                QFile file(file_name);
                file.open(QFile::ReadOnly);
                QByteArray ba = file.readAll();
                if(iter->suffix().toLower() == QLatin1String("fb2"))
                    readFB2(ba, file_name, QLatin1String(""));
                else
                    readEPUB(ba, file_name, QLatin1String(""));
                count++;
            }
            else if(iter->suffix().toLower() == QLatin1String("zip"))
            {
                if(_update_type == UT_NEW)
                {
                    emit Message(iter->fileName());
                    QString arh_name = file_name.right(file_name.length() - _path.length());
                    if(arh_name.at(0) == '/' || arh_name.at(0) == '\\')
                            arh_name=arh_name.right(arh_name.length()-1);
                    query->exec(QStringLiteral("SELECT * FROM book where archive='%1' LIMIT 1").arg(arh_name));
                    if(query->next())
                        continue;
                }
                QuaZip uz(file_name);
                uz.setFileNameCodec(QTextCodec::codecForName("IBM 866"));
                if (!uz.open(QuaZip::mdUnzip))
                {
                    qDebug()<<("Error open archive!")<<" "<<file_name;
                    continue;
                }

                QuaZipFile zip_file(&uz);
                QList<QuaZipFileInfo64> list = uz.getFileInfoList64();
                foreach(const QuaZipFileInfo64 &str, list  )
                {
                    //app->processEvents();
                    QCoreApplication::processEvents();
                    if(!loop)
                        break;
                    QBuffer buffer;
                    if(count == 0)
                        query->exec(QStringLiteral("BEGIN;"));
                    QuaZipFileInfo zip_fi;
                    str.toQuaZipFileInfo(zip_fi);
                    if(zip_fi.name.right(3).toLower() == QLatin1String("fb2"))
                    {
                        //uz.extractFile(str.filename,&buffer,UnZip::SkipPaths,16*1024);
                        SetCurrentZipFileName(&uz, zip_fi.name);
                        zip_file.open(QIODevice::ReadOnly);
                        buffer.setData(zip_file.read(16*1024));
                        zip_file.close();
                        readFB2(buffer.data(), str.name, file_name, str.uncompressedSize);
                    }
                    else if(zip_fi.name.right(3).toLower() == QLatin1String("epub"))
                    {
                        SetCurrentZipFileName(&uz, zip_fi.name);
                        zip_file.open(QIODevice::ReadOnly);
                        buffer.setData(zip_file.readAll());
                        readEPUB(buffer.data(), str.name, file_name, str.uncompressedSize);
                    }
                    else if(zip_fi.name.right(3).toLower() != QLatin1String("fbd"))
                    {
                        QFileInfo fi(str.name);
                        if(!fi.completeBaseName().isEmpty() && fi.completeBaseName().at(0) != '.')
                        {
                            QString fbd = fi.path() + QLatin1String("/") + fi.completeBaseName() + QLatin1String(".fbd");
                            if(SetCurrentZipFileName(&uz, fbd))
                            {
                                SetCurrentZipFileName(&uz, zip_fi.name);
                                zip_file.open(QIODevice::ReadOnly);
                                buffer.setData(zip_file.readAll());
                                readFB2(buffer.data(), str.name, file_name, str.uncompressedSize);
                            }
                        }
                    }

                    count++;
                    if(count == 1000)
                    {
                        query->exec(QStringLiteral("COMMIT;"));
                        count = 0;
                    }
                }
            }
            if(count == 1000)
            {
                query->exec(QStringLiteral("COMMIT;"));
                count = 0;
            }
        }
    }
}

void ImportThread::process()
{
    if(_save_only)
    {
        emit End();
        return;
    }
    if(_name.isEmpty())
    {
        emit Message(tr("Empty library name"));
        emit End();
        return;
    }
    QFileInfo fi(RelativeToAbsolutePath(options.sDatabasePath));
    QString sDbFile = fi.canonicalFilePath();
    QSqlDatabase dbase = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("importdb"));
    dbase.setDatabaseName(sDbFile);
    if (!dbase.open())
    {
        qDebug() << ("Error connect! ")<<sDbFile;
        return;
    }

    query=new QSqlQuery(dbase);

    switch(_update_type)
    {
    case UT_FULL:
        ClearLib(dbase, existingID, false);
        break;
    case UT_DEL_AND_NEW:
        ClearLib(dbase, existingID, true);
        break;
    }

    if(_fileName.isEmpty())
    {
        importFB2_main(_path);
        query->exec(QStringLiteral("drop table if exists tmp;"));
        query->exec(QStringLiteral("create table tmp as select id from book where id_lib=%1 and deleted=1;").arg(QString::number(existingID)));
        query->exec(QStringLiteral("delete from book where id_lib=%1 and id in (select id from tmp);").arg(QString::number(existingID)));
        query->exec(QStringLiteral("delete from book_genre where id_lib=%1 and id_book in (select id from tmp);").arg(QString::number(existingID)));
        query->exec(QStringLiteral("delete from book_author where id_lib=%1 and id_book in (select id from tmp);").arg(QString::number(existingID)));
        query->exec(QStringLiteral("drop table if exists tmp;"));
        query->exec(QStringLiteral("VACUUM"));
        emit End();
        return;
    }
    QuaZip archiveFile;
    QuaZip uz(_fileName);
    if (!uz.open(QuaZip::mdUnzip))
    {
        qDebug()<<("Error open inpx!");
        emit End();
        return;
    }
    QStringList list = uz.getFileNameList();
    qlonglong book_count = 0;
#define _NAME           0
#define _SERIA          1
#define _NUM_IN_SERIA   2
#define _FILE           3
#define _SIZE           4
#define _ID_IN_LIB      5
#define _DELETED        6
#define _FORMAT         7
#define _DATE           8
#define _LANGUAGE       9
#define _STAR           10
#define _KEYS           11
#define _AUTHORS        12
#define _JANRES         13
#define _FOLDER         14
#define _TAG            15
#define _TAG_SERIA      16
#define _TAG_AUTHOR     17

    int field_index[20];
    field_index[_NAME]          =2; //name
    field_index[_SERIA]         =3; //seria
    field_index[_NUM_IN_SERIA]  =4; //num_in_seria
    field_index[_FILE]          =5; //file
    field_index[_SIZE]          =6; //size
    field_index[_ID_IN_LIB]     =7; //id_in_lib
    field_index[_DELETED]       =8; //deleted
    field_index[_FORMAT]        =9; //format
    field_index[_DATE]          =10;//date
    field_index[_LANGUAGE]      =11;//language
    field_index[_STAR]          =12;//star
    field_index[_KEYS]          =13;//keys
    field_index[_AUTHORS]       =0; //Authors
    field_index[_JANRES]        =1; //Janres
    field_index[_FOLDER]        =-1; //Folder
    field_index[_TAG]           =-1; //Tag
    field_index[_TAG_SERIA]     =-1; //TagSeria
    field_index[_TAG_AUTHOR]    =-1; //TagAuthor

    QuaZipFile zip_file(&uz);
    foreach(const QString &str , list  )
    {
        if(QString(str).toUpper() == QLatin1String("STRUCTURE.INFO"))
        {
            for(unsigned int i=0; i<sizeof(field_index)/sizeof(int); i++)
                field_index[i] = -1;
            QBuffer outbuff;
            SetCurrentZipFileName(&uz, str);
            zip_file.open(QIODevice::ReadOnly);
            outbuff.setData(zip_file.readAll());
            zip_file.close();
            QStringList lines = (QString::fromUtf8(outbuff.data())).split(QStringLiteral("\n"));
            foreach(const QString &line, lines)
            {
                QStringList substrings = line.toUpper().split(QStringLiteral(";"));
                int i = 0;
                foreach(const QString &substring, substrings)
                {
                    if(substring == QLatin1String("TITLE"))
                        field_index[_NAME] = i;
                    else if(substring == QLatin1String("SERIES"))
                        field_index[_SERIA] = i;
                    else if(substring == QLatin1String("SERNO"))
                        field_index[_NUM_IN_SERIA] = i;
                    else if(substring == QLatin1String("FILE"))
                        field_index[_FILE] = i;
                    else if(substring == QLatin1String("SIZE"))
                        field_index[_SIZE] = i;
                    else if(substring == QLatin1String("LIBID"))
                        field_index[_ID_IN_LIB] = i;
                    else if(substring == QLatin1String("DEL"))
                        field_index[_DELETED] = i;
                    else if(substring == QLatin1String("EXT"))
                        field_index[_FORMAT] = i;
                    else if(substring == QLatin1String("DATE"))
                        field_index[_DATE] = i;
                    else if(substring == QLatin1String("LANG"))
                        field_index[_LANGUAGE] = i;
                    else if(substring == QLatin1String("STARS"))
                        field_index[_STAR] = i;
                    else if(substring == QLatin1String("KEYWORDS"))
                        field_index[_KEYS] = i;
                    else if(substring == QLatin1String("AUTHOR"))
                        field_index[_AUTHORS] = i;
                    else if(substring == QLatin1String("GENRE"))
                        field_index[_JANRES] = i;
                    else if(substring == QLatin1String("FOLDER"))
                        field_index[_FOLDER] = i;
                    else if(substring == QLatin1String("TAG"))
                        field_index[_TAG] = i;
                    else if(substring == QLatin1String("TAGSERIES"))
                        field_index[_TAG_SERIA] = i;
                    else if(substring == QLatin1String("TAGAUTHOR"))
                        field_index[_TAG_AUTHOR] = i;
                  i++;
                }

            }
            break;
        }
    }
    dbase.transaction();

    foreach(const QString &str , list  )
    {
        QCoreApplication::processEvents();
        if(!loop)
        {
            break;
        }
        if(str.right(3).toUpper() != QLatin1String("INP"))
            continue;
        emit Message(str);
        if(_update_type == UT_NEW)
        {
            query->exec(QStringLiteral("SELECT * FROM book where archive='%1' and id_lib=%2 LIMIT 1").arg(str, QString::number(existingID)));
            if(query->next())
                continue;
        }
        QBuffer outbuff;
        SetCurrentZipFileName(&uz, str);
        zip_file.open(QIODevice::ReadOnly);
        outbuff.setData(zip_file.readAll());
        zip_file.close();
        //uz.extractFile(str,&outbuff,UnZip::SkipPaths);
        QStringList lines = (QString::fromUtf8(outbuff.data())).split('\n');
        qlonglong count=0;
        foreach(const QString &line, lines)
        {
            if(line.isEmpty())
                continue;

            QCoreApplication::processEvents();
            if(!loop)
            {
                break;
            }
            QStringList substrings = line.split(QChar(4));
            if(substrings.count() == 0)
                continue;
            QString name;
            if(substrings.count() > field_index[_NAME])
            {
                name = substrings[field_index[_NAME]].trimmed();
            }
            QString Seria;
            if(substrings.count() > field_index[_SERIA])
                Seria = substrings[field_index[_SERIA]].trimmed();
            int num_in_seria = 0;
            if(substrings.count() > field_index[_NUM_IN_SERIA])
            {
                num_in_seria = substrings[field_index[_NUM_IN_SERIA]].trimmed().toInt();
            }
            QString file;
            if(substrings.count() > field_index[_FILE])
            {
                file = substrings[field_index[_FILE]].trimmed();
            }
            int size = 0;
            if(substrings.count() > field_index[_SIZE])
            {
                size = substrings[field_index[_SIZE]].trimmed().toInt();
            }
            int id_in_lib = 0;
            if(substrings.count() > field_index[_ID_IN_LIB])
            {
                id_in_lib = substrings[field_index[_ID_IN_LIB]].trimmed().toInt();
            }
            bool deleted = 0;
            if(substrings.count()>field_index[_DELETED])
            {
                deleted = (substrings[field_index[_DELETED]].trimmed().toInt() > 0);
            }
            QString format;
            if(substrings.count() > field_index[_FORMAT])
            {
                format = substrings[field_index[_FORMAT]].trimmed();
            }

            QDate date;
            if(substrings.count() > field_index[_DATE])
            {
                date = QDate::fromString(substrings[field_index[_DATE]].trimmed(), QStringLiteral("yyyy-MM-dd"));
            }
            QString language;
            if(substrings.count() > field_index[_LANGUAGE])
            {
                language = substrings[field_index[_LANGUAGE]].trimmed();
            }
            qlonglong star = 0;
            if(substrings.count() > field_index[_STAR] && field_index[_STAR] >= 0)
            {
                star = substrings[field_index[_STAR]].trimmed().toInt();
            }
            QString keys;
            if(substrings.count() > field_index[_KEYS])
            {
                keys = substrings[field_index[_KEYS]].trimmed();
            }
            QString folder = str;
            if(substrings.count() > field_index[_FOLDER] && field_index[_FOLDER] >= 0)
            {
                folder = substrings[field_index[_FOLDER]].trimmed();
            }
            int tag = 0;
            if(substrings.count() > field_index[_TAG] && field_index[_TAG] >= 0)
            {
                tag = substrings[field_index[_TAG]].trimmed().toInt();
            }
            int tag_seria = 0;
            if(substrings.count() > field_index[_TAG_SERIA] && field_index[_TAG_SERIA] >= 0)
            {
                tag_seria = substrings[field_index[_TAG_SERIA]].trimmed().toInt();
            }
            QStringList tag_author;
            if(substrings.count() > field_index[_TAG_AUTHOR] && field_index[_TAG_AUTHOR] >= 0)
            {
                tag_author = substrings[field_index[_TAG_AUTHOR]].trimmed().split(':');
            }
            qlonglong id_seria = 0;
            if(!Seria.isEmpty())
                id_seria = AddSeria(Seria,existingID,tag_seria);

            qlonglong id_book;
            if(!bWoDeleted_ || !deleted){
                id_book = AddBook(star, name, id_seria, num_in_seria, file, size, id_in_lib, deleted, format, date, language, keys, existingID, folder, tag);

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
                QStringList Authors = substrings[field_index[_AUTHORS]].split(':', Qt::SkipEmptyParts);
#else
                QStringList Authors = substrings[field_index[_AUTHORS]].split(':');
#endif
                int author_count = 0;
                foreach(const QString &sAuthor, Authors)
                {
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
                    if(sAuthor.isEmpty())
                        continue;
#endif
                    int tag_auth = 0;
                    if(author_count < tag_author.count())
                    {
                        if(!tag_author[author_count].trimmed().isEmpty())
                            tag_auth = tag_author[author_count].trimmed().toInt();
                    }
                    QString sAuthorLow = sAuthor.toLower();
                    if(format == QLatin1String("fb2") && sAuthorLow.contains(QStringLiteral("автор")) && (sAuthorLow.contains(QStringLiteral("неизвестен")) || sAuthorLow.contains(QStringLiteral("неизвестный"))))
                    {
                        QString sFile, sArchive;
                        QBuffer buffer;
                        sFile = QStringLiteral("%1.%2").arg(file, format);
                        sArchive = QStringLiteral("%1/%2").arg(_path, folder.replace(QLatin1String(".inp"), QLatin1String(".zip")));
                        if(archiveFile.getZipName() != sArchive){
                            archiveFile.close();
                            archiveFile.setZipName(sArchive);
                            if (!archiveFile.open(QuaZip::mdUnzip))
                                qDebug()<<("Error open archive!")<<" "<<sArchive;
                        }
                        QuaZipFile zip_file(&archiveFile);
                        SetCurrentZipFileName(&archiveFile, sFile);
                        if(!zip_file.open(QIODevice::ReadOnly))
                            qDebug()<<"Error open file: "<<sFile;
                        buffer.setData(zip_file.read(16*1024));
                        zip_file.close();

                        QDomDocument doc;
                        doc.setContent(buffer.data());
                        QDomElement title_info = doc.elementsByTagName(QStringLiteral("title-info")).at(0).toElement();
                        QDomNodeList listAuthor = title_info.elementsByTagName(QStringLiteral("author"));

                        for(int i=0; i<listAuthor.count(); i++)
                        {
                            SAuthor author;
                            author.sFirstName = listAuthor.at(i).toElement().elementsByTagName(QStringLiteral("first-name")).at(0).toElement().text();
                            author.sLastName = listAuthor.at(i).toElement().elementsByTagName(QStringLiteral("last-name")).at(0).toElement().text();
                            author.sMiddleName = listAuthor.at(i).toElement().elementsByTagName(QStringLiteral("middle-name")).at(0).toElement().text();
                            sAuthorLow = author.getName().toLower();
                            if(!sAuthorLow.contains(QStringLiteral("неизвестен")) && !sAuthorLow.contains(QStringLiteral("неизвестный"))){
                                //QString sAuthor = QString("%1,%2,%3").arg(lastname,firstname,middlename);
                                addAuthor(author, existingID, id_book, author_count == 0, tag_auth);
                                author_count++;
                            }
                        }
                    }else{
                        SAuthor author(sAuthor);
                        addAuthor(author, existingID, id_book, author_count == 0, tag_auth);
                        author_count++;
                    }
                }

                if(substrings.count() >= field_index[_JANRES] + 1)
                {
                    QStringList Janres = substrings[field_index[_JANRES]].split(QStringLiteral(":"));
                    bool first = true;
                    foreach(const QString &janre, Janres)
                    {
                        if(!first && janre.trimmed().isEmpty())
                            continue;
                        AddGenre(id_book, janre.trimmed(), existingID);
                        first = false;
                    }
                }
                count++;
                book_count++;
                if(count == 1000)
                {
                    emit Message(tr("Books adds:") + QLatin1String(" ") + QString::number(book_count));
                    count = 0;
                }

            }
        }
        if(count > 0)
        {
            emit Message(tr("Books adds: ") + QString::number(book_count));
        }
    }
    dbase.commit();
    emit End();
    dbase.close();
}

void ImportThread::break_import()
{
    loop = false;
}

