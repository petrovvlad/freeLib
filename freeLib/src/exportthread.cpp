#include <QXmlQuery>
#include <QApplication>
#include <QTextStream>
#include <QTimer>

#include "exportthread.h"
#include "common.h"
#include "SmtpClient/smtpclient.h"
#include "SmtpClient/mimeattachment.h"
#include "SmtpClient/mimetext.h"
#include "QProcess"
#include "QFileInfo"
#include "fb2mobi/fb2mobi.h"
#include "quazip/quazip/quazip.h"
#include "quazip/quazip/quazipfile.h"

QString Transliteration(QString str)
{
    str=str.trimmed();
    QString fn;
    int i, rU, rL;
    QString validChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890-_,.()[]{}<>!@#$%^&+=\\/";
    QString rusUpper = QString::fromUtf8("АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЫЭЮЯ");
    QString rusLower = QString::fromUtf8("абвгдеёжзийклмнопрстуфхцчшщыэюя");
    QStringList latUpper, latLower;
    latUpper <<"A"<<"B"<<"V"<<"G"<<"D"<<"E"<<"Jo"<<"Zh"<<"Z"<<"I"<<"J"<<"K"<<"L"<<"M"<<"N"
        <<"O"<<"P"<<"R"<<"S"<<"T"<<"U"<<"F"<<"H"<<"C"<<"Ch"<<"Sh"<<"Sh"<<"I"<<"E"<<"Ju"<<"Ja";
    latLower <<"a"<<"b"<<"v"<<"g"<<"d"<<"e"<<"jo"<<"zh"<<"z"<<"i"<<"j"<<"k"<<"l"<<"m"<<"n"
        <<"o"<<"p"<<"r"<<"s"<<"t"<<"u"<<"f"<<"h"<<"c"<<"ch"<<"sh"<<"sh"<<"i"<<"e"<<"ju"<<"ja";
    for (i=0; i < str.size(); ++i){
        if ( validChars.contains(str[i]) ){
            fn = fn + str[i];
        }else if (str[i] == ' '){  //replace spaces
            fn = fn + " ";
        }else if (str[i] == '?'){  //replace ?
            fn = fn + ".";
        }else if (str[i] == '*'){  //replace *
            fn = fn + ".";
        }else if (str[i] == '~'){  //replace ~
            fn = fn + ".";
        }else{
            rU = rusUpper.indexOf(str[i]);
            rL = rusLower.indexOf(str[i]);
            if (rU >= 0)         fn = fn + latUpper[rU];
            else if (rL >= 0)   fn = fn + latLower[rL];
        }
    }
    if (fn.isEmpty() ) fn = "file";
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

    QSettings *settings=GetSettings();
    if(!settings->value("extended_symbols",false).toBool() || windows)
    {

        str=str.replace("\"","'");
        str=str.replace(QRegExp("^([a-zA-Z]\\:|\\\\\\\\[^\\/\\\\:*?\"<>|]+\\\\[^\\/\\\\:*?\"<>|]+)(\\\\[^\\/\\\\:*?\"<>|]+)+(\\.[^\\/\\\\:*?\"<>|]+)$"),"_");
        str=str.left(2)+str.mid(2).replace(":","_");
    }
    else
    {
        if(mac)
            str=str.replace(QRegExp("[:]"),"_");
    }
    str=str.replace(QRegExp("[/\\]"),"_");
    qDebug()<<str;
    return str;
}

ExportThread::ExportThread(QObject *parent) :
    QObject(parent)
{
    ID_lib=-1;
}

void ExportThread::start(qlonglong id_lib,QString path)
{
    loop_enable=true;
    ID_lib=id_lib;
    export_dir=path;
}

void ExportThread::start(QString _export_dir, const QList<book_info> &list_books, SendType send, qlonglong id_author)
{
    ID_lib=-1;
    loop_enable=true;
    book_list=list_books;
    send_type=send;
    IDauthor=id_author;
    export_dir=RelativeToAbsolutePath(_export_dir);
   // QThread::start();
}

