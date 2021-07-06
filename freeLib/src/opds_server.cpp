#include <QSettings>
#include <QTcpSocket>
#include <QApplication>
#include <QNetworkProxy>
#include <algorithm>

#include "aboutdialog.h"
#include "opds_server.h"
#include "build_number.h"
#include "fb2mobi/fb2mobi.h"

#define SAVE_INDEX  4
//#define MAX_BOOKS_PER_PAGE  15
#define MAX_COLUMN_COUNT    3
QString fillParams(QString str, SBook& book);

opds_server::opds_server(QObject *parent) :
    QObject(parent)
{
    connect(&OPDS_server, SIGNAL(newConnection()), this, SLOT(new_connection()));
    OPDS_server_status=0;
    port=0;
}

QDomElement opds_server::AddTextNode(QString name, QString text,QDomNode &node)
{
    QDomElement el=doc.createElement(name);
    node.appendChild(el);
    if(!text.isEmpty())
    {
        QDomText txt=doc.createTextNode(text);
        el.appendChild(txt);
    }
    return el;
}

QString OPDS_MIME_TYPE(QString type)
{
    type=type.toLower();
    if(type=="jpg" || type=="jpeg")
        return "image/jpeg";
    if(type=="png")
        return "image/png";
    if(type=="txt")
        return "text/plain";
    if(type=="fb2")
        return "application/fb2+zip";
    if(type=="epub")
        return "application/epub+zip";
    if(type=="mobi" || type=="azw" || type=="azw3")
        return "application/x-mobipocket-ebook";
    return "application/octet-stream";
}

QDomElement opds_server::doc_header(QString session, bool html, QString lib_name, QString lib_url)
{
    doc.clear();
    if(html)
    {
        QDomProcessingInstruction xmlProcessingInstruction = doc.createProcessingInstruction("DOCTYPE", "HTML");
        doc.appendChild(xmlProcessingInstruction);
        QDomElement html=doc.createElement("HTML");
        doc.appendChild(html);
        QDomElement head=doc.createElement("HEAD");
        html.appendChild(head);
        AddTextNode("TITLE",lib_name,head);

        QString css=QString(
                "a.lib {font-size:%1em;font-weight: bold;}\n"
                "a.book {font-size:%1em;font-weight: bold;}\n"
                "div.author {font-size: %2em; background: #eeeeee; border-radius: 0.5em ;margin: 0.5em;padding: 0.1em;}\n"
                "div.caption {font-size: %1em;font-weight: bold;padding-bottom: 0.1em;color: #000000;text-decoration: underline;}\n"
                "div.book {font-size: %3em;font-weight: bold;padding-bottom: 0.1em;color: #000000;}\n"
                "input {font-size: %3em;font-weight: bold;padding-bottom: 0.1em;color: #000000;}\n"
                "a {font-size: %3em;font-weight: #000000; color: black;text-decoration: none;}\n"
                "a.block {display: block;}\n"
                "a:active {color: #000000;text-decoration: underline;}\n"
                "a:link {color: #000000;text-decoration: none;}\n"
                "a:visited {color: #000000;text-decoration: none;}\n"
                "a:focus {color: #000000;text-decoration: underline;}\n"
                "a:hover {color: #000000;text-decoration: underline;}\n"
                "a.author {font-weight: #444444;}\n"
                "a.author:active {color: #444444;}\n"
                "a.author:link {color: #444444;}\n"
                "a.author:visited {color: #444444;}\n"
                "a.author:focus {color: #444444;}\n"
                "a.author:hover {color: #444444;}\n"
                "body {background-color: #ffffff;}\n"
                "img.cover {height: %4em; float:left; margin-right: 0.5em;}\n"

                    ).arg(
                    for_mobile?"3":"2",for_mobile?"1.5":"1",for_mobile?"2":"1.5",for_mobile?"8":"6");
                ;
        QDomElement style=AddTextNode("style",css,head);
        style.setAttribute("type","text/css");
        QDomElement meta=doc.createElement("META");
        meta.setAttribute("http-equiv","Content-Type");
        meta.setAttribute("content","text/html; charset=utf-8");
        head.appendChild(meta);

        QDomElement link;
        QString icon_file="/icon_256x256.png";
        if(for_preview)
            icon_file="/splash_opera.png";

        link=doc.createElement("LINK");
        link.setAttribute("rel","icon");
        link.setAttribute("type","image/png");
        link.setAttribute("href",icon_file+(session.isEmpty()?"":"?session="+session));
        head.appendChild(link);
        link=doc.createElement("LINK");
        link.setAttribute("rel","shortcut icon");
        link.setAttribute("type","image/png");
        link.setAttribute("href",icon_file+(session.isEmpty()?"":"?session="+session));
        head.appendChild(link);
        link=doc.createElement("LINK");
        link.setAttribute("rel","apple-touch-icon");
        link.setAttribute("type","image/png");
        link.setAttribute("href",icon_file+(session.isEmpty()?"":"?session="+session));
        head.appendChild(link);

        QDomElement body=doc.createElement("BODY");
        html.appendChild(body);
        QDomElement div=AddTextNode("a","",body);
        div.setAttribute("class","lib");
        div.setAttribute("href",(lib_url.isEmpty()?"/":lib_url)+(session.isEmpty()?"":"?session="+session));
        QDomElement img=doc.createElement("img");
        img.setAttribute("src","/home.png");
        img.setAttribute("border","0");
        img.setAttribute("height",QString("%1px").arg(for_mobile?"48":"32"));
        div.appendChild(img);
        div=AddTextNode("a"," "+lib_name,body);
        div.setAttribute("class","lib");
        div.setAttribute("href",(lib_url.isEmpty()?"/":lib_url)+(session.isEmpty()?"":"?session="+session));

        QDomElement hr=doc.createElement("HR");
        hr.setAttribute("size","3");
        hr.setAttribute("color","black");
        body.appendChild(hr);

        return body;
    }
    else
    {
        QDomProcessingInstruction xmlProcessingInstruction = doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"utf-8\"");
        doc.appendChild(xmlProcessingInstruction);
        QDomElement feed=doc.createElement("feed");
        doc.appendChild(feed);
        feed.setAttribute("xmlns","http://www.w3.org/2005/Atom");
        feed.setAttribute("xmlns:dc","http://purl.org/dc/terms/");
        feed.setAttribute("xmlns:os","http://a9.com/-/spec/opensearch/1.1/");
        feed.setAttribute("xmlns:opds","http://opds-spec.org/2010/catalog");
        return feed;
    }
}

