#define QT_USE_QSTRINGBUILDER
#include "library.h"

#include <QDomDocument>
#include <QStringBuilder>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QApplication>
#include <QStandardPaths>
#include <QDir>

#include "quazip/quazip/quazip.h"
#include "quazip/quazip/quazipfile.h"

QMap<uint,SLib> mLibs;
QMap <uint,SGenre> mGenre;
bool SetCurrentZipFileName(QuaZip *zip,const QString &name);
QString RelativeToAbsolutePath(QString path);

void loadLibrary(uint idLibrary)
{
    if(idLibrary == 0)
        return;
    if(!QSqlDatabase::database(QStringLiteral("libdb"), false).isOpen())
        return;

    qint64 t_start = QDateTime::currentMSecsSinceEpoch();
    QSqlQuery query(QSqlDatabase::database(QStringLiteral("libdb")));
    query.setForwardOnly(true);
    SLib& lib = mLibs[idLibrary];
    lib.mSerials.clear();
    query.prepare(QStringLiteral("SELECT id, name FROM seria WHERE id_lib=:id_lib;"));
    //                                   0   1
    query.bindValue(QStringLiteral(":id_lib"), idLibrary);
    if(!query.exec())
        qDebug() << query.lastError().text();
    while (query.next()) {
        uint idSerial = query.value(0).toUInt();
        QString sName = query.value(1).toString();
        lib.mSerials[idSerial].sName = sName;
    }
    query.prepare(QStringLiteral("SELECT seria_tag.id_seria, seria_tag.id_tag FROM seria_tag INNER JOIN seria ON seria.id = seria_tag.id_seria WHERE seria.id_lib = :id_lib"));
    query.bindValue(QStringLiteral(":id_lib"),idLibrary);
    if(!query.exec())
        qDebug() << query.lastError().text();
    while (query.next()) {
        uint idSeria = query.value(0).toUInt();
        uint idTag = query.value(1).toUInt();
        if(lib.mSerials.contains(idSeria))
            lib.mSerials[idSeria].listIdTags << idTag;
    }
    qint64 t_end = QDateTime::currentMSecsSinceEpoch();
    qDebug()<< "loadSeria " << t_end-t_start << "msec";

    t_start = QDateTime::currentMSecsSinceEpoch();
    lib.mAuthors.insert(0,SAuthor());
    query.prepare(QStringLiteral("SELECT author.id, name1, name2, name3 FROM author WHERE id_lib=:id_lib;"));
    //                                     0          1      2      3
    query.bindValue(QStringLiteral(":id_lib"),idLibrary);
    query.exec();
    while (query.next()) {
        uint idAuthor = query.value(0).toUInt();
        SAuthor &author = lib.mAuthors[idAuthor];
        author.sFirstName = query.value(2).toString().trimmed();;
        author.sLastName = query.value(1).toString().trimmed();;
        author.sMiddleName = query.value(3).toString().trimmed();;
    }
    query.prepare(QStringLiteral("SELECT author_tag.id_author, author_tag.id_tag FROM author_tag INNER JOIN author ON author.id = author_tag.id_author WHERE author.id_lib = :id_lib"));
    query.bindValue(QStringLiteral(":id_lib"),idLibrary);
    if(!query.exec())
        qDebug() << query.lastError().text();
    while (query.next()) {
        uint idAuthor = query.value(0).toUInt();
        uint idTag = query.value(1).toUInt();
        if(lib.mAuthors.contains(idAuthor))
            lib.mAuthors[idAuthor].listIdTags << idTag;
    }

    t_end = QDateTime::currentMSecsSinceEpoch();
    qDebug()<< "loadAuthor " << t_end-t_start << "msec";

    lib.mBooks.clear();
    query.setForwardOnly(true);
    query.prepare(QStringLiteral("SELECT id, name, star, id_seria, num_in_seria, language, file, size, deleted, date, format, id_inlib, archive, first_author_id FROM book WHERE id_lib=:id_lib;"));
    //                                    0  1     2     3         4             5         6     7     8        9     10      11        12       13
    query.bindValue(QStringLiteral(":id_lib"),idLibrary);
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
    }

    lib.mAuthorBooksLink.clear();
    query.prepare(QStringLiteral("SELECT id_book, id_author FROM book_author WHERE id_lib=:id_lib;"));
    //                                     0       1
    query.bindValue(QStringLiteral(":id_lib"),idLibrary);
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
    query.prepare(QStringLiteral("SELECT id_book, id_genre FROM book_genre WHERE id_lib=:id_lib;"));
    //                                   0        1
    query.bindValue(QStringLiteral(":id_lib"),idLibrary);
    if(!query.exec())
        qDebug() << query.lastError().text();
    while (query.next()) {
        uint idBook = query.value(0).toUInt();
        uint idGenre = query.value(1).toUInt();
        if(idGenre == 0) idGenre = 1112; // Прочие/Неотсортированное
        if(lib.mBooks.contains(idBook))
            lib.mBooks[idBook].listIdGenres << idGenre;
    }
    query.prepare(QStringLiteral("SELECT book_tag.id_book, book_tag.id_tag FROM book_tag INNER JOIN book ON book.id = book_tag.id_book WHERE book.id_lib = :id_lib"));
    query.bindValue(QStringLiteral(":id_lib"),idLibrary);
    if(!query.exec())
        qDebug() << query.lastError().text();
    while (query.next()) {
        uint idBook = query.value(0).toUInt();
        uint idTag = query.value(1).toUInt();
        if(lib.mBooks.contains(idBook))
            lib.mBooks[idBook].listIdTags << idTag;
    }
    lib.bLoaded = true;

    t_end = QDateTime::currentMSecsSinceEpoch();
    qDebug()<< "loadBooks " << t_end-t_start << "msec";
}

