#define QT_USE_QSTRINGBUILDER
#include "opds_server.h"

#include <algorithm>
#include <QSettings>
#include <QTcpSocket>
#include <QNetworkProxy>
#include <QStringBuilder>
#include <QDir>
#include <QRegularExpression>

#include "config-freelib.h"
#include "fb2mobi/fb2mobi.h"
#include "utilites.h"

#define SAVE_INDEX  4
#define MAX_COLUMN_COUNT    3

opds_server::opds_server(QObject *parent) :
    QObject(parent)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
    httpServer_.route("/", QHttpServerRequest::Method::Get, [this](const QHttpServerRequest &request)
    {
        return rootHTTP(0, request);
    });

    httpServer_.route("/http", QHttpServerRequest::Method::Get, [this](const QHttpServerRequest &request)
    {
        return rootHTTP(0, request);
    });

    httpServer_.route("/opds", QHttpServerRequest::Method::Get, [this](const QHttpServerRequest &request)
    { return rootOPDS(0, request); });

    httpServer_.route("/http_<arg>", QHttpServerRequest::Method::Get, [this](uint idLib, const QHttpServerRequest &request)
    {
        return rootHTTP(idLib, request);
    });

    httpServer_.route("/opds_<arg>", QHttpServerRequest::Method::Get, [this](uint idLib, const QHttpServerRequest &request)
    { return rootOPDS(idLib, request); });

    httpServer_.route("/<arg>.png", QHttpServerRequest::Method::Get, [this](const QString &sUrl/*, const QHttpServerRequest &request*/)
    {
        QByteArray ba = image(sUrl + u".png"_s);
        return QHttpServerResponse(ba);
    });

    httpServer_.route("/http_<arg>/covers/<arg>/cover.jpg", QHttpServerRequest::Method::Get,  [this](uint idLib, uint idBook/*, const QHttpServerRequest &request*/)
    {
        QByteArray ba = cover(idLib, idBook);
        return QHttpServerResponse(ba);
    });

    httpServer_.route("/opds_<arg>/covers/<arg>/cover.jpg", QHttpServerRequest::Method::Get,  [this](uint idLib, uint idBook/*, const QHttpServerRequest &request*/)
    {
        QByteArray ba = cover(idLib, idBook);
        return QHttpServerResponse(ba);
    });

    httpServer_.route("/http_<arg>/authorsindex", QHttpServerRequest::Method::Get,  [this](uint idLib, const QHttpServerRequest &request)
    {
        return authorsIndexHTTP(idLib, u""_s, false, request);
    });

    httpServer_.route("/opds_<arg>/authorsindex", QHttpServerRequest::Method::Get,  [this](uint idLib, const QHttpServerRequest &request)
    { return authorsIndexOPDS(idLib, u""_s, false, request); });

    httpServer_.route("/http_<arg>/authorsindex/<arg>", QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return authorsIndexHTTP(idLib, sIndex, false, request); });

    httpServer_.route("/opds_<arg>/authorsindex/<arg>", QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return authorsIndexOPDS(idLib, sIndex, false, request); });

    httpServer_.route("/http_<arg>/authorsindex/<arg>/books", QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return authorsIndexHTTP(idLib, sIndex, true, request); });

    httpServer_.route("/opds_<arg>/authorsindex/<arg>/books", QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return authorsIndexOPDS(idLib, sIndex, true, request); });

    httpServer_.route("/http_<arg>/author/<arg>", QHttpServerRequest::Method::Get,  [this](uint idLib, uint idAuthor, const QHttpServerRequest &request)
    { return authorHTTP(idLib, idAuthor, request); });

    httpServer_.route("/opds_<arg>/author/<arg>", QHttpServerRequest::Method::Get,  [this](uint idLib, uint idAuthor, const QHttpServerRequest &request)
    { return authorOPDS(idLib, idAuthor, request); });

    httpServer_.route("/http_<arg>/authorbooks/<arg>", QHttpServerRequest::Method::Get,  [this](uint idLib, uint idAuthor, const QHttpServerRequest &request)
    { return authorBooksHTTP(idLib, idAuthor, request); });

    httpServer_.route("/opds_<arg>/authorbooks/<arg>", QHttpServerRequest::Method::Get,  [this](uint idLib, uint idAuthor, const QHttpServerRequest &request)
    { return authorBooksOPDS(idLib, idAuthor, request); });

    httpServer_.route("/http_<arg>/authorsequences/<arg>", QHttpServerRequest::Method::Get,  [this](uint idLib, uint idAuthor, const QHttpServerRequest &request)
    { return authorSequencesHTTP(idLib, idAuthor, request); });

    httpServer_.route("/opds_<arg>/authorsequences/<arg>", QHttpServerRequest::Method::Get,  [this](uint idLib, uint idAuthor, const QHttpServerRequest &request)
    { return authorSequencesOPDS(idLib, idAuthor, request); });

    httpServer_.route("/http_<arg>/authorsequence/<arg>/<arg>", QHttpServerRequest::Method::Get, [this](uint idLib, uint idAuthor, uint idSequence, const QHttpServerRequest &request)
    { return authorSequencesHTTP(idLib, idAuthor, idSequence, request); });

    httpServer_.route("/opds_<arg>/authorsequence/<arg>/<arg>", QHttpServerRequest::Method::Get, [this](uint idLib, uint idAuthor, uint idSequence, const QHttpServerRequest &request)
    { return authorSequencesOPDS(idLib, idAuthor, idSequence, request); });

    httpServer_.route("/http_<arg>/authorsequenceless/<arg>", QHttpServerRequest::Method::Get,  [this](uint idLib, uint idAuthor, const QHttpServerRequest &request)
    { return authorSequencelessHTTP(idLib, idAuthor, request); });

    httpServer_.route("/opds_<arg>/authorsequenceless/<arg>", QHttpServerRequest::Method::Get,  [this](uint idLib, uint idAuthor, const QHttpServerRequest &request)
    { return authorSequencelessOPDS(idLib, idAuthor, request); });

    httpServer_.route("/http_<arg>/sequencesindex", QHttpServerRequest::Method::Get,  [this](uint idLib, const QHttpServerRequest &request)
    { return sequencesIndexHTTP(idLib, u""_s, false, request); });

    httpServer_.route("/opds_<arg>/sequencesindex", QHttpServerRequest::Method::Get,  [this](uint idLib, const QHttpServerRequest &request)
    { return sequencesIndexOPDS(idLib, u""_s, false, request); });

    httpServer_.route("/http_<arg>/sequencesindex/<arg>", QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return sequencesIndexHTTP(idLib, sIndex, false, request); });

    httpServer_.route("/opds_<arg>/sequencesindex/<arg>", QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return sequencesIndexOPDS(idLib, sIndex, false, request); });

    httpServer_.route("/http_<arg>/sequencesindex/<arg>/books", QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return sequencesIndexHTTP(idLib, sIndex, true, request); });

    httpServer_.route("/opds_<arg>/sequencesindex/<arg>/books", QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return sequencesIndexOPDS(idLib, sIndex, true, request); });

    httpServer_.route("/http_<arg>/sequencebooks/<arg>", QHttpServerRequest::Method::Get,  [this](uint idLib, uint idSequence, const QHttpServerRequest &request)
    { return sequenceBooksHTTP(idLib, idSequence, request); });

    httpServer_.route("/opds_<arg>/sequencebooks/<arg>", QHttpServerRequest::Method::Get,  [this](uint idLib, uint idSequence, const QHttpServerRequest &request)
    { return sequenceBooksOPDS(idLib, idSequence, request); });

    httpServer_.route("/http_<arg>/genres", QHttpServerRequest::Method::Get,  [this](uint idLib, const QHttpServerRequest &request)
    { return genresHTTP(idLib, 0, request); });

    httpServer_.route("/opds_<arg>/genres", QHttpServerRequest::Method::Get,  [this](uint idLib, const QHttpServerRequest &request)
    { return genresOPDS(idLib, 0, request); });

    httpServer_.route("/http_<arg>/genres/<arg>", QHttpServerRequest::Method::Get,  [this](uint idLib, uint idGenre, const QHttpServerRequest &request)
    {        return genresHTTP(idLib, idGenre, request);    });

    httpServer_.route("/opds_<arg>/genres/<arg>", QHttpServerRequest::Method::Get,  [this](uint idLib, uint idGenre, const QHttpServerRequest &request)
    { return genresOPDS(idLib, idGenre, request); });

    httpServer_.route("/http_<arg>/book/<arg>/<arg>", QHttpServerRequest::Method::Get,  [this](uint idLib, uint idBook, const QString &sFormat)
    { return bookHTTP(idLib, idBook, sFormat); });

    httpServer_.route("/opds_<arg>/book/<arg>/<arg>", QHttpServerRequest::Method::Get,  [this](uint idLib, uint idBook, const QString &sFormat)
    { return bookOPDS(idLib, idBook, sFormat); });

    httpServer_.route("/http_<arg>/search", QHttpServerRequest::Method::Get,  [this](uint idLib, const QHttpServerRequest &request)
    { return searchHTTP(idLib, request); });

    httpServer_.route("/opds_<arg>/search", QHttpServerRequest::Method::Get,  [this](uint idLib, const QHttpServerRequest &request)
    { return searchOPDS(idLib, request); });

    httpServer_.route("/opds_<arg>/opensearch.xml", QHttpServerRequest::Method::Get,  [](uint /*idLib*/, const QHttpServerRequest &request){
        QString sUrl = request.url().toString();
        sUrl.chop(u"opensearch.xml"_s.size());
        QString sTemplate = sUrl +u"search?search_string={searchTerms}"_s;
        QString result = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
               "<OpenSearchDescription xmlns=\"http://a9.com/-/spec/opensearch/1.1/\">"
                "<ShortName>freeLib</ShortName>"
                "<Description>Search on freeLib</Description>"
                "<InputEncoding>UTF-8</InputEncoding>"
                "<OutputEncoding>UTF-8</OutputEncoding>"
                "<Url type=\"application/atom+xml\" template=\"" + sTemplate + "\"/>"
                "<Url type=\"application/atom+xml;profile=opds-catalog;kind=acquisition\" template=\"" + sTemplate +"\"/>"
               "</OpenSearchDescription>";
        return result;
    });
#else
    connect(&OPDS_server, &QTcpServer::newConnection, this, &opds_server::new_connection);
#endif
    OPDS_server_status = 0;
    port = 0;
}

QDomElement opds_server::AddTextNode(const QString &name, const QString &text, QDomNode &node)
{
    QDomElement el = doc.createElement(name);
    node.appendChild(el);
    if(!text.isEmpty())
    {
        QDomText txt = doc.createTextNode(text);
        el.appendChild(txt);
    }
    return el;
}