/*
QString opds_server::books_list(QString lib_url, QString current_url, QString id_lib, QString author, QString seria, QString ganre, QTextStream &ts, bool opds, QString lib_name, QString session, bool all, QString search)
{
    if(current_url.right(1)=="/")
        current_url=current_url.left(current_url.length()-1);
    QSettings *settings=GetSettings();
    long MAX_BOOKS_PER_PAGE=settings->value("books_per_page",15).toInt();
    if(MAX_BOOKS_PER_PAGE==0)
        MAX_BOOKS_PER_PAGE=1000000;
    bool show_covers=settings->value("srv_covers",true).toBool();
    bool show_annotation=settings->value("srv_annotation",true).toBool();
    QDomElement feed;
    long first_book=0;
    if(params.contains("page"))
        first_book=params.value("page").toLong();
    if(opds)
    {
        ts<<WriteSuccess("application/atom+xml;charset=utf-8");
        feed=doc_header(session);
    }
    else
    {
        ts<<WriteSuccess();
        feed=doc_header(session,true,lib_name,lib_url);
    }
    if(author.isEmpty() && seria.isEmpty() && ganre.isEmpty() && search.isEmpty())
        return "";

    QSqlQuery query(QSqlDatabase::database("libdb"));
    if(!search.isEmpty())
    {
        if(opds)
        {
            AddTextNode("title",tr("Books search"),feed);
        }
        else
        {
            QDomElement div=AddTextNode("DIV",tr("Books search"),feed);
            div.setAttribute("class","caption");
        }
    }
    else if(!seria.isEmpty() && seria!="-1")
    {
        query.exec("SELECT name FROM seria WHERE id= "+seria);
        QString seria_str;
        while(query.next())
        {
            seria_str=query.value(0).toString().trimmed();
        }
        if(opds)
        {
            AddTextNode("title",tr("Books of sequence")+" ("+seria_str+")",feed);
        }
        else
        {
            QDomElement div=AddTextNode("DIV",tr("Books of sequence")+" ("+seria_str+")",feed);
            div.setAttribute("class","caption");
        }
    }
    else if(all || !ganre.isEmpty())
    {
        query.exec("SELECT name1||' '||name2||' '||name3 FROM author WHERE id= "+author);
        QString author_str;
        while(query.next())
        {
            author_str=query.value(0).toString().trimmed();
        }
        if(opds)
        {
            AddTextNode("title",tr("Books by ABC")+" ("+author_str+")",feed);
        }
        else
        {
            QDomElement div=AddTextNode("DIV",tr("Books by ABC")+" ("+author_str+")",feed);
            div.setAttribute("class","caption");
        }
    }
    else
    {
        query.exec("SELECT name1||' '||name2||' '||name3 FROM author WHERE id= "+author);
        QString author_str;
        while(query.next())
        {
            author_str=query.value(0).toString().trimmed();
        }
        if(opds)
        {
            AddTextNode("title",tr("Books without sequence")+" ("+author_str+")",feed);
        }
        else
        {
            QDomElement div=AddTextNode("DIV",tr("Books without sequence")+" ("+author_str+")",feed);
            div.setAttribute("class","caption");
        }
    }
    if(opds)
    {
        AddTextNode("updated",QDateTime::currentDateTimeUtc().toString(Qt::ISODate),feed);
        AddTextNode("icon","/icon_256x256.png",feed);
    }
    QString condition="";
    if(!author.isEmpty())
        condition+="book_author.id_author="+author+" and ";
    if(!seria.isEmpty())
        condition+=QString("(book.id_seria="+seria+" %1) and ").arg(seria=="-1"?"|| book.id_seria=0":"");
    QString add_ganre="";
    if(!ganre.isEmpty())
    {
        condition+="janre.id="+ganre+" and ";
        add_ganre="left join book_janre on book_janre.id_book=book.id left join janre on book_janre.id_janre=janre.id";
    }
    if(!search.isEmpty())
    {
        qDebug()<<search;
        QStringList str_author=search.split(' ');
        QString where_author;
        foreach (QString str, str_author)
        {
            if(str.trimmed().isEmpty())
                continue;
            where_author+=QString(where_author.isEmpty()?"":" AND ")+" author.rus_index LIKE '%"+ToIndex(str.trimmed())+"%'";
        }

        //condition+="(("+where_author+") or book.name_index LIKE '%"+ToIndex(search)+"%' ) and ";
        condition+="(("+where_author+") or book.name LIKE '%"+search+"%' ) and ";
    }
    if(!id_lib.isEmpty())
        condition+="book_author.id_lib="+id_lib+" and ";
    if(!settings->value("ShowDeleted",false).toBool())
        condition+="not deleted and ";
    if(search.isEmpty())
    {
        query.exec(QString("SELECT book.name,book.id,book.file,book.format,seria.name,name1||' '||name2||' '||name3,seria.id,author.id,num_in_seria,book.language from book_author "
                   "join book on book.id=book_author.id_book left join author on book_author.id_author=author.id left join seria on book.id_seria=seria.id "+add_ganre+" WHERE "+
               condition.left(condition.length()-5)+" ORDER BY author.name1, seria.name,num_in_seria,book.name LIMIT %1 OFFSET %2").
               arg(QString::number(MAX_BOOKS_PER_PAGE+1),QString::number(first_book*MAX_BOOKS_PER_PAGE)));
    }
    else
    {
        query.exec(QString("SELECT DISTINCT book.name,book.id,book.file,book.format,seria.name,first_author.name1||' '||first_author.name2||' '||first_author.name3,seria.id,first_author_id,num_in_seria,book.language from book_author "
                   "join book on book.id=book_author.id_book join author AS first_author on book.first_author_id=first_author.id left join author on book_author.id_author=author.id left join seria on book.id_seria=seria.id "+add_ganre+" WHERE "+
               condition.left(condition.length()-5)+" ORDER BY first_author.rus_index, seria.name,num_in_seria,book.name_index LIMIT %1 OFFSET %2").
               arg(QString::number(MAX_BOOKS_PER_PAGE+1),QString::number(first_book*MAX_BOOKS_PER_PAGE)));
    }
    QSqlQuery queryAuthors(QSqlDatabase::database("libdb"));
    QSqlQuery queryJanres(QSqlDatabase::database("libdb"));
    long current_books_count=MAX_BOOKS_PER_PAGE;
    QString parameters="";
    QStringList keys=params.keys();
    foreach (QString param, keys)
    {
        //qDebug()<<"param="<<param;
        if(param!="page")
        {
            parameters+="&"+param+"="+params.value(param);
        }
    }
    parameters+=(session.isEmpty()?"":QString((parameters.isEmpty()?"?":"&"))+"session="+session);
    QList<qlonglong> id_books;
    while(query.next())
    {
        id_books<<query.value(1).toLongLong();
        if(current_books_count==0)
        {
            if(opds)
            {

                QDomElement entry=doc.createElement("entry");
                feed.appendChild(entry);
                AddTextNode("updated",QDateTime::currentDateTimeUtc().toString(Qt::ISODate),entry);
                AddTextNode("id","tag:root",entry);
                AddTextNode("title",tr("Next page"),entry);
                QDomElement el=AddTextNode("link","",entry);
                el.setAttribute("href",lib_url+current_url+QString("?page=%1").arg(QString::number(first_book+1))+parameters);
                el.setAttribute("type","application/atom+xml;profile=opds-catalog");
                el=AddTextNode("link","",entry);
                el.setAttribute("href","/arrow_right.png"+(session.isEmpty()?"":"?session="+session));
                el.setAttribute("rel","http://opds-spec.org/image");
                el.setAttribute("type","image/jpeg");
            }
            else
            {
                QDomElement entry=doc.createElement("div");
                feed.appendChild(entry);
               // entry.setAttribute("class","author");

                QDomElement el=AddTextNode("a",tr("Next page"),entry);
                el.setAttribute("href",lib_url+current_url+QString("?page=%1").arg(QString::number(first_book+1))+parameters);
                el.setAttribute("class","author");
            }
            break;
        }
        current_books_count--;
        if(opds)
        {
            QDomElement entry=doc.createElement("entry");
            feed.appendChild(entry);
            AddTextNode("updated",QDateTime::currentDateTimeUtc().toString(Qt::ISODate),entry);
            AddTextNode("id","tag:book:"+query.value(1).toString(),entry);
            AddTextNode("title",query.value(0).toString()+(query.value(4).toString().isEmpty()?"":" ("+query.value(4).toString()+")"),entry);
            queryAuthors.exec("SELECT name1||' '||name2||' '||name3 FROM book_author JOIN author ON id_author=author.id WHERE id_book="+query.value(1).toString());
            while(queryAuthors.next())
            {
                QDomElement author=doc.createElement("author");
                entry.appendChild(author);
                AddTextNode("name",queryAuthors.value(0).toString(),author);
            }
            queryJanres.exec("SELECT name FROM book_janre JOIN janre ON id_janre=janre.id WHERE id_book="+query.value(1).toString());
            while(queryJanres.next())
            {
                QDomElement category=doc.createElement("category");
                entry.appendChild(category);
                category.setAttribute("term",queryJanres.value(0).toString());
                category.setAttribute("label",queryJanres.value(0).toString());
            }

            if(query.value(3).toString().toLower()=="fb2")
            {
                QDomElement el=AddTextNode("link","",entry);
                el.setAttribute("href",lib_url+"/book/"+ QUrl::toPercentEncoding(query.value(1).toString())+"/fb2"+(session.isEmpty()?"":"?session="+session));
                el.setAttribute("rel","http://opds-spec.org/acquisition/open-access");
                el.setAttribute("type","application/fb2");
            }
            if(query.value(3).toString().toLower()=="fb2" || query.value(3).toString().toLower()=="epub")
            {
                QDomElement el=AddTextNode("link","",entry);
                el.setAttribute("href",lib_url+"/book/"+ QUrl::toPercentEncoding(query.value(1).toString())+"/epub"+(session.isEmpty()?"":"?session="+session));
                el.setAttribute("rel","http://opds-spec.org/acquisition/open-access");
                el.setAttribute("type","application/epub+zip");
            }
            if(query.value(3).toString().toLower()=="fb2" || query.value(3).toString().toLower()=="epub" || query.value(3).toString().toLower()=="mobi")
            {
                QDomElement el=AddTextNode("link","",entry);
                el.setAttribute("href",lib_url+"/book/"+ QUrl::toPercentEncoding(query.value(1).toString())+"/mobi"+(session.isEmpty()?"":"?session="+session));
                el.setAttribute("rel","http://opds-spec.org/acquisition/open-access");
                el.setAttribute("type","application/x-mobipocket-ebook");
            }
            if(!(query.value(3).toString().toLower()=="fb2" || query.value(3).toString().toLower()=="epub" || query.value(3).toString().toLower()=="mobi"))
            {
                QDomElement el=AddTextNode("link","",entry);
                el.setAttribute("href",lib_url+"/book/"+ QUrl::toPercentEncoding(query.value(1).toString())+"/download"+(session.isEmpty()?"":"?session="+session));
                el.setAttribute("rel","http://opds-spec.org/acquisition/open-access");
                el.setAttribute("type","application/"+query.value(3).toString().toLower());
            }
            QDomElement el;

            el=AddTextNode("link","",entry);
            el.setAttribute("href",lib_url+"/book/"+ QUrl::toPercentEncoding(query.value(1).toString())+"/download"+(session.isEmpty()?"":"?session="+session));// /"+query.value(2).toString()+"."+query.value(3).toString());
            el.setAttribute("rel","alternate");
            el.setAttribute("type","application/"+query.value(3).toString().toLower());
            el.setAttribute("title",tr("Download"));

            if(show_covers)
            {
                el=AddTextNode("link","",entry);
                el.setAttribute("href",lib_url+"/covers/"+ QUrl::toPercentEncoding(query.value(1).toString())+"/cover.jpg"+(session.isEmpty()?"":"?session="+session));
                el.setAttribute("rel","http://opds-spec.org/image");
                el.setAttribute("type","image/jpeg");
                el=AddTextNode("link","",entry);
                el.setAttribute("href",lib_url+"/covers/"+ QUrl::toPercentEncoding(query.value(1).toString())+"/cover.jpg"+(session.isEmpty()?"":"?session="+session));
                el.setAttribute("rel","x-stanza-cover-image");
                el.setAttribute("type","image/jpeg");
                el=AddTextNode("link","",entry);
                el.setAttribute("href",lib_url+"/covers/"+ QUrl::toPercentEncoding(query.value(1).toString())+"/cover.jpg"+(session.isEmpty()?"":"?session="+session));
                el.setAttribute("rel","http://opds-spec.org/thumbnail");
                el.setAttribute("type","image/jpeg");
                el=AddTextNode("link","",entry);
                el.setAttribute("href",lib_url+"/covers/"+ QUrl::toPercentEncoding(query.value(1).toString())+"/cover.jpg"+(session.isEmpty()?"":"?session="+session));
                el.setAttribute("rel","x-stanza-cover-image-thumbnail");
                el.setAttribute("type","image/jpeg");
            }
            AddTextNode("dc:language",query.value(9).toString(),entry);
            AddTextNode("dc:format",query.value(3).toString(),entry);

            if(show_annotation)
            {
                QBuffer outbuff;
                QBuffer infobuff;
                QFileInfo fi_book;
                fi_book=GetBookFile(outbuff,infobuff,query.value(1).toLongLong());
                if(fi_book.suffix().toLower()=="fb2")
                {
                    book_info book_inf_tmp;
                    GetBookInfo(book_inf_tmp,infobuff.size()==0?outbuff.data():infobuff.data(),"fb2",false,query.value(1).toLongLong());
                    el=AddTextNode("content",book_inf_tmp.annotation,entry);
                    el.setAttribute("type","text/html");
                }
            }

        }
        else
        {
            QDomElement entry=doc.createElement("div");
            entry.setAttribute("class","author");
            feed.appendChild(entry);
            QDomElement table=doc.createElement("table");
            table.setAttribute("width","100%");
            entry.appendChild(table);
            QDomElement tr=doc.createElement("tr");
            table.appendChild(tr);
            entry=doc.createElement("td");
            tr.appendChild(entry);

            if(show_covers)
            {
                QDomElement el=doc.createElement("img");
                entry.appendChild(el);
                el.setAttribute("src",lib_url+"/covers/"+ QUrl::toPercentEncoding(query.value(1).toString())+"/cover.jpg");
                el.setAttribute("class","cover");
            }
            if(!search.isEmpty())
            {
                if(!query.value(5).toString().isEmpty())
                {
                    QDomElement el=AddTextNode("a",query.value(5).toString(),entry);
                    el.setAttribute("class","book");
                    el.setAttribute("href",QString("/author/%1").arg(query.value(7).toString())+(session.isEmpty()?"":"?session="+session));
                }
            }
            QDomElement el=AddTextNode("div",
                 query.value(0).toString()+(query.value(4).toString().isEmpty()?"":" ("+
                 query.value(4).toString()+(query.value(8).toInt()==0?"":" ["+query.value(8).toString()+"]")+")"),entry);
            el.setAttribute("class","book");
            QDomElement br=doc.createElement("br");
            entry.appendChild(br);

            if(query.value(3).toString().toLower()=="fb2")
            {
                QDomElement el=AddTextNode("a","fb2",entry);
                el.setAttribute("href",lib_url+"/book/"+ QUrl::toPercentEncoding(query.value(1).toString())+"/fb2"+(session.isEmpty()?"":"?session="+session));
                el.setAttribute("class","author");
                el=AddTextNode("a","epub",entry);
                el.setAttribute("href",lib_url+"/book/"+ QUrl::toPercentEncoding(query.value(1).toString())+"/epub"+(session.isEmpty()?"":"?session="+session));
                el.setAttribute("class","author");
                el=AddTextNode("a","mobi",entry);
                el.setAttribute("href",lib_url+"/book/"+ QUrl::toPercentEncoding(query.value(1).toString())+"/mobi"+(session.isEmpty()?"":"?session="+session));
                el.setAttribute("class","author");
                el=AddTextNode("a","azw3",entry);
                el.setAttribute("href",lib_url+"/book/"+ QUrl::toPercentEncoding(query.value(1).toString())+"/azw3"+(session.isEmpty()?"":"?session="+session));
                el.setAttribute("class","author");
            }
            else if(query.value(3).toString().toLower()=="epub")
            {
                QDomElement el=AddTextNode("a","epub",entry);
                el.setAttribute("href",lib_url+"/book/"+ QUrl::toPercentEncoding(query.value(1).toString())+"/epub"+(session.isEmpty()?"":"?session="+session));
                el.setAttribute("class","author");
                el=AddTextNode("a","mobi",entry);
                el.setAttribute("href",lib_url+"/book/"+ QUrl::toPercentEncoding(query.value(1).toString())+"/mobi"+(session.isEmpty()?"":"?session="+session));
                el.setAttribute("class","author");
            }
            else if(query.value(3).toString().toLower()=="mobi")
            {
                QDomElement el=AddTextNode("a","mobi",entry);
                el.setAttribute("href",lib_url+"/book/"+ QUrl::toPercentEncoding(query.value(1).toString())+"/mobi"+(session.isEmpty()?"":"?session="+session));
                el.setAttribute("class","author");
            }
            else
            {
                QDomElement el=AddTextNode("a",query.value(3).toString().toLower(),entry);
                el.setAttribute("href",lib_url+"/book/"+ QUrl::toPercentEncoding(query.value(1).toString())+"/download"+(session.isEmpty()?"":"?session="+session));
                el.setAttribute("class","author");
            }
            if(show_annotation)
            {
                QBuffer outbuff;
                QBuffer infobuff;
                QFileInfo fi_book;
                fi_book=GetBookFile(outbuff,infobuff,query.value(1).toLongLong());
                if(fi_book.suffix().toLower()=="fb2")
                {
                    book_info book_inf_tmp;
                    GetBookInfo(book_inf_tmp,infobuff.size()==0?outbuff.data():infobuff.data(),"fb2",false,query.value(1).toLongLong());
                    QDomDocument an;
                    an.setContent(QString("<dev>%1</dev>").arg(book_inf_tmp.annotation));
                    QDomNode an_node=doc.importNode(an.childNodes().at(0),true);
                    entry.appendChild(an_node);
                }
            }
        }
         //<link href="/b/190619/download" rel="http://opds-spec.org/acquisition/disabled" type="application/fb2+zip" />
        if(current_books_count==0 && first_book>0)
        {
            if(opds)
            {
                QDomElement entry=doc.createElement("entry");
                feed.appendChild(entry);
                AddTextNode("updated",QDateTime::currentDateTimeUtc().toString(Qt::ISODate),entry);
                AddTextNode("id","tag:root",entry);
                AddTextNode("title",tr("Previous page"),entry);
                QDomElement el=AddTextNode("link","",entry);
                el.setAttribute("href",lib_url+current_url+QString("?page=%1").arg(QString::number(first_book-1))+parameters);
                el.setAttribute("type","application/atom+xml;profile=opds-catalog");

                el=AddTextNode("link","",entry);
                el.setAttribute("href","/arrow_left.png"+(session.isEmpty()?"":"?session="+session));
                el.setAttribute("rel","http://opds-spec.org/image");
                el.setAttribute("type","image/jpeg");
            }
            else
            {
                QDomElement entry=doc.createElement("div");
                feed.appendChild(entry);
                QDomElement el=AddTextNode("a",tr("Previous page"),entry);
                el.setAttribute("href",lib_url+current_url+QString("?page=%1").arg(QString::number(first_book-1))+parameters);
                el.setAttribute("class","author");
//                QDomElement img=doc.createElement("img");
//                img.setAttribute("src","/arrow_left.png");
//                img.setAttribute("alt",tr("Previous page"));
//                img.setAttribute("height",QString("%1px").arg(for_mobile?"64":"32"));
//                el.appendChild(img);
//                AddTextNode("b","    ",entry);
            }
        }
    }
    if(current_books_count>0 && first_book>0)
    {
        if(opds)
        {
            QDomElement entry=doc.createElement("entry");
            feed.appendChild(entry);
            AddTextNode("updated",QDateTime::currentDateTimeUtc().toString(Qt::ISODate),entry);
            AddTextNode("id","tag:root",entry);
            AddTextNode("title",tr("Previous page"),entry);
            QDomElement el=AddTextNode("link","",entry);
            el.setAttribute("href",lib_url+current_url+QString("?page=%1").arg(QString::number(first_book-1))+parameters);
            el.setAttribute("type","application/atom+xml;profile=opds-catalog");

            el=AddTextNode("link","",entry);
            el.setAttribute("href","/arrow_left.png"+(session.isEmpty()?"":"?session="+session));
            el.setAttribute("rel","http://opds-spec.org/image");
            el.setAttribute("type","image/jpeg");
        }
        else
        {
            QDomElement entry=doc.createElement("div");
            feed.appendChild(entry);
            QDomElement el=AddTextNode("a",tr("Previous page"),entry);
            el.setAttribute("href",lib_url+current_url+QString("?page=%1").arg(QString::number(first_book-1))+parameters);
            el.setAttribute("class","author");
            //QDomElement img=doc.createElement("img");
            //img.setAttribute("src","/arrow_left.png");
            //img.setAttribute("alt",tr("Previous page"));
            //img.setAttribute("height",QString("%1px").arg(for_mobile?"64":"32"));
            //el.appendChild(img);
            //AddTextNode("b","    ",entry);
        }
    }
    if(opds)
        return doc.toString();
    else
    {
        QString str;
        QTextStream t(&str,QIODevice::WriteOnly);
        doc.namedItem("HTML").save(t,SAVE_INDEX);
        return str;
    }
}
*/

