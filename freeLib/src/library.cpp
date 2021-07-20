#include <QDomDocument>

#include "common.h"
#include "library.h"
#include "quazip/quazip/quazip.h"
#include "quazip/quazip/quazipfile.h"

QMap<int,SLib> mLibs;
QMap <uint,SGenre> mGenre;

void loadLibrary(uint idLibrary)
{
    if(!db_is_open)
        return;

    qint64 t_start = QDateTime::currentMSecsSinceEpoch();
    QSqlQuery query(QSqlDatabase::database("libdb"));
    query.setForwardOnly(true);
    SLib& lib = mLibs[idLibrary];
    lib.mSerials.clear();
    query.prepare("SELECT id, name, favorite FROM seria WHERE id_lib=:id_lib;");
    //                    0   1     2
    query.bindValue(":id_lib",idLibrary);
    if(!query.exec())
        qDebug() << query.lastError().text();
    while (query.next()) {
        uint idSerial = query.value(0).toUInt();
        QString sName = query.value(1).toString();
        lib.mSerials[idSerial].sName = sName;
        lib.mSerials[idSerial].nTag = static_cast<uchar>(query.value(2).toUInt());
    }
    qint64 t_end = QDateTime::currentMSecsSinceEpoch();
    qDebug()<< "loadSeria " << t_end-t_start << "msec";

    t_start = QDateTime::currentMSecsSinceEpoch();
    lib.mAuthors.insert(0,SAuthor());
    query.prepare("SELECT author.id, name1, name2, name3, author.favorite FROM author WHERE id_lib=:id_lib;");
    //                    0          1      2      3      4
    query.bindValue(":id_lib",idLibrary);
    query.exec();
    while (query.next()) {
        uint idAuthor = query.value(0).toUInt();
        int nTag = query.value(4).toInt();
        SAuthor &author = lib.mAuthors[idAuthor];
        author.sFirstName = query.value(2).toString().trimmed();;
        author.sLastName = query.value(1).toString().trimmed();;
        author.sMiddleName = query.value(3).toString().trimmed();;
        lib.mAuthors[idAuthor].nTag = nTag;
    }
    t_end = QDateTime::currentMSecsSinceEpoch();
    qDebug()<< "loadAuthor " << t_end-t_start << "msec";

    lib.mBooks.clear();
    query.setForwardOnly(true);
    query.prepare("SELECT id, name, star, id_seria, num_in_seria, language, file, size, deleted, date, format, id_inlib, archive, first_author_id, favorite FROM book WHERE id_lib=:id_lib;");
    //                     0  1     2     3         4             5         6     7     8        9     10      11        12       13               14
    query.bindValue(":id_lib",idLibrary);
    if(!query.exec())
        qDebug() << query.lastError().text();
    while (query.next()) {
        QString sName = query.value(1).toString();
        if(sName.isEmpty())
            continue;
        uint id = query.value(0).toUInt();
        SBook &book = lib.mBooks[id];
        book.sName = sName;
        book.nStars = qvariant_cast<uchar>(query.value(2));
        book.idSerial = query.value(3).toUInt();
        book.numInSerial = query.value(4).toUInt();
        QString sLaguage = query.value(5).toString().toLower();
        int idLaguage = lib.vLaguages.indexOf(sLaguage);
        if(idLaguage<0){
            idLaguage =lib.vLaguages.count();
            lib.vLaguages << sLaguage;
        }
        book.idLanguage = static_cast<uchar>(idLaguage);
        book.sFile = query.value(6).toString();
        book.nSize = query.value(7).toUInt();
        book.bDeleted = query.value(8).toBool();
        book.date = query.value(9).toDate();
        book.sFormat = query.value(10).toString();
        book.idInLib = query.value(11).toUInt();
        book.sArchive = query.value(12).toString();
        book.idFirstAuthor = query.value(13).toUInt();
        book.nTag = qvariant_cast<uchar>(query.value(14));
    }

    lib.mAuthorBooksLink.clear();
    query.prepare("SELECT id_book, id_author FROM book_author WHERE id_lib=:id_lib;");
    //                     0       1
    query.bindValue(":id_lib",idLibrary);
    if(!query.exec())
        qDebug() << query.lastError().text();
    while (query.next()) {
        uint idBook = query.value(0).toUInt();
        uint idAuthor = query.value(1).toUInt();
        if(lib.mBooks.contains(idBook) && lib.mAuthors.contains(idAuthor)){
            lib.mAuthorBooksLink.insert(idAuthor,idBook);
            lib.mBooks[idBook].listIdAuthors << idAuthor;
        }
    }
    auto iBook = lib.mBooks.begin();
    uint emptycount = 0;
    while(iBook != lib.mBooks.end()){
        if(iBook->listIdAuthors.isEmpty()){
            iBook->listIdAuthors << 0;
            lib.mAuthorBooksLink.insert(0,iBook.key());
            emptycount++;
        }
        ++iBook;
    }
    query.prepare("SELECT id_book, id_janre FROM book_janre WHERE id_lib=:id_lib;");
    //                     0       1
    query.bindValue(":id_lib",idLibrary);
    if(!query.exec())
        qDebug() << query.lastError().text();
    while (query.next()) {
        uint idBook = query.value(0).toUInt();
        uint idGenre = query.value(1).toUInt();
        if(idGenre==0) idGenre = 1112; // Прочие/Неотсортированное
        if(lib.mBooks.contains(idBook))
            lib.mBooks[idBook].listIdGenres << idGenre;
    }
    lib.bLoaded = true;

    t_end = QDateTime::currentMSecsSinceEpoch();
    qDebug()<< "loadBooks " << t_end-t_start << "msec";
}

