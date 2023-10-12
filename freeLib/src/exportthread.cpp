#include "exportthread.h"

#include <QApplication>
#include <QTextStream>
#include <QTimer>
#include <QStringBuilder>
#include <QProcess>
#include <QFileInfo>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <unistd.h>


#include "SmtpClient/src/smtpclient.h"
#include "SmtpClient/src/mimeattachment.h"
#include "SmtpClient/src/mimetext.h"
#include "quazip/quazip/quazipfile.h"

#include "fb2mobi/fb2mobi.h"
#include "library.h"
#include "utilites.h"

QString ValidateFileName(QString str)
{
    bool windows = false;
    bool mac = false;
#ifdef WIN32
    windows=true;
#endif
#ifdef Q_OS_MAC
    mac=true;
#endif

    if(!options.bExtendedSymbols || windows)
    {

        str = str.replace('\"', '\'');
        static const QRegularExpression re(QStringLiteral("^([a-zA-Z]\\:|\\\\\\\\[^\\/\\\\:*?\"<>|]+\\\\[^\\/\\\\:*?\"<>|]+)(\\\\[^\\/\\\\:*?\"<>|]+)+(\\.[^\\/\\\\:*?\"<>|]+)$"));
        str = str.replace(re, QStringLiteral("_"));
        bool bMtp = str.startsWith(u"mtp:");
        str = str.left(bMtp ?4 :2) + str.mid(bMtp ?4 :2).replace(':', '_');
    }
    else
    {
        if(mac){
            static const QRegularExpression re(QStringLiteral("[:]"));
            str = str.replace(re, QStringLiteral("_"));
        }
    }
    return str;
}

ExportThread::ExportThread(const ExportOptions *pExportOptions) :
    QObject(nullptr)
{
    idLib_ = 0;
    pExportOptions_ = pExportOptions;
}

void ExportThread::start(uint idLib, const QString &path)
{
    stopped_.store(false, std::memory_order_relaxed);
    idLib_ = idLib;
    sExportDir_ = path;
    if(!libs[idLib].bLoaded)
        loadLibrary(idLib);
}

void ExportThread::start(const QString &_export_dir, const QList<uint> &list_books, SendType send, qlonglong id_author)
{
    idLib_ = 0;
    stopped_.store(false, std::memory_order_relaxed);
    book_list = list_books;
    send_type = send;
    IDauthor = id_author;
    sExportDir_ = RelativeToAbsolutePath(_export_dir);
}

QString BuildFileName(QString filename)
{
    return filename.replace('/', '.').replace('\\', '.').replace('*', '.').replace('|', '.').replace(':', '.').replace('?', '.')
            .replace('<', '.').replace('>', '.').replace('\"', '\'');
}