QList<uint> opds_server::book_list(SLib &lib, uint idAuthor, uint idSeria, uint idGenre, QString sSearch, bool sequenceless = false)
{
    QList<uint> listBooks;
    if(idAuthor!=0 && idSeria>0){
        QMultiHash<uint,uint>::const_iterator i = lib.mAuthorBooksLink.find(idAuthor);
        while(i!=lib.mAuthorBooksLink.end() && i.key()==idAuthor){
            if(!lib.mBooks[i.value()].bDeleted && lib.mBooks[i.value()].idSerial == idSeria)
                listBooks<<i.value();
            ++i;
        }
        std::sort(listBooks.begin(), listBooks.end(),[lib](const uint& lhs, const uint& rhs){return lib.mBooks[lhs].numInSerial<lib.mBooks[rhs].numInSerial;});
    }
    if(idAuthor != 0 && idSeria==0){
        QMultiHash<uint,uint>::const_iterator i = lib.mAuthorBooksLink.constFind(idAuthor);
        if(sequenceless){
            while(i!=lib.mAuthorBooksLink.constEnd() && i.key()==idAuthor){
                if(!lib.mBooks[i.value()].bDeleted && lib.mBooks[i.value()].idSerial == 0)
                    listBooks<<i.value();
                ++i;
            }
        }else{
            while(i!=lib.mAuthorBooksLink.constEnd() && i.key()==idAuthor){
                if(!lib.mBooks[i.value()].bDeleted)
                    listBooks<<i.value();
                ++i;
            }
        }
        std::sort(listBooks.begin(), listBooks.end(),[lib](const uint& lhs, const uint& rhs){return lib.mBooks[lhs].sName<lib.mBooks[rhs].sName;});
    }
    if(idAuthor==0 && idSeria>0){
        auto iBook = lib.mBooks.constBegin();
        while(iBook != lib.mBooks.constEnd()){
            if(!iBook->bDeleted && iBook->idSerial == idSeria)
                listBooks << iBook.key();
            ++iBook;
        }
        std::sort(listBooks.begin(), listBooks.end(),[lib](const uint& lhs, const uint& rhs){return lib.mBooks[lhs].numInSerial<lib.mBooks[rhs].numInSerial;});
    }
    if(idGenre!=0){
        auto iBook = lib.mBooks.constBegin();
        while(iBook != lib.mBooks.constEnd()){
            if(!iBook->bDeleted)
                foreach(auto iGenre,iBook->listIdGenres){
                    if(iGenre == idGenre){
                        listBooks << iBook.key();
                        break;
                    }
                }
            ++iBook;
        }
        std::sort(listBooks.begin(), listBooks.end(),[lib](const uint& lhs, const uint& rhs){return lib.mBooks[lhs].sName<lib.mBooks[rhs].sName;});
    }
    if(!sSearch.isEmpty()){
        auto iBook = lib.mBooks.constBegin();
        while(iBook != lib.mBooks.constEnd()){
            if(!iBook->bDeleted){
                if(iBook->sName.contains(sSearch,Qt::CaseInsensitive))
                    listBooks << iBook.key();
                else{
                    foreach(uint idAuthor,iBook->listIdAuthors){
                        if(lib.mAuthors[idAuthor].getName().contains(sSearch,Qt::CaseInsensitive)){
                            listBooks<< iBook.key();
                            break;
                        }
                    }
                }
            }
            ++iBook;
        }
        std::sort(listBooks.begin(), listBooks.end(), [lib](const uint& lhs, const uint& rhs){
            if(lib.mBooks[lhs].idFirstAuthor != lib.mBooks[rhs].idFirstAuthor)
                return lib.mAuthors[lib.mBooks[lhs].idFirstAuthor].getName()<lib.mAuthors[lib.mBooks[rhs].idFirstAuthor].getName();
            else
                return lib.mBooks[lhs].sName<lib.mBooks[rhs].sName;

        });

    }
    return listBooks;
}

QString opds_server::FillPage(QList<uint> listBooks, SLib& lib, QString sTitle, QString lib_url, QString current_url, QTextStream &ts, bool opds, uint nPage, QString session, bool bShowAuthor)
{
    QSettings *settings=GetSettings();
    long MAX_BOOKS_PER_PAGE=settings->value("books_per_page",15).toInt();
    if(MAX_BOOKS_PER_PAGE==0)
        MAX_BOOKS_PER_PAGE=1000000;
    bool show_covers=settings->value("srv_covers",true).toBool();
    bool show_annotation=settings->value("srv_annotation",true).toBool();
    QDomElement feed;
    if(opds)
    {
        ts<<WriteSuccess("application/atom+xml;charset=utf-8");
        feed=doc_header(session);
    }
    else
    {
        ts<<WriteSuccess();
        feed=doc_header(session,true,lib.name,lib_url);
    }
    if(listBooks.isEmpty())
        return "";
    QString parameters="";
    QStringList keys=params.keys();
    foreach (QString param, keys)
    {
        //qDebug()<<"param="<<param;
        if(param!="page")
        {
            parameters+="&"+param+"="+params.value(param);
        }
    }
    parameters+=(session.isEmpty()?"":QString((parameters.isEmpty()?"?":"&"))+"session="+session);

    if(opds)
    {
        AddTextNode("title",sTitle,feed);
        AddTextNode("updated",QDateTime::currentDateTimeUtc().toString(Qt::ISODate),feed);
        AddTextNode("icon","/icon_256x256.png",feed);
        int iBook =0;
        foreach(uint idBook,listBooks){
            if(iBook>=nPage*MAX_BOOKS_PER_PAGE && iBook<(nPage+1)*MAX_BOOKS_PER_PAGE){
                SBook& book = lib.mBooks[idBook];
                QString sIdBook = QString::number(idBook);
                QDomElement entry=doc.createElement("entry");
                feed.appendChild(entry);
                AddTextNode("updated",QDateTime::currentDateTimeUtc().toString(Qt::ISODate),entry);
                AddTextNode("id",QString("tag:book:%1").arg(idBook),entry);
                AddTextNode("title",book.sName+(book.idSerial==0?"":" ("+lib.mSerials[book.idSerial].sName+")"),entry);
                foreach(uint idAuthor,book.listIdAuthors){
                    QDomElement author=doc.createElement("author");
                    entry.appendChild(author);
                    AddTextNode("name",lib.mAuthors[idAuthor].getName(),author);
                }
                foreach(uint idGenre,book.listIdGenres){
                    QDomElement category=doc.createElement("category");
                    entry.appendChild(category);
                    category.setAttribute("term",mGenre[idGenre].sName);
                    category.setAttribute("label",mGenre[idGenre].sName);
                }
                if(book.sFormat=="fb2")
                {
                    QDomElement el=AddTextNode("link","",entry);
                    el.setAttribute("href",lib_url+"/book/"+ QUrl::toPercentEncoding(sIdBook)+"/fb2"+(session.isEmpty()?"":"?session="+session));
                    el.setAttribute("rel","http://opds-spec.org/acquisition/open-access");
                    el.setAttribute("type","application/fb2");
                }
                else if(book.sFormat=="epub" || book.sFormat=="mobi"){
                    QDomElement el=AddTextNode("link","",entry);
                    el.setAttribute("href",lib_url+"/book/"+ QUrl::toPercentEncoding(sIdBook)+"/download"+(session.isEmpty()?"":"?session="+session));
                    el.setAttribute("rel","http://opds-spec.org/acquisition/open-access");
                    el.setAttribute("type","application/" + book.sFormat);
                }
                QDomElement el;

                el=AddTextNode("link","",entry);
                el.setAttribute("href",lib_url+"/book/"+ QUrl::toPercentEncoding(sIdBook)+"/download"+(session.isEmpty()?"":"?session="+session));
                el.setAttribute("rel","alternate");
                el.setAttribute("type","application/"+book.sFormat);
                el.setAttribute("title",tr("Download"));

                if(show_covers)
                {
                    el=AddTextNode("link","",entry);
                    el.setAttribute("href",lib_url+"/covers/"+ QUrl::toPercentEncoding(sIdBook)+"/cover.jpg"+(session.isEmpty()?"":"?session="+session));
                    el.setAttribute("rel","http://opds-spec.org/image");
                    el.setAttribute("type","image/jpeg");
                    el=AddTextNode("link","",entry);
                    el.setAttribute("href",lib_url+"/covers/"+ QUrl::toPercentEncoding(sIdBook)+"/cover.jpg"+(session.isEmpty()?"":"?session="+session));
                    el.setAttribute("rel","x-stanza-cover-image");
                    el.setAttribute("type","image/jpeg");
                    el=AddTextNode("link","",entry);
                    el.setAttribute("href",lib_url+"/covers/"+ QUrl::toPercentEncoding(sIdBook)+"/cover.jpg"+(session.isEmpty()?"":"?session="+session));
                    el.setAttribute("rel","http://opds-spec.org/thumbnail");
                    el.setAttribute("type","image/jpeg");
                    el=AddTextNode("link","",entry);
                    el.setAttribute("href",lib_url+"/covers/"+ QUrl::toPercentEncoding(sIdBook)+"/cover.jpg"+(session.isEmpty()?"":"?session="+session));
                    el.setAttribute("rel","x-stanza-cover-image-thumbnail");
                    el.setAttribute("type","image/jpeg");
                }
                AddTextNode("dc:language",lib.vLaguages[book.idLanguage],entry);
                AddTextNode("dc:format",book.sFormat,entry);

                if(show_annotation)
                {
                    QBuffer outbuff;
                    QBuffer infobuff;
                    QFileInfo fi_book;
                    fi_book=GetBookFile(outbuff,infobuff,idBook);
                    if(fi_book.suffix().toLower()=="fb2")
                    {
                        if(book.sAnnotation.isEmpty()){
                            book_info book_inf_tmp;
                            GetBookInfo(book_inf_tmp,infobuff.size()==0?outbuff.data():infobuff.data(),"fb2",false,idBook);
                            book.sAnnotation = book_inf_tmp.annotation;
                        }
                        el=AddTextNode("content",book.sAnnotation,entry);
                        el.setAttribute("type","text/html");
                    }
                }
            }
            iBook++;
        }
        if(nPage>=1){
            QDomElement entry=doc.createElement("entry");
            feed.appendChild(entry);
            AddTextNode("updated",QDateTime::currentDateTimeUtc().toString(Qt::ISODate),entry);
            AddTextNode("id","tag:root",entry);
            AddTextNode("title",tr("Previous page"),entry);
            QDomElement el=AddTextNode("link","",entry);
            el.setAttribute("href",lib_url+current_url+QString("?page=%1").arg(nPage-1)+parameters);
            el.setAttribute("type","application/atom+xml;profile=opds-catalog");

            el=AddTextNode("link","",entry);
            el.setAttribute("href","/arrow_left.png"+(session.isEmpty()?"":"?session="+session));
            el.setAttribute("rel","http://opds-spec.org/image");
            el.setAttribute("type","image/jpeg");
        }
        if(listBooks.size()>(nPage+1)*MAX_BOOKS_PER_PAGE){
            QDomElement entry=doc.createElement("entry");
            feed.appendChild(entry);
            AddTextNode("updated",QDateTime::currentDateTimeUtc().toString(Qt::ISODate),entry);
            AddTextNode("id","tag:root",entry);
            AddTextNode("title",tr("Next page"),entry);
            QDomElement el=AddTextNode("link","",entry);
            el.setAttribute("href",lib_url+current_url+QString("?page=%1").arg(QString::number(nPage+1))+parameters);
            el.setAttribute("type","application/atom+xml;profile=opds-catalog");
            el=AddTextNode("link","",entry);
            el.setAttribute("href","/arrow_right.png"+(session.isEmpty()?"":"?session="+session));
            el.setAttribute("rel","http://opds-spec.org/image");
            el.setAttribute("type","image/jpeg");
        }
        return doc.toString();
    }
    else
    {
        QDomElement div=AddTextNode("DIV",sTitle,feed);
        div.setAttribute("class","caption");
        int iBook =0;
        foreach(uint idBook,listBooks){
            if(iBook>=nPage*MAX_BOOKS_PER_PAGE && iBook<(nPage+1)*MAX_BOOKS_PER_PAGE){
                SBook& book = lib.mBooks[idBook];
                QString sIdBook = QString::number(idBook);
                QDomElement entry=doc.createElement("div");
                entry.setAttribute("class","author");
                feed.appendChild(entry);

                QDomElement table=doc.createElement("table");
                table.setAttribute("width","100%");
                entry.appendChild(table);
                QDomElement tr=doc.createElement("tr");
                table.appendChild(tr);
                entry=doc.createElement("td");
                tr.appendChild(entry);

                if(show_covers)
                {
                    QDomElement el=doc.createElement("img");
                    entry.appendChild(el);
                    el.setAttribute("src",lib_url+"/covers/"+ QUrl::toPercentEncoding(sIdBook)+"/cover.jpg");
                    el.setAttribute("class","cover");
                }
                if(bShowAuthor){
                    QDomElement el=AddTextNode("a",lib.mAuthors[book.idFirstAuthor].getName(),entry);
                    el.setAttribute("class","book");
                    el.setAttribute("href",QString("/author/%1").arg(book.idFirstAuthor)+(session.isEmpty()?"":"?session="+session));
                }
                QDomElement el=AddTextNode("div",
                     book.sName+(book.idSerial==0?"":" ("+lib.mSerials[book.idSerial].sName+"["+QString::number(book.numInSerial)+"])"),entry);
                el.setAttribute("class","book");
                QDomElement br=doc.createElement("br");
                entry.appendChild(br);

                if(book.sFormat=="fb2")
                {
                    QDomElement el=AddTextNode("a","fb2",entry);
                    el.setAttribute("href",lib_url+"/book/"+ QUrl::toPercentEncoding(sIdBook)+"/fb2"+(session.isEmpty()?"":"?session="+session));
                    el.setAttribute("class","author");
                    el=AddTextNode("a","epub",entry);
                    el.setAttribute("href",lib_url+"/book/"+ QUrl::toPercentEncoding(sIdBook)+"/epub"+(session.isEmpty()?"":"?session="+session));
                    el.setAttribute("class","author");
                    el=AddTextNode("a","mobi",entry);
                    el.setAttribute("href",lib_url+"/book/"+ QUrl::toPercentEncoding(sIdBook)+"/mobi"+(session.isEmpty()?"":"?session="+session));
                    el.setAttribute("class","author");
                    el=AddTextNode("a","azw3",entry);
                    el.setAttribute("href",lib_url+"/book/"+ QUrl::toPercentEncoding(sIdBook)+"/azw3"+(session.isEmpty()?"":"?session="+session));
                    el.setAttribute("class","author");
                }
                else if(book.sFormat=="epub")
                {
                    QDomElement el=AddTextNode("a","epub",entry);
                    el.setAttribute("href",lib_url+"/book/"+ QUrl::toPercentEncoding(sIdBook)+"/epub"+(session.isEmpty()?"":"?session="+session));
                    el.setAttribute("class","author");
                    el=AddTextNode("a","mobi",entry);
                    el.setAttribute("href",lib_url+"/book/"+ QUrl::toPercentEncoding(sIdBook)+"/mobi"+(session.isEmpty()?"":"?session="+session));
                    el.setAttribute("class","author");
                }
                else if(book.sFormat=="mobi")
                {
                    QDomElement el=AddTextNode("a","mobi",entry);
                    el.setAttribute("href",lib_url+"/book/"+ QUrl::toPercentEncoding(sIdBook)+"/mobi"+(session.isEmpty()?"":"?session="+session));
                    el.setAttribute("class","author");
                }
                else
                {
                    QDomElement el=AddTextNode("a",book.sFormat,entry);
                    el.setAttribute("href",lib_url+"/book/"+ QUrl::toPercentEncoding(sIdBook)+"/download"+(session.isEmpty()?"":"?session="+session));
                    el.setAttribute("class","author");
                }

                if(show_annotation)
                {
                    QBuffer outbuff;
                    QBuffer infobuff;
                    QFileInfo fi_book;
                    fi_book=GetBookFile(outbuff,infobuff,idBook);
                    if(fi_book.suffix().toLower()=="fb2")
                    {
                        if(book.sAnnotation.isEmpty()){
                            book_info book_inf_tmp;
                            GetBookInfo(book_inf_tmp,infobuff.size()==0?outbuff.data():infobuff.data(),"fb2",false,idBook);
                            book.sAnnotation = book_inf_tmp.annotation;
                        }
                        QDomDocument an;
                        an.setContent(QString("<dev>%1</dev>").arg(book.sAnnotation));
                        QDomNode an_node=doc.importNode(an.childNodes().at(0),true);
                        entry.appendChild(an_node);
                    }
                }
            }
            iBook++;
        }
        if(nPage>=1){
            QDomElement entry=doc.createElement("div");
            feed.appendChild(entry);
            QDomElement el=AddTextNode("a",tr("Previous page"),entry);
            el.setAttribute("href",lib_url+current_url+QString("?page=%1").arg(QString::number(nPage-1))+parameters);
            el.setAttribute("class","author");
        }
        if(listBooks.size()>(nPage+1)*MAX_BOOKS_PER_PAGE){
            QDomElement entry=doc.createElement("div");
            feed.appendChild(entry);

            QDomElement el=AddTextNode("a",tr("Next page"),entry);
            el.setAttribute("href",lib_url+current_url+QString("?page=%1").arg(QString::number(nPage+1))+parameters);
            el.setAttribute("class","author");

        }
        QString str;
        QTextStream t(&str,QIODevice::WriteOnly);
        doc.namedItem("HTML").save(t,SAVE_INDEX);
        return str;
    }
    //return "";
}

