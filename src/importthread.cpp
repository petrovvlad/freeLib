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
#include "epubreader.h"

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


void ImportThread::addSequence(const QString &str, uint idLib, uint idBook, uint numInSequence, const QVariantList *pTags)
{
    if(str.trimmed().isEmpty())
        return;
    QString name = str.trimmed();
    uint id;
    if(mSequences_.contains(name)){
        id = mSequences_.at(name);
    }else{
        queryInsertSeria_.bindValue(u":name"_s, name);
        queryInsertSeria_.bindValue(u":id_lib"_s, idLib);
        if(!queryInsertSeria_.exec()) [[unlikely]]
            LogWarning << queryInsertSeria_.lastError().text();
        id = queryInsertSeria_.lastInsertId().toUInt();
        mSequences_[name] = id;
    }
    auto it = mBookSequences_.find(idBook);
    bool bFoundBookSequence = false;
    if(it != mBookSequences_.end()){
        auto  &v = it->second;
        if(contains(v, id))
            bFoundBookSequence = true;
    }
    if(!bFoundBookSequence){
        queryInsertBookSequence_.bindValue(u":idBook"_s, idBook);
        queryInsertBookSequence_.bindValue(u":idSequence"_s, id);
        queryInsertBookSequence_.bindValue(u":numInSequence"_s, numInSequence);
        if(!queryInsertBookSequence_.exec()) [[unlikely]]
            LogWarning << queryInsertBookSequence_.lastError().text();
        mBookSequences_[idBook].push_back(id);
    }

    QVariantList lTags;
    if(pTags != nullptr )
        lTags = *pTags;
    if(auto it = mSequenceTags_.find(name); it != mSequenceTags_.end()){
        const auto& tags = it->second;
        lTags.reserve(tags.size() + lTags.size());
        std::ranges::copy(tags, std::back_inserter(lTags));
    }

    if(lTags.size()>0){
        QVariantList listIdSequence;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        listIdSequence.fill(id, lTags.size());
#else
        for(int i=0; i<lTags.size(); ++i)
            listIdSequence << id;
#endif
        queryInsertSequennceTag_.bindValue(u":idSeria"_s, listIdSequence);
        queryInsertSequennceTag_.bindValue(u":idTag"_s, lTags);
        if(!queryInsertSequennceTag_.execBatch()) [[unlikely]]
            LogWarning << queryInsertSequennceTag_.lastError().text();
    }
    return;
}

uint ImportThread::addAuthor(const SAuthor &author, uint libID, uint idBook, bool bFirstAuthor, const QVariantList *pTags)
{
    uint idAuthor = hashAuthors_[author];
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
            LogWarning << queryInsertAuthor_.lastError().text();
        idAuthor = queryInsertAuthor_.lastInsertId().toUInt();
        if(auto it = stTagetAuthors_.find(author); it != stTagetAuthors_.end())
        {
            const auto& tags = it->idTags;
            lTags.reserve(tags.size() + lTags.size());
            std::ranges::copy(it->idTags, std::back_inserter(lTags));
        }
        hashAuthors_[author] = idAuthor;       
    }
    else
    {
    }
    if(bFirstAuthor){
        if(!query_.exec(u"UPDATE book SET first_author_id="_s % QString::number(idAuthor) % u" WHERE id="_s % QString::number(idBook))) [[unlikely]]
            LogWarning << query_.lastError().text();
    }

    queryInsertBookAuthor_.bindValue(u":idBook"_s, idBook);
    queryInsertBookAuthor_.bindValue(u":idAuthor"_s, idAuthor);
    queryInsertBookAuthor_.bindValue(u":idLib"_s, libID);
    if(!queryInsertBookAuthor_.exec())[[unlikely]] {
        LogWarning << queryInsertBookAuthor_.lastError().text();
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
        if(!queryInsertAuthorTag_.execBatch()) [[unlikely]]
            LogWarning << queryInsertAuthorTag_.lastError().text();
    }
    return idAuthor;
}

