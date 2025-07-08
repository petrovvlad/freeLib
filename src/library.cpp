#define QT_USE_QSTRINGBUILDER
#include "library.h"

#include <QDomDocument>
#include <QStringBuilder>
#include <QSqlQuery>
#include <QSqlError>
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

void loadLibrary(uint idLibrary)
{
    if(idLibrary == 0)
        return;
    if(!QSqlDatabase::database(QStringLiteral("libdb"), false).isOpen())
        return;

    qint64 timeStart, timeEnd;
    if(g::bVerbose)
        timeStart = QDateTime::currentMSecsSinceEpoch();

    QSqlQuery query(QSqlDatabase::database(QStringLiteral("libdb")));
    query.setForwardOnly(true);
    if(!query.exec(u"SELECT id FROM lib WHERE id=%1;"_s.arg(idLibrary)) || !query.first()) [[unlikely]]
        return;
    SLib& lib = g::libs[idLibrary];
    auto future = QtConcurrent::run([idLibrary, &lib]()
    {
        {
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
            QSqlDatabase dbReadAuthors = QSqlDatabase::cloneDatabase(u"libdb"_s, u"readauthors"_s);
#else
            QFileInfo fi(RelativeToAbsolutePath(g::options.sDatabasePath));
            QString sDbFile = fi.canonicalFilePath();
            QSqlDatabase dbReadAuthors = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("readauthors"));
            dbReadAuthors.setDatabaseName(sDbFile);
#endif
            lib.authors.clear();
            lib.authors.emplace(0, SAuthor());
            if (!dbReadAuthors.open()) [[unlikely]]
            {
                LogWarning << dbReadAuthors.lastError().text();
                return;
            }
            QSqlQuery query(dbReadAuthors);
            query.setForwardOnly(true);

            lib.serials.clear();
            query.prepare(u"SELECT id, name FROM seria WHERE id_lib=:id_lib;"_s);
            //                     0   1
            query.bindValue(u":id_lib"_s, idLibrary);
            if(!query.exec()) [[unlikely]]
                LogWarning << query.lastError().text();
            while (query.next()) {
                uint idSerial = query.value(0).toUInt();
                QString sName = query.value(1).toString();
                lib.serials[idSerial].sName = sName;
            }
            query.prepare(u"SELECT seria_tag.id_seria, seria_tag.id_tag FROM seria_tag INNER JOIN seria ON seria.id = seria_tag.id_seria WHERE seria.id_lib = :id_lib"_s);
            query.bindValue(u":id_lib"_s,idLibrary);
            if(!query.exec()) [[unlikely]]
                LogWarning << query.lastError().text();
            while (query.next()) {
                uint idSeria = query.value(0).toUInt();
                uint idTag = query.value(1).toUInt();
                if(lib.serials.contains(idSeria))
                    lib.serials[idSeria].idTags.insert(idTag);
            }

            query.prepare(u"SELECT author.id, name1, name2, name3 FROM author WHERE id_lib=:id_lib;"_s);
            //                     0          1      2      3
            query.bindValue(u":id_lib"_s, idLibrary);
            query.exec();
            while (query.next()) {
                uint idAuthor = query.value(0).toUInt();
                SAuthor &author = lib.authors[idAuthor];
                author.sFirstName = query.value(2).toString().trimmed();;
                author.sLastName = query.value(1).toString().trimmed();;
                author.sMiddleName = query.value(3).toString().trimmed();;
            }
            query.prepare(u"SELECT author_tag.id_author, author_tag.id_tag FROM author_tag INNER JOIN author ON author.id = author_tag.id_author WHERE author.id_lib = :id_lib"_s);
            //                     0                     1
            query.bindValue(u":id_lib"_s, idLibrary);
            if(!query.exec()) [[unlikely]]
                LogWarning << query.lastError().text();
            while (query.next()) {
                uint idAuthor = query.value(0).toUInt();
                uint idTag = query.value(1).toUInt();
                if(lib.authors.contains(idAuthor))
                    lib.authors[idAuthor].idTags.insert(idTag);
            }
        }
        QSqlDatabase::removeDatabase(u"readauthors"_s);
    });