void loadGenres()
{
    if(!db_is_open)
        return;
    qint64 t_start = QDateTime::currentMSecsSinceEpoch();
    QSqlQuery query(QSqlDatabase::database("libdb"));

    mGenre.clear();
    query.prepare("SELECT id, name, id_parent, sort_index FROM janre;");
    //                    0   1     2          3
    if(!query.exec())
        qDebug() << query.lastError().text();
    while (query.next()) {
        uint idGenre = query.value(0).toUInt();
        SGenre &genre = mGenre[idGenre];
        genre.sName = query.value(1).toString();
        genre.idParrentGenre = static_cast<ushort>(query.value(2).toUInt());
        genre.nSort = static_cast<ushort>(query.value(3).toUInt());
    }
    qint64 t_end = QDateTime::currentMSecsSinceEpoch();
    qDebug()<< "loadGenre " << t_end-t_start << "msec";
}

SAuthor::SAuthor()
{

}

SAuthor::SAuthor(QString sName)
{
    QStringList listNames=sName.split(",");
    if(listNames.count()>0)
        sLastName=listNames[0].trimmed();
    if(listNames.count()>1)
        sFirstName=listNames[1].trimmed();
    if(listNames.count()>2)
        sMiddleName=listNames[1].trimmed();

}

QString SAuthor::getName() const
{
    QString sAuthorName = QString("%1 %2 %3").arg(sLastName,sFirstName,sMiddleName).trimmed();
    if(sAuthorName.trimmed().isEmpty())
        sAuthorName = QCoreApplication::translate("MainWindow","unknown author");
    return sAuthorName;
}

uint SLib::findAuthor(SAuthor &author)
{
    uint idAuthor = 0;
    auto iAuthor = mAuthors.constBegin();
    while(iAuthor!= mAuthors.constEnd() ){
        if(author.sFirstName== iAuthor->sFirstName && author.sMiddleName==iAuthor->sMiddleName && author.sLastName==iAuthor->sLastName){
            idAuthor = iAuthor.key();
            break;
        }
        iAuthor++;
    }
    return idAuthor;
}

uint SLib::findSerial(QString sSerial)
{
    uint idSerial = 0;
    auto iSerial = mSerials.constBegin();
    while(iSerial != mSerials.constEnd()){
        if(sSerial == iSerial->sName){
            idSerial = iSerial.key();
            break;
        }
        iSerial++;
    }
    return idSerial;
}

