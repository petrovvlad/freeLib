#define QT_USE_QSTRINGBUILDER
#include "library.h"

#include <algorithm>
#include <QDomDocument>
#include <QStringBuilder>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QApplication>
#include <QStandardPaths>
#include <QDir>
#include <QtConcurrent>

#ifdef QUAZIP_STATIC
#include "quazip/quazip/quazipfile.h"
#else
#include <quazip/quazipfile.h>
#endif

#include "utilites.h"
#include "options.h"
#ifdef USE_DEJVULIBRE
#include "djvu.h"
#endif

QMap<uint, SLib> libs;
QMap <ushort, SGenre> genres;
QString RelativeToAbsolutePath(QString path);
extern bool bVerbose;

void loadLibrary(uint idLibrary)
{
    if(idLibrary == 0)
        return;
    if(!QSqlDatabase::database(QStringLiteral("libdb"), false).isOpen())
        return;

    qint64 timeStart, timeEnd;
    if(bVerbose)
        timeStart = QDateTime::currentMSecsSinceEpoch();

    QSqlQuery query(QSqlDatabase::database(QStringLiteral("libdb")));
    query.setForwardOnly(true);
    SLib& lib = libs[idLibrary];
    auto future = QtConcurrent::run([idLibrary, &lib]()
    {
        {
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
            QSqlDatabase dbReadAuthors = QSqlDatabase::cloneDatabase(QStringLiteral("libdb"), QStringLiteral("readauthors"));
#else
            QFileInfo fi(RelativeToAbsolutePath(options.sDatabasePath));
            QString sDbFile = fi.canonicalFilePath();
            QSqlDatabase dbReadAuthors = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("readauthors"));
            dbReadAuthors.setDatabaseName(sDbFile);
#endif
            lib.authors.clear();
            lib.authors.insert(0, SAuthor());
            if (!dbReadAuthors.open())
            {
                MyDBG << dbReadAuthors.lastError().text();
                return;
            }
            QSqlQuery query(dbReadAuthors);
            query.setForwardOnly(true);

            lib.serials.clear();
            query.prepare(QStringLiteral("SELECT id, name FROM seria WHERE id_lib=:id_lib;"));
            //                                   0   1
            query.bindValue(QStringLiteral(":id_lib"), idLibrary);
            if(!query.exec())
                qDebug() << query.lastError().text();
            while (query.next()) {
                uint idSerial = query.value(0).toUInt();
                QString sName = query.value(1).toString();
                lib.serials[idSerial].sName = sName;
            }
            query.prepare(QStringLiteral("SELECT seria_tag.id_seria, seria_tag.id_tag FROM seria_tag INNER JOIN seria ON seria.id = seria_tag.id_seria WHERE seria.id_lib = :id_lib"));
            query.bindValue(QStringLiteral(":id_lib"),idLibrary);
            if(!query.exec())
                qDebug() << query.lastError().text();
            while (query.next()) {
                uint idSeria = query.value(0).toUInt();
                uint idTag = query.value(1).toUInt();
                if(lib.serials.contains(idSeria))
                    lib.serials[idSeria].listIdTags << idTag;
            }

            query.prepare(QStringLiteral("SELECT author.id, name1, name2, name3 FROM author WHERE id_lib=:id_lib;"));
            //                                     0          1      2      3
            query.bindValue(QStringLiteral(":id_lib"), idLibrary);
            query.exec();
            while (query.next()) {
                uint idAuthor = query.value(0).toUInt();
                SAuthor &author = lib.authors[idAuthor];
                author.sFirstName = query.value(2).toString().trimmed();;
                author.sLastName = query.value(1).toString().trimmed();;
                author.sMiddleName = query.value(3).toString().trimmed();;
            }
            query.prepare(QStringLiteral("SELECT author_tag.id_author, author_tag.id_tag FROM author_tag INNER JOIN author ON author.id = author_tag.id_author WHERE author.id_lib = :id_lib"));
            query.bindValue(QStringLiteral(":id_lib"), idLibrary);
            if(!query.exec())
                qDebug() << query.lastError().text();
            while (query.next()) {
                uint idAuthor = query.value(0).toUInt();
                uint idTag = query.value(1).toUInt();
                if(lib.authors.contains(idAuthor))
                    lib.authors[idAuthor].listIdTags << idTag;
            }
        }
        QSqlDatabase::removeDatabase(QStringLiteral("readauthors"));
    });

