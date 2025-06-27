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
#include <QUrl>
#include <unistd.h>

#ifdef USE_KIO
#include <KIO/Job>
#include <KIO/FileCopyJob>
#include <KIO/StatJob>
#include <KIO/MkdirJob>
#endif

#include "SmtpClient/src/smtpclient.h"
#include "SmtpClient/src/mimeattachment.h"
#include "SmtpClient/src/mimetext.h"
#ifdef QUAZIP_STATIC
#include "quazip/quazip/quazipfile.h"
#else
#include <quazip/quazipfile.h>
#endif

#include "fb2mobi/fb2mobi.h"
#include "library.h"
#include "utilites.h"
#include "bookfile.h"

QString validateFileName(QString str)
{
    if(!g::options.bExtendedSymbols)
    {
        str = str.replace('\"', '\'');
        static const QRegularExpression re(u"[?!*<>|]"_s);
        str.replace(re, u""_s);
    }
#ifdef Q_OS_MAC
    static const QRegularExpression re(u"[:]"_s);
    str = str.replace(re, u"_"_s);
#endif
    return str;
}

void validateFileName(QUrl &url)
{
    if(!g::options.bExtendedSymbols)
    {
        QString sPath = url.path();
        sPath = sPath.replace('\"', '\'');
        static const QRegularExpression re(u"[?!*<>|:]"_s);
        sPath.replace(re, u""_s);
        url.setPath(sPath);
    }
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
    if(!g::libs[idLib].bLoaded)
        loadLibrary(idLib);
}

void ExportThread::start(const QString &_export_dir, const std::vector<uint> &vBooks, SendType send, qlonglong id_author)
{
    idLib_ = 0;
    stopped_.store(false, std::memory_order_relaxed);
    vBbooks_ = vBooks;
    send_type = send;
    IDauthor = id_author;
    sExportDir_ = RelativeToAbsolutePath(_export_dir);
}

QString BuildFileName(QString filename)
{
    return filename.replace('/', '.').replace('\\', '.').replace('*', '.').replace('|', '.').replace(':', '.').replace('?', '.')
            .replace('<', '.').replace('>', '.').replace('\"', '\'');
}

#ifdef USE_KIO
bool kioMkDir(const QUrl &dir)
{
    auto statJob = KIO::stat(dir, KIO::HideProgressInfo);
    statJob->start();
    statJob->exec();
    if (statJob->error()){
        QUrl upDir = dir;
        QString sPath = upDir.path();
        sPath = sPath.left(sPath.lastIndexOf(u'/', -2));
        if(sPath.isEmpty())
            return false;
        upDir.setPath(sPath);
        if( !kioMkDir(upDir) )
            return false;
        auto newDir = dir;
        sPath = newDir.path();
        if(sPath.at(sPath.length()-1) == u'/')
            sPath.chop(1);
        newDir.setPath(sPath);
        auto dirCreateJob = KIO::mkdir(newDir);
        dirCreateJob->start();
        dirCreateJob->exec();
        if(dirCreateJob->error()){
            LogWarning << dirCreateJob->errorString();
            return false;
        }
        return true;
    }else
        return true;
}
#endif

