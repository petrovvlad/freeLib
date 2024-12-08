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

#ifdef QUAZIP_STATIC
#include "quazip/quazip/quazipfile.h"
#else
#include <quazip/quazipfile.h>
#endif

#include "utilites.h"
#include "options.h"

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


uint ImportThread::AddSeria(const QString &str, qlonglong libID, const QVariantList *pTags)
{
    if(str.trimmed().isEmpty())
        return 0;
    QString name = str.trimmed();
    query_.exec(u"SELECT id FROM seria WHERE name='"_s % name % u"' and id_lib="_s % QString::number(libID));
    if(query_.next())
    {
        uint id = query_.value(0).toLongLong();
        return id;
    }
    queryInsertSeria_.bindValue(QStringLiteral(":name"), name);
    queryInsertSeria_.bindValue(QStringLiteral(":id_lib"), libID);
    if(!queryInsertSeria_.exec())
        MyDBG << queryInsertSeria_.lastError().text();
    uint id = queryInsertSeria_.lastInsertId().toLongLong();

    QVariantList lTags;
    if(pTags != nullptr )
        lTags = *pTags;
    if(mSequenceTags_.contains(name)){
        const auto &vTags = mSequenceTags_.at(name);
        std::ranges::copy(vTags, std::back_inserter(lTags));
    }

    if(lTags.size()>0){
        QVariantList listIdSequence;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        listIdSequence.fill(id, lTags.size());
#else
        for(int i=0; i<lTags.size(); ++i)
            listIdSequence << id;
#endif
        queryInsertSequennceTag_.bindValue(QStringLiteral(":idSeria"), listIdSequence);
        queryInsertSequennceTag_.bindValue(QStringLiteral(":idTag"), lTags);
        if(!queryInsertSequennceTag_.execBatch())
            MyDBG << queryInsertSequennceTag_.lastError().text();
    }
    return id;
}

uint ImportThread::addAuthor(const SAuthor &author, uint libID, uint idBook, bool bFirstAuthor, const QVariantList *pTags)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    uint idAuthor = hashAuthors_[author];
#else
    uint idAuthor = 0;
    QString sQuery = QStringLiteral("SELECT id FROM author WHERE id_lib=%1 and %2 and %3 and %4").arg(
                QString::number(libID),
                (author.sLastName.isEmpty() ?QStringLiteral("(name1 is null or name1=\"\")") :(QStringLiteral("name1=\"") + author.sLastName + QStringLiteral("\""))),
                (author.sFirstName.isEmpty() ?QStringLiteral("(name2 is null or name2=\"\")") :(QStringLiteral("name2=\"") + author.sFirstName + QStringLiteral("\""))),
                (author.sMiddleName.isEmpty() ?QStringLiteral("(name3 is null or name3=\"\")") :(QStringLiteral("name3=\"") + author.sMiddleName + QStringLiteral("\""))));
    query_.exec(sQuery);
    if(query_.next())
    {
        idAuthor = query_.value(0).toLongLong();
    }
#endif
    QVariantList lTags;
    if(pTags != nullptr )
        lTags = *pTags;

    if(idAuthor == 0)
    {
        queryInsertAuthor_.bindValue(u":name1"_s, author.sLastName);
        queryInsertAuthor_.bindValue(u":name2"_s, author.sFirstName);
        queryInsertAuthor_.bindValue(u":name3"_s, author.sMiddleName);
        queryInsertAuthor_.bindValue(u":id_lib"_s, libID);
        if(!queryInsertAuthor_.exec())
            MyDBG << queryInsertAuthor_.lastError().text();
        idAuthor = queryInsertAuthor_.lastInsertId().toLongLong();
        if(mAuthorsTags_.contains(author.getName()))
        {
            const auto &vTags = mAuthorsTags_.at(author.getName());
            std::ranges::copy(vTags, std::back_inserter(lTags));
        }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        hashAuthors_[author] = idAuthor;       
#endif
    }
    else
    {
    }
    if(bFirstAuthor){
        if(!query_.exec(u"UPDATE book SET first_author_id="_s % QString::number(idAuthor) % u" WHERE id="_s % QString::number(idBook)))
            MyDBG << query_.lastError().text();
    }

    queryInsertBookAuthor_.bindValue(u":idBook"_s, idBook);
    queryInsertBookAuthor_.bindValue(u":idAuthor"_s, idAuthor);
    queryInsertBookAuthor_.bindValue(u":idLib"_s, libID);
    if(!queryInsertBookAuthor_.exec()){
        MyDBG << queryInsertBookAuthor_.lastError().text();
        return 0;
    }

    if(lTags.size()>0){
        QVariantList listAuthors;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        listAuthors.fill(idAuthor, lTags.size());
#else
        for(int i=0; i<lTags.size(); ++i)
            listAuthors << idAuthor;
#endif
        queryInsertAuthorTag_.bindValue(u":idAuthor"_s, listAuthors);
        queryInsertAuthorTag_.bindValue(u":idTag"_s, lTags);
        if(!queryInsertAuthorTag_.execBatch())
            MyDBG << queryInsertAuthorTag_.lastError().text();
    }
    return idAuthor;
}

