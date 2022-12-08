#define QT_USE_QSTRINGBUILDER
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
#include "quazip/quazip/quazip.h"
#include "quazip/quazip/quazipfile.h"

#include "fb2mobi/fb2mobi.h"
#include "library.h"
#include "utilites.h"

QString Transliteration(QString str)
{
    str=str.trimmed();
    QString fn;
    int i, rU, rL;
    QString validChars = QStringLiteral("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890-_,.()[]{}<>!@#$%^&+=\\/");
    QString rusUpper = QStringLiteral("АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЫЭЮЯ");
    QString rusLower = QStringLiteral("абвгдеёжзийклмнопрстуфхцчшщыэюя");
    QStringList latUpper, latLower;
    latUpper <<QStringLiteral("A")<<QStringLiteral("B")<<QStringLiteral("V")<<QStringLiteral("G")<<QStringLiteral("D")<<QStringLiteral("E")<<QStringLiteral("Jo")<<QStringLiteral("Zh")<<
               QStringLiteral("Z")<<QStringLiteral("I")<<QStringLiteral("J")<<QStringLiteral("K")<<QStringLiteral("L")<<QStringLiteral("M")<<QStringLiteral("N")<<QStringLiteral("O")<<
               QStringLiteral("P")<<QStringLiteral("R")<<QStringLiteral("S")<<QStringLiteral("T")<<QStringLiteral("U")<<QStringLiteral("F")<<QStringLiteral("H")<<QStringLiteral("C")<<
               QStringLiteral("Ch")<<QStringLiteral("Sh")<<QStringLiteral("Sh")<<QStringLiteral("I")<<QStringLiteral("E")<<QStringLiteral("Ju")<<QStringLiteral("Ja");
    latLower <<QStringLiteral("a")<<QStringLiteral("b")<<QStringLiteral("v")<<QStringLiteral("g")<<QStringLiteral("d")<<QStringLiteral("e")<<QStringLiteral("jo")<<QStringLiteral("zh")<<
               QStringLiteral("z")<<QStringLiteral("i")<<QStringLiteral("j")<<QStringLiteral("k")<<QStringLiteral("l")<<QStringLiteral("m")<<QStringLiteral("n")<<QStringLiteral("o")<<
               QStringLiteral("p")<<QStringLiteral("r")<<QStringLiteral("s")<<QStringLiteral("t")<<QStringLiteral("u")<<QStringLiteral("f")<<QStringLiteral("h")<<QStringLiteral("c")<<
               QStringLiteral("ch")<<QStringLiteral("sh")<<QStringLiteral("sh")<<QStringLiteral("i")<<QStringLiteral("e")<<QStringLiteral("ju")<<QStringLiteral("ja");
    for (i=0; i < str.size(); ++i){
        if ( validChars.contains(str[i]) ){
            fn = fn + str[i];
        }else if (str[i] == ' '){  //replace spaces
            fn = fn + QLatin1String(" ");
        }else if (str[i] == '?'){  //replace ?
            fn = fn + QLatin1String(".");
        }else if (str[i] == '*'){  //replace *
            fn = fn + QLatin1String(".");
        }else if (str[i] == '~'){  //replace ~
            fn = fn + QLatin1String(".");
        }else{
            rU = rusUpper.indexOf(str[i]);
            rL = rusLower.indexOf(str[i]);
            if (rU >= 0)
                fn = fn + latUpper[rU];
            else if (rL >= 0)
                fn = fn + latLower[rL];
        }
    }
    if (fn.isEmpty() )
        fn = QStringLiteral("file");
    return fn;
}

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
        str=str.left(bMtp ?4 :2) + str.mid(bMtp ?4 :2).replace(':', '_');
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
    ID_lib = -1;
    pExportOptions_ = pExportOptions;
}

void ExportThread::start(qlonglong id_lib, const QString &path)
{
    loop_enable = true;
    ID_lib = id_lib;
    export_dir = path;
}