bool ExportThread::convert(QList<QBuffer*> outbuff, uint idLib, const QString &file_name, int count, uint idBook)
{
    SLib& lib = libs[idLib];
    Q_CHECK_PTR(pExportOptions_);
    QString tool_path,tool_arg,tool_ext;
    auto iTool = options.tools.constBegin();
    while(iTool != options.tools.constEnd()){
        if(iTool.key() == pExportOptions_->sCurrentTool)
        {
            tool_path = iTool->sPath;
            tool_arg = iTool->sArgs;
            tool_ext = iTool->sExt;
            break;
        }
        ++iTool;
    }

    QDir book_dir;
    QFileInfo fi(file_name);

    QString tmp_dir;
    if(QStandardPaths::standardLocations(QStandardPaths::TempLocation).count() > 0)
        tmp_dir = QStandardPaths::standardLocations(QStandardPaths::TempLocation).at(0);
    book_dir.mkpath(tmp_dir + QStringLiteral("/freeLib"));

    QStringList out_file;
    int i = 0;
    QFile file;
    for(QBuffer* buf: outbuff)
    {
        out_file << tmp_dir + QStringLiteral("/freeLib/book%1.").arg(QString::number(i)) + fi.suffix();
        i++;
        QFile::remove(out_file.last());
        file.setFileName(out_file.last());
        file.open(QFile::WriteOnly);
        file.write(buf->data());
        file.close();
    }
    if(out_file.count() == 0)
        return false;
    QString current_out_file = out_file.first();
    if(
            (pExportOptions_->sOutputFormat == u"MOBI" ||
             pExportOptions_->sOutputFormat == u"EPUB" ||
             pExportOptions_->sOutputFormat == u"AZW3" ||
             pExportOptions_->sOutputFormat == u"MOBI7" ||
             pExportOptions_->sOutputFormat == u"PDF") &&
            (fi.suffix().toLower() == u"fb2" || fi.suffix().toLower() == u"epub"))
    {
        fb2mobi conv(pExportOptions_, idLib);
        current_out_file = conv.convert(out_file, idBook);
    }

    QString book_file_name = fi.path() % QStringLiteral("/") % fi.completeBaseName() % QStringLiteral(".") % QFileInfo(current_out_file).suffix();

    if(send_type == ST_Mail)
    {
       if(count > 1)
       {
           QEventLoop loop; QTimer::singleShot(pExportOptions_->nEmailPause*1000, &loop, &QEventLoop::quit); loop.exec();
       }
       SmtpClient smtp(pExportOptions_->sEmailServer, pExportOptions_->nEmailServerPort, (SmtpClient::ConnectionType)pExportOptions_->nEmailConnectionType);

       MimeMessage msg(true);
       msg.setHeaderEncoding(MimePart::Base64);
       EmailAddress sender(pExportOptions_->sEmailFrom, QStringLiteral(""));
       msg.setSender(sender);
       EmailAddress to(pExportOptions_->sEmail, QStringLiteral(""));
       msg.addRecipient(to);
       QString caption = pExportOptions_->sEmailSubject;
       msg.setSubject(caption.isEmpty() ?QStringLiteral("freeLib") :caption);

       QBuffer outbuff;
       QString FileName = current_out_file;
       if(FileName.isEmpty())
       {
           return false;
       }
       QFile book_file(FileName);
       if(!book_file.open(QFile::ReadOnly))
       {
           qDebug()<<"Error open file name "<<FileName;
           return false;
       }
       outbuff.setData(book_file.readAll());
       book_file.close();
       MimeText *pText = new MimeText;
       QString onlyFileName = QFileInfo(book_file_name).fileName();
       pText->setText(onlyFileName);
       msg.addPart(pText);
       msg.addPart(new MimeAttachment(outbuff.data(), onlyFileName));
       smtp.connectToHost();
       smtp.waitForReadyConnected();
       smtp.login(pExportOptions_->sEmailUser, pExportOptions_->sEmailPassword);
       smtp.waitForAuthenticated();
       smtp.sendMail(msg);
       if(!smtp.waitForMailSent())
       {
           qDebug()<<"Error send e-mail.";
           return false;
       }

       smtp.quit();
       return true;
    }
    else
    {
       QStringList listArg;
       book_file_name = ValidateFileName(book_file_name);
       if(!pExportOptions_->bPostprocessingCopy)
        {
            //book_dir.mkpath(book_dir.cleanPath(QFileInfo(book_file_name).absolutePath()));
            if(book_file_name.startsWith(u"mtp:/")){
                QString sArg = QStringLiteral("move \"%1\" \"%2\"").arg(current_out_file, book_file_name);
                listArg << sArg;
                QProcess::execute(QStringLiteral("kioclient5"), listArg);

            }else{              
                QDir dir(book_file_name);
                dir.mkpath(book_dir.cleanPath(QFileInfo(book_file_name).absolutePath()));
                QFile::remove(book_file_name);
                QFile::rename(current_out_file, book_file_name);
            }
        }
        else
            book_file_name = current_out_file;
        QProcess proc;
        if(!tool_path.isEmpty())
        {
            QFileInfo fi_tmp(book_file_name);
            QString ex = lib.fillParams(tool_path, idBook, fi_tmp);
            QStringList listArg = tool_arg.split(QStringLiteral(" "));
            for(int i = 0; i != listArg.size(); ++i)
                listArg[i] = lib.fillParams(listArg[i], idBook, fi_tmp);
              //Колонка «имя выходного файла» временно спрятана
//            if(!tool_ext.isEmpty())
//            {
//                book_file_name = tool_ext;
//                book_file_name = lib.fillParams(book_file_name, idBook, fi_tmp);
//                listArg << book_file_name;
//            }
            qDebug()<<ex<<listArg;
            QProcess::execute(ex, listArg);
        }
        return QFileInfo::exists(book_file_name);
    }
}