uint ImportThread::AddBook(qlonglong star, QString &name, qlonglong id_seria, int num_in_seria, const QString &file,
             int size, int IDinLib, bool deleted, const QString &format, QDate date, const QString &language, const QString &keys, qlonglong id_lib, const QString &archive, const QVariantList *pTags)
{
    QString sRepairLanguge = language.toLower();
    if(sRepairLanguge == u"io")
        sRepairLanguge = u"eo"_s;
    else if(sRepairLanguge == u"ua")
        sRepairLanguge = u"uk"_s;
    else if(sRepairLanguge == u"sh")
        sRepairLanguge = u"sr"_s;
    else if(sRepairLanguge == u"in")
        sRepairLanguge = u"id"_s;
    else if(sRepairLanguge == u"кг")
        sRepairLanguge = u"ru"_s;
    name.replace(u"..."_s, u"…"_s);

    queryInsertBook_.bindValue(QStringLiteral(":name"), name);
    queryInsertBook_.bindValue(QStringLiteral(":star"), star);
    queryInsertBook_.bindValue(QStringLiteral(":id_seria"), id_seria!=0 ?id_seria :QVariant());
    queryInsertBook_.bindValue(QStringLiteral(":num_in_seria"), num_in_seria);
    queryInsertBook_.bindValue(QStringLiteral(":language"), sRepairLanguge);
    queryInsertBook_.bindValue(QStringLiteral(":file"), file);
    queryInsertBook_.bindValue(QStringLiteral(":size"), size);
    queryInsertBook_.bindValue(QStringLiteral(":deleted"), deleted);
    queryInsertBook_.bindValue(QStringLiteral(":date"), date);
    queryInsertBook_.bindValue(QStringLiteral(":keys"), keys);
    queryInsertBook_.bindValue(QStringLiteral(":id_inlib"), IDinLib);
    queryInsertBook_.bindValue(QStringLiteral(":id_lib"), id_lib);
    queryInsertBook_.bindValue(QStringLiteral(":format"), format);
    queryInsertBook_.bindValue(QStringLiteral(":archive"), archive);
    if(!queryInsertBook_.exec()){
        MyDBG << queryInsertBook_.lastError().text();
        return 0;
    }

    uint id = queryInsertBook_.lastInsertId().toUInt();

    QVariantList lTags;
    if(pTags != nullptr )
        lTags = *pTags;
    if(mBooksTags_.contains(IDinLib)){
        auto &vTags = mBooksTags_.at(IDinLib);
        std::ranges::copy(vTags, std::back_inserter(lTags));
    }

    if(lTags.size()>0){
        QVariantList listIdBook;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        listIdBook.fill(id, lTags.size());
#else
        for(int i=0; i<lTags.size(); ++i)
            listIdBook << id;
#endif
        queryInsertBookTag_.bindValue(u":idBook"_s, listIdBook);
        queryInsertBookTag_.bindValue(u":idTag"_s, lTags);
        if(!queryInsertBookTag_.execBatch())
            MyDBG << queryInsertBookTag_.lastError().text();
    }
    return id;
}

void ImportThread::AddGenre(uint idBook, const QString &sGenre, uint idLib)
{
    ushort idGenre = 0;
    QString sCorrectGenre = sGenre.toLower();
    sCorrectGenre.replace(' ', '_');
    if(mGenreKeys_.contains(sCorrectGenre)){
        idGenre = mGenreKeys_[sCorrectGenre];
        queryInsertBookGenre_.bindValue(QStringLiteral(":idBook"), idBook);
        queryInsertBookGenre_.bindValue(QStringLiteral(":idGenre"), idGenre);
        queryInsertBookGenre_.bindValue(QStringLiteral(":idLib"), idLib);
        if(!queryInsertBookGenre_.exec())
            MyDBG << queryInsertBookGenre_.lastError().text();
    }else
        qDebug() << "Неизвестный жанр: " + sGenre;
}

void ImportThread::init(uint id, const SLib &lib, uchar nUpdateType)
{
    sInpxFile_ = RelativeToAbsolutePath(lib.sInpx);
    if(!QFileInfo::exists(sInpxFile_) || !QFileInfo(sInpxFile_).isFile())
    {
        sInpxFile_ = lib.sInpx;
    }
    sPath_ = RelativeToAbsolutePath(lib.path);
    sName_ = lib.name;
    nUpdateType_ = nUpdateType;
    idLib_ = id;
    stopped_.store(false, std::memory_order_relaxed);
    bFirstAuthorOnly_ = lib.bFirstAuthor;
    bWoDeleted_ = lib.bWoDeleted;
    if(nUpdateType_ == UT_NEW){
        if(lib.books.empty()){
            QSqlQuery query(QSqlDatabase::database(QStringLiteral("libdb")));
            query.setForwardOnly(true);
            query.prepare(u"SELECT id FROM book WHERE id_lib=:id_lib;"_s);
            query.bindValue(u":id_lib"_s,idLib_);
            if(!query.exec())
                MyDBG << query.lastError().text();
            else{
                while (query.next()){
                    stIdBookInLib_.insert(query.value(0).toUInt());
                }
            }
        }else{
            stIdBookInLib_.reserve(lib.books.size());
            for(const auto &book :lib.books)
                stIdBookInLib_.insert(book.second.idInLib);
        }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        if(lib.authors.empty()){
            QSqlQuery query(QSqlDatabase::database(QStringLiteral("libdb")));
            query.setForwardOnly(true);
            query.prepare(QStringLiteral("SELECT id, name1, name2, name3 FROM author WHERE id_lib=:idLib;"));
            //                                     0          1      2      3
            query.bindValue(QStringLiteral(":idLib"), idLib_);
            query.exec();
            while (query.next()) {
                uint idAuthor = query.value(0).toUInt();
                SAuthor author;
                author.sFirstName = query.value(2).toString().trimmed();;
                author.sLastName = query.value(1).toString().trimmed();;
                author.sMiddleName = query.value(3).toString().trimmed();;
                hashAuthors_[author] = idAuthor;
            }
        }else{
            for(const auto &iAuthor :lib.authors){
                hashAuthors_[iAuthor.second] = iAuthor.first;
            }
        }
#endif
    }
    if(nUpdateType_ == UT_FULL){
        for(const auto &[idAuthor, author] :lib.authors){
            if(!author.vIdTags.empty())
                mAuthorsTags_[author.getName()] = std::move(author.vIdTags);
        }

        for(const auto &[idBook, book] :lib.books){
            if(!book.vIdTags.empty() && book.idInLib!=0)
                mBooksTags_[book.idInLib] = std::move(book.vIdTags);
        }

        for(const auto &[idSequence, sequence] :lib.serials){
            if(!sequence.vIdTags.empty())
                mSequenceTags_[sequence.sName] = std::move(sequence.vIdTags);
        }
    }
}

