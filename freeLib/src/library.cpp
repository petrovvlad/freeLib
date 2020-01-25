#include "common.h"
#include "library.h"


//SLib current_lib;
QMap<int,SLib> mLibs;
QMap <uint,SGenre> mGenre;

void loadLibrary(uint idLibrary)
{
    if(!db_is_open)
        return;

    qint64 t_start = QDateTime::currentMSecsSinceEpoch();
    QSqlQuery query(QSqlDatabase::database("libdb"));
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
    query.prepare("SELECT author.id, name1||' '||name2||' '||name3, author.favorite FROM author WHERE id_lib=:id_lib;");
    query.bindValue(":id_lib",idLibrary);
    query.exec();
    while (query.next()) {
        uint idAuthor = query.value(0).toUInt();
        QString sName = query.value(1).toString().trimmed();
        if(sName.isEmpty())
            sName = QCoreApplication::translate("MainWindow","unknown author");
        int nTag = query.value(2).toInt();
        lib.mAuthors[idAuthor].sName = sName;
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
        book.nFile = query.value(6).toUInt();
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

    t_end = QDateTime::currentMSecsSinceEpoch();
    qDebug()<< "loadBooks " << t_end-t_start << "msec";

}

void loadGenres()
{
    if(!db_is_open)
        return;
    qint64 t_start = QDateTime::currentMSecsSinceEpoch();
    QSqlQuery query(QSqlDatabase::database("libdb"));

    t_start = QDateTime::currentMSecsSinceEpoch();
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