void ExportThread::export_books()
{
    QDir dir = sExportDir_;
    QFileInfo fi;

    if(pExportOptions_->bOriginalFileName && send_type != ST_Mail && pExportOptions_->sOutputFormat == u"-")
    {
        QString LibPath = libs[idCurrentLib].path;
        for(auto idBook: book_list)
        {
            QString archive, file;
            SBook &book = libs[idCurrentLib].books[idBook];
            if(!book.sArchive.isEmpty())
            {
                archive = book.sArchive.replace(QStringLiteral(".inp"), QStringLiteral(".zip"));
            }
            file = book.sName % QStringLiteral(".") % book.sFormat;
            LibPath = RelativeToAbsolutePath(LibPath);
            archive = archive.replace('\\', '/');
            file = file.replace('\\', '/');
            if(archive.isEmpty())
            {
                QDir().mkpath(QFileInfo(dir.absolutePath() % QStringLiteral("/") % file).absolutePath());
                QFile().copy(LibPath % QStringLiteral("/") % file, dir.absolutePath() % QStringLiteral("/") + file);
                continue;
            }
            QuaZip uz(LibPath % QStringLiteral("/") % archive);
            if(!uz.open(QuaZip::mdUnzip))
            {
                qDebug()<<("Error open archive!")<<" "<<archive;
                continue;
            }
            if(uz.getEntriesCount() > 1)
            {
                uz.close();
                QString out_dir = dir.absolutePath() % QStringLiteral("/") % archive.left( archive.length() - 4 );
                QDir().mkpath(out_dir);
                QBuffer buff;
                libs[idCurrentLib].getBookFile(idBook, &buff);

                QFile::remove(out_dir % QStringLiteral("/") % file);
                QFile outfile;
                outfile.setFileName(out_dir % QStringLiteral("/") % file);
                outfile.open(QFile::WriteOnly);
                outfile.write(buff.data());
                outfile.close();
            }
            else
            {
                QDir().mkpath(QFileInfo(dir.absolutePath() % QStringLiteral("/") % archive).absolutePath());
                QFile().copy(LibPath + QStringLiteral("/") % archive, dir.absolutePath() % QStringLiteral("/") % archive);
            }
            successful_export_books << idBook;
        }
        return;
    }

    QList<QList<uint> > books_group;
    bool bNeedGroupSeries =
            (pExportOptions_->sOutputFormat == u"EPUB" || pExportOptions_->sOutputFormat == u"MOBI" ||
             pExportOptions_->sOutputFormat == u"AZW3" || pExportOptions_->sOutputFormat == u"MOBI7") &&
            pExportOptions_->bJoinSeries;

    for(auto idBook: book_list)
    {
        SBook &book = libs[idCurrentLib].books[idBook];
        if(bNeedGroupSeries && book.idSerial != 0){
            auto iBookGroup = books_group.begin();
            while(iBookGroup != books_group.end()){
                if(libs[idCurrentLib].books[iBookGroup->first()].idSerial == book.idSerial)
                    break;
                ++iBookGroup;
            }
            if(iBookGroup != books_group.end())
                *iBookGroup << idBook;
            else{
                books_group << QList<uint>();
                books_group.last() << idBook;
            }

        }else{
            books_group << QList<uint>();
            books_group.last() << idBook;
        }
    }

    auto nMaxFileName = pathconf(sExportDir_.toUtf8().data(), _PC_NAME_MAX);
    uint count = 0;
    for(const auto &listBooks: books_group)
    {
        if(stopped_.load(std::memory_order_relaxed))
            break;
        QList<QBuffer*> buffers;
        QString sFileName;;
        for(auto idBook: listBooks)
        {
            QApplication::processEvents();
            count++;
            if(stopped_.load(std::memory_order_relaxed))
                break;
            buffers << new QBuffer(this);
            SBook &book = libs[idCurrentLib].books[idBook];
            fi = libs[idCurrentLib].getBookFile(idBook, buffers.last());
            if(fi.fileName().isEmpty())
            {
                emit Progress(count * 100 / book_list.count(), count);
                continue;
            }
            if(pExportOptions_->bOriginalFileName)
            {
                QString arh = book.sArchive;
                arh = arh.left(arh.length()-4);
                sFileName = arh.isEmpty() ?QStringLiteral("") :QStringLiteral("%1/%2.%3").arg(arh, book.sFile, book.sFormat);
            }
            else
            {
                sFileName = pExportOptions_->sExportFileName;
                if(sFileName.isEmpty())
                    sFileName = ExportOptions::sDefaultEexpFileName;
                QStringList listPartName = libs[idCurrentLib].fillParams(sFileName, idBook).split(QStringLiteral("/"));
                //Проверка и уменьшение длины имени файла до nMaxFileName байт
                for(int i = 0; i<listPartName.size(); i++){
                    auto &sPartName = listPartName[i];
                    int nSizeExt = i==listPartName.size()-1 ?book.sFormat.toUtf8().size() + 1 :0;
                    while(sPartName.toUtf8().size() + nSizeExt > nMaxFileName){
                        sPartName.chop(1);
                    }
                    if(i == 0)
                        sFileName = sPartName.trimmed();
                    else
                        sFileName += QStringLiteral("/") + sPartName.trimmed();
                }

                sFileName += QStringLiteral(".") + book.sFormat;
                if(pExportOptions_->bTransliteration)
                    sFileName = Transliteration(sFileName);
            }

            sFileName = dir.path() % QStringLiteral("/") % sFileName;
        }

        if(convert(buffers, idCurrentLib, sFileName, count, listBooks[0]))
        {
            for(auto idBook: listBooks)
                successful_export_books << idBook;
        }
        emit Progress(count * 100 / book_list.count(), count);
        for(auto buf: buffers)
            delete buf;
    }
}