#ifdef __cpp_lib_atomic_wait
    std::atomic_int_fast32_t anCount[2] = {0, 0};
    std::atomic_uint anTotalCount{0};
    std::atomic_bool abFinished{false};

    const uint sizeBufffer{30000};
    QVariant *buff = new QVariant[15*sizeBufffer*2];

    auto futureReadBooks = QtConcurrent::run([&]
    {
        {
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
            QSqlDatabase dbReadBooks = QSqlDatabase::cloneDatabase(QStringLiteral("libdb"), QStringLiteral("readdb"));
#else
            QFileInfo fi(RelativeToAbsolutePath(options.sDatabasePath));
            QString sDbFile = fi.canonicalFilePath();
            QSqlDatabase dbReadBooks = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("importdb"));
            dbReadBooks.setDatabaseName(sDbFile);
#endif
            uint nBuff = 0;
            if (!dbReadBooks.open())
            {
                MyDBG << dbReadBooks.lastError().text();
                return;
            }
            QSqlQuery query(dbReadBooks);
            query.setForwardOnly(true);
            query.prepare(QStringLiteral("SELECT id, name, star, id_seria, num_in_seria, language, file, size, deleted, date, format, id_inlib, archive, first_author_id, keys FROM book WHERE id_lib=:id_lib;"));
            //                                    0  1     2     3         4             5         6     7     8        9     10      11        12       13               14
            query.bindValue(QStringLiteral(":id_lib"), idLibrary);
            if(!query.exec())
                MyDBG << query.lastError().text();
            uint i = 0;
            while (query.next()) {
                anCount[nBuff].wait(sizeBufffer);
                int k = nBuff * 15 * sizeBufffer + i * 15;
                for(int iValue = 0; iValue<=14; iValue++){
                    buff[k + iValue] = query.value(iValue);
                }
                if(++i == sizeBufffer){
                    i = 0;
                    anTotalCount += sizeBufffer;
                    anCount[nBuff].store(sizeBufffer);
                    anCount[nBuff].notify_one();
                    nBuff = (nBuff==0) ?1 :0;
                }
            }
            anTotalCount += i;
            anCount[nBuff].store(i);
            anCount[nBuff].notify_all();
            abFinished.store(true);
        }
        QSqlDatabase::removeDatabase(QStringLiteral("readdb"));
    });

    lib.books.clear();
    uint nBuff = 0;
    uint nTotalCount{0};
    while(!abFinished || nTotalCount != anTotalCount){
        anCount[nBuff].wait(-1);
        uint nCount = anCount[nBuff].load();
        for(uint i =0; i<nCount; i++){
            nTotalCount++;
            uint k = nBuff * 15 * sizeBufffer + i * 15;
            QString sName = buff[k + 1].toString();
            if(sName.isEmpty())
                continue;
            uint id = buff[k].toUInt();
            SBook &book = lib.books[id];
            book.sName = sName;
            book.nStars = qvariant_cast<uchar>(buff[k+2]);
            book.idSerial = buff[k + 3].toUInt();
            book.numInSerial = buff[k + 4].toUInt();
            QString sLaguage = buff[k + 5].toString().toLower();
            int idLaguage = lib.vLaguages.indexOf(sLaguage);
            if(idLaguage < 0){
                idLaguage = lib.vLaguages.count();
                lib.vLaguages << sLaguage;
            }
            book.idLanguage = static_cast<uchar>(idLaguage);
            book.sFile = buff[k + 6].toString();
            book.nSize = buff[k + 7].toUInt();
            book.bDeleted = buff[k + 8].toBool();
            book.date = buff[k + 9].toDate();
            book.sFormat = buff[k + 10].toString();
            book.idInLib = buff[k + 11].toUInt();
            book.sArchive = buff[k + 12].toString();
            book.idFirstAuthor = buff[k + 13].toUInt();
            book.sKeywords = buff[k + 14].toString();
        }
        anCount[nBuff].store(-1);
        anCount[nBuff].notify_all();
        nBuff = (nBuff==0) ?1 :0;
    }
    delete[] buff;