void ImportThread::init(uint id, const SLib &lib, const QStringList &files)
{
    idLib_ = id;
    listFiles_ = files;
    sPath_ = RelativeToAbsolutePath(lib.path);
    bFirstAuthorOnly_ = lib.bFirstAuthor;
    stopped_.store(false, std::memory_order_relaxed);
}

void ImportThread::readFB2(const QByteArray& ba, QString file_name, QString arh_name, qint32 file_size)
{
    if(arh_name.isEmpty())
    {
        file_name = file_name.right(file_name.length() - sPath_.length());
        if(!file_name.isEmpty() && (file_name.at(0) == '/' || file_name.at(0) == '\\'))
            file_name = file_name.right(file_name.length()-1);
    }
    else
    {
        QString sTempZipDir = QDir::tempPath() + QStringLiteral("/freeLib/zip/") ;
        if(arh_name.startsWith(sTempZipDir)){
            arh_name = arh_name.right(arh_name.length() - sTempZipDir.length());
            arh_name.replace(QStringLiteral(".zip.dir/"), QStringLiteral(".zip/"));
        }else
            arh_name = arh_name.right(arh_name.length() - sPath_.length());
        if(!arh_name.isEmpty() && (arh_name.at(0) == '/' || arh_name.at(0) == '\\'))
            arh_name = arh_name.right(arh_name.length()-1);
    }

    QFileInfo fi(file_name);
    file_name = file_name.left(file_name.length()-fi.suffix().length()-1);
    query_.exec(QStringLiteral("SELECT id FROM book where id_lib=%1 and file='%2' and archive='%3'").arg(QString::number(idLib_), file_name, arh_name));
    if(query_.next()) //если книга найдена, то просто снимаем пометку удаления
    {
        query_.exec(QStringLiteral("update book set deleted=0 where id=") + query_.value(0).toString());
        return;
    }

    QDomDocument doc;
    doc.setContent(ba);
    QDomElement title_info = doc.elementsByTagName(QStringLiteral("title-info")).at(0).toElement();
    QString sTitle = title_info.elementsByTagName(QStringLiteral("book-title")).at(0).toElement().text();
    QString sLanguage = title_info.elementsByTagName(QStringLiteral("lang")).at(0).toElement().text().left(2);
    QString sSeria = title_info.elementsByTagName(QStringLiteral("sequence")).at(0).attributes().namedItem(QStringLiteral("name")).toAttr().value().trimmed();
    uint numInSerial = title_info.elementsByTagName(QStringLiteral("sequence")).at(0).attributes().namedItem(QStringLiteral("number")).toAttr().value().trimmed().toUInt();

    std::vector<SAuthor> vAuthors;
    QDomNodeList authors = title_info.elementsByTagName(QStringLiteral("author"));
    for(int i=0; i<authors.count(); i++)
    {
        SAuthor author;
        auto element = authors.at(i).toElement();
        author.sFirstName = element.elementsByTagName(QStringLiteral("first-name")).at(0).toElement().text().trimmed();
        author.sLastName = element.elementsByTagName(QStringLiteral("last-name")).at(0).toElement().text().trimmed();
        author.sMiddleName = element.elementsByTagName(QStringLiteral("middle-name")).at(0).toElement().text().trimmed();
        vAuthors.push_back(author);
    }
    QStringList listGenres;
    QDomNodeList genre = title_info.elementsByTagName(QStringLiteral("genre"));
    for(int i=0; i<genre.count(); i++)
    {
        listGenres << genre.at(i).toElement().text().trimmed();
    }
    QDomElement publish_info = doc.elementsByTagName(QStringLiteral("publish-info")).at(0).toElement();
    QString sIsbn = publish_info.elementsByTagName(QStringLiteral("isbn")).at(0).toElement().text();

    qlonglong id_seria = AddSeria(sSeria, idLib_);
    qlonglong id_book = AddBook(0, sTitle, id_seria, numInSerial, file_name, (file_size == 0 ?ba.size() :file_size), 0, false, fi.suffix(), QDate::currentDate(),
                                sLanguage, u""_s, idLib_, arh_name);

    bool first_author = true;
    for(const SAuthor &author: vAuthors)
    {
        addAuthor(author, idLib_, id_book, first_author);
        first_author = false;
        if(bFirstAuthorOnly_)
            break;
    }
    for(const auto &sGenre: std::as_const(listGenres))
        AddGenre(id_book, sGenre, idLib_);
}

