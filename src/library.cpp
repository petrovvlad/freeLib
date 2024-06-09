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
#include <QtConcurrent>

#ifdef QUAZIP_STATIC
#include "quazip/quazip/quazipfile.h"
#else
#include <quazip/quazipfile.h>
#endif

#include "utilites.h"
#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
#include "options.h"
#endif


std::unordered_map<uint, SLib> libs;
std::unordered_map<ushort, SGenre> genres;
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
            lib.authors.emplace(0, SAuthor());//insert(0, SAuthor());
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
                    lib.serials[idSeria].vIdTags.push_back(idTag);
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
                    lib.authors[idAuthor].vIdTags.push_back(idTag);
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
            auto iLang = std::find(lib.vLaguages.cbegin(), lib.vLaguages.cend(), sLaguage);
            int idLanguage = iLang - lib.vLaguages.cbegin();
            if(idLanguage == lib.vLaguages.size())
                lib.vLaguages.push_back(sLaguage);
            book.idLanguage = static_cast<uchar>(idLanguage);
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
                    lib.books[idBook].vIdGenres.push_back(idGenre);
            }
            query.prepare(QStringLiteral("SELECT book_tag.id_book, book_tag.id_tag FROM book_tag INNER JOIN book ON book.id = book_tag.id_book WHERE book.id_lib = :id_lib"));
            query.bindValue(QStringLiteral(":id_lib"),idLibrary);
            if(!query.exec())
                qDebug() << query.lastError().text();
            while (query.next()) {
                uint idBook = query.value(0).toUInt();
                uint idTag = query.value(1).toUInt();
                if(lib.books.contains(idBook))
                    lib.books[idBook].vIdTags.push_back( idTag );
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
            lib.authorBooksLink.insert({idAuthor, idBook});
            lib.books[idBook].vIdAuthors.push_back(idAuthor);
        }
    }
    for(auto &book :lib.books){
        if(book.second.vIdAuthors.empty()){
            book.second.vIdAuthors.push_back(0);
            lib.authorBooksLink.insert({0, book.first});
        }
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
    QStringList listNames = sName.split(u","_s);
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
#ifdef __cpp_lib_ranges
    auto it = std::ranges::find_if(authors, [&author](const auto &a){
        return author.sFirstName == a.second.sFirstName && author.sMiddleName == a.second.sMiddleName && author.sLastName == a.second.sLastName;
    });
    if(it != authors.cend())
        idAuthor = it->first;

#else
    for(const auto &iAuthor :authors){
        if(author.sFirstName == iAuthor.second.sFirstName && author.sMiddleName == iAuthor.second.sMiddleName && author.sLastName == iAuthor.second.sLastName){
            idAuthor = iAuthor.first;
            break;
        }
    }
#endif
    return idAuthor;
}

uint SLib::findSerial(const QString &sSerial) const
{
    uint idSerial = 0;
#ifdef __cpp_lib_ranges
    auto it = std::ranges::find_if(serials, [&sSerial](const auto &s){
        return sSerial == s.second.sName;
    });
    if(it != serials.cend())
        idSerial = it->first;
#else
    for(const auto &iSerial :serials){
        if(sSerial == iSerial.second.sName){
            idSerial = iSerial.first;
            break;
        }
    }
#endif
    return idSerial;
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
            const auto listSerials =  serials[book.idSerial].sName.split(QStringLiteral(" "));
            for(const QString &sSerial: listSerials)
            {
                if(!sSerial.isEmpty())
                    abbr += sSerial.at(0);
            }
        }
        result.replace(u"%abbrs"_s, abbr.toLower());

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
    for(auto &book :books){
        auto &vIdTags = book.second.vIdTags;
        auto it = std::find(vIdTags.cbegin(), vIdTags.cend(), idTag);
        if(it!=vIdTags.end())
            vIdTags.erase(it);
    }
    for(auto &iSerial :serials){
        auto &vIdTags = iSerial.second.vIdTags;
        auto it = std::find(vIdTags.begin(), vIdTags.end(), idTag);
        if(it != vIdTags.end())
            vIdTags.erase(it);
    }
    for(auto &iAuthor :authors){
        auto &vIdTags = iAuthor.second.vIdTags;
        auto it = std::find(vIdTags.cbegin(), vIdTags.cend(), idTag);
        if(it != vIdTags.end())
            vIdTags.erase(it);
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
            MyDBG << "Error open INPX file: " << sInpx;
        } else
            if(setCurrentZipFileName(&uz, u"COLLECTION.INFO"_s))
            {
                QBuffer outbuff;
                QuaZipFile zip_file(&uz);
                zip_file.open(QIODevice::ReadOnly);
                outbuff.setData(zip_file.readAll());
                zip_file.close();
                sName = QString::fromUtf8(outbuff.data().left(outbuff.data().indexOf('\n')));
            }
    }
    return sName.simplified();
}