#ifdef __cpp_lib_atomic_wait
    std::atomic_int_fast32_t anCount[2] = {0, 0};
    std::atomic_uint anTotalCount{0};
    std::atomic_bool abFinished{false};

    const uint sizeBufffer{30000};
    constexpr uint nColCount = 13;
    QVariant *buff = new QVariant[nColCount*sizeBufffer*2];

    auto futureReadBooks = QtConcurrent::run([&]
    {
        {
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
            QSqlDatabase dbReadBooks = QSqlDatabase::cloneDatabase(QStringLiteral("libdb"), QStringLiteral("readdb"));
#else //QT_VERSION
            QFileInfo fi(RelativeToAbsolutePath(g::options.sDatabasePath));
            QString sDbFile = fi.canonicalFilePath();
            QSqlDatabase dbReadBooks = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("importdb"));
            dbReadBooks.setDatabaseName(sDbFile);
#endif //QT_VERSION
            uint nBuff = 0;
            if (!dbReadBooks.open()) [[unlikely]]
            {
                LogWarning << dbReadBooks.lastError().text();
                return;
            }
            QSqlQuery query(dbReadBooks);
            query.setForwardOnly(true);
            query.prepare(u"SELECT id, name, star, language, file, size, deleted, date, format, id_inlib, archive, first_author_id, keys FROM book WHERE id_lib=:id_lib;"_s);
            //                     0   1     2     3         4     5     6        7     8       9         10       11               12
            query.bindValue(u":id_lib"_s, idLibrary);
            if(!query.exec()) [[unlikely]]
                LogWarning << query.lastError().text();
            uint i = 0;
            while (query.next()) {
                anCount[nBuff].wait(sizeBufffer);
                int k = nBuff * nColCount * sizeBufffer + i * nColCount;
                for(int iValue = 0; iValue<nColCount; iValue++){
                    buff[k + iValue] = query.value(iValue);
                }
                if(++i == sizeBufffer) [[unlikely]]{
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
        QSqlDatabase::removeDatabase(u"readdb"_s);
    });

    lib.books.clear();
    uint nBuff = 0;
    uint nTotalCount{0};
    while(!abFinished || nTotalCount != anTotalCount){
        anCount[nBuff].wait(-1);
        uint nCount = anCount[nBuff].load();
        for(uint i =0; i<nCount; i++){
            nTotalCount++;
            uint k = nBuff * nColCount * sizeBufffer + i * nColCount;
            QString sName = buff[k + 1].toString();
            if(sName.isEmpty()) [[unlikely]]
                continue;
            uint id = buff[k].toUInt();
            SBook &book = lib.books[id];
            book.sName = sName;
            book.nStars = qvariant_cast<uchar>(buff[k+2]);
            QString sLaguage = buff[k + 3].toString().toLower();
            auto iLang = std::find(lib.vLaguages.cbegin(), lib.vLaguages.cend(), sLaguage);
            int idLanguage = iLang - lib.vLaguages.cbegin();
            if(idLanguage == lib.vLaguages.size()) [[unlikely]]
                lib.vLaguages.push_back(std::move(sLaguage));
            book.idLanguage = static_cast<uchar>(idLanguage);
            book.sFile = buff[k + 4].toString();
            book.nSize = buff[k + 5].toUInt();
            book.bDeleted = buff[k + 6].toBool();
            book.date = buff[k + 7].toDate();
            if(lib.earliestDate_.isNull() || book.date<lib.earliestDate_)
                lib.earliestDate_ = book.date;
            book.sFormat = buff[k + 8].toString();
            book.idInLib = buff[k + 9].toUInt();
            book.sArchive = buff[k + 10].toString();
            book.idFirstAuthor = buff[k + 11].toUInt();
            book.sKeywords = buff[k + 12].toString();
        }
        anCount[nBuff].store(-1);
        anCount[nBuff].notify_all();
        nBuff = (nBuff==0) ?1 :0;
    }
    delete[] buff;
#else //__cpp_lib_atomic_wait
    lib.books.clear();
    query.prepare(u"SELECT id, name, star, language, file, size, deleted, date, format, id_inlib, archive, first_author_id, keys FROM book WHERE id_lib=:id_lib;"_s);
    //                     0   1     2     3         4     5     6        7     8       9         10       11               12
    query.bindValue(u":id_lib"_s,idLibrary);
    if(!query.exec())
        LogWarning << query.lastError().text();
    while (query.next()) {
        QString sName = query.value(1).toString();
        if(sName.isEmpty())
            continue;
        uint id = query.value(0).toUInt();
        SBook &book = lib.books[id];
        book.sName = sName;
        book.nStars = qvariant_cast<uchar>(query.value(2));
        QString sLaguage = query.value(3).toString().toLower();
        auto iLang = std::find(lib.vLaguages.cbegin(), lib.vLaguages.cend(), sLaguage);
        int idLanguage = iLang - lib.vLaguages.cbegin();
        if(idLanguage == lib.vLaguages.size())
            lib.vLaguages.push_back(sLaguage);
        book.idLanguage = static_cast<uchar>(idLanguage);
        book.sFile = query.value(4).toString();
        book.nSize = query.value(5).toUInt();
        book.bDeleted = query.value(6).toBool();
        book.date = query.value(7).toDate();
        if(lib.earliestDate_.isNull() || book.date<lib.earliestDate_)
            lib.earliestDate_ = book.date;
        book.sFormat = query.value(8).toString();
        book.idInLib = query.value(9).toUInt();
        book.sArchive = query.value(10).toString();
        book.idFirstAuthor = query.value(11).toUInt();
        book.sKeywords = query.value(12).toString();
    }
#endif //__cpp_lib_atomic_wait
    future.waitForFinished();

#ifdef __cpp_lib_atomic_wait
    futureReadBooks.waitForFinished();
#endif

    future = QtConcurrent::run([idLibrary, &lib]()
    {
        {
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
            QSqlDatabase dbReadGenre = QSqlDatabase::cloneDatabase(u"libdb"_s, u"readgenre"_s);
#else
            QFileInfo fi(RelativeToAbsolutePath(g::options.sDatabasePath));
            QString sDbFile = fi.canonicalFilePath();
            QSqlDatabase dbReadGenre = QSqlDatabase::addDatabase(u"QSQLITE"_s, u"readgenre"_s);
            dbReadGenre.setDatabaseName(sDbFile);
#endif
            if (!dbReadGenre.open()) [[unlikely]]
            {
                LogWarning << dbReadGenre.lastError().text();
                return;
            }
            QSqlQuery query(dbReadGenre);
            query.setForwardOnly(true);

            //query.prepare(u"SELECT id_book, id_genre FROM book_genre WHERE id_lib=:id_lib;"_s);
            //Не влияет на суммарное время загрузки
            query.prepare(u"SELECT id_book, id_genre FROM book_genre INNER JOIN book ON book.id = book_genre.id_book WHERE book.id_lib = :id_lib"_s);
            //                     0        1
            query.bindValue(u":id_lib"_s, idLibrary);
            if(!query.exec()) [[unlikely]]
                LogWarning << query.lastError().text();
            while (query.next()) {
                uint idBook = query.value(0).toUInt();
                ushort idGenre = query.value(1).toUInt();
                if(idGenre == 0) idGenre = 1112; // Прочие/Неотсортированное
                if(lib.books.contains(idBook))
                    lib.books.at(idBook).vIdGenres.push_back(idGenre);
            }
            query.prepare(u"SELECT book_tag.id_book, book_tag.id_tag FROM book_tag INNER JOIN book ON book.id = book_tag.id_book WHERE book.id_lib = :id_lib"_s);
            //                     0                 1
            query.bindValue(u":id_lib"_s,idLibrary);
            if(!query.exec()) [[unlikely]]
                LogWarning << query.lastError().text();
            while (query.next()) {
                uint idBook = query.value(0).toUInt();
                uint idTag = query.value(1).toUInt();
                if(lib.books.contains(idBook))
                    lib.books[idBook].idTags.insert( idTag );
            }
        }
        QSqlDatabase::removeDatabase(u"readgenre"_s);
    });

    auto futureSequence = QtConcurrent::run([idLibrary, &lib]()
    {
        {
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
            QSqlDatabase dbReadSequence = QSqlDatabase::cloneDatabase(u"libdb"_s, u"readsequences"_s);
#else
            QFileInfo fi(RelativeToAbsolutePath(g::options.sDatabasePath));
            QString sDbFile = fi.canonicalFilePath();
            QSqlDatabase dbReadSequence = QSqlDatabase::addDatabase(u"QSQLITE"_s, u"readsequences"_s);
            dbReadSequence.setDatabaseName(sDbFile);
#endif
            if (!dbReadSequence.open()) [[unlikely]]
            {
                LogWarning << dbReadSequence.lastError().text();
                return;
            }
            QSqlQuery query(dbReadSequence);
            query.setForwardOnly(true);

            query.prepare(u"SELECT id_book, id_sequence, num_in_sequence FROM book_sequence INNER JOIN book ON book.id=id_book WHERE book.id_lib = :id_lib;"_s);
            //                     0        1            2
            query.bindValue(u":id_lib"_s, idLibrary);
            if(!query.exec()) [[unlikely]]
                LogWarning << query.lastError().text();
            while (query.next()) {
                uint idBook = query.value(0).toUInt();
                uint idSequence = query.value(1).toUInt();
                uint numInSequence = query.value(2).toUInt();
                if(lib.books.contains(idBook) && lib.serials.contains(idSequence)){
                    lib.books.at(idBook).mSequences[idSequence] = numInSequence;
                }
            }
        }
        QSqlDatabase::removeDatabase(u"readsequences"_s);
    });

    lib.authorBooksLink.clear();
    query.prepare(u"SELECT id_book, id_author FROM book_author WHERE id_lib=:id_lib;"_s);
    //                     0        1
    //Выполняется дольше чем предыдущий вариант
    //query.prepare(u"SELECT id_book, id_author FROM book_author INNER JOIN book ON book.id=id_book WHERE book.id_lib = :id_lib;"_s);
    query.bindValue(u":id_lib"_s, idLibrary);
    if(!query.exec()) [[unlikely]]
        LogWarning << query.lastError().text();
    while (query.next()) {
        uint idBook = query.value(0).toUInt();
        uint idAuthor = query.value(1).toUInt();
        if(lib.books.contains(idBook) && lib.authors.contains(idAuthor)) [[likely]]{
            lib.authorBooksLink.insert({idAuthor, idBook});
            lib.books[idBook].vIdAuthors.push_back(idAuthor);
        }
    }
    for(auto &[idBook, book] :lib.books){
        if(book.vIdAuthors.empty()) [[unlikely]]{
            book.vIdAuthors.push_back(0);
            lib.authorBooksLink.insert({0, idBook});
        }
    }
    future.waitForFinished();
    futureSequence.waitForFinished();
    lib.bLoaded = true;

    if(g::bVerbose){
        timeEnd = QDateTime::currentMSecsSinceEpoch();
        qDebug() << "loadLibrary " << timeEnd - timeStart << "msec";
    }
}

void loadGenres()
{
    QString sFile = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + u"/genre.tsv"_s;
    if(!QFile::exists(sFile))
        sFile = u":/genre.tsv"_s;
    QFile file(sFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LogWarning << "Cannot open file:" << sFile;
        return;
    }
    QTextStream in(&file);
    in.readLine(); // Пропускаем заголовок
    g::genres.reserve(320);
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList fields = line.split(u"\t"_s, Qt::SkipEmptyParts);
        if (fields.size() >= 3) {
            ushort id = fields[1].toUShort();
            SGenre &genre = g::genres[id];
            genre.idParrentGenre = fields[0].toUShort();
            genre.sName = fields[2];
            if(genre.idParrentGenre !=0)
                genre.listKeys = fields[3].split(u',', Qt::SkipEmptyParts);
        }
    }
    file.close();
}

