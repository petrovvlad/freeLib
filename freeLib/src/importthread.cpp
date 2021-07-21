#include <QDomDocument>
#include <QByteArray>
#include <QBuffer>

#include "importthread.h"
#include "quazip/quazip/quazip.h"
#include "quazip/quazip/quazipfile.h"
#include "common.h"

void ClearLib(QSqlDatabase dbase, qlonglong id_lib, bool delete_only)
{
    QSqlQuery query(dbase);
    if(delete_only)
    {
        query.exec("update book set deleted=1 where id_lib="+QString::number(id_lib));
    }
    else
    {
        query.exec("delete from book where id_lib="+QString::number(id_lib));
        query.exec("delete from author where id_lib="+QString::number(id_lib));
        query.exec("delete from seria where id_lib="+QString::number(id_lib));
        query.exec("delete from book_author where id_lib="+QString::number(id_lib));
        query.exec("delete from book_janre where id_lib="+QString::number(id_lib));
        query.exec("VACUUM");
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



qlonglong ImportThread::AddSeria(QString str, qlonglong libID, int tag)
{
    if(str.trimmed().isEmpty())
        return -1;
    QString name=str.trimmed();
    query->exec("SELECT id FROM seria WHERE name='"+name+"' and id_lib="+QString::number(libID));
    if(query->next())
    {
        qlonglong id=query->value(0).toLongLong();
        return id;
    }
    query->prepare("INSERT INTO seria(name,id_lib,favorite) values(:name,:id_lib,:favorite)");
    query->bindValue(":name",name);
    query->bindValue(":id_lib",libID);
    query->bindValue(":favorite",tag);
    if(!query->exec())
        qDebug() << query->lastError().text();
    qlonglong id = query->lastInsertId().toLongLong();
    return id;
}

qlonglong ImportThread::AddAuthor(QString str, qlonglong libID, qlonglong id_book, bool first_author, QString language, int tag)
{
    if(str.trimmed().isEmpty())
        return -1;
    QStringList names=str.split(',');
    QString name1;
    if(names.count()>0)
        name1=names[0].trimmed();
    QString name2;
    if(names.count()>1)
        name2=names[1].trimmed();
    QString name3;
    if(names.count()>2)
        name3=names[2].trimmed();
    if(name1.isEmpty() && name2.isEmpty() && name3.isEmpty())
        return -1;

    query->prepare("SELECT id,favorite FROM author WHERE id_lib=:id_lib and name1=:name1 and name2=:name2 and name3=:name3");
    query->bindValue(":id_lib",libID);
    query->bindValue(":name1",name1);
    query->bindValue(":name2",name2);
    query->bindValue(":name3",name3);
    query->exec();
    qlonglong id=0;
    if(query->next())
    {
        id=query->value(0).toLongLong();
    }
    if(id==0)
    {
        query->prepare("INSERT INTO author(name1,name2,name3,id_lib,favorite) values(:name1,:name2,:name3,:id_lib,:favorite)");
        query->bindValue(":name1",name1);
        query->bindValue(":name2",name2);
        query->bindValue(":name3",name3);
        query->bindValue(":id_lib",libID);
        query->bindValue(":favorite",tag);
        if(!query->exec())
            qDebug() << query->lastError().text();
        id = query->lastInsertId().toLongLong();
    }
    else
    {
        if(query->value(1).toInt()!=tag && tag>0)
            query->exec("UPDATE author SET favorite="+QString::number(tag) +" WHERE id="+QString::number(id));
    }
    if(first_author)
        query->exec("UPDATE book SET first_author_id="+QString::number(id)+" WHERE id="+QString::number(id_book));
    query->exec("INSERT INTO book_author(id_book,id_author,id_lib,language) values("+QString::number(id_book)+","+QString::number(id)+","+QString::number(libID)+",'"+language+"')");
    return id;
}

qlonglong ImportThread::addAuthor(SAuthor author, uint libID, uint idBook, bool first_author, QString language, int tag)
{
    QString sQuery = QString("SELECT id,favorite FROM author WHERE id_lib=%1 and %2 and %3 and %4").arg(
                QString::number(libID),
                (author.sLastName.isEmpty() ?"name1 is null or name1=\"\"" :"name1=\""+author.sLastName+"\""),
                (author.sFirstName.isEmpty() ?"name2 is null or name2=\"\"" :"name2=\""+author.sFirstName+"\""),
                (author.sMiddleName.isEmpty() ?"name3 is null or name3=\"\"" :"name3=\""+author.sMiddleName+"\""));
    query->prepare(sQuery);
    query->exec();
    qlonglong id=0;
    if(query->next())
    {
        id=query->value(0).toLongLong();
    }
    if(id==0)
    {
        query->prepare("INSERT INTO author(name1,name2,name3,id_lib,favorite) values(:name1,:name2,:name3,:id_lib,:favorite)");
        query->bindValue(":name1",author.sLastName);
        query->bindValue(":name2",author.sFirstName);
        query->bindValue(":name3",author.sMiddleName);
        query->bindValue(":id_lib",libID);
        query->bindValue(":favorite",tag);
        if(!query->exec())
            qDebug() << query->lastError().text();
        id = query->lastInsertId().toLongLong();
    }
    else
    {
        if(query->value(1).toInt()!=tag && tag>0)
            query->exec("UPDATE author SET favorite="+QString::number(tag) +" WHERE id="+QString::number(id));
    }
    if(first_author)
        query->exec("UPDATE book SET first_author_id=" +QString::number(id)+" WHERE id="+QString::number(idBook));
    if(!query->exec("INSERT INTO book_author(id_book,id_author,id_lib,language) values("+QString::number(idBook)+","+QString::number(id)+","+QString::number(libID)+",'"+language+"')"))
        qDebug() << query->lastError().text();
    return id;
}

qlonglong ImportThread::AddBook(qlonglong star, QString name, qlonglong id_seria, int num_in_seria, QString file,
             int size, int IDinLib, bool deleted, QString format, QDate date, QString language, QString keys, qlonglong id_lib, QString archive, int tag)
{
    query->prepare("INSERT INTO book(name,star,id_seria,num_in_seria,language,file,size,'deleted',date,keys,id_inlib,id_lib,format,archive,favorite) "
                   "values(:name,:star,:id_seria,:num_in_seria,:language,:file,:size,:deleted,:date,:keys,:id_inlib,:id_lib,:format,:archive,:favorite)");

    query->bindValue(":name",name);
    query->bindValue(":star",star);
    query->bindValue(":id_seria",id_seria);
    query->bindValue(":num_in_seria",num_in_seria);
    query->bindValue(":language",language);
    query->bindValue(":file",file);
    query->bindValue(":size",size);
    query->bindValue(":deleted",deleted);
    query->bindValue(":date",date);
    query->bindValue(":keys",keys);
    query->bindValue(":id_inlib",IDinLib);
    query->bindValue(":id_lib",id_lib);
    query->bindValue(":format",format);
    query->bindValue(":archive",archive);
    query->bindValue(":favorite",tag);
    if(!query->exec())
        qDebug() << query->lastError().text();
    qlonglong id = query->lastInsertId().toLongLong();

    return id;
}
qlonglong ImportThread::AddGenre(qlonglong id_book,QString janre,qlonglong id_lib,QString language)
{
    qlonglong id_janre=0;
    janre.replace(" ","_");
    query->exec("SELECT id,main_janre FROM janre where keys LIKE '%"+janre.toLower()+";%'");
    if(query->next())
    {
        id_janre=query->value(0).toLongLong();
    }
    else
        qDebug()<<"Неизвестный жанр: "+janre;
    query->exec("INSERT INTO book_janre(id_book,id_janre,id_lib,language) values("+QString::number(id_book)+","+QString::number(id_janre)+","+QString::number(id_lib)+",'"+language+"')");
    query->exec("select last_insert_rowid()");
    query->next();
    qlonglong id=query->value(0).toLongLong();
    return id;
}

void ImportThread::start(QString fileName, QString name, QString path, long ID, int update_type, bool save_only, bool firstAuthor, bool bWoDeleted)
{
    _fileName=RelativeToAbsolutePath(fileName);
    if(!QFileInfo::exists(_fileName) || !QFileInfo(_fileName).isFile())
    {
        _fileName=fileName;
    }
    _path=RelativeToAbsolutePath(path);
    _name=name;
    _update_type=update_type;
    existingID=ID;
    loop=true;
    _save_only=save_only;
    _firstAuthorOnly=firstAuthor;
    bWoDeleted_ = bWoDeleted;
}

/*void ImportThread::readFB2_test(const QByteArray& ba,QString file_name,QString arh_name)
{
    return;
    if(arh_name.isEmpty())
    {
        file_name=file_name.right(file_name.length()-_path.length());
        if(file_name.left(1)=="/" || file_name.left(1)=="\\")
                file_name=file_name.right(file_name.length()-1);
    }
    else
    {
        arh_name=arh_name.right(arh_name.length()-_path.length());
        if(arh_name.left(1)=="/" || arh_name.left(1)=="\\")
                arh_name=arh_name.right(arh_name.length()-1);
    }
    file_name=file_name.left(file_name.length()-4);
    if(query->next()) //если книга найдена, то просто снимаем пометку удаления
    {
        query->exec("update book set deleted=0 where id="+query->value(0).toString());
        return;
    }
    QString title;
    title="test";//title_info.elementsByTagName("book-title").at(0).toElement().text().trimmed();
    QString language="ru";//title_info.elementsByTagName("lang").at(0).toElement().text();
    QString seria="";//title_info.elementsByTagName("sequence").at(0).attributes().namedItem("name").toAttr().value().trimmed();
    QString num_seria="0";//title_info.elementsByTagName("sequence").at(0).attributes().namedItem("number").toAttr().value().trimmed();


    qlonglong id_seria=AddSeria(seria,existingID,0);
    qlonglong id_book=AddBook(0,title,id_seria,num_seria.toInt(),file_name,ba.size(),0,false,"fb2",QDate::currentDate(),language,"",existingID,arh_name,0);
    return;
    bool first_author=true;
    AddAuthor("Иванов, Иван,Иванович ",
              existingID,id_book,first_author,language,0);
    first_author=false;
    AddGenre(id_book,"sf",existingID,language);
}*/

void ImportThread::readFB2(const QByteArray& ba, QString file_name, QString arh_name, qint32 file_size)
{
    if(arh_name.isEmpty())
    {
        file_name=file_name.right(file_name.length()-_path.length());
        if(!file_name.isEmpty() && (file_name.at(0)=="/" || file_name.at(0)=="\\"))
                file_name=file_name.right(file_name.length()-1);
    }
    else
    {
        arh_name=arh_name.right(arh_name.length()-_path.length());
        if(!arh_name.isEmpty() && (arh_name.at(0)=="/" || arh_name.at(0)=="\\"))
                arh_name=arh_name.right(arh_name.length()-1);
    }
    QFileInfo fi(file_name);
    file_name=file_name.left(file_name.length()-fi.suffix().length()-1);
    query->exec(QString("SELECT id FROM book where id_lib=%1 and file='%2' and archive='%3'").arg(QString::number(existingID),file_name,arh_name));
    if(query->next()) //если книга найдена, то просто снимаем пометку удаления
    {
        query->exec("update book set deleted=0 where id="+query->value(0).toString());
        return;
    }
    emit Message(tr("Book add:")+" "+file_name);

    //book_info bi;
    //GetBookInfo(bi,ba,"fb2",true);
    QDomDocument doc;
    doc.setContent(ba);
    QDomElement title_info=doc.elementsByTagName("title-info").at(0).toElement();
    QString sTitle=title_info.elementsByTagName("book-title").at(0).toElement().text();
    QString sLanguage=title_info.elementsByTagName("lang").at(0).toElement().text();
    QString sSeria = title_info.elementsByTagName("sequence").at(0).attributes().namedItem("name").toAttr().value().trimmed();
    uint numInSerial = title_info.elementsByTagName("sequence").at(0).attributes().namedItem("number").toAttr().value().trimmed().toUInt();

    QList<SAuthor> listAuthors;
    QDomNodeList authors=title_info.elementsByTagName("author");
    for(int i=0;i<authors.count();i++)
    {
        SAuthor author;
        author.sFirstName = authors.at(i).toElement().elementsByTagName("first-name").at(0).toElement().text().trimmed();
        author.sLastName = authors.at(i).toElement().elementsByTagName("last-name").at(0).toElement().text().trimmed();
        author.sMiddleName = authors.at(i).toElement().elementsByTagName("middle-name").at(0).toElement().text().trimmed();
        listAuthors << author;
    }
    QStringList listGenres;
    QDomNodeList genre=title_info.elementsByTagName("genre");
    for(int i=0;i<genre.count();i++)
    {
        listGenres << genre.at(i).toElement().text().trimmed();
    }
    QDomElement publish_info=doc.elementsByTagName("publish-info").at(0).toElement();
    QString sIsbn = publish_info.elementsByTagName("isbn").at(0).toElement().text();

    qlonglong id_seria=AddSeria(sSeria,existingID,0);
    qlonglong id_book=AddBook(0,sTitle,id_seria,numInSerial,file_name,(file_size==0?ba.size():file_size),0,false,fi.suffix(),QDate::currentDate(),sLanguage,"",existingID,arh_name,0);

    bool first_author=true;
    foreach(SAuthor author,listAuthors)
    {
        addAuthor(author, existingID,id_book,first_author,sLanguage,0);
        first_author=false;
        if(_firstAuthorOnly)
            break;
    }
    foreach(const auto sGenre,listGenres)
        AddGenre(id_book,sGenre,existingID,sLanguage);
}

void ImportThread::readEPUB(const QByteArray &ba, QString file_name, QString arh_name, qint32 file_size)
{
    if(arh_name.isEmpty())
    {
        file_name=file_name.right(file_name.length()-_path.length());
        if(file_name.left(1)=="/" || file_name.left(1)=="\\")
                file_name=file_name.right(file_name.length()-1);
    }
    else
    {
        arh_name=arh_name.right(arh_name.length()-_path.length());
        if(arh_name.left(1)=="/" || arh_name.left(1)=="\\")
                arh_name=arh_name.right(arh_name.length()-1);
    }
    file_name=file_name.left(file_name.length()-5);
    query->exec(QString("SELECT id FROM book where id_lib=%1 and file='%2' and archive='%3'").arg(QString::number(existingID),file_name,arh_name));
    if(query->next()) //если книга найдена, то просто снимаем пометку удаления
    {
        query->exec("update book set deleted=0 where id="+query->value(0).toString());
        return;
    }
    emit Message(tr("Book add: ")+file_name);

    QBuffer buf;
    buf.setData(ba);
    QuaZip zip(&buf);
    zip.open(QuaZip::mdUnzip);
    QBuffer info;
    SetCurrentZipFileName(&zip,"META-INF/container.xml");
    QuaZipFile zip_file(&zip);
    zip_file.open(QIODevice::ReadOnly);
    info.setData(zip_file.readAll());
    zip_file.close();
    QDomDocument doc;
    doc.setContent(info.data());
    QDomNode root=doc.documentElement();
    bool need_loop=true;
    QString rel_path;
    QString sTitle,sLanguage;
    QList<SAuthor> listAuthors;
    QStringList listGenres;

    for(int i=0;i<root.childNodes().count() && need_loop;i++)
    {
        if(root.childNodes().at(i).nodeName().toLower()=="rootfiles")
        {
            QDomNode roots=root.childNodes().at(i);
            for(int j=0;j<roots.childNodes().count() && need_loop;j++)
            {
                if(roots.childNodes().at(j).nodeName().toLower()=="rootfile")
                {
                    QString path=roots.childNodes().at(j).attributes().namedItem("full-path").toAttr().value();
                    QBuffer opf_buf;
                    QFileInfo fi(path);
                    rel_path=fi.path();
                    SetCurrentZipFileName(&zip,path);
                    zip_file.open(QIODevice::ReadOnly);
                    opf_buf.setData(zip_file.readAll());
                    zip_file.close();

                    QDomDocument opf;
                    opf.setContent(opf_buf.data());
                    QDomNode meta=opf.documentElement().namedItem("metadata");
                    for(int m=0;m<meta.childNodes().count();m++)
                    {
                        if(meta.childNodes().at(m).nodeName().right(5)=="title")
                        {
                            sTitle = meta.childNodes().at(m).toElement().text().trimmed();
                        }
                        else if(meta.childNodes().at(m).nodeName().right(8)=="language")
                        {
                            sLanguage = meta.childNodes().at(m).toElement().text().trimmed();
                        }
                        else if(meta.childNodes().at(m).nodeName().right(7)=="creator")
                        {
                            SAuthor author;
                            QStringList names=meta.childNodes().at(m).toElement().text().trimmed().split(" ");
                            if(names.count()>0)
                                author.sFirstName = names[0];
                            if(names.count()>1)
                                author.sMiddleName = names[1];
                            if(names.count()>2)
                                author.sLastName = names[2];
                            listAuthors << author;
                        }
                        else if(meta.childNodes().at(m).nodeName().right(7)=="subject")
                        {
                            listGenres << meta.childNodes().at(m).toElement().text().trimmed();
                        }
                    }
                    need_loop=false;
                }
            }
        }
    }
    zip.close();
    buf.close();
    info.close();

    if(sTitle.isEmpty() || listAuthors.count()==0)
    {
        return;
    }

    qlonglong id_book=AddBook(0,sTitle,0,0,file_name,(file_size==0?ba.size():file_size),0,false,"epub",QDate::currentDate(),sLanguage,"",existingID,arh_name,0);

    bool first_author=true;
    foreach(SAuthor author,listAuthors)
    {
        addAuthor(author, existingID,id_book,first_author,sLanguage,0);
        first_author=false;
    }
    foreach(const auto sGenre,listGenres)
        AddGenre(id_book,sGenre,existingID,sLanguage);
}

void ImportThread::importFB2_main(QString path)
{
    int count=0;
    importFB2(path, count);
    if(count>0)
        query->exec("COMMIT;");
}

void ImportThread::importFB2(QString path, int &count)
{
    QDir dir(path);
    QFileInfoList info_list = dir.entryInfoList(QDir::NoSymLinks|QDir::NoDotAndDotDot|QDir::Readable|QDir::Files|QDir::Dirs);
    QList<QFileInfo>::iterator iter=info_list.begin();
    QString file_name;
    for(iter=info_list.begin();iter != info_list.end() && loop;iter++)
    {
        app->processEvents();
        file_name = iter->absoluteFilePath();
        if(iter->isDir())
        {
            importFB2(file_name,count);
        }
        else
        {
            if(iter->suffix().toLower()!="fbd" &&
                    !(iter->suffix().toLower()=="zip" ||
                     iter->suffix().toLower()=="fb2" ||
                     iter->suffix().toLower()=="epub"))
            {
                QString fbd=iter->absolutePath()+"/"+iter->completeBaseName()+".fbd";
                QFile file(fbd);
                if(file.exists())
                {
                    file.open(QFile::ReadOnly);
                    readFB2(file.readAll(),file_name,"",iter->size());
                    count++;
                }
            }
            else if(iter->suffix().toLower()=="fb2" || iter->suffix().toLower()=="epub")
            {
                if(count==0)
                    query->exec("BEGIN;");
                QFile file(file_name);
                file.open(QFile::ReadOnly);
                QByteArray ba=file.readAll();
                if(iter->suffix().toLower()=="fb2")
                    readFB2(ba,file_name,"");
                else
                    readEPUB(ba,file_name,"");
                count++;
            }
            else if(iter->suffix().toLower()=="zip")
            {
                if(_update_type==UT_NEW)
                {
                    emit Message(iter->fileName());
                    QString arh_name=file_name.right(file_name.length()-_path.length());
                    if(arh_name.left(1)=="/" || arh_name.left(1)=="\\")
                            arh_name=arh_name.right(arh_name.length()-1);
                    query->exec(QString("SELECT * FROM book where archive='%1' LIMIT 1").arg(arh_name));
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
                QList<QuaZipFileInfo64> list=uz.getFileInfoList64();
                foreach( QuaZipFileInfo64 str , list  )
                {
                    app->processEvents();
                    if(!loop)
                        break;
                    QBuffer buffer;
                    if(count==0)
                        query->exec("BEGIN;");
                    QuaZipFileInfo zip_fi;
                    str.toQuaZipFileInfo(zip_fi);
                    if(zip_fi.name.right(3).toLower()=="fb2")
                    {
                        //uz.extractFile(str.filename,&buffer,UnZip::SkipPaths,16*1024);
                        SetCurrentZipFileName(&uz,zip_fi.name);
                        zip_file.open(QIODevice::ReadOnly);
                        buffer.setData(zip_file.read(16*1024));
                        zip_file.close();
                        readFB2(buffer.data(),str.name,file_name, str.uncompressedSize);
                    }
                    else if(zip_fi.name.right(3).toLower()=="epub")
                    {
                        SetCurrentZipFileName(&uz,zip_fi.name);
                        zip_file.open(QIODevice::ReadOnly);
                        buffer.setData(zip_file.readAll());
                        readEPUB(buffer.data(),str.name,file_name, str.uncompressedSize);
                    }
                    else if(zip_fi.name.right(3).toLower()!="fbd")
                    {
                        QFileInfo fi(str.name);
                        if(fi.completeBaseName().left(1)!="." && !fi.completeBaseName().isEmpty())
                        {
                            QString fbd=fi.path()+"/"+fi.completeBaseName()+".fbd";
                            if(SetCurrentZipFileName(&uz,fbd))
                            {
                                SetCurrentZipFileName(&uz,zip_fi.name);
                                zip_file.open(QIODevice::ReadOnly);
                                buffer.setData(zip_file.readAll());
                                readFB2(buffer.data(),str.name,file_name, str.uncompressedSize);
                            }
                        }
                    }

                    count++;
                    if(count==1000)
                    {
                        query->exec("COMMIT;");
                        count=0;
                    }
                }
            }
            if(count==1000)
            {
                query->exec("COMMIT;");
                count=0;
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
    QSettings *settings=GetSettings();
    QFileInfo fi(RelativeToAbsolutePath(settings->value("database_path").toString()));
    QString sDbFile=fi.canonicalFilePath();
    QSqlDatabase dbase = QSqlDatabase::addDatabase("QSQLITE","importdb");
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
        ClearLib(dbase,existingID,false);
        break;
    case UT_DEL_AND_NEW:
        ClearLib(dbase,existingID,true);
        break;
    }

    if(_fileName.isEmpty())
    {
        importFB2_main(_path);
        query->exec("drop table if exists tmp;");
        query->exec(QString("create table tmp as select id from book where id_lib=%1 and deleted=1;").arg(QString::number(existingID)));
        query->exec(QString("delete from book where id_lib=%1 and id in (select id from tmp);").arg(QString::number(existingID)));
        query->exec(QString("delete from book_janre where id_lib=%1 and id_book in (select id from tmp);").arg(QString::number(existingID)));
        query->exec(QString("delete from book_author where id_lib=%1 and id_book in (select id from tmp);").arg(QString::number(existingID)));
        query->exec("drop table if exists tmp;");
        query->exec("VACUUM");
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
    qlonglong book_count=0;
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
    foreach( QString str , list  )
    {
        if(QString(str).toUpper()=="STRUCTURE.INFO")
        {
            for(unsigned int i=0;i<sizeof(field_index)/sizeof(int);i++)
                field_index[i]=-1;
            QBuffer outbuff;
            SetCurrentZipFileName(&uz,str);
            zip_file.open(QIODevice::ReadOnly);
            outbuff.setData(zip_file.readAll());
            zip_file.close();
            QStringList lines=(QString::fromUtf8(outbuff.data())).split('\n');
            foreach(QString line,lines)
            {
                QStringList substrings=line.toUpper().split(';');
                int i=0;
                foreach(QString substring,substrings)
                {
                    if(substring=="TITLE")
                        field_index[_NAME]=i;
                    else if(substring=="SERIES")
                        field_index[_SERIA]=i;
                    else if(substring=="SERNO")
                        field_index[_NUM_IN_SERIA]=i;
                    else if(substring=="FILE")
                        field_index[_FILE]=i;
                    else if(substring=="SIZE")
                        field_index[_SIZE]=i;
                    else if(substring=="LIBID")
                        field_index[_ID_IN_LIB]=i;
                    else if(substring=="DEL")
                        field_index[_DELETED]=i;
                    else if(substring=="EXT")
                        field_index[_FORMAT]=i;
                    else if(substring=="DATE")
                        field_index[_DATE]=i;
                    else if(substring=="LANG")
                        field_index[_LANGUAGE]=i;
                    else if(substring=="STARS")
                        field_index[_STAR]=i;
                    else if(substring=="KEYWORDS")
                        field_index[_KEYS]=i;
                    else if(substring=="AUTHOR")
                        field_index[_AUTHORS]=i;
                    else if(substring=="GENRE")
                        field_index[_JANRES]=i;
                    else if(substring=="FOLDER")
                        field_index[_FOLDER]=i;
                    else if(substring=="TAG")
                        field_index[_TAG]=i;
                    else if(substring=="TAGSERIES")
                        field_index[_TAG_SERIA]=i;
                    else if(substring=="TAGAUTHOR")
                        field_index[_TAG_AUTHOR]=i;
                  i++;
                }

            }
            break;
        }
    }
    dbase.transaction();

    foreach( QString str , list  )
    {
        app->processEvents();
        if(!loop)
        {
            break;
        }
        if(str.right(3).toUpper()!="INP")
            continue;
        emit Message(str);
        if(_update_type==UT_NEW)
        {
            query->exec(QString("SELECT * FROM book where archive='%1' and id_lib=%2 LIMIT 1").arg(str,QString::number(existingID)));
            if(query->next())
                continue;
        }
        QBuffer outbuff;
        SetCurrentZipFileName(&uz,str);
        zip_file.open(QIODevice::ReadOnly);
        outbuff.setData(zip_file.readAll());
        zip_file.close();
        //uz.extractFile(str,&outbuff,UnZip::SkipPaths);
        QStringList lines=(QString::fromUtf8(outbuff.data())).split('\n');
        qlonglong t1_=0,t2_=0,t3_=0,t4_=0,count=0;
        foreach(QString line,lines)
        {
            if(line.isEmpty())
                continue;

            qlonglong t0=QDateTime::currentMSecsSinceEpoch();
            app->processEvents();
            if(!loop)
            {
                break;
            }
            QStringList substrings=line.split(QChar(4));
            if(substrings.count()==0)
                continue;
            QString name;
            if(substrings.count()>field_index[_NAME])
            {
                name=substrings[field_index[_NAME]].trimmed();
            }
            QString Seria;
            if(substrings.count()>field_index[_SERIA])
                Seria=substrings[field_index[_SERIA]].trimmed();
            int num_in_seria=0;
            if(substrings.count()>field_index[_NUM_IN_SERIA])
            {
                num_in_seria=substrings[field_index[_NUM_IN_SERIA]].trimmed().toInt();
            }
            QString file;
            if(substrings.count()>field_index[_FILE])
            {
                file=substrings[field_index[_FILE]].trimmed();
            }
            int size=0;
            if(substrings.count()>field_index[_SIZE])
            {
                size=substrings[field_index[_SIZE]].trimmed().toInt();
            }
            int id_in_lib=0;
            if(substrings.count()>field_index[_ID_IN_LIB])
            {
                id_in_lib=substrings[field_index[_ID_IN_LIB]].trimmed().toInt();
            }
            bool deleted=0;
            if(substrings.count()>field_index[_DELETED])
            {
                deleted=(substrings[field_index[_DELETED]].trimmed().toInt()>0);
            }
            QString format;
            if(substrings.count()>field_index[_FORMAT])
            {
                format=substrings[field_index[_FORMAT]].trimmed();
            }

            QDate date;
            if(substrings.count()>field_index[_DATE])
            {
                date=QDate::fromString(substrings[field_index[_DATE]].trimmed(),"yyyy-MM-dd");
            }
            QString language;
            if(substrings.count()>field_index[_LANGUAGE])
            {
                language=substrings[field_index[_LANGUAGE]].trimmed();
            }
            qlonglong star=0;
            if(substrings.count()>field_index[_STAR] && field_index[_STAR]>=0)
            {
                star=substrings[field_index[_STAR]].trimmed().toInt();
            }
            QString keys;
            if(substrings.count()>field_index[_KEYS])
            {
                keys=substrings[field_index[_KEYS]].trimmed();
            }
            QString folder=str;
            if(substrings.count()>field_index[_FOLDER] && field_index[_FOLDER]>=0)
            {
                folder=substrings[field_index[_FOLDER]].trimmed();
            }
            int tag=0;
            if(substrings.count()>field_index[_TAG] && field_index[_TAG]>=0)
            {
                tag=substrings[field_index[_TAG]].trimmed().toInt();
            }
            int tag_seria=0;
            if(substrings.count()>field_index[_TAG_SERIA] && field_index[_TAG_SERIA]>=0)
            {
                tag_seria=substrings[field_index[_TAG_SERIA]].trimmed().toInt();
            }
            QStringList tag_author;
            if(substrings.count()>field_index[_TAG_AUTHOR] && field_index[_TAG_AUTHOR]>=0)
            {
                tag_author=substrings[field_index[_TAG_AUTHOR]].trimmed().split(":");
            }
            qlonglong id_seria=0;
            if(!Seria.isEmpty())
                id_seria=AddSeria(Seria,existingID,tag_seria);

            qlonglong t1=QDateTime::currentMSecsSinceEpoch();
            qlonglong id_book;
            if(!bWoDeleted_ || !deleted){
                id_book=AddBook(star,name,id_seria,num_in_seria,file,size,id_in_lib,deleted,format,date,language,keys,existingID,folder,tag);
                qlonglong t2=QDateTime::currentMSecsSinceEpoch();

                QStringList Authors=substrings[field_index[_AUTHORS]].split(':');
                int author_count=0;
                foreach(QString sAuthor,Authors)
                {
                    int tag_auth=0;
                    if(author_count<tag_author.count())
                    {
                        if(!tag_author[author_count].trimmed().isEmpty())
                            tag_auth=tag_author[author_count].trimmed().toInt();
                    }
                    QString sAuthorLow = sAuthor.toLower();
                    if(format == "fb2" && sAuthorLow.contains("автор") && (sAuthorLow.contains("неизвестен") || sAuthorLow.contains("неизвестный")))
                    {
                        QString sFile, sArchive;
                        QBuffer buffer;
                        sFile = QString("%1.%2").arg(file).arg(format);
                        sArchive = QString("%1/%2").arg(_path).arg(folder.replace(".inp",".zip"));
                        if(archiveFile.getZipName() != sArchive){
                            archiveFile.close();
                            archiveFile.setZipName(sArchive);
                            if (!archiveFile.open(QuaZip::mdUnzip))
                                qDebug()<<("Error open archive!")<<" "<<sArchive;
                        }
                        QuaZipFile zip_file(&archiveFile);
                        SetCurrentZipFileName(&archiveFile,sFile);
                        if(!zip_file.open(QIODevice::ReadOnly))
                            qDebug()<<"Error open file: "<<sFile;
                        buffer.setData(zip_file.read(16*1024));
                        zip_file.close();

                        QDomDocument doc;
                        doc.setContent(buffer.data());
                        QDomElement title_info=doc.elementsByTagName("title-info").at(0).toElement();
                        QDomNodeList listAuthor=title_info.elementsByTagName("author");

                        for(int i=0;i<listAuthor.count();i++)
                        {
                            SAuthor author;
                            author.sFirstName = listAuthor.at(i).toElement().elementsByTagName("first-name").at(0).toElement().text();
                            author.sLastName = listAuthor.at(i).toElement().elementsByTagName("last-name").at(0).toElement().text();
                            author.sMiddleName = listAuthor.at(i).toElement().elementsByTagName("middle-name").at(0).toElement().text();
                            //QString sAuthor = author=lastname+","+firstname+","+middlename;
                            sAuthorLow = author.getName().toLower();
                            if(!sAuthorLow.contains("неизвестен") && !sAuthorLow.contains("неизвестный")){
                                //QString sAuthor = QString("%1,%2,%3").arg(lastname,firstname,middlename);
                                addAuthor(author,existingID,id_book,author_count==0,language,tag_auth);
                                author_count++;
                            }
                        }
                    }else{
                        SAuthor author(sAuthor);
                        addAuthor(author,existingID,id_book,author_count==0,language,tag_auth);
                        author_count++;
                    }
                }

                qlonglong t3=QDateTime::currentMSecsSinceEpoch();
                if(substrings.count()>=field_index[_JANRES]+1)
                {
                    QStringList Janres=substrings[field_index[_JANRES]].split(':');
                    bool first=true;
                    foreach(QString janre,Janres)
                    {
                        if(!first && janre.trimmed().isEmpty())
                            continue;
                        AddGenre(id_book,janre.trimmed(),existingID,language);
                        first=false;
                    }
                }
                qlonglong t4=QDateTime::currentMSecsSinceEpoch();
                t1_+=t1-t0;
                t2_+=t2-t1;
                t3_+=t3-t2;
                t4_+=t4-t3;
                count++;
                book_count++;
                if(count==1000)
                {
                    emit Message(tr("Books adds:")+" "+QString::number(book_count));
                    count=0;
                    t1_=0;
                    t2_=0;
                    t3_=0;
                    t4_=0;
                }

            }
        }
        if(count>0)
        {
            emit Message(tr("Books adds: ")+QString::number(book_count));
        }
    }
    dbase.commit();
    emit End();
    dbase.close();
}

void ImportThread::break_import()
{
    loop=false;
}