bool ExportThread::convert(const std::vector<QBuffer *> &vOutBuff, uint idLib, const QString &file_name, int count, uint idBook)
{
    SLib& lib = g::libs[idLib];
    Q_CHECK_PTR(pExportOptions_);
    QString tool_path,tool_arg;
    for(const auto &iTool :g::options.tools){
        if(iTool.first == pExportOptions_->sCurrentTool)
        {
            tool_path = iTool.second.sPath;
            tool_arg = iTool.second.sArgs;
            break;
        }
    }

    QDir book_dir;
    QFileInfo fi(file_name);

    QString tmp_dir;
    if(QStandardPaths::standardLocations(QStandardPaths::TempLocation).count() > 0)
        tmp_dir = QStandardPaths::standardLocations(QStandardPaths::TempLocation).at(0);
    book_dir.mkpath(tmp_dir + u"/freeLib"_s);

    std::vector<QString> out_file;
    int i = 0;
    QFile file;
    for(const QBuffer* buf: vOutBuff)
    {
        out_file.emplace_back(tmp_dir % u"/freeLib/book%1."_s.arg(QString::number(i)) % fi.suffix());
        i++;
        QFile::remove(out_file.back());
        file.setFileName(out_file.back());
        file.open(QFile::WriteOnly);
        file.write(buf->data());
        file.close();
    }
    if(out_file.size() == 0) [[unlikely]]
        return false;
    QString current_out_file = out_file.front();
    if(
        (pExportOptions_->format == mobi ||
         pExportOptions_->format == epub ||
         pExportOptions_->format == azw3 ||
         pExportOptions_->format == mobi7 ||
         pExportOptions_->format == pdf ) &&
            (fi.suffix().toLower() == u"fb2" || fi.suffix().toLower() == u"epub"))
    {
        fb2mobi conv(pExportOptions_, idLib);
        current_out_file = conv.convert(out_file, idBook);
    }

    QString sBookFileName = fi.path() % u"/"_s % fi.completeBaseName() % u"."_s % QFileInfo(current_out_file).suffix();

    if(send_type == ST_Mail)
    {
       if(count > 1)
       {
           QEventLoop loop; QTimer::singleShot(pExportOptions_->nEmailPause*1000, &loop, &QEventLoop::quit); loop.exec();
       }
       SmtpClient smtp(pExportOptions_->sEmailServer, pExportOptions_->nEmailServerPort, (SmtpClient::ConnectionType)pExportOptions_->nEmailConnectionType);

       MimeMessage msg(true);
       msg.setHeaderEncoding(MimePart::Base64);
       EmailAddress sender(pExportOptions_->sEmailFrom, u""_s);
       msg.setSender(sender);
       EmailAddress to(pExportOptions_->sEmail, u""_s);
       msg.addRecipient(to);
       QString caption = pExportOptions_->sEmailSubject;
       msg.setSubject(caption.isEmpty() ?u"freeLib"_s :caption);

       QBuffer outbuff;
       QString FileName = current_out_file;
       if(FileName.isEmpty())
       {
           return false;
       }
       QFile book_file(FileName);
       if(!book_file.open(QFile::ReadOnly)) [[unlikely]]
       {
           LogWarning  << "Error open file name:" << FileName;
           return false;
       }
       outbuff.setData(book_file.readAll());
       book_file.close();
       MimeText *pText = new MimeText;
       QString onlyFileName = QFileInfo(sBookFileName).fileName();
       pText->setText(onlyFileName);
       msg.addPart(pText);
       msg.addPart(new MimeAttachment(outbuff.data(), onlyFileName));
       smtp.connectToHost();
       smtp.waitForReadyConnected();
       smtp.login(pExportOptions_->sEmailUser, pExportOptions_->sEmailPassword);
       smtp.waitForAuthenticated();
       smtp.sendMail(msg);
       if(!smtp.waitForMailSent()) [[unlikely]]
       {
           LogWarning << "Error send e-mail.";
           return false;
       }

       smtp.quit();
       return true;
    }
    else
    {
#ifdef USE_KIO
        QUrl urlDst = QUrl(sBookFileName, QUrl::TolerantMode);
#else
        sBookFileName = validateFileName(sBookFileName);
#endif
       if(!pExportOptions_->bPostprocessingCopy)
        {
#ifdef USE_KIO
           QUrl urlSrc = current_out_file;
           if(urlSrc.scheme().isEmpty())
               urlSrc.setScheme(u"file"_s);
           if(urlDst.scheme().isEmpty())
               urlDst.setScheme(u"file"_s);
           urlDst.setPath(sBookFileName);
           urlDst.setQuery(u""_s);
           validateFileName(urlDst);
           if(!kioMkDir(urlDst.adjusted(QUrl::RemoveFilename))) [[unlikely]]{
               LogWarning << "Could not make dir:" << urlDst;
               return false;
           }
           KIO::FileCopyJob *jobCopy = KIO::file_move(urlSrc, urlDst, -1, KIO::Overwrite | KIO::HideProgressInfo );
           jobCopy->start();
           jobCopy->exec();
           if (jobCopy->error()) [[unlikely]]{
               LogWarning << jobCopy->errorString();
               return false;
           }

           auto statJob = KIO::stat(urlDst, KIO::HideProgressInfo);
           statJob->start();
           statJob->exec();
           if(tool_path.isEmpty())
               return !statJob->error();

#else //USE_KIO
           QDir dir(sBookFileName);
           dir.mkpath(book_dir.cleanPath(QFileInfo(sBookFileName).absolutePath()));
           QFile::remove(sBookFileName);
           QFile::rename(current_out_file, sBookFileName);
           if(tool_path.isEmpty())
               return QFileInfo::exists(sBookFileName);
#endif //USE_KIO
        }
        else
            sBookFileName = current_out_file;
        if(!tool_path.isEmpty())
        {
#ifdef USE_KIO
            QFileInfo fiTmp(urlDst.path());
#else
            QFileInfo fiTmp(sBookFileName);
#endif
            QString ex = lib.fillParams(tool_path, idBook, fiTmp);
            QStringList listArg = tool_arg.split(u" "_s);
            for(int i = 0; i != listArg.size(); ++i)
                listArg[i] = lib.fillParams(listArg[i], idBook, fiTmp);
            return (QProcess::execute(ex, listArg) == 0);
        }
    }
    return false;
}