void ExportThread::start(QString _export_dir, const QStringList &list_books,SendType send)
{
    ID_lib=-1;
    loop_enable=true;
    book_list_str=list_books;
    send_type=send;
    IDauthor=0;
    export_dir=RelativeToAbsolutePath(_export_dir);
  //  QThread::start();
}

QString BuildFileName(QString filename)
{
    return filename.replace("/",".").replace("\\",".").replace("*",".").replace("|",".").replace(":",".").replace("?",".").replace("<",".").replace(">",".").replace("\"","'");
}

void ExportThread::FB2export()
{
    QSettings *settings=GetSettings();
    QDir dir=export_dir;
    QString FileName;
    long count=0;
    foreach(QString i,book_list_str)
    {
        app->processEvents();
        count++;
        emit Progress(count*100/book_list_str.count(),count);
        if(!loop_enable)
            break;
        FileName=i;
        QFile file(FileName);
        if(!file.open(QIODevice::ReadOnly))
        {
            return;
        }
        QByteArray ba=file.readAll();
        file.close();
        QFileInfo fi(file);
        book_info bi;
        if(fi.suffix().toLower()=="fb2")
        {
            GetBookInfo(bi,ba,"fb2",true);
        }
        else if(fi.suffix().toLower()=="epub")
        {
            GetBookInfo(bi,ba,"epub",true);
        }
        else if(fi.suffix().toLower()=="zip")
        {
            QuaZip zip(fi.absoluteFilePath());
            zip.open(QuaZip::mdUnzip);
            QStringList files=zip.getFileNameList();
            QuaZipFile zip_file(&zip);
            bool find=false;
            foreach (QString str, files)
            {
                QFileInfo arh_file(str);
                if(arh_file.suffix().toLower()=="fb2" || arh_file.suffix().toLower()=="epub")
                {
                    QBuffer buf;
                    if(SetCurrentZipFileName(&zip,str))
                    {
                        zip_file.open(QIODevice::ReadOnly);
                        buf.setData(zip_file.readAll());
                        FileName=fi.absolutePath()+"/"+fi.completeBaseName()+"/"+str;
                        ba=buf.data();
                        GetBookInfo(bi,ba,arh_file.suffix().toLower(),true);
                        fi.setFile(str);
                        find=true;
                    }
                    else
                    {
                        qDebug()<<str;
                    }
                }
            }
            if(!find)
                continue;
        }
        else
            continue;
        QString file_name;
        if(settings->value("originalFileName",false).toBool())
        {
            file_name=QFileInfo(FileName).fileName();
        }
        else
        {
            file_name=settings->value("ExportFileName",default_exp_file_name).toString().trimmed();
            if(file_name.isEmpty())
                file_name=default_exp_file_name;
            file_name=fillParams(file_name,bi,fi)+"."+fi.suffix();;
            if(settings->value("transliteration",false).toBool())
                file_name=Transliteration(file_name);
        }
        file_name=fillParams(dir.path(),bi,fi)+"/"+file_name;
        QBuffer buf(&ba);
        convert((QList<QBuffer*>())<<&buf, file_name,count,false,bi);

    }
}
bool ExportThread::convert(QList<QBuffer*> outbuff, QString file_name, int count, bool remove_old, book_info &bi)
{
    QSettings *settings=GetSettings();
    QString tool=settings->value("current_tool").toString();
    QString tool_path,tool_arg,tool_ext;
    int count_tools=settings->beginReadArray("tools");
    for(int i=0;i<count_tools;i++)
    {
        settings->setArrayIndex(i);
        if(settings->value("name").toString()==tool)
        {
            tool_path=settings->value("path").toString();
            tool_arg=settings->value("args").toString();
            tool_ext=settings->value("ext").toString();
            break;
        }
    }
    settings->endArray();
    QDir book_dir;
    QFileInfo fi(file_name);

    QString tmp_dir;
    if(QStandardPaths::standardLocations(QStandardPaths::TempLocation).count()>0)
        tmp_dir=QStandardPaths::standardLocations(QStandardPaths::TempLocation).at(0);
    book_dir.mkpath(tmp_dir+"/freeLib");

    QStringList out_file;
    int i=0;
    QFile file;
    foreach (QBuffer* buf, outbuff)
    {
        out_file<<tmp_dir+QString("/freeLib/book%1.").arg(QString::number(i))+fi.suffix();
        i++;
        QFile::remove(out_file.last());
        file.setFileName(out_file.last());
        file.open(QFile::WriteOnly);
        file.write(buf->data());
        file.close();
    }
    if(out_file.count()==0)
        return false;
    QString current_out_file=out_file.first();
    if(
            (settings->value("OutputFormat").toString()=="MOBI" ||
             settings->value("OutputFormat").toString()=="EPUB" ||
             settings->value("OutputFormat").toString()=="AZW3" ||
             settings->value("OutputFormat").toString()=="MOBI7" ||
             settings->value("OutputFormat").toString()=="PDF") &&
            (fi.suffix().toLower()=="fb2" || fi.suffix().toLower()=="epub"))
    {
        fb2mobi conv;
        current_out_file=conv.convert(out_file,remove_old,settings->value("OutputFormat").toString(),bi);
    }

    QString book_file_name=fi.absolutePath()+"/"+fi.completeBaseName()+"."+QFileInfo(current_out_file).suffix();

    if(send_type==ST_Mail)
    {
       if(count>1)
       {
           QEventLoop loop; QTimer::singleShot(settings->value("PauseMail",5).toInt()*1000, &loop, SLOT(quit())); loop.exec();
       }
       SmtpClient smtp(settings->value("EmailServer").toString(),settings->value("EmailPort").toInt());
       smtp.setConnectionType((SmtpClient::ConnectionType)settings->value("ConnectionType").toInt());
       if(!settings->value("EmailUser").toString().isEmpty())
           smtp.setUser(settings->value("EmailUser").toString());
       if(!settings->value("EmailPassword").toString().isEmpty())
           smtp.setPassword(decodeStr(settings->value("EmailPassword").toString()));
       MimeMessage msg(true);

       msg.setHeaderEncoding(MimePart::Base64);
       msg.setSender(new EmailAddress(settings->value("from_email").toString(),""));
       msg.addRecipient(new EmailAddress(settings->value("Email").toString(), ""));
       QString caption=settings->value("mail_subject",AppName).toString();
       msg.setSubject(caption.isEmpty()?AppName:caption);

       QBuffer outbuff;//=new QBuffer;
       QString FileName=current_out_file;
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
       MimeText text;
       QString onlyFileName=QFileInfo(book_file_name).fileName();
       text.setText(onlyFileName);
       msg.addPart(&text);
       msg.addPart(new MimeAttachment(&outbuff,onlyFileName));
       smtp.connectToHost();
       smtp.login();
       if(!smtp.sendMail(msg))
       {
           qDebug()<<"Error send e-mail.";
           return false;
       }

       smtp.quit();
       return true;
    }
    else
    {
        book_file_name=ValidateFileName(book_file_name);
        if(!settings->value("PostprocessingCopy",false).toBool())
        {
            book_dir.mkpath(book_dir.cleanPath(QFileInfo(book_file_name).absolutePath()));
            QFile::remove(book_file_name);
            QFile::rename(current_out_file,book_file_name);
        }
        else
            book_file_name=current_out_file;
        QProcess proc;
        if(!tool_path.isEmpty())
        {
            QFileInfo fi_tmp(book_file_name);
            QString arg=tool_arg;
            if(!tool_ext.isEmpty())
            {
                book_file_name=tool_ext;
                book_file_name=fillParams(book_file_name,bi,fi_tmp);
            }
            QString ex=fillParams(tool_path,bi,fi_tmp);
            arg=fillParams(arg,bi,fi_tmp);
            qDebug()<<ex<<arg;
            proc.execute((ex+" "+arg).trimmed());
        }
        return QFileInfo(book_file_name).exists();
    }
}