void opds_server::process(QString url, QTextStream &ts, QString session)
{
    QSettings *settings=GetSettings();
    bool browseDir=settings->value("browseDir",false).toBool();
    //QString result;
    url=QUrl::fromPercentEncoding(url.toLatin1());
    int posQuestion=url.indexOf("?");
    QStringList strings;
    if(posQuestion>=0)
    {
        strings=url.left(posQuestion).split("/");
        strings.last().append(url.right(url.length()-posQuestion));
    }
    else
    {
        strings=url.split("/");
    }
    //qDebug()<<strings;
    int id_lib=idCurrentLib;
    QString lib_url="/http";
    qDebug()<<"url:"<<url;
    QSqlQuery query(QSqlDatabase::database("libdb"));
    bool opds=false;
    params.clear();
    if(strings.count()>1)
    {
        if(strings[1].startsWith("opds"))
        {
            url+="/";
            if(strings[1].startsWith("opds_"))
            {
                id_lib=strings[1].replace("opds_","").toInt();
                lib_url="/opds_"+QString::number(id_lib);
            }
            else
            {
                lib_url="/opds";
                id_lib = idCurrentLib;
            }
            strings.removeFirst();
            url.remove(0,1);
            url=url.right(url.length()-url.indexOf("/"));
            opds=true;
        }
        else if(strings[1].startsWith("http"))
        {
            url+="/";
            if(strings[1].startsWith("http_"))
            {
                id_lib=strings[1].replace("http_","").toInt();
//                query.exec("SELECT name FROM lib WHERE id="+QString::number(id_lib));
//                if(query.next())
//                    libName+=" ("+query.value(0).toString()+")";
                lib_url="/http_"+QString::number(id_lib);
            }
            else
            {
                lib_url="/http";
                id_lib = idCurrentLib;
            }
            strings.removeFirst();
            url.remove(0,1);
            url=url.right(url.length()-url.indexOf("/"));
            opds=false;
        }
    }
    SLib &lib = mLibs[id_lib];
    if(!lib.bLoaded)
        loadLibrary(id_lib);

    uint nPage = 0;
    if(!QFileInfo::exists(url))
    {
        if(strings.count()>0)
        {
            QString last=strings.last();
            int pos=last.indexOf('?');
            if(pos>=0)
            {
                QStringList str_params=last.right(last.length()-pos-1).split('&');
                foreach (QString str, str_params)
                {
                    int pos_eqv=str.indexOf('=');
                    if(pos_eqv>0)
                        params.insert(str.left(pos_eqv),str.right(str.length()-pos_eqv-1));
                    else
                        params.insert(str,"");
                    if(str.left(pos_eqv) == "page")
                        nPage = str.right(str.length()-pos_eqv-1).toUInt();
                }
                strings.last()=strings.last().left(pos);
            }
            pos=url.indexOf('?');
            if(pos>0)
            {
                url=url.left(pos);
            }
        }

        //qDebug()<<"sss";
    }

//    qDebug()<<params;

    QFileInfo fi(url);
    if(url=="/robots.txt" || url=="/robots.txt/")
    {
        ts<<WriteSuccess(OPDS_MIME_TYPE("txt"));
        ts.flush();
        ts<<"User-agent: *"<<char(10);
        ts<<"Disallow: /";
        ts.flush();
    }
    //qDebug()<<fi.fileName();
    if(url.startsWith("/download/",Qt::CaseInsensitive) && browseDir)
    {
        QSettings *settings=GetSettings();
        QString Path=settings->value("dirForBrowsing").toString();
        QString additionalPath=url.mid(QString("/download").length());
        QDir dir(Path+additionalPath);
        QString canonical_path=dir.canonicalPath().toLower();
        if(canonical_path.left(Path.length())==Path.toLower())
        {
            QFile file(Path+additionalPath);
            file.open(QFile::ReadOnly);
            ts<<WriteSuccess(OPDS_MIME_TYPE(QFileInfo(file).suffix()));
            ts.flush();
            ts.device()->write(file.readAll());
        }
    }
    else if((url.endsWith("cover.jpg/",Qt::CaseInsensitive) || url.endsWith("cover.jpg",Qt::CaseInsensitive)) && !QFileInfo::exists(url))
    {
        QString id=strings[2];
        fb2mobi fb;
        QString file=fb.convert(id.toLongLong());
        process(file,ts,session);
    }
    else if(fi.suffix().toLower()=="ico")
    {
        QString ico=QApplication::applicationDirPath()+"/xsl/opds/"+url;
        QFile file(ico);
        file.open(QFile::ReadOnly);
        ts<<WriteSuccess("image/x-icon");
        ts.flush();
        ts.device()->write(file.readAll());
    }
    else if(fi.suffix().toLower()=="png")
    {
        QString ico=QApplication::applicationDirPath()+"/xsl/opds/"+url;
        if(fi.exists())
            ico=fi.absoluteFilePath();
        QFile file(ico);
        file.open(QFile::ReadOnly);
        ts<<WriteSuccess("image/png");
        ts.flush();
        ts.device()->write(file.readAll());
    }
    else if(fi.suffix().toLower()=="jpg")
    {
        //qDebug()<<"!!!-jpg"<<fi.absoluteFilePath();
        QString ico=QApplication::applicationDirPath()+"/xsl/opds/"+url;
        //qDebug()<<ico;
        //if(QFileInfo::exists(fi.absoluteFilePath()))
            ico=fi.absoluteFilePath();
        //qDebug()<<ico;
        QFile file(ico);
        file.open(QFile::ReadOnly);
        ts<<WriteSuccess("image/jpeg");
        ts.flush();
        ts.device()->write(file.readAll());
    }
    else if(url=="/")
    {
        if(opds)
        {
            QDomElement feed=doc_header(session);
            AddTextNode("id","tag:root",feed);
            AddTextNode("title",lib.name,feed);
           // QDateTime td=QDateTime::currentDateTime();
           // td.setTimeSpec(Qt::OffsetFromUTC);
            AddTextNode("updated",QDateTime::currentDateTimeUtc().toString(Qt::ISODate),feed);
            AddTextNode("icon","/icon_256x256.png",feed);

            QDomElement link=doc.createElement("link");
            link.setAttribute("href",lib_url+"/search?search_string={searchTerms}"+(session.isEmpty()?"":"&session="+session));
            link.setAttribute("rel","search");
            link.setAttribute("type","application/atom+xml");
            feed.appendChild(link);

            QDomElement entry;
            QDomElement el;
            if(db_is_open)
            {
                entry=doc.createElement("entry");
                feed.appendChild(entry);
                AddTextNode("updated",QDateTime::currentDateTimeUtc().toString(Qt::ISODate),entry);
                AddTextNode("id","tag:root:authors",entry);
                AddTextNode("title",tr("Books by authors"),entry);
                el=AddTextNode("content",tr("Finding books by authors"),entry);
                el.setAttribute("type","text");
                el=AddTextNode("link","",entry);
                el.setAttribute("href",lib_url+"/authorsindex"+(session.isEmpty()?"":"?session="+session));
                el.setAttribute("type","application/atom+xml;profile=opds-catalog");

                entry=doc.createElement("entry");
                feed.appendChild(entry);
                AddTextNode("updated",QDateTime::currentDateTimeUtc().toString(Qt::ISODate),entry);
                AddTextNode("id","tag:root:sequences",entry);
                AddTextNode("title",tr("Books by sequences"),entry);
                el=AddTextNode("content",tr("Finding books by sequences"),entry);
                el.setAttribute("type","text");
                el=AddTextNode("link","",entry);
                el.setAttribute("href",lib_url+"/sequencesindex"+(session.isEmpty()?"":"?session="+session));
                el.setAttribute("type","application/atom+xml;profile=opds-catalog");

                entry=doc.createElement("entry");
                feed.appendChild(entry);
                AddTextNode("updated",QDateTime::currentDateTimeUtc().toString(Qt::ISODate),entry);
                AddTextNode("id","tag:root:genre",entry);
                AddTextNode("title",tr("Books by genre"),entry);
                el=AddTextNode("content",tr("Finding books by genre"),entry);
                el.setAttribute("type","text");
                el=AddTextNode("link","",entry);
                el.setAttribute("href",lib_url+"/genres"+(session.isEmpty()?"":"?session="+session));
                el.setAttribute("type","application/atom+xml;profile=opds-catalog");
            }
            if(browseDir)
            {
                entry=doc.createElement("entry");
                feed.appendChild(entry);
                AddTextNode("updated",QDateTime::currentDateTimeUtc().toString(Qt::ISODate),entry);
                AddTextNode("id","tag:root:genre",entry);
                AddTextNode("title",tr("Browse directory"),entry);
                el=AddTextNode("content",tr("Finding books by directory"),entry);
                el.setAttribute("type","text");
                el=AddTextNode("link","",entry);
                el.setAttribute("href",lib_url+"/directory"+(session.isEmpty()?"":"?session="+session));
                el.setAttribute("type","application/atom+xml;profile=opds-catalog");
            }

            ts<<WriteSuccess("application/atom+xml;charset=utf-8");
            ts<<doc.toString();
        }
        else
        {
            ts<<WriteSuccess();
            QDomElement feed=doc_header(session,true,lib.name,lib_url);
            QDomElement div=doc.createElement("DIV");
            feed.appendChild(div);
            QDomElement el;
            if(db_is_open)
            {
                el=AddTextNode("A",tr("Finding books by authors"),div);
                el.setAttribute("href",lib_url+"/authorsindex"+(session.isEmpty()?"":"?session="+session));
                div=doc.createElement("DIV");
                feed.appendChild(div);
                el=AddTextNode("A",tr("Finding books by sequences"),div);
                el.setAttribute("href",lib_url+"/sequencesindex"+(session.isEmpty()?"":"?session="+session));
                div=doc.createElement("DIV");
                feed.appendChild(div);
                el=AddTextNode("A",tr("Finding books by genre"),div);
                el.setAttribute("href",lib_url+"/genres"+(session.isEmpty()?"":"?session="+session));
            }
            if(browseDir)
            {
                div=doc.createElement("DIV");
                feed.appendChild(div);
                el=AddTextNode("A",tr("Browse directory"),div);
                el.setAttribute("href",lib_url+"/directory"+(session.isEmpty()?"":"?session="+session));
            }
            if(db_is_open)
            {
                QDomElement hr=doc.createElement("HR");
                hr.setAttribute("size","3");
                hr.setAttribute("color","black");
                feed.appendChild(hr);

                QDomElement form=doc.createElement("FORM");
                form.setAttribute("method","get");
                form.setAttribute("action","search");
                feed.appendChild(form);

                div=doc.createElement("DIV");
                //div.setAttribute("class","book");
                form.appendChild(div);
                el=AddTextNode("div",tr("Finding books by name/author: "),div);
                el.setAttribute("class","book");
                div.appendChild(el);

                el=doc.createElement("INPUT");
                el.setAttribute("type","text");
                el.setAttribute("name","search_string");
                //el.setAttribute("class","book");
                div.appendChild(el);

                el=doc.createElement("INPUT");
                el.setAttribute("type","hidden");
                el.setAttribute("name","session");
                el.setAttribute("value",session);
                div.appendChild(el);

                el=doc.createElement("INPUT");
                el.setAttribute("type","submit");
                el.setAttribute("value",tr("Find"));
                div.appendChild(el);
/*
                hr=doc.createElement("HR");
                hr.setAttribute("size","3");
                hr.setAttribute("color","black");
                feed.appendChild(hr);

                form=doc.createElement("FORM");
                form.setAttribute("method","post");
                form.setAttribute("enctype","multipart/form-data");
                form.setAttribute("action","convert");
                feed.appendChild(form);

                div=doc.createElement("DIV");
                //div.setAttribute("class","book");
                form.appendChild(div);
                el=AddTextNode("div",tr("Convert book: "),div);
                el.setAttribute("class","book");
                div.appendChild(el);

                el=doc.createElement("INPUT");
                el.setAttribute("type","file");
                el.setAttribute("name","book");
                div.appendChild(el);

                el=doc.createElement("INPUT");
                el.setAttribute("type","submit");
                el.setAttribute("value",tr("Send"));
                div.appendChild(el);
*/
            }


            ts<<"<!DOCTYPE html>";
            doc.namedItem("HTML").save(ts,SAVE_INDEX);
        }
    }
    else if(url.startsWith("/convert",Qt::CaseInsensitive) && !opds)
    {
        QDomElement feed;
        feed=doc_header(session,true,lib.name,lib_url);
        ts<<WriteSuccess();
        ts<<"<!DOCTYPE html>";
        doc.namedItem("HTML").save(ts,SAVE_INDEX);
    }
    else if(url.startsWith("/search",Qt::CaseInsensitive))
    {
        if(!params.contains("search_string"))
        {
            process("/",ts,session);
            return;
        }
        if(params.value("search_string").isEmpty())
        {
            process("/",ts,session);
            return;
        }

        //qDebug()<<(params.value("search_string"))<<123;
        //qDebug()<<QUrl::fromPercentEncoding(params.value("search_string").toUtf8())<<1234;
        //ts<<books_list(lib_url,url,id_lib>0?QString::number(id_lib):"","","","",ts,opds,libName,session,false,QString(params.value("search_string")).replace("+"," ").replace("%20"," "));
        QList<uint> listBooks = book_list(lib,0,0,0,QString(params.value("search_string")).replace("+"," ").replace("%20"," "));
        ts<<FillPage(listBooks,lib,tr("Books search"),lib_url, url,ts,opds,nPage,session,true);


    }
    else if(url.startsWith("/sequencebooks",Qt::CaseInsensitive) && strings.count()>=3)
    {
        //QString id_author="";
        QString id_sequence=strings[2];
        //ts<<books_list(lib_url,url,id_lib>0?QString::number(id_lib):"",id_author,id_sequence,"",ts,opds,libName,session,true);
        QList<uint> listBooks = book_list(lib,0,id_sequence.toUInt(),0,"");
        ts<<FillPage(listBooks,lib,tr("Books of sequence")+" ("+lib.mSerials[id_sequence.toUInt()].sName+")",lib_url, url,ts,opds,nPage,session,false);

    }
    else if(url.startsWith("/sequencesindex",Qt::CaseInsensitive))
    {
        QString index="";
        if(strings.count()>2)
            index=strings[2];
        bool by_books=false;
        if(strings.count()>3)
        {
            by_books=strings[3]=="books";
        }
      //  qDebug()<<strings<<index;
        QDomElement feed;
        if(opds)
        {
            feed=doc_header(session);
            AddTextNode("title",tr("Books by sequences"),feed);
            AddTextNode("updated",QDateTime::currentDateTimeUtc().toString(Qt::ISODate),feed);
            AddTextNode("icon","/icon_256x256.png",feed);
            ts<<WriteSuccess("application/atom+xml;charset=utf-8");
        }
        else
        {
            feed=doc_header(session,true,lib.name,lib_url);
            ts<<WriteSuccess();
        }
        auto iSerial = lib.mSerials.constBegin();
        QMap <QString,int> mCount;
        QSet <QString> setSerials;
        int count=0;
        while(iSerial != lib.mSerials.constEnd()){
            if(iSerial->sName.left(index.length()).toLower()==index.toLower()){
                count++;
                QString sNewIndex = iSerial->sName.left(index.length()+1).toLower();
                sNewIndex[0] = sNewIndex[0].toUpper();
                if(mCount.contains(sNewIndex))
                    mCount[sNewIndex]++;
                else
                    mCount[sNewIndex] = 1;
                if(sNewIndex.length()==iSerial->sName.length())
                    setSerials.insert(sNewIndex);
            }
            ++iSerial;
        }
        QList<QString> listKeys = mCount.keys();
        std::sort(listKeys.begin(), listKeys.end(),[](const QString& lhs, const QString& rhs) {return QString::localeAwareCompare(lhs,rhs)<0;});

        if(count>30 && !by_books)
        {
            QDomElement tag_table;
            QDomElement tag_tr;
            int current_column=0;
            if(!opds)
            {
                tag_table=doc.createElement("TABLE");
                feed.appendChild(tag_table);
            }
            foreach(auto iIndex,listKeys)
            {
                if(iIndex.trimmed().isEmpty() || iIndex[0] == '\0')
                    continue;
                if(opds)
                {

                    QDomElement entry=doc.createElement("entry");
                    feed.appendChild(entry);
                    AddTextNode("updated",QDateTime::currentDateTimeUtc().toString(Qt::ISODate),entry);
                    AddTextNode("id","tag:sequences:"+iIndex,entry);
                    AddTextNode("title",iIndex,entry);
                    QDomElement el=AddTextNode("content",QString::number(mCount[iIndex])+" "+tr("series beginning with")+" '"+iIndex+"'",entry);
                    el.setAttribute("type","text");
                    el=AddTextNode("link","",entry);
                    if(mCount[iIndex]==1)
                    {
                        auto iSerial = lib.mSerials.constBegin();
                        while(iSerial != lib.mSerials.constEnd()){
                            if(iSerial.value().sName.left(iIndex.size()).toLower() == iIndex.toLower())
                            {
                                el.setAttribute("href",lib_url+"/sequencebooks/"+ QString::fromLatin1(QUrl::toPercentEncoding(QString::number(iSerial.key())))+(session.isEmpty()?"":"?session="+session));
                                break;
                            }
                            ++iSerial;
                        }

                    }
                    else
                    {
                        el.setAttribute("href",lib_url+"/sequencesindex/"+ QString::fromLatin1(QUrl::toPercentEncoding(iIndex).constData())+
                                        (setSerials.contains(iIndex) ?"/books":"")+(session.isEmpty()?"":"?session="+session));
                    }
                    el.setAttribute("type","application/atom+xml;profile=opds-catalog");
                }
                else
                {

                    if(current_column==0)
                    {
                        tag_tr=doc.createElement("tr");
                        tag_table.appendChild(tag_tr);
                    }
                    QDomElement td=doc.createElement("td");
                    tag_tr.appendChild(td);

                    QDomElement div=doc.createElement("DIV");
                    div.setAttribute("class","author");
                    td.appendChild(div);
                    QDomElement el=AddTextNode("a",iIndex,div);
                    el.setAttribute("class","block");
                    AddTextNode("div",QString::number(mCount[iIndex])+" "+tr("series beginning with")+" '"+iIndex+"'",div);
                    if(mCount[iIndex]==1)
                    {
                        auto iSerial = lib.mSerials.constBegin();
                        while(iSerial != lib.mSerials.constEnd()){
                            if(iSerial.value().sName.left(iIndex.size()).toLower() == iIndex.toLower())
                            {
                                el.setAttribute("href",lib_url+"/sequencebooks/"+ QString::fromLatin1(QUrl::toPercentEncoding(QString::number(iSerial.key())))+(session.isEmpty()?"":"?session="+session));
                                break;
                            }
                            ++iSerial;
                        }

                    }
                    else
                    {
                        el.setAttribute("href",lib_url+"/sequencesindex/"+ QString::fromLatin1(QUrl::toPercentEncoding(iIndex).constData())+
                                        (setSerials.contains(iIndex) ?"/books":"")+(session.isEmpty()?"":"?session="+session));

                    }
                    current_column++;
                    if(current_column==MAX_COLUMN_COUNT)
                        current_column=0;
                }
            }
            if(!opds)
                while(current_column<MAX_COLUMN_COUNT)
                {
                    AddTextNode("td", "",tag_tr);
                    current_column++;
                }
        }
        else
        {
            QList<uint> listSerialId;
            iSerial = lib.mSerials.constBegin();
            while(iSerial != lib.mSerials.constEnd())
            {
                if(iSerial->sName.left((index.length())).toLower() == index.toLower())
                {
                    listSerialId << iSerial.key();
                }
                ++iSerial;
            }
            std::sort(listSerialId.begin(),listSerialId.end(),[lib](const uint& lhs,const uint& rhs) {return lib.mSerials[lhs].sName < lib.mSerials[rhs].sName;});

            foreach(auto iIndex,listSerialId)
            {
                uint nBooksCount=0;
                auto iBook = lib.mBooks.constBegin();
                while(iBook != lib.mBooks.constEnd())
                {
                    if(iBook->idSerial == iIndex)
                        nBooksCount++;
                    ++iBook;
                }
                if(opds)
                {
                    QDomElement entry=doc.createElement("entry");
                    feed.appendChild(entry);
                    AddTextNode("updated",QDateTime::currentDateTimeUtc().toString(Qt::ISODate),entry);
                    AddTextNode("id","tag:sequences:"+QString::number(iIndex),entry);
                    AddTextNode("title",lib.mSerials[iIndex].sName,entry);
                    QDomElement el=AddTextNode("content",QString::number(nBooksCount)+" "+tr("books"),entry);
                    el.setAttribute("type","text");
                    el=AddTextNode("link","",entry);
                    el.setAttribute("href",lib_url+"/sequencebooks/"+ QString::fromLatin1(QUrl::toPercentEncoding(QString::number(iIndex)).constData())+(session.isEmpty()?"":"?session="+session));
                    el.setAttribute("type","application/atom+xml;profile=opds-catalog");
                }
                else
                {
                    QDomElement div=doc.createElement("DIV");
                    div.setAttribute("class","author");
                    feed.appendChild(div);
                    QDomElement el=AddTextNode("a",lib.mSerials[iIndex].sName,div);
                    el.setAttribute("class","block");
                    el.setAttribute("href",lib_url+"/sequencebooks/"+ QString::fromLatin1(QUrl::toPercentEncoding(QString::number(iIndex)).constData())+(session.isEmpty()?"":"?session="+session));
                    AddTextNode("div",QString::number(nBooksCount)+" "+tr("books"),div);
                }
            }
        }
        if(opds)
            ts<<doc.toString();
        else
            doc.namedItem("HTML").save(ts,SAVE_INDEX);
    }
    else if(url.startsWith("/genres",Qt::CaseInsensitive))
    {
        QList<uint> listIdGenres;
        QMap<uint,uint> mCounts;
        if(strings.count()>2)
        {
            uint idParrentGenre = strings[2].toUInt();
            if(mGenre[idParrentGenre].idParrentGenre>0){
                QList<uint> listBooks = book_list(lib,0,0,idParrentGenre,"");
                ts<<FillPage(listBooks,lib,tr("Books by ABC"),lib_url, url,ts,opds,nPage,session,true);
                return;
            }
            auto iGenre = mGenre.constBegin();
            while(iGenre!=mGenre.constEnd()){
                if(iGenre->idParrentGenre==idParrentGenre)
                    listIdGenres << iGenre.key();
                ++iGenre;
            }
            auto iBook = lib.mBooks.constBegin();
            while(iBook!=lib.mBooks.constEnd()){
                foreach (uint iGenre, iBook->listIdGenres){
                    if(mGenre[iGenre].idParrentGenre==idParrentGenre){
                        if(mCounts.contains(iGenre))
                            mCounts[iGenre]++;
                        else
                            mCounts[iGenre] = 1;
                    }
                }
                ++iBook;
            }
        }
        else
        {
            auto iGenre = mGenre.constBegin();
            while(iGenre!=mGenre.constEnd()){
                if(iGenre->idParrentGenre==0)
                    listIdGenres << iGenre.key();
                ++iGenre;
            }
        }
        QDomElement feed;
        if(opds)
        {
            feed=doc_header(session);
            AddTextNode("title",tr("Books by genre"),feed);
            AddTextNode("updated",QDateTime::currentDateTimeUtc().toString(Qt::ISODate),feed);
            AddTextNode("icon","/icon_256x256.png",feed);
            ts<<WriteSuccess("application/atom+xml;charset=utf-8");
        }
        else
        {
            feed=doc_header(session,true,lib.name,lib_url);
            ts<<WriteSuccess();
        }
        foreach(uint idGenre,listIdGenres)
        {
            if(opds)
            {
                QDomElement entry=doc.createElement("entry");
                feed.appendChild(entry);
                AddTextNode("updated",QDateTime::currentDateTimeUtc().toString(Qt::ISODate),entry);
                AddTextNode("id","tag:root:genre:"+mGenre[idGenre].sName ,entry);
                AddTextNode("title",mGenre[idGenre].sName,entry);
                if(strings.count()>2)
                {
                    QDomElement el=AddTextNode("content",QString::number(mCounts[idGenre])/*query.value(2).toString()*/+" "+tr("books"),entry);
                    el.setAttribute("type","text");
                }
                else
                {
                    QDomElement el=AddTextNode("content",tr("Books of genre")+" "+mGenre[idGenre].sName,entry);
                    el.setAttribute("type","text");
                }
                QDomElement el=AddTextNode("link","",entry);
                el.setAttribute("href",lib_url+"/genres/"+ QString::fromLatin1(QUrl::toPercentEncoding(QString::number(idGenre)))+(session.isEmpty()?"":"?session="+session));
                        el.setAttribute("type","application/atom+xml;profile=opds-catalog");

            }
            else
            {
                QDomElement div=doc.createElement("DIV");
                feed.appendChild(div);
                div.setAttribute("class","author");
                QDomElement el=AddTextNode("A",mGenre[idGenre].sName,div);
                el.setAttribute("class","block");
                el.setAttribute("href","/genres/"+QString::number(idGenre)+(session.isEmpty()?"":"?session="+session));
                if(strings.count()>2)
                {
                    QDomElement el=AddTextNode("div",QString::number(mCounts[idGenre])+" "+tr("books"),div);
                }
            }
        }
        if(opds)
            ts<<doc.toString();
        else
            doc.namedItem("HTML").save(ts,SAVE_INDEX);
        qDebug() << doc.toString();
    }
    else if(url.startsWith("/authorsindex",Qt::CaseInsensitive))
    {
        QString index="";
        if(strings.count()>2)
            index=strings[2];
        bool by_books=false;
        if(strings.count()>3)
        {
            by_books=strings[3]=="books";
        }
        QDomElement feed;
        if(opds)
        {
            feed=doc_header(session);
            AddTextNode("title",tr("Books by authors"),feed);
            AddTextNode("updated",QDateTime::currentDateTimeUtc().toString(Qt::ISODate),feed);
            AddTextNode("icon","/icon_256x256.png",feed);
            ts<<WriteSuccess("application/atom+xml;charset=utf-8");
        }
        else
        {
            feed=doc_header(session,true,lib.name,lib_url);
            ts<<WriteSuccess();
        }
        auto iAuthor = lib.mAuthors.constBegin();
        QMap <QString,int> mCount;

        QSet <QString> setAuthors;
        int count=0;
        while(iAuthor != lib.mAuthors.constEnd())
        {
            if(iAuthor->getName().left(index.length()).toLower()==index.toLower())
            {
                count++;
                QString sNewIndex = iAuthor->getName().left(index.length()+1).toLower();
                sNewIndex[0] = sNewIndex[0].toUpper();
                if(mCount.contains(sNewIndex))
                    mCount[sNewIndex]++;
                else
                    mCount[sNewIndex] = 1;
                if(sNewIndex.length()==iAuthor->getName().length())
                    setAuthors.insert(sNewIndex);
            }
            ++iAuthor;
        }
        QList<QString> listKeys = mCount.keys();
        std::sort(listKeys.begin(), listKeys.end(),[](const QString& lhs, const QString& rhs) {return QString::localeAwareCompare(lhs,rhs)<0;});

        if(count>30 && !by_books)
        {
            QDomElement tag_table;
            QDomElement tag_tr;
            int current_column=0;
            if(!opds)
            {
                tag_table=doc.createElement("TABLE");
                feed.appendChild(tag_table);
            }

            foreach(auto iIndex,listKeys)
            {
                if(opds)
                {
                    QDomElement entry=doc.createElement("entry");
                    feed.appendChild(entry);
                    AddTextNode("updated",QDateTime::currentDateTimeUtc().toString(Qt::ISODate),entry);
                    AddTextNode("id","tag:authors:"+iIndex,entry);
                    AddTextNode("title",iIndex,entry);
                    QDomElement el=AddTextNode("content",QString::number(mCount[iIndex])+" "+tr("authors beginning with")+" '"+iIndex+"'",entry);
                    el.setAttribute("type","text");
                    el=AddTextNode("link","",entry);
                    if(mCount[iIndex]==1)
                    {
                        iAuthor = lib.mAuthors.constBegin();
                        while(iAuthor != lib.mAuthors.constEnd())
                        {
                            if(iAuthor.value().getName().left(iIndex.size()).toLower() == iIndex.toLower())
                            {
                                el.setAttribute("href",lib_url+"/author/"+ QString::fromLatin1(QUrl::toPercentEncoding(QString::number(iAuthor.key())))+(session.isEmpty()?"":"?session="+session));
                                break;
                            }
                            ++iAuthor;
                        }
                    }
                    else
                    {
                        el.setAttribute("href",lib_url+"/authorsindex/"+ QString::fromLatin1(QUrl::toPercentEncoding(iIndex).constData())+
                                        (setAuthors.contains(iIndex)?"/books":"")+(session.isEmpty()?"":"?session="+session));

                    }
                    el.setAttribute("type","application/atom+xml;profile=opds-catalog");
                }
                else
                {
                    if(current_column==0)
                    {
                        tag_tr=doc.createElement("tr");
                        tag_table.appendChild(tag_tr);
                    }
                    QDomElement td=doc.createElement("td");
                    tag_tr.appendChild(td);
                    QDomElement div=doc.createElement("div");
                    div.setAttribute("class","author");
                    td.appendChild(div);
                    QDomElement el=AddTextNode("a",iIndex,div);
                    el.setAttribute("class","block");
                    AddTextNode("div",QString::number(mCount[iIndex])+" "+tr("authors beginning with")+" '"+iIndex+"'",div);
                    if(mCount[iIndex]==1)
                    {
                        iAuthor = lib.mAuthors.constBegin();
                        while(iAuthor != lib.mAuthors.constEnd())
                        {
                            if(iAuthor.value().getName().left(iIndex.size()).toLower() == iIndex.toLower())
                            {
                                el.setAttribute("href",lib_url+"/author/"+ QString::fromLatin1(QUrl::toPercentEncoding(QString::number(iAuthor.key())))+(session.isEmpty()?"":"?session="+session));
                                break;
                            }
                            ++iAuthor;
                        }
                    }
                    else
                    {
                        el.setAttribute("href",lib_url+"/authorsindex/"+ QString::fromLatin1(QUrl::toPercentEncoding(iIndex).constData())+
                                        (setAuthors.contains(iIndex)?"/books":"")+(session.isEmpty()?"":"?session="+session));

                    }

                    current_column++;
                    if(current_column==MAX_COLUMN_COUNT)
                        current_column=0;
                }
            }
            if(!opds)
            {
                while(current_column<MAX_COLUMN_COUNT)
                {
                    AddTextNode("td", "",tag_tr);
                    current_column++;
                }
            }
        }
        else
        {
            QList<uint> listAuthorId;
            iAuthor =lib.mAuthors.constBegin();
            while(iAuthor != lib.mAuthors.constEnd())
            {
                if(iAuthor->getName().left(index.length()).toLower()==index.toLower())
                {
                    listAuthorId << iAuthor.key();
                }
                ++iAuthor;
            }
            std::sort(listAuthorId.begin(),listAuthorId.end(),[lib](const uint& lhs,const uint& rhs) {return lib.mAuthors[lhs].getName() < lib.mAuthors[rhs].getName();});
            foreach(auto iIndex,listAuthorId)
            {
                uint nBooksCount=0;
                QMultiHash<uint,uint>::const_iterator i = lib.mAuthorBooksLink.constFind(iIndex);
                while(i!=lib.mAuthorBooksLink.constEnd() && i.key()==iIndex){
                    nBooksCount++;
                    ++i;
                }

                if(opds)
                {
                    QDomElement entry=doc.createElement("entry");
                    feed.appendChild(entry);
                    AddTextNode("updated",QDateTime::currentDateTimeUtc().toString(Qt::ISODate),entry);
                    AddTextNode("id","tag:author:"+QString::number(iIndex),entry);
                    AddTextNode("title",lib.mAuthors[iIndex].getName(),entry);
                    QDomElement el=AddTextNode("content",QString::number(nBooksCount)+" "+tr("books"),entry);
                    el.setAttribute("type","text");
                    el=AddTextNode("link","",entry);
                    el.setAttribute("href",lib_url+"/author/"+ QString::fromLatin1(QUrl::toPercentEncoding(QString::number(iIndex)).constData())+(session.isEmpty()?"":"?session="+session));
                    el.setAttribute("type","application/atom+xml;profile=opds-catalog");
                }
                else
                {
                    QDomElement div=doc.createElement("DIV");
                    div.setAttribute("class","author");
                    feed.appendChild(div);
                    QDomElement el=AddTextNode("a",lib.mAuthors[iIndex].getName(),div);
                    el.setAttribute("class","block");
                    el.setAttribute("href",lib_url+"/author/"+ QString::fromLatin1(QUrl::toPercentEncoding(QString::number(iIndex)).constData())+(session.isEmpty()?"":"?session="+session));
                    AddTextNode("div",QString::number(nBooksCount)+" "+tr("books"),div);
                }
            }

        }

        if(opds)
            ts<<doc.toString();
        else
            doc.namedItem("HTML").save(ts,SAVE_INDEX);
    }
    else if(url.startsWith("/authorsequenceless/",Qt::CaseInsensitive) && strings.count()>=3)
    {
        uint idAuthor = strings[2].toUInt();

        QList<uint> listBooks = book_list(lib,idAuthor,0,0,"",true);
        ts<<FillPage(listBooks,lib,tr("Books without sequence")+" ("+lib.mAuthors[idAuthor].getName()+")",lib_url, url,ts,opds,nPage,session,false);

    }
    else if(url.startsWith("/authorbooks/",Qt::CaseInsensitive) && strings.count()>=3)
    {
        uint idAuthor = strings[2].toUInt();
        QList<uint> listBooks = book_list(lib,idAuthor,0,0,"",false);
        ts<<FillPage(listBooks,lib,tr("Books by ABC")+" ("+lib.mAuthors[idAuthor].getName()+")",lib_url, url,ts,opds,nPage,session,false);

    }
    else if(url.startsWith("/authorsequences/",Qt::CaseInsensitive) && strings.count()>=3)
    {
        QString id=strings[2];
        uint idAuthor = id.toUInt();
        QDomElement feed;
        if(opds)
        {
            feed=doc_header(session);
            AddTextNode("id","tag:author:"+query.value(0).toString(),feed);
            AddTextNode("title",tr("Book sequences")+" "+lib.mAuthors[idAuthor].getName(),feed);
            AddTextNode("updated",QDateTime::currentDateTimeUtc().toString(Qt::ISODate),feed);
            AddTextNode("icon","/icon_256x256.png",feed);
        }
        else
        {
             feed=doc_header(session,true,lib.name,lib_url);
             QDomElement div=AddTextNode("DIV",tr("Book sequences")+" "+lib.mAuthors[idAuthor].getName(),feed);
             div.setAttribute("class","caption");
        }

        QMultiHash<uint,uint>::const_iterator i = lib.mAuthorBooksLink.constFind(idAuthor);
        QMap<uint,uint> mapCountBooks;
        while(i!=lib.mAuthorBooksLink.constEnd() && i.key()==idAuthor){
            SBook& book = lib.mBooks[i.value()];
            if(book.idSerial>0){
                if(mapCountBooks.contains(book.idSerial))
                    mapCountBooks[book.idSerial]++;
                        else
                    mapCountBooks[book.idSerial]=1;
            }
            ++i;
        }
        QList<uint> listSerials = mapCountBooks.keys();
        std::sort(listSerials.begin(),listSerials.end(),[lib](const uint& lhs,const uint& rhs) {return QString::localeAwareCompare(lib.mSerials[lhs].sName, lib.mSerials[rhs].sName) < 0;});

        foreach(uint idSerial,listSerials)
        {
            if(opds)
            {
                QDomElement entry=doc.createElement("entry");
                feed.appendChild(entry);
                AddTextNode("updated",QDateTime::currentDateTimeUtc().toString(Qt::ISODate),entry);
                AddTextNode("id","tag:author:"+id+":sequence:"+QString::number(idSerial),entry);
                AddTextNode("title",lib.mSerials[idSerial].sName,entry);
                QDomElement el=AddTextNode("content",QString::number(mapCountBooks[idSerial])+" "+tr("books in sequence"),entry);
                el.setAttribute("type","text");
                el=AddTextNode("link","",entry);
                el.setAttribute("href",lib_url+"/authorsequence/"+ QString::fromLatin1(QUrl::toPercentEncoding(id))+"/"+QString::fromLatin1(QUrl::toPercentEncoding(QString::number(idSerial)))+(session.isEmpty()?"":"?session="+session));
                el.setAttribute("type","application/atom+xml;profile=opds-catalog");
            }
            else
            {
                QDomElement entry=doc.createElement("div");
                entry.setAttribute("class","author");
                feed.appendChild(entry);
                QDomElement el=AddTextNode("a",lib.mSerials[idSerial].sName,entry);
                el.setAttribute("class","block");
                el.setAttribute("href",lib_url+"/authorsequence/"+ QString::fromLatin1(QUrl::toPercentEncoding(id))+"/"+QString::fromLatin1(QUrl::toPercentEncoding(QString::number(idSerial)))+(session.isEmpty()?"":"?session="+session));
                AddTextNode("div",QString::number(mapCountBooks[idSerial])+" "+tr("books in sequence"),entry);
            }
        }
        if(opds)
        {
            ts<<WriteSuccess("application/atom+xml;charset=utf-8");
            ts<<doc.toString();
        }
        else
        {
            ts<<WriteSuccess();
            doc.namedItem("HTML").save(ts,SAVE_INDEX);
        }

    }
    else if(url.startsWith("/authorsequence/",Qt::CaseInsensitive) && strings.count()>=4)
    {
        QString id_author=strings[2];
        QString id_sequence=strings[3];
        uint idSequence = id_sequence.toUInt();
        QList<uint> listBooks = book_list(lib,id_author.toUInt(),idSequence,0,"");
        ts<<FillPage(listBooks,lib,tr("Books of sequence")+" ("+lib.mSerials[idSequence].sName +")",lib_url, url,ts,opds,nPage,session,false);
    }
    else if(url.startsWith("/author/",Qt::CaseInsensitive) && strings.count()>=3)
    {
        QString sIdAuthor = strings[2];
        uint idAuthor = sIdAuthor.toUInt();
        QString sAuthor = lib.mAuthors[idAuthor].getName();
        if(opds)
        {
            QDomElement feed=doc_header(session);
            AddTextNode("id","tag:author:"+query.value(0).toString(),feed);
            AddTextNode("title",tr("Books by")+" "+sAuthor,feed);
            AddTextNode("updated",QDateTime::currentDateTimeUtc().toString(Qt::ISODate),feed);
            AddTextNode("icon","/icon_256x256.png"+(session.isEmpty()?"":"?session="+session),feed);

            QDomElement entry=doc.createElement("entry");
            feed.appendChild(entry);
            AddTextNode("updated",QDateTime::currentDateTimeUtc().toString(Qt::ISODate),entry);
            AddTextNode("id","tag:author:"+sIdAuthor+":sequences",entry);
            AddTextNode("title",tr("Books by sequences"),entry);
            QDomElement el=AddTextNode("link","",entry);
            el.setAttribute("href",lib_url+"/authorsequences/"+ QString::fromLatin1(QUrl::toPercentEncoding(sIdAuthor).constData())+(session.isEmpty()?"":"?session="+session));
            el.setAttribute("type","application/atom+xml;profile=opds-catalog");

            entry=doc.createElement("entry");
            feed.appendChild(entry);
            AddTextNode("updated",QDateTime::currentDateTimeUtc().toString(Qt::ISODate),entry);
            AddTextNode("id","tag:author:"+sIdAuthor+":sequenceless",entry);
            AddTextNode("title",tr("Books without sequence"),entry);
            el=AddTextNode("link","",entry);
            el.setAttribute("href",lib_url+"/authorsequenceless/"+ QString::fromLatin1(QUrl::toPercentEncoding(sIdAuthor).constData())+(session.isEmpty()?"":"?session="+session));
            el.setAttribute("type","application/atom+xml;profile=opds-catalog");

            entry=doc.createElement("entry");
            feed.appendChild(entry);
            AddTextNode("updated",QDateTime::currentDateTimeUtc().toString(Qt::ISODate),entry);
            AddTextNode("id","tag:author:"+sIdAuthor+":sequences",entry);
            AddTextNode("title",tr("All books"),entry);
            el=AddTextNode("link","",entry);
            el.setAttribute("href",lib_url+"/authorbooks/"+ QString::fromLatin1(QUrl::toPercentEncoding(sIdAuthor).constData())+(session.isEmpty()?"":"?session="+session));
            el.setAttribute("type","application/atom+xml;profile=opds-catalog");

            ts<<WriteSuccess("application/atom+xml;charset=utf-8");
            ts<<doc.toString();
        }
        else
        {
            QDomElement feed=doc_header(session,true,lib.name,lib_url);
            QDomElement div_auth=doc.createElement("DIV");;
            div_auth.setAttribute("class","author");
            feed.appendChild(div_auth);
            QDomElement div_caption=AddTextNode("div",tr("Books by")+" "+sAuthor,div_auth);
            div_caption.setAttribute("class","caption");

            QDomElement div=doc.createElement("DIV");
            div_auth.appendChild(div);
            QDomElement el=AddTextNode("a",tr("Books by sequences"),div);
            el.setAttribute("class","block");
            el.setAttribute("href",lib_url+"/authorsequences/"+ QString::fromLatin1(QUrl::toPercentEncoding(sIdAuthor).constData())+(session.isEmpty()?"":"?session="+session));

            div=doc.createElement("DIV");
            div_auth.appendChild(div);
            el=AddTextNode("a",tr("Books without sequence"),div);
            el.setAttribute("class","block");
            el.setAttribute("href",lib_url+"/authorsequenceless/"+ QString::fromLatin1(QUrl::toPercentEncoding(sIdAuthor).constData())+(session.isEmpty()?"":"?session="+session));


            div=doc.createElement("DIV");
            div_auth.appendChild(div);
            el=AddTextNode("a",tr("All books"),div);
            el.setAttribute("class","block");
            el.setAttribute("href",lib_url+"/authorbooks/"+ QString::fromLatin1(QUrl::toPercentEncoding(sIdAuthor).constData())+(session.isEmpty()?"":"?session="+session));

            ts<<WriteSuccess();
            doc.namedItem("HTML").save(ts,SAVE_INDEX);
        }
    }
    else if(url.startsWith("/book/",Qt::CaseInsensitive) && strings.count()>=4)
    {
        QString id=strings[2];
        uint idBook = id.toUInt();
        QString format=strings[3];
        convert(lib,idBook,format,"",opds,ts);
    }
    else if(url.startsWith("/directory",Qt::CaseInsensitive) && browseDir)
    {
        QSettings *settings=GetSettings();
        QString Path=settings->value("dirForBrowsing").toString();
        QString additionalPath=url.mid(QString("/directory").length());
        if(opds)
        {
            QDomElement feed=doc_header(session);
            AddTextNode("id","tag:directory",feed);
            AddTextNode("title",tr("Browse directory"),feed);
            AddTextNode("updated",QDateTime::currentDateTimeUtc().toString(Qt::ISODate),feed);
            AddTextNode("icon","/icon_256x256.png"+(session.isEmpty()?"":"?session="+session),feed);
            QDir dir(Path+additionalPath);
            QString canonical_path=dir.canonicalPath().toLower();
            if(canonical_path.left(Path.length())==Path.toLower())
            {
                QFileInfoList files=dir.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Readable,QDir::DirsFirst|QDir::Name|QDir::IgnoreCase);
                QDomElement entry,el;
                foreach (QFileInfo file, files)
                {
                    if(file.isDir())
                    {
                        entry=doc.createElement("entry");
                        feed.appendChild(entry);
                        AddTextNode("updated",QDateTime::currentDateTimeUtc().toString(Qt::ISODate),entry);
                        AddTextNode("id","tag:directory:"+file.fileName(),entry);
                        AddTextNode("title","/"+file.fileName(),entry);
                        el=AddTextNode("link","",entry);
                        el.setAttribute("href","/opds/directory"+additionalPath+file.fileName()+(session.isEmpty()?"":"?session="+session));
                        el.setAttribute("type","application/atom+xml;profile=opds-catalog");
                    }
                    else
                    {
                        entry=doc.createElement("entry");
                        feed.appendChild(entry);
                        AddTextNode("updated",QDateTime::currentDateTimeUtc().toString(Qt::ISODate),entry);
                        AddTextNode("id","tag:book:"+file.fileName(),entry);
                        AddTextNode("title",file.fileName(),entry);

                        if(file.suffix().toLower()=="fb2")
                        {
                            el=AddTextNode("link","",entry);
                            el.setAttribute("href","/download"+additionalPath+file.fileName()+(session.isEmpty()?"":"?session="+session));
                            el.setAttribute("rel","http://opds-spec.org/acquisition/open-access");
                            el.setAttribute("type","application/fb2+zip");
                            el=AddTextNode("link","",entry);
                            el.setAttribute("href","/convert/epub"+additionalPath+file.fileName()+(session.isEmpty()?"":"?session="+session));
                            el.setAttribute("rel","http://opds-spec.org/acquisition/open-access");
                            el.setAttribute("type","application/epub+zip");
                            el=AddTextNode("link","",entry);
                            el.setAttribute("href","/convert/mobi"+additionalPath+file.fileName()+(session.isEmpty()?"":"?session="+session));
                            el.setAttribute("rel","http://opds-spec.org/acquisition/open-access");
                            el.setAttribute("type","application/x-mobipocket-ebook");
                        }
                        if(file.suffix().toLower()=="epub")
                        {
                            el=AddTextNode("link","",entry);
                            el.setAttribute("href","/download"+additionalPath+file.fileName()+(session.isEmpty()?"":"?session="+session));
                            el.setAttribute("rel","http://opds-spec.org/acquisition/open-access");
                            el.setAttribute("type","application/epub+zip");
                            el=AddTextNode("link","",entry);
                            el.setAttribute("href","/convert/mobi"+additionalPath+file.fileName()+(session.isEmpty()?"":"?session="+session));
                            el.setAttribute("rel","http://opds-spec.org/acquisition/open-access");
                            el.setAttribute("type","application/x-mobipocket-ebook");
                        }
                        if(file.suffix().toLower()=="mobi")
                        {
                            el=AddTextNode("link","",entry);
                            el.setAttribute("href","/download"+additionalPath+file.fileName()+(session.isEmpty()?"":"?session="+session));
                            el.setAttribute("rel","http://opds-spec.org/acquisition/open-access");
                            el.setAttribute("type","application/x-mobipocket-ebook");
                        }
                        if(!(file.suffix().toLower()=="fb2" || file.suffix().toLower()=="mobi" || file.suffix().toLower()=="epub"))
                        {
                            QDomElement el=AddTextNode("link","",entry);
                            el.setAttribute("href",lib_url+"/download/"+ additionalPath+file.fileName()+(session.isEmpty()?"":"?session="+session));
                            el.setAttribute("rel","http://opds-spec.org/acquisition/open-access");
                            el.setAttribute("type","application/"+file.suffix());
                        }
                        QDomElement el=AddTextNode("link","",entry);
                        el.setAttribute("href",lib_url+"/download/"+ additionalPath+file.fileName()+(session.isEmpty()?"":"?session="+session));// /"+query.value(2).toString()+"."+query.value(3).toString());
                        el.setAttribute("rel","alternate");
                        el.setAttribute("type","text/html");
                        el.setAttribute("title",tr("Download"));
                    }
                }
            }
            ts<<WriteSuccess("application/atom+xml;charset=utf-8");
            ts<<doc.toString();
        }
        else
        {
            QDomElement feed=doc_header(session,true,lib.name,lib_url);
            QDomElement div_auth=doc.createElement("DIV");;
            div_auth.setAttribute("class","author");
            feed.appendChild(div_auth);
            QDomElement div_caption=AddTextNode("div",tr("Browse directory"),div_auth);
            div_caption.setAttribute("class","caption");
            AddTextNode("div",additionalPath,div_auth);
            QDir dir(Path+additionalPath);
            QString canonical_path=dir.canonicalPath().toLower();
            if(canonical_path.left(Path.length())==Path.toLower())
            {
                QFileInfoList files=dir.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Readable,QDir::DirsFirst|QDir::Name|QDir::IgnoreCase);
                foreach (QFileInfo file, files)
                {
                    QDomElement entry=doc.createElement("div");
                    entry.setAttribute("class","author");
                    feed.appendChild(entry);
                    if(file.isDir())
                    {
                        QDomElement el=AddTextNode("a","/"+file.fileName(),entry);
                        el.setAttribute("class","block");
                        el.setAttribute("href","/directory"+additionalPath+"/"+file.fileName()+(session.isEmpty()?"":"?session="+session));
                    }
                    else
                    {
                        QDomElement el=AddTextNode("a",file.fileName(),entry);
                        el.setAttribute("class","");
                        el.setAttribute("href","/download"+additionalPath+"/"+file.fileName()+(session.isEmpty()?"":"?session="+session));
                        if(file.suffix().toLower()=="fb2" || file.suffix().toLower()=="epub")
                        {
                            QDomElement el_b=doc.createElement("b");
                            entry.appendChild(el_b);

                            el=AddTextNode("a"," [mobi]",el_b);
                            el.setAttribute("class","");
                            el.setAttribute("href","/convert/mobi"+additionalPath+"/"+file.fileName()+(session.isEmpty()?"":"?session="+session));

                            if(file.suffix().toLower()!="epub")
                            {
                                el=AddTextNode("a"," [epub]",el_b);
                                el.setAttribute("class","");
                                el.setAttribute("href","/convert/epub"+additionalPath+"/"+file.fileName()+(session.isEmpty()?"":"?session="+session));
                            }

                            el=AddTextNode("a"," [azw3]",el_b);
                            el.setAttribute("class","");
                            el.setAttribute("href","/convert/azw3"+additionalPath+"/"+file.fileName()+(session.isEmpty()?"":"?session="+session));
                        }
                    }
                }
            }

            ts<<WriteSuccess();
            doc.namedItem("HTML").save(ts,SAVE_INDEX);
        }
    }
    else if(url.startsWith("/convert/",Qt::CaseInsensitive) && browseDir)
    {
        QSettings *settings=GetSettings();
        QString Path=settings->value("dirForBrowsing").toString();
        QString additionalPath=url.mid(QString("/convert/").length());
        QString format=additionalPath.left(additionalPath.indexOf('/'));
        additionalPath=additionalPath.mid(format.length());
        QDir dir(Path+additionalPath);
        QString canonical_path=dir.canonicalPath().toLower();
        if(canonical_path.left(Path.length())==Path.toLower())
        {
            //qDebug()<<format<<Path+additionalPath;
            convert(lib,0,format,Path+additionalPath,opds,ts);
        }
    }
    // return result;
}