void ExportThread::process()
{

    if(book_list.count() > 0)
        export_books();
    if(idLib_ > 0)
        export_lib();

    emit End();
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

void ExportThread::export_lib()
{
    QMap<uint, QString> tagsName;
    {
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
        QSqlDatabase dbase = QSqlDatabase::cloneDatabase(QStringLiteral("libdb"), QStringLiteral("exportbdb"));
#else
        if(!QSqlDatabase::database(QStringLiteral("exportbdb"), false).isOpen())
        {
            openDB(QStringLiteral("exportbdb"));
        }
        QSqlDatabase dbase = QSqlDatabase::database(QStringLiteral("exportbdb"), false);
#endif

        if (!dbase.open())
        {
            qDebug() << dbase.lastError().text();
            return;
        }
        QSqlQuery query(dbase);
        query.exec(QStringLiteral("SELECT id,name FROM tag"));
        while(query.next())
        {
            uint id = query.value(0).toUInt();
            QString sName = query.value(1).toString().trimmed();
            tagsName[id] = sName;
        }
    }
    QSqlDatabase::removeDatabase(QStringLiteral("exportbdb"));

    QString tmp_dir = QStringLiteral("freeLib");
    if(QStandardPaths::standardLocations(QStandardPaths::TempLocation).count() > 0)
        tmp_dir = QStandardPaths::standardLocations(QStandardPaths::TempLocation).at(0) + QStringLiteral("/freeLib");
    QString impx_file = tmp_dir + QStringLiteral("/freeLib.inp");
    QString structure_file = tmp_dir + QStringLiteral("/structure.info");
    QString version_file = tmp_dir + QStringLiteral("/version.info");
    QString collection_file = tmp_dir + QStringLiteral("/collection.info");
    QDir().mkpath(tmp_dir);
    QFile inpx(impx_file);
    QFile structure(structure_file);
    structure.open(QFile::WriteOnly);
    //structure.write(QStringLiteral("AUTHOR;TITLE;SERIES;SERNO;GENRE;LIBID;INSNO;FILE;FOLDER;EXT;SIZE;LANG;DATE;DEL;KEYWORDS;CRC32;TAG;TAGAUTHOR;TAGSERIES;\r\n").toUtf8());
    structure.write(QStringLiteral("AUTHOR;TITLE;SERIES;SERNO;GENRE;LIBID;FILE;FOLDER;EXT;SIZE;LANG;STARS;DATE;DEL;KEYWORDS;TAG;TAGAUTHOR;TAGSERIES;\r\n").toUtf8());
    structure.close();
    QFile version(version_file);
    version.open(QFile::WriteOnly);
    version.write((QDate::currentDate().toString(QStringLiteral("dd.MM.yyyy")) + QStringLiteral("\r\n")).toUtf8());
    version.close();
    QFile collection(collection_file);
    collection.open(QFile::WriteOnly);
    collection.write((libs[idCurrentLib].name + QStringLiteral("\r\n")).toUtf8());
    collection.write((libs[idCurrentLib].name + QStringLiteral("\r\n")).toUtf8());
    collection.write(QStringLiteral("0\r\n").toUtf8());
    collection.write(QStringLiteral("freeLib\r\n").toUtf8());
    collection.close();
    inpx.open(QFile::WriteOnly);
    if(!inpx.isOpen())
    {
        qDebug()<<"Error create file: "+impx_file;
        return;
    }


    const SLib &lib = libs[idLib_];
    uint nCount = 0;
    auto iBook = lib.books.constBegin();
    while(iBook != lib.books.constEnd()){
        QString sBookTags;
        for(uint i=0; i<iBook->listIdTags.size(); i++)
            sBookTags += tagsName[iBook->listIdTags[i]] + ":";

        QString sAuthors;
        QString sAuthorTags;
        for(qsizetype i=0; i<iBook->listIdAuthors.size(); i++){
            const SAuthor &author = lib.authors[iBook->listIdAuthors[i]];
            sAuthors += author.sLastName % "," % author.sFirstName % ","  % author.sMiddleName % ":";
            //FIXME Разделить метки по авторам через ::
            for(uint j=0; j<author.listIdTags.size(); j++)
                sAuthorTags += tagsName[author.listIdTags[j]] + ":";
        }

        QString sSerialTags;
        if(iBook->idSerial != 0){
            const auto &listTags = lib.serials[iBook->idSerial].listIdTags;
            uint count = listTags.size();
            for(uint i=0; i<count; i++){
                sSerialTags += tagsName[listTags[i]] + ":";
            }
        }
        QString sLine = (QStringLiteral("%1\4%2\4%3\4%4\4%5\4%6\4%7\4%8").arg(
                             sAuthors,                                                   //AUTHOR
                             iBook->sName,                                               //TITLE
                             iBook->idSerial !=0 ?lib.serials[iBook->idSerial].sName :QStringLiteral(""),     //SERIES
                             QString::number(iBook->numInSerial),                                            //SERNO
                             iBook->listIdGenres.size()>0 ?genres[iBook->listIdGenres.constFirst()].listKeys.constFirst() + QStringLiteral(":") :QStringLiteral(""),//GENRE
                             QString::number(iBook->idInLib),                                      //LIBID
                             iBook->sFile,                                                         //FILE
                             iBook->sArchive                                                       //FOLDER
                         ) %
                         QStringLiteral("\4%1\4%2\4%3\4%4\4%5\4%6\4%7").arg
                        (
                            iBook->sFormat,                         //EXT
                            QString::number(iBook->nSize),          //SIZE
                            lib.vLaguages[iBook->idLanguage],       //LANG
                            QString::number(iBook->nStars),         //STARS
                            iBook->date.toString(QStringLiteral("yyyy-MM-dd")),  //DATE
                            iBook->bDeleted ?"1" :"",                            //DEL
                            iBook->sKeywords                                     //KEYWORDS
                        ) %
                        QStringLiteral("\4%1\4%2\4%3\r\n").arg
                        (
                             sBookTags,     //TAG
                             sAuthorTags,   //TAGAUTHOR
                             sSerialTags     //TAGSERIES
                        ));

        inpx.write(sLine.toUtf8());
        ++iBook;
        nCount++;
        if(uint(nCount / 1000) * 1000 == nCount)
            emit Progress( nCount * 100 / lib.books.count(), nCount );
    }
    inpx.close();
    QuaZip zip(sExportDir_ + QStringLiteral("/%1.inpx").arg(BuildFileName(lib.name)));
    zip.open(QuaZip::mdCreate);
    QuaZipFile zip_file(&zip);
    QFile file;

    zip_file.open(QIODevice::WriteOnly, QuaZipNewInfo(QStringLiteral("freeLib.inp")), nullptr, 0, Z_DEFLATED, 5);
    file.setFileName(impx_file);
    file.open(QIODevice::ReadOnly);
    zip_file.write(file.readAll());
    file.close();
    zip_file.close();

    zip_file.open(QIODevice::WriteOnly, QuaZipNewInfo(QStringLiteral("structure.info")), nullptr, 0, Z_DEFLATED, 5);
    file.setFileName(structure_file);
    file.open(QIODevice::ReadOnly);
    zip_file.write(file.readAll());
    file.close();
    zip_file.close();

    zip_file.open(QIODevice::WriteOnly, QuaZipNewInfo(QStringLiteral("version.info")), nullptr, 0, Z_DEFLATED, 5);
    file.setFileName(version_file);
    file.open(QIODevice::ReadOnly);
    zip_file.write(file.readAll());
    file.close();
    zip_file.close();

    zip_file.open(QIODevice::WriteOnly, QuaZipNewInfo(QStringLiteral("collection.info")), nullptr, 0, Z_DEFLATED, 5);
    file.setFileName(collection_file);
    file.open(QIODevice::ReadOnly);
    zip_file.write(file.readAll());
    file.close();
    zip_file.close();

    zip.close();
    emit Progress(100, lib.books.count());
}

void ExportThread::break_exp()
{
    stopped_.store(true, std::memory_order_relaxed);
}