QList<uint> opds_server::book_list(SLib &lib, uint idAuthor, uint idSeria, ushort idGenre, const QString &sSearch, bool sequenceless = false)
{
    QList<uint> listBooks;
    if(idAuthor !=0 && idSeria != 0){
        QMultiHash<uint, uint>::const_iterator i = lib.mAuthorBooksLink.constFind(idAuthor);
        while(i != lib.mAuthorBooksLink.constEnd() && i.key() == idAuthor){
            if(!lib.mBooks[i.value()].bDeleted && lib.mBooks[i.value()].idSerial == idSeria)
                listBooks << i.value();
            ++i;
        }
        std::sort(listBooks.begin(), listBooks.end(),[lib](uint lhs, uint rhs){ return lib.mBooks[lhs].numInSerial < lib.mBooks[rhs].numInSerial; });
    }
    if(idAuthor != 0 && idSeria == 0){
        auto i = lib.mAuthorBooksLink.constFind(idAuthor);
        if(sequenceless){
            while(i != lib.mAuthorBooksLink.constEnd() && i.key() == idAuthor){
                if(!lib.mBooks[i.value()].bDeleted && lib.mBooks[i.value()].idSerial == 0)
                    listBooks << i.value();
                ++i;
            }
        }else{
            while(i != lib.mAuthorBooksLink.constEnd() && i.key() == idAuthor){
                if(!lib.mBooks[i.value()].bDeleted)
                    listBooks << i.value();
                ++i;
            }
        }
        std::sort(listBooks.begin(), listBooks.end(), [lib](uint lhs, uint rhs){ return lib.mBooks[lhs].sName < lib.mBooks[rhs].sName; });
    }
    if(idAuthor == 0 && idSeria != 0){
        auto iBook = lib.mBooks.constBegin();
        while(iBook != lib.mBooks.constEnd()){
            if(!iBook->bDeleted && iBook->idSerial == idSeria)
                listBooks << iBook.key();
            ++iBook;
        }
        std::sort(listBooks.begin(), listBooks.end(),[lib](uint lhs, uint rhs){ return lib.mBooks[lhs].numInSerial < lib.mBooks[rhs].numInSerial; });
    }
    if(idGenre != 0){
        auto iBook = lib.mBooks.constBegin();
        while(iBook != lib.mBooks.constEnd()){
            if(!iBook->bDeleted){
                foreach(auto iGenre, iBook->listIdGenres){
                    if(iGenre == idGenre){
                        listBooks << iBook.key();
                        break;
                    }
                }
            }
            ++iBook;
        }
        std::sort(listBooks.begin(), listBooks.end(), [lib](uint lhs, uint rhs){ return lib.mBooks[lhs].sName < lib.mBooks[rhs].sName; });
    }
    if(!sSearch.isEmpty()){
        auto iBook = lib.mBooks.constBegin();
        while(iBook != lib.mBooks.constEnd()){
            if(!iBook->bDeleted){
                if(iBook->sName.contains(sSearch, Qt::CaseInsensitive))
                    listBooks << iBook.key();
                else{
                    foreach(uint idAuthor, iBook->listIdAuthors){
                        if(lib.mAuthors[idAuthor].getName().contains(sSearch, Qt::CaseInsensitive)){
                            listBooks << iBook.key();
                            break;
                        }
                    }
                }
            }
            ++iBook;
        }
        std::sort(listBooks.begin(), listBooks.end(), [lib](uint lhs, uint rhs){
            if(lib.mBooks[lhs].idFirstAuthor != lib.mBooks[rhs].idFirstAuthor)
                return lib.mAuthors[lib.mBooks[lhs].idFirstAuthor].getName() < lib.mAuthors[lib.mBooks[rhs].idFirstAuthor].getName();
            else
                return lib.mBooks[lhs].sName < lib.mBooks[rhs].sName;

        });

    }
    return listBooks;
}

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
QDomElement opds_server::doc_header(const QString &session, bool html, const QString &lib_name, const QString &lib_url)
{
    doc.clear();
    if(html)
    {
        QDomProcessingInstruction xmlProcessingInstruction = doc.createProcessingInstruction(QStringLiteral("DOCTYPE"), QStringLiteral("HTML"));
        doc.appendChild(xmlProcessingInstruction);
        QDomElement html = doc.createElement(QStringLiteral("HTML"));
        doc.appendChild(html);
        QDomElement head = doc.createElement(QStringLiteral("HEAD"));
        html.appendChild(head);
        AddTextNode(QStringLiteral("TITLE"), lib_name, head);

        QString css = QStringLiteral(
                          "a.lib {font-size:%1em;font-weight: bold;}\n"
                          "a.book {font-size:%1em;font-weight: bold;}\n"
                          "div.author {font-size: %2em; background: #eeeeee; border-radius: 0.5em ;margin: 0.5em;padding: 0.1em;}\n"
                          "div.caption {font-size: %1em;font-weight: bold;padding-bottom: 0.1em;color: #000000;text-decoration: underline;}\n"
                          "div.book {font-size: %3em;font-weight: bold;padding-bottom: 0.1em;color: #000000;}\n"
                          "input {font-size: %3em;font-weight: bold;padding-bottom: 0.1em;color: #000000;}\n"
                          "a {font-size: %3em;font-weight: bold; color: black;text-decoration: none;}\n"
                          //                "a {font-size: %3em;font-weight: #000000; color: black;text-decoration: none;}\n"
                          "a.block {display: block;}\n"
                          "a:active {color: #000000;text-decoration: underline;}\n"
                          "a:link {color: #000000;text-decoration: none;}\n"
                          "a:visited {color: #000000;text-decoration: none;}\n"
                          "a:focus {color: #000000;text-decoration: underline;}\n"
                          "a:hover {color: #000000;text-decoration: underline;}\n"
                          //                "a.author {font-weight: #444444;}\n"
                          "a.author {font-weight: bold;}\n"
                          "a.author:active {color: #444444;}\n"
                          "a.author:link {color: #444444;}\n"
                          "a.author:visited {color: #444444;}\n"
                          "a.author:focus {color: #444444;}\n"
                          "a.author:hover {color: #444444;}\n"
                          "body {background-color: #ffffff;}\n"
                          "img.cover {height: %4em; float:left; margin-right: 0.5em;}\n"
                          ).arg(
                              for_mobile ?QStringLiteral("3") :QStringLiteral("2"), for_mobile ?QStringLiteral("1.5") :QStringLiteral("1"),
                              for_mobile ?QStringLiteral("2") :QStringLiteral("1.5"), for_mobile ?QStringLiteral("8") :QStringLiteral("6"));
        ;
        QDomElement style = AddTextNode(QStringLiteral("style"), css, head);
        style.setAttribute(QStringLiteral("type"),QStringLiteral("text/css"));
        QDomElement meta = doc.createElement(QStringLiteral("META"));
        meta.setAttribute(QStringLiteral("http-equiv"), QStringLiteral("Content-Type"));
        meta.setAttribute(QStringLiteral("content"), QStringLiteral("text/html; charset=utf-8"));
        head.appendChild(meta);

        QDomElement link;
        QString icon_file = QStringLiteral("/icon_256x256.png");
        if(for_preview)
            icon_file = QStringLiteral("/splash_opera.png");

        link = doc.createElement(QStringLiteral("LINK"));
        link.setAttribute(QStringLiteral("rel"), QStringLiteral("icon"));
        link.setAttribute(QStringLiteral("type"), QStringLiteral("image/png"));
        link.setAttribute(QStringLiteral("href"), icon_file + (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
        head.appendChild(link);
        link = doc.createElement(QStringLiteral("LINK"));
        link.setAttribute(QStringLiteral("rel"), QStringLiteral("shortcut icon"));
        link.setAttribute(QStringLiteral("type"), QStringLiteral("image/png"));
        link.setAttribute(QStringLiteral("href"), icon_file + (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
        head.appendChild(link);
        link = doc.createElement(QStringLiteral("LINK"));
        link.setAttribute(QStringLiteral("rel"), QStringLiteral("apple-touch-icon"));
        link.setAttribute(QStringLiteral("type"), QStringLiteral("image/png"));
        link.setAttribute(QStringLiteral("href"), icon_file + (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
        head.appendChild(link);

        QDomElement body = doc.createElement(QStringLiteral("BODY"));
        html.appendChild(body);
        QDomElement div = AddTextNode(QStringLiteral("a"), QLatin1String(""), body);
        div.setAttribute(QStringLiteral("class"), QStringLiteral("lib"));
        div.setAttribute(QStringLiteral("href"), (lib_url.isEmpty() ?QStringLiteral("/") :lib_url) + (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
        QDomElement img = doc.createElement(QStringLiteral("img"));
        img.setAttribute(QStringLiteral("src"), QStringLiteral("/home.png"));
        img.setAttribute(QStringLiteral("border"), QStringLiteral("0"));
        img.setAttribute(QStringLiteral("height"), QStringLiteral("%1px").arg(for_mobile ?QStringLiteral("48") :QStringLiteral("32")));
        div.appendChild(img);
        div = AddTextNode(QStringLiteral("a"), QLatin1String(" ") + lib_name, body);
        div.setAttribute(QStringLiteral("class"), QStringLiteral("lib"));
        div.setAttribute(QStringLiteral("href"), (lib_url.isEmpty() ?QStringLiteral("/") :lib_url) + (session.isEmpty() ?QString() :QLatin1String("?session=") + session));

        QDomElement hr = doc.createElement(QStringLiteral("HR"));
        hr.setAttribute(QStringLiteral("size"), QStringLiteral("3"));
        hr.setAttribute(QStringLiteral("color"), QStringLiteral("black"));
        body.appendChild(hr);

        return body;
    }
    else
    {
        QDomProcessingInstruction xmlProcessingInstruction = doc.createProcessingInstruction(QStringLiteral("xml"), QStringLiteral("version=\"1.0\" encoding=\"utf-8\""));
        doc.appendChild(xmlProcessingInstruction);
        QDomElement feed = doc.createElement(QStringLiteral("feed"));
        doc.appendChild(feed);
        feed.setAttribute(QStringLiteral("xmlns"), QStringLiteral("http://www.w3.org/2005/Atom"));
        feed.setAttribute(QStringLiteral("xmlns:dc"), QStringLiteral("http://purl.org/dc/terms/"));
        feed.setAttribute(QStringLiteral("xmlns:os"), QStringLiteral("http://a9.com/-/spec/opensearch/1.1/"));
        feed.setAttribute(QStringLiteral("xmlns:opds"), QStringLiteral("http://opds-spec.org/2010/catalog"));
        return feed;
    }
}

QString opds_server::FillPage(QList<uint> listBooks, SLib& lib, const QString &sTitle, const QString &lib_url, const QString &current_url, QTextStream &ts, bool opds, uint nPage, const QString &session, bool bShowAuthor)
{
    auto nMaxBooksPerPage = options.nOpdsBooksPerPage;
    if(nMaxBooksPerPage == 0)
        nMaxBooksPerPage = std::numeric_limits<typeof(nMaxBooksPerPage)>::max();
    QDomElement feed;
    if(opds)
    {
        ts << WriteSuccess(QStringLiteral("application/atom+xml;charset=utf-8"));
        feed = doc_header(session);
    }
    else
    {
        ts << WriteSuccess();
        feed = doc_header(session, true, lib.name, lib_url);
    }
    if(listBooks.isEmpty())
        return QLatin1String("");
    QString parameters;
    QStringList keys = params.keys();
    foreach (const QString &param, keys)
    {
        if(param != QLatin1String("page"))
        {
            parameters += QLatin1String("&") + param + QLatin1String("=") + params.value(param);
        }
    }
    parameters += session.isEmpty() ?QString() :(parameters.isEmpty() ?QLatin1String("?") :QLatin1String("&")) + QLatin1String("session=") + session;

    if(opds)
    {
        AddTextNode(QStringLiteral("title"), sTitle,feed);
        AddTextNode(QStringLiteral("updated"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate), feed);
        AddTextNode(QStringLiteral("icon"), QStringLiteral("/icon_256x256.png"), feed);
        uint iBook = 0;
        foreach(uint idBook, listBooks){
            if(iBook >= nPage*nMaxBooksPerPage && iBook < (nPage+1)*nMaxBooksPerPage){
                SBook& book = lib.mBooks[idBook];
                QString sIdBook = QString::number(idBook);
                QDomElement entry = doc.createElement(QStringLiteral("entry"));
                feed.appendChild(entry);
                AddTextNode(QStringLiteral("updated"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate), entry);
                AddTextNode(QStringLiteral("id"), QStringLiteral("tag:book:%1").arg(idBook), entry);
                QString sSerial = book.idSerial == 0 ?QString() :lib.mSerials[book.idSerial].sName;
                AddTextNode(QStringLiteral("title"), book.sName + (sSerial.isEmpty() ?QString() :QLatin1String(" (") + sSerial + QLatin1String(")")), entry);
                foreach(uint idAuthor, book.listIdAuthors){
                    QDomElement author = doc.createElement(QStringLiteral("author"));
                    entry.appendChild(author);
                    AddTextNode(QStringLiteral("name"), lib.mAuthors[idAuthor].getName(), author);
                }
                foreach(uint idGenre, book.listIdGenres){
                    QDomElement category = doc.createElement(QStringLiteral("category"));
                    entry.appendChild(category);
                    category.setAttribute(QStringLiteral("term"), mGenre[idGenre].sName);
                    category.setAttribute(QStringLiteral("label"), mGenre[idGenre].sName);
                }
                if(book.sFormat == QLatin1String("fb2"))
                {
                    QDomElement el = AddTextNode(QStringLiteral("link"), QLatin1String(""), entry);
                    el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/book/") + QUrl::toPercentEncoding(sIdBook) + QLatin1String("/fb2")
                                    + (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
                    el.setAttribute(QStringLiteral("rel"), QStringLiteral("http://opds-spec.org/acquisition/open-access"));
                    el.setAttribute(QStringLiteral("type"), QStringLiteral("application/fb2"));
                }
                else if(book.sFormat == QLatin1String("epub") || book.sFormat == QLatin1String("mobi")){
                    QDomElement el = AddTextNode(QStringLiteral("link"), QLatin1String(""), entry);
                    el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/book/") + QUrl::toPercentEncoding(sIdBook) + QLatin1String("/download")
                                    + (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
                    el.setAttribute(QStringLiteral("rel"), QStringLiteral("http://opds-spec.org/acquisition/open-access"));
                    el.setAttribute(QStringLiteral("type"), QLatin1String("application/") + book.sFormat);
                }
                QDomElement el;

                el=AddTextNode(QStringLiteral("link"), QLatin1String(""), entry);
                el.setAttribute(QStringLiteral("href"),lib_url + QLatin1String("/book/") + QUrl::toPercentEncoding(sIdBook) + QLatin1String("/download")
                                + (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
                el.setAttribute(QStringLiteral("rel"), QStringLiteral("alternate"));
                el.setAttribute(QStringLiteral("type"), QLatin1String("application/") + book.sFormat);
                el.setAttribute(QStringLiteral("title"), tr("Download"));

                if(options.bOpdsShowCover)
                {
                    el=AddTextNode(QStringLiteral("link"), QLatin1String(""), entry);
                    el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/covers/") + QUrl::toPercentEncoding(sIdBook) + QLatin1String("/cover.jpg") +
                                    (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
                    el.setAttribute(QStringLiteral("rel"), QStringLiteral("http://opds-spec.org/image"));
                    el.setAttribute(QStringLiteral("type"), QStringLiteral("image/jpeg"));
                    el=AddTextNode(QStringLiteral("link"), QLatin1String(""), entry);
                    el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/covers/") + QUrl::toPercentEncoding(sIdBook) + QLatin1String("/cover.jpg") +
                                    (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
                    el.setAttribute(QStringLiteral("rel"), QStringLiteral("x-stanza-cover-image"));
                    el.setAttribute(QStringLiteral("type"), QStringLiteral("image/jpeg"));
                    el=AddTextNode(QStringLiteral("link"), QLatin1String(""), entry);
                    el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/covers/") + QUrl::toPercentEncoding(sIdBook) + QLatin1String("/cover.jpg") +
                                    (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
                    el.setAttribute(QStringLiteral("rel"), QStringLiteral("http://opds-spec.org/thumbnail"));
                    el.setAttribute(QStringLiteral("type"), QStringLiteral("image/jpeg"));
                    el=AddTextNode(QStringLiteral("link"), QLatin1String(""), entry);
                    el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/covers/") + QUrl::toPercentEncoding(sIdBook) + QLatin1String("/cover.jpg") +
                                    (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
                    el.setAttribute(QStringLiteral("rel"), QStringLiteral("x-stanza-cover-image-thumbnail"));
                    el.setAttribute(QStringLiteral("type"), QStringLiteral("image/jpeg"));
                }
                AddTextNode(QStringLiteral("dc:language"), lib.vLaguages[book.idLanguage], entry);
                AddTextNode(QStringLiteral("dc:format"), book.sFormat, entry);

                if(options.bOpdsShowAnotation)
                {
                    QBuffer outbuff;
                    QFileInfo fi_book;
                    fi_book = lib.getBookFile(idBook, &outbuff);
                    if(fi_book.suffix().toLower() == QLatin1String("fb2"))
                    {
                        if(book.sAnnotation.isEmpty()){
                            lib.loadAnnotation(idBook);
                        }
                        el = AddTextNode(QStringLiteral("content"), book.sAnnotation, entry);
                        el.setAttribute(QStringLiteral("type"), QStringLiteral("text/html"));
                    }
                }
            }
            iBook++;
        }
        if(nPage >= 1){
            QDomElement entry = doc.createElement(QStringLiteral("entry"));
            feed.appendChild(entry);
            AddTextNode(QStringLiteral("updated"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate), entry);
            AddTextNode(QStringLiteral("id"), QStringLiteral("tag:root"), entry);
            AddTextNode(QStringLiteral("title"), tr("Previous page"), entry);
            QDomElement el = AddTextNode(QStringLiteral("link"), QLatin1String(""), entry);
            el.setAttribute(QStringLiteral("href"), lib_url + current_url + QStringLiteral("?page=%1").arg(nPage-1) + parameters);
            el.setAttribute(QStringLiteral("type"), QStringLiteral("application/atom+xml;profile=opds-catalog"));

            el=AddTextNode(QStringLiteral("link"), QLatin1String(""), entry);
            el.setAttribute(QStringLiteral("href"),QLatin1String("/arrow_left.png") + (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
            el.setAttribute(QStringLiteral("rel"), QStringLiteral("http://opds-spec.org/image"));
            el.setAttribute(QStringLiteral("type"), QStringLiteral("image/jpeg"));
        }
        if(static_cast<uint>(listBooks.size()) > (nPage+1)*nMaxBooksPerPage){
            QDomElement entry = doc.createElement(QStringLiteral("entry"));
            feed.appendChild(entry);
            AddTextNode(QStringLiteral("updated"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate), entry);
            AddTextNode(QStringLiteral("id"), QStringLiteral("tag:root"), entry);
            AddTextNode(QStringLiteral("title"), tr("Next page"), entry);
            QDomElement el = AddTextNode(QStringLiteral("link"), QLatin1String(""), entry);
            el.setAttribute(QStringLiteral("href"), lib_url + current_url + QStringLiteral("?page=%1").arg(nPage+1) + parameters);
            el.setAttribute(QStringLiteral("type"), QStringLiteral("application/atom+xml;profile=opds-catalog"));
            el=AddTextNode(QStringLiteral("link"), QLatin1String(""), entry);
            el.setAttribute(QStringLiteral("href"), QLatin1String("/arrow_right.png") + (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
            el.setAttribute(QStringLiteral("rel"), QStringLiteral("http://opds-spec.org/image"));
            el.setAttribute(QStringLiteral("type"), QStringLiteral("image/jpeg"));
        }
        return doc.toString();
    }
    else
    {
        QDomElement div = AddTextNode(QStringLiteral("DIV"), sTitle, feed);
        div.setAttribute(QStringLiteral("class"), QStringLiteral("caption"));
        uint iBook = 0;
        foreach(uint idBook, listBooks){
            if(iBook >= nPage*nMaxBooksPerPage && iBook < (nPage+1)*nMaxBooksPerPage){
                SBook& book = lib.mBooks[idBook];
                QString sIdBook = QString::number(idBook);
                QDomElement entry = doc.createElement(QStringLiteral("div"));
                entry.setAttribute(QStringLiteral("class"), QStringLiteral("author"));
                feed.appendChild(entry);

                QDomElement table = doc.createElement(QStringLiteral("table"));
                table.setAttribute(QStringLiteral("width"), QStringLiteral("100%"));
                entry.appendChild(table);
                QDomElement tr = doc.createElement(QStringLiteral("tr"));
                table.appendChild(tr);
                entry = doc.createElement(QStringLiteral("td"));
                tr.appendChild(entry);

                if(options.bOpdsShowCover)
                {
                    QDomElement el = doc.createElement(QStringLiteral("img"));
                    entry.appendChild(el);
                    el.setAttribute(QStringLiteral("src"), lib_url + QLatin1String("/covers/") + QUrl::toPercentEncoding(sIdBook) + QLatin1String("/cover.jpg"));
                    el.setAttribute(QStringLiteral("class"), QStringLiteral("cover"));
                }
                if(bShowAuthor){
                    QDomElement el = AddTextNode(QStringLiteral("a"), lib.mAuthors[book.idFirstAuthor].getName(), entry);
                    el.setAttribute(QStringLiteral("class"), QStringLiteral("book"));
                    el.setAttribute(QStringLiteral("href"), QStringLiteral("/author/%1").arg(book.idFirstAuthor) +
                                    (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
                }

                QString sSerial = book.idSerial == 0 ?QString() :lib.mSerials[book.idSerial].sName;
                QDomElement el = AddTextNode(QStringLiteral("div"), book.sName + (sSerial.isEmpty() ?QString()
                                 :(QLatin1String(" (") + sSerial + QLatin1String("[") + QString::number(book.numInSerial) + QLatin1String("])"))), entry);
                el.setAttribute(QStringLiteral("class"), QStringLiteral("book"));
                QDomElement br = doc.createElement(QStringLiteral("br"));
                entry.appendChild(br);

                if(book.sFormat == QLatin1String("fb2"))
                {
                    QDomElement el = AddTextNode(QStringLiteral("a"), QStringLiteral("fb2"), entry);
                    el.setAttribute(QStringLiteral("href"),lib_url + QLatin1String("/book/") + QUrl::toPercentEncoding(sIdBook) + QLatin1String("/fb2") +
                                    (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
                    el.setAttribute(QStringLiteral("class"), QStringLiteral("author"));
                    el=AddTextNode(QStringLiteral("a"), QStringLiteral("epub"), entry);
                    el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/book/") + QUrl::toPercentEncoding(sIdBook) + QLatin1String("/epub") +
                                    (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
                    el.setAttribute(QStringLiteral("class"), QStringLiteral("author"));
                    el=AddTextNode(QStringLiteral("a"), QStringLiteral("mobi"), entry);
                    el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/book/") + QUrl::toPercentEncoding(sIdBook) + QLatin1String("/mobi") +
                                    (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
                    el.setAttribute(QStringLiteral("class"), QStringLiteral("author"));
                    el=AddTextNode(QStringLiteral("a"), QStringLiteral("azw3"), entry);
                    el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/book/") + QUrl::toPercentEncoding(sIdBook) + QLatin1String("/azw3") +
                                    (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
                    el.setAttribute(QStringLiteral("class"), QStringLiteral("author"));
                }
                else if(book.sFormat == QLatin1String("epub"))
                {
                    QDomElement el = AddTextNode(QStringLiteral("a"), QStringLiteral("epub"), entry);
                    el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/book/") + QUrl::toPercentEncoding(sIdBook) + QLatin1String("/epub") +
                                    (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
                    el.setAttribute(QStringLiteral("class"), QStringLiteral("author"));
                    el=AddTextNode(QStringLiteral("a"), QStringLiteral("mobi"), entry);
                    el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/book/") + QUrl::toPercentEncoding(sIdBook) + QLatin1String("/mobi") +
                                    (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
                    el.setAttribute(QStringLiteral("class"), QStringLiteral("author"));
                }
                else if(book.sFormat == QLatin1String("mobi"))
                {
                    QDomElement el = AddTextNode(QStringLiteral("a"), QStringLiteral("mobi"), entry);
                    el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/book/") + QUrl::toPercentEncoding(sIdBook) + QLatin1String("/mobi") +
                                    (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
                    el.setAttribute(QStringLiteral("class"), QStringLiteral("author"));
                }
                else
                {
                    QDomElement el=AddTextNode(QStringLiteral("a"), book.sFormat,entry);
                    el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/book/") + QUrl::toPercentEncoding(sIdBook) + QLatin1String("/download") +
                                    (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
                    el.setAttribute(QStringLiteral("class"), QStringLiteral("author"));
                }

                if(options.bOpdsShowAnotation)
                {
                    QBuffer outbuff;
                    QFileInfo fi_book;
                    fi_book = lib.getBookFile(idBook, &outbuff);
                    if(fi_book.suffix().toLower() == QLatin1String("fb2"))
                    {
                        if(book.sAnnotation.isEmpty())
                            lib.loadAnnotation(idBook);
                        QDomDocument an;
                        an.setContent(QStringLiteral("<dev>%1</dev>").arg(book.sAnnotation));
                        QDomNode an_node = doc.importNode(an.childNodes().at(0), true);
                        entry.appendChild(an_node);
                    }
                }
            }
            iBook++;
        }
        if(nPage >= 1){
            QDomElement entry = doc.createElement(QStringLiteral("div"));
            feed.appendChild(entry);
            QDomElement el = AddTextNode(QStringLiteral("a"), tr("Previous page"), entry);
            el.setAttribute(QStringLiteral("href"), lib_url + current_url + QStringLiteral("?page=%1").arg(nPage-1) + parameters);
            el.setAttribute(QStringLiteral("class"), QStringLiteral("author"));
        }
        if(static_cast<uint>(listBooks.size()) > (nPage+1)*nMaxBooksPerPage){
            QDomElement entry = doc.createElement(QStringLiteral("div"));
            feed.appendChild(entry);

            QDomElement el = AddTextNode(QStringLiteral("a"), tr("Next page"), entry);
            el.setAttribute(QStringLiteral("href"), lib_url + current_url + QStringLiteral("?page=%1").arg(nPage+1) + parameters);
            el.setAttribute(QStringLiteral("class"), QStringLiteral("author"));

        }
        QString str;
        QTextStream t(&str, QIODevice::WriteOnly);
        doc.namedItem(QStringLiteral("HTML")).save(t, SAVE_INDEX);
        return str;
    }
}

QString OPDS_MIME_TYPE(QString type)
{
    type = type.toLower();
    if(type == QLatin1String("jpg") || type == QLatin1String("jpeg"))
        return QStringLiteral("image/jpeg");
    if(type == QLatin1String("png"))
        return QStringLiteral("image/png");
    if(type == QLatin1String("txt"))
        return QStringLiteral("text/plain");
    if(type == QLatin1String("fb2"))
        return QStringLiteral("application/fb2+zip");
    if(type == QLatin1String("epub"))
        return QStringLiteral("application/epub+zip");
    if(type == QLatin1String("mobi") || type == QLatin1String("azw") || type == QLatin1String("azw3"))
        return QStringLiteral("application/x-mobipocket-ebook");
    return QStringLiteral("application/octet-stream");
}
#else

bool opds_server::checkAuth(const QHttpServerRequest &request, QUrl &url)
{
    bool bResult = false;
    url = request.url();
    if(options.bOpdsNeedPassword){
        QUrlQuery urlquery(url);
        QString sSession = urlquery.queryItemValue(u"session"_s);
        if(!sSession.isEmpty() && sessions.contains(sSession)){
            sessions[sSession] = QDateTime::currentDateTime();
            bResult = true;
        }else{
            auto auth = request.value("authorization"_ba).simplified();
            if (auth.size() > 6 && auth.first(6).toLower() == "basic "_ba) {
                auto token = auth.sliced(6);
                auto userPass = QByteArray::fromBase64(token);

                if (auto colon = userPass.indexOf(':'); colon > 0) {
                    auto user = userPass.first(colon);
                    auto password = userPass.sliced(colon + 1);

                    if (user == options.sOpdsUser && password == options.sOpdsPassword){
                        bResult = true;
                        static QString chars = u"abdefhiknrstyzABDEFGHKNQRSTYZ23456789"_s;
                        const int sessionNumberLen = 16;
                        sSession = u""_s;
                        srand(time(NULL));
                        for(int i=0; i<sessionNumberLen; i++)
                            sSession += chars[rand() % 32];
                        sessions.insert(sSession, QDateTime::currentDateTime());
                        if(urlquery.hasQueryItem(u"session"_s))
                            urlquery.removeQueryItem(u"session"_s);
                        urlquery.addQueryItem(u"session"_s, sSession);
                        url.setQuery(urlquery);
                    }
                }
            }
        }
    }else
        bResult = true;

    auto timeExpire = QDateTime::currentDateTime().addSecs(-60*60);
    foreach (const QString &key, sessions.keys())
    {
        if(sessions.value(key) < timeExpire)
            sessions.remove(key);
    }

    return bResult;
}

QDomElement opds_server::docHeaderHTTP(const QString &sSesionQuery, const QString &sLibName, const QString &sLibUrl)
{
    doc.clear();
    QDomProcessingInstruction xmlProcessingInstruction = doc.createProcessingInstruction(u"DOCTYPE"_s, u"HTML"_s);
    doc.appendChild(xmlProcessingInstruction);
    QDomElement html = doc.createElement(u"HTML"_s);
    doc.appendChild(html);
    QDomElement head = doc.createElement(u"HEAD"_s);
    html.appendChild(head);
    AddTextNode(u"TITLE"_s, sLibName, head);

    QString css =   u"a.lib {font-size:%1em;font-weight: bold;}\n"
                      "a.book {font-size:%1em;font-weight: bold;}\n"
                      "div.author {font-size: %2em; background: #eeeeee; border-radius: 0.5em ;margin: 0.5em;padding: 0.1em;}\n"
                      "div.caption {font-size: %1em;font-weight: bold;padding-bottom: 0.1em;color: #000000;text-decoration: underline;}\n"
                      "div.book {font-size: %3em;font-weight: bold;padding-bottom: 0.1em;color: #000000;}\n"
                      "input {font-size: %3em;font-weight: bold;padding-bottom: 0.1em;color: #000000;}\n"
                      "a {font-size: %3em;font-weight: bold; color: black;text-decoration: none;}\n"
                      //                "a {font-size: %3em;font-weight: #000000; color: black;text-decoration: none;}\n"
                      "a.block {display: block;}\n"
                      "a:active {color: #000000;text-decoration: underline;}\n"
                      "a:link {color: #000000;text-decoration: none;}\n"
                      "a:visited {color: #000000;text-decoration: none;}\n"
                      "a:focus {color: #000000;text-decoration: underline;}\n"
                      "a:hover {color: #000000;text-decoration: underline;}\n"
                      //                "a.author {font-weight: #444444;}\n"
                      "a.author {font-weight: bold;}\n"
                      "a.author:active {color: #444444;}\n"
                      "a.author:link {color: #444444;}\n"
                      "a.author:visited {color: #444444;}\n"
                      "a.author:focus {color: #444444;}\n"
                      "a.author:hover {color: #444444;}\n"
                      "body {background-color: #ffffff;}\n"
                      "img.cover {height: %4em; float:left; margin-right: 0.5em;}\n"_s
                      .arg(
                          for_mobile ?u"3"_s :u"2"_s, for_mobile ?u"1.5"_s :u"1"_s,
                          for_mobile ?u"2"_s :u"1.5"_s, for_mobile ?u"8"_s :u"6"_s);
    ;
    QDomElement style = AddTextNode(u"style"_s, css, head);
    style.setAttribute(u"type"_s, u"text/css"_s);
    QDomElement meta = doc.createElement(u"META"_s);
    meta.setAttribute(u"http-equiv"_s, u"Content-Type"_s);
    meta.setAttribute(u"content"_s, u"text/html; charset=utf-8"_s);
    head.appendChild(meta);

    QDomElement link;
    QString icon_file = u"/icon_256x256.png"_s;
    if(for_preview)
        icon_file = u"/splash_opera.png"_s;

    link = doc.createElement(u"LINK"_s);
    link.setAttribute(u"rel"_s, u"icon"_s);
    link.setAttribute(u"type"_s, u"image/png"_s);
    link.setAttribute(u"href"_s, icon_file + sSesionQuery);
    head.appendChild(link);
    link = doc.createElement(u"LINK"_s);
    link.setAttribute(u"rel"_s, u"shortcut icon"_s);
    link.setAttribute(u"type"_s, u"image/png"_s);
    link.setAttribute(u"href"_s, icon_file + sSesionQuery);
    head.appendChild(link);
    link = doc.createElement(u"LINK"_s);
    link.setAttribute(u"rel"_s, u"apple-touch-icon"_s);
    link.setAttribute(u"type"_s, u"image/png"_s);
    link.setAttribute(u"href"_s, icon_file + sSesionQuery);
    head.appendChild(link);

    QDomElement body = doc.createElement(u"BODY"_s);
    html.appendChild(body);
    QDomElement div = AddTextNode(u"a"_s, u""_s, body);
    div.setAttribute(u"class"_s, u"lib"_s);
    div.setAttribute(u"href"_s, (sLibUrl.isEmpty() ?u"/"_s :sLibUrl) + sSesionQuery);
    QDomElement img = doc.createElement(u"img"_s);
    img.setAttribute(u"src"_s, u"/home.png"_s);
    img.setAttribute(u"border"_s, u"0"_s);
    img.setAttribute(u"height"_s, for_mobile ?u"48px"_s :u"32px"_s);
    div.appendChild(img);
    div = AddTextNode(u"a"_s, u" "_s + sLibName, body);
    div.setAttribute(u"class"_s, u"lib"_s);
    div.setAttribute(u"href"_s, (sLibUrl.isEmpty() ?u"/"_s :sLibUrl) + sSesionQuery);

    QDomElement hr = doc.createElement(u"HR"_s);
    hr.setAttribute(u"size"_s, u"3"_s);
    hr.setAttribute(u"color"_s, u"black"_s);
    body.appendChild(hr);

    return body;

}

QDomElement opds_server::docHeaderOPDS(const QString &sTitle, const QString &sID, const QString &sLibUrl, const QString &sSesionQuery)
{
    doc.clear();
    QDomProcessingInstruction xmlProcessingInstruction = doc.createProcessingInstruction(u"xml"_s, u"version=\"1.0\" encoding=\"utf-8\""_s);
    doc.appendChild(xmlProcessingInstruction);
    QDomElement feed = doc.createElement(u"feed"_s);
    doc.appendChild(feed);
    feed.setAttribute(u"xmlns"_s, u"http://www.w3.org/2005/Atom"_s);
    feed.setAttribute(u"xmlns:dc"_s, u"http://purl.org/dc/terms/"_s);
    feed.setAttribute(u"xmlns:os"_s, u"http://a9.com/-/spec/opensearch/1.1/"_s);
    feed.setAttribute(u"xmlns:opds"_s, u"http://opds-spec.org/2010/catalog"_s);

    AddTextNode(u"updated"_s, QDateTime::currentDateTimeUtc().toString(Qt::ISODate), feed);
    AddTextNode(u"icon"_s, u"/icon_256x256.png"_s, feed);
    if(!sID.isEmpty())
        AddTextNode(u"id"_s, sID, feed);
    AddTextNode(u"title"_s, sTitle, feed);

    QDomElement link;

    link = doc.createElement(u"link"_s);
    link.setAttribute(u"rel"_s, u"start"_s);
    link.setAttribute(u"href"_s, sLibUrl);
    link.setAttribute(u"title"_s, u"Home"_s);
    link.setAttribute(u"type"_s, u"application/atom+xml;profile=opds-catalog;kind=navigation"_s);
    feed.appendChild(link);


    link = doc.createElement(u"link"_s);
    link.setAttribute(u"href"_s, sLibUrl + u"/opensearch.xml"_s);
    link.setAttribute(u"rel"_s, u"search"_s);
    link.setAttribute(u"type"_s, u"application/opensearchdescription+xml"_s);
    feed.appendChild(link);

    link = doc.createElement(u"link"_s);
    link.setAttribute(u"href"_s, sLibUrl + u"/search?search_string={searchTerms}"_s + sSesionQuery);
    link.setAttribute(u"rel"_s, u"search"_s);
    link.setAttribute(u"type"_s, u"application/atom+xml"_s);
    feed.appendChild(link);

    return feed;
}

SLib& opds_server::getLib(uint &idLib, const QString &sTypeServer, QString *pLibUrl)
{
    if(idLib == 0)
        idLib = idCurrentLib;
    if(pLibUrl != nullptr)
        *pLibUrl = sTypeServer +u"_"_s + QString::number(idLib);
    SLib &lib = mLibs[idLib];
    if(!lib.bLoaded)
        loadLibrary(idLib);

    return lib;
}

QHttpServerResponse opds_server::responseHTTP()
{
    QString str = u"<!DOCTYPE html>\n"_s;
    QTextStream ts(&str, QIODevice::WriteOnly);
    doc.namedItem(u"HTML"_s).save(ts, SAVE_INDEX);
    QHttpServerResponse result(str);
    result.addHeader("Server"_ba, "freeLib "_ba + FREELIB_VERSION);
    result.addHeader("Connection"_ba, "keep-alive"_ba);
    result.addHeader("Pragma"_ba, "no-cache"_ba);

    return result;
}

QHttpServerResponse opds_server::responseUnauthorized()
{
    QHttpServerResponse result(u"HTTP/1.1 401 Authorization Required"_s, QHttpServerResponder::StatusCode::Unauthorized);
    result.addHeader("WWW-Authenticate"_ba, "Basic"_ba);
    result.addHeader("Content-Type"_ba, "text/html;charset=utf-8");
    result.addHeader("Connection"_ba, "close"_ba);
    return result;
}

QHttpServerResponse opds_server::FillPageHTTP(QList<uint> listBooks, SLib &lib, const QString &sTitle, const QString &sLibUrl, const QUrl &url, bool bShowAuthor)
{
    auto nMaxBooksPerPage = options.nOpdsBooksPerPage;
    if(nMaxBooksPerPage == 0)
        nMaxBooksPerPage = std::numeric_limits<typeof(nMaxBooksPerPage)>::max();

    QUrlQuery urlquery(url);
    uint nPage = urlquery.queryItemValue(u"page"_s).toUInt();
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSesionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    QDomElement feed;
    feed = docHeaderHTTP(sSesionQuery, lib.name, sLibUrl);

    QDomElement div = AddTextNode(u"DIV"_s, sTitle, feed);
    div.setAttribute(u"class"_s, u"caption"_s);
    uint iBook = 0;
    foreach(uint idBook, listBooks){
        if(iBook >= nPage*nMaxBooksPerPage && iBook < (nPage+1)*nMaxBooksPerPage){
            SBook& book = lib.mBooks[idBook];
            QString sIdBook = QString::number(idBook);
            QDomElement entry = doc.createElement(u"div"_s);
            entry.setAttribute(u"class"_s, u"author"_s);
            feed.appendChild(entry);

            QDomElement table = doc.createElement(u"table"_s);
            table.setAttribute(u"width"_s, u"100%"_s);
            entry.appendChild(table);
            QDomElement tr = doc.createElement(u"tr"_s);
            table.appendChild(tr);
            entry = doc.createElement(u"td"_s);
            tr.appendChild(entry);

            if(options.bOpdsShowCover)
            {
                QDomElement el = doc.createElement(u"img"_s);
                entry.appendChild(el);
                el.setAttribute(u"src"_s, sLibUrl + u"/covers/"_s + sIdBook + u"/cover.jpg"_s);
                el.setAttribute(u"class"_s, u"cover"_s);
            }
            if(bShowAuthor){
                QDomElement el = AddTextNode(u"a"_s, lib.mAuthors[book.idFirstAuthor].getName(), entry);
                el.setAttribute(u"class"_s, u"book"_s);
                el.setAttribute(u"href"_s, sLibUrl + u"/author/"_s + QString::number(book.idFirstAuthor) + sSession);
            }

            QString sSerial = book.idSerial == 0 ?u""_s :lib.mSerials[book.idSerial].sName;
            QDomElement el = AddTextNode(u"div"_s, book.sName + (sSerial.isEmpty() || book.numInSerial==0  ?u""_s
                                                                                                            :(u" ("_s + sSerial + u"["_s + QString::number(book.numInSerial) + u"])"_s)), entry);
            el.setAttribute(u"class"_s, u"book"_s);
            QDomElement br = doc.createElement(u"br"_s);
            entry.appendChild(br);

            if(book.sFormat == u"fb2"_s)
            {
                QDomElement el = AddTextNode(u"a"_s, u"fb2"_s, entry);
                el.setAttribute(u"href"_s, sLibUrl + u"/book/"_s + QUrl::toPercentEncoding(sIdBook) + u"/fb2"_s + sSesionQuery);
                el.setAttribute(u"class"_s, u"author"_s);
                el = AddTextNode(u"a"_s, u"epub"_s, entry);
                el.setAttribute(u"href"_s, sLibUrl + u"/book/"_s + QUrl::toPercentEncoding(sIdBook) + u"/epub"_s + sSesionQuery);
                el.setAttribute(u"class"_s, u"author"_s);
                el=AddTextNode(u"a"_s, u"mobi"_s, entry);
                el.setAttribute(u"href"_s, sLibUrl + u"/book/"_s + QUrl::toPercentEncoding(sIdBook) + u"/mobi"_s + sSesionQuery);
                el.setAttribute(u"class"_s, u"author"_s);
                el = AddTextNode(u"a"_s, u"azw3"_s, entry);
                el.setAttribute(u"href"_s, sLibUrl + u"/book/"_s + QUrl::toPercentEncoding(sIdBook) + u"/azw3"_s + sSesionQuery);
                el.setAttribute(u"class"_s, u"author"_s);
            }
            else if(book.sFormat == u"epub"_s)
            {
                QDomElement el = AddTextNode(u"a"_s, u"epub"_s, entry);
                el.setAttribute(u"href"_s, sLibUrl + u"/book/"_s + QUrl::toPercentEncoding(sIdBook) + u"/epub"_s + sSesionQuery);
                el.setAttribute(u"class"_s, u"author"_s);
                el = AddTextNode(u"a"_s, u"mobi"_s, entry);
                el.setAttribute(u"href"_s, sLibUrl + u"/book/"_s + QUrl::toPercentEncoding(sIdBook) + u"/mobi"_s + sSesionQuery);
                el.setAttribute(u"class"_s, u"author"_s);
            }
            else if(book.sFormat == u"mobi"_s)
            {
                QDomElement el = AddTextNode(u"a"_s, u"mobi"_s, entry);
                el.setAttribute(u"href"_s, sLibUrl + u"/book/"_s + QUrl::toPercentEncoding(sIdBook) + u"/mobi"_s + sSesionQuery);
                el.setAttribute(u"class"_s, u"author"_s);
            }
            else
            {
                QDomElement el = AddTextNode(u"a"_s, book.sFormat,entry);
                el.setAttribute(u"href"_s, sLibUrl + u"/book/"_s + QUrl::toPercentEncoding(sIdBook) + u"/download"_s + sSesionQuery);
                el.setAttribute(u"class"_s, u"author"_s);
            }

            if(options.bOpdsShowAnotation)
            {
                QBuffer outbuff;
                QFileInfo fi_book;
                fi_book = lib.getBookFile(idBook, &outbuff);
                if(fi_book.suffix().toLower() == u"fb2"_s)
                {
                    if(book.sAnnotation.isEmpty())
                        lib.loadAnnotation(idBook);
                    QDomDocument an;
                    an.setContent(u"<dev>"_s + book.sAnnotation + u"</dev>"_s);
                    QDomNode an_node = doc.importNode(an.childNodes().at(0), true);
                    entry.appendChild(an_node);
                }
            }
        }
        iBook++;
    }
    auto listQueryItems = urlquery.queryItems();
    if(nPage >= 1){
        for(int i=0; i<listQueryItems.size(); i++){
            if(listQueryItems[i].first == u"page"_s)
                listQueryItems[i].second = QString::number(nPage-1);
        }
        urlquery.setQueryItems(listQueryItems);
        QUrl urlPrevious = url.toString(QUrl::RemoveQuery);
        urlPrevious.setQuery(urlquery);

        QDomElement entry = doc.createElement(u"div"_s);
        feed.appendChild(entry);
        QDomElement el = AddTextNode(u"a"_s, tr("Previous page"), entry);
        el.setAttribute(u"href"_s, urlPrevious.toString());
        el.setAttribute(u"class"_s, u"author"_s);
    }
    if(static_cast<uint>(listBooks.size()) > (nPage+1)*nMaxBooksPerPage){
        int i = 0;
        for(i=0; i<listQueryItems.size(); i++){
            if(listQueryItems[i].first == u"page"_s){
                listQueryItems[i].second = QString::number(nPage+1);
                break;
            }
        }
        if(i >= listQueryItems.size())
            listQueryItems << QPair<QString, QString> (u"page"_s, QString::number(nPage+1));
        urlquery.setQueryItems(listQueryItems);
        QUrl urlNext = url.toString(QUrl::RemoveQuery);
        urlNext.setQuery(urlquery);

        QDomElement entry = doc.createElement(u"div"_s);
        feed.appendChild(entry);

        QDomElement el = AddTextNode(u"a"_s, tr("Next page"), entry);
        el.setAttribute(u"href"_s, urlNext.toString());
        el.setAttribute(u"class"_s, u"author"_s);

    }
    QString str = u"<!DOCTYPE html>\n"_s;
    QTextStream ts(&str);
    doc.namedItem(u"HTML"_s).save(ts, SAVE_INDEX);
    QHttpServerResponse result(str);
    result.addHeader("Server"_ba, "freeLib "_ba + FREELIB_VERSION);
    result.addHeader("Connection"_ba, "keep-alive"_ba);
    result.addHeader("Pragma"_ba, "no-cache"_ba);

    return result;
}

QString opds_server::FillPageOPDS(QList<uint> listBooks, SLib &lib, const QString &sTitle,const QString &sId, const QString &sLibUrl, const QUrl &url/*bool bShowAuthor*/)
{
    auto nMaxBooksPerPage = options.nOpdsBooksPerPage;
    if(nMaxBooksPerPage == 0)
        nMaxBooksPerPage = std::numeric_limits<typeof(nMaxBooksPerPage)>::max();

    QUrlQuery urlquery(url);
    uint nPage = urlquery.queryItemValue(u"page"_s).toUInt();
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSesionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    QDomElement feed = docHeaderOPDS(sTitle, sId, sLibUrl, sSesionQuery);

    uint iBook = 0;
    foreach(uint idBook, listBooks){
        if(iBook >= nPage*nMaxBooksPerPage && iBook < (nPage+1)*nMaxBooksPerPage){
            const SBook& book = lib.mBooks[idBook];
            QString sIdBook = QString::number(idBook);
            QDomElement entry = doc.createElement(u"entry"_s);
            feed.appendChild(entry);
            AddTextNode(u"updated"_s, book.date.toString(Qt::ISODate), entry);
            AddTextNode(u"id"_s, u"tag:book:"_s + QString::number(idBook), entry);
            QString sSerial = book.idSerial == 0 ?QString() :lib.mSerials[book.idSerial].sName;
            AddTextNode(u"title"_s, book.sName + (sSerial.isEmpty() ?QString() :u" ("_s + sSerial + u")"_s), entry);
            foreach(uint idAuthor, book.listIdAuthors){
                QDomElement author = doc.createElement(u"author"_s);
                entry.appendChild(author);
                AddTextNode(u"name"_s, lib.mAuthors[idAuthor].getName(), author);
            }
            foreach(auto idGenre, book.listIdGenres){
                QDomElement category = doc.createElement(u"category"_s);
                entry.appendChild(category);
                category.setAttribute(u"term"_s, mGenre[idGenre].sName);
                category.setAttribute(u"label"_s, mGenre[idGenre].sName);
            }
            if(book.sFormat == u"fb2"_s)
            {
                QDomElement el = AddTextNode(u"link"_s, u""_s, entry);
                el.setAttribute(u"href"_s, sLibUrl + u"/book/"_s + sIdBook + u"/fb2"_s + sSesionQuery);
                el.setAttribute(u"rel"_s, u"http://opds-spec.org/acquisition/open-access"_s);
                el.setAttribute(u"type"_s, u"application/fb2"_s);
            }
            else if(book.sFormat == u"epub"_s || book.sFormat == u"mobi"_s){
                QDomElement el = AddTextNode(u"link"_s, u""_s, entry);
                el.setAttribute(u"href"_s, sLibUrl + u"/book/"_s + QUrl::toPercentEncoding(sIdBook) + u"/download"_s
                                               + sSesionQuery);
                el.setAttribute(u"rel"_s, u"http://opds-spec.org/acquisition/open-access"_s);
                el.setAttribute(u"type"_s, u"application/"_s + book.sFormat);
            }
            QDomElement el;

            el = AddTextNode(u"link"_s, u""_s, entry);
            el.setAttribute(QStringLiteral("href"), sLibUrl + u"/book/"_s + QUrl::toPercentEncoding(sIdBook) + u"/download"_s
                                                        + sSesionQuery);
            el.setAttribute(u"rel"_s, u"alternate"_s);
            el.setAttribute(u"type"_s, u"application/"_s + book.sFormat);
            el.setAttribute(u"title"_s, tr("Download"));

            if(options.bOpdsShowCover)
            {
                el = AddTextNode(u"link"_s, u""_s, entry);
                el.setAttribute(u"href"_s, sLibUrl + u"/covers/"_s + sIdBook + u"/cover.jpg"_s + sSesionQuery);
                el.setAttribute(u"rel"_s, u"http://opds-spec.org/image"_s);
                el.setAttribute(u"type"_s, u"image/jpeg"_s);

                el = AddTextNode(u"link"_s, u""_s, entry);
                el.setAttribute(u"href"_s, sLibUrl + u"/covers/"_s + sIdBook + u"/cover.jpg"_s + sSesionQuery);
                el.setAttribute(u"rel"_s, u"x-stanza-cover-image"_s);
                el.setAttribute(u"type"_s, u"image/jpeg"_s);

                el = AddTextNode(u"link"_s, u""_s, entry);
                el.setAttribute(u"href"_s, sLibUrl + u"/covers/"_s + sIdBook + u"/cover.jpg"_s + sSesionQuery);
                el.setAttribute(u"rel"_s, u"http://opds-spec.org/thumbnail"_s);
                el.setAttribute(u"type"_s, u"image/jpeg"_s);

                el = AddTextNode(u"link"_s, u""_s, entry);
                el.setAttribute(u"href"_s, sLibUrl + u"/covers/"_s + sIdBook + u"/cover.jpg"_s + sSesionQuery);
                el.setAttribute(u"rel"_s, u"x-stanza-cover-image-thumbnail"_s);
                el.setAttribute(u"type"_s, u"image/jpeg"_s);
            }
            AddTextNode(u"dc:language"_s, lib.vLaguages[book.idLanguage], entry);
            AddTextNode(u"dc:format"_s, book.sFormat, entry);

            if(options.bOpdsShowAnotation)
            {
                QBuffer outbuff;
                QFileInfo fi_book;
                fi_book = lib.getBookFile(idBook, &outbuff);
                if(fi_book.suffix().toLower() == u"fb2"_s)
                {
                    if(book.sAnnotation.isEmpty()){
                        lib.loadAnnotation(idBook);
                    }
                    el = AddTextNode(u"content"_s, book.sAnnotation, entry);
                    el.setAttribute(u"type"_s, u"text/html"_s);
                }
            }
        }
        iBook++;
    }

    auto listQueryItems = urlquery.queryItems();
    if(nPage >= 1){
        for(int i=0; i<listQueryItems.size(); i++){
                 if(listQueryItems[i].first == u"page"_s)
                    listQueryItems[i].second = QString::number(nPage-1);
        }
        urlquery.setQueryItems(listQueryItems);
        QUrl urlPrevious = url.toString(QUrl::RemoveQuery);
        urlPrevious.setQuery(urlquery);

        QDomElement entry = doc.createElement(u"entry"_s);
        feed.appendChild(entry);
        AddTextNode(u"updated"_s, QDateTime::currentDateTimeUtc().toString(Qt::ISODate), entry);
        AddTextNode(u"id"_s, u"tag:root"_s, entry);
        AddTextNode(u"title"_s, tr("Previous page"), entry);
        QDomElement el = AddTextNode(u"link"_s, u""_s, entry);
        el.setAttribute(u"href"_s, urlPrevious.toString());
        el.setAttribute(u"type"_s, u"application/atom+xml;profile=opds-catalog"_s);

        el = AddTextNode(u"link"_s, u""_s, entry);
        el.setAttribute(u"href"_s, u"/arrow_left.png"_s + sSesionQuery);
        el.setAttribute(u"rel"_s, u"http://opds-spec.org/image"_s);
        el.setAttribute(u"type"_s, u"image/jpeg"_s);
    }
    if(static_cast<uint>(listBooks.size()) > (nPage+1)*nMaxBooksPerPage){
        int i = 0;
        for(i=0; i<listQueryItems.size(); i++){
                 if(listQueryItems[i].first == u"page"_s){
                    listQueryItems[i].second = QString::number(nPage+1);
                    break;
                 }
        }
        if(i >= listQueryItems.size())
                 listQueryItems << QPair<QString, QString> (u"page"_s, QString::number(nPage+1));
        urlquery.setQueryItems(listQueryItems);
        QUrl urlNext = url.toString(QUrl::RemoveQuery);
        urlNext.setQuery(urlquery);

        QDomElement entry = doc.createElement(u"entry"_s);
        feed.appendChild(entry);
        AddTextNode(u"updated"_s, QDateTime::currentDateTimeUtc().toString(Qt::ISODate), entry);
        AddTextNode(u"id"_s, u"tag:root"_s, entry);
        AddTextNode(u"title"_s, tr("Next page"), entry);
        QDomElement el = AddTextNode(u"link"_s, u""_s, entry);
        el.setAttribute(u"href"_s, urlNext.toString());
        el.setAttribute(u"type"_s, u"application/atom+xml;profile=opds-catalog"_s);
        el = AddTextNode(u"link"_s, u""_s, entry);
        el.setAttribute(u"href"_s, u"/arrow_right.png"_s + sSesionQuery);
        el.setAttribute(u"rel"_s, u"http://opds-spec.org/image"_s);
        el.setAttribute(u"type"_s, u"image/jpeg"_s);
    }
    return doc.toString();
}
#endif

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
void opds_server::process(QString url, QTextStream &ts, const QString &session)
{
    int http_settings = options.nHttpExport - 1;
    if(http_settings == -1)
    {
        int count = options.vExportOptions.count();
        for(int i=0; i<count; i++)
        {
            if(options.vExportOptions[i].bDefault)
            {
                http_settings = i;
                break;
            }
        }
    }
    pExportOptions_ = &options.vExportOptions[http_settings];
    url = QUrl::fromPercentEncoding(url.toLatin1());
    int posQuestion = url.indexOf('?');
    QStringList strings;
    if(posQuestion >= 0)
    {
        strings = url.left(posQuestion).split(QStringLiteral("/"));
        strings.last().append(url.right(url.length() - posQuestion));
    }
    else
    {
        strings = url.split(QStringLiteral("/"));
    }
    int id_lib = idCurrentLib;
    QString lib_url = QStringLiteral("/http");
    qDebug()<<"url:"<<url;
    bool opds = false;
    params.clear();
    if(strings.count() > 1)
    {
        if(strings.at(1).startsWith(QLatin1String("opds")))
        {
            url += QLatin1String("/");
            if(strings.at(1).startsWith(QLatin1String("opds_")))
            {
                id_lib = strings[1].replace(QLatin1String("opds_"), QLatin1String("")).toInt();
                lib_url = QLatin1String("/opds_") + QString::number(id_lib);
            }
            else
            {
                lib_url = QStringLiteral("/opds");
                id_lib = idCurrentLib;
            }
            strings.removeFirst();
            url.remove(0, 1);
            url = url.right(url.length() - url.indexOf('/'));
            opds = true;
        }
        else if(strings.at(1).startsWith(QLatin1String("http")))
        {
            url += QChar(u'/');
            if(strings.at(1).startsWith(QLatin1String("http_")))
            {
                id_lib = strings[1].replace(QLatin1String("http_"), QLatin1String("")).toInt();
                lib_url = QLatin1String("/http_") + QString::number(id_lib);
            }
            else
            {
                lib_url = QStringLiteral("/http");
                id_lib = idCurrentLib;
            }
            strings.removeFirst();
            url.remove(0, 1);
            url = url.right(url.length() - url.indexOf('/'));
            opds = false;
        }
    }
    SLib &lib = mLibs[id_lib];
    if(!lib.bLoaded)
        loadLibrary(id_lib);

    uint nPage = 0;
    if(!QFileInfo::exists(url))
    {
        if(strings.count() > 0)
        {
            QString last = strings.last();
            int pos = last.indexOf('?');
            if(pos >= 0)
            {
                QStringList str_params = last.right(last.length() - pos - 1).split(QStringLiteral("&"));
                foreach (QString str, str_params)
                {
                    int pos_eqv = str.indexOf('=');
                    if(pos_eqv > 0)
                        params.insert(str.left(pos_eqv), str.right(str.length() - pos_eqv - 1));
                    else
                        params.insert(str, QLatin1String(""));
                    if(str.left(pos_eqv) == QLatin1String("page"))
                        nPage = str.right(str.length() - pos_eqv - 1).toUInt();
                }
                strings.last() = strings.last().left(pos);
            }
            pos = url.indexOf('?');
            if(pos > 0)
            {
                url = url.left(pos);
            }
        }
    }

    QFileInfo fi(url);
    if(url == QLatin1String("/robots.txt") || url == QLatin1String("/robots.txt/"))
    {
        ts << WriteSuccess(OPDS_MIME_TYPE(QStringLiteral("txt")));
        ts.flush();
        ts << "User-agent: *" << char(10);
        ts << "Disallow: /";
        ts.flush();
    }
    else if((url.endsWith(QLatin1String("cover.jpg/"), Qt::CaseInsensitive) || url.endsWith(QLatin1String("cover.jpg"), Qt::CaseInsensitive)) && !QFileInfo::exists(url))
    {
        // проверить код
        QString id = strings[2];
        fb2mobi fb(pExportOptions_, id_lib);
        QString file = fb.convert(id.toLongLong());
        process(file, ts, session);
    }
    else if(fi.suffix().toLower() == QLatin1String("ico"))
    {
        QString ico = QLatin1String(":/xsl/opds/") + url;
        QFile file(ico);
        file.open(QFile::ReadOnly);
        ts << WriteSuccess(QStringLiteral("image/x-icon"));
        ts.flush();
        ts.device()->write(file.readAll());
    }
    else if(fi.suffix().toLower() == QLatin1String("png"))
    {
        QString ico = QLatin1String(":/xsl/opds/") + url;
        if(fi.exists())
            ico = fi.absoluteFilePath();
        QFile file(ico);
        file.open(QFile::ReadOnly);
        ts << WriteSuccess(QStringLiteral("image/png"));
        ts.flush();
        ts.device()->write(file.readAll());
    }
    else if(fi.suffix().toLower() == QLatin1String("jpg"))
    {
        QString ico = QLatin1String(":/xsl/opds/") + url;
        // проверить
            ico = fi.absoluteFilePath();
        QFile file(ico);
        file.open(QFile::ReadOnly);
        ts << WriteSuccess(QStringLiteral("image/jpeg"));
        ts.flush();
        ts.device()->write(file.readAll());
    }
    else if(url == QLatin1String("/"))
    {
        if(opds)
        {
            QDomElement feed = doc_header(session);
            AddTextNode(QStringLiteral("id"), QStringLiteral("tag:root"), feed);
            AddTextNode(QStringLiteral("title"), lib.name, feed);
            AddTextNode(QStringLiteral("updated"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate), feed);
            AddTextNode(QStringLiteral("icon"), QStringLiteral("/icon_256x256.png"), feed);
            QDomElement link = doc.createElement(QStringLiteral("link"));
            link.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/search?search_string={searchTerms}")
                              + (session.isEmpty() ?QString() :QLatin1String("&session=") + session));
            link.setAttribute(QStringLiteral("rel"), QStringLiteral("search"));
            link.setAttribute(QStringLiteral("type"), QStringLiteral("application/atom+xml"));
            feed.appendChild(link);

            QDomElement entry;
            QDomElement el;

            entry = doc.createElement(QStringLiteral("entry"));
            feed.appendChild(entry);
            AddTextNode(QStringLiteral("updated"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate), entry);
            AddTextNode(QStringLiteral("id"), QStringLiteral("tag:root:authors"), entry);
            AddTextNode(QStringLiteral("title"), tr("Books by authors"), entry);
            el = AddTextNode(QStringLiteral("content"), tr("Finding books by authors"), entry);
            el.setAttribute(QStringLiteral("type"), QStringLiteral("text"));
            el = AddTextNode(QStringLiteral("link"), QLatin1String(""), entry);
            el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/authorsindex") + (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
            el.setAttribute(QStringLiteral("type"), QStringLiteral("application/atom+xml;profile=opds-catalog"));

            entry = doc.createElement(QStringLiteral("entry"));
            feed.appendChild(entry);
            AddTextNode(QStringLiteral("updated"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate), entry);
            AddTextNode(QStringLiteral("id"), QStringLiteral("tag:root:sequences"), entry);
            AddTextNode(QStringLiteral("title"), tr("Books by sequences"), entry);
            el = AddTextNode(QStringLiteral("content"),tr("Finding books by sequences"), entry);
            el.setAttribute(QStringLiteral("type"), QStringLiteral("text"));
            el = AddTextNode(QStringLiteral("link"), QLatin1String(""),entry);
            el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/sequencesindex") + (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
            el.setAttribute(QStringLiteral("type"), QStringLiteral("application/atom+xml;profile=opds-catalog"));

            entry = doc.createElement(QStringLiteral("entry"));
            feed.appendChild(entry);
            AddTextNode(QStringLiteral("updated"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate), entry);
            AddTextNode(QStringLiteral("id"), QStringLiteral("tag:root:genre"), entry);
            AddTextNode(QStringLiteral("title"), tr("Books by genre"), entry);
            el=AddTextNode(QStringLiteral("content"), tr("Finding books by genre"), entry);
            el.setAttribute(QStringLiteral("type"), QStringLiteral("text"));
            el=AddTextNode(QStringLiteral("link"), QLatin1String(""), entry);
            el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/genres") + (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
            el.setAttribute(QStringLiteral("type"), QStringLiteral("application/atom+xml;profile=opds-catalog"));

            ts<<WriteSuccess(QStringLiteral("application/atom+xml;charset=utf-8"));
            ts<<doc.toString();
        }
        else
        {
            ts<<WriteSuccess();
            QDomElement feed = doc_header(session, true, lib.name, lib_url);
            QDomElement div = doc.createElement(QStringLiteral("DIV"));
            feed.appendChild(div);
            QDomElement el;

            el = AddTextNode(QStringLiteral("A"), tr("Finding books by authors"), div);
            el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/authorsindex") + (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
            div = doc.createElement(QStringLiteral("DIV"));
            feed.appendChild(div);
            el = AddTextNode(QStringLiteral("A"), tr("Finding books by sequences"), div);
            el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/sequencesindex") + (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
            div = doc.createElement(QStringLiteral("DIV"));
            feed.appendChild(div);
            el = AddTextNode(QStringLiteral("A"), tr("Finding books by genre"), div);
            el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/genres") + (session.isEmpty() ?QString() :QLatin1String("?session=") + session));

            QDomElement hr = doc.createElement(QStringLiteral("HR"));
            hr.setAttribute(QStringLiteral("size"), QStringLiteral("3"));
            hr.setAttribute(QStringLiteral("color"), QStringLiteral("black"));
            feed.appendChild(hr);

            QDomElement form = doc.createElement(QStringLiteral("FORM"));
            form.setAttribute(QStringLiteral("method"), QStringLiteral("get"));
            form.setAttribute(QStringLiteral("action"), QStringLiteral("search"));
            feed.appendChild(form);

            div = doc.createElement(QStringLiteral("DIV"));
            //div.setAttribute("class","book");
            form.appendChild(div);
            el = AddTextNode(QStringLiteral("div"), tr("Finding books by name/author: "), div);
            el.setAttribute(QStringLiteral("class"), QStringLiteral("book"));
            div.appendChild(el);

            el = doc.createElement(QStringLiteral("INPUT"));
            el.setAttribute(QStringLiteral("type"), QStringLiteral("text"));
            el.setAttribute(QStringLiteral("name"), QStringLiteral("search_string"));
            //el.setAttribute("class","book");
            div.appendChild(el);

            el = doc.createElement(QStringLiteral("INPUT"));
            el.setAttribute(QStringLiteral("type"), QStringLiteral("hidden"));
            el.setAttribute(QStringLiteral("name"), QStringLiteral("session"));
            el.setAttribute(QStringLiteral("value"), session);
            div.appendChild(el);

            el = doc.createElement(QStringLiteral("INPUT"));
            el.setAttribute(QStringLiteral("type"), QStringLiteral("submit"));
            el.setAttribute(QStringLiteral("value"), tr("Find"));
            div.appendChild(el);

            ts << "<!DOCTYPE html>";
            doc.namedItem(QStringLiteral("HTML")).save(ts, SAVE_INDEX);
        }
    }
    else if(url.startsWith(QLatin1String("/convert"), Qt::CaseInsensitive) && !opds)
    {
        QDomElement feed;
        feed = doc_header(session, true, lib.name, lib_url);
        ts << WriteSuccess();
        ts << "<!DOCTYPE html>";
        doc.namedItem(QStringLiteral("HTML")).save(ts, SAVE_INDEX);
    }
    else if(url.startsWith(QLatin1String("/search"), Qt::CaseInsensitive))
    {
        if(!params.contains(QStringLiteral("search_string")))
        {
            process(QStringLiteral("/"), ts, session);
            return;
        }
        if(params.value(QStringLiteral("search_string")).isEmpty())
        {
            process(QStringLiteral("/"), ts, session);
            return;
        }

        QList<uint> listBooks = book_list(lib, 0, 0, 0, QString(params.value(QStringLiteral("search_string"))).replace('+', ' ')
                                          .replace(QLatin1String("%20"), QLatin1String(" ")));
        ts << FillPage(listBooks, lib, tr("Books search"), lib_url, url, ts, opds, nPage, session, true);


    }
    else if(url.startsWith(QLatin1String("/sequencebooks"), Qt::CaseInsensitive) && strings.count() >= 3)
    {
        QString id_sequence = strings.at(2);
        QList<uint> listBooks = book_list(lib, 0, id_sequence.toUInt(), 0, QLatin1String(""));
        ts << FillPage(listBooks, lib, tr("Books of sequence") + QLatin1String(" (") + lib.mSerials[id_sequence.toUInt()].sName + QLatin1String(")"), lib_url, url, ts, opds, nPage, session, false);

    }
    else if(url.startsWith(QLatin1String("/sequencesindex"), Qt::CaseInsensitive))
    {
        QString index = QLatin1String("");
        if(strings.count() > 2)
            index = strings.at(2);
        bool by_books = false;
        if(strings.count() > 3)
        {
            by_books = strings.at(3) == QLatin1String("books");
        }
        QDomElement feed;
        if(opds)
        {
            feed = doc_header(session);
            AddTextNode(QStringLiteral("title"), tr("Books by sequences"), feed);
            AddTextNode(QStringLiteral("updated"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate), feed);
            AddTextNode(QStringLiteral("icon"), QStringLiteral("/icon_256x256.png"), feed);
            ts << WriteSuccess(QStringLiteral("application/atom+xml;charset=utf-8"));
        }
        else
        {
            feed = doc_header(session, true, lib.name, lib_url);
            ts << WriteSuccess();
        }
        auto iSerial = lib.mSerials.constBegin();
        QMap <QString, int> mCount;
        QSet <QString> setSerials;
        int count = 0;
        while(iSerial != lib.mSerials.constEnd()){
            if(iSerial->sName.left(index.length()).toLower() == index.toLower()){
                count++;
                QString sNewIndex = iSerial->sName.left(index.length()+1).toLower();
                sNewIndex[0] = sNewIndex[0].toUpper();
                if(mCount.contains(sNewIndex))
                    mCount[sNewIndex]++;
                else
                    mCount[sNewIndex] = 1;
                if(sNewIndex.length() == iSerial->sName.length())
                    setSerials.insert(sNewIndex);
            }
            ++iSerial;
        }
        QList<QString> listKeys = mCount.keys();
        std::sort(listKeys.begin(), listKeys.end(), [](const QString& lhs, const QString& rhs) {return QString::localeAwareCompare(lhs, rhs) < 0;});

        if(count > 30 && !by_books)
        {
            QDomElement tag_table;
            QDomElement tag_tr;
            int current_column = 0;
            if(!opds)
            {
                tag_table = doc.createElement(QStringLiteral("TABLE"));
                feed.appendChild(tag_table);
            }
            foreach(const QString &iIndex, listKeys)
            {
                if(iIndex.trimmed().isEmpty() || iIndex[0] == '\0')
                    continue;
                if(opds)
                {

                    QDomElement entry = doc.createElement(QStringLiteral("entry"));
                    feed.appendChild(entry);
                    AddTextNode(QStringLiteral("updated"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate), entry);
                    AddTextNode(QStringLiteral("id"), QLatin1String("tag:sequences:") + iIndex, entry);
                    AddTextNode(QStringLiteral("title"), iIndex, entry);
                    QDomElement el=AddTextNode(QStringLiteral("content"), QString::number(mCount[iIndex]) + QLatin1String(" ") + tr("series beginning with") +
                                               QLatin1String(" '") + iIndex + QLatin1String("'"), entry);
                    el.setAttribute(QStringLiteral("type"), QStringLiteral("text"));
                    el=AddTextNode(QStringLiteral("link"), QLatin1String(""), entry);
                    if(mCount[iIndex] == 1)
                    {
                        auto iSerial = lib.mSerials.constBegin();
                        while(iSerial != lib.mSerials.constEnd()){
                            if(setSerials.contains(iIndex) ?iSerial.value().sName.toLower() == iIndex.toLower()
                                    :iSerial.value().sName.left(iIndex.size()).toLower() == iIndex.toLower())
                            {
                                el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/sequencebooks/") + QString::fromLatin1(QUrl::toPercentEncoding(QString::number(iSerial.key())))
                                                + (session.isEmpty() && mCount[iIndex]<30 ?QString() :QLatin1String("?session=") + session));
                                break;
                            }
                            ++iSerial;
                        }

                    }
                    else
                    {
                        el.setAttribute(QStringLiteral("href"),lib_url + QLatin1String("/sequencesindex/") +  QString::fromLatin1(QUrl::toPercentEncoding(iIndex).constData())
                                        + (setSerials.contains(iIndex) ?QStringLiteral("/books") :QString()) + (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
                    }
                    el.setAttribute(QStringLiteral("type"), QStringLiteral("application/atom+xml;profile=opds-catalog"));
                }
                else
                {

                    if(current_column == 0)
                    {
                        tag_tr = doc.createElement(QStringLiteral("tr"));
                        tag_table.appendChild(tag_tr);
                    }
                    QDomElement td = doc.createElement(QStringLiteral("td"));
                    tag_tr.appendChild(td);

                    QDomElement div = doc.createElement(QStringLiteral("DIV"));
                    div.setAttribute(QStringLiteral("class"), QStringLiteral("author"));
                    td.appendChild(div);
                    QDomElement el = AddTextNode(QStringLiteral("a"), iIndex, div);
                    el.setAttribute(QStringLiteral("class"), QStringLiteral("block"));
                    AddTextNode(QStringLiteral("div"), QString::number(mCount[iIndex]) + QLatin1String(" ") + tr("series beginning with") + QLatin1String(" '") +
                                iIndex + QLatin1String("'"), div);
                    if(mCount[iIndex] == 1)
                    {
                        auto iSerial = lib.mSerials.constBegin();
                        while(iSerial != lib.mSerials.constEnd()){
                            if(setSerials.contains(iIndex) ?iSerial.value().sName.toLower() == iIndex.toLower()
                                                           :iSerial.value().sName.left(iIndex.size()).toLower() == iIndex.toLower())
                            {
                                el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/sequencebooks/") + QString::fromLatin1(QUrl::toPercentEncoding(QString::number(iSerial.key()))) +
                                                (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
                                break;
                            }
                            ++iSerial;
                        }

                    }
                    else
                    {
                        el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/sequencesindex/") + QString::fromLatin1(QUrl::toPercentEncoding(iIndex).constData()) +
                                        (setSerials.contains(iIndex) && mCount[iIndex]<30 ?QLatin1String("/books") :QLatin1String("")) + (session.isEmpty() ?QString() :QLatin1String("?session=") + session));

                    }
                    current_column++;
                    if(current_column == MAX_COLUMN_COUNT)
                        current_column = 0;
                }
            }
            if(!opds)
                while(current_column < MAX_COLUMN_COUNT)
                {
                    AddTextNode(QStringLiteral("td"), QLatin1String(""), tag_tr);
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
            std::sort(listSerialId.begin(), listSerialId.end(),[lib](uint lhs, uint rhs) {return lib.mSerials[lhs].sName < lib.mSerials[rhs].sName;});

            foreach(const auto &iIndex, listSerialId)
            {
                uint nBooksCount = 0;
                auto iBook = lib.mBooks.constBegin();
                while(iBook != lib.mBooks.constEnd())
                {
                    if(iBook->idSerial == iIndex)
                        nBooksCount++;
                    ++iBook;
                }
                if(opds)
                {
                    QDomElement entry = doc.createElement(QStringLiteral("entry"));
                    feed.appendChild(entry);
                    AddTextNode(QStringLiteral("updated"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate), entry);
                    AddTextNode(QStringLiteral("id"),"tag:sequences:" + QString::number(iIndex), entry);
                    AddTextNode(QStringLiteral("title"), lib.mSerials[iIndex].sName, entry);
                    QDomElement el = AddTextNode(QStringLiteral("content"), QString::number(nBooksCount) + QLatin1String(" ") + tr("books"), entry);
                    el.setAttribute(QStringLiteral("type"), QStringLiteral("text"));
                    el = AddTextNode(QStringLiteral("link"), QLatin1String(""), entry);
                    el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/sequencebooks/") + QString::fromLatin1(QUrl::toPercentEncoding(QString::number(iIndex)).constData()) +
                                    (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
                    el.setAttribute(QStringLiteral("type"), QStringLiteral("application/atom+xml;profile=opds-catalog"));
                }
                else
                {
                    QDomElement div = doc.createElement(QStringLiteral("DIV"));
                    div.setAttribute(QStringLiteral("class"), QStringLiteral("author"));
                    feed.appendChild(div);
                    QDomElement el = AddTextNode(QStringLiteral("a"), lib.mSerials[iIndex].sName, div);
                    el.setAttribute(QStringLiteral("class"), QStringLiteral("block"));
                    el.setAttribute(QStringLiteral("href"),lib_url + QLatin1String("/sequencebooks/") + QString::fromLatin1(QUrl::toPercentEncoding(QString::number(iIndex)).constData()) +
                                    (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
                    AddTextNode(QStringLiteral("div"), QString::number(nBooksCount) + QLatin1String(" ") + tr("books"), div);
                }
            }
        }
        if(opds)
            ts << doc.toString();
        else
            doc.namedItem(QStringLiteral("HTML")).save(ts, SAVE_INDEX);
    }
    else if(url.startsWith(QLatin1String("/genres"), Qt::CaseInsensitive))
    {
        QList<uint> listIdGenres;
        QMap<uint, uint> mCounts;
        if(strings.count() > 2)
        {
            uint idParrentGenre = strings.at(2).toUInt();
            if(mGenre[idParrentGenre].idParrentGenre > 0){
                QList<uint> listBooks = book_list(lib, 0, 0, idParrentGenre, QLatin1String(""));
                ts << FillPage(listBooks, lib, tr("Books by ABC"), lib_url, url, ts, opds, nPage, session, true);
                return;
            }
            auto iGenre = mGenre.constBegin();
            while(iGenre != mGenre.constEnd()){
                if(iGenre->idParrentGenre == idParrentGenre)
                    listIdGenres << iGenre.key();
                ++iGenre;
            }
            auto iBook = lib.mBooks.constBegin();
            while(iBook != lib.mBooks.constEnd()){
                foreach (uint iGenre, iBook->listIdGenres){
                    if(mGenre[iGenre].idParrentGenre == idParrentGenre){
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
            while(iGenre != mGenre.constEnd()){
                if(iGenre->idParrentGenre == 0)
                    listIdGenres << iGenre.key();
                ++iGenre;
            }
        }
        QDomElement feed;
        if(opds)
        {
            feed = doc_header(session);
            AddTextNode(QStringLiteral("title"), tr("Books by genre"), feed);
            AddTextNode(QStringLiteral("updated"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate), feed);
            AddTextNode(QStringLiteral("icon"), QStringLiteral("/icon_256x256.png"), feed);
            ts << WriteSuccess(QStringLiteral("application/atom+xml;charset=utf-8"));
        }
        else
        {
            feed = doc_header(session, true, lib.name, lib_url);
            ts << WriteSuccess();
        }
        foreach(uint idGenre, listIdGenres)
        {
            if(opds)
            {
                QDomElement entry = doc.createElement(QStringLiteral("entry"));
                feed.appendChild(entry);
                AddTextNode(QStringLiteral("updated"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate), entry);
                AddTextNode(QStringLiteral("id"), QLatin1String("tag:root:genre:") + mGenre[idGenre].sName, entry);
                AddTextNode(QStringLiteral("title"), mGenre[idGenre].sName, entry);
                if(strings.count() > 2)
                {
                    QDomElement el = AddTextNode(QStringLiteral("content"), QString::number(mCounts[idGenre]) + QLatin1String(" ") + tr("books"), entry);
                    el.setAttribute(QStringLiteral("type"), QStringLiteral("text"));
                }
                else
                {
                    QDomElement el = AddTextNode(QStringLiteral("content"), tr("Books of genre") + QLatin1String(" ") + mGenre[idGenre].sName, entry);
                    el.setAttribute(QStringLiteral("type"), QStringLiteral("text"));
                }
                QDomElement el=AddTextNode(QStringLiteral("link"),QLatin1String(""),entry);
                el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/genres/") + QString::fromLatin1(QUrl::toPercentEncoding(QString::number(idGenre))) +
                                (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
                        el.setAttribute(QStringLiteral("type"), QStringLiteral("application/atom+xml;profile=opds-catalog"));

            }
            else
            {
                QDomElement div = doc.createElement(QStringLiteral("DIV"));
                feed.appendChild(div);
                div.setAttribute(QStringLiteral("class"), QStringLiteral("author"));
                QDomElement el = AddTextNode(QStringLiteral("A"), mGenre[idGenre].sName,div);
                el.setAttribute(QStringLiteral("class"), QStringLiteral("block"));
                el.setAttribute(QStringLiteral("href"), QLatin1String("/genres/") + QString::number(idGenre) + (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
                if(strings.count() > 2)
                {
                    QDomElement el = AddTextNode(QStringLiteral("div"), QString::number(mCounts[idGenre]) + QLatin1String(" ") + tr("books"), div);
                }
            }
        }
        if(opds)
            ts << doc.toString();
        else
            doc.namedItem(QStringLiteral("HTML")).save(ts, SAVE_INDEX);
        qDebug() << doc.toString();
    }
    else if(url.startsWith(QLatin1String("/authorsindex"), Qt::CaseInsensitive))
    {
        QString index;;
        if(strings.count() > 2)
            index = strings.at(2);
        bool by_books = false;
        if(strings.count() > 3)
        {
            by_books = strings.at(3) == QLatin1String("books");
        }
        QDomElement feed;
        if(opds)
        {
            feed = doc_header(session);
            AddTextNode(QStringLiteral("title"), tr("Books by authors"), feed);
            AddTextNode(QStringLiteral("updated"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate), feed);
            AddTextNode(QStringLiteral("icon"), QStringLiteral("/icon_256x256.png"), feed);
            ts << WriteSuccess(QStringLiteral("application/atom+xml;charset=utf-8"));
        }
        else
        {
            feed = doc_header(session, true, lib.name, lib_url);
            ts << WriteSuccess();
        }
        auto iAuthor = lib.mAuthors.constBegin();
        QMap<QString, int> mCount;

        QSet <QString> setAuthors;
        int count = 0;
        while(iAuthor != lib.mAuthors.constEnd())
        {
            if(iAuthor->getName().left(index.length()).toLower() == index.toLower())
            {
                count++;
                QString sNewIndex = iAuthor->getName().left(index.length() + 1).toLower();
                sNewIndex[0] = sNewIndex[0].toUpper();
                if(mCount.contains(sNewIndex))
                    mCount[sNewIndex]++;
                else
                    mCount[sNewIndex] = 1;
                if(sNewIndex.length() == iAuthor->getName().length())
                    setAuthors.insert(sNewIndex);
            }
            ++iAuthor;
        }
        QList<QString> listKeys = mCount.keys();
        std::sort(listKeys.begin(), listKeys.end(), [](const QString& lhs, const QString& rhs) {return QString::localeAwareCompare(lhs, rhs) < 0;});

        if(count>30 && !by_books)
        {
            QDomElement tag_table;
            QDomElement tag_tr;
            int current_column = 0;
            if(!opds)
            {
                tag_table = doc.createElement(QStringLiteral("TABLE"));
                feed.appendChild(tag_table);
            }

            foreach(const QString &iIndex, listKeys)
            {
                if(opds)
                {
                    QDomElement entry = doc.createElement(QStringLiteral("entry"));
                    feed.appendChild(entry);
                    AddTextNode(QStringLiteral("updated"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate), entry);
                    AddTextNode(QStringLiteral("id"), QLatin1String("tag:authors:") + iIndex, entry);
                    AddTextNode(QStringLiteral("title"), iIndex, entry);
                    QDomElement el = AddTextNode(QStringLiteral("content"), QString::number(mCount[iIndex]) + QLatin1String(" ") + tr("authors beginning with") +
                                                 QLatin1String(" '") + iIndex + QLatin1String("'"), entry);
                    el.setAttribute(QStringLiteral("type"), QStringLiteral("text"));
                    el = AddTextNode(QStringLiteral("link"), QLatin1String(""), entry);
                    if(mCount[iIndex] == 1)
                    {
                        iAuthor = lib.mAuthors.constBegin();
                        while(iAuthor != lib.mAuthors.constEnd())
                        {
                            if(setAuthors.contains(iIndex) ?iAuthor.value().getName().toLower() == iIndex.toLower()
                                    :iAuthor.value().getName().left(iIndex.size()).toLower() == iIndex.toLower())
                            {
                                el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/author/") + QString::fromLatin1(QUrl::toPercentEncoding(QString::number(iAuthor.key()))) +
                                                (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
                                break;
                            }
                            ++iAuthor;
                        }
                    }
                    else
                    {
                        el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/authorsindex/") + QString::fromLatin1(QUrl::toPercentEncoding(iIndex).constData()) +
                                        (setAuthors.contains(iIndex) && mCount[iIndex]<30 ?QStringLiteral("/books") :QString()) + (session.isEmpty() ?QString() :QLatin1String("?session=") + session));

                    }
                    el.setAttribute(QStringLiteral("type"), QStringLiteral("application/atom+xml;profile=opds-catalog"));
                }
                else
                {
                    if(current_column == 0)
                    {
                        tag_tr = doc.createElement(QStringLiteral("tr"));
                        tag_table.appendChild(tag_tr);
                    }
                    QDomElement td = doc.createElement(QStringLiteral("td"));
                    tag_tr.appendChild(td);
                    QDomElement div = doc.createElement(QStringLiteral("div"));
                    div.setAttribute(QStringLiteral("class"), QStringLiteral("author"));
                    td.appendChild(div);
                    QDomElement el = AddTextNode(QStringLiteral("a"), iIndex, div);
                    el.setAttribute(QStringLiteral("class"), QStringLiteral("block"));
                    AddTextNode(QStringLiteral("div"), QString::number(mCount[iIndex]) + QLatin1String(" ") + tr("authors beginning with") + QLatin1String(" '") +
                                iIndex + QLatin1String("'"), div);
                    if(mCount[iIndex] == 1)
                    {
                        iAuthor = lib.mAuthors.constBegin();
                        while(iAuthor != lib.mAuthors.constEnd())
                        {
                            if(setAuthors.contains(iIndex) ?iAuthor.value().getName().toLower() == iIndex.toLower()
                                    :iAuthor.value().getName().left(iIndex.size()).toLower() == iIndex.toLower())
                            {
                                el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/author/") + QString::fromLatin1(QUrl::toPercentEncoding(QString::number(iAuthor.key()))) +
                                                (session.isEmpty() && mCount[iIndex]<30 ?QString() :QLatin1String("?session=") + session));
                                break;
                            }
                            ++iAuthor;
                        }
                    }
                    else
                    {
                        el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/authorsindex/") + QString::fromLatin1(QUrl::toPercentEncoding(iIndex).constData()) +
                                        (setAuthors.contains(iIndex) ?QStringLiteral("/books") :QString()) + (session.isEmpty() ?QString() :QLatin1String("?session=") + session));

                    }

                    current_column++;
                    if(current_column == MAX_COLUMN_COUNT)
                        current_column = 0;
                }
            }
            if(!opds)
            {
                while(current_column < MAX_COLUMN_COUNT)
                {
                    AddTextNode(QStringLiteral("td"), QLatin1String(""), tag_tr);
                    current_column++;
                }
            }
        }
        else
        {
            QList<uint> listAuthorId;
            iAuthor = lib.mAuthors.constBegin();
            while(iAuthor != lib.mAuthors.constEnd())
            {
                if(iAuthor->getName().left(index.length()).toLower() == index.toLower())
                {
                    listAuthorId << iAuthor.key();
                }
                ++iAuthor;
            }
            std::sort(listAuthorId.begin(),listAuthorId.end(),[lib](uint lhs,uint rhs) {return lib.mAuthors[lhs].getName() < lib.mAuthors[rhs].getName();});
            foreach(uint iIndex, listAuthorId)
            {
                uint nBooksCount = 0;
                QMultiHash<uint,uint>::const_iterator i = lib.mAuthorBooksLink.constFind(iIndex);
                while(i != lib.mAuthorBooksLink.constEnd() && i.key() == iIndex){
                    nBooksCount++;
                    ++i;
                }

                if(opds)
                {
                    QDomElement entry = doc.createElement(QStringLiteral("entry"));
                    feed.appendChild(entry);
                    AddTextNode(QStringLiteral("updated"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate), entry);
                    AddTextNode(QStringLiteral("id"), QLatin1String("tag:author:") + QString::number(iIndex), entry);
                    AddTextNode(QStringLiteral("title"), lib.mAuthors[iIndex].getName(), entry);
                    QDomElement el=AddTextNode(QStringLiteral("content"), QString::number(nBooksCount) + QLatin1String(" ") + tr("books"), entry);
                    el.setAttribute(QStringLiteral("type"), QStringLiteral("text"));
                    el=AddTextNode(QStringLiteral("link"), QLatin1String(""), entry);
                    el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/author/") + QString::fromLatin1(QUrl::toPercentEncoding(QString::number(iIndex)).constData()) +
                                    (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
                    el.setAttribute(QStringLiteral("type"), QStringLiteral("application/atom+xml;profile=opds-catalog"));
                }
                else
                {
                    QDomElement div = doc.createElement(QStringLiteral("DIV"));
                    div.setAttribute(QStringLiteral("class"), QStringLiteral("author"));
                    feed.appendChild(div);
                    QDomElement el = AddTextNode(QStringLiteral("a"),lib.mAuthors[iIndex].getName(),div);
                    el.setAttribute(QStringLiteral("class"), QStringLiteral("block"));
                    el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/author/") +
                                    QString::fromLatin1(QUrl::toPercentEncoding(QString::number(iIndex)).constData()) + (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
                    AddTextNode(QStringLiteral("div"), QString::number(nBooksCount) + QLatin1String(" ") + tr("books"), div);
                }
            }

        }

        if(opds)
            ts << doc.toString();
        else
            doc.namedItem(QStringLiteral("HTML")).save(ts, SAVE_INDEX);
    }
    else if(url.startsWith(QLatin1String("/authorsequenceless/"), Qt::CaseInsensitive) && strings.count() >= 3)
    {
        uint idAuthor = strings.at(2).toUInt();

        QList<uint> listBooks = book_list(lib, idAuthor, 0, 0, QLatin1String(""), true);
        ts<<FillPage(listBooks, lib, tr("Books without sequence") + QLatin1String(" (") + lib.mAuthors[idAuthor].getName() + QLatin1String(")"), lib_url, url, ts, opds, nPage, session, false);

    }
    else if(url.startsWith(QLatin1String("/authorbooks/"), Qt::CaseInsensitive) && strings.count() >= 3)
    {
        uint idAuthor = strings.at(2).toUInt();
        QList<uint> listBooks = book_list(lib,idAuthor, 0, 0, QLatin1String(""), false);
        ts << FillPage(listBooks,lib,tr("Books by ABC") + QLatin1String(" (") + lib.mAuthors[idAuthor].getName() + QLatin1String(")"), lib_url, url,ts, opds, nPage, session, false);
    }
    else if(url.startsWith(QLatin1String("/authorsequences/"), Qt::CaseInsensitive) && strings.count() >= 3)
    {
        QString id = strings.at(2);
        uint idAuthor = id.toUInt();
        QDomElement feed;
        if(opds)
        {
            feed = doc_header(session);
            AddTextNode(QStringLiteral("id"), QLatin1String("tag:author:") + id, feed);
            AddTextNode(QStringLiteral("title"), tr("Book sequences") + QLatin1String(" ") + lib.mAuthors[idAuthor].getName(), feed);
            AddTextNode(QStringLiteral("updated"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate), feed);
            AddTextNode(QStringLiteral("icon"), QStringLiteral("/icon_256x256.png"), feed);
        }
        else
        {
             feed = doc_header(session, true, lib.name, lib_url);
             QDomElement div = AddTextNode(QStringLiteral("DIV"), tr("Book sequences") + QLatin1String(" ") + lib.mAuthors[idAuthor].getName(), feed);
             div.setAttribute(QStringLiteral("class"), QStringLiteral("caption"));
        }

        QMultiHash<uint,uint>::const_iterator i = lib.mAuthorBooksLink.constFind(idAuthor);
        QMap<uint, uint> mapCountBooks;
        while(i != lib.mAuthorBooksLink.constEnd() && i.key() == idAuthor){
            SBook& book = lib.mBooks[i.value()];
            if(book.idSerial > 0){
                if(mapCountBooks.contains(book.idSerial))
                    mapCountBooks[book.idSerial]++;
                else
                    mapCountBooks[book.idSerial] = 1;
            }
            ++i;
        }
        QList<uint> listSerials = mapCountBooks.keys();
        std::sort(listSerials.begin(), listSerials.end(), [lib](uint lhs,uint rhs) {return QString::localeAwareCompare(lib.mSerials[lhs].sName, lib.mSerials[rhs].sName) < 0;});

        foreach(uint idSerial, listSerials)
        {
            if(opds)
            {
                QDomElement entry = doc.createElement(QStringLiteral("entry"));
                feed.appendChild(entry);
                AddTextNode(QStringLiteral("updated"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate), entry);
                AddTextNode(QStringLiteral("id"), QLatin1String("tag:author:") + id + QLatin1String(":sequence:") + QString::number(idSerial), entry);
                AddTextNode(QStringLiteral("title"), lib.mSerials[idSerial].sName, entry);
                QDomElement el=AddTextNode(QStringLiteral("content"), QString::number(mapCountBooks[idSerial]) + QLatin1String(" ") + tr("books in sequence"), entry);
                el.setAttribute(QStringLiteral("type"), QStringLiteral("text"));
                el=AddTextNode(QStringLiteral("link"), QLatin1String(""), entry);
                el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/authorsequence/") + QString::fromLatin1(QUrl::toPercentEncoding(id)) +
                                QLatin1String("/") + QString::fromLatin1(QUrl::toPercentEncoding(QString::number(idSerial))) +
                                (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
                el.setAttribute(QStringLiteral("type"), QStringLiteral("application/atom+xml;profile=opds-catalog"));
            }
            else
            {
                QDomElement entry = doc.createElement(QStringLiteral("div"));
                entry.setAttribute(QStringLiteral("class"), QStringLiteral("author"));
                feed.appendChild(entry);
                QDomElement el = AddTextNode(QStringLiteral("a"), lib.mSerials[idSerial].sName, entry);
                el.setAttribute(QStringLiteral("class"), QStringLiteral("block"));
                el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/authorsequence/") + QString::fromLatin1(QUrl::toPercentEncoding(id)) + QLatin1String("/") +
                                QString::fromLatin1(QUrl::toPercentEncoding(QString::number(idSerial))) +
                                (session.isEmpty() ?QString() :("?session=") + session));
                AddTextNode(QStringLiteral("div"), QString::number(mapCountBooks[idSerial]) + QLatin1String(" ") + tr("books in sequence"), entry);
            }
        }
        if(opds)
        {
            ts << WriteSuccess(QStringLiteral("application/atom+xml;charset=utf-8"));
            ts << doc.toString();
        }
        else
        {
            ts << WriteSuccess();
            doc.namedItem(QStringLiteral("HTML")).save(ts, SAVE_INDEX);
        }

    }
    else if(url.startsWith(QLatin1String("/authorsequence/"), Qt::CaseInsensitive) && strings.count() >=4 )
    {
        QString id_author = strings.at(2);
        QString id_sequence = strings.at(3);
        uint idSequence = id_sequence.toUInt();
        QList<uint> listBooks = book_list(lib,id_author.toUInt(), idSequence, 0, QLatin1String(""));
        ts << FillPage(listBooks, lib, tr("Books of sequence") + QLatin1String(" (") + lib.mSerials[idSequence].sName + QLatin1String(")"), lib_url, url, ts, opds, nPage, session, false);
    }
    else if(url.startsWith(QLatin1String("/author/"), Qt::CaseInsensitive) && strings.count() >= 3)
    {
        QString sIdAuthor = strings.at(2);
        uint idAuthor = sIdAuthor.toUInt();
        QString sAuthor = lib.mAuthors[idAuthor].getName();
        if(opds)
        {
            QDomElement feed = doc_header(session);
            AddTextNode(QStringLiteral("id"), QLatin1String("tag:author:") + sIdAuthor/*query.value(0).toString()*/,feed);
            AddTextNode(QStringLiteral("title"), tr("Books by") + QLatin1String(" ") + sAuthor,feed);
            AddTextNode(QStringLiteral("updated"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate), feed);
            AddTextNode(QStringLiteral("icon"),QLatin1String("/icon_256x256.png") +(session.isEmpty()?QString():QLatin1String("?session=") + session), feed);

            QDomElement entry = doc.createElement(QStringLiteral("entry"));
            feed.appendChild(entry);
            AddTextNode(QStringLiteral("updated"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate), entry);
            AddTextNode(QStringLiteral("id"), QStringLiteral("tag:author:") + sIdAuthor + QLatin1String(":sequences"), entry);
            AddTextNode(QStringLiteral("title"), tr("Books by sequences"), entry);
            QDomElement el = AddTextNode(QStringLiteral("link"), QLatin1String(""), entry);
            el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/authorsequences/") + QString::fromLatin1(QUrl::toPercentEncoding(sIdAuthor).constData()) +
                            (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
            el.setAttribute(QStringLiteral("type"), QStringLiteral("application/atom+xml;profile=opds-catalog"));

            entry=doc.createElement(QStringLiteral("entry"));
            feed.appendChild(entry);
            AddTextNode(QStringLiteral("updated"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate), entry);
            AddTextNode(QStringLiteral("id"), QLatin1String("tag:author:") + sIdAuthor + QLatin1String(":sequenceless"), entry);
            AddTextNode(QStringLiteral("title"),tr("Books without sequence"), entry);
            el=AddTextNode(QStringLiteral("link"), QLatin1String(""), entry);
            el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/authorsequenceless/") + QString::fromLatin1(QUrl::toPercentEncoding(sIdAuthor).constData()) +
                            (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
            el.setAttribute(QStringLiteral("type"), QStringLiteral("application/atom+xml;profile=opds-catalog"));

            entry = doc.createElement(QStringLiteral("entry"));
            feed.appendChild(entry);
            AddTextNode(QStringLiteral("updated"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate),entry);
            AddTextNode(QStringLiteral("id"), QLatin1String("tag:author:") + sIdAuthor + QLatin1String(":sequences"), entry);
            AddTextNode(QStringLiteral("title"), tr("All books"), entry);
            el=AddTextNode(QStringLiteral("link"), QLatin1String(""), entry);
            el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/authorbooks/") + QString::fromLatin1(QUrl::toPercentEncoding(sIdAuthor).constData()) +
                            (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
            el.setAttribute(QStringLiteral("type"), QStringLiteral("application/atom+xml;profile=opds-catalog"));

            ts<<WriteSuccess(QStringLiteral("application/atom+xml;charset=utf-8"));
            ts<<doc.toString();
        }
        else
        {
            QDomElement feed = doc_header(session, true, lib.name, lib_url);
            QDomElement div_auth = doc.createElement(QStringLiteral("DIV"));;
            div_auth.setAttribute(QStringLiteral("class"), QStringLiteral("author"));
            feed.appendChild(div_auth);
            QDomElement div_caption = AddTextNode(QStringLiteral("div"), tr("Books by") + QLatin1String(" ") + sAuthor, div_auth);
            div_caption.setAttribute(QStringLiteral("class"), QStringLiteral("caption"));

            QDomElement div = doc.createElement(QStringLiteral("DIV"));
            div_auth.appendChild(div);
            QDomElement el = AddTextNode(QStringLiteral("a"), tr("Books by sequences"), div);
            el.setAttribute(QStringLiteral("class"), QStringLiteral("block"));
            el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/authorsequences/") + QString::fromLatin1(QUrl::toPercentEncoding(sIdAuthor).constData()) +
                            (session.isEmpty() ?QString() :QLatin1String("?session=") + session));

            div = doc.createElement(QStringLiteral("DIV"));
            div_auth.appendChild(div);
            el = AddTextNode(QStringLiteral("a"), tr("Books without sequence"), div);
            el.setAttribute(QStringLiteral("class"), QStringLiteral("block"));
            el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/authorsequenceless/") + QString::fromLatin1(QUrl::toPercentEncoding(sIdAuthor).constData()) +
                            (session.isEmpty() ?QString() :QLatin1String("?session=") + session));


            div = doc.createElement(QStringLiteral("DIV"));
            div_auth.appendChild(div);
            el = AddTextNode(QStringLiteral("a"), tr("All books"), div);
            el.setAttribute(QStringLiteral("class"), QStringLiteral("block"));
            el.setAttribute(QStringLiteral("href"),lib_url + QLatin1String("/authorbooks/") + QString::fromLatin1(QUrl::toPercentEncoding(sIdAuthor).constData()) +
                            (session.isEmpty() ?QString() :QLatin1String("?session=") + session));

            ts<<WriteSuccess();
            doc.namedItem(QStringLiteral("HTML")).save(ts, SAVE_INDEX);
        }
    }
    else if(url.startsWith(QLatin1String("/book/"), Qt::CaseInsensitive) && strings.count() >= 4)
    {
        QString id = strings.at(2);
        uint idBook = id.toUInt();
        QString format = strings.at(3);
        convert(id_lib, idBook, format, QLatin1String(""), opds, ts);
    }
}

QString opds_server::WriteSuccess(const QString &contentType, bool isGZip)
{
    return QLatin1String("HTTP/1.1 200 OK\n"
            "Server: freeLib ") + QLatin1String(FREELIB_VERSION) + QLatin1String("\n"
            "Content-Type: ") + contentType + QLatin1String("\n"
            "Pragma: no-cache\n"
            "Accept-Ranges: bytes\n") +
            (isGZip?"Content-Encoding: gzip\n":"") +
//            "Set-Cookie: PWDCHECK=1;\n"+
//            "Connection: close\n\n";
            QLatin1String("Connection: Keep-Alive\n\n");
}
#endif

void opds_server::stop_server()
{
    if(OPDS_server_status == 1)
    {
        OPDS_server_status = 0;
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
        auto listTcpServsrs = httpServer_.servers();
        foreach (auto server, listTcpServsrs) {
            server->close();
        }
#else
        auto i = OPDS_clients.constBegin();
        while(i != OPDS_clients.constEnd())
        {
            i.value()->close();
            ++i;
        }
        OPDS_clients.clear();
        OPDS_server.close();
#endif
    }
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
QHttpServerResponse opds_server::rootHTTP(uint idLib, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QUrlQuery urlquery(url);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSesionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    QString sLibUrl;
    const SLib &lib = getLib(idLib, u"/http"_s, &sLibUrl);

    QDomElement feed = docHeaderHTTP(sSesionQuery, lib.name, sLibUrl);
    QDomElement div = doc.createElement(u"DIV"_s);
    feed.appendChild(div);
    QDomElement el;

    el = AddTextNode(u"A"_s, tr("Finding books by authors"), div);
    el.setAttribute(u"href"_s, sLibUrl + u"/authorsindex"_s + sSesionQuery );
    div = doc.createElement(u"DIV"_s);
    feed.appendChild(div);
    el = AddTextNode(u"A"_s, tr("Finding books by sequences"), div);
    el.setAttribute(u"href"_s, sLibUrl + u"/sequencesindex"_s + sSesionQuery);
    div = doc.createElement(u"DIV"_s);
    feed.appendChild(div);
    el = AddTextNode(u"A"_s, tr("Finding books by genre"), div);
    el.setAttribute(u"href"_s, sLibUrl + u"/genres"_s + sSesionQuery);

    QDomElement hr = doc.createElement(u"HR"_s);
    hr.setAttribute(u"size"_s, u"3"_s);
    hr.setAttribute(u"color"_s, u"black"_s);
    feed.appendChild(hr);

    QDomElement form = doc.createElement(u"FORM"_s);
    form.setAttribute(u"method"_s, u"get"_s);
    form.setAttribute(u"action"_s, sLibUrl + u"/search"_s);
    feed.appendChild(form);

    div = doc.createElement(u"DIV"_s);
    //div.setAttribute("class","book");
    form.appendChild(div);
    el = AddTextNode(u"div"_s, tr("Finding books by name/author: "), div);
    el.setAttribute(u"class"_s, u"book"_s);
    div.appendChild(el);

    el = doc.createElement(u"INPUT"_s);
    el.setAttribute(u"type"_s, u"text"_s);
    el.setAttribute(u"name"_s, u"search_string"_s);
    //el.setAttribute("class","book");
    div.appendChild(el);

    el = doc.createElement(u"INPUT"_s);
    el.setAttribute(u"type"_s, u"hidden"_s);
    el.setAttribute(u"name"_s, u"session"_s);
    //el.setAttribute(QStringLiteral("value"), session);
    div.appendChild(el);

    el = doc.createElement(u"INPUT"_s);
    el.setAttribute(u"type"_s, u"submit"_s);
    el.setAttribute(u"value"_s, tr("Find"));
    div.appendChild(el);
    //    }

    return responseHTTP();
}

QHttpServerResponse opds_server::rootOPDS(uint idLib, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QUrlQuery urlquery(url);
    QString sLibUrl;
    const SLib &lib = getLib(idLib, u"/opds"_s, &sLibUrl);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSesionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    QDomElement feed = docHeaderOPDS(lib.name, u"tag:root"_s, sLibUrl, sSesionQuery);

    QDomElement entry;
    QDomElement el;
    entry = doc.createElement(u"entry"_s);
    feed.appendChild(entry);
    AddTextNode(u"updated"_s, QDateTime::currentDateTimeUtc().toString(Qt::ISODate), entry);
    AddTextNode(u"id"_s, u"tag:root:authors"_s, entry);
    AddTextNode(u"title"_s, tr("Books by authors"), entry);
    el = AddTextNode(u"content"_s, tr("Finding books by authors"), entry);
    el.setAttribute(u"type"_s, u"text"_s);
    el = AddTextNode(u"link"_s, u""_s, entry);
    el.setAttribute(u"href"_s, sLibUrl + u"/authorsindex"_s + sSesionQuery);
    el.setAttribute(u"type"_s, u"application/atom+xml;profile=opds-catalog"_s);

    entry = doc.createElement(u"entry"_s);
    feed.appendChild(entry);
    AddTextNode(u"updated"_s, QDateTime::currentDateTimeUtc().toString(Qt::ISODate), entry);
    AddTextNode(u"id"_s, u"tag:root:sequences"_s, entry);
    AddTextNode(u"title"_s, tr("Books by sequences"), entry);
    el = AddTextNode(u"content"_s, tr("Finding books by sequences"), entry);
    el.setAttribute(u"type"_s, u"text"_s);
    el = AddTextNode(u"link"_s, u""_s, entry);
    el.setAttribute(u"href"_s, sLibUrl + u"/sequencesindex"_s + sSesionQuery);
    el.setAttribute(u"type"_s, u"application/atom+xml;profile=opds-catalog"_s);

    entry = doc.createElement(u"entry"_s);
    feed.appendChild(entry);
    AddTextNode(u"updated"_s, QDateTime::currentDateTimeUtc().toString(Qt::ISODate), entry);
    AddTextNode(u"id"_s, u"tag:root:genre"_s, entry);
    AddTextNode(u"title"_s, tr("Books by genre"), entry);
    el = AddTextNode(u"content"_s, tr("Finding books by genre"), entry);
    el.setAttribute(u"type"_s, u"text"_s);
    el = AddTextNode(u"link"_s, u""_s, entry);
    el.setAttribute(u"href"_s, sLibUrl + u"/genres"_s + sSesionQuery);
    el.setAttribute(u"type"_s, u"application/atom+xml;profile=opds-catalog"_s);

    QHttpServerResponse result(doc.toString());
    return result;
}

QByteArray opds_server::image(const QString &sFile)
{
    QString ico;
    if(QFile::exists(sFile)){
        QFileInfo fi(sFile);
        ico = fi.absoluteFilePath();
    }else{
        ico = u":/xsl/opds/"_s + sFile;
    }

    QFile file(ico);
    file.open(QFile::ReadOnly);
    QByteArray ba = file.readAll();
    return ba;
}

QByteArray opds_server::cover(uint idLib, uint idBook)
{
    QByteArray baResult;
    SLib &lib = getLib(idLib);
    SBook &book = mLibs[idLib].mBooks[idBook];
    if(book.sAnnotation.isEmpty() && book.sImg.isEmpty())
        lib.loadAnnotation(idBook);

    QString sCover;
    if(book.sImg.isEmpty()){
        int http_settings = options.nHttpExport - 1;
        if(http_settings == -1)
        {
            int count = options.vExportOptions.count();
            for(int i=0; i<count; i++)
            {
                if(options.vExportOptions[i].bDefault)
                {
                    http_settings = i;
                    break;
                }
            }
        }
        pExportOptions_ = &options.vExportOptions[http_settings];
        fb2mobi fb(pExportOptions_, idLib);
        sCover = fb.convert(idBook);
    }else
        sCover = book.sImg;

    baResult = image(sCover);
    return baResult;
}

QHttpServerResponse opds_server::authorsIndexHTTP(uint idLib, const QString &sIndex, bool bByBooks, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    const SLib &lib = getLib(idLib, u"/http"_s, &sLibUrl);
    QUrlQuery urlquery(url);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSesionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    QDomElement feed = docHeaderHTTP(sSesionQuery, lib.name, sLibUrl);

    auto iAuthor = lib.mAuthors.constBegin();
    QMap<QString, int> mCount;

    QSet <QString> setAuthors;
    int count = 0;
    while(iAuthor != lib.mAuthors.constEnd())
    {
        QString sAuthorName = iAuthor->getName();
        if(sAuthorName.left(sIndex.length()).toLower() == sIndex.toLower())
        {
            count++;
            QString sNewIndex = sAuthorName.left(sIndex.length() + 1).toLower();
            sNewIndex[0] = sNewIndex[0].toUpper();
            if(mCount.contains(sNewIndex))
                mCount[sNewIndex]++;
            else
                mCount[sNewIndex] = 1;
            if(sNewIndex.length() == sAuthorName.length())
                setAuthors.insert(sNewIndex);
        }
        ++iAuthor;
    }
    QList<QString> listKeys = mCount.keys();
    std::sort(listKeys.begin(), listKeys.end(), [](const QString& lhs, const QString& rhs) {return QString::localeAwareCompare(lhs, rhs) < 0;});

    if(count>30 && !bByBooks)
    {
        QDomElement tag_table;
        QDomElement tag_tr;
        int nCurrentColumn = 0;
        tag_table = doc.createElement(u"TABLE"_s);
        feed.appendChild(tag_table);

        foreach(const QString &iIndex, listKeys)
        {
            if(nCurrentColumn == 0)
            {
                tag_tr = doc.createElement(u"tr"_s);
                tag_table.appendChild(tag_tr);
            }
            QDomElement td = doc.createElement(u"td"_s);
            tag_tr.appendChild(td);
            QDomElement div = doc.createElement(u"div"_s);
            div.setAttribute(u"class"_s, u"author"_s);
            td.appendChild(div);
            QDomElement el = AddTextNode(u"a"_s, iIndex, div);
            el.setAttribute(u"class"_s, u"block"_s);
            AddTextNode(u"div"_s, QString::number(mCount[iIndex]) + u" "_s + tr("authors beginning with") + u" '"_s +
                        iIndex + u"'"_s, div);
            if(mCount[iIndex] == 1)
            {
                iAuthor = lib.mAuthors.constBegin();
                while(iAuthor != lib.mAuthors.constEnd())
                {
                    if(setAuthors.contains(iIndex) ?iAuthor.value().getName().toLower() == iIndex.toLower()
                            :iAuthor.value().getName().left(iIndex.size()).toLower() == iIndex.toLower())
                    {
                        el.setAttribute(u"href"_s, sLibUrl + u"/author/"_s + QString::number(iAuthor.key()) + sSesionQuery);
                        break;
                    }
                    ++iAuthor;
                }
            }
            else
            {
                el.setAttribute(u"href"_s, sLibUrl + u"/authorsindex/"_s+ QString::fromLatin1(QUrl::toPercentEncoding(iIndex).constData()) +
                                (setAuthors.contains(iIndex) ?u"/books"_s :u""_s) + sSesionQuery);

            }

            nCurrentColumn++;
            if(nCurrentColumn == MAX_COLUMN_COUNT)
                nCurrentColumn = 0;
        }
        while(nCurrentColumn < MAX_COLUMN_COUNT)
        {
            AddTextNode(u"td"_s, u""_s, tag_tr);
            nCurrentColumn++;
        }
    }
    else
    {
        QList<uint> listAuthorId;
        iAuthor = lib.mAuthors.constBegin();
        while(iAuthor != lib.mAuthors.constEnd())
        {
            if(iAuthor->getName().left(sIndex.length()).toLower() == sIndex.toLower())
            {
                listAuthorId << iAuthor.key();
            }
            ++iAuthor;
        }
        std::sort(listAuthorId.begin(), listAuthorId.end(), [lib](uint lhs,uint rhs) {return lib.mAuthors[lhs].getName() < lib.mAuthors[rhs].getName();});
        foreach(uint iIndex, listAuthorId)
        {
            uint nBooksCount = 0;
            auto i = lib.mAuthorBooksLink.constFind(iIndex);
            while(i != lib.mAuthorBooksLink.constEnd() && i.key() == iIndex){
                if(!lib.mBooks[i.value()].bDeleted)
                    nBooksCount++;
                ++i;
            }

            QDomElement div = doc.createElement(u"DIV"_s);
            div.setAttribute(u"class"_s, u"author"_s);
            feed.appendChild(div);
            QDomElement el = AddTextNode(u"a"_s, lib.mAuthors[iIndex].getName(), div);
            el.setAttribute(u"class"_s, u"block"_s);
            el.setAttribute(u"href"_s, sLibUrl + u"/author/"_s + QString::number(iIndex) + sSesionQuery);
            AddTextNode(u"div"_s, QString::number(nBooksCount) + u" "_s + tr("books"), div);
        }
    }

    return responseHTTP();
}

QHttpServerResponse opds_server::authorsIndexOPDS(uint idLib, const QString &sIndex, bool bByBooks, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    const SLib &lib = getLib(idLib, u"/opds"_s, &sLibUrl);
    QUrlQuery urlquery(url);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSesionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    QDomElement feed = docHeaderOPDS(tr("Books by authors"), u"tag:root:authors"_s, sLibUrl, sSesionQuery);

    auto iAuthor = lib.mAuthors.constBegin();
    QMap<QString, int> mCount;

    QSet <QString> setAuthors;
    int count = 0;
    while(iAuthor != lib.mAuthors.constEnd())
    {
        QString sAuthorName = iAuthor->getName();
        if(sAuthorName.left(sIndex.length()).toLower() == sIndex.toLower())
        {
            count++;
            QString sNewIndex = sAuthorName.left(sIndex.length() + 1).toLower();
            sNewIndex[0] = sNewIndex[0].toUpper();
            if(mCount.contains(sNewIndex))
                mCount[sNewIndex]++;
            else
                mCount[sNewIndex] = 1;
            if(sNewIndex.length() == sAuthorName.length())
                setAuthors.insert(sNewIndex);
        }
        ++iAuthor;
    }
    QList<QString> listKeys = mCount.keys();
    std::sort(listKeys.begin(), listKeys.end(), [](const QString& lhs, const QString& rhs) {return QString::localeAwareCompare(lhs, rhs) < 0;});

    if(count>30 && !bByBooks)
    {
        QDomElement tag_table;
        QDomElement tag_tr;
//        int nCurrentColumn = 0;

        foreach(const QString &iIndex, listKeys)
        {
            QDomElement entry = doc.createElement(u"entry"_s);
            feed.appendChild(entry);
            AddTextNode(u"updated"_s, QDateTime::currentDateTimeUtc().toString(Qt::ISODate), entry);
            AddTextNode(u"id"_s, u"tag:authors:"_s + iIndex, entry);
            AddTextNode(u"title"_s, iIndex, entry);
            QDomElement el = AddTextNode(u"content"_s, QString::number(mCount[iIndex]) + u" "_s + tr("authors beginning with") +
                                                           u" '"_s + iIndex + u"'"_s, entry);
            el.setAttribute(u"type"_s, u"text"_s);
            el = AddTextNode(u"link"_s, u""_s, entry);
            if(mCount[iIndex] == 1)
            {
                iAuthor = lib.mAuthors.constBegin();
                while(iAuthor != lib.mAuthors.constEnd())
                {
                    if(setAuthors.contains(iIndex) ?iAuthor.value().getName().toLower() == iIndex.toLower()
                                                    :iAuthor.value().getName().left(iIndex.size()).toLower() == iIndex.toLower())
                    {
                        el.setAttribute(u"href"_s, sLibUrl + u"/author/"_s + QString::number(iAuthor.key()) + sSesionQuery);
                        break;
                    }
                    ++iAuthor;
                }
            }
            else
            {
                el.setAttribute(u"href"_s, sLibUrl + u"/authorsindex/"_s + QString::fromLatin1(QUrl::toPercentEncoding(iIndex).constData()) +
                                               (setAuthors.contains(iIndex) && mCount[iIndex]<30 ?u"/books"_s :u""_s) + sSesionQuery);

            }
            el.setAttribute(u"type"_s, u"application/atom+xml;profile=opds-catalog"_s);
        }
    }
    else
    {
        QList<uint> listAuthorId;
        iAuthor = lib.mAuthors.constBegin();
        while(iAuthor != lib.mAuthors.constEnd())
        {
            if(iAuthor->getName().left(sIndex.length()).toLower() == sIndex.toLower())
            {
                listAuthorId << iAuthor.key();
            }
            ++iAuthor;
        }
        std::sort(listAuthorId.begin(),listAuthorId.end(),[lib](uint lhs,uint rhs) {return lib.mAuthors[lhs].getName() < lib.mAuthors[rhs].getName();});
        foreach(uint iIndex, listAuthorId)
        {
            uint nBooksCount = 0;
            auto i = lib.mAuthorBooksLink.constFind(iIndex);
            while(i != lib.mAuthorBooksLink.constEnd() && i.key() == iIndex){
                if(!lib.mBooks[i.value()].bDeleted)
                    nBooksCount++;
                ++i;
            }

            QDomElement entry = doc.createElement(u"entry"_s);
            feed.appendChild(entry);
            AddTextNode(u"updated"_s, QDateTime::currentDateTimeUtc().toString(Qt::ISODate), entry);
            AddTextNode(u"id"_s, u"tag:author:"_s + QString::number(iIndex), entry);
            AddTextNode(u"title"_s, lib.mAuthors[iIndex].getName(), entry);
            QDomElement el = AddTextNode(u"content"_s, QString::number(nBooksCount) + u" "_s + tr("books"), entry);
            el.setAttribute(u"type"_s, u"text"_s);
            el = AddTextNode(u"link"_s, u""_s, entry);
            el.setAttribute(u"href"_s, sLibUrl + u"/author/"_s + QString::number(iIndex));
            el.setAttribute(u"type"_s, u"application/atom+xml;profile=opds-catalog"_s);
        }

    }

    QHttpServerResponse result(doc.toString());
    return result;
}

QHttpServerResponse opds_server::authorHTTP(uint idLib, uint idAuthor, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    const SLib &lib = getLib(idLib, u"/http"_s, &sLibUrl);
    QUrlQuery urlquery(url);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSesionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    QString sAuthor = lib.mAuthors[idAuthor].getName();
    QString sIdAuthor = QString::number(idAuthor);
    QDomElement feed = docHeaderHTTP(sSesionQuery, lib.name, sLibUrl);
    QDomElement div_auth = doc.createElement(u"DIV"_s);;
    div_auth.setAttribute(u"class"_s, u"author"_s);
    feed.appendChild(div_auth);
    QDomElement div_caption = AddTextNode(u"div"_s, tr("Books by") + u" "_s + sAuthor, div_auth);
    div_caption.setAttribute(u"class"_s, u"caption"_s);

    QDomElement div = doc.createElement(u"DIV"_s);
    div_auth.appendChild(div);
    QDomElement el = AddTextNode(u"a"_s, tr("Books by sequences"), div);
    el.setAttribute(u"class"_s, u"block"_s);
    el.setAttribute(u"href"_s, sLibUrl + u"/authorsequences/"_s + sIdAuthor + sSesionQuery);

    div = doc.createElement(u"DIV"_s);
    div_auth.appendChild(div);
    el = AddTextNode(u"a"_s, tr("Books without sequence"), div);
    el.setAttribute(u"class"_s, u"block"_s);
    el.setAttribute(u"href"_s, sLibUrl + u"/authorsequenceless/"_s + sIdAuthor + sSesionQuery);


    div = doc.createElement(u"DIV"_s);
    div_auth.appendChild(div);
    el = AddTextNode(u"a"_s, tr("All books"), div);
    el.setAttribute(u"class"_s, u"block"_s);
    el.setAttribute(u"href"_s, sLibUrl + u"/authorbooks/"_s + sIdAuthor + sSesionQuery);

    return responseHTTP();
}

QHttpServerResponse opds_server::authorOPDS(uint idLib, uint idAuthor, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    const SLib &lib = getLib(idLib, u"/opds"_s, &sLibUrl);
    QUrlQuery urlquery(url);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSesionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    QString sAuthor = lib.mAuthors[idAuthor].getName();
    QString sIdAuthor = QString::number(idAuthor);
    QString sCurrentDateTime = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    QDomElement feed = docHeaderOPDS(tr("Books by") + u" "_s + sAuthor, u"tag:author:"_s + sIdAuthor, sLibUrl, sSesionQuery);

    QDomElement entry = doc.createElement(u"entry"_s);
    feed.appendChild(entry);
    AddTextNode(u"updated"_s, sCurrentDateTime, entry);
    AddTextNode(u"id"_s, u"tag:author:"_s + sIdAuthor + u":sequences"_s, entry);
    AddTextNode(u"title"_s, tr("Books by sequences"), entry);
    QDomElement el = AddTextNode(u"link"_s, u""_s, entry);
    el.setAttribute(u"href"_s, sLibUrl + u"/authorsequences/"_s + sIdAuthor + sSesionQuery);
    el.setAttribute(u"type"_s, u"application/atom+xml;profile=opds-catalog"_s);

    entry = doc.createElement(u"entry"_s);
    feed.appendChild(entry);
    AddTextNode(u"updated"_s, sCurrentDateTime, entry);
    AddTextNode(u"id"_s, u"tag:author:"_s + sIdAuthor + u":sequenceless"_s, entry);
    AddTextNode(u"title"_s, tr("Books without sequence"), entry);
    el = AddTextNode(u"link"_s, u""_s, entry);
    el.setAttribute(u"href"_s, sLibUrl + u"/authorsequenceless/"_s + sIdAuthor + sSesionQuery);
    el.setAttribute(u"type"_s, u"application/atom+xml;profile=opds-catalog"_s);

    entry = doc.createElement(u"entry"_s);
    feed.appendChild(entry);
    AddTextNode(u"updated"_s, sCurrentDateTime,entry);
    AddTextNode(u"id"_s, u"tag:author:"_s + sIdAuthor + u":sequences"_s, entry);
    AddTextNode(u"title"_s, tr("All books"), entry);
    el = AddTextNode(u"link"_s, u""_s, entry);
    el.setAttribute(u"href"_s, sLibUrl + u"/authorbooks/"_s + sIdAuthor + sSesionQuery);
    el.setAttribute(u"type"_s, u"application/atom+xml;profile=opds-catalog"_s);

    QHttpServerResponse result(doc.toString());
    return result;
}

QHttpServerResponse opds_server::authorBooksHTTP(uint idLib, uint idAuthor, const QHttpServerRequest &request)
{
    QString sLibUrl;
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    SLib &lib = getLib(idLib, u"/http"_s, &sLibUrl);

    QList<uint> listBooks = book_list(lib, idAuthor, 0, 0, u""_s, false);
    return FillPageHTTP(listBooks, lib, tr("Books by ABC") + u" ("_s + lib.mAuthors[idAuthor].getName() + u")"_s, sLibUrl, url, false);
}

QHttpServerResponse opds_server::authorBooksOPDS(uint idLib, uint idAuthor, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib &lib = getLib(idLib, u"/opds"_s, &sLibUrl);

    QList<uint> listBooks = book_list(lib, idAuthor, 0, 0, u""_s, false);
    QString sPage = FillPageOPDS(listBooks, lib, tr("Books by ABC") + u" ("_s + lib.mAuthors[idAuthor].getName() + u")"_s, u"id:autorbooks:"_s + QString::number(idAuthor), sLibUrl, url);

    QHttpServerResponse result(sPage);
    return result;
}

QHttpServerResponse opds_server::authorSequencesHTTP(uint idLib, uint idAuthor, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib &lib = getLib(idLib, u"/http"_s, &sLibUrl);
    QUrlQuery urlquery(url);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSesionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    QDomElement feed = docHeaderHTTP(sSesionQuery, lib.name, sLibUrl);
    QDomElement div = AddTextNode(u"DIV"_s, tr("Book sequences") + u" "_s + lib.mAuthors[idAuthor].getName(), feed);
    div.setAttribute(u"class"_s, u"caption"_s);

    auto i = lib.mAuthorBooksLink.constFind(idAuthor);
    QMap<uint, uint> mapCountBooks;
    while(i != lib.mAuthorBooksLink.constEnd() && i.key() == idAuthor){
        SBook& book = lib.mBooks[i.value()];
        if(!book.bDeleted && book.idSerial > 0){
            if(mapCountBooks.contains(book.idSerial))
                mapCountBooks[book.idSerial]++;
            else
                mapCountBooks[book.idSerial] = 1;
        }
        ++i;
    }
    QList<uint> listSerials = mapCountBooks.keys();
    std::sort(listSerials.begin(), listSerials.end(), [lib](uint lhs, uint rhs) {return QString::localeAwareCompare(lib.mSerials[lhs].sName, lib.mSerials[rhs].sName) < 0;});

    QString sIdAuthor = QString::number(idAuthor);
    foreach(uint idSerial, listSerials)
    {

        QDomElement entry = doc.createElement(u"div"_s);
        entry.setAttribute(u"class"_s, u"author"_s);
        feed.appendChild(entry);
        QDomElement el = AddTextNode(u"a"_s, lib.mSerials[idSerial].sName, entry);
        el.setAttribute(u"class"_s, u"block"_s);
        el.setAttribute(u"href"_s, sLibUrl + u"/authorsequence/"_s + sIdAuthor + u"/"_s + QString::number(idSerial) + sSesionQuery);
        AddTextNode(u"div"_s, QString::number(mapCountBooks[idSerial]) + u" "_s + tr("books in sequence"), entry);
    }
    return responseHTTP();
}

QHttpServerResponse opds_server::authorSequencesOPDS(uint idLib, uint idAuthor, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    const SLib &lib = getLib(idLib, u"/opds"_s, &sLibUrl);

    QUrlQuery urlquery(url);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSesionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    QString sIdAuthor = QString::number(idAuthor);
    QString sCurrentDateTime = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    QDomElement feed = docHeaderOPDS(tr("Book sequences") + u" "_s + lib.mAuthors[idAuthor].getName(), u"tag:author:"_s + sIdAuthor, sLibUrl, sSesionQuery);

    QMultiHash<uint,uint>::const_iterator i = lib.mAuthorBooksLink.constFind(idAuthor);
    QMap<uint, uint> mapCountBooks;
    while(i != lib.mAuthorBooksLink.constEnd() && i.key() == idAuthor){
        const SBook& book = lib.mBooks[i.value()];
        if(!book.bDeleted && book.idSerial > 0){
            if(mapCountBooks.contains(book.idSerial))
                mapCountBooks[book.idSerial]++;
            else
                mapCountBooks[book.idSerial] = 1;
        }
        ++i;
    }
    QList<uint> listSerials = mapCountBooks.keys();
    std::sort(listSerials.begin(), listSerials.end(), [lib](uint lhs,uint rhs) {return QString::localeAwareCompare(lib.mSerials[lhs].sName, lib.mSerials[rhs].sName) < 0;});

    foreach(uint idSerial, listSerials)
    {
        QString sIdSerial = QString::number(idSerial);
        QDomElement entry = doc.createElement(u"entry"_s);
        feed.appendChild(entry);
        AddTextNode(u"updated"_s, sCurrentDateTime, entry);
        AddTextNode(u"id"_s, u"tag:author:"_s + sIdAuthor + u":sequence:"_s + sIdSerial, entry);
        AddTextNode(u"title"_s, lib.mSerials[idSerial].sName, entry);
        QDomElement el = AddTextNode(u"content"_s, QString::number(mapCountBooks[idSerial]) + u" "_s + tr("books in sequence"), entry);
        el.setAttribute(u"type"_s, u"text"_s);
        el = AddTextNode(u"link"_s, u""_s, entry);
        el.setAttribute(u"href"_s, sLibUrl + u"/authorsequence/"_s + sIdAuthor + u"/"_s + sIdSerial + sSesionQuery);
        el.setAttribute(u"type"_s, u"application/atom+xml;profile=opds-catalog"_s);
    }
    QHttpServerResponse result(doc.toString());
    return result;
}

QHttpServerResponse opds_server::authorSequencesHTTP(uint idLib, uint idAuthor, uint idSequence, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib &lib = getLib(idLib, u"/http"_s, &sLibUrl);

    QList<uint> listBooks = book_list(lib, idAuthor, idSequence, 0, u""_s);
    return FillPageHTTP(listBooks, lib, tr("Books of sequence") + u" ("_s + lib.mSerials[idSequence].sName + u")"_s, sLibUrl, url, false);
}

QHttpServerResponse opds_server::authorSequencesOPDS(uint idLib, uint idAuthor, uint idSequence, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib &lib = getLib(idLib, u"/opds"_s, &sLibUrl);

    QList<uint> listBooks = book_list(lib, idAuthor, idSequence, 0, u""_s);
    QString sPage = FillPageOPDS(listBooks, lib, tr("Books of sequence") + u" ("_s + lib.mSerials[idSequence].sName + u")"_s,
                                 u"tag:author:"_s + QString::number(idAuthor) + u":sequence:"_s + QString::number(idSequence), sLibUrl, url);
    QHttpServerResponse result(sPage);
    return result;
}

QHttpServerResponse opds_server::authorSequencelessHTTP(uint idLib, uint idAuthor, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib &lib = getLib(idLib, u"/http"_s, &sLibUrl);

    QList<uint> listBooks = book_list(lib, idAuthor, 0, 0, u""_s, true);
    return FillPageHTTP(listBooks, lib, tr("Books without sequence") + u" ("_s + lib.mAuthors[idAuthor].getName() + u")"_s, sLibUrl, url, false);
}

QHttpServerResponse opds_server::authorSequencelessOPDS(uint idLib, uint idAuthor, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib &lib = getLib(idLib, u"/opds"_s, &sLibUrl);

    QList<uint> listBooks = book_list(lib, idAuthor, 0, 0, u""_s, true);
    QString sPage = FillPageOPDS(listBooks, lib, tr("Books without sequence") + u" ("_s + lib.mAuthors[idAuthor].getName() + u")"_s,
                                 u"tag:author:"_s + QString::number(idAuthor) + u":sequenceless"_s, sLibUrl, url);
    QHttpServerResponse result(sPage);
    return result;
}

QHttpServerResponse opds_server::sequencesIndexHTTP(uint idLib, const QString &sIndex, bool bByBooks, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    const SLib &lib = getLib(idLib, u"/http"_s, &sLibUrl);
    QUrlQuery urlquery(url);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSesionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    QDomElement feed;
    feed = docHeaderHTTP(sSesionQuery, lib.name, sLibUrl);
    auto iSerial = lib.mSerials.constBegin();
    QMap <QString, int> mCount;
    QSet <QString> stSerials;
    int count = 0;
    while(iSerial != lib.mSerials.constEnd()){
        if(iSerial->sName.left(sIndex.length()).toLower() == sIndex.toLower()){
            count++;
            QString sNewIndex = iSerial->sName.left(sIndex.length()+1).toLower();
            sNewIndex[0] = sNewIndex[0].toUpper();
            if(mCount.contains(sNewIndex))
                mCount[sNewIndex]++;
            else
                mCount[sNewIndex] = 1;
            if(sNewIndex.length() == iSerial->sName.length())
                stSerials.insert(sNewIndex);
        }
        ++iSerial;
    }
    QList<QString> listKeys = mCount.keys();
    std::sort(listKeys.begin(), listKeys.end(), [](const QString& lhs, const QString& rhs) {return QString::localeAwareCompare(lhs, rhs) < 0;});

    if(count > 30 && !bByBooks)
    {
        QDomElement tag_table;
        QDomElement tag_tr;
        int current_column = 0;
        tag_table = doc.createElement(u"TABLE"_s);
        feed.appendChild(tag_table);
        foreach(const QString &iIndex, listKeys)
        {
            if(iIndex.trimmed().isEmpty() || iIndex[0] == '\0')
                continue;

            if(current_column == 0)
            {
                tag_tr = doc.createElement(u"tr"_s);
                tag_table.appendChild(tag_tr);
            }
            QDomElement td = doc.createElement(u"td"_s);
            tag_tr.appendChild(td);

            QDomElement div = doc.createElement(u"DIV"_s);
            div.setAttribute(u"class"_s, u"author"_s);
            td.appendChild(div);
            QDomElement el = AddTextNode(u"a"_s, iIndex, div);
            el.setAttribute(u"class"_s, u"block"_s);
            AddTextNode(u"div"_s, QString::number(mCount[iIndex]) + u" "_s + tr("series beginning with") + u" '"_s +
                        iIndex + u"'"_s, div);
            if(mCount[iIndex] == 1)
            {
                auto iSerial = lib.mSerials.constBegin();
                while(iSerial != lib.mSerials.constEnd()){
                    if(stSerials.contains(iIndex) ?iSerial.value().sName.toLower() == iIndex.toLower()
                            :iSerial.value().sName.left(iIndex.size()).toLower() == iIndex.toLower())
                    {
                        el.setAttribute(u"href"_s, sLibUrl + u"/sequencebooks/"_s + QString::number(iSerial.key()) + sSesionQuery);
                        break;
                    }
                    ++iSerial;
                }

            }
            else
            {
                el.setAttribute(u"href"_s, sLibUrl + u"/sequencesindex/"_s + QString::fromLatin1(QUrl::toPercentEncoding(iIndex).constData()) +
                                (stSerials.contains(iIndex) && mCount[iIndex]<30 ?u"/books"_s :u""_s) + sSesionQuery );

            }
            current_column++;
            if(current_column == MAX_COLUMN_COUNT)
                current_column = 0;
        }
        while(current_column < MAX_COLUMN_COUNT)
        {
            AddTextNode(u"td"_s, u""_s, tag_tr);
            current_column++;
        }
    }
    else
    {
        QList<uint> listSerialId;
        iSerial = lib.mSerials.constBegin();
        while(iSerial != lib.mSerials.constEnd())
        {
            if(iSerial->sName.left((sIndex.length())).toLower() == sIndex.toLower())
            {
                listSerialId << iSerial.key();
            }
            ++iSerial;
        }
        std::sort(listSerialId.begin(), listSerialId.end(),[lib](uint lhs, uint rhs) {return lib.mSerials[lhs].sName < lib.mSerials[rhs].sName;});

        foreach(const auto &iIndex, listSerialId)
        {
            uint nBooksCount = 0;
            auto iBook = lib.mBooks.constBegin();
            while(iBook != lib.mBooks.constEnd())
            {
                if(!iBook->bDeleted && iBook->idSerial == iIndex)
                    nBooksCount++;
                ++iBook;
            }
                QDomElement div = doc.createElement(u"DIV"_s);
                div.setAttribute(u"class"_s, u"author"_s);
                feed.appendChild(div);
                QDomElement el = AddTextNode(u"a"_s, lib.mSerials[iIndex].sName, div);
                el.setAttribute(u"class"_s, u"block"_s);
                el.setAttribute(u"href"_s ,sLibUrl + u"/sequencebooks/"_s + QString::number(iIndex) + sSesionQuery );
                AddTextNode(u"div"_s, QString::number(nBooksCount) + u" "_s + tr("books"), div);
        }
    }

    return responseHTTP();
}

QHttpServerResponse opds_server::sequencesIndexOPDS(uint idLib, const QString &sIndex, bool bByBooks, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    const SLib &lib = getLib(idLib, u"/opds"_s, &sLibUrl);
    QUrlQuery urlquery(url);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSesionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    QDomElement feed = docHeaderOPDS(tr("Books by sequences"), u"tag:root:sequences"_s, sLibUrl, sSesionQuery);

    auto iSerial = lib.mSerials.constBegin();
    QMap <QString, int> mCount;
    QSet <QString> stSerials;
    int count = 0;
    while(iSerial != lib.mSerials.constEnd()){
        if(iSerial->sName.left(sIndex.length()).toLower() == sIndex.toLower()){
            count++;
            QString sNewIndex = iSerial->sName.left(sIndex.length()+1).toLower();
            sNewIndex[0] = sNewIndex[0].toUpper();
            if(mCount.contains(sNewIndex))
                mCount[sNewIndex]++;
            else
                mCount[sNewIndex] = 1;
            if(sNewIndex.length() == iSerial->sName.length())
                stSerials.insert(sNewIndex);
        }
        ++iSerial;
    }
    QList<QString> listKeys = mCount.keys();
    std::sort(listKeys.begin(), listKeys.end(), [](const QString& lhs, const QString& rhs) {return QString::localeAwareCompare(lhs, rhs) < 0;});

    if(count > 30 && !bByBooks)
    {
        QDomElement tag_table;
        QDomElement tag_tr;
        foreach(const QString &iIndex, listKeys)
        {
            if(iIndex.trimmed().isEmpty() || iIndex[0] == '\0')
                continue;

            QDomElement entry = doc.createElement(u"entry"_s);
            feed.appendChild(entry);
            AddTextNode(u"updated"_s, QDateTime::currentDateTimeUtc().toString(Qt::ISODate), entry);
            AddTextNode(u"id"_s, u"tag:sequences:"_s + iIndex, entry);
            AddTextNode(u"title"_s, iIndex, entry);
            QDomElement el = AddTextNode(u"content"_s, QString::number(mCount[iIndex]) + u" "_s + tr("series beginning with") +
                                         u" '"_s + iIndex + u"'"_s, entry);
            el.setAttribute(u"type"_s, u"text"_s);
            el = AddTextNode(u"link"_s, u""_s, entry);
            if(mCount[iIndex] == 1)
            {
                auto iSerial = lib.mSerials.constBegin();
                while(iSerial != lib.mSerials.constEnd()){
                    if(stSerials.contains(iIndex) ?iSerial.value().sName.toLower() == iIndex.toLower()
                            :iSerial.value().sName.left(iIndex.size()).toLower() == iIndex.toLower())
                    {
                        el.setAttribute(u"href"_s, sLibUrl + u"/sequencebooks/"_s + QString::number(iSerial.key()) +sSesionQuery);
                        break;
                    }
                    ++iSerial;
                }

            }
            else
            {
                el.setAttribute(u"href"_s, sLibUrl + u"/sequencesindex/"_s +  QString::fromLatin1(QUrl::toPercentEncoding(iIndex).constData())
                                + (stSerials.contains(iIndex) ?u"/books"_s :u""_s) + sSesionQuery);
            }
            el.setAttribute(u"type"_s, u"application/atom+xml;profile=opds-catalog"_s);
        }
    }
    else
    {
        QList<uint> listSerialId;
        iSerial = lib.mSerials.constBegin();
        while(iSerial != lib.mSerials.constEnd())
        {
            if(iSerial->sName.left((sIndex.length())).toLower() == sIndex.toLower())
            {
                listSerialId << iSerial.key();
            }
            ++iSerial;
        }
        std::sort(listSerialId.begin(), listSerialId.end(),[lib](uint lhs, uint rhs) {return lib.mSerials[lhs].sName < lib.mSerials[rhs].sName;});

        foreach(const auto &iIndex, listSerialId)
        {
            uint nBooksCount = 0;
            auto iBook = lib.mBooks.constBegin();
            while(iBook != lib.mBooks.constEnd())
            {
                if(!iBook->bDeleted && iBook->idSerial == iIndex)
                    nBooksCount++;
                ++iBook;
            }
            QDomElement entry = doc.createElement(u"entry"_s);
            feed.appendChild(entry);
            AddTextNode(u"updated"_s, QDateTime::currentDateTimeUtc().toString(Qt::ISODate), entry);
            AddTextNode(u"id"_s, u"tag:sequences:"_s + QString::number(iIndex), entry);
            AddTextNode(u"title"_s, lib.mSerials[iIndex].sName, entry);
            QDomElement el = AddTextNode(u"content"_s, QString::number(nBooksCount) + u" "_s + tr("books"), entry);
            el.setAttribute(u"type"_s, u"text"_s);
            el = AddTextNode(u"link"_s, u""_s, entry);
            el.setAttribute(u"href"_s, sLibUrl + u"/sequencebooks/"_s + QString::number(iIndex) + sSesionQuery);
            el.setAttribute(u"type"_s, u"application/atom+xml;profile=opds-catalog"_s);
        }
    }
    QHttpServerResponse result(doc.toString());
    return result;
}

QHttpServerResponse opds_server::sequenceBooksHTTP(uint idLib, uint idSequence, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib &lib = getLib(idLib, u"/http"_s, &sLibUrl);

    QList<uint> listBooks = book_list(lib, 0, idSequence, 0, u""_s);
    return FillPageHTTP(listBooks, lib, tr("Books of sequence") + u" ("_s + lib.mSerials[idSequence].sName + u")"_s, sLibUrl, url, false);

}

QHttpServerResponse opds_server::sequenceBooksOPDS(uint idLib, uint idSequence, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib &lib = getLib(idLib, u"/opds"_s, &sLibUrl);

    QList<uint> listBooks = book_list(lib, 0, idSequence, 0, u""_s);
    QString sPage = FillPageOPDS(listBooks, lib, tr("Books of sequence") + u" ("_s + lib.mSerials[idSequence].sName + u")"_s, u"tag:sequences:"_s + QString::number(idSequence), sLibUrl, url);
    QHttpServerResponse result(sPage);
    return result;
}

QHttpServerResponse opds_server::bookHTTP(uint idLib, uint idBook, const QString &sFormat)
{
    return convert(idLib, idBook, sFormat, u""_s, false);
}

QHttpServerResponse opds_server::bookOPDS(uint idLib, uint idBook, const QString &sFormat)
{
    return convert(idLib, idBook, sFormat, u""_s, true);
}

QHttpServerResponse opds_server::genresHTTP(uint idLib, ushort idParentGenre, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib &lib = getLib(idLib, u"/http"_s, &sLibUrl);
    QUrlQuery urlquery(url);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSesionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    Q_ASSERT(!lib.mSerials.contains(0));
    QList<ushort> listIdGenres;
    QMap<ushort, uint> mCounts;
    if(idParentGenre != 0)
    {
        if(mGenre[idParentGenre].idParrentGenre > 0){
            QList<uint> listBooks = book_list(lib, 0, 0, idParentGenre, u""_s);
            return FillPageHTTP(listBooks, lib, tr("Books by ABC"), sLibUrl, url, true);
        }
        auto iGenre = mGenre.constBegin();
        while(iGenre != mGenre.constEnd()){
            if(iGenre->idParrentGenre == idParentGenre)
                listIdGenres << iGenre.key();
            ++iGenre;
        }
        auto iBook = lib.mBooks.constBegin();
        while(iBook != lib.mBooks.constEnd()){
            if(!iBook->bDeleted){
                foreach (auto iGenre, iBook->listIdGenres){
                    if(mGenre[iGenre].idParrentGenre == idParentGenre){
                        if(mCounts.contains(iGenre))
                            mCounts[iGenre]++;
                        else
                            mCounts[iGenre] = 1;
                    }
                }
            }
            ++iBook;
        }
    }
    else
    {
        auto iGenre = mGenre.constBegin();
        while(iGenre != mGenre.constEnd()){
            if(iGenre->idParrentGenre == 0)
                listIdGenres << iGenre.key();
            ++iGenre;
        }
    }
    QDomElement feed;
    feed = docHeaderHTTP(sSesionQuery, lib.name, sLibUrl);

    foreach(ushort idGenre, listIdGenres)
    {
        QDomElement div = doc.createElement(u"DIV"_s);
        feed.appendChild(div);
        div.setAttribute(u"class"_s, u"author"_s);
        QDomElement el = AddTextNode(u"A"_s, mGenre[idGenre].sName,div);
        el.setAttribute(u"class"_s, u"block"_s);
        el.setAttribute(u"href"_s, sLibUrl + u"/genres/"_s + QString::number(idGenre) + sSesionQuery );
        if(idParentGenre != 0)
        {
            QDomElement el = AddTextNode(u"div"_s, QString::number(mCounts[idGenre]) + u" "_s + tr("books"), div);
        }
    }

    return responseHTTP();
}

QHttpServerResponse opds_server::genresOPDS(uint idLib, ushort idParentGenre, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib &lib = getLib(idLib, u"/opds"_s, &sLibUrl);

    QUrlQuery urlquery(url);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSesionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;


    QList<ushort> listIdGenres;
    QMap<ushort, uint> mCounts;
    if(idParentGenre != 0)
    {
        if(mGenre[idParentGenre].idParrentGenre > 0){
            QList<uint> listBooks = book_list(lib, 0, 0, idParentGenre, u""_s);
            QString sPage =  FillPageOPDS(listBooks, lib, tr("Books by ABC"), u""_s, sLibUrl, url);
            QHttpServerResponse result(sPage);
            return result;
        }
        auto iGenre = mGenre.constBegin();
        while(iGenre != mGenre.constEnd()){
            if(iGenre->idParrentGenre == idParentGenre)
                listIdGenres << iGenre.key();
            ++iGenre;
        }
        auto iBook = lib.mBooks.constBegin();
        while(iBook != lib.mBooks.constEnd()){
            if(!iBook->bDeleted){
                foreach (auto iGenre, iBook->listIdGenres){
                    if(mGenre[iGenre].idParrentGenre == idParentGenre){
                        if(mCounts.contains(iGenre))
                            mCounts[iGenre]++;
                        else
                            mCounts[iGenre] = 1;
                    }
                }
            }
            ++iBook;
        }
    }
    else
    {
        auto iGenre = mGenre.constBegin();
        while(iGenre != mGenre.constEnd()){
            if(iGenre->idParrentGenre == 0)
                listIdGenres << iGenre.key();
            ++iGenre;
        }
    }

    QString sCurrentDateTime = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    QDomElement feed;
    feed = docHeaderOPDS(tr("Books by genre"), u"tag:root:genre"_s, sLibUrl, sSesionQuery);

    foreach(uint idGenre, listIdGenres)
    {
        QDomElement entry = doc.createElement(u"entry"_s);
        feed.appendChild(entry);
        AddTextNode(u"updated"_s, sCurrentDateTime, entry);
        AddTextNode(u"id"_s, u"tag:root:genre:"_s + mGenre[idGenre].sName, entry);
        AddTextNode(u"title"_s, mGenre[idGenre].sName, entry);
        if(idParentGenre != 0)
        {
            QDomElement el = AddTextNode(u"content"_s, QString::number(mCounts[idGenre]) + u" "_s + tr("books"), entry);
            el.setAttribute(u"type"_s, u"text"_s);
        }
        else
        {
            QDomElement el = AddTextNode(u"content"_s, tr("Books of genre") + u" "_s + mGenre[idGenre].sName, entry);
            el.setAttribute(u"type"_s, u"text"_s);
        }
        QDomElement el = AddTextNode(u"link"_s, u""_s, entry);
        el.setAttribute(u"href"_s, sLibUrl + u"/genres/"_s + QString::number(idGenre) + sSesionQuery );
        el.setAttribute(u"type"_s, u"application/atom+xml;profile=opds-catalog"_s);

    }

    QHttpServerResponse result(doc.toString());
    return result;
}

QHttpServerResponse opds_server::searchHTTP(uint idLib, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib &lib = getLib(idLib, u"/http"_s, &sLibUrl);

    QUrlQuery urlquery(url);
    QString sSearchString = urlquery.queryItemValue(u"search_string"_s);

    QList<uint> listBooks = book_list(lib, 0, 0, 0, sSearchString.replace('+', ' ').replace(u"%20"_s, u" "_s));
    return FillPageHTTP(listBooks, lib, tr("Books search"), sLibUrl, url, true);
}

QHttpServerResponse opds_server::searchOPDS(uint idLib, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QUrlQuery urlquery(url);
    QString sLibUrl;
    SLib &lib = getLib(idLib, u"/opds"_s, &sLibUrl);

    QString sSearchString = urlquery.queryItemValue(u"search_string"_s);
    QList<uint> listBooks = book_list(lib, 0, 0, 0, sSearchString.replace('+', ' ').replace(u"%20"_s, u" "_s));
    return FillPageOPDS(listBooks, lib, tr("Books search"), u""_s, sLibUrl, url);
}

QHttpServerResponse opds_server::convert(uint idLib, uint idBook, const QString &sFormat, const QString &sFileName, bool opds)
{
    QBuffer outbuff;
    QByteArray baContentType;
    QString sContentDisposition;
    QFileInfo fiBook;
    SLib &lib = getLib(idLib);
    if(idBook == 0)
    {
        fiBook.setFile(sFileName);
        QFile file(sFileName);
        file.open(QFile::ReadOnly);
        outbuff.setData(file.readAll());
    }
    else
    {
        fiBook = lib.getBookFile(idBook, &outbuff);
    }

    ExportOptions *pExportOptions = nullptr;
    int count = options.vExportOptions.count();
    for(int i=0; i<count; i++)
    {
        if((options.vExportOptions[i].sOutputFormat.toLower() == sFormat) || (sFormat == u"fb2"_s && options.vExportOptions[i].sOutputFormat == u"-"_s && lib.mBooks[idBook].sFormat == u"fb2"_s))
        {
            pExportOptions = &options.vExportOptions[i];
            break;
        }
    }

    if(outbuff.size() != 0)
    {
        QString sBookFileName;
        if(pExportOptions != nullptr)
            sBookFileName = pExportOptions->sExportFileName;
        if(sBookFileName.isEmpty())
            sBookFileName = QString(ExportOptions::sDefaultEexpFileName);
        sBookFileName = lib.fillParams(sBookFileName, idBook) + u"."_s + (sFormat == u"download"_s ? fiBook.suffix().toLower() :sFormat);
        if(pExportOptions != nullptr && pExportOptions->bTransliteration)
            sBookFileName = Transliteration(sBookFileName);
        sBookFileName.replace(' ', '_');
        sBookFileName.replace('\"', '_');
        sBookFileName.replace('\'', '_');
        sBookFileName.replace(',', '_');
        sBookFileName.replace(u"__"_s, u"_"_s);
        //book_file_name.replace("\\","_");
        //book_file_name.replace("/","_");
        QFileInfo book_file(sBookFileName);
        sBookFileName = book_file.fileName();
        if(pExportOptions != nullptr && pExportOptions->bOriginalFileName)
            sBookFileName = fiBook.completeBaseName() + u"."_s + sFormat;;
        if(sFormat == u"epub"_s || sFormat == u"mobi"_s || sFormat == u"azw3"_s)
        {
            QFile file;
            QString sTmpDir;
            //if(QStandardPaths::standardLocations(QStandardPaths::TempLocation).count() > 0)
            sTmpDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
            QDir().mkpath(sTmpDir + u"/freeLib"_s);
            file.setFileName(sTmpDir + u"/freeLib/book0."_s + fiBook.suffix());
            file.open(QFile::WriteOnly);
            file.write(outbuff.data());
            file.close();
            QFileInfo fi(file);

            fb2mobi conv(pExportOptions, idLib);
            QString sOutFile = conv.convert(QStringList() << fi.absoluteFilePath(), idBook);
            file.setFileName(sOutFile);
            file.open(QFile::ReadOnly);
            outbuff.close();
            outbuff.setData(file.readAll());
            if(opds)
            {
                if(sFormat == u"epub"_s)
                    baContentType = "application/epub+zip"_ba;
                else
                    baContentType = "application/x-mobipocket-ebook"_ba;
            }
            else
            {
                if(sFormat == u"epub"_s){
                    baContentType = "application/epub+zip"_ba;
                    sContentDisposition = u"attachment; filename=\""_s + sBookFileName + u"\""_s;
                }
                else if(sFormat == u"mobi"_s){
                    baContentType = "application/x-mobipocket-ebook"_ba;
                    sContentDisposition = u"attachment; filename=\""_s + sBookFileName + u"\""_s;
                }
                else{
                    baContentType = "text/plain; charset=UTF-8"_ba;
                    sContentDisposition = u"attachment; filename=\""_s + sBookFileName + u"\""_s;
                }
            }
        }
        else
        {
            baContentType = "text/plain; charset=UTF-8"_ba;
            sContentDisposition = u"attachment; filename=\""_s + sBookFileName + u"\""_s;
        }
    }
    QHttpServerResponse result(outbuff.data());
    result.addHeader("Content-Type"_ba, baContentType);
    if(!sContentDisposition.isEmpty())
        result.addHeader("Content-Disposition"_ba, sContentDisposition.toUtf8());
    return result;
}


#endif

void opds_server::server_run()
{
    if(options.nOpdsPort != port && OPDS_server_status == 1)
    {
        stop_server();
    }
#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
    QNetworkProxy OPDSproxy;
    OPDSproxy.setType(QNetworkProxy::NoProxy);
    OPDS_server.setProxy(OPDSproxy);
#endif
    port = options.nOpdsPort;
    if(options.bOpdsEnable)
    {
        if(OPDS_server_status == 0)
        {
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
            if(!httpServer_.listen(QHostAddress::Any, port))
                qDebug() << "Unable to start the server.";
#else
            if(!OPDS_server.listen(QHostAddress::Any, port))
                qDebug()<<QStringLiteral("Unable to start the server: %1.").arg(OPDS_server.errorString());
#endif
            else
                OPDS_server_status = 1;
        }
    }
    else
    {
        stop_server();
    }
}

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#define session_number_len  16
void opds_server::slotRead()
{
    QTcpSocket* clientSocket = qobject_cast<QTcpSocket*>(sender());
    if(clientSocket->bytesAvailable() == 0)
        return;
    qintptr idusersocs = clientSocket->socketDescriptor();
    QStringList TCPtokens;
    QStringList AUTHtokens;
    for_preview = false;
    for_mobile = false;
    //bool PWDCHECK=false;
    while(clientSocket->bytesAvailable() > 0)
    {
        QString str = clientSocket->readLine();
        static const QRegularExpression re(QStringLiteral("[ \r\n][ \r\n]*"));
        QStringList tokens = str.split(re);
        if (tokens[0] == QLatin1String("GET") || tokens[0] == QLatin1String("POST"))
            TCPtokens=tokens;
        if (tokens[0] == QLatin1String("Authorization:"))
            AUTHtokens=tokens;
        if(tokens[0] == QLatin1String("X-Purpose:"))
            for_preview=(tokens[1]==QLatin1String("preview"));
        if(tokens[0] == QLatin1String("User-Agent:"))
            for_mobile = (tokens.contains(QStringLiteral("Mobile"), Qt::CaseInsensitive));
//        if(tokens[0] == "Cookie:")
//            for_mobile=(tokens.contains("PWDCHECK=1",Qt::CaseInsensitive));
//        qDebug()<<tokens;
    }
    if (TCPtokens.count() > 0)
    {
        QTextStream os(clientSocket);
        os.setAutoDetectUnicode(true);
        bool auth = true;
        QString session;
        if(options.bOpdsNeedPassword)
        {
            auth = false;
            if(AUTHtokens.count() > 2)
            {
                QByteArray ba;
                ba.append(AUTHtokens.at(2).toLatin1());
                QStringList auth_str = QString(QByteArray::fromBase64(ba)).split(QStringLiteral(":"));
                if(auth_str.count() == 2)
                {
                    auth = (options.sOpdsUser == auth_str.at(0) && options.sOpdsPassword == auth_str.at(1));
                }
            }
            if(TCPtokens.at(1).contains(QLatin1String("session=")) && !auth)
            {
                int pos = TCPtokens.at(1).indexOf(QLatin1String("session="));
                session = TCPtokens.at(1).mid(pos + 8, session_number_len);
                auth = sessions.contains(session);
                if(auth)
                {
                    sessions[session] = QDateTime::currentDateTime();
                }
            }
            else
            {
                if(auth)
                {
                    //генерируем номер сессии
                    QString chars = QStringLiteral("abdefhiknrstyzABDEFGHKNQRSTYZ23456789");
                    session = QLatin1String("");
                    for(int i=0; i<session_number_len; i++)
                        session += chars[rand() % 32];
                    sessions.insert(session, QDateTime::currentDateTime());
                }
            }
            foreach (const QString &key, sessions.keys())
            {
                if(sessions.value(key) < QDateTime::currentDateTime().addSecs(-60*60))
                    sessions.remove(key);
            }
        }
        if(auth)
        {
            process(TCPtokens.at(1), os, session);
        }
        else
        {
            os << QLatin1String("HTTP/1.1 401 Authorization Required\n"
            "WWW-Authenticate: Basic\n"
            "Content-Type: text/html;charset=utf-8\n"
            "Connection: close\n\n");
        }
        os.flush();
        clientSocket->flush();
    }
    clientSocket->close();
    OPDS_clients.remove(idusersocs);
}

void opds_server::new_connection()
{
    if(OPDS_server_status == 1)
    {
        QTcpSocket* clientSocket = OPDS_server.nextPendingConnection();
        int idusersocs = clientSocket->socketDescriptor();
        OPDS_clients[idusersocs] = clientSocket;
        connect(OPDS_clients[idusersocs], &QIODevice::readyRead, this, &opds_server::slotRead);
    }
}

void opds_server::convert(uint idLib, uint idBook, const QString &format, const QString &file_name, bool opds, QTextStream &ts)
{
    QBuffer outbuff;
    QFileInfo fi_book;
    SLib &lib = mLibs[idLib];
    if(idBook == 0)
    {
        fi_book.setFile(file_name);
        QFile file(file_name);
        file.open(QFile::ReadOnly);
        outbuff.setData(file.readAll());
    }
    else
    {
        fi_book = lib.getBookFile(idBook, &outbuff);
    }

    if(outbuff.size() != 0)
    {
        QString book_file_name = pExportOptions_->sExportFileName;
        if(book_file_name.isEmpty())
            book_file_name = QLatin1String(ExportOptions::sDefaultEexpFileName);
        book_file_name = lib.fillParams(book_file_name, idBook) + QLatin1String(".") + (format == QLatin1String("download") ? fi_book.suffix().toLower() :format);
        if(pExportOptions_->bTransliteration)
            book_file_name=Transliteration(book_file_name);
        book_file_name.replace(' ', '_');
        book_file_name.replace('\"', '_');
        book_file_name.replace('\'', '_');
        book_file_name.replace(',', '_');
        book_file_name.replace(QLatin1String("__"), QLatin1String("_"));
        //book_file_name.replace("\\","_");
        //book_file_name.replace("/","_");
        QFileInfo book_file(book_file_name);
        book_file_name=book_file.fileName();
        if(pExportOptions_->bOriginalFileName)
            book_file_name = fi_book.completeBaseName() + QLatin1String(".") + format;;
        if(format == QLatin1String("epub") || format == QLatin1String("mobi") || format == QLatin1String("azw3"))
        {
            QFile file;
            QString tmp_dir;
            if(QStandardPaths::standardLocations(QStandardPaths::TempLocation).count() > 0)
                tmp_dir = QStandardPaths::standardLocations(QStandardPaths::TempLocation).at(0);
            QDir().mkpath(tmp_dir + QLatin1String("/freeLib"));
            file.setFileName(tmp_dir + QLatin1String("/freeLib/book0.") + fi_book.suffix());
            file.open(QFile::WriteOnly);
            file.write(outbuff.data());
            file.close();
            QFileInfo fi(file);


            fb2mobi conv(pExportOptions_, idLib);
            QString out_file = conv.convert(QStringList() << fi.absoluteFilePath(), idBook);
            file.setFileName(out_file);
            file.open(QFile::ReadOnly);
            outbuff.close();
            outbuff.setData(file.readAll());
            if(opds)
            {
                if(format == QLatin1String("epub"))
                    ts << WriteSuccess(QStringLiteral("application/epub+zip"));
                else
                    ts << WriteSuccess(QStringLiteral("application/x-mobipocket-ebook"));
            }
            else
            {
                if(format == QLatin1String("epub"))
                    ts << WriteSuccess(QLatin1String("application/epub+zip\nContent-Disposition: attachment; filename=\"") + book_file_name + QLatin1String("\""));
                else if(format == QLatin1String("mobi"))
                    ts << WriteSuccess(QLatin1String("application/x-mobipocket-ebook\nContent-Disposition: attachment; filename=\"") + book_file_name + QLatin1String("\""));
                else
                    ts << WriteSuccess(QLatin1String("text/plain; charset=UTF-8\nContent-Disposition: attachment; filename=\"") + book_file_name + QLatin1String("\""));
            }
        }
        else
        {
            ts << WriteSuccess(QLatin1String("text/plain; charset=UTF-8\nContent-Disposition: attachment; filename=\"") + book_file_name + QLatin1String("\""));
        }
        ts.flush();
        ts.device()->write(outbuff.data());
    }
}
#endif