void ExportThread::export_books()
{
    QDir dir = sExportDir_;
    if(!dir.exists())
        dir.mkdir(sExportDir_);

    const auto &lib = g::libs[g::idCurrentLib];
    if(pExportOptions_->bOriginalFileName && send_type != ST_Mail && pExportOptions_->format == asis)
    {
        QString LibPath = lib.path;
        for(auto idBook: vBbooks_)
        {
            QString archive, file;
            const SBook &book = lib.books.at(idBook);
            if(!book.sArchive.isEmpty())
            {
                archive = book.sArchive;
                archive.replace(QStringLiteral(".inp"), QStringLiteral(".zip"));
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
            if(!uz.open(QuaZip::mdUnzip)) [[unlikely]]
            {
                LogWarning << "Error open archive:" << archive;
                continue;
            }
            if(uz.getEntriesCount() > 1)
            {
                uz.close();
                QString out_dir = dir.absolutePath() % u"/"_s % archive.left( archive.length() - 4 );
                QDir().mkpath(out_dir);
                BookFile fileBook(g::idCurrentLib, idBook);

                QFile::remove(out_dir % u"/"_s % file);
                QFile outfile;
                outfile.setFileName(out_dir % u"/"_s % file);
                outfile.open(QFile::WriteOnly);
                outfile.write(fileBook.data());
                outfile.close();
            }
            else
            {
                QDir().mkpath(QFileInfo(dir.absolutePath() % QStringLiteral("/") % archive).absolutePath());
                QFile().copy(LibPath + QStringLiteral("/") % archive, dir.absolutePath() % QStringLiteral("/") % archive);
            }
            vSuccessfulExportBooks.push_back(idBook);
        }
        return;
    }

    std::vector<std::vector<uint> > vBbooksGroup;
    bool bNeedGroupSeries =
        (pExportOptions_->format == epub || pExportOptions_->format == mobi ||
         pExportOptions_->format == azw3 || pExportOptions_->format == mobi7) &&
            pExportOptions_->bJoinSeries;

    for(auto idBook: vBbooks_)
    {
        const SBook &book = lib.books.at(idBook);
        if(bNeedGroupSeries && !book.mSequences.empty()){
            auto iBookGroup = vBbooksGroup.begin();
            while(iBookGroup != vBbooksGroup.end()){
                auto &groupeSequence = lib.books.at(iBookGroup->front()).mSequences;
                if(!groupeSequence.empty() && groupeSequence.begin()->first == book.mSequences.begin()->first){
                    break;
                }
                ++iBookGroup;
            }
            if(iBookGroup != vBbooksGroup.end())
                iBookGroup->push_back(idBook);
            else{
                vBbooksGroup.emplace_back(std::vector<uint>());
                vBbooksGroup.back().push_back(idBook);
            }

        }else{
            vBbooksGroup.emplace_back(std::vector<uint>());
            vBbooksGroup.back().push_back(idBook);
        }
    }

#ifndef Q_OS_WIN
    auto nMaxFileName = pathconf(sExportDir_.toUtf8().data(), _PC_NAME_MAX);
#else
    auto nMaxFileName = PATH_MAX;
#endif
    if(nMaxFileName<0)
        nMaxFileName = 255;
    uint count = 0;
    for(const auto &vBooks: vBbooksGroup)
    {
        if(stopped_.load(std::memory_order_relaxed))
            break;
        std::vector<QBuffer*> vBuffers;
        QString sFileName;;
        for(auto idBook: vBooks)
        {
            QApplication::processEvents();
            count++;
            if(stopped_.load(std::memory_order_relaxed))
                break;
            vBuffers.push_back(new QBuffer(this));
            const SBook &book = lib.books.at(idBook);
            BookFile fileBook(g::idCurrentLib, idBook);
            QByteArray baBook = fileBook.data();
            vBuffers.back()->setData(baBook);
            if(baBook.size() == 0) [[unlikely]]
            {
                emit Progress(count * 100 / vBbooks_.size(), count);
                continue;
            }
            QString sFormat;
            if(book.sFormat.endsWith(u".zip") && book.sFormat.size()>4)
                sFormat = book.sFormat.left(book.sFormat.size()-4);
            else
                sFormat = book.sFormat;
            if(pExportOptions_->bOriginalFileName)
            {
                QString arh = book.sArchive;
                arh = arh.left(arh.length()-4);
                sFileName = arh.isEmpty() ?u""_s :u"%1/%2.%3"_s.arg(arh, book.sFile, sFormat);
            }
            else
            {
                sFileName = pExportOptions_->sExportFileName;
                if(sFileName.isEmpty())
                    sFileName = ExportOptions::sDefaultEexpFileName;
                QStringList listPartName = g::libs[g::idCurrentLib].fillParams(sFileName, idBook).split(u"/"_s);
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
                        sFileName += u"/"_s + sPartName.trimmed();
                }

                sFileName += u"."_s + sFormat;
                if(pExportOptions_->bTransliteration)
                    sFileName = Transliteration(sFileName);
            }

            sFileName = dir.path() % u"/"_s % sFileName;
        }

        if(convert(vBuffers, g::idCurrentLib, sFileName, count, vBooks[0]))
        {
            for(auto idBook: vBooks)
                vSuccessfulExportBooks.push_back(idBook);
        }
        emit Progress(count * 100 / vBbooks_.size(), count);
        for(auto buf: vBuffers)
            delete buf;
    }
}