uint ImportThread::addBook(uchar star, QString &name, int num_in_seria, const QString &file,
             int size, uint idInLib, bool deleted, const QString &format, QDate date, const QString &language, const QString &keys, qlonglong id_lib, const QString &archive, const QVariantList *pTags)
{
    if(idInLib!=0 && mIdBookInLib_.contains(idInLib)){
        uint idBook = mIdBookInLib_[idInLib];
        return idBook;
    }
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
    else if(sRepairLanguge == u"ge")
        sRepairLanguge = u"de"_s;
    name.replace(u"..."_s, u"…"_s);

    queryInsertBook_.bindValue(u":name"_s, name);
    queryInsertBook_.bindValue(u":star"_s, star);
    queryInsertBook_.bindValue(u":language"_s, sRepairLanguge);
    queryInsertBook_.bindValue(u":file"_s, file);
    queryInsertBook_.bindValue(u":size"_s, size);
    queryInsertBook_.bindValue(u":deleted"_s, deleted);
    queryInsertBook_.bindValue(u":date"_s, date);
    queryInsertBook_.bindValue(u":keys"_s, keys);
    queryInsertBook_.bindValue(u":id_inlib"_s, idInLib);
    queryInsertBook_.bindValue(u":id_lib"_s, id_lib);
    queryInsertBook_.bindValue(u":format"_s, format);
    queryInsertBook_.bindValue(u":archive"_s, archive);
    if(!queryInsertBook_.exec()){
        LogWarning << queryInsertBook_.lastError().text();
        return 0;
    }

    uint id = queryInsertBook_.lastInsertId().toUInt();
    mIdBookInLib_[idInLib] = id;

    QVariantList lTags;
    if(pTags != nullptr )
        lTags = *pTags;
    if(auto it = mBooksTags_.find(idInLib); it != mBooksTags_.end()) {
        const auto& tags = it->second;
        lTags.reserve(lTags.size() + it->second.size());
        std::ranges::copy(tags, std::back_inserter(lTags));
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
        if(!queryInsertBookTag_.execBatch()) [[unlikely]]
            LogWarning << queryInsertBookTag_.lastError().text();
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
        queryInsertBookGenre_.bindValue(u":idBook"_s, idBook);
        queryInsertBookGenre_.bindValue(u":idGenre"_s, idGenre);
        queryInsertBookGenre_.bindValue(u":idLib"_s, idLib);
        if(!queryInsertBookGenre_.exec()) [[unlikely]]
            LogWarning << queryInsertBookGenre_.lastError().text();
    }else [[unlikely]]
        qDebug() << u"Неизвестный жанр: "_s + sGenre;
}

void ImportThread::cleanUnsortedGenre()
{
    QString sql = u"DELETE FROM book_genre "
                  u"WHERE book_genre.id_genre = 1112 "
                  u"AND EXISTS ( "
                  u"  SELECT 1 "
                  u"  FROM book_genre bg "
                  u"  WHERE bg.id_book = book_genre.id_book "
                  u"  AND bg.id_genre != 1112 "
                  u");"_s;
    if (!query_.exec(sql))
        LogWarning << "Error cleaning genre 1112: " << query_.lastError().text();
}