SAuthor::SAuthor()
{
}

SAuthor::SAuthor(const QString &sName)
{
    QStringList listNames = sName.split(u","_s);
    if(listNames.count() > 0) [[likely]]
        sLastName = listNames[0].trimmed();
    if(listNames.count() > 1)
        sFirstName = listNames[1].trimmed();
    if(listNames.count() > 2)
        sMiddleName = listNames[2].trimmed();

}

QString SAuthor::getName() const
{
    QString sAuthorName = QStringLiteral("%1 %2 %3").arg(sLastName, sFirstName, sMiddleName).trimmed();
    if(sAuthorName.trimmed().isEmpty()) [[unlikely]]
        sAuthorName = QCoreApplication::translate("MainWindow","unknown author");
    return sAuthorName;
}

uint SLib::findAuthor(SAuthor &author) const
{
    uint idAuthor = 0;
#ifdef __cpp_lib_ranges
    auto it = std::ranges::find_if(authors, [&author](const auto &a){
        return author == a.second;
    });
    if(it != authors.cend()) [[likely]]
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

QString removeStartDots(const QString& str)
{
    static auto reg = QRegularExpression(u"[^.]"_s);
    QString result = str;
    result.remove(0, result.indexOf(reg));
    return result;
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
%id - id книги в библиотеке

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

    if(result.contains(u"%s"_s)){
        if(book.mSequences.empty()){
            if(bNestedBlock)
                return u""_s;
            else
                result.replace(u"%s"_s, u""_s);

        }else {
            result.replace(u"/%s"_s, u"/"_s + removeStartDots(serials[book.mSequences.begin()->first].sName));
            result.replace(u"%s"_s, serials[book.mSequences.begin()->first].sName);
        }
    }

    if(bNestedBlock){
        if(result.contains(u"%fi"_s) || result.contains(u"%nf"_s))
            if(sFirstAuthor.sFirstName.isEmpty())
                return u""_s;
        if(result.contains(u"%mi"_s) || result.contains(u"%nm"_s))
            if(sFirstAuthor.sMiddleName.isEmpty())
                return u""_s;
        if(result.contains(u"%li"_s) || result.contains(u"%nl"_s))
            if(sFirstAuthor.sLastName.isEmpty())
                return u""_s;
        if(result.contains(u"%abbrs"_s))
            if(book.mSequences.empty())
                return u""_s;
        if(result.contains(u"%id"_s))
            if(book.idInLib == 0)
                return u""_s;
    }else{
        result.replace(u"%app_dir"_s, QApplication::applicationDirPath() + u"/"_s);

        QString abbr = u""_s;
        if(!book.mSequences.empty()){
            const auto listSerials =  serials[book.mSequences.begin()->first].sName.split(u" "_s);
            for(const QString &sSerial: listSerials)
            {
                if(!sSerial.isEmpty())
                    abbr += sSerial.at(0);
            }
        }
        result.replace(u"%abbrs"_s, abbr.toLower());

        result.replace(u"%fi"_s, sFirstAuthor.sFirstName.isEmpty() ?u""_s :(sFirstAuthor.sFirstName.at(0)));
        result.replace(u"%mi"_s, sFirstAuthor.sMiddleName.isEmpty() ?u""_s :(sFirstAuthor.sMiddleName.at(0)));
        result.replace(u"%li"_s, sFirstAuthor.sLastName.isEmpty() ?u""_s :(sFirstAuthor.sLastName.at(0)));
        result.replace(u"%nf"_s, sFirstAuthor.sFirstName);
        result.replace(u"%nm"_s, sFirstAuthor.sMiddleName);
        result.replace(u"%nl"_s, sFirstAuthor.sLastName);
        result.replace(u"%id"_s, QString::number( book.idInLib ));

        result.replace(u"  "_s, u" "_s);
        result.replace(u"/ "_s, u"/"_s);
        result.replace(u"////"_s, u"/"_s);
        result.replace(u"///"_s, u"/"_s);
        result.replace(u"//"_s, u"/"_s);
        result.replace(u"/%b"_s, u"/"_s + removeStartDots(book.sName));
        result.replace(u"%b"_s, book.sName);
        result.replace(u"%a"_s, sFirstAuthor.getName());
    }

    auto in = result.indexOf(u"%n"_s);
    if(in >= 0)
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        int len = QStringView{result}.sliced(in + 2, 1).toInt();
#else
        int len = result.midRef(in + 2, 1).toInt();
#endif
        if(book.mSequences.empty() || book.mSequences.begin()->second == 0){
            if(bNestedBlock)
                return u""_s;
            else
                result.replace(u"%n"_s + QString::number(len), u""_s);
        }else{
            QString sNumInSeria = QString::number(book.mSequences.begin()->second);
            QString zerro;
            result.replace(u"%n"_s + (len>0 ?QString::number(len) :u""_s),
                           (len>0 ?zerro.fill(u'0', len-sNumInSeria.length()) :u""_s) + sNumInSeria);
        }
    }

    return result;
}