QString opds_server::WriteSuccess(QString contentType, bool isGZip)
{
    return QString("HTTP/1.1 200 OK\n")+
            "Server: freeLib "+PROG_VERSION+"\n"+
            "Content-Type: " + contentType + "\n"+
            "Pragma: no-cache\n"+
            "Accept-Ranges: bytes\n"+
            (isGZip?"Content-Encoding: gzip\n":"")+
//            "Set-Cookie: PWDCHECK=1;\n"+
//            "Connection: close\n\n";
            "Connection: Keep-Alive\n\n";
}

void opds_server::stop_server()
{
    if(OPDS_server_status==1)
    {
        foreach(int i,OPDS_clients.keys())
        {
              //  QTextStream os(OPDS_clients[i]);
              //  os.setAutoDetectUnicode(true);
              //  os << QDateTime::currentDateTime().toString() << "\n";
                OPDS_clients[i]->close();
                OPDS_clients.remove(i);
        }
        OPDS_server.close();
        OPDS_server_status=0;
    }
}

void opds_server::server_run(int _port)
{
    QSettings *settings=GetSettings();
    int new_port=settings->value("OPDS_port",default_OPDS_port).toInt();
    if(_port!=-1)
        new_port=_port;
    if(new_port!=port && OPDS_server_status==1)
    {
        stop_server();
    }
    QNetworkProxy OPDSproxy;
    OPDSproxy.setType(QNetworkProxy::NoProxy);
    OPDS_server.setProxy(OPDSproxy);
    port=new_port;
    if(settings->value("OPDS_enable",false).toBool())
    {
        if(OPDS_server_status==0)
        {
            if(!OPDS_server.listen(QHostAddress::Any,port))
                qDebug()<<QString("Unable to start the server: %1.").arg(OPDS_server.errorString());
            else
                OPDS_server_status=1;
        }
    }
    else
    {
        stop_server();
    }
    //delete settings;
}
#define session_number_len  16