void ExportThread::process()
{

    if(!vBbooks_.empty())
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
    std::unordered_map<uint, QString> tagsName;
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

        if (!dbase.open()) [[unlikely]]
        {
            LogWarning << dbase.lastError().text();
            return;
        }
        QSqlQuery query(dbase);
        query.exec(u"SELECT id,name FROM tag"_s);
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
    QString sInpxFile = tmp_dir + u"/freeLib.inp"_s;
    QString structure_file = tmp_dir + QStringLiteral("/structure.info");
    QString version_file = tmp_dir + QStringLiteral("/version.info");
    QString collection_file = tmp_dir + QStringLiteral("/collection.info");
    QDir().mkpath(tmp_dir);
    QFile inpx(sInpxFile);
    QFile structure(structure_file);
    structure.open(QFile::WriteOnly);
    structure.write(u"AUTHOR;TITLE;SERIES;SERNO;GENRE;LIBID;FILE;FOLDER;EXT;SIZE;LANG;STARS;DATE;DEL;KEYWORDS;TAG;TAGAUTHOR;TAGSERIES;\r\n"_s.toUtf8());
    structure.close();
    QFile version(version_file);
    version.open(QFile::WriteOnly);
    version.write((QDate::currentDate().toString(QStringLiteral("dd.MM.yyyy")) + QStringLiteral("\r\n")).toUtf8());
    version.close();
    QFile collection(collection_file);
    collection.open(QFile::WriteOnly);
    const auto &lib = g::libs[idLib_];
    collection.write((lib.name + u"\r\n"_s).toUtf8());
    collection.write((lib.name + u"\r\n"_s).toUtf8());
    collection.write(u"0\r\n"_s.toUtf8());
    collection.write(u"freeLib\r\n"_s.toUtf8());
    collection.close();
    inpx.open(QFile::WriteOnly);
    if(!inpx.isOpen()) [[unlikely]]
    {
        LogWarning << u"Error create file:"_s + sInpxFile;
        return;
    }

    uint nCount = 0;
    for(const auto &iBook :lib.books){
        const auto &book = iBook.second;
        QString sBookTags;
        for(auto idTag: book.idTags)
            sBookTags += tagsName[idTag] + ':';

        QString sAuthors;
        for(auto idAuthor : book.vIdAuthors){
            const SAuthor &author = lib.authors.at(idAuthor);
            sAuthors += author.sLastName % "," % author.sFirstName % ","  % author.sMiddleName % ":";
        }

        QString sAuthorTags;
        auto &firstAuthor = lib.authors.at(book.vIdAuthors.front());
        for(auto idTag :firstAuthor.idTags)
            sAuthorTags += tagsName[idTag] + u':';

        QString sSerialTags;
        if(!book.mSequences.empty()){
            const auto &idTags = lib.serials.at(book.mSequences.begin()->first).idTags;
            for(auto idTag :idTags){
                sSerialTags += tagsName[idTag] + ':';
            }
        }

        QString sGenres;
        for(auto idGenre :book.vIdGenres){
            sGenres += g::genres.at(idGenre).listKeys.constFirst() + ':';
        }
        //TODO Добавить мультисерийность при экспорте.
        bool bSecuence = !book.mSequences.empty();
        QString sNumInSeria = bSecuence ?QString::number(book.mSequences.begin()->second) :u""_s;
        QString sSequence = bSecuence ?lib.serials.at(book.mSequences.begin()->first).sName :u""_s;
        QString sLine = (u"%1\4%2\4%3\4%4\4%5\4%6\4%7\4%8"_s.arg(
                             sAuthors,      //AUTHOR
                             book.sName,    //TITLE
                             sSequence,     //SERIES
                             sNumInSeria,   //SERNO
                             sGenres,       //GENRE
                             QString::number(book.idInLib),  //LIBID
                             book.sFile,    //FILE
                             book.sArchive  //FOLDER
                         ) %
                         u"\4%1\4%2\4%3\4%4\4%5\4%6\4%7"_s.arg
                        (
                            book.sFormat,                         //EXT
                            QString::number(book.nSize),          //SIZE
                            lib.vLaguages[book.idLanguage],       //LANG
                            QString::number(book.nStars),         //STARS
                            book.date.toString(u"yyyy-MM-dd"_s),  //DATE
                            book.bDeleted ?"1" :"",               //DEL
                            book.sKeywords                        //KEYWORDS
                        ) %
                        u"\4%1\4%2\4%3\r\n"_s.arg
                        (
                             sBookTags,     //TAG
                             sAuthorTags,   //TAGAUTHOR
                             sSerialTags    //TAGSERIES
                        ));

        inpx.write(sLine.toUtf8());
        nCount++;
        if(uint(nCount / 1000) * 1000 == nCount) [[unlikely]]
            emit Progress( nCount * 100 / lib.books.size(), nCount );
    }
    inpx.close();
    QuaZip zip(sExportDir_ % u"/"_s % BuildFileName(lib.name) % u".inpx"_s);
    zip.open(QuaZip::mdCreate);
    QuaZipFile zip_file(&zip);
    QFile file;

    zip_file.open(QIODevice::WriteOnly, QuaZipNewInfo(u"freeLib.inp"_s), nullptr, 0, Z_BZIP2ED, Z_BEST_COMPRESSION);
    file.setFileName(sInpxFile);
    file.open(QIODevice::ReadOnly);
    zip_file.write(file.readAll());
    file.close();
    zip_file.close();

    zip_file.open(QIODevice::WriteOnly, QuaZipNewInfo(u"structure.info"_s), nullptr, 0, Z_DEFLATED, 5);
    file.setFileName(structure_file);
    file.open(QIODevice::ReadOnly);
    zip_file.write(file.readAll());
    file.close();
    zip_file.close();

    zip_file.open(QIODevice::WriteOnly, QuaZipNewInfo(u"version.info"_s), nullptr, 0, Z_DEFLATED, 5);
    file.setFileName(version_file);
    file.open(QIODevice::ReadOnly);
    zip_file.write(file.readAll());
    file.close();
    zip_file.close();

    zip_file.open(QIODevice::WriteOnly, QuaZipNewInfo(u"collection.info"_s), nullptr, 0, Z_DEFLATED, 5);
    file.setFileName(collection_file);
    file.open(QIODevice::ReadOnly);
    zip_file.write(file.readAll());
    file.close();
    zip_file.close();

    zip.close();
    emit Progress(100, lib.books.size());
}

void ExportThread::break_exp()
{
    stopped_.store(true, std::memory_order_relaxed);
}