void ExportThread::export_books()
{
    QSettings *settings=GetSettings();
    QDir dir=export_dir;
    QFileInfo fi;

    if(settings->value("originalFileName",false).toBool() && send_type!=ST_Mail && settings->value("OutputFormat").toString()=="-")
    {
        foreach(book_info i,book_list)
        {
            QString archive,file;
            QString LibPath=mLibs[idCurrentLib].path;
            SBook &book = mLibs[idCurrentLib].mBooks[i.id];
            if(!book.sArchive.isEmpty())
            {
                archive=book.sArchive.replace(".inp",".zip");
            }
            file=book.sName+"."+book.sFormat;
            LibPath=RelativeToAbsolutePath(LibPath);
            archive=archive.replace("\\","/");
            file=file.replace("\\","/");
            if(archive.isEmpty())
            {
                QDir().mkpath(QFileInfo(dir.absolutePath()+"/"+file).absolutePath());
                QFile().copy(LibPath+"/"+file,dir.absolutePath()+"/"+file);
                continue;
            }
            QuaZip uz(LibPath+"/"+archive);
            if(!uz.open(QuaZip::mdUnzip))
            {
                qDebug()<<("Error open archive!")<<" "<<archive;
                continue;
            }
            if(uz.getEntriesCount()>1)
            {
                uz.close();
                QString out_dir=dir.absolutePath()+"/"+archive.left(archive.length()-4);
                QDir().mkpath(out_dir);
                QBuffer buff,infobuff;
                GetBookFile(buff,infobuff,i.id);

                QFile::remove(out_dir+"/"+file);
                QFile outfile;
                outfile.setFileName(out_dir+"/"+file);
                outfile.open(QFile::WriteOnly);
                outfile.write(buff.data());
                outfile.close();
            }
            else
            {
                QDir().mkpath(QFileInfo(dir.absolutePath()+"/"+archive).absolutePath());
                QFile().copy(LibPath+"/"+archive,dir.absolutePath()+"/"+archive);
            }
            successful_export_books<<i.id;
        }
        return;
    }

    long count=0;
    QList<QList<book_info> > books_group;
    qlonglong current_seria_id=-1;
    bool need_group_series=
            (settings->value("OutputFormat").toString()=="EPUB" || settings->value("OutputFormat").toString()=="MOBI" ||
             settings->value("OutputFormat").toString()=="AZW3" || settings->value("OutputFormat").toString()=="MOBI7") &&
            settings->value("join_series").toBool();
    foreach(book_info i,book_list)
    {
        GetBookInfo(i,QByteArray(),"",true,i.id);
        if(current_seria_id!=i.id_seria || !need_group_series || i.id_seria==-1)
        {
            books_group<<QList<book_info>();
        }
        current_seria_id=i.id_seria;
        books_group.last()<<i;
    }
    count=0;

    foreach(QList<book_info> bi,books_group)
    {
        if(!loop_enable)
            break;
        QList<QBuffer*> buffers;
        QString file_name;
        foreach(book_info i,bi)
        {
            app->processEvents();
            count++;
            if(!loop_enable)
                break;
            buffers<<new QBuffer(this);
            QBuffer infobuff;
            fi=GetBookFile(*buffers.last(),infobuff,i.id);
            SBook &book = mLibs[idCurrentLib].mBooks[i.id];
            if(fi.fileName().isEmpty())
            {
                emit Progress(count*100/book_list.count(),count);
                continue;
            }
            if(settings->value("originalFileName",false).toBool())
            {
                QString arh=book.sArchive;
                arh=arh.left(arh.length()-4);
                file_name=arh.isEmpty()?"":QString("%1/%2.%3").arg(arh).arg(book.nFile).arg(book.sFormat);
            }
            else
            {
                file_name=settings->value("ExportFileName",default_exp_file_name).toString().trimmed();
                if(file_name.isEmpty())
                    file_name=default_exp_file_name;
                QString author=BuildFileName(mLibs[idCurrentLib].mAuthors[book.idFirstAuthor].sName);
                QString seria_name=BuildFileName(mLibs[idCurrentLib].mSerials[book.idSerial].sName);
                QString book_name=BuildFileName(book.sName);
                QString ser_num = QString::number(book.numInSerial);
                if(buffers.count()>1)
                {
                    ser_num="0";
                    book_name=seria_name;
                    seria_name="";
                }
                file_name=fillParams(file_name,QFileInfo(),seria_name,book_name,author,ser_num)+"."+book.sFormat;
                if(settings->value("transliteration",false).toBool())
                    file_name=Transliteration(file_name);
            }
            file_name=dir.path()+"/"+file_name;
        }

        if(convert(buffers, file_name,count,true,bi[0]))
        {
            foreach(book_info i,bi)
                successful_export_books<<i.id;
        }
        emit Progress(count*100/book_list.count(),count);
        foreach (QBuffer* buf, buffers)
            delete buf;
    }
}