#else
    lib.books.clear();
    query.prepare(QStringLiteral("SELECT id, name, star, id_seria, num_in_seria, language, file, size, deleted, date, format, id_inlib, archive, first_author_id, keys FROM book WHERE id_lib=:id_lib;"));
    //                                    0  1     2     3         4             5         6     7     8        9     10      11        12       13               14
    query.bindValue(QStringLiteral(":id_lib"),idLibrary);
    if(!query.exec())
        qDebug() << query.lastError().text();
    while (query.next()) {
        QString sName = query.value(1).toString();
        if(sName.isEmpty())
            continue;
        uint id = query.value(0).toUInt();
        SBook &book = lib.books[id];
        book.sName = sName;
        book.nStars = qvariant_cast<uchar>(query.value(2));
        book.idSerial = query.value(3).toUInt();
        book.numInSerial = query.value(4).toUInt();
        QString sLaguage = query.value(5).toString().toLower();
        int idLaguage = lib.vLaguages.indexOf(sLaguage);
        if(idLaguage < 0){
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
        book.sKeywords = query.value(14).toString();
    }
#endif
    future.waitForFinished();

    future = QtConcurrent::run([idLibrary, &lib]()
    {
        {
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
            QSqlDatabase dbReadDB = QSqlDatabase::cloneDatabase(QStringLiteral("libdb"), QStringLiteral("readdb"));
#else
            QFileInfo fi(RelativeToAbsolutePath(options.sDatabasePath));
            QString sDbFile = fi.canonicalFilePath();
            QSqlDatabase dbReadDB = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("importdb"));
            dbReadDB.setDatabaseName(sDbFile);
#endif
            if (!dbReadDB.open())
            {
                MyDBG << dbReadDB.lastError().text();
                return;
            }
            QSqlQuery query(dbReadDB);
            query.setForwardOnly(true);

            query.prepare(QStringLiteral("SELECT id_book, id_genre FROM book_genre WHERE id_lib=:id_lib;"));
            //                                   0        1
            query.bindValue(QStringLiteral(":id_lib"), idLibrary);
            if(!query.exec())
                qDebug() << query.lastError().text();
            while (query.next()) {
                uint idBook = query.value(0).toUInt();
                ushort idGenre = query.value(1).toUInt();
                if(idGenre == 0) idGenre = 1112; // Прочие/Неотсортированное
                if(lib.books.contains(idBook))
                    lib.books[idBook].listIdGenres << idGenre;
            }
            query.prepare(QStringLiteral("SELECT book_tag.id_book, book_tag.id_tag FROM book_tag INNER JOIN book ON book.id = book_tag.id_book WHERE book.id_lib = :id_lib"));
            query.bindValue(QStringLiteral(":id_lib"),idLibrary);
            if(!query.exec())
                qDebug() << query.lastError().text();
            while (query.next()) {
                uint idBook = query.value(0).toUInt();
                uint idTag = query.value(1).toUInt();
                if(lib.books.contains(idBook))
                    lib.books[idBook].listIdTags << idTag;
            }
        }
        QSqlDatabase::removeDatabase(QStringLiteral("readdb"));
    });

    lib.authorBooksLink.clear();
    query.prepare(QStringLiteral("SELECT id_book, id_author FROM book_author WHERE id_lib=:id_lib;"));
    //                                     0       1
    query.bindValue(QStringLiteral(":id_lib"), idLibrary);
    if(!query.exec())
        qDebug() << query.lastError().text();
    while (query.next()) {
        uint idBook = query.value(0).toUInt();
        uint idAuthor = query.value(1).toUInt();
        if(lib.books.contains(idBook) && lib.authors.contains(idAuthor)){
            lib.authorBooksLink.insert(idAuthor,idBook);
            lib.books[idBook].listIdAuthors << idAuthor;
        }
    }
    auto iBook = lib.books.begin();
    while(iBook != lib.books.end()){
        if(iBook->listIdAuthors.isEmpty()){
            iBook->listIdAuthors << 0;
            lib.authorBooksLink.insert(0, iBook.key());
        }
        ++iBook;
    }