void opds_server::slotRead()
{
    QTcpSocket* clientSocket = dynamic_cast<QTcpSocket*>(sender());
    if(clientSocket->bytesAvailable()==0)
        return;
    qintptr idusersocs=clientSocket->socketDescriptor();
    QStringList TCPtokens;
    QStringList AUTHtokens;
    for_preview=false;
    for_mobile=false;
    //bool PWDCHECK=false;
    while(clientSocket->bytesAvailable()>0)
    {
        QString str=clientSocket->readLine();
        QStringList tokens = QString(str).split(QRegExp("[ \r\n][ \r\n]*"));
        if (tokens[0] == "GET" || tokens[0] == "POST")
            TCPtokens=tokens;
        if (tokens[0] == "Authorization:")
            AUTHtokens=tokens;
        if(tokens[0] == "X-Purpose:")
            for_preview=(tokens[1]=="preview");
        if(tokens[0] == "User-Agent:")
            for_mobile=(tokens.contains("Mobile",Qt::CaseInsensitive));
//        if(tokens[0] == "Cookie:")
//            for_mobile=(tokens.contains("PWDCHECK=1",Qt::CaseInsensitive));
//        qDebug()<<tokens;
    }
    if (TCPtokens.count()>0)
    {
        QSettings *settings=GetSettings();
        QTextStream os(clientSocket);
        os.setAutoDetectUnicode(true);
        bool auth=true;
        QString session;
        if(settings->value("HTTP_need_pasword",false).toBool())
        {
            auth=false;
            if(AUTHtokens.count()>2)
            {
                QByteArray ba;
                ba.append(AUTHtokens[2]);
                QStringList auth_str=QString(QByteArray::fromBase64(ba)).split(":");
                if(auth_str.count()==2)
                {
                    auth=(settings->value("HTTP_user").toString()==auth_str[0] && settings->value("HTTP_password").toString()==auth_str[1]);
                }
            }
            if(TCPtokens[1].contains("session=") && !auth)
            {
                int pos=TCPtokens[1].indexOf("session=");
                session=TCPtokens[1].mid(pos+8,session_number_len);
               // qDebug()<<sessions;
                auth=sessions.contains(session);
                if(auth)
                {
                    sessions[session]=QDateTime::currentDateTime();
                }
            }
            else
            {
                if(auth)
                {
                    //генерируем номер сессии
                    QString chars = "abdefhiknrstyzABDEFGHKNQRSTYZ23456789";
                    session="";
                    for(int i=0;i<session_number_len;i++)
                        session+=chars[rand()%32];
                    sessions.insert(session,QDateTime::currentDateTime());
                }
            }
            foreach (QString key, sessions.keys())
            {
                if(sessions.value(key)<QDateTime::currentDateTime().addSecs(-60*60))
                    sessions.remove(key);
            }
        }
        //qDebug()<<auth;
        if(auth)
        {
            process(TCPtokens[1],os,session);
        }
        else
        {
            os<<QString("HTTP/1.1 401 Authorization Required\n")+
            "WWW-Authenticate: Basic\n"+
            "Content-Type: text/html;charset=utf-8\n"+
            "Connection: close\n\n";
        }
        os.flush();
        clientSocket->flush();
    }
    clientSocket->close();
    OPDS_clients.remove(idusersocs);
}