void ImportThread::readEPUB(const QByteArray &ba, QString file_name, QString arh_name, qint32 file_size)
{
    if(arh_name.isEmpty())
    {
        file_name = file_name.right(file_name.length() - sPath_.length());
        if(file_name.at(0) == '/' || file_name.at(0) == '\\')
            file_name=file_name.right(file_name.length()-1);
    }
    else
    {
        QString sTempZipDir = QDir::tempPath() + QStringLiteral("/freeLib/zip/") ;
        if(arh_name.startsWith(sTempZipDir)){
            arh_name = arh_name.right(arh_name.length() - sTempZipDir.length());
            arh_name.replace(QStringLiteral(".zip.dir/"), QStringLiteral(".zip/"));
        }else
            arh_name = arh_name.right(arh_name.length() - sPath_.length());
        if(arh_name.at(0) == '/' || arh_name.at(0) == '\\')
            arh_name = arh_name.right(arh_name.length()-1);
    }

    file_name = file_name.left(file_name.length()-5);
    query_.exec(QStringLiteral("SELECT id FROM book where id_lib=%1 and file='%2' and archive='%3'").arg(QString::number(idLib_), file_name, arh_name));
    if(query_.next()) //если книга найдена, то просто снимаем пометку удаления
    {
        query_.exec(QStringLiteral("update book set deleted=0 where id=") + query_.value(0).toString());
        return;
    }

    QBuffer buf;
    buf.setData(ba);
    QuaZip zip(&buf);
    zip.open(QuaZip::mdUnzip);
    QBuffer info;
    setCurrentZipFileName(&zip, QStringLiteral("META-INF/container.xml"));
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
    std::vector<SAuthor> vAuthors;
    QStringList listGenres;

    for(int i=0; i<root.childNodes().count() && need_loop; i++)
    {
        if(root.childNodes().at(i).nodeName().toLower() == u"rootfiles")
        {
            QDomNode roots = root.childNodes().at(i);
            for(int j=0; j<roots.childNodes().count() && need_loop; j++)
            {
                if(roots.childNodes().at(j).nodeName().toLower() == u"rootfile")
                {
                    QString path = roots.childNodes().at(j).attributes().namedItem(QStringLiteral("full-path")).toAttr().value();
                    QBuffer opf_buf;
                    QFileInfo fi(path);
                    rel_path = fi.path();
                    setCurrentZipFileName(&zip,path);
                    zip_file.open(QIODevice::ReadOnly);
                    opf_buf.setData(zip_file.readAll());
                    zip_file.close();

                    QDomDocument opf;
                    opf.setContent(opf_buf.data());
                    QDomNode meta = opf.documentElement().namedItem(QStringLiteral("metadata"));
                    for(int m=0; m<meta.childNodes().count(); m++)
                    {
                        if(meta.childNodes().at(m).nodeName().right(5) == u"title")
                        {
                            sTitle = meta.childNodes().at(m).toElement().text().trimmed();
                        }
                        else if(meta.childNodes().at(m).nodeName().right(8) == u"language")
                        {
                            sLanguage = meta.childNodes().at(m).toElement().text().trimmed().left(2);
                        }
                        else if(meta.childNodes().at(m).nodeName().right(7) == u"creator")
                        {
                            SAuthor author;
                            QStringList names = meta.childNodes().at(m).toElement().text().trimmed().split(QStringLiteral(" "));
                            if(names.count() > 0)
                                author.sFirstName = names.at(0);
                            if(names.count() > 1)
                                author.sMiddleName = names.at(1);
                            if(names.count() > 2)
                                author.sLastName = names.at(2);
                            vAuthors.push_back(author);
                        }
                        else if(meta.childNodes().at(m).nodeName().right(7) == u"subject")
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

    if(sTitle.isEmpty() || vAuthors.empty())
    {
        return;
    }
    uint idBook = AddBook(0, sTitle, 0, 0, file_name, (file_size == 0 ?ba.size() :file_size), 0, false, u"epub"_s, QDate::currentDate(),
                                sLanguage, u""_s, idLib_, arh_name);

    bool first_author = true;
    for(const SAuthor &author: vAuthors)
    {
        addAuthor(author, idLib_, idBook, first_author);
        if(bFirstAuthorOnly_)
            break;
        first_author = false;
    }
    for(const auto &sGenre: std::as_const(listGenres))
        AddGenre(idBook, sGenre, idLib_);
}


QFileInfoList getFileList(const QString &sPath)
{
    QStringList filter = {u"*.fb2"_s, u"*.epub"_s, u"*.zip"_s};
    QDir dir(sPath);
    const auto &listEntry = dir.entryInfoList(filter, QDir::NoSymLinks|QDir::NoDotAndDotDot|QDir::Readable|QDir::Files|QDir::AllDirs);
    QFileInfoList listFiles;
    for(const auto &entry :listEntry){
        if(entry.isFile())
            listFiles << entry;
        else
            listFiles << getFileList(entry.absoluteFilePath());
    }
    return listFiles;
}

void ImportThread::importBooksFromPath(const QString &sPath)
{
    auto listFiles = getFileList(sPath);
    if(listFiles.count() > 0)
        importBooksFromList(listFiles);
    QDir(QDir::tempPath() + u"/freeLib/zip/"_s).removeRecursively();
}


void ImportThread::importBooksFromList(const QFileInfoList &listFiles)
{
    uint nBooksCount = 0;
    query_.exec(u"BEGIN;"_s);

    for(size_t i=0; i<listFiles.count() && !stopped_.load(std::memory_order_relaxed); i++)
    {
        auto &fileInfo = listFiles[i];
        QString sFileName = fileInfo.absoluteFilePath();
        QString sSuffix = fileInfo.suffix().toLower();
        if(sSuffix != u"fbd" &&
         !(sSuffix == u"zip" || sSuffix == u"fb2" || sSuffix == u"epub"))
        {
            QString fbd = fileInfo.absolutePath() + u"/"_s + fileInfo.completeBaseName() + u".fbd"_s;
            QFile file(fbd);
            if(file.exists())
            {
                file.open(QFile::ReadOnly);
                readFB2(file.readAll(), sFileName, u""_s, fileInfo.size());
                nBooksCount++;
            }
        }
        else if(sSuffix == u"fb2" || sSuffix == u"epub")
        {
            QFile file(sFileName);
            file.open(QFile::ReadOnly);
            QByteArray ba = file.readAll();
            if(sSuffix == u"fb2")
                readFB2(ba, sFileName, u""_s);
            else
                readEPUB(ba, sFileName, u""_s);
            nBooksCount++;
        }
        else if(sSuffix == u"zip")
        {
            QString sArchName;
            if(nUpdateType_ == UT_NEW)
            {
                sArchName = sFileName.right(sFileName.length() - sPath_.length());
                if(sArchName.at(0) == '/' || sArchName.at(0) == '\\')
                    sArchName = sArchName.right(sArchName.length()-1);
                query_.exec(u"SELECT * FROM book where archive='%1' LIMIT 1"_s.arg(sArchName));
                if(query_.next())
                    continue;
            }
            QDir dir(sPath_);

            sArchName = dir.relativeFilePath(sFileName) ;
            importBooksFromZip(sPath_ + QStringLiteral("/"), sArchName, nBooksCount);
        }
        emit progress(nBooksCount, (float)(i + 1)/(float)(listFiles.count()));
    }
    query_.exec(u"COMMIT;"_s);

}

void ImportThread::importBooksFromZip(const QString &sPath, const QString &sArchName, uint &nBooksCount)
{
    QString sZipFileName = sPath + sArchName;
    QuaZip uz(sZipFileName);
    uz.setFileNameCodec(QTextCodec::codecForName("IBM 866"));
    if (!uz.open(QuaZip::mdUnzip))
    {
        qDebug() << "Error open archive! " << sPath + sArchName;
        return;
    }

    QuaZipFile fileInZip(&uz);
    const auto list = uz.getFileInfoList();
    for(const QuaZipFileInfo &fiZip: list  )
    {
        if(stopped_.load(std::memory_order_relaxed))
            break;

        if(fiZip.uncompressedSize == 0)
            continue;
        QBuffer buffer;
        if(fiZip.name.right(3).toLower() == u"fb2")
        {
            setCurrentZipFileName(&uz, fiZip.name);
            fileInZip.open(QIODevice::ReadOnly);
            buffer.setData(fileInZip.read(16*1024));
            fileInZip.close();
            readFB2(buffer.data(), fiZip.name, sZipFileName, fiZip.uncompressedSize);
            nBooksCount++;
        }
        else if(fiZip.name.right(4).toLower() == u"epub")
        {
            setCurrentZipFileName(&uz, fiZip.name);
            fileInZip.open(QIODevice::ReadOnly);
            buffer.setData(fileInZip.readAll());
            fileInZip.close();
            readEPUB(buffer.data(), fiZip.name, sZipFileName, fiZip.uncompressedSize);
            nBooksCount++;
        }
        else if(fiZip.name.right(4).toLower() == u".zip")
        {
            setCurrentZipFileName(&uz, fiZip.name);
            fileInZip.open(QIODevice::ReadOnly);
            buffer.setData(fileInZip.readAll());
            fileInZip.close();
            QString sZipDir;
            if(sPath.startsWith(QDir::tempPath() + u"/freeLib/zip/"_s))
                sZipDir = sPath + sArchName + u".dir/"_s;
            else
                sZipDir = QDir::tempPath() + u"/freeLib/zip/"_s + sArchName + u".dir/"_s;
            QDir dirZip(sZipDir);
            dirZip.mkpath(sZipDir);
            QFile fileZiped(sZipDir + fiZip.name);
            fileZiped.open(QIODevice::WriteOnly);
            fileZiped.write(buffer.data());
            fileZiped.close();
            importBooksFromZip(sZipDir , fiZip.name, nBooksCount);

            fileZiped.remove();
        }
        else if(fiZip.name.right(3).toLower() != u"fbd")
        {
            QFileInfo fi(fiZip.name);
            if(!fi.completeBaseName().isEmpty() && fi.completeBaseName().at(0) != '.')
            {
                QString fbd = fi.path() + u"/"_s + fi.completeBaseName() + u".fbd"_s;
                if(setCurrentZipFileName(&uz, fbd))
                {
                    setCurrentZipFileName(&uz, fiZip.name);
                    fileInZip.open(QIODevice::ReadOnly);
                    buffer.setData(fileInZip.readAll());
                    readFB2(buffer.data(), fiZip.name, sZipFileName, fiZip.uncompressedSize);
                    nBooksCount++;
                }
            }
        }

    }
}

void ImportThread::process()
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
    QSqlDatabase dbase = QSqlDatabase::cloneDatabase(QStringLiteral("libdb"), QStringLiteral("importdb"));
#else
    QFileInfo fi(RelativeToAbsolutePath(g::options.sDatabasePath));
    QString sDbFile = fi.canonicalFilePath();
    QSqlDatabase dbase = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("importdb"));
    dbase.setDatabaseName(sDbFile);
#endif
    if (!dbase.open())
    {
        MyDBG << dbase.lastError().text();
        return;
    }

    query_ = QSqlQuery(dbase);
    query_.setForwardOnly(true);
    std::unordered_map<QString, uint> mTags;
    mTags.clear();
    query_.exec(u"PRAGMA cache_size = 2000"_s);
    query_.exec(QStringLiteral("SELECT id,name FROM tag"));
    while(query_.next())
    {
        uint idTag = query_.value(0).toUInt();
        QString sTagName = query_.value(1).toString().trimmed();
        mTags[sTagName] = idTag;
    }

    query_.exec(QStringLiteral("SELECT id, keys FROM genre where NOT keys='';"));
    while(query_.next())
    {
        ushort idGenre = query_.value(0).toUInt();
        QString sKeys = query_.value(1).toString();
        const QStringList listKeys = sKeys.split(QStringLiteral(";"));
        for(const auto &sKey :listKeys){
            if(!sKey.isEmpty()){
                mGenreKeys_[sKey] = idGenre;
            }
        }
    }

    queryInsertBook_ = QSqlQuery(dbase);
    queryInsertBook_.prepare(QStringLiteral("INSERT INTO book(name,star,id_seria,num_in_seria,language,file,size,'deleted',date,keys,id_inlib,id_lib,format,archive) "
                                  "values(:name,:star,:id_seria,:num_in_seria,:language,:file,:size,:deleted,:date,:keys,:id_inlib,:id_lib,:format,:archive);"));

    queryInsertAuthor_ = QSqlQuery(dbase);
    queryInsertAuthor_.prepare(QStringLiteral("INSERT INTO author(name1,name2,name3,id_lib) values(:name1,:name2,:name3,:id_lib);"));

    queryInsertSeria_ = QSqlQuery(dbase);
    queryInsertSeria_.prepare(QStringLiteral("INSERT INTO seria(name,id_lib) values(:name,:id_lib)"));

    queryInsertBookAuthor_ = QSqlQuery(dbase);
    queryInsertBookAuthor_.prepare(QStringLiteral("INSERT OR IGNORE INTO book_author(id_book,id_author,id_lib) values(:idBook,:idAuthor,:idLib);"));

    queryInsertBookGenre_ = QSqlQuery(dbase);
    queryInsertBookGenre_.prepare(QStringLiteral("INSERT OR IGNORE INTO book_genre(id_book,id_genre,id_lib) values(:idBook,:idGenre,:idLib);"));

    queryInsertAuthorTag_ = QSqlQuery(dbase);
    queryInsertAuthorTag_.prepare(u"INSERT INTO author_tag (id_author,id_tag) VALUES(:idAuthor, :idTag)"_s);

    queryInsertBookTag_ = QSqlQuery(dbase);
    queryInsertBookTag_.prepare(u"INSERT INTO book_tag(id_book, id_tag) values(:idBook, :idTag)"_s);

    queryInsertSequennceTag_ = QSqlQuery(dbase);
    queryInsertSequennceTag_.prepare(u"INSERT INTO seria_tag(id_seria, id_tag) values(:idSeria, :idTag)"_s);


    if(!listFiles_.isEmpty()){
        QFileInfoList listFilInfo;
        listFilInfo.reserve(listFiles_.count());
        for(const auto &sFile :std::as_const(listFiles_))
            listFilInfo << QFileInfo(sFile);
        if(listFiles_.count() > 0)
            importBooksFromList(listFilInfo);
        emit End();
        return;
    }

    switch(nUpdateType_)
    {
    case UT_FULL:
        ClearLib(dbase, idLib_, false);
        break;
    case UT_DEL_AND_NEW:
        ClearLib(dbase, idLib_, true);
        break;
    }

    if(sInpxFile_.isEmpty())
    {
        importBooksFromPath(sPath_);
        // query_.exec(QStringLiteral("DELETE FROM book WHERE id_lib=%1 AND deleted=TRUE;").arg(QString::number(idLib_)));
        // query_.exec(QStringLiteral("VACUUM"));
        emit End();
        return;
    }
    QuaZip archiveFile;
    QuaZip uz(sInpxFile_);
    if (!uz.open(QuaZip::mdUnzip))
    {
        MyDBG << ("Error open inpx!");
        emit End();
        return;
    }

    if(setCurrentZipFileName(&uz, u"version.info"_s))
    {
        QBuffer outbuff;
        QuaZipFile zip_file(&uz);
        zip_file.open(QIODevice::ReadOnly);
        outbuff.setData(zip_file.readAll());
        zip_file.close();
        QString sVersion = QString::fromUtf8(outbuff.data()).simplified();
        if(!query_.exec(u"UPDATE lib SET version='"_s + sVersion + u"' WHERE id="_s + QString::number(idLib_)))
            MyDBG << query_.lastError().text();
    }

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
    if(setCurrentZipFileName(&uz, u"structure.info"_s))
    {
        for(unsigned int i=0; i<sizeof(field_index)/sizeof(int); i++)
            field_index[i] = -1;
        QBuffer outbuff;
        zip_file.open(QIODevice::ReadOnly);
        outbuff.setData(zip_file.readAll());
        zip_file.close();
        const QStringList lines = (QString::fromUtf8(outbuff.data())).split(QStringLiteral("\n"));
        for(const QString &line: lines)
        {
            const QStringList substrings = line.toUpper().split(QStringLiteral(";"));
            int i = 0;
            for(const QString &substring: substrings)
            {
                if(substring == u"TITLE")
                    field_index[_NAME] = i;
                else if(substring == u"SERIES")
                    field_index[_SERIA] = i;
                else if(substring == u"SERNO")
                    field_index[_NUM_IN_SERIA] = i;
                else if(substring == u"FILE")
                    field_index[_FILE] = i;
                else if(substring == u"SIZE")
                    field_index[_SIZE] = i;
                else if(substring == u"LIBID")
                    field_index[_ID_IN_LIB] = i;
                else if(substring == u"DEL")
                    field_index[_DELETED] = i;
                else if(substring == u"EXT")
                    field_index[_FORMAT] = i;
                else if(substring == u"DATE")
                    field_index[_DATE] = i;
                else if(substring == u"LANG")
                    field_index[_LANGUAGE] = i;
                else if(substring == u"STARS")
                    field_index[_STAR] = i;
                else if(substring == u"KEYWORDS")
                    field_index[_KEYS] = i;
                else if(substring == u"AUTHOR")
                    field_index[_AUTHORS] = i;
                else if(substring == u"GENRE")
                    field_index[_JANRES] = i;
                else if(substring == u"FOLDER")
                    field_index[_FOLDER] = i;
                else if(substring == u"TAG")
                    field_index[_TAG] = i;
                else if(substring == u"TAGSERIES")
                    field_index[_TAG_SERIA] = i;
                else if(substring == u"TAGAUTHOR")
                    field_index[_TAG_AUTHOR] = i;
                i++;
            }

        }
    }

    uint nBooksCount = 0;
    const QStringList listFiles = uz.getFileNameList();
    QStringList listInpFiles;
    for(const auto &sFileName :listFiles)
        if(sFileName.right(4).toUpper() == u".INP")
            listInpFiles << sFileName;

    dbase.transaction();

    uint iStart = 0;
    for(uint i=0; i<listInpFiles.count(); i++)
    {
        QCoreApplication::processEvents();
        if(stopped_.load(std::memory_order_relaxed))
        {
            break;
        }
        const QString str = listInpFiles[i];
        if(nUpdateType_ == UT_NEW)
        {
            query_.exec(u"SELECT * FROM book WHERE archive='%1' AND id_lib=%2 LIMIT 1"_s.arg(str, QString::number(idLib_)));
            if(query_.next())
                continue;
        }
        QBuffer outbuff;
        setCurrentZipFileName(&uz, str);
        zip_file.open(QIODevice::ReadOnly);
        outbuff.setData(zip_file.readAll());
        zip_file.close();
        const QStringList lines = (QString::fromUtf8(outbuff.data())).split('\n');
        uint iLine = 0;
        uint count = 0;
        for(const auto &line: lines)
        {
            iLine++;
            if(line.isEmpty())
                continue;

            QCoreApplication::processEvents();
            if(stopped_.load(std::memory_order_relaxed))
                break;

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
            uint idInLib = 0;
            if(substrings.count() > field_index[_ID_IN_LIB])
            {
                idInLib = substrings[field_index[_ID_IN_LIB]].trimmed().toUInt();
                if(nUpdateType_ == UT_NEW && stIdBookInLib_.contains(idInLib))
                    continue;
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
                language = substrings[field_index[_LANGUAGE]].trimmed().left(2);
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
            QVariantList listBookTags;
            if(substrings.count() > field_index[_TAG] && field_index[_TAG] >= 0)
            {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
                const QStringList listStrBookTags = substrings[field_index[_TAG]].trimmed().split(':', Qt::SkipEmptyParts);
#else
                QStringList listStrBookTags = substrings[field_index[_TAG]].trimmed().split(':');
#endif
                for(const auto &sTag :listStrBookTags){
                    if(mTags.contains(sTag))
                        listBookTags << mTags[sTag];
                }
            }
            QVariantList listSeriaTags;
            if(substrings.count() > field_index[_TAG_SERIA] && field_index[_TAG_SERIA] >= 0)
            {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
                const QStringList listStrSeriaTags = substrings[field_index[_TAG_SERIA]].trimmed().split(':', Qt::SkipEmptyParts);
#else
                QStringList listStrSeriaTags = substrings[field_index[_TAG_SERIA]].trimmed().split(':');
#endif
                for(const auto &sTag :listStrSeriaTags){
                    if(mTags.contains(sTag))
                        listSeriaTags << mTags[sTag];
                }
            }
            QVariantList listAuthorTags;
            if(substrings.count() > field_index[_TAG_AUTHOR] && field_index[_TAG_AUTHOR] >= 0)
            {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
                const QStringList listStrAuthorTags = substrings[field_index[_TAG_AUTHOR]].trimmed().split(':', Qt::SkipEmptyParts);
#else
                QStringList listStrAuthorTags = substrings[field_index[_TAG_AUTHOR]].trimmed().split(':');
#endif
                for(const auto &sTag :listStrAuthorTags){
                    if(mTags.contains(sTag))
                        listAuthorTags << mTags[sTag];
                }
            }
            uint idSeria = 0;
            if(!Seria.isEmpty())
                idSeria = AddSeria(Seria, idLib_, &listSeriaTags);

            qlonglong id_book;
            if(!bWoDeleted_ || !deleted){
                id_book = AddBook(star, name, idSeria, num_in_seria, file, size, idInLib, deleted, format, date, language, keys, idLib_, folder, &listBookTags);

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
                const QStringList Authors = substrings[field_index[_AUTHORS]].split(':', Qt::SkipEmptyParts);
#else
                QStringList Authors = substrings[field_index[_AUTHORS]].split(':');
#endif
                int author_count = 0;
                for(const QString &sAuthor: Authors)
                {
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
                    if(sAuthor.isEmpty())
                        continue;
#endif
                    QString sAuthorLow = sAuthor.toLower();
                    bool bUnkownAuthor = (sAuthorLow.contains(QStringLiteral("автор")) && (sAuthorLow.contains(QStringLiteral("неизвестен")) || sAuthorLow.contains(QStringLiteral("неизвестный"))))
                                      || sAuthorLow == QStringLiteral("неизвестно");
                    if(format == u"fb2" && Authors.size()==1 && bUnkownAuthor)
                    {
                        QString sFile, sArchive;
                        QBuffer buffer;
                        sFile = QStringLiteral("%1.%2").arg(file, format);
                        sArchive = QStringLiteral("%1/%2").arg(sPath_, folder.replace(QStringLiteral(".inp"), QStringLiteral(".zip")));
                        if(archiveFile.getZipName() != sArchive){
                            archiveFile.close();
                            archiveFile.setZipName(sArchive);
                            if (!archiveFile.open(QuaZip::mdUnzip))
                                MyDBG << "Error open archive! " << sArchive;
                        }
                        QuaZipFile zip_file(&archiveFile);
                        setCurrentZipFileName(&archiveFile, sFile);
                        if(!zip_file.open(QIODevice::ReadOnly))
                            MyDBG << "Error open file: " << sFile;
                        buffer.setData(zip_file.read(16*1024));
                        zip_file.close();

                        QDomDocument doc;
                        doc.setContent(buffer.data());
                        QDomElement title_info = doc.elementsByTagName(QStringLiteral("title-info")).at(0).toElement();
                        QDomNodeList listAuthor = title_info.elementsByTagName(QStringLiteral("author"));

                        for(int i=0; i<listAuthor.count(); i++)
                        {
                            SAuthor author;
                            auto element = listAuthor.at(i).toElement();
                            author.sFirstName = element.elementsByTagName(u"first-name"_s).at(0).toElement().text();
                            author.sLastName = element.elementsByTagName(u"last-name"_s).at(0).toElement().text();
                            author.sMiddleName = element.elementsByTagName(u"middle-name"_s).at(0).toElement().text();
                            sAuthorLow = author.getName().toLower();
                            if(!sAuthorLow.contains(u"неизвест"_s) && !sAuthorLow.isEmpty()){
                                bool bFirstAuthor = author_count == 0;
                                addAuthor(author, idLib_, id_book, bFirstAuthor, bFirstAuthor ?&listAuthorTags :nullptr);
                                author_count++;
                            }
                        }
                    }else{
                        if(!bUnkownAuthor){
                            SAuthor author(sAuthor);
                            bool bFirstAuthor = author_count == 0;
                            addAuthor(author, idLib_, id_book, bFirstAuthor, bFirstAuthor ?&listAuthorTags :nullptr);
                            author_count++;
                        }
                    }
                }

                if(substrings.count() >= field_index[_JANRES] + 1)
                {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
                    const QStringList listGenres = substrings[field_index[_JANRES]].split(':', Qt::SkipEmptyParts);
#else
                    QStringList listGenres;
                    QStringList tmpList = substrings[field_index[_JANRES]].split(QStringLiteral(":"));
                    for(int i=0; i<tmpList.size(); i++){
                        QString str = tmpList.at(i);
                        if(!str.isEmpty())
                            listGenres << str;
                    }
#endif

                    bool first = true;
                    for(const QString &genre: listGenres)
                    {
                        if(!first && genre.trimmed().isEmpty())
                            continue;
                        AddGenre(id_book, genre.trimmed(), idLib_);
                        first = false;
                    }
                }
                count++;
                if(nBooksCount == 0)
                    iStart = i;
                nBooksCount++;
                if(count == 500)
                {
                    float fProgress;
                    if(listInpFiles.count()>1)
                        fProgress = (float)(i - iStart)/(float)(listInpFiles.count() - iStart);
                    else
                        fProgress = (float)(iLine - iStart)/(float)(lines.count() - iStart);

                    emit progress(nBooksCount, fProgress);
                    count = 0;
                }
            }
        }
        if(count > 0)
            emit progress(nBooksCount, (float)(i - iStart + 1)/(float)(listInpFiles.count() - iStart));
    }
    dbase.commit();
    if(!stopped_)
        emit progress(nBooksCount, 1.0f);

    emit End();
    dbase.close();
}

void ImportThread::break_import()
{
    stopped_.store(true, std::memory_order_relaxed);
}