void ExportThread::process()
{

    if(book_list.count()>0)
        export_books();
    if(book_list_str.count()>0)
        FB2export();
    if(ID_lib>0)
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
    QSqlQuery query(QSqlDatabase::database("libdb"));
    query.exec(QString("SELECT book.id,author.id,janre.id,book.name,star,num_in_seria,book.language,file,size,deleted,date,format,book.keys,archive,date,book.id_inlib, "
               "book.favorite,seria.name,seria.favorite,janre.main_janre||':',author.favorite,author.name1||','||author.name2||','||author.name3||':' "
               "FROM book_author "
               "JOIN book on book.id=book_author.id_book "
               "LEFT JOIN book_janre ON book_janre.id_book=book.id "
               "LEFT JOIN janre ON janre.id=book_janre.id_janre "
               "LEFT JOIN seria ON seria.id=book.id_seria "
               "JOIN author ON author.id=book_author.id_author "
               "WHERE book_author.id_lib=%1 "
               "ORDER BY book.id,seria.id,num_in_seria;").arg(QString::number(ID_lib)));
    int count=0;
    qlonglong lastbookid=-1;

    QString tmp_dir="freeLib";
    if(QStandardPaths::standardLocations(QStandardPaths::TempLocation).count()>0)
        tmp_dir=QStandardPaths::standardLocations(QStandardPaths::TempLocation).at(0)+"/freeLib";
    QString impx_file=tmp_dir+"/freeLib.inp";
    QString structure_file=tmp_dir+"/structure.info";
    QString version_file=tmp_dir+"/version.info";
    QString collection_file=tmp_dir+"/collection.info";
    QDir().mkpath(tmp_dir);
    QFile inpx(impx_file);
    QFile structure(structure_file);
    structure.open(QFile::WriteOnly);
    structure.write(QString("AUTHOR;TITLE;SERIES;SERNO;GENRE;LIBID;INSNO;FILE;FOLDER;EXT;SIZE;LANG;DATE;DEL;KEYWORDS;CRC32;TAG;TAGAUTHOR;TAGSERIES;\r\n").toUtf8());
    structure.close();
    QFile version(version_file);
    version.open(QFile::WriteOnly);
    version.write((QDate::currentDate().toString("dd.MM.yyyy")+"\r\n").toUtf8());
    version.close();
    QFile collection(collection_file);
    collection.open(QFile::WriteOnly);
    collection.write((mLibs[idCurrentLib].name+"\r\n").toUtf8());
    collection.write((mLibs[idCurrentLib].name+"\r\n").toUtf8());
    collection.write(QString("0\r\n").toUtf8());
    collection.write(QString("freeLib\r\n").toUtf8());
    collection.close();
    inpx.open(QFile::WriteOnly);
    if(!inpx.isOpen())
    {
        qDebug()<<"Error create file: "+impx_file;
        emit Progress(0,count);
        return;
    }
    QString authors;
    QString fav_authors;
    QString janres;
    bool next_book=false;
    QString buf;
    while(loop_enable)
    {
        loop_enable=query.next();
        if(!loop_enable)
        {
            if(lastbookid<0)
                break;
            next_book=true;
        }
        else
            next_book=(lastbookid!=query.value(0).toLongLong() && lastbookid>=0);
        if(next_book)
        {
            count++;
            if(int(count/1000)*1000==count)
                emit Progress(0,count);
            inpx.write(buf.toUtf8());
            authors="";
            janres="";
            fav_authors="";
        }
        if(loop_enable)
        {
            lastbookid=query.value(0).toLongLong();
            authors+=query.value(21).toString().trimmed();
            janres+=query.value(19).toString().trimmed();
            fav_authors+=(query.value(20).toString().trimmed()+":");
            buf=(QString("%1\4%2\4%3\4%4\4%5\4%6\4%7\4%8\4%9").arg
                        (
                            authors,                                //AUTHOR
                            query.value(3).toString().trimmed(),    //TITLE
                            query.value(17).toString().trimmed(),   //SERIES
                            query.value(5).toString().trimmed(),    //SERNO
                            janres,                                 //GENRE
                            query.value(15).toString().trimmed(),   //LIBID
                            "",                                     //INSNO
                            query.value(7).toString().trimmed(),    //FILE
                            query.value(13).toString().trimmed()    //FOLDER
                        )+
                           QString("\4%1\4%2\4%3\4%4\4%5\4%6\4%7").arg
                        (
                            query.value(11).toString().trimmed(),   //EXT
                            query.value(8).toString().trimmed(),    //SIZE
                            query.value(6).toString().trimmed(),    //LANG
                            query.value(10).toDate().toString("yyyy-MM-dd"),    //DATE
                            query.value(9).toBool()?"1":"",         //DEL
                            query.value(12).toString().trimmed(),   //KEYWORDS
                            ""                                      //CRC32
                        )+
                        QString("\4%1\4%2\4%3\r\n").arg
                        (
                             query.value(16).toString().trimmed(),  //FAV_BOOK
                             fav_authors,                           //FAV_AUTHOR
                             query.value(18).toString().trimmed()   //FAV_SERIES
                        )
                        );
        }
    }
    inpx.close();
    QuaZip zip(export_dir+QString("/%1.inpx").arg(BuildFileName(mLibs[idCurrentLib].name)));
    qDebug()<<zip.getZipName();
    qDebug()<<zip.open(QuaZip::mdCreate);
    QuaZipFile zip_file(&zip);
    QFile file;

    zip_file.open(QIODevice::WriteOnly,QuaZipNewInfo("freeLib.inp"),0,0,8,5);
    file.setFileName(impx_file);
    file.open(QIODevice::ReadOnly);
    zip_file.write(file.readAll());
    file.close();
    zip_file.close();

    zip_file.open(QIODevice::WriteOnly,QuaZipNewInfo("structure.info"),0,0,8,5);
    file.setFileName(structure_file);
    file.open(QIODevice::ReadOnly);
    zip_file.write(file.readAll());
    file.close();
    zip_file.close();

    zip_file.open(QIODevice::WriteOnly,QuaZipNewInfo("version.info"),0,0,8,5);
    file.setFileName(version_file);
    file.open(QIODevice::ReadOnly);
    zip_file.write(file.readAll());
    file.close();
    zip_file.close();

    zip_file.open(QIODevice::WriteOnly,QuaZipNewInfo("collection.info"),0,0,8,5);
    file.setFileName(collection_file);
    file.open(QIODevice::ReadOnly);
    zip_file.write(file.readAll());
    file.close();
    zip_file.close();

    zip.close();
    emit Progress(0,count);
}

void ExportThread::break_exp()
{
    loop_enable=false;
}

//void ExportThread::smtpError(SmtpError e)
//{
//    qDebug()<<e;
//}