void SLib::loadAnnotation(uint idBook)
{
    QString sFile,sArchive;
    QFileInfo fi;
    QBuffer buffer, buffer_info;

    SBook& book = mBooks[idBook];
    if(book.sArchive.isEmpty()){
        sFile = QString("%1/%2.%3").arg(path,book.sFile,book.sFormat);
    }else{
        sFile = QString("%1.%2").arg(book.sFile,book.sFormat);
        sArchive = QString("%1/%2").arg(path,book.sArchive.replace(".inp",".zip"));
    }

    sArchive=sArchive.replace("\\","/");
    if(sArchive.isEmpty())
    {
        QFile book_file(sFile);
        if(!book_file.open(QFile::ReadOnly))
        {
            qDebug()<<("Error open file!")<<" "<<sFile;
            book.sAnnotation = "<font color=\"red\">" + QCoreApplication::translate("MainWindow","Can't find file: %1").arg(sFile) + "</font>";
            return;
        }
        buffer.setData(book_file.readAll());
        fi.setFile(book_file);
        fi.setFile(sFile);
        QString fbd=fi.absolutePath()+"/"+fi.completeBaseName()+".fbd";
        QFile info_file(fbd);
        if(info_file.exists())
        {
            info_file.open(QFile::ReadOnly);
            buffer_info.setData(info_file.readAll());
        }
    }
    else
    {
        QuaZip uz(sArchive);
        if (!uz.open(QuaZip::mdUnzip))
        {
            qDebug()<<("Error open archive!")<<" "<<sArchive;
            book.sAnnotation = "<font color=\"red\">" + QCoreApplication::translate("MainWindow","Can't find file: %1").arg(sFile) + "</font>";
            return;
        }

        QuaZipFile zip_file(&uz);
        SetCurrentZipFileName(&uz,sFile);
        if(!zip_file.open(QIODevice::ReadOnly))
        {
            qDebug()<<"Error open file: "<<sFile;
        }
        buffer.setData(zip_file.readAll());
        zip_file.close();
        fi.setFile(sFile);
        QString fbd=fi.path()+"/"+fi.completeBaseName()+".fbd";

        if(SetCurrentZipFileName(&uz,fbd))
        {
            zip_file.open(QIODevice::ReadOnly);
            buffer.setData(zip_file.readAll());
            zip_file.close();
        }

        fi.setFile(sArchive+"/"+sFile);
    }

    if(book.sFormat=="epub"){
        QuaZip zip(&buffer);
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
                            if(meta.childNodes().at(m).nodeName().right(11)=="description")
                            {
                                QBuffer buff;
                                buff.open(QIODevice::WriteOnly);
                                QTextStream ts(&buff);
                                ts.setCodec("UTF-8");
                                meta.childNodes().at(m).save(ts,0,QDomNode::EncodingFromTextStream);
                                book.sAnnotation=QString::fromUtf8(buff.data().data());
                            }
                            else if(meta.childNodes().at(m).nodeName().right(4)=="meta" /*&& !info_only*/)
                            {
                                if(meta.childNodes().at(m).attributes().namedItem("name").toAttr().value()=="cover")
                                {

                                    QString cover=meta.childNodes().at(m).attributes().namedItem("content").toAttr().value();
                                    QDomNode manifest=opf.documentElement().namedItem("manifest");
                                    for(int man=0;man<manifest.childNodes().count();man++)
                                    {
                                        if(manifest.childNodes().at(man).attributes().namedItem("id").toAttr().value()==cover)
                                        {
                                            QBuffer img;
                                            cover=rel_path+"/"+manifest.childNodes().at(man).attributes().namedItem("href").toAttr().value();

                                            SetCurrentZipFileName(&zip,cover);
                                            zip_file.open(QIODevice::ReadOnly);
                                            img.setData(zip_file.readAll());
                                            zip_file.close();

                                            //проверить как работает
                                            QString sImgFile = QString("%1/freeLib/%2")
                                                    .arg(QStandardPaths::standardLocations(QStandardPaths::TempLocation).constFirst())
                                                    .arg(idBook);
                                            QPixmap image;
                                            image.loadFromData(img.data());
                                            image.save(sImgFile);

                                            book.sImg=QString("<td valign=top style=\"width:1px\"><center><img src=\"file:%1\"></center></td>").arg(sImgFile);
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                        need_loop=false;
                    }
                }
            }
        }
        zip.close();
        info.close();
    }else if(book.sFormat=="fb2"){
        QDomDocument doc;
        doc.setContent(buffer.data());
        QDomElement title_info=doc.elementsByTagName("title-info").at(0).toElement();
            QString cover=QString::fromStdString( title_info.elementsByTagName("coverpage").at(0).toElement().elementsByTagName("image").at(0).attributes().namedItem("l:href").toAttr().value().toStdString());
            if(!cover.isEmpty() && cover.at(0)=="#")
            {
                QDomNodeList binarys=doc.elementsByTagName("binary");
                for(int i=0;i<binarys.count();i++)
                {
                    if(binarys.at(i).attributes().namedItem("id").toAttr().value()==cover.right(cover.length()-1))
                    {
                        QString sImgFile = QString("%1/freeLib/%2.jpg")
                                .arg(QStandardPaths::standardLocations(QStandardPaths::TempLocation).constFirst())
                                .arg(idBook);
                        QPixmap image;
                        QByteArray ba;
                        ba.append(binarys.at(i).toElement().text().toLatin1());
                        QByteArray ba64 = QByteArray::fromBase64(ba);
                        image.loadFromData(ba64);
                        image.save(sImgFile);
                        book.sImg = QString("<td valign=top><center><img src=\"file:%1\"></center></td>").arg(sImgFile);
                        break;
                    }
                }
            }
            QBuffer buff;
            buff.open(QIODevice::WriteOnly);
            QTextStream ts(&buff);
            ts.setCodec("UTF-8");
            title_info.elementsByTagName("annotation").at(0).save(ts,0,QDomNode::EncodingFromTextStream);
            book.sAnnotation=QString::fromUtf8(buff.data().data());
            book.sAnnotation.replace("<annotation>","",Qt::CaseInsensitive);
            book.sAnnotation.replace("</annotation>","",Qt::CaseInsensitive);
    }
}