void ImportThread::init(uint id, SLib &lib, uchar nUpdateType)
{
    sInpxFile_ = RelativeToAbsolutePath(lib.sInpx);
    if(!QFileInfo::exists(sInpxFile_) || !QFileInfo(sInpxFile_).isFile())
        sInpxFile_ = lib.sInpx;
    sPath_ = RelativeToAbsolutePath(lib.path);
    sName_ = lib.name;
    nUpdateType_ = nUpdateType;
    idLib_ = id;
    stopped_ = false;
    bFirstAuthorOnly_ = lib.bFirstAuthor;
    bWoDeleted_ = lib.bWoDeleted;
    if(nUpdateType_ == UT_NEW){
        if(lib.books.empty()){
            QSqlQuery query(QSqlDatabase::database(u"libdb"_s));
            query.setForwardOnly(true);
            query.prepare(u"SELECT book.id, book.id_inlib, book_sequence.id_sequence FROM book JOIN book_sequence ON book.id = book_sequence.id_book WHERE book.id_lib=:id_lib;"_s);
            //                     0        1              2
            query.bindValue(u":id_lib"_s,idLib_);
            if(!query.exec()) [[unlikely]]
                LogWarning << query.lastError().text();
            else{
                while (query.next()){
                    uint idBook = query.value(0).toUInt();
                    uint idInLib = query.value(1).toUInt();
                    uint idSequence = query.value(2).toUInt();
                    mIdBookInLib_[idInLib] = idBook;
                    mBookSequences_[idBook].push_back(idSequence);
                }
            }
            query.prepare(u"SELECT id, name FROM seria WHERE id_lib=:id_lib;"_s);
            query.bindValue(u":id_lib"_s,idLib_);
            if(!query.exec()) [[unlikely]]
                LogWarning << query.lastError().text();
            else{
                while (query.next()){
                    uint id = query.value(0).toUInt();
                    QString sName = query.value(1).toString();
                    mSequences_[sName] = id;
                }
            }
        }else{
            mIdBookInLib_.reserve(lib.books.size());
            for(const auto &[idBook, book] :lib.books){
                mIdBookInLib_[book.idInLib] = idBook;
                for(auto it :book.mSequences)
                    mBookSequences_[idBook].push_back(it.first);
            }
            for(const auto &[id, sequence] :lib.serials)
                mSequences_[sequence.sName] = id;
        }

        if(lib.authors.empty()){
            QSqlQuery query(QSqlDatabase::database(u"libdb"_s));
            query.setForwardOnly(true);
            query.prepare(u"SELECT id, name1, name2, name3 FROM author WHERE id_lib=:idLib;"_s);
            //                     0   1      2      3
            query.bindValue(u":idLib"_s, idLib_);
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
    }
    if(nUpdateType_ == UT_FULL){
        if(lib.authors.empty()){
            std::unordered_map<int, SAuthor> tempAuthors;
            QSqlQuery query(QSqlDatabase::database(u"libdb"_s));
            query.setForwardOnly(true);
            query.prepare(u"SELECT DISTINCT a.id, a.name1, a.name2, a.name3, a.id_lib "
                          u"FROM author a "
                          u"JOIN author_tag at ON a.id = at.id_author "
                          u"WHERE a.id_lib = :idLib"_s);
            query.bindValue(u":idLib"_s, idLib_);
            if (!query.exec()) [[unlikely]]
                LogWarning << query.lastError().text();
            else{
                while (query.next()) {
                    int id = query.value(0).toInt();
                    SAuthor author;
                    author.sFirstName = query.value(2).toString().trimmed();;
                    author.sLastName = query.value(1).toString().trimmed();;
                    author.sMiddleName = query.value(3).toString().trimmed();;
                    tempAuthors[id] = author;
                }
            }
            query.prepare(u"SELECT at.id_author, at.id_tag FROM author_tag at JOIN author a ON a.id = at.id_author WHERE a.id_lib = :idLib"_s);
            query.bindValue(u":idLib"_s, idLib_);

            if (!query.exec()) [[unlikely]]
                LogWarning << query.lastError().text();
            else{
                while (query.next()) {
                    uint idAuthor = query.value(0).toUInt();
                    uint idTag = query.value(1).toUInt();
                    if (tempAuthors.contains(idAuthor))
                        tempAuthors[idAuthor].idTags.insert(idTag);
                }
            }
            for (const auto& pair : tempAuthors)
                stTagetAuthors_.insert(pair.second);
        }else{
            for(auto &[idAuthor, author] :lib.authors){
                if(!author.idTags.empty())
                    stTagetAuthors_.insert(std::move(author));
            }
        }

        if(lib.books.empty()){
            std::unordered_map<int, SAuthor> tempAuthors;
            QSqlQuery query(QSqlDatabase::database(u"libdb"_s));
            query.setForwardOnly(true);
            query.prepare(u"SELECT b.id_inlib, bt.id_tag FROM book_tag bt JOIN book b ON b.id = bt.id_book WHERE b.id_lib = :idLib"_s);
            query.bindValue(u":idLib"_s, idLib_);
            if (!query.exec()) [[unlikely]]
                LogWarning << query.lastError().text();
            else{
                while (query.next()) {
                    uint idBook = query.value(0).toUInt();
                    uint idTag = query.value(1).toUInt();
                    mBooksTags_[idBook].insert(idTag);
                }
            }
        }else{
            for(const auto &[idBook, book] :lib.books){
                if(!book.idTags.empty() && book.idInLib!=0)
                    mBooksTags_[book.idInLib] = std::move(book.idTags);
            }
        }

        if(lib.serials.empty()){
            QSqlQuery query(QSqlDatabase::database(u"libdb"_s));
            query.prepare(u"SELECT s.name, st.id_tag FROM seria s JOIN seria_tag st ON s.id = st.id_seria WHERE s.id_lib = :idLib"_s);
            query.setForwardOnly(true);
            query.bindValue(u":idLib"_s, idLib_);
            if (!query.exec()) [[unlikely]]
                LogWarning << query.lastError().text();
            else{
                while (query.next()) {
                    QString sName = query.value(0).toString();
                    uint idTag = query.value(1).toUInt();
                    mSequenceTags_[sName].insert(idTag);
                }
            }
        }else{
            for(const auto &[idSequence, sequence] :lib.serials){
                if(!sequence.idTags.empty())
                    mSequenceTags_[sequence.sName] = std::move(sequence.idTags);
            }
        }
    }
}

void ImportThread::init(uint id, SLib &lib, const QStringList &files)
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
        QString sTempZipDir = QDir::tempPath() + u"/freeLib/zip/"_s ;
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
    query_.exec(u"SELECT id FROM book where id_lib=%1 and file='%2' and archive='%3'"_s.arg(QString::number(idLib_), file_name, arh_name));
    if(query_.next()) //если книга найдена, то просто снимаем пометку удаления
    {
        query_.exec(u"update book set deleted=0 where id="_s + query_.value(0).toString());
        return;
    }

    QDomDocument doc;
    doc.setContent(ba);
    QDomElement title_info = doc.elementsByTagName(u"title-info"_s).at(0).toElement();
    QString sTitle = title_info.elementsByTagName(u"book-title"_s).at(0).toElement().text();
    QString sLanguage = title_info.elementsByTagName(u"lang"_s).at(0).toElement().text().left(2);
    QString sSeria = title_info.elementsByTagName(u"sequence"_s).at(0).attributes().namedItem(QStringLiteral("name")).toAttr().value().trimmed();
    uint numInSerial = title_info.elementsByTagName(u"sequence"_s).at(0).attributes().namedItem(QStringLiteral("number")).toAttr().value().trimmed().toUInt();

    std::vector<SAuthor> vAuthors;
    QDomNodeList authors = title_info.elementsByTagName(u"author"_s);
    for(int i=0; i<authors.count(); i++)
    {
        SAuthor author;
        auto element = authors.at(i).toElement();
        author.sFirstName = element.elementsByTagName(u"first-name"_s).at(0).toElement().text().trimmed();
        author.sLastName = element.elementsByTagName(u"last-name"_s).at(0).toElement().text().trimmed();
        author.sMiddleName = element.elementsByTagName(u"middle-name"_s).at(0).toElement().text().trimmed();
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

    uint idBook = addBook(0, sTitle, numInSerial, file_name, (file_size == 0 ?ba.size() :file_size), 0, false, fi.suffix(), QDate::currentDate(),
                                sLanguage, u""_s, idLib_, arh_name);
    addSequence(sSeria, idLib_, idBook, numInSerial);

    bool first_author = true;
    for(const SAuthor &author: vAuthors)
    {
        addAuthor(author, idLib_, idBook, first_author);
        first_author = false;
        if(bFirstAuthorOnly_)
            break;
    }
    for(const auto &sGenre: std::as_const(listGenres))
        AddGenre(idBook, sGenre, idLib_);
}

void ImportThread::readEPUB(const QByteArray &ba, QString sFileName, QString sArhName, qint32 fileSize) {
    if (sArhName.isEmpty()) {
        sFileName = sFileName.right(sFileName.length() - sPath_.length());
        if (!sFileName.isEmpty() && (sFileName.at(0) == '/' || sFileName.at(0) == '\\')) {
            sFileName = sFileName.right(sFileName.length() - 1);
        }
    } else {
        QString sTempZipDir = QDir::tempPath() + u"/freeLib/zip/"_s;
        if (sArhName.startsWith(sTempZipDir)) {
            sArhName = sArhName.right(sArhName.length() - sTempZipDir.length());
            sArhName.replace(u".zip.dir/"_s, u".zip/"_s);
        } else {
            sArhName = sArhName.right(sArhName.length() - sPath_.length());
        }
        if (!sArhName.isEmpty() && (sArhName.at(0) == '/' || sArhName.at(0) == '\\')) {
            sArhName = sArhName.right(sArhName.length() - 1);
        }
    }

    sFileName = sFileName.left(sFileName.length() - 5);
    query_.exec(u"SELECT id FROM book WHERE id_lib=%1 AND file='%2' AND archive='%3'"_s
                    .arg(QString::number(idLib_), sFileName, sArhName));
    if (query_.next()) {
        query_.exec(u"UPDATE book SET deleted=0 WHERE id=%1"_s.arg(query_.value(0).toString()));
        return;
    }

    EpubReader reader(ba);
    EpubReader::BookMetadata metadata;
    if (!reader.readMetadata(metadata)) {
        LogWarning << "Failed to read metadata for:" << sFileName;
        return;
    }

    uint idBook = addBook(0, metadata.title, 0, sFileName, (fileSize == 0 ? ba.size() : fileSize),
                          0, false, u"epub"_s, QDate::currentDate(),
                          metadata.language, u""_s, idLib_, sArhName);
    if(idBook == 0) {
        LogWarning << "Failed to add book:" << metadata.title;
        return;
    }

    bool first_author = true;
    for(const SAuthor &author : metadata.authors) {
        addAuthor(author, idLib_, idBook, first_author);
        if(bFirstAuthorOnly_)
            break;
        first_author = false;
    }

    for(const auto &genre : metadata.genres)
        AddGenre(idBook, genre, idLib_);
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
            QString fbd = fileInfo.absolutePath() % u"/"_s % fileInfo.completeBaseName() % u".fbd"_s;
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
            importBooksFromZip(sPath_ + u"/"_s, sArchName, nBooksCount);
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
    if (!uz.open(QuaZip::mdUnzip)) [[unlikely]]
    {
        LogWarning << "Error open archive:" << sZipFileName;
        return;
    }

    QuaZipFile fileInZip(&uz);
    const auto list = uz.getFileInfoList64();
    for(const auto &fiZip: list  )
    {
        if(stopped_.load(std::memory_order_relaxed)) [[unlikely]]
            break;

        if(fiZip.uncompressedSize == 0) [[unlikely]]
            continue;
        QByteArray fileData;
        bool isFB2 = fiZip.name.endsWith(u".fb2", Qt::CaseInsensitive);
        bool isEPUB = fiZip.name.endsWith(u".epub", Qt::CaseInsensitive);

        if(isFB2 || isEPUB)
        {
            setCurrentZipFileName(&uz, fiZip.name);
            if(!fileInZip.open(QIODevice::ReadOnly)) {
                LogWarning << "Error open:" << fiZip.name << "in" << sArchName;
                continue;
            }
            if(isFB2)
                fileData = fileInZip.read(16*1024);
            else
                fileData = fileInZip.readAll();
            fileInZip.close();

            if(isFB2)
                readFB2(fileData, fiZip.name, sZipFileName, fiZip.uncompressedSize);
            else
                readEPUB(fileData, fiZip.name, sZipFileName, fiZip.uncompressedSize);
            nBooksCount++;
        }
        else if(fiZip.name.endsWith(u".zip", Qt::CaseInsensitive))
        {
            setCurrentZipFileName(&uz, fiZip.name);
            if(!fileInZip.open(QIODevice::ReadOnly)) {
                LogWarning << "Error open " << fiZip.name << " in " << sArchName;
                continue;
            }
            fileData = fileInZip.readAll();
            fileInZip.close();
            QString sZipDir;
            if(sPath.startsWith(QDir::tempPath() + u"/freeLib/zip/"_s))
                sZipDir = sPath % sArchName % u".dir/"_s;
            else
                sZipDir = QDir::tempPath() % u"/freeLib/zip/"_s % sArchName % u".dir/"_s;
            QDir dirZip(sZipDir);
            dirZip.mkpath(sZipDir);
            QFile fileZiped(sZipDir + fiZip.name);
            fileZiped.open(QIODevice::WriteOnly);
            fileZiped.write(fileData);
            fileZiped.close();
            importBooksFromZip(sZipDir , fiZip.name, nBooksCount);

            fileZiped.remove();
        }
        else if(fiZip.name.endsWith(u".fbd", Qt::CaseInsensitive))
        {
            QFileInfo fi(fiZip.name);
            if(!fi.completeBaseName().isEmpty() && fi.completeBaseName().at(0) != '.')
            {
                QString fbd = fi.completeBaseName() + u".fbd"_s;
                if(setCurrentZipFileName(&uz, fbd))
                {
                    if(!fileInZip.open(QIODevice::ReadOnly)) {
                        LogWarning << "Error open:" << fiZip.name << "in" << sArchName;
                        continue;
                    }
                    fileData = fileInZip.readAll();
                    readFB2(fileData, fiZip.name, sZipFileName, fiZip.uncompressedSize);
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
    if (!dbase.open()) [[unlikely]]
    {
        LogWarning << dbase.lastError().text();
        return;
    }

    query_ = QSqlQuery(dbase);
    query_.setForwardOnly(true);
    std::unordered_map<QString, uint> mTags;
    mTags.clear();
    query_.exec(u"PRAGMA cache_size = 2000"_s);
    query_.exec(u"SELECT id,name FROM tag"_s);
    while(query_.next())
    {
        uint idTag = query_.value(0).toUInt();
        QString sTagName = query_.value(1).toString().trimmed();
        mTags[sTagName] = idTag;
    }

    for(auto &[idGenre, genre] :g::genres){
        for(auto &sKey : genre.listKeys)
            mGenreKeys_[sKey] = idGenre;
    }

    queryInsertBook_ = QSqlQuery(dbase);
    queryInsertBook_.prepare(u"INSERT INTO book(name,star,language,file,size,'deleted',date,keys,id_inlib,id_lib,format,archive) "
                              "values(:name,:star,:language,:file,:size,:deleted,:date,:keys,:id_inlib,:id_lib,:format,:archive);"_s);

    queryInsertAuthor_ = QSqlQuery(dbase);
    queryInsertAuthor_.prepare(QStringLiteral("INSERT INTO author(name1,name2,name3,id_lib) values(:name1,:name2,:name3,:id_lib);"));

    queryInsertSeria_ = QSqlQuery(dbase);
    queryInsertSeria_.prepare(QStringLiteral("INSERT INTO seria(name,id_lib) values(:name,:id_lib)"));

    queryInsertBookAuthor_ = QSqlQuery(dbase);
    queryInsertBookAuthor_.prepare(QStringLiteral("INSERT OR IGNORE INTO book_author(id_book,id_author,id_lib) values(:idBook,:idAuthor,:idLib);"));

    queryInsertBookSequence_ = QSqlQuery(dbase);
    queryInsertBookSequence_.prepare(u"INSERT OR IGNORE INTO book_sequence(id_book,id_sequence,num_in_sequence) values(:idBook,:idSequence,:numInSequence);"_s);


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
    if (!uz.open(QuaZip::mdUnzip)) [[unlikely]]
    {
        LogWarning << "Error open inpx!";
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
        if(!query_.exec(u"UPDATE lib SET version='"_s % sVersion % u"' WHERE id="_s % QString::number(idLib_))) [[unlikely]]
            LogWarning << query_.lastError().text();
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
        if(stopped_.load(std::memory_order_relaxed)) [[unlikely]]
            break;
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
            if(line.isEmpty()) [[unlikely]]
                continue;

            QCoreApplication::processEvents();
            if(stopped_.load(std::memory_order_relaxed)) [[unlikely]]
                break;

            QStringList substrings = line.split(QChar(4));
            if(substrings.count() == 0) [[unlikely]]
                continue;
            QString name;
            if(substrings.count() > field_index[_NAME])
            {
                name = substrings[field_index[_NAME]].trimmed();
                if(name.isEmpty()) [[unlikely]]
                    continue;
            }
            QString sSequence;
            if(substrings.count() > field_index[_SERIA])
                sSequence = substrings[field_index[_SERIA]].trimmed();
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
                if(nUpdateType_ == UT_NEW && mIdBookInLib_.contains(idInLib)){
                    if(sSequence.isEmpty())
                        continue;
                    if( mSequences_.contains(sSequence)){
                        auto idBook = mIdBookInLib_.at(idInLib);
                        uint idSequence = mSequences_.at(sSequence);
                        if(mBookSequences_.contains(idBook)){
                            const auto &vSequences = mBookSequences_.at(idBook);

                            if(std::ranges::find(vSequences, idSequence) != vSequences.end())
                                continue;
                        }
                    }
                }
            }
            bool deleted = 0;
            if(substrings.count()>field_index[_DELETED])
            {
                deleted = (substrings[field_index[_DELETED]].trimmed().toInt() > 0);
            }
            QString format;
            if(substrings.count() > field_index[_FORMAT])
            {
                format = substrings[field_index[_FORMAT]].trimmed().toLower();
                if(format == u"zip"){
                    if(file.endsWith(u".pdf", Qt::CaseInsensitive)){
                        format = u"pdf.zip"_s;
                        file.chop(4);
                        if(name.endsWith(u"(pdf)"))
                            name.chop(5);
                    } else
                    if(file.endsWith(u".djvu", Qt::CaseInsensitive)){
                        format = u"djvu.zip"_s;
                        file.chop(5);
                        if(name.endsWith(u"(djvu)") || name.endsWith(u"[djvu]"))
                            name.chop(6);
                    } else
                    if(file.endsWith(u".epub", Qt::CaseInsensitive)){
                        format = u"epub.zip"_s;
                        file.chop(5);
                        if(name.endsWith(u"(epub)") || name.endsWith(u"[epub]"))
                            name.chop(6);
                    }
                }
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
            if(!bWoDeleted_ || !deleted){
                uint idBook = addBook(star, name, num_in_seria, file, size, idInLib, deleted, format, date, language, keys, idLib_, folder, &listBookTags);
                if(!sSequence.isEmpty())
                    addSequence(sSequence, idLib_, idBook, num_in_seria, &listSeriaTags);


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
                            if (!archiveFile.open(QuaZip::mdUnzip)) [[unlikely]]
                                LogWarning << "Error open archive:" << sArchive;
                        }
                        QuaZipFile zip_file(&archiveFile);
                        setCurrentZipFileName(&archiveFile, sFile);
                        if(!zip_file.open(QIODevice::ReadOnly)) [[unlikely]]
                            LogWarning << "Error open file:" << sFile;
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
                            if(!sAuthorLow.contains(u"неизвест"_s) && !sAuthorLow.isEmpty()) [[unlikely]] {
                                bool bFirstAuthor = author_count == 0;
                                addAuthor(author, idLib_, idBook, bFirstAuthor, bFirstAuthor ?&listAuthorTags :nullptr);
                                author_count++;
                            }
                        }
                    }else{
                        if(!bUnkownAuthor) [[unlikely]]{
                            SAuthor author(sAuthor);
                            bool bFirstAuthor = author_count == 0;
                            addAuthor(author, idLib_, idBook, bFirstAuthor, bFirstAuthor ?&listAuthorTags :nullptr);
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
                        AddGenre(idBook, genre.trimmed(), idLib_);
                        first = false;
                    }
                }
                count++;
                if(nBooksCount == 0)
                    iStart = i;
                nBooksCount++;
                if(count == 500) [[unlikely]]
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
    cleanUnsortedGenre();
    dbase.commit();
    if(!stopped_) [[likely]]
        emit progress(nBooksCount, 1.0f);

    emit End();
    dbase.close();
}

void ImportThread::break_import()
{
    stopped_.store(true, std::memory_order_relaxed);
}

