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


#include "SmtpClient/src/smtpclient.h"
#include "SmtpClient/src/mimeattachment.h"
#include "SmtpClient/src/mimetext.h"
#include "quazip/quazip/quazipfile.h"

#include "fb2mobi/fb2mobi.h"
#include "library.h"
#include "utilites.h"

QString ValidateFileName(QString str)
{
    bool windows=false;
    bool mac=false;
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
        bool bMtp = str.startsWith(QLatin1String("mtp:"));
        str = str.left(bMtp ?4 :2) + str.mid(bMtp ?4 :2).replace(':', '_');
    }
    else
    {
        if(mac){
            static const QRegularExpression re(QStringLiteral("[:]"));
            str = str.replace(re, QStringLiteral("_"));
        }
    }
    static const QRegularExpression re2(QStringLiteral("[/\\]"));
    str = str.replace(re2, QStringLiteral("_"));
    qDebug()<<str;
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
    loop_enable = true;
    idLib_ = idLib;
    sExportDir_ = path;
    if(!mLibs[idLib].bLoaded)
        loadLibrary(idLib);
}

void ExportThread::start(const QString &_export_dir, const QList<uint> &list_books, SendType send, qlonglong id_author)
{
    idLib_ = 0;
    loop_enable = true;
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
    SLib& lib = mLibs[idLib];
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
    foreach (QBuffer* buf, outbuff)
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
            (pExportOptions_->sOutputFormat == QLatin1String("MOBI") ||
             pExportOptions_->sOutputFormat == QLatin1String("EPUB") ||
             pExportOptions_->sOutputFormat == QLatin1String("AZW3") ||
             pExportOptions_->sOutputFormat == QLatin1String("MOBI7") ||
             pExportOptions_->sOutputFormat == QLatin1String("PDF")) &&
            (fi.suffix().toLower() == QLatin1String("fb2") || fi.suffix().toLower() == QLatin1String("epub")))
    {
        fb2mobi conv(pExportOptions_, idLib);
        current_out_file = conv.convert(out_file, idBook);
    }

    QString book_file_name = fi.path() % QLatin1String("/") % fi.completeBaseName() % QLatin1String(".") % QFileInfo(current_out_file).suffix();

    if(send_type == ST_Mail)
    {
       if(count > 1)
       {
           QEventLoop loop; QTimer::singleShot(pExportOptions_->nEmailPause*1000, &loop, &QEventLoop::quit); loop.exec();
       }
       SmtpClient smtp(pExportOptions_->sEmailServer, pExportOptions_->nEmailServerPort, (SmtpClient::ConnectionType)pExportOptions_->nEmailConnectionType);

       MimeMessage msg(true);
       msg.setHeaderEncoding(MimePart::Base64);
       EmailAddress sender(pExportOptions_->sEmailFrom, QLatin1String(""));
       msg.setSender(sender);
       EmailAddress to(pExportOptions_->sEmail, QLatin1String(""));
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
        book_file_name=ValidateFileName(book_file_name);
        if(!pExportOptions_->bPostprocessingCopy)
        {
            //book_dir.mkpath(book_dir.cleanPath(QFileInfo(book_file_name).absolutePath()));
            if(book_file_name.startsWith(QLatin1String("mtp:/"))){
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

    if(pExportOptions_->bOriginalFileName && send_type != ST_Mail && pExportOptions_->sOutputFormat == QLatin1String("-"))
    {
        QString LibPath = mLibs[idCurrentLib].path;
        foreach(uint idBook, book_list)
        {
            QString archive, file;
            SBook &book = mLibs[idCurrentLib].mBooks[idBook];
            if(!book.sArchive.isEmpty())
            {
                archive = book.sArchive.replace(QLatin1String(".inp"), QLatin1String(".zip"));
            }
            file = book.sName % QLatin1String(".") % book.sFormat;
            LibPath = RelativeToAbsolutePath(LibPath);
            archive = archive.replace('\\', '/');
            file = file.replace('\\', '/');
            if(archive.isEmpty())
            {
                QDir().mkpath(QFileInfo(dir.absolutePath() % QLatin1String("/") % file).absolutePath());
                QFile().copy(LibPath % QLatin1String("/") % file, dir.absolutePath() % QLatin1String("/") + file);
                continue;
            }
            QuaZip uz(LibPath % QLatin1String("/") % archive);
            if(!uz.open(QuaZip::mdUnzip))
            {
                qDebug()<<("Error open archive!")<<" "<<archive;
                continue;
            }
            if(uz.getEntriesCount() > 1)
            {
                uz.close();
                QString out_dir = dir.absolutePath() % QLatin1String("/") % archive.left( archive.length() - 4 );
                QDir().mkpath(out_dir);
                QBuffer buff;
                mLibs[idCurrentLib].getBookFile(idBook, &buff);

                QFile::remove(out_dir % QLatin1String("/") % file);
                QFile outfile;
                outfile.setFileName(out_dir % QLatin1String("/") % file);
                outfile.open(QFile::WriteOnly);
                outfile.write(buff.data());
                outfile.close();
            }
            else
            {
                QDir().mkpath(QFileInfo(dir.absolutePath() % QLatin1String("/") % archive).absolutePath());
                QFile().copy(LibPath + QLatin1String("/") % archive, dir.absolutePath() % QLatin1String("/") % archive);
            }
            successful_export_books << idBook;
        }
        return;
    }

    QList<QList<uint> > books_group;
    bool bNeedGroupSeries =
            (pExportOptions_->sOutputFormat == QLatin1String("EPUB") || pExportOptions_->sOutputFormat == QLatin1String("MOBI") ||
             pExportOptions_->sOutputFormat == QLatin1String("AZW3") || pExportOptions_->sOutputFormat == QLatin1String("MOBI7")) &&
            pExportOptions_->bJoinSeries;

    foreach(uint idBook, book_list)
    {
        SBook &book = mLibs[idCurrentLib].mBooks[idBook];
        if(bNeedGroupSeries && book.idSerial != 0){
            auto iBookGroup = books_group.begin();
            while(iBookGroup != books_group.end()){
                if(mLibs[idCurrentLib].mBooks[iBookGroup->first()].idSerial == book.idSerial)
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

    uint count = 0;
    foreach(const QList<uint> &listBooks, books_group)
    {
        if(!loop_enable)
            break;
        QList<QBuffer*> buffers;
        QString file_name;
        foreach(uint idBook, listBooks)
        {
            QApplication::processEvents();
            count++;
            if(!loop_enable)
                break;
            buffers << new QBuffer(this);
            SBook &book = mLibs[idCurrentLib].mBooks[idBook];
            fi = mLibs[idCurrentLib].getBookFile(idBook, buffers.last());
            if(fi.fileName().isEmpty())
            {
                emit Progress(count * 100 / book_list.count(), count);
                continue;
            }
            if(pExportOptions_->bOriginalFileName)
            {
                QString arh = book.sArchive;
                arh = arh.left(arh.length()-4);
                file_name = arh.isEmpty() ?QLatin1String("") :QStringLiteral("%1/%2.%3").arg(arh, book.sFile, book.sFormat);
            }
            else
            {
                file_name = pExportOptions_->sExportFileName;
                if(file_name.isEmpty())
                    file_name = QLatin1String(ExportOptions::sDefaultEexpFileName);
                file_name = mLibs[idCurrentLib].fillParams(file_name, idBook) % QLatin1String(".") % book.sFormat;
                if(pExportOptions_->bTransliteration)
                    file_name = Transliteration(file_name);
            }
            file_name = dir.path() % QLatin1String("/") % file_name;
        }

        if(convert(buffers, idCurrentLib, file_name, count, listBooks[0]))
        {
            foreach(uint idBook, listBooks)
                successful_export_books << idBook;
        }
        emit Progress(count * 100 / book_list.count(), count);
        foreach (QBuffer* buf, buffers)
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
    structure.write(QStringLiteral("AUTHOR;TITLE;SERIES;SERNO;GENRE;LIBID;FILE;FOLDER;EXT;SIZE;LANG;STARS;DATE;DEL;KEYWORDS;\r\n").toUtf8());
    structure.close();
    QFile version(version_file);
    version.open(QFile::WriteOnly);
    version.write((QDate::currentDate().toString(QStringLiteral("dd.MM.yyyy")) + QStringLiteral("\r\n")).toUtf8());
    version.close();
    QFile collection(collection_file);
    collection.open(QFile::WriteOnly);
    collection.write((mLibs[idCurrentLib].name + QStringLiteral("\r\n")).toUtf8());
    collection.write((mLibs[idCurrentLib].name + QStringLiteral("\r\n")).toUtf8());
    collection.write(QStringLiteral("0\r\n").toUtf8());
    collection.write(QStringLiteral("freeLib\r\n").toUtf8());
    collection.close();
    inpx.open(QFile::WriteOnly);
    if(!inpx.isOpen())
    {
        qDebug()<<"Error create file: "+impx_file;
        return;
    }

    const SLib &lib = mLibs[idLib_];
    uint nCount = 0;
    auto iBook = lib.mBooks.constBegin();
    while(iBook != lib.mBooks.constEnd()){
        QString sAuthors;
        for(qsizetype i=0; i<iBook->listIdAuthors.size(); i++){
            const SAuthor &author = lib.mAuthors[iBook->listIdAuthors[i]];
            sAuthors += author.sLastName % "," % author.sFirstName % ","  % author.sMiddleName % ":";
        }
        QString sLine = (QStringLiteral("%1\4%2\4%3\4%4\4%5\4%6\4%7\4%8").arg(
                             sAuthors,                                                   //AUTHOR
                             iBook->sName,                                               //TITLE
                             iBook->idSerial !=0 ?lib.mSerials[iBook->idSerial].sName :QLatin1String(""),     //SERIES
                             QString::number(iBook->numInSerial),                                            //SERNO
                             iBook->listIdGenres.size()>0 ?mGenre[iBook->listIdGenres.constFirst()].listKeys.constFirst() + QStringLiteral(":") :QLatin1String(""),//GENRE
                             QString::number(iBook->idInLib),                                      //LIBID
                             iBook->sFile,                                                         //FILE
                             iBook->sArchive                                                       //FOLDER
                             ) +
                         QStringLiteral("\4%1\4%2\4%3\4%4\4%5\4%6\4%7\r\n").arg
                        (
                            iBook->sFormat,                         //EXT
                            QString::number(iBook->nSize),          //SIZE
                            lib.vLaguages[iBook->idLanguage],       //LANG
                            QString::number(iBook->nStars),         //STARS
                            iBook->date.toString(QStringLiteral("yyyy-MM-dd")),     //DATE
                            iBook->bDeleted ?"1" :"",                               //DEL
                            iBook->sKeywords                                        //KEYWORDS
                        )/*+
                        QStringLiteral("\4%1\4%2\4%3\r\n").arg
                        (
                             query.value(16).toString().trimmed(),  //FAV_BOOK
                             fav_authors,                           //FAV_AUTHOR
                             query.value(18).toString().trimmed()   //FAV_SERIES
                        )*/);

        inpx.write(sLine.toUtf8());
        ++iBook;
        nCount++;
        if(uint(nCount / 1000) * 1000 == nCount)
            emit Progress( nCount * 100 / lib.mBooks.count(), nCount );
    }
    inpx.close();
    QuaZip zip(sExportDir_ + QStringLiteral("/%1.inpx").arg(BuildFileName(lib.name)));
    qDebug()<<zip.getZipName();
    qDebug()<<zip.open(QuaZip::mdCreate);
    QuaZipFile zip_file(&zip);
    QFile file;

    zip_file.open(QIODevice::WriteOnly, QuaZipNewInfo(QStringLiteral("freeLib.inp")), 0, 0, 8, 5);
    file.setFileName(impx_file);
    file.open(QIODevice::ReadOnly);
    zip_file.write(file.readAll());
    file.close();
    zip_file.close();

    zip_file.open(QIODevice::WriteOnly, QuaZipNewInfo(QStringLiteral("structure.info")), 0, 0, 8, 5);
    file.setFileName(structure_file);
    file.open(QIODevice::ReadOnly);
    zip_file.write(file.readAll());
    file.close();
    zip_file.close();

    zip_file.open(QIODevice::WriteOnly, QuaZipNewInfo(QStringLiteral("version.info")), 0, 0, 8, 5);
    file.setFileName(version_file);
    file.open(QIODevice::ReadOnly);
    zip_file.write(file.readAll());
    file.close();
    zip_file.close();

    zip_file.open(QIODevice::WriteOnly, QuaZipNewInfo(QStringLiteral("collection.info")), 0, 0, 8, 5);
    file.setFileName(collection_file);
    file.open(QIODevice::ReadOnly);
    zip_file.write(file.readAll());
    file.close();
    zip_file.close();

    zip.close();
    emit Progress(100, lib.mBooks.count());
}

void ExportThread::break_exp()
{
    loop_enable = false;
}

//void ExportThread::smtpError(SmtpError e)
//{
//    qDebug()<<e;
//}