#ifdef __cpp_lib_atomic_wait
    futureReadBooks.waitForFinished();
#endif
    future.waitForFinished();
    lib.bLoaded = true;

    if(bVerbose){
        timeEnd = QDateTime::currentMSecsSinceEpoch();
        qDebug()<< "loadLibrary " << timeEnd - timeStart << "msec";
    }
}

void loadGenres()
{
    if(!QSqlDatabase::database(QStringLiteral("libdb"), false).isOpen())
        return;
    qint64 timeStart;
    if(bVerbose)
        timeStart = QDateTime::currentMSecsSinceEpoch();
    QSqlQuery query(QSqlDatabase::database(QStringLiteral("libdb")));

    genres.clear();
    query.prepare(QStringLiteral("SELECT id, name, id_parent, sort_index, keys FROM genre;"));
    //                                   0   1     2          3           4
    if(!query.exec())
        qDebug() << query.lastError().text();
    while (query.next()) {
        ushort idGenre = static_cast<ushort>(query.value(0).toUInt());
        SGenre &genre = genres[idGenre];
        genre.sName = query.value(1).toString();
        genre.idParrentGenre = static_cast<ushort>(query.value(2).toUInt());
        QString sKeys = query.value(4).toString();
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        genre.listKeys = sKeys.split(';', Qt::SkipEmptyParts);
#else
        genre.listKeys = sKeys.split(';');
#endif
        genre.nSort = static_cast<ushort>(query.value(3).toUInt());
    }
    if(bVerbose){
        qint64 timeEnd = QDateTime::currentMSecsSinceEpoch();
        qDebug()<< "loadGenre " << timeEnd - timeStart << "msec";
    }
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
    auto iAuthor = authors.constBegin();
    while(iAuthor != authors.constEnd() ){
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
    auto iSerial = serials.constBegin();
    while(iSerial != serials.constEnd()){
        if(sSerial == iSerial->sName){
            idSerial = iSerial.key();
            break;
        }
        iSerial++;
    }
    return idSerial;
}

void cleanCache()
{
    qint64 nSizeDir = 0;
    static QStringList nameFilters = {u"*.jpg"_s};
    QDir dirCovers(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + u"/covers"_s);
    QFileInfoList fiList = dirCovers.entryInfoList(nameFilters, QDir::Files, QDir::Time);
    uint nCount = fiList.count();
    uint i=0;
    if(options.bOpdsEnable){
        uint nCountOpds = std::min(nCount, options.nOpdsBooksPerPage*2U);
        for(; i<nCountOpds; i++)
            nSizeDir += fiList[i].size();
    }
    for(; i<nCount; i++){
        const auto &fi = fiList[i];
        nSizeDir += fi.size();
        if(nSizeDir > options.nCacheSize && i>1)
            QFile::remove(fi.absoluteFilePath());
    }

}

void SLib::loadAnnotationAndCover(uint idBook)
{
    QBuffer buffer;
    getBookFile(idBook, &buffer);
    SBook& book = books[idBook];
    QString sImg = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + u"/covers/"_s + QString::number(idBook) + u".jpg"_s;

    if(book.sFormat == u"epub"){
        QuaZip zip(&buffer);
        zip.open(QuaZip::mdUnzip);
        QBuffer info;
        setCurrentZipFileName(&zip, u"META-INF/container.xml"_s);
        QuaZipFile zip_file(&zip);
        zip_file.open(QIODevice::ReadOnly);
        info.setData(zip_file.readAll());
        zip_file.close();
        QDomDocument doc;
        doc.setContent(info.data());
        QDomNode root = doc.documentElement();
        bool need_loop = true;
        QString rel_path;
        for(int i=0; i<root.childNodes().count() && need_loop; i++)
        {
            if(root.childNodes().at(i).nodeName().toLower() == u"rootfiles")
            {
                QDomNode roots = root.childNodes().at(i);
                for(int j=0; j<roots.childNodes().count() && need_loop; j++)
                {
                    if(roots.childNodes().at(j).nodeName().toLower() == u"rootfile")
                    {
                        QString path = roots.childNodes().at(j).attributes().namedItem(u"full-path"_s).toAttr().value();
                        QBuffer opf_buf;
                        QFileInfo fi(path);
                        rel_path = fi.path();
                        setCurrentZipFileName(&zip, path);
                        zip_file.open(QIODevice::ReadOnly);
                        opf_buf.setData(zip_file.readAll());
                        zip_file.close();

                        QDomDocument opf;
                        opf.setContent(opf_buf.data());
                        QDomNode meta = opf.documentElement().namedItem(u"metadata"_s);
                        for(int m=0; m<meta.childNodes().count(); m++)
                        {
                            if(meta.childNodes().at(m).nodeName().right(11) == u"description")
                            {
                                book.sAnnotation = meta.childNodes().at(m).toElement().text();
                            }
                            else if(meta.childNodes().at(m).nodeName().right(4) == u"meta")
                            {
                                if(meta.childNodes().at(m).attributes().namedItem(u"name"_s).toAttr().value() == u"cover")
                                {
                                    if(!QFile::exists(sImg))
                                    {
                                        QString cover = meta.childNodes().at(m).attributes().namedItem(u"content"_s).toAttr().value();
                                        QDomNode manifest = opf.documentElement().namedItem(u"manifest"_s);
                                        for(int man=0; man<manifest.childNodes().count(); man++)
                                        {
                                            if(manifest.childNodes().at(man).attributes().namedItem(u"id"_s).toAttr().value() == cover)
                                            {
                                                cover = rel_path + u"/"_s + manifest.childNodes().at(man).attributes().namedItem(u"href"_s).toAttr().value();

                                                setCurrentZipFileName(&zip, cover);
                                                zip_file.open(QIODevice::ReadOnly);
                                                QByteArray ba = zip_file.readAll();
                                                zip_file.close();

                                                book.sImg = sImg;
                                                QFile fileImage(book.sImg);
                                                if(fileImage.open(QFile::WriteOnly)){
                                                    fileImage.write(ba);
                                                    fileImage.close();
                                                    cleanCache();
                                                }
                                                break;
                                            }
                                        }
                                    }else
                                        book.sImg = sImg;
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
    }else if(book.sFormat == u"fb2"){
        QDomDocument doc;
        doc.setContent(buffer.data());
        QDomElement title_info = doc.elementsByTagName(u"title-info"_s).at(0).toElement();
        if(!QFile::exists(sImg))
        {
            QString cover  = title_info.elementsByTagName(u"coverpage"_s).at(0).toElement().elementsByTagName(u"image"_s).at(0).attributes().namedItem(u"l:href"_s).toAttr().value();
            if(!cover.isEmpty() && cover.at(0) == u'#')
            {
                QDomNodeList binarys = doc.elementsByTagName(u"binary"_s);
                for(int i=0; i<binarys.count(); i++)
                {
                    if(binarys.at(i).attributes().namedItem(QStringLiteral("id")).toAttr().value() == cover.right(cover.length() - 1))
                    {
                        book.sImg = sImg;
                        QByteArray ba64;
                        ba64.append(binarys.at(i).toElement().text().toLatin1());
                        QByteArray ba = QByteArray::fromBase64(ba64);
                        QFile fileImage(book.sImg);
                        if(fileImage.open(QIODevice::ReadWrite)){
                            fileImage.write(ba);
                            fileImage.close();
                            cleanCache();
                        }
                        break;
                    }
                }
            }
        }else
            book.sImg = sImg;
        book.sAnnotation = title_info.elementsByTagName(u"annotation"_s).at(0).toElement().text();
    }
#ifdef USE_DEJVULIBRE
    else if(book.sFormat == u"djvu" || book.sFormat == u"djv"){
        if(!QFile::exists(sImg)){
            QString sDjVuFile = QDir::tempPath() + u"/book.djvu"_s;
            QFile fileDjVu(sDjVuFile);
            if(!fileDjVu.open(QIODevice::ReadWrite))
                return;
            fileDjVu.write(buffer.buffer());
            fileDjVu.close();
            DjVu djvu;
            if(djvu.openDocument(sDjVuFile)){
                QImage cover = djvu.getCover();
                if(!cover.isNull()){
                    book.sImg = sImg;
                    cover.save(sImg);
                    cleanCache();
                }
            }
            fileDjVu.remove();
        }else
            book.sImg = sImg;
    }
#endif

}

QFileInfo SLib::getBookFile(uint idBook, QBuffer *pBuffer, QBuffer *pBufferInfo, QDateTime *fileData)
{
    QString file, archive;
    QFileInfo fi;
    SBook &book = books[idBook];
    QString LibPath = RelativeToAbsolutePath(path);
    bool bZipInZip = false;
    if(book.sArchive.isEmpty()){
        file = QStringLiteral("%1/%2.%3").arg(LibPath, book.sFile, book.sFormat);
    }else{
        file = QStringLiteral("%1.%2").arg(book.sFile, book.sFormat);
        QString sArchive = book.sArchive;
        sArchive.replace(QStringLiteral(".inp"), QStringLiteral(".zip"));
        archive = QStringLiteral("%1/%2").arg(LibPath, sArchive);

        if(sArchive.contains(QStringLiteral(".zip/")))
            bZipInZip = true;
    }

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
            QString fbd = fi.absolutePath() + QStringLiteral("/") + fi.completeBaseName() + QStringLiteral(".fbd");
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
        if(bZipInZip){
            QString sTempZipDir = QDir::tempPath() + QStringLiteral("/freeLib/zip/") ;

            QStringList sZipChain;
            qsizetype posNext = 0;
            qsizetype posPrev = 0;
            while(posNext != -1){
                posNext = book.sArchive.indexOf(QStringLiteral(".zip"), posPrev);
                if(posNext != -1){
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                    sZipChain << book.sArchive.sliced(posPrev, posNext + 4 - posPrev);
#else
                    QStringRef sZip = book.sArchive.leftRef(posNext + 4);
                    sZipChain << sZip.right(sZip.length() - posPrev).toString();
#endif

                    posPrev = posNext + 5;
                }
            }
            for(int i=1; i<sZipChain.count(); i++){
                if(i==1){
                    sTempZipDir = QDir::tempPath() + QStringLiteral("/freeLib/zip/");
                    archive = QStringLiteral("%1/%2").arg(LibPath, sZipChain[0]);
                }else{
                    archive = QDir::tempPath() + QStringLiteral("/freeLib/zip");
                    for(int j=0; j<i; j++)
                        archive += QStringLiteral("/") + sZipChain[j];
                    archive += QStringLiteral(".tmp");
                }
                sTempZipDir += sZipChain[i-1] + QStringLiteral("/");
                QDir dirZip(sTempZipDir);
                dirZip.mkpath(sTempZipDir);
                QuaZip uz(archive);
                if (!uz.open(QuaZip::mdUnzip))
                {
                    qDebug() << "Error open archive! " << archive;
                    return fi;
                }
                QuaZipFile zip_file(&uz);
                setCurrentZipFileName(&uz, sZipChain[i]);
                if(!zip_file.open(QIODevice::ReadOnly))
                {
                    qDebug()<<"Error open file: " << sZipChain[i];
                }
                QBuffer buffer;
                buffer.setData(zip_file.readAll());
                zip_file.close();
                archive = sTempZipDir + sZipChain[i] + ".tmp";
                QFile tempZipFile(archive);
                tempZipFile.open(QIODevice::WriteOnly);
                tempZipFile.write(buffer.data());
                tempZipFile.close();
            }
        }
        QuaZip uz(archive);
        if (!uz.open(QuaZip::mdUnzip))
        {
            qDebug() << "Error open archive! " << archive;
            return fi;
        }

        if(fileData)
        {
            setCurrentZipFileName(&uz, file);
            QuaZipFileInfo64 zip_fi;
            if(uz.getCurrentFileInfo(&zip_fi))
            {
                *fileData = zip_fi.dateTime;
            }
        }
        QuaZipFile zip_file(&uz);
        setCurrentZipFileName(&uz, file);
        if(!zip_file.open(QIODevice::ReadOnly))
        {
            qDebug()<<"Error open file: " << file;
        }
        if(pBuffer != nullptr)
        {
            pBuffer->setData(zip_file.readAll());
        }
        zip_file.close();

        if(pBufferInfo != nullptr){
            fi.setFile(file);
            QString fbd = fi.path() + QStringLiteral("/") + fi.completeBaseName() + QStringLiteral(".fbd");
            if(setCurrentZipFileName(&uz, fbd))
            {
                zip_file.open(QIODevice::ReadOnly);
                pBufferInfo->setData(zip_file.readAll());
                zip_file.close();
            }
        }

        fi.setFile(archive + QStringLiteral("/") + file);
        if(bZipInZip)
            QDir(QDir::tempPath() + QStringLiteral("/freeLib/zip/")).removeRecursively();
    }
    return fi;
}

/*

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

QString SLib::fillParams(const QString &str, uint idBook, bool bNestedBlock)
{
    QString result = str;
    for(qsizetype i=0; i<result.size()-1; i++){
        if(result.at(i) == u'['){
            uint numOpenBrackets = 0;
            for(qsizetype j=i+1 ;j<result.size(); j++){
                if(result.at(j) == u'[')
                    numOpenBrackets++;
                if(result.at(j) == u']'){
                    if(numOpenBrackets == 0){
                        uint nBlockSize = j - i - 1;
                        QString sReplace = fillParams(result.mid(i+1, nBlockSize), idBook, true);
                        result = result.left(i) % sReplace % result.right(result.size() - j - 1);
                        i = i + sReplace.size() - 1;
                        break;
                    }else{
                        numOpenBrackets--;
                    }
                }
            }
        }
    }

    SBook& book = books[idBook];

    const SAuthor& sFirstAuthor = authors[book.idFirstAuthor];

    if(result.contains(QStringLiteral("%s"))){
        if(book.idSerial == 0){
            if(bNestedBlock)
                return QStringLiteral("");
            else
                result.replace(QStringLiteral("%s"), QStringLiteral(""));

        }else {
            result.replace(QStringLiteral("%s"), serials[book.idSerial].sName);
        }
    }


    if(bNestedBlock){
        if(result.contains(QStringLiteral("%fi")) || result.contains(QStringLiteral("%nf")))
            if(sFirstAuthor.sFirstName.isEmpty())
                return QStringLiteral("");
        if(result.contains(QStringLiteral("%mi")) || result.contains(QStringLiteral("%nm")))
            if(sFirstAuthor.sMiddleName.isEmpty())
                return QStringLiteral("");
        if(result.contains(QStringLiteral("%li")) || result.contains(QStringLiteral("%nl")))
            if(sFirstAuthor.sLastName.isEmpty())
                return QStringLiteral("");
        if(result.contains(QStringLiteral("%abbrs")))
            if(book.idSerial == 0)
                return QStringLiteral("");
    }else{
        result.replace(QStringLiteral("%app_dir"), QApplication::applicationDirPath() + QStringLiteral("/"));

        QString abbr = QStringLiteral("");
        if(book.idSerial != 0){
            for(const QString &str: serials[book.idSerial].sName.split(QStringLiteral(" ")))
            {
                if(!str.isEmpty())
                    abbr += str.at(0);
            }
        }
        result.replace(QStringLiteral("%abbrs"), abbr.toLower());

        result.replace(QStringLiteral("%fi"), sFirstAuthor.sFirstName.isEmpty() ?QStringLiteral("") :(sFirstAuthor.sFirstName.at(0)));
        result.replace(QStringLiteral("%mi"), sFirstAuthor.sMiddleName.isEmpty() ?QStringLiteral("") :(sFirstAuthor.sMiddleName.at(0)));
        result.replace(QStringLiteral("%li"), sFirstAuthor.sLastName.isEmpty() ?QStringLiteral("") :(sFirstAuthor.sLastName.at(0)));
        result.replace(QStringLiteral("%nf"), sFirstAuthor.sFirstName);
        result.replace(QStringLiteral("%nm"), sFirstAuthor.sMiddleName);
        result.replace(QStringLiteral("%nl"), sFirstAuthor.sLastName);

        result.replace(QStringLiteral("%b"), book.sName);
        result.replace(QStringLiteral("%a"), sFirstAuthor.getName());
        result.replace(QStringLiteral("  "), QStringLiteral(" "));
        result.replace(QStringLiteral("/ "), QStringLiteral("/"));
        result.replace(QStringLiteral("/."), QStringLiteral("/"));
        result.replace(QStringLiteral("////"), QStringLiteral("/"));
        result.replace(QStringLiteral("///"), QStringLiteral("/"));
        result.replace(QStringLiteral("//"), QStringLiteral("/"));
    }

    auto in = result.indexOf(QStringLiteral("%n"));
    if(in >= 0)
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        int len = QStringView{result}.sliced(in + 2, 1).toInt();
#else
        int len = result.midRef(in + 2, 1).toInt();
#endif
        if(book.numInSerial == 0){
            if(bNestedBlock)
                return QStringLiteral("");
            else
                result.replace("%n" + QString::number(len), QStringLiteral(""));
        }else{
            QString sNumInSeria = QString::number(book.numInSerial);
            QString zerro;
            result.replace(QStringLiteral("%n") + (len>0 ?QString::number(len) :QStringLiteral("")),
                           (len>0 ?zerro.fill(u'0', len-sNumInSeria.length()) :QStringLiteral("")) + sNumInSeria);
        }
    }

    return result;
}

QString SLib::fillParams(const QString &str, uint idBook, const QFileInfo &book_file)
{
    QString result = str;
    result.replace(QStringLiteral("%fn"), book_file.completeBaseName()).
        replace(QStringLiteral("%f"), book_file.absoluteFilePath()).
        replace(QStringLiteral("%d"), book_file.absoluteDir().path());
    result = fillParams(result, idBook);
    return result;
}

void SLib::deleteTag(uint idTag)
{
    auto iBook = books.begin();
    while(iBook != books.end()){
        iBook->listIdTags.removeOne(idTag);
        ++iBook;
    }
    auto iSerial = serials.begin();
    while(iSerial != serials.end()){
        iSerial->listIdTags.removeOne(idTag);
        ++iSerial;
    }
    auto iAuthor = authors.begin();
    while(iAuthor != authors.end()){
        iAuthor->listIdTags.removeOne(idTag);
        ++iAuthor;
    }
    QSqlQuery query(QSqlDatabase::database(QStringLiteral("libdb")));
    query.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
    query.exec(QStringLiteral("DELETE FROM book_tag WHERE id_tag=%1").arg(idTag));
    query.exec(QStringLiteral("DELETE FROM seria_tag WHERE id_tag=%1").arg(idTag));
    query.exec(QStringLiteral("DELETE FROM author_tag WHERE id_tag=%1").arg(idTag));
    query.exec(QStringLiteral("DELETE FROM tag WHERE id=%1").arg(idTag));
}

QString SLib::nameFromInpx(const QString &sInpx)
{
    QString sName;
    if(!sInpx.isEmpty()){
        QuaZip uz(sInpx);
        if(!uz.open(QuaZip::mdUnzip))
        {
            qDebug()<<"Error open INPX file: " << sInpx;
            //return sName;
        } else
            if(setCurrentZipFileName(&uz, QStringLiteral("COLLECTION.INFO")))
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