void ExportThread::start(const QString &_export_dir, const QList<uint> &list_books, SendType send, qlonglong id_author)
{
    ID_lib = -1;
    loop_enable = true;
    book_list = list_books;
    send_type = send;
    IDauthor = id_author;
    export_dir = RelativeToAbsolutePath(_export_dir);
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
    int index = 0;
    while(iTool != options.tools.constEnd()){
        if(iTool.key() == pExportOptions_->sCurrentTool)
        {
            tool_path = iTool->sPath;
            tool_arg = iTool->sArgs;
            tool_ext = iTool->sExt;
            break;
        }
        ++index;
        ++iTool;
    }

    QDir book_dir;
    QFileInfo fi(file_name);

    QString tmp_dir;
    if(QStandardPaths::standardLocations(QStandardPaths::TempLocation).count() > 0)
        tmp_dir = QStandardPaths::standardLocations(QStandardPaths::TempLocation).at(0);
    book_dir.mkpath(tmp_dir + QLatin1String("/freeLib"));

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

    QString book_file_name = fi.path() + QLatin1String("/") + fi.completeBaseName() + QLatin1String(".") + QFileInfo(current_out_file).suffix();

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
       msg.setSubject(caption.isEmpty() ?AppName :caption);

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
    QDir dir = export_dir;
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
            file = book.sName + QLatin1String(".") + book.sFormat;
            LibPath = RelativeToAbsolutePath(LibPath);
            archive = archive.replace('\\', '/');
            file = file.replace('\\', '/');
            if(archive.isEmpty())
            {
                QDir().mkpath(QFileInfo(dir.absolutePath() + QLatin1String("/") + file).absolutePath());
                QFile().copy(LibPath + QLatin1String("/") + file, dir.absolutePath() + QLatin1String("/") + file);
                continue;
            }
            QuaZip uz(LibPath + QLatin1String("/") + archive);
            if(!uz.open(QuaZip::mdUnzip))
            {
                qDebug()<<("Error open archive!")<<" "<<archive;
                continue;
            }
            if(uz.getEntriesCount() > 1)
            {
                uz.close();
                QString out_dir = dir.absolutePath() + QLatin1String("/") + archive.left( archive.length() - 4 );
                QDir().mkpath(out_dir);
                QBuffer buff;
                mLibs[idCurrentLib].getBookFile(idBook, &buff);

                QFile::remove(out_dir + QLatin1String("/") + file);
                QFile outfile;
                outfile.setFileName(out_dir + QLatin1String("/") + file);
                outfile.open(QFile::WriteOnly);
                outfile.write(buff.data());
                outfile.close();
            }
            else
            {
                QDir().mkpath(QFileInfo(dir.absolutePath() + QLatin1String("/") + archive).absolutePath());
                QFile().copy(LibPath + QLatin1String("/") + archive, dir.absolutePath() + QLatin1String("/") + archive);
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
                file_name = mLibs[idCurrentLib].fillParams(file_name, idBook) + QLatin1String(".") + book.sFormat;
                if(pExportOptions_->bTransliteration)
                    file_name = Transliteration(file_name);
            }
            file_name = dir.path() + QLatin1String("/") + file_name;
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
    if(ID_lib > 0)
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
    if(!openDB(QStringLiteral("ExpThrdDb")))
        return;
    QSqlQuery query(QSqlDatabase::database(QStringLiteral("ExpThrdDb")));
    if(!query.exec(QStringLiteral("SELECT book.id,author.id,genre.id,book.name,star,num_in_seria,book.language,file,size,deleted,date,format,book.keys,archive,date,book.id_inlib, "
                                  //      0       1         2        3         4    5            6             7    8    9       10   11     12        13      14   15
               "seria.name,genre.keys,author.name1||','||author.name2||','||author.name3||':' "
//              16          17         18
               "FROM book_author "
               "JOIN book on book.id=book_author.id_book "
               "LEFT JOIN book_genre ON book_genre.id_book=book.id "
               "LEFT JOIN genre ON genre.id=book_genre.id_genre "
               "LEFT JOIN seria ON seria.id=book.id_seria "
               "JOIN author ON author.id=book_author.id_author "
               "WHERE book_author.id_lib=%1 "
               "ORDER BY book.id,seria.id,num_in_seria;").arg(ID_lib)))
    {
        qDebug() << query.lastError();
        QSqlDatabase::database(QStringLiteral("ExpThrdDb")).close();
        return;
    }
    int count = 0;
    qlonglong lastbookid= - 1;

    QString tmp_dir = QStringLiteral("freeLib");
    if(QStandardPaths::standardLocations(QStandardPaths::TempLocation).count() > 0)
        tmp_dir = QStandardPaths::standardLocations(QStandardPaths::TempLocation).at(0) + QLatin1String("/freeLib");
    QString impx_file = tmp_dir + QLatin1String("/freeLib.inp");
    QString structure_file = tmp_dir + QLatin1String("/structure.info");
    QString version_file = tmp_dir + QLatin1String("/version.info");
    QString collection_file = tmp_dir + QLatin1String("/collection.info");
    QDir().mkpath(tmp_dir);
    QFile inpx(impx_file);
    QFile structure(structure_file);
    structure.open(QFile::WriteOnly);
    structure.write(QStringLiteral("AUTHOR;TITLE;SERIES;SERNO;GENRE;LIBID;INSNO;FILE;FOLDER;EXT;SIZE;LANG;DATE;DEL;KEYWORDS;CRC32;TAG;TAGAUTHOR;TAGSERIES;\r\n").toUtf8());
    structure.close();
    QFile version(version_file);
    version.open(QFile::WriteOnly);
    version.write((QDate::currentDate().toString(QStringLiteral("dd.MM.yyyy")) + QLatin1String("\r\n")).toUtf8());
    version.close();
    QFile collection(collection_file);
    collection.open(QFile::WriteOnly);
    collection.write((mLibs[idCurrentLib].name + QLatin1String("\r\n")).toUtf8());
    collection.write((mLibs[idCurrentLib].name + QLatin1String("\r\n")).toUtf8());
    collection.write(QStringLiteral("0\r\n").toUtf8());
    collection.write(QStringLiteral("freeLib\r\n").toUtf8());
    collection.close();
    inpx.open(QFile::WriteOnly);
    if(!inpx.isOpen())
    {
        qDebug()<<"Error create file: "+impx_file;
        emit Progress(0, count);
        return;
    }
    QString authors;
    QString fav_authors;
    QString janres;
    bool next_book = false;
    QString buf;
    while(loop_enable)
    {
        loop_enable = query.next();
        if(!loop_enable)
        {
            if(lastbookid < 0)
                break;
            next_book = true;
        }
        else
            next_book = (lastbookid != query.value(0).toLongLong() && lastbookid >= 0);
        if(next_book)
        {
            count++;
            if(int(count / 1000) * 1000 == count)
                emit Progress(0, count);
            inpx.write(buf.toUtf8());
            authors = QLatin1String("");
            janres = QLatin1String("");
            fav_authors = QLatin1String("");
        }
        if(loop_enable)
        {
            lastbookid = query.value(0).toLongLong();
            authors += query.value(18).toString().trimmed();
            QString sGenre = query.value(17).toString().trimmed();
            int nSemicolonPos = sGenre.indexOf(';');
            sGenre.truncate(nSemicolonPos);
            janres += sGenre + QLatin1String(":");
            //fav_authors += (query.value(20).toString().trimmed() + QLatin1String(":"));
            buf = (QStringLiteral("%1\4%2\4%3\4%4\4%5\4%6\4%7\4%8\4%9").arg
                        (
                            authors,                                //AUTHOR
                            query.value(3).toString().trimmed(),    //TITLE
                            query.value(16).toString().trimmed(),   //SERIES
                            query.value(5).toString().trimmed(),    //SERNO
                            janres,                                 //GENRE
                            query.value(15).toString().trimmed(),   //LIBID
                            "",                                     //INSNO
                            query.value(7).toString().trimmed(),    //FILE
                            query.value(13).toString().trimmed()    //FOLDER
                        )+
                           QStringLiteral("\4%1\4%2\4%3\4%4\4%5\4%6\4%7").arg
                        (
                            query.value(11).toString().trimmed(),   //EXT
                            query.value(8).toString().trimmed(),    //SIZE
                            query.value(6).toString().trimmed(),    //LANG
                            query.value(10).toDate().toString(QStringLiteral("yyyy-MM-dd")),    //DATE
                            query.value(9).toBool()?"1":"",         //DEL
                            query.value(12).toString().trimmed(),   //KEYWORDS
                            ""                                      //CRC32
                        )/*+
                        QStringLiteral("\4%1\4%2\4%3\r\n").arg
                        (
                             query.value(16).toString().trimmed(),  //FAV_BOOK
                             fav_authors,                           //FAV_AUTHOR
                             query.value(18).toString().trimmed()   //FAV_SERIES
                        )*/
                        );
        }
    }
    QSqlDatabase::database(QStringLiteral("ExpThrdDb")).close();
    inpx.close();
    QuaZip zip(export_dir + QStringLiteral("/%1.inpx").arg(BuildFileName(mLibs[idCurrentLib].name)));
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
    QSqlDatabase::database(QStringLiteral("ExpThrdDb")).close();
    emit Progress(0, count);
}

void ExportThread::break_exp()
{
    loop_enable = false;
}

//void ExportThread::smtpError(SmtpError e)
//{
//    qDebug()<<e;
//}