void loadGenres()
{
    if(!QSqlDatabase::database(QStringLiteral("libdb"), false).isOpen())
        return;
    qint64 t_start = QDateTime::currentMSecsSinceEpoch();
    QSqlQuery query(QSqlDatabase::database(QStringLiteral("libdb")));

    mGenre.clear();
    query.prepare(QStringLiteral("SELECT id, name, id_parent, sort_index FROM genre;"));
    //                                   0   1     2          3
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

SAuthor::SAuthor(const QString &sName)
{
    QStringList listNames = sName.split(QStringLiteral(","));
    if(listNames.count() > 0)
        sLastName = listNames[0].trimmed();
    if(listNames.count() > 1)
        sFirstName = listNames[1].trimmed();
    if(listNames.count() > 2)
        sMiddleName = listNames[2].trimmed();

}

QString SAuthor::getName() const
{
    QString sAuthorName = QStringLiteral("%1 %2 %3").arg(sLastName, sFirstName, sMiddleName).trimmed();
    if(sAuthorName.trimmed().isEmpty())
        sAuthorName = QCoreApplication::translate("MainWindow","unknown author");
    return sAuthorName;
}

uint SLib::findAuthor(SAuthor &author) const
{
    uint idAuthor = 0;
    auto iAuthor = mAuthors.constBegin();
    while(iAuthor != mAuthors.constEnd() ){
        if(author.sFirstName == iAuthor->sFirstName && author.sMiddleName == iAuthor->sMiddleName && author.sLastName == iAuthor->sLastName){
            idAuthor = iAuthor.key();
            break;
        }
        iAuthor++;
    }
    return idAuthor;
}

uint SLib::findSerial(const QString &sSerial) const
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
    QBuffer buffer;
    getBookFile(idBook, &buffer);
    SBook& book = mBooks[idBook];

    if(book.sFormat == QLatin1String("epub")){
        QuaZip zip(&buffer);
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
        for(int i=0;i<root.childNodes().count() && need_loop;i++)
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
                            if(meta.childNodes().at(m).nodeName().right(11) == QLatin1String("description"))
                            {
                                QBuffer buff;
                                buff.open(QIODevice::WriteOnly);
                                QTextStream ts(&buff);
                                meta.childNodes().at(m).save(ts, 0, QDomNode::EncodingFromTextStream);
                                book.sAnnotation = QString::fromUtf8(buff.data().data());
                            }
                            else if(meta.childNodes().at(m).nodeName().right(4) == QLatin1String("meta"))
                            {
                                if(meta.childNodes().at(m).attributes().namedItem(QStringLiteral("name")).toAttr().value() == QLatin1String("cover"))
                                {
                                    QString cover = meta.childNodes().at(m).attributes().namedItem(QStringLiteral("content")).toAttr().value();
                                    QDomNode manifest = opf.documentElement().namedItem(QStringLiteral("manifest"));
                                    for(int man=0; man<manifest.childNodes().count(); man++)
                                    {
                                        if(manifest.childNodes().at(man).attributes().namedItem(QStringLiteral("id")).toAttr().value()==cover)
                                        {
                                            cover = rel_path + QLatin1String("/") + manifest.childNodes().at(man).attributes().namedItem(QStringLiteral("href")).toAttr().value();

                                            SetCurrentZipFileName(&zip, cover);
                                            zip_file.open(QIODevice::ReadOnly);
                                            QByteArray ba = zip_file.readAll();
                                            zip_file.close();

                                            book.sImg = QStringLiteral("%1/freeLib/%2.jpg")
                                                    .arg(QStandardPaths::standardLocations(QStandardPaths::TempLocation).constFirst())
                                                    .arg(idBook);
                                            QFile fileImage(book.sImg);
                                            if(fileImage.open(QFile::WriteOnly)){
                                                fileImage.write(ba);
                                                fileImage.close();
                                            }
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                        need_loop = false;
                    }
                }
            }
        }
        zip.close();
        info.close();
    }else if(book.sFormat == QLatin1String("fb2")){
        QDomDocument doc;
        doc.setContent(buffer.data());
        QDomElement title_info = doc.elementsByTagName(QStringLiteral("title-info")).at(0).toElement();
        QString cover  = title_info.elementsByTagName(QStringLiteral("coverpage")).at(0).toElement().elementsByTagName(QStringLiteral("image")).at(0).attributes().namedItem(QStringLiteral("l:href")).toAttr().value();
        if(!cover.isEmpty() && cover.at(0) == QLatin1String("#"))
        {
            QDomNodeList binarys = doc.elementsByTagName(QStringLiteral("binary"));
            for(int i=0; i<binarys.count(); i++)
            {
                if(binarys.at(i).attributes().namedItem(QStringLiteral("id")).toAttr().value() == cover.right(cover.length() - 1))
                {
                    book.sImg = QStringLiteral("%1/freeLib/%2.jpg")
                            .arg(QStandardPaths::standardLocations(QStandardPaths::TempLocation).constFirst())
                            .arg(idBook);
                    QByteArray ba64;
                    ba64.append(binarys.at(i).toElement().text().toLatin1());
                    QByteArray ba = QByteArray::fromBase64(ba64);
                    QFile fileImage(book.sImg);
                    if(fileImage.open(QIODevice::ReadWrite)){
                        fileImage.write(ba);
                        fileImage.close();
                    }
                    break;
                }
            }
        }
        QBuffer buff;
        buff.open(QIODevice::WriteOnly);
        QTextStream ts(&buff);
        title_info.elementsByTagName(QStringLiteral("annotation")).at(0).save(ts, 0, QDomNode::EncodingFromTextStream);
        book.sAnnotation = QString::fromUtf8(buff.data().data());
        book.sAnnotation.replace(QLatin1String("<annotation>"), QLatin1String(""), Qt::CaseInsensitive);
        book.sAnnotation.replace(QLatin1String("</annotation>"), QLatin1String(""), Qt::CaseInsensitive);
    }
}