QString SLib::fillParams(const QString &str, uint idBook, const QFileInfo &book_file)
{
    QString result = str;
    result = fillParams(result, idBook);
    result.replace(u"%fn"_s, book_file.completeBaseName()).
        replace(u"%f"_s, book_file.filePath()/*absoluteFilePath()*/).
        replace(u"%d"_s, book_file.absoluteDir().path());
    return result;
}

void SLib::deleteTag(uint idTag)
{
    for(auto &book :books)
        book.second.idTags.erase(idTag);
    for(auto &iSerial :serials)
        iSerial.second.idTags.erase(idTag);
    for(auto &iAuthor :authors)
        iAuthor.second.idTags.erase(idTag);
    QSqlQuery query(QSqlDatabase::database(QStringLiteral("libdb")));
    query.exec(u"PRAGMA foreign_keys = ON"_s);
    query.exec(u"DELETE FROM book_tag WHERE id_tag=%1"_s.arg(idTag));
    query.exec(u"DELETE FROM seria_tag WHERE id_tag=%1"_s.arg(idTag));
    query.exec(u"DELETE FROM author_tag WHERE id_tag=%1"_s.arg(idTag));
    query.exec(u"DELETE FROM tag WHERE id=%1"_s.arg(idTag));
}

QString SLib::nameFromInpx(const QString &sInpx)
{
    QString sName;
    if(!sInpx.isEmpty()){
        QuaZip uz(sInpx);
        if(!uz.open(QuaZip::mdUnzip)) [[unlikely]]
        {
            LogWarning << "Error open INPX file: " << sInpx;
        } else
            if(setCurrentZipFileName(&uz, u"COLLECTION.INFO"_s)) [[likely]]
            {
                QBuffer outbuff;
                QuaZipFile zip_file(&uz);
                zip_file.open(QIODevice::ReadOnly);
                outbuff.setData(zip_file.readAll());
                uint nMarkerSize = 0;
                auto data = outbuff.data();
                if(outbuff.size()>4){
                    quint32 marker = *((uint*)(data.data())) & 0x00ffffff;
                    if((marker & 0x00ffffff) == 0x00bfbbef) //UTF8
                        nMarkerSize = 3;
                }
                zip_file.close();
                sName = QString::fromUtf8(data.mid(nMarkerSize, data.indexOf('\n', nMarkerSize)-nMarkerSize));

            }
    }
    return sName.simplified();
}