QFileInfo SLib::getBookFile(QBuffer &buffer, QBuffer &buffer_info, uint id_book, bool caption, QDateTime *file_data)
{
    QString file,archive;
    QFileInfo fi;
    SBook &book = mBooks[id_book];
    //QString LibPath=mLibs[idCurrentLib].path;
    QString LibPath=RelativeToAbsolutePath(path);
    if(book.sArchive.isEmpty()){
        file = QString("%1/%2.%3").arg(LibPath,book.sFile,book.sFormat);
    }else{
        file = QString("%1.%2").arg(book.sFile,book.sFormat);
        archive = QString("%1/%2").arg(LibPath,book.sArchive.replace(".inp",".zip"));
    }

    archive=archive.replace("\\","/");
    if(archive.isEmpty())
    {
        QFile book_file(file);
        if(!book_file.open(QFile::ReadOnly))
        {
            qDebug()<<("Error open file!")<<" "<<file;
            return fi;
        }
        buffer.setData(book_file.readAll());
        fi.setFile(book_file);
        if(file_data)
        {
            *file_data=fi.birthTime();
        }
        fi.setFile(file);
        QString fbd=fi.absolutePath()+"/"+fi.completeBaseName()+".fbd";
        QFile info_file(fbd);
        if(info_file.exists())
        {
            info_file.open(QFile::ReadOnly);
            buffer_info.setData(info_file.readAll());
        }
    }
    else
    {
        QuaZip uz(archive);
        if (!uz.open(QuaZip::mdUnzip))
        {
            qDebug()<<("Error open archive!")<<" "<<archive;
            return fi;
        }

        if(file_data)
        {
            SetCurrentZipFileName(&uz,file);
            QuaZipFileInfo64 zip_fi;
            if(uz.getCurrentFileInfo(&zip_fi))
            {
                *file_data=zip_fi.dateTime;
            }
        }
        QuaZipFile zip_file(&uz);
        SetCurrentZipFileName(&uz,file);
        if(!zip_file.open(QIODevice::ReadOnly))
        {
            qDebug()<<"Error open file: "<<file;
        }
        if(caption)
        {
            buffer.setData(zip_file.read(16*1024));
        }
        else
        {
            buffer.setData(zip_file.readAll());
        }
        zip_file.close();
        fi.setFile(file);
        QString fbd=fi.path()+"/"+fi.completeBaseName()+".fbd";

        if(SetCurrentZipFileName(&uz,fbd))
        {
            zip_file.open(QIODevice::ReadOnly);
            buffer.setData(zip_file.readAll());
            zip_file.close();
        }

        fi.setFile(archive+"/"+file);
    }
    return fi;
}