void opds_server::new_connection()
{
    if(OPDS_server_status==1)
    {
        QTcpSocket* clientSocket=OPDS_server.nextPendingConnection();
        int idusersocs=clientSocket->socketDescriptor();
        OPDS_clients[idusersocs]=clientSocket;
        connect(OPDS_clients[idusersocs],SIGNAL(readyRead()),this, SLOT(slotRead()));
    }
}

void opds_server::convert(SLib &lib, uint idBook, QString format, QString file_name, bool opds, QTextStream &ts)
{
    QBuffer outbuff;
    QBuffer infobuff;
    QFileInfo fi_book;
    if(idBook==0)
    {
        fi_book.setFile(file_name);
        QFile file(file_name);
        file.open(QFile::ReadOnly);
        outbuff.setData(file.readAll());
    }
    else
    {
        fi_book=GetBookFile(outbuff,infobuff,idBook);
    }

    if(outbuff.size()!=0)
    {
        QSettings *settings=GetSettings();
        QString book_file_name=settings->value("ExportFileName",default_exp_file_name).toString().trimmed();
        if(book_file_name.isEmpty())
            book_file_name=default_exp_file_name;
        SBook& book = lib.mBooks[idBook];
        book_file_name=fillParams(book_file_name,book)+"."+(format=="download"?fi_book.suffix().toLower():format);
        if(settings->value("transliteration",false).toBool())
            book_file_name=Transliteration(book_file_name);
        book_file_name.replace(" ","_");
        book_file_name.replace("\"","_");
        book_file_name.replace("'","_");
        book_file_name.replace(",","_");
        book_file_name.replace("__","_");
        //book_file_name.replace("\\","_");
        //book_file_name.replace("/","_");
        QFileInfo book_file(book_file_name);
        book_file_name=book_file.fileName();
        if(settings->value("originalFileName",false).toBool())
            book_file_name=fi_book.completeBaseName()+"."+format;;
        if(format=="epub" || format=="mobi" || format=="azw3")
        {
            QFile file;
            QString tmp_dir;
            if(QStandardPaths::standardLocations(QStandardPaths::TempLocation).count()>0)
                tmp_dir=QStandardPaths::standardLocations(QStandardPaths::TempLocation).at(0);
            QDir().mkpath(tmp_dir+"/freeLib");
            file.setFileName(tmp_dir+"/freeLib/book0."+fi_book.suffix());
            file.open(QFile::WriteOnly);
            file.write(outbuff.data());
            file.close();
            fb2mobi conv;
            QSettings* settings=GetSettings();
            int http_settings=settings->value("httpExport",0).toInt()-1;
            if(http_settings==-1)
            {
                int count=settings->beginReadArray("export");
                for(int i=0;i<count;i++)
                {
                    settings->setArrayIndex(i);
                    if(settings->value("Default").toBool())
                    {
                        http_settings=i;
                        break;
                    }
                }
                settings->endArray();
            }
            SetCurrentExportSettings(http_settings);

            // Не удалять
            //QString out_file=conv.convert(QStringList()<<fi.absoluteFilePath(),false,format.toUpper(),bi);
            //qDebug()<<out_file;
            //file.setFileName(out_file);
            file.open(QFile::ReadOnly);
            outbuff.close();
            outbuff.setData(file.readAll());
            if(opds)
            {
                if(format=="epub")
                    ts<<WriteSuccess("application/epub+zip");
                else
                    ts<<WriteSuccess("application/x-mobipocket-ebook");
            }
            else
            {
                if(format=="epub")
                    ts<<WriteSuccess("application/epub+zip\nContent-Disposition: attachment; filename=\""+book_file_name+"\"");
                else if(format=="mobi")
                    ts<<WriteSuccess("application/x-mobipocket-ebook\nContent-Disposition: attachment; filename=\""+book_file_name+"\"");
                else
                    ts<<WriteSuccess("text/plain; charset=UTF-8\nContent-Disposition: attachment; filename=\""+book_file_name+"\"");
            }
        }
        else
        {
            ts<<WriteSuccess("text/plain; charset=UTF-8\nContent-Disposition: attachment; filename=\""+book_file_name+"\"");
        }
        ts.flush();
        ts.device()->write(outbuff.data());
    }
}