QFileInfo SLib::getBookFile(uint idBook, QBuffer *pBuffer, QBuffer *pBufferInfo, QDateTime *fileData)
{
    QString file,archive;
    QFileInfo fi;
    SBook &book = mBooks[idBook];
    QString LibPath = RelativeToAbsolutePath(path);
    if(book.sArchive.isEmpty()){
        file = QStringLiteral("%1/%2.%3").arg(LibPath, book.sFile, book.sFormat);
    }else{
        file = QStringLiteral("%1.%2").arg(book.sFile, book.sFormat);
        QString sArchive = book.sArchive;
        sArchive.replace(QLatin1String(".inp"), QLatin1String(".zip"));
        archive = QStringLiteral("%1/%2").arg(LibPath, sArchive);
    }

    archive = archive.replace('\\', '/');
    if(archive.isEmpty())
    {
        QFile book_file(file);
        if(!book_file.open(QFile::ReadOnly))
        {
            qDebug()<<("Error open file!")<<" "<<file;
            return fi;
        }
        if(pBuffer != nullptr)
            pBuffer->setData(book_file.readAll());
        fi.setFile(book_file);
        if(fileData)
        {
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
            *fileData = fi.birthTime();
#else
            *fileData = fi.created();
#endif
        }

        if(pBufferInfo != nullptr){
            fi.setFile(file);
            QString fbd = fi.absolutePath() + QLatin1String("/") + fi.completeBaseName() + QLatin1String(".fbd");
            QFile info_file(fbd);
            if(info_file.exists())
            {
                info_file.open(QFile::ReadOnly);
                pBufferInfo->setData(info_file.readAll());
            }
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

        if(fileData)
        {
            SetCurrentZipFileName(&uz, file);
            QuaZipFileInfo64 zip_fi;
            if(uz.getCurrentFileInfo(&zip_fi))
            {
                *fileData = zip_fi.dateTime;
            }
        }
        QuaZipFile zip_file(&uz);
        SetCurrentZipFileName(&uz,file);
        if(!zip_file.open(QIODevice::ReadOnly))
        {
            qDebug()<<"Error open file: "<<file;
        }
        if(pBuffer != nullptr)
        {
            pBuffer->setData(zip_file.readAll());
        }
        zip_file.close();

        if(pBufferInfo != nullptr){
            fi.setFile(file);
            QString fbd = fi.path() + QLatin1String("/") + fi.completeBaseName() + QLatin1String(".fbd");
            if(SetCurrentZipFileName(&uz, fbd))
            {
                zip_file.open(QIODevice::ReadOnly);
                pBufferInfo->setData(zip_file.readAll());
                zip_file.close();
            }
        }

        fi.setFile(archive + QLatin1String("/") + file);
    }
    return fi;
}

/*
 *
%a - автор книги.
%b - заголовок книги.
%s - серия.
%nx - номер в серии, где x-число знаков.
%abbrs - аббревиатура серии.
%nl - фамилия.
%nm - отчество.
%nf - имя.
%li,%fi,%mi - первые буквы фамилии,имени,отчества.
%f - полный путь к файлу книги с именем файла.
%fn - имя файла книги без пути и расширения.
%d - полный путь к файлу книги.
%app_dir - путь к директории программы.

*/

QString SLib::fillParams(const QString &str, uint idBook)
{
    SBook& book = mBooks[idBook];
    QString result = str;
    QString abbr = QLatin1String("");
    foreach(const QString &str, mSerials[book.idSerial].sName.split(QStringLiteral(" ")))
    {
        if(!str.isEmpty())
            abbr += str.at(0);
    }
    result.replace(QLatin1String("%abbrs"), abbr.toLower());
    result.replace(QLatin1String("%app_dir"), QApplication::applicationDirPath() + QLatin1String("/"));

    //result.removeOne("%no_point");
    const SAuthor& sFirstAuthor = mAuthors[book.idFirstAuthor];

    result.replace(QLatin1String("%fi"), sFirstAuthor.sFirstName.left(1) + (sFirstAuthor.sFirstName.isEmpty() ?QLatin1String("") :QLatin1String("."))).
            replace(QLatin1String("%mi"), sFirstAuthor.sMiddleName.left(1) + (sFirstAuthor.sMiddleName.isEmpty() ?QLatin1String("") :QLatin1String("."))).
            replace(QLatin1String("%li"), sFirstAuthor.sLastName.left(1) + (sFirstAuthor.sLastName.isEmpty() ?QLatin1String("") :QLatin1String("."))).
            replace(QLatin1String("%nf"), sFirstAuthor.sFirstName).
            replace(QLatin1String("%nm"), sFirstAuthor.sMiddleName).
            replace(QLatin1String("%nl"), sFirstAuthor.sLastName);

    result = result.replace(QLatin1String("%s"), mSerials[book.idSerial].sName)
            .replace(QLatin1String("%b"), book.sName)
            .replace(QLatin1String("%a"), sFirstAuthor.getName())
            .replace(QLatin1String(","), QLatin1String(" ")).trimmed();
    QString num_in_seria = QString::number(book.numInSerial);
    if(result.contains(QLatin1String("%n")))
    {
        int len = result.mid(result.indexOf(QLatin1String("%n")) + 2, 1).toInt();
        QString zerro;
        if(book.numInSerial == 0)
            result.replace("%n" + QString::number(len), QLatin1String(""));
        else
            result.replace(QLatin1String("%n") + (len>0 ?QString::number(len) :QLatin1String("")),
                           (len>0 ?zerro.fill(u'0', len-num_in_seria.length()) :QLatin1String("")) + num_in_seria + QLatin1String(" "));
    }
    result.replace(QLatin1String("/ "), QLatin1String("/"));
    result.replace(QLatin1String("/."), QLatin1String("/"));
    result.replace(QLatin1String("////"), QLatin1String("/"));
    result.replace(QLatin1String("///"), QLatin1String("/"));
    result.replace(QLatin1String("//"), QLatin1String("/"));
    return result;
}

QString SLib::fillParams(const QString &str, uint idBook, const QFileInfo &book_file)
{
    QString result = str;
    result.replace(QLatin1String("%fn"), book_file.completeBaseName()).
           replace(QLatin1String("%f"), book_file.absoluteFilePath()).
           replace(QLatin1String("%d"), book_file.absoluteDir().path());
    result = fillParams(result, idBook);
    return result;
}

void SLib::deleteTag(uint idTag)
{
    auto iBook = mBooks.begin();
    while(iBook != mBooks.end()){
        iBook->listIdTags.removeOne(idTag);
        ++iBook;
    }
    auto iSerial = mSerials.begin();
    while(iSerial != mSerials.end()){
        iSerial->listIdTags.removeOne(idTag);
        ++iSerial;
    }
    auto iAuthor = mAuthors.begin();
    while(iAuthor != mAuthors.end()){
        iAuthor->listIdTags.removeOne(idTag);
        ++iAuthor;
    }
    QSqlQuery query(QSqlDatabase::database(QStringLiteral("libdb")));
    query.exec(QStringLiteral("DELETE FROM book_tag WHERE id_tag=%1").arg(idTag));
    query.exec(QStringLiteral("DELETE FROM seria_tag WHERE id_tag=%1").arg(idTag));
    query.exec(QStringLiteral("DELETE FROM author_tag WHERE id_tag=%1").arg(idTag));
    query.exec(QStringLiteral("DELETE FROM tag WHERE id=%1").arg(idTag));
}

QString SLib::nameFromInpx(QString sInpx)
{
    QString sName;
    if(!sInpx.isEmpty()){
        QuaZip uz(sInpx);
        if(!uz.open(QuaZip::mdUnzip))
        {
            qDebug()<<"Error open INPX file: " << sInpx;
            //return sName;
        } else
        if(SetCurrentZipFileName(&uz, QStringLiteral("COLLECTION.INFO")))
        {
            QBuffer outbuff;
            QuaZipFile zip_file(&uz);
            zip_file.open(QIODevice::ReadOnly);
            outbuff.setData(zip_file.readAll());
            zip_file.close();
            sName = QString::fromUtf8(outbuff.data().left(outbuff.data().indexOf('\n')));
        }
    }
    return sName;
}

