#define QT_USE_QSTRINGBUILDER
#include "opds_server.h"

#include <algorithm>
#include <unordered_set>
#include <QSettings>
#include <QTcpSocket>
#include <QNetworkProxy>
#include <QStringBuilder>
#include <QDir>
#include <QRegularExpression>

#include "config-freelib.h"
#include "fb2mobi/fb2mobi.h"
#include "utilites.h"
#include "bookfile.h"

#define SAVE_INDEX  4
#define MAX_COLUMN_COUNT    3

opds_server::opds_server(QObject *parent) :
    QObject(parent)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
    httpServer_.route(u"/"_s, QHttpServerRequest::Method::Get, [this](const QHttpServerRequest &request)
    {
        return rootHTTP(0, request);
    });

    httpServer_.route(u"/http"_s, QHttpServerRequest::Method::Get, [this](const QHttpServerRequest &request)
    {
        return rootHTTP(0, request);
    });

    httpServer_.route(u"/opds"_s, QHttpServerRequest::Method::Get, [this](const QHttpServerRequest &request)
    { return rootOPDS(0, request); });

    httpServer_.route(u"/http_<arg>"_s, QHttpServerRequest::Method::Get, [this](uint idLib, const QHttpServerRequest &request)
    {
        return rootHTTP(idLib, request);
    });

    httpServer_.route(u"/opds_<arg>"_s, QHttpServerRequest::Method::Get, [this](uint idLib, const QHttpServerRequest &request)
    { return rootOPDS(idLib, request); });

    httpServer_.route(u"/<arg>.png"_s, QHttpServerRequest::Method::Get, [](const QString &sUrl)
    {
        QString ico = u":/xsl/opds/"_s % sUrl % u".png"_s;
        QFile file(ico);
        file.open(QFile::ReadOnly);
        QByteArray ba = file.readAll();
        if(ba.isEmpty())
            return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
        return QHttpServerResponse(ba);
    });

    httpServer_.route(u"/<arg>.css"_s, QHttpServerRequest::Method::Get, [](const QString &sUrl)
    {
        QString sCSS = u":/xsl/opds/"_s % sUrl % u".css"_s;
        QFile file(sCSS);
        file.open(QFile::ReadOnly);
        QByteArray ba = file.readAll();
        if(ba.isEmpty())
            return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
        QHttpServerResponse response("text/css"_ba, ba);
        response.addHeader("Cache-Control"_ba,"max-age=3600"_ba);
        return response;
    });


    httpServer_.route(u"/http_<arg>/covers/<arg>/cover.jpg"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idBook/*, const QHttpServerRequest &request*/)
    {
        QByteArray ba = cover(idLib, idBook);
        if(ba.isEmpty())
            return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
        return QHttpServerResponse(ba);
    });

    httpServer_.route(u"/opds_<arg>/covers/<arg>/cover.jpg"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idBook/*, const QHttpServerRequest &request*/)
    {
        QByteArray ba = cover(idLib, idBook);
        if(ba.isEmpty())
            return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
        return QHttpServerResponse(ba);
    });

    httpServer_.route(u"/http_<arg>/authorsindex"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QHttpServerRequest &request)
    {
        return authorsIndexHTTP(idLib, u""_s, false, request);
    });

    httpServer_.route(u"/opds_<arg>/authorsindex"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QHttpServerRequest &request)
    { return authorsIndexOPDS(idLib, u""_s, false, request); });

    httpServer_.route(u"/http_<arg>/authorsindex/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return authorsIndexHTTP(idLib, sIndex, false, request); });

    httpServer_.route(u"/opds_<arg>/authorsindex/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return authorsIndexOPDS(idLib, sIndex, false, request); });

    httpServer_.route(u"/http_<arg>/authorsindex/<arg>/books"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return authorsIndexHTTP(idLib, sIndex, true, request); });

    httpServer_.route(u"/opds_<arg>/authorsindex/<arg>/books"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return authorsIndexOPDS(idLib, sIndex, true, request); });

    httpServer_.route(u"/http_<arg>/author/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idAuthor, const QHttpServerRequest &request)
    { return authorHTTP(idLib, idAuthor, request); });

    httpServer_.route(u"/opds_<arg>/author/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idAuthor, const QHttpServerRequest &request)
    { return authorOPDS(idLib, idAuthor, request); });

    httpServer_.route(u"/http_<arg>/authorbooks/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idAuthor, const QHttpServerRequest &request)
    { return authorBooksHTTP(idLib, idAuthor, request); });

    httpServer_.route(u"/opds_<arg>/authorbooks/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idAuthor, const QHttpServerRequest &request)
    { return authorBooksOPDS(idLib, idAuthor, request); });

    httpServer_.route(u"/http_<arg>/authorsequences/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idAuthor, const QHttpServerRequest &request)
    { return authorSequencesHTTP(idLib, idAuthor, request); });

    httpServer_.route(u"/opds_<arg>/authorsequences/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idAuthor, const QHttpServerRequest &request)
    { return authorSequencesOPDS(idLib, idAuthor, request); });

    httpServer_.route(u"/http_<arg>/authorsequence/<arg>/<arg>"_s, QHttpServerRequest::Method::Get, [this](uint idLib, uint idAuthor, uint idSequence, const QHttpServerRequest &request)
    { return authorSequencesHTTP(idLib, idAuthor, idSequence, request); });

    httpServer_.route(u"/opds_<arg>/authorsequence/<arg>/<arg>"_s, QHttpServerRequest::Method::Get, [this](uint idLib, uint idAuthor, uint idSequence, const QHttpServerRequest &request)
    { return authorSequencesOPDS(idLib, idAuthor, idSequence, request); });

    httpServer_.route(u"/http_<arg>/authorsequenceless/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idAuthor, const QHttpServerRequest &request)
    { return authorSequencelessHTTP(idLib, idAuthor, request); });

    httpServer_.route(u"/opds_<arg>/authorsequenceless/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idAuthor, const QHttpServerRequest &request)
    { return authorSequencelessOPDS(idLib, idAuthor, request); });

    httpServer_.route(u"/http_<arg>/sequencesindex"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QHttpServerRequest &request)
    { return sequencesIndexHTTP(idLib, u""_s, false, request); });

    httpServer_.route(u"/opds_<arg>/sequencesindex"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QHttpServerRequest &request)
    { return sequencesIndexOPDS(idLib, u""_s, false, request); });

    httpServer_.route(u"/http_<arg>/sequencesindex/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return sequencesIndexHTTP(idLib, sIndex, false, request); });

    httpServer_.route(u"/opds_<arg>/sequencesindex/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return sequencesIndexOPDS(idLib, sIndex, false, request); });

    httpServer_.route(u"/http_<arg>/sequencesindex/<arg>/books"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return sequencesIndexHTTP(idLib, sIndex, true, request); });

    httpServer_.route(u"/opds_<arg>/sequencesindex/<arg>/books"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return sequencesIndexOPDS(idLib, sIndex, true, request); });

    httpServer_.route(u"/http_<arg>/sequencebooks/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idSequence, const QHttpServerRequest &request)
    { return sequenceBooksHTTP(idLib, idSequence, request); });

    httpServer_.route(u"/opds_<arg>/sequencebooks/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idSequence, const QHttpServerRequest &request)
    { return sequenceBooksOPDS(idLib, idSequence, request); });

    httpServer_.route(u"/http_<arg>/genres"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QHttpServerRequest &request)
    { return genresHTTP(idLib, 0, request); });

    httpServer_.route(u"/opds_<arg>/genres"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QHttpServerRequest &request)
    { return genresOPDS(idLib, 0, request); });

    httpServer_.route(u"/http_<arg>/genres/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idGenre, const QHttpServerRequest &request)
    {        return genresHTTP(idLib, idGenre, request);    });

    httpServer_.route(u"/opds_<arg>/genres/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idGenre, const QHttpServerRequest &request)
    { return genresOPDS(idLib, idGenre, request); });

    httpServer_.route(u"/http_<arg>/book/<arg>/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idBook, const QString &sFormat)
    { return bookHTTP(idLib, idBook, sFormat); });

    httpServer_.route(u"/opds_<arg>/book/<arg>/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idBook, const QString &sFormat)
    { return bookOPDS(idLib, idBook, sFormat); });

    httpServer_.route(u"/http_<arg>/search"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QHttpServerRequest &request)
    { return searchHTTP(idLib, request); });

    httpServer_.route(u"/http_<arg>/searchauthor"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QHttpServerRequest &request)
                      { return searchAuthorHTTP(idLib, request); });

    httpServer_.route(u"/opds_<arg>/search"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QHttpServerRequest &request)
    { return searchOPDS(idLib, request); });

    httpServer_.route(u"/opds_<arg>/opensearch.xml"_s, QHttpServerRequest::Method::Get,  [](uint /*idLib*/, const QHttpServerRequest &request){
        QString sUrl = request.url().toString();
        sUrl.chop(u"opensearch.xml"_s.size());
        QString sTemplate = sUrl +u"search?search_string={searchTerms}"_s;
        QString result = u"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
               "<OpenSearchDescription xmlns=\"http://a9.com/-/spec/opensearch/1.1/\">"
                "<ShortName>freeLib</ShortName>"
                "<Description>Search on freeLib</Description>"
                "<InputEncoding>UTF-8</InputEncoding>"
                "<OutputEncoding>UTF-8</OutputEncoding>"
                "<Url type=\"application/atom+xml\" template=\""_s + sTemplate + u"\"/>"_s
                u"<Url type=\"application/atom+xml;profile=opds-catalog;kind=acquisition\" template=\""_s + sTemplate + u"\"/>"_s
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


std::vector<uint> opds_server::book_list(const SLib &lib, uint idAuthor, uint idSeria, ushort idGenre, const QString &sSearch, bool sequenceless = false)
{
    std::vector<uint> vBooks;
    if(idAuthor !=0 && idSeria != 0){
        auto range = lib.authorBooksLink.equal_range(idAuthor);
        for (auto it = range.first; it != range.second; ++it) {
            auto &book = lib.books.at(it->second);
            if(!book.bDeleted && book.idSerial == idSeria)
                if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == lib.vLaguages[book.idLanguage])
                    vBooks.push_back(it->second);
        }
        std::sort(vBooks.begin(), vBooks.end(),[&lib](uint lhs, uint rhs){ return lib.books.at(lhs).numInSerial < lib.books.at(rhs).numInSerial; });
    }
    if(idAuthor != 0 && idSeria == 0){
        auto range = lib.authorBooksLink.equal_range(idAuthor);
        if(sequenceless){
            for (auto it = range.first; it != range.second; ++it) {
                auto &book = lib.books.at(it->second);
                if(!lib.books.at(it->second).bDeleted && lib.books.at(it->second).idSerial == 0)
                    if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == lib.vLaguages[book.idLanguage])
                        vBooks.push_back(it->second);
            }
        }else{
            for (auto it = range.first; it != range.second; ++it) {
                auto &book = lib.books.at(it->second);
                if(!lib.books.at(it->second).bDeleted)
                    if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == lib.vLaguages[book.idLanguage])
                        vBooks.push_back(it->second);
            }
        }
        std::sort(vBooks.begin(), vBooks.end(), [&lib](uint lhs, uint rhs){ return QString::localeAwareCompare(lib.books.at(lhs).sName, lib.books.at(rhs).sName) < 0; });
    }
    if(idAuthor == 0 && idSeria != 0){
        for(const auto &iBook :lib.books){
            if(!iBook.second.bDeleted && iBook.second.idSerial == idSeria)
                if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == lib.vLaguages[iBook.second.idLanguage])
                    vBooks.push_back(iBook.first);
        }
        std::sort(vBooks.begin(), vBooks.end(),[&lib](uint lhs, uint rhs){ return lib.books.at(lhs).numInSerial < lib.books.at(rhs).numInSerial; });
    }
    if(idGenre != 0){
        for(const auto &iBook :lib.books){
            if(!iBook.second.bDeleted){
                for(auto iGenre: iBook.second.vIdGenres){
                    if(iGenre == idGenre){
                        if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == lib.vLaguages[iBook.second.idLanguage]){
                            vBooks.push_back(iBook.first);
                            break;
                        }
                    }
                }
            }
        }
        std::sort(vBooks.begin(), vBooks.end(), [&lib](uint lhs, uint rhs){ return QString::localeAwareCompare(lib.books.at(lhs).sName,lib.books.at(rhs).sName) < 0; });
    }
    if(!sSearch.isEmpty()){
        for(const auto &iBook :lib.books){
            if(!iBook.second.bDeleted){
                if(iBook.second.sName.contains(sSearch, Qt::CaseInsensitive)){
                    if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == lib.vLaguages[iBook.second.idLanguage])
                        vBooks.push_back(iBook.first);
                }else{
                    for(uint idAuthor: iBook.second.vIdAuthors){
                        if(lib.authors.at(idAuthor).getName().contains(sSearch, Qt::CaseInsensitive)){
                            if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == lib.vLaguages[iBook.second.idLanguage]){
                                vBooks.push_back(iBook.first);
                                break;
                            }
                        }
                    }
                }
            }
        }
        std::sort(vBooks.begin(), vBooks.end(), [&lib](uint lhs, uint rhs){
            if(lib.books.at(lhs).idFirstAuthor != lib.books.at(rhs).idFirstAuthor)
                return QString::localeAwareCompare(lib.authors.at(lib.books.at(lhs).idFirstAuthor).getName(), lib.authors.at(lib.books.at(rhs).idFirstAuthor).getName()) < 0;
            else
                return QString::localeAwareCompare(lib.books.at(lhs).sName, lib.books.at(rhs).sName) < 0;

        });

    }
    return vBooks;
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
                              bMobile_ ?QStringLiteral("3") :QStringLiteral("2"), bMobile_ ?QStringLiteral("1.5") :QStringLiteral("1"),
                              bMobile_ ?QStringLiteral("2") :QStringLiteral("1.5"), bMobile_ ?QStringLiteral("8") :QStringLiteral("6"));
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
        img.setAttribute(QStringLiteral("height"), QStringLiteral("%1px").arg(bMobile_ ?QStringLiteral("48") :QStringLiteral("32")));
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

QString opds_server::FillPage(std::vector<uint> listBooks, SLib& lib, const QString &sTitle, const QString &lib_url, const QString &current_url, QTextStream &ts, bool opds, uint nPage, const QString &session, bool bShowAuthor)
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
    if(listBooks.empty())
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
                SBook& book = lib.books[idBook];
                QString sIdBook = QString::number(idBook);
                QDomElement entry = doc.createElement(QStringLiteral("entry"));
                feed.appendChild(entry);
                AddTextNode(QStringLiteral("updated"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate), entry);
                AddTextNode(QStringLiteral("id"), QStringLiteral("tag:book:%1").arg(idBook), entry);
                QString sSerial = book.idSerial == 0 ?QString() :lib.serials[book.idSerial].sName;
                AddTextNode(QStringLiteral("title"), book.sName + (sSerial.isEmpty() ?QString() :QLatin1String(" (") + sSerial + QLatin1String(")")), entry);
                foreach(uint idAuthor, book.vIdAuthors){
                    QDomElement author = doc.createElement(QStringLiteral("author"));
                    entry.appendChild(author);
                    AddTextNode(QStringLiteral("name"), lib.authors[idAuthor].getName(), author);
                }
                foreach(uint idGenre, book.vIdGenres){
                    QDomElement category = doc.createElement(QStringLiteral("category"));
                    entry.appendChild(category);
                    category.setAttribute(QStringLiteral("term"), genres[idGenre].sName);
                    category.setAttribute(QStringLiteral("label"), genres[idGenre].sName);
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
                    if(book.sFormat == QLatin1String("fb2"))
                    {
                        if(book.sAnnotation.isEmpty()){
                            BookFile file(&lib, idBook);
                            book.sAnnotation = file.annotation();
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
                SBook& book = lib.books[idBook];
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
                    QDomElement el = AddTextNode(QStringLiteral("a"), lib.authors[book.idFirstAuthor].getName(), entry);
                    el.setAttribute(QStringLiteral("class"), QStringLiteral("book"));
                    el.setAttribute(QStringLiteral("href"), QStringLiteral("/author/%1").arg(book.idFirstAuthor) +
                                    (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
                }

                QString sSerial = book.idSerial == 0 ?QString() :lib.serials[book.idSerial].sName;
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
                    if(book.sFormat == QLatin1String("fb2"))
                    {
                        if(book.sAnnotation.isEmpty()){
                            BookFile file(&lib, idBook);
                            book.sAnnotation = file.annotation();
                        }
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
    auto timeExpire = QDateTime::currentDateTime().addSecs(-60*60);
    erase_if(sessions, [&timeExpire](const auto &it) { return it.second < timeExpire; });

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
                auto colon = userPass.indexOf(':');
                if (colon > 0)
                {
                    QByteArrayView view = userPass;
                    QString sUser = QString::fromUtf8(view.first(colon));
                    QString password = QString::fromUtf8(view.sliced(colon + 1));
                    if(!options.baOpdsPasswordSalt.isEmpty() && !options.baOpdsPasswordHash.isEmpty()){
                        auto hashPassword = passwordToHash(password, options.baOpdsPasswordSalt);

                        if (sUser == options.sOpdsUser && hashPassword == options.baOpdsPasswordHash){
                            bResult = true;
                            static QString chars = u"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"_s;
                            const int sessionNumberLen = 16;
                            sSession = u""_s;
                            srand(time(nullptr));
                            for(int i=0; i<sessionNumberLen; i++)
                                sSession += chars[rand() % chars.size()];
                            sessions.insert({sSession, QDateTime::currentDateTime()});
                            if(urlquery.hasQueryItem(u"session"_s))
                                urlquery.removeQueryItem(u"session"_s);
                            urlquery.addQueryItem(u"session"_s, sSession);
                            url.setQuery(urlquery);
                        }
                    }
                }
            }
        }
    }else
        bResult = true;

    return bResult;
}

QDomElement opds_server::docHeaderHTTP(const QString &sSesionQuery, const QString &sLibName, const QString &sLibUrl)
{
    doc.clear();
    QDomProcessingInstruction xmlProcessingInstruction = doc.createProcessingInstruction(u"DOCTYPE"_s, u"HTML"_s);
    doc.appendChild(xmlProcessingInstruction);
    QDomElement html = doc.createElement(u"html"_s);
    doc.appendChild(html);
    QDomElement head = doc.createElement(u"head"_s);
    html.appendChild(head);
    AddTextNode(u"title"_s, sLibName, head);

    QDomElement link;
    link = doc.createElement(u"link"_s);
    link.setAttribute(u"rel"_s, u"stylesheet"_s);
    link.setAttribute(u"href"_s, u"/styles.css"_s);
    head.appendChild(link);


    QDomElement meta = doc.createElement(u"META"_s);
    meta.setAttribute(u"charset"_s, u"utf-8"_s);
    head.appendChild(meta);

    if(!options.sBaseUrl.isEmpty()){
        QDomElement baseUrl = doc.createElement(u"base"_s);
        baseUrl.setAttribute(u"href"_s, options.sBaseUrl);
        head.appendChild(baseUrl);
    }

    QString sIconFile = u"/icon_256x256.png"_s;

    link = doc.createElement(u"link"_s);
    link.setAttribute(u"rel"_s, u"icon"_s);
    link.setAttribute(u"type"_s, u"image/png"_s);
    link.setAttribute(u"href"_s, sIconFile);
    head.appendChild(link);
    link = doc.createElement(u"link"_s);
    link.setAttribute(u"rel"_s, u"shortcut icon"_s);
    link.setAttribute(u"type"_s, u"image/png"_s);
    link.setAttribute(u"href"_s, sIconFile + sSesionQuery);
    head.appendChild(link);
    link = doc.createElement(u"link"_s);
    link.setAttribute(u"rel"_s, u"apple-touch-icon"_s);
    link.setAttribute(u"type"_s, u"image/png"_s);
    link.setAttribute(u"href"_s, sIconFile + sSesionQuery);
    head.appendChild(link);

    QDomElement body = doc.createElement(u"body"_s);
    html.appendChild(body);
    QDomElement div = AddTextNode(u"a"_s, u""_s, body);
    div.setAttribute(u"class"_s, u"lib"_s);
    div.setAttribute(u"href"_s, (sLibUrl.isEmpty() ?u"/"_s :sLibUrl) + sSesionQuery);
    QDomElement img = doc.createElement(u"img"_s);
    img.setAttribute(u"src"_s, u"/home.png"_s);
    img.setAttribute(u"border"_s, u"0"_s);
    img.setAttribute(u"class"_s,u"home"_s);
    div.appendChild(img);
    div = AddTextNode(u"a"_s, sLibName, body);
    div.setAttribute(u"class"_s, u"lib"_s);
    div.setAttribute(u"href"_s, (sLibUrl.isEmpty() ?u"/"_s :sLibUrl) + sSesionQuery);

    QDomElement hr = doc.createElement(u"hr"_s);
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

SLib* opds_server::getLib(uint &idLib, const QString &sTypeServer, QString *pLibUrl)
{
    if(idLib == 0)
        idLib = idCurrentLib;
    if(pLibUrl != nullptr)
        *pLibUrl = sTypeServer +u"_"_s + QString::number(idLib);
    if(!libs.contains(idLib) || !libs.at(idLib).bLoaded)
        loadLibrary(idLib);
    if(!libs.contains(idLib))
        return nullptr;
    SLib &lib = libs[idLib];
    lib.timeHttp = std::chrono::system_clock::now();
    return &lib;
}

QHttpServerResponse opds_server::responseHTTP()
{
    QString str = u"<!DOCTYPE html>\n"_s;
    QTextStream ts(&str, QIODevice::WriteOnly);
    doc.namedItem(u"HTML"_s).save(ts, 2);
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

QHttpServerResponse opds_server::FillPageHTTP(const std::vector<uint> &vBooks, SLib &lib, const QString &sTitle, const QString &sLibUrl, const QUrl &url, bool bShowAuthor)
{
    bool bKindleInstallsed = kindlegenInstalled();
    auto nMaxBooksPerPage = options.nOpdsBooksPerPage;
    if(nMaxBooksPerPage == 0)
        nMaxBooksPerPage = std::numeric_limits<typeof(nMaxBooksPerPage)>::max();

    QUrlQuery urlquery(url);
    uint nPage = urlquery.queryItemValue(u"page"_s).toUInt();
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSesionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;


    uint iBookBegin = nPage*nMaxBooksPerPage;
    uint iBookEnd = std::min(static_cast<uint>(vBooks.size()), (nPage+1)*nMaxBooksPerPage);
    if(options.bOpdsShowAnotation)
    {
        std::vector<uint> vBooksNeedAnnotations;
        for(uint iBook = iBookBegin; iBook < iBookEnd; ++iBook){
            uint idBook = vBooks.at(iBook);
            SBook& book = lib.books.at(idBook);
            if(book.sAnnotation.isEmpty())
                vBooksNeedAnnotations.push_back(idBook);
        }

        QtConcurrent::blockingMap(vBooksNeedAnnotations, [&](auto idBook){
            SBook& book = lib.books.at(idBook);
            BookFile file(&lib, idBook);
            book.sAnnotation = file.annotation();
        });
    }

    QDomElement feed;
    feed = docHeaderHTTP(sSesionQuery, lib.name, sLibUrl);
    QDomElement div = AddTextNode(u"DIV"_s, sTitle, feed);
    div.setAttribute(u"class"_s, u"caption"_s);

    for(uint iBook = iBookBegin; iBook < iBookEnd; ++iBook){
        uint idBook = vBooks.at(iBook);
        SBook& book = lib.books.at(idBook);
        QString sIdBook = QString::number(idBook);
        QDomElement entry = doc.createElement(u"div"_s);
        entry.setAttribute(u"class"_s, u"item"_s);
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
            QDomElement el = AddTextNode(u"a"_s, lib.authors.at(book.idFirstAuthor).getName(), entry);
            el.setAttribute(u"class"_s, u"book"_s);
            el.setAttribute(u"href"_s, sLibUrl + u"/author/"_s + QString::number(book.idFirstAuthor) + sSesionQuery);
        }

        QString sSerial = book.idSerial == 0 ?u""_s :lib.serials.at(book.idSerial).sName;
        QDomElement el = AddTextNode(u"div"_s, book.sName + (sSerial.isEmpty() || book.numInSerial==0  ?u""_s
                                                                                                        :(u" ("_s + sSerial + u"["_s + QString::number(book.numInSerial) + u"])"_s)), entry);
        el.setAttribute(u"class"_s, u"book"_s);
        QDomElement br = doc.createElement(u"br"_s);
        entry.appendChild(br);

        if(book.sFormat == u"fb2"_s)
        {
            QDomElement el = AddTextNode(u"a"_s, u"fb2"_s, entry);
            el.setAttribute(u"href"_s, sLibUrl + u"/book/"_s + sIdBook + u"/fb2"_s + sSesionQuery);
            el.setAttribute(u"class"_s, u"item"_s);
            el = AddTextNode(u"a"_s, u"epub"_s, entry);
            el.setAttribute(u"href"_s, sLibUrl + u"/book/"_s + sIdBook + u"/epub"_s + sSesionQuery);
            el.setAttribute(u"class"_s, u"item"_s);
            if(bKindleInstallsed){
                el=AddTextNode(u"a"_s, u"mobi"_s, entry);
                el.setAttribute(u"href"_s, sLibUrl + u"/book/"_s + sIdBook + u"/mobi"_s + sSesionQuery);
                el.setAttribute(u"class"_s, u"item"_s);
                el = AddTextNode(u"a"_s, u"azw3"_s, entry);
                el.setAttribute(u"href"_s, sLibUrl + u"/book/"_s + sIdBook + u"/azw3"_s + sSesionQuery);
                el.setAttribute(u"class"_s, u"item"_s);
            }
        }
        else if(book.sFormat == u"epub"_s)
        {
            QDomElement el = AddTextNode(u"a"_s, u"epub"_s, entry);
            el.setAttribute(u"href"_s, sLibUrl + u"/book/"_s + sIdBook + u"/epub"_s + sSesionQuery);
            el.setAttribute(u"class"_s, u"item"_s);
            if(bKindleInstallsed){
                el = AddTextNode(u"a"_s, u"mobi"_s, entry);
                el.setAttribute(u"href"_s, sLibUrl + u"/book/"_s + sIdBook + u"/mobi"_s + sSesionQuery);
                el.setAttribute(u"class"_s, u"item"_s);
            }
        }
        else if(book.sFormat == u"mobi"_s)
        {
            QDomElement el = AddTextNode(u"a"_s, u"mobi"_s, entry);
            el.setAttribute(u"href"_s, sLibUrl + u"/book/"_s + sIdBook + u"/mobi"_s + sSesionQuery);
            el.setAttribute(u"class"_s, u"item"_s);
        }
        else
        {
            QDomElement el = AddTextNode(u"a"_s, book.sFormat, entry);
            el.setAttribute(u"href"_s, sLibUrl + u"/book/"_s + sIdBook + u"/download"_s + sSesionQuery);
            el.setAttribute(u"class"_s, u"item"_s);
        }

        if(options.bOpdsShowAnotation)
        {
            QDomDocument an;
            an.setContent(u"<div>"_s + book.sAnnotation + u"</div>"_s);
            QDomNode an_node = doc.importNode(an.childNodes().at(0), true);
            entry.appendChild(an_node);
        }
    }
    if(nPage >= 1){
        QString sHref = url.toString(QUrl::RemoveQuery) % u"?page="_s % QString::number(nPage-1);
        if(!sSession.isEmpty())
            sHref += u"&session="_s + sSession;

        QDomElement entry = doc.createElement(u"div"_s);
        feed.appendChild(entry);
        QDomElement el = AddTextNode(u"a"_s, tr("Previous page"), entry);
        el.setAttribute(u"href"_s, sHref);
        el.setAttribute(u"class"_s, u"item"_s);
    }
    if(static_cast<uint>(vBooks.size()) > (nPage+1)*nMaxBooksPerPage){
        QString sHref = url.toString(QUrl::RemoveQuery) % u"?page="_s % QString::number(nPage+1);
        if(!sSession.isEmpty())
            sHref += u"&session="_s + sSession;

        QDomElement entry = doc.createElement(u"div"_s);
        feed.appendChild(entry);

        QDomElement el = AddTextNode(u"a"_s, tr("Next page"), entry);
        el.setAttribute(u"href"_s, sHref);
        el.setAttribute(u"class"_s, u"item"_s);

    }
    QString str = u"<!DOCTYPE html>\n"_s;
    QTextStream ts(&str);
    doc.namedItem(u"HTML"_s).save(ts, 2);
    QHttpServerResponse result(str);
    result.addHeader("Server"_ba, "freeLib "_ba + FREELIB_VERSION);
    result.addHeader("Connection"_ba, "keep-alive"_ba);
    result.addHeader("Pragma"_ba, "no-cache"_ba);

    return result;
}

QString opds_server::FillPageOPDS(const std::vector<uint> &vBooks, SLib &lib, const QString &sTitle,const QString &sId, const QString &sLibUrl, const QUrl &url)
{
    auto nMaxBooksPerPage = options.nOpdsBooksPerPage;
    if(nMaxBooksPerPage == 0)
        nMaxBooksPerPage = std::numeric_limits<typeof(nMaxBooksPerPage)>::max();

    QUrlQuery urlquery(url);
    uint nPage = urlquery.queryItemValue(u"page"_s).toUInt();
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSesionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    uint iBookBegin = nPage*nMaxBooksPerPage;
    uint iBookEnd = std::min(static_cast<uint>(vBooks.size()), (nPage+1)*nMaxBooksPerPage);
    if(options.bOpdsShowAnotation)
    {
        std::vector<uint> vBooksNeedAnnotations;
        for(uint iBook = iBookBegin; iBook < iBookEnd; ++iBook){
            uint idBook = vBooks.at(iBook);
            SBook& book = lib.books.at(idBook);
            if(book.sAnnotation.isEmpty())
                vBooksNeedAnnotations.push_back(idBook);
        }

        QtConcurrent::blockingMap(vBooksNeedAnnotations, [&](auto idBook){
            SBook& book = lib.books.at(idBook);
            BookFile file(&lib, idBook);
            book.sAnnotation = file.annotation();
        });
    }

    QDomElement feed = docHeaderOPDS(sTitle, sId, sLibUrl, sSesionQuery);

    for(uint iBook = iBookBegin; iBook < iBookEnd; ++iBook){
        uint idBook = vBooks.at(iBook);
        SBook& book = lib.books[idBook];
        QString sIdBook = QString::number(idBook);
        QDomElement entry = doc.createElement(u"entry"_s);
        feed.appendChild(entry);
        AddTextNode(u"updated"_s, book.date.toString(Qt::ISODate), entry);
        AddTextNode(u"id"_s, u"tag:book:"_s + QString::number(idBook), entry);
        QString sSerial = book.idSerial == 0 ?QString() :lib.serials[book.idSerial].sName;
        AddTextNode(u"title"_s, book.sName + (sSerial.isEmpty() ?QString() :u" ("_s + sSerial + u")"_s), entry);
        for(uint idAuthor: book.vIdAuthors){
            QDomElement author = doc.createElement(u"author"_s);
            entry.appendChild(author);
            AddTextNode(u"name"_s, lib.authors[idAuthor].getName(), author);
        }
        for(auto idGenre: book.vIdGenres){
            QDomElement category = doc.createElement(u"category"_s);
            entry.appendChild(category);
            category.setAttribute(u"term"_s, genres[idGenre].sName);
            category.setAttribute(u"label"_s, genres[idGenre].sName);
        }
        QDomElement el;
        if(book.sFormat == u"fb2"_s)
        {
            el = AddTextNode(u"link"_s, u""_s, entry);
            el.setAttribute(u"href"_s, sLibUrl + u"/book/"_s + sIdBook + u"/fb2"_s + sSesionQuery);
            el.setAttribute(u"rel"_s, u"http://opds-spec.org/acquisition/open-access"_s);
            el.setAttribute(u"type"_s, u"application/fb2"_s);

            el = AddTextNode(u"link"_s, u""_s, entry);
            el.setAttribute(u"href"_s, sLibUrl + u"/book/"_s + sIdBook + u"/epub"_s + sSesionQuery);
            el.setAttribute(u"rel"_s, u"http://opds-spec.org/acquisition/open-access"_s);
            el.setAttribute(u"type"_s, u"application/epub+zip"_s);
        }
        else if(book.sFormat == u"epub"_s || book.sFormat == u"mobi"_s){
            el = AddTextNode(u"link"_s, u""_s, entry);
            el.setAttribute(u"href"_s, sLibUrl + u"/book/"_s + sIdBook + u"/download"_s
                                           + sSesionQuery);
            el.setAttribute(u"rel"_s, u"http://opds-spec.org/acquisition/open-access"_s);
            el.setAttribute(u"type"_s, u"application/"_s + book.sFormat);
        }

        el = AddTextNode(u"link"_s, u""_s, entry);
        el.setAttribute(QStringLiteral("href"), sLibUrl + u"/book/"_s + sIdBook + u"/download"_s
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
            if(book.sAnnotation.isEmpty()){
                BookFile file(&lib, idBook);
                book.sAnnotation = file.annotation();
            }
            el = AddTextNode(u"content"_s, book.sAnnotation, entry);
            el.setAttribute(u"type"_s, u"text/html"_s);
        }
    }

    if(nPage >= 1){
        QString sHref = url.toString(QUrl::RemoveQuery) % u"?page="_s % QString::number(nPage-1);
        if(!sSession.isEmpty())
            sHref += u"&session="_s + sSession;

        QDomElement entry = doc.createElement(u"entry"_s);
        feed.appendChild(entry);
        AddTextNode(u"updated"_s, QDateTime::currentDateTimeUtc().toString(Qt::ISODate), entry);
        AddTextNode(u"id"_s, u"tag:root"_s, entry);
        AddTextNode(u"title"_s, tr("Previous page"), entry);
        QDomElement el = AddTextNode(u"link"_s, u""_s, entry);
        el.setAttribute(u"href"_s, sHref);
        el.setAttribute(u"type"_s, u"application/atom+xml;profile=opds-catalog"_s);

        el = AddTextNode(u"link"_s, u""_s, entry);
        el.setAttribute(u"href"_s, u"/arrow_left.png"_s + sSesionQuery);
        el.setAttribute(u"rel"_s, u"http://opds-spec.org/image"_s);
        el.setAttribute(u"type"_s, u"image/jpeg"_s);
    }
    if(static_cast<uint>(vBooks.size()) > (nPage+1)*nMaxBooksPerPage){
        QString sHref = url.toString(QUrl::RemoveQuery) % u"?page="_s % QString::number(nPage+1);
        if(!sSession.isEmpty())
            sHref += u"&session="_s + sSession;

        QDomElement entry = doc.createElement(u"entry"_s);
        feed.appendChild(entry);
        AddTextNode(u"updated"_s, QDateTime::currentDateTimeUtc().toString(Qt::ISODate), entry);
        AddTextNode(u"id"_s, u"tag:root"_s, entry);
        AddTextNode(u"title"_s, tr("Next page"), entry);
        QDomElement el = AddTextNode(u"link"_s, u""_s, entry);
        el.setAttribute(u"href"_s, sHref);
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
        int count = options.vExportOptions.size();
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
    SLib &lib = libs[id_lib];
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

        std::vector<uint> listBooks = book_list(lib, 0, 0, 0, QString(params.value(QStringLiteral("search_string"))).replace('+', ' ')
                                          .replace(QLatin1String("%20"), QLatin1String(" ")));
        ts << FillPage(listBooks, lib, tr("Books search"), lib_url, url, ts, opds, nPage, session, true);


    }
    else if(url.startsWith(QLatin1String("/sequencebooks"), Qt::CaseInsensitive) && strings.count() >= 3)
    {
        QString id_sequence = strings.at(2);
        std::vector<uint> listBooks = book_list(lib, 0, id_sequence.toUInt(), 0, QLatin1String(""));
        ts << FillPage(listBooks, lib, tr("Books of sequence") + QLatin1String(" (") + lib.serials[id_sequence.toUInt()].sName + QLatin1String(")"), lib_url, url, ts, opds, nPage, session, false);

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
        QMap <QString, int> mCount;
        QSet <QString> setSerials;
        int count = 0;
        for(const auto &iSerial :lib.serials){
            if(iSerial.second.sName.left(index.length()).toLower() == index.toLower()){
                count++;
                QString sNewIndex = iSerial.second.sName.left(index.length()+1).toLower();
                sNewIndex[0] = sNewIndex[0].toUpper();
                if(mCount.contains(sNewIndex))
                    mCount[sNewIndex]++;
                else
                    mCount[sNewIndex] = 1;
                if(sNewIndex.length() == iSerial.second.sName.length())
                    setSerials.insert(sNewIndex);
            }
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
                        for(const auto &iSerial :lib.serials){
                            if(setSerials.contains(iIndex) ?iSerial.second.sName.toLower() == iIndex.toLower()
                                    :iSerial.second.sName.left(iIndex.size()).toLower() == iIndex.toLower())
                            {
                                el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/sequencebooks/") + QString::fromLatin1(QUrl::toPercentEncoding(QString::number(iSerial.first)))
                                                + (session.isEmpty() && mCount[iIndex]<30 ?QString() :QLatin1String("?session=") + session));
                                break;
                            }
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
                        for(const auto &iSerial :lib.serials){
                            if(setSerials.contains(iIndex) ?iSerial.second.sName.toLower() == iIndex.toLower()
                                                           :iSerial.second.sName.left(iIndex.size()).toLower() == iIndex.toLower())
                            {
                                el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/sequencebooks/") + QString::fromLatin1(QUrl::toPercentEncoding(QString::number(iSerial.first))) +
                                                (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
                                break;
                            }
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
            for(const auto &iSerial :lib.serials)
            {
                if(iSerial.second.sName.left((index.length())).toLower() == index.toLower())
                {
                    listSerialId << iSerial.first;
                }
            }
            std::sort(listSerialId.begin(), listSerialId.end(),[&lib](uint lhs, uint rhs) {return lib.serials.at(lhs).sName < lib.serials.at(rhs).sName;});

            foreach(const auto &iIndex, listSerialId)
            {
                uint nBooksCount = 0;
                for(const auto &book :lib.books)
                {
                    if(book.second.idSerial == iIndex)
                        nBooksCount++;
                }
                if(opds)
                {
                    QDomElement entry = doc.createElement(QStringLiteral("entry"));
                    feed.appendChild(entry);
                    AddTextNode(QStringLiteral("updated"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate), entry);
                    AddTextNode(QStringLiteral("id"),"tag:sequences:" + QString::number(iIndex), entry);
                    AddTextNode(QStringLiteral("title"), lib.serials[iIndex].sName, entry);
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
                    QDomElement el = AddTextNode(QStringLiteral("a"), lib.serials[iIndex].sName, div);
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
            if(genres[idParrentGenre].idParrentGenre > 0){
                std::vector<uint> listBooks = book_list(lib, 0, 0, idParrentGenre, QLatin1String(""));
                ts << FillPage(listBooks, lib, tr("Books by ABC"), lib_url, url, ts, opds, nPage, session, true);
                return;
            }
            auto iGenre = genres.cbegin();
            while(iGenre != genres.cend()){
                if(iGenre->second.idParrentGenre == idParrentGenre)
                    listIdGenres << iGenre->first;
                ++iGenre;
            }
            for(const auto &book :lib.books){
                foreach (uint iGenre, book.second.vIdGenres){
                    if(genres[iGenre].idParrentGenre == idParrentGenre){
                        if(mCounts.contains(iGenre))
                            mCounts[iGenre]++;
                        else
                            mCounts[iGenre] = 1;
                    }
                }
            }
        }
        else
        {
            auto iGenre = genres.cbegin();
            while(iGenre != genres.cend()){
                if(iGenre->second.idParrentGenre == 0)
                    listIdGenres << iGenre->first;
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
                AddTextNode(QStringLiteral("id"), QLatin1String("tag:root:genre:") + genres[idGenre].sName, entry);
                AddTextNode(QStringLiteral("title"), genres[idGenre].sName, entry);
                if(strings.count() > 2)
                {
                    QDomElement el = AddTextNode(QStringLiteral("content"), QString::number(mCounts[idGenre]) + QLatin1String(" ") + tr("books"), entry);
                    el.setAttribute(QStringLiteral("type"), QStringLiteral("text"));
                }
                else
                {
                    QDomElement el = AddTextNode(QStringLiteral("content"), tr("Books of genre") + QLatin1String(" ") + genres[idGenre].sName, entry);
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
                QDomElement el = AddTextNode(QStringLiteral("A"), genres[idGenre].sName,div);
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
        QMap<QString, int> mCount;

        QSet <QString> setAuthors;
        int count = 0;
        for(const auto &iAuthor :lib.authors)
        {
            if(iAuthor.second.getName().left(index.length()).toLower() == index.toLower())
            {
                count++;
                QString sNewIndex = iAuthor.second.getName().left(index.length() + 1).toLower();
                sNewIndex[0] = sNewIndex[0].toUpper();
                if(mCount.contains(sNewIndex))
                    mCount[sNewIndex]++;
                else
                    mCount[sNewIndex] = 1;
                if(sNewIndex.length() == iAuthor.second.getName().length())
                    setAuthors.insert(sNewIndex);
            }
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
                        for(const auto &iAuthor :lib.authors)
                        {
                            if(setAuthors.contains(iIndex) ?iAuthor.second.getName().toLower() == iIndex.toLower()
                                    :iAuthor.second.getName().left(iIndex.size()).toLower() == iIndex.toLower())
                            {
                                el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/author/") + QString::fromLatin1(QUrl::toPercentEncoding(QString::number(iAuthor.first))) +
                                                (session.isEmpty() ?QString() :QLatin1String("?session=") + session));
                                break;
                            }
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
                        for(const auto &iAuthor :lib.authors)
                        {
                            if(setAuthors.contains(iIndex) ?iAuthor.second.getName().toLower() == iIndex.toLower()
                                    :iAuthor.second.getName().left(iIndex.size()).toLower() == iIndex.toLower())
                            {
                                el.setAttribute(QStringLiteral("href"), lib_url + QLatin1String("/author/") + QString::fromLatin1(QUrl::toPercentEncoding(QString::number(iAuthor.first))) +
                                                (session.isEmpty() && mCount[iIndex]<30 ?QString() :QLatin1String("?session=") + session));
                                break;
                            }
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
            for(const auto &iAuthor :lib.authors)
            {
                if(iAuthor.second.getName().left(index.length()).toLower() == index.toLower())
                {
                    listAuthorId << iAuthor.first;
                }
            }
            std::sort(listAuthorId.begin(),listAuthorId.end(),[&lib](uint lhs,uint rhs) {return lib.authors.at(lhs).getName() < lib.authors.at(rhs).getName();});
            foreach(uint iIndex, listAuthorId)
            {
                uint nBooksCount = 0;
                auto range = lib.authorBooksLink.equal_range(iIndex);
                for (auto it = range.first; it != range.second; ++it) {
                    nBooksCount++;
                }

                if(opds)
                {
                    QDomElement entry = doc.createElement(QStringLiteral("entry"));
                    feed.appendChild(entry);
                    AddTextNode(QStringLiteral("updated"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate), entry);
                    AddTextNode(QStringLiteral("id"), QLatin1String("tag:author:") + QString::number(iIndex), entry);
                    AddTextNode(QStringLiteral("title"), lib.authors[iIndex].getName(), entry);
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
                    QDomElement el = AddTextNode(QStringLiteral("a"),lib.authors[iIndex].getName(),div);
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

        std::vector<uint> listBooks = book_list(lib, idAuthor, 0, 0, QLatin1String(""), true);
        ts<<FillPage(listBooks, lib, tr("Books without sequence") + QLatin1String(" (") + lib.authors[idAuthor].getName() + QLatin1String(")"), lib_url, url, ts, opds, nPage, session, false);

    }
    else if(url.startsWith(QLatin1String("/authorbooks/"), Qt::CaseInsensitive) && strings.count() >= 3)
    {
        uint idAuthor = strings.at(2).toUInt();
        std::vector<uint> listBooks = book_list(lib,idAuthor, 0, 0, QLatin1String(""), false);
        ts << FillPage(listBooks,lib,tr("Books by ABC") + QLatin1String(" (") + lib.authors[idAuthor].getName() + QLatin1String(")"), lib_url, url,ts, opds, nPage, session, false);
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
            AddTextNode(QStringLiteral("title"), tr("Book sequences") + QLatin1String(" ") + lib.authors[idAuthor].getName(), feed);
            AddTextNode(QStringLiteral("updated"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate), feed);
            AddTextNode(QStringLiteral("icon"), QStringLiteral("/icon_256x256.png"), feed);
        }
        else
        {
             feed = doc_header(session, true, lib.name, lib_url);
             QDomElement div = AddTextNode(QStringLiteral("DIV"), tr("Book sequences") + QLatin1String(" ") + lib.authors[idAuthor].getName(), feed);
             div.setAttribute(QStringLiteral("class"), QStringLiteral("caption"));
        }

        auto range = lib.authorBooksLink.equal_range(idAuthor);
        QMap<uint, uint> mapCountBooks;
        for (auto it = range.first; it != range.second; ++it) {
            SBook& book = lib.books[it->second];
            if(book.idSerial > 0){
                if(mapCountBooks.contains(book.idSerial))
                    mapCountBooks[book.idSerial]++;
                else
                    mapCountBooks[book.idSerial] = 1;
            }
        }
        QList<uint> listSerials = mapCountBooks.keys();
        std::sort(listSerials.begin(), listSerials.end(), [&lib](uint lhs,uint rhs) {return QString::localeAwareCompare(lib.serials.at(lhs).sName, lib.serials.at(rhs).sName) < 0;});

        foreach(uint idSerial, listSerials)
        {
            if(opds)
            {
                QDomElement entry = doc.createElement(QStringLiteral("entry"));
                feed.appendChild(entry);
                AddTextNode(QStringLiteral("updated"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate), entry);
                AddTextNode(QStringLiteral("id"), QLatin1String("tag:author:") + id + QLatin1String(":sequence:") + QString::number(idSerial), entry);
                AddTextNode(QStringLiteral("title"), lib.serials[idSerial].sName, entry);
                QDomElement el=AddTextNode(QStringLiteral("content"), QString::number(mapCountBooks[idSerial]) + QLatin1String(" ") + tr("books in sequence"), entry);
                el.setAttribute(QStringLiteral("type"), QStringLiteral("text"));
                el = AddTextNode(QStringLiteral("link"), QLatin1String(""), entry);
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
                QDomElement el = AddTextNode(QStringLiteral("a"), lib.serials[idSerial].sName, entry);
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
        std::vector<uint> listBooks = book_list(lib,id_author.toUInt(), idSequence, 0, QLatin1String(""));
        ts << FillPage(listBooks, lib, tr("Books of sequence") + QLatin1String(" (") + lib.serials[idSequence].sName + QLatin1String(")"), lib_url, url, ts, opds, nPage, session, false);
    }
    else if(url.startsWith(QLatin1String("/author/"), Qt::CaseInsensitive) && strings.count() >= 3)
    {
        QString sIdAuthor = strings.at(2);
        uint idAuthor = sIdAuthor.toUInt();
        QString sAuthor = lib.authors[idAuthor].getName();
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
        const auto listTcpServsrs = httpServer_.servers();
        for(auto &server: listTcpServsrs) {
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

    QString sLibUrl;
    const SLib *pLib = getLib(idLib, u"/http"_s, &sLibUrl);
    if(pLib == nullptr)
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSesionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;
    QDomElement feed = docHeaderHTTP(sSesionQuery, pLib->name, sLibUrl);
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
    feed.appendChild(hr);
    attachSearchFormHTTP(feed, tr("Finding books by name/author: "), sLibUrl + u"/search"_s, u""_s, sSession);

    return responseHTTP();
}

QHttpServerResponse opds_server::rootOPDS(uint idLib, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QUrlQuery urlquery(url);
    QString sLibUrl;
    const SLib *pLib = getLib(idLib, u"/opds"_s, &sLibUrl);
    if(pLib == nullptr)
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSesionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    QDomElement feed = docHeaderOPDS(pLib->name, u"tag:root"_s, sLibUrl, sSesionQuery);

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

QByteArray opds_server::cover(uint idLib, uint idBook)
{
    QByteArray baResult;
    if(libs.contains(idLib) && libs.at(idLib).books.contains(idBook)){
        BookFile file(idLib, idBook);
        QImage img = file.cover();
        QBuffer buffer(&baResult);
        buffer.open(QIODevice::WriteOnly);
        img.save(&buffer, "jpg");
    }
    return baResult;
}

struct LocaleAwareQStringComparator {
    bool operator()(const QString& str1, const QString& str2) const {
        return str1.localeAwareCompare(str2) < 0;
    }
};

void opds_server::attachSearchFormHTTP(QDomElement &feed, const QString &sTitle, const QString &sAction, const QString &sSearch, const QString &sSession)
{
    QDomElement div = doc.createElement(u"div"_s);

    QDomElement form = doc.createElement(u"form"_s);
    form.setAttribute(u"method"_s, u"get"_s);
    form.setAttribute(u"action"_s, sAction);
    feed.appendChild(form);
    form.appendChild(div);

    QDomElement el;
    el = AddTextNode(u"div"_s, sTitle, div);
    el.setAttribute(u"class"_s, u"book"_s);
    div.appendChild(el);

    el = doc.createElement(u"input"_s);
    el.setAttribute(u"type"_s, u"search"_s);
    el.setAttribute(u"name"_s, u"search_string"_s);
    if(!sSearch.isEmpty())
        el.setAttribute(u"value"_s, sSearch);
    div.appendChild(el);

    if(!sSession.isEmpty()){
        el = doc.createElement(u"input"_s);
        el.setAttribute(u"type"_s, u"hidden"_s);
        el.setAttribute(u"name"_s, u"session"_s);
        el.setAttribute(u"value"_s, sSession);
        div.appendChild(el);
    }

    el = doc.createElement(u"input"_s);
    el.setAttribute(u"type"_s, u"submit"_s);
    el.setAttribute(u"value"_s, tr("Find"));
    div.appendChild(el);
}

QHttpServerResponse opds_server::authorsIndexHTTP(uint idLib, const QString &sIndex, bool bByBooks, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    const SLib *pLib = getLib(idLib, u"/http"_s, &sLibUrl);
    if(pLib == nullptr)
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSesionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    QDomElement feed = docHeaderHTTP(sSesionQuery, pLib->name, sLibUrl);

    std::map<QString, int, LocaleAwareQStringComparator> mCount;

    std::unordered_set<QString> setAuthors;
    std::unordered_set<uint>stIdAuthors;

    int count = 0;
    QString sLowerIndex = sIndex.toCaseFolded();
    for(const auto &iBook :pLib->books){
        if(!iBook.second.bDeleted){
            if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == pLib->vLaguages[iBook.second.idLanguage])
                for(auto idAuthor :iBook.second.vIdAuthors)
                    stIdAuthors.insert(idAuthor);
        }
    }
    for(auto idAuthor :stIdAuthors)
    {
        const auto &author  = pLib->authors.at(idAuthor);
        QString sAuthorName = author.getName();
        if(sAuthorName.left(sIndex.length()).toCaseFolded() == sLowerIndex)
        {
            count++;
            QString sNewIndex = sAuthorName.left(sIndex.length() + 1).toCaseFolded();

            sNewIndex[0] = sNewIndex[0].toUpper();
            ++mCount[sNewIndex];
            if(sNewIndex.length() == sAuthorName.length())
                setAuthors.insert(sNewIndex);
        }
    }

    if(count>30 && !bByBooks)
    {
        attachSearchFormHTTP(feed, tr("Search authors: "), sLibUrl + u"/searchauthor"_s, sIndex, sSession);

        QDomElement tag_table;
        QDomElement tag_tr;
        int nCurrentColumn = 0;
        tag_table = doc.createElement(u"TABLE"_s);
        feed.appendChild(tag_table);

        for(const auto &iIndex :mCount)
        {
            if(nCurrentColumn == 0)
            {
                tag_tr = doc.createElement(u"tr"_s);
                tag_table.appendChild(tag_tr);
            }
            QDomElement td = doc.createElement(u"td"_s);
            tag_tr.appendChild(td);
            QDomElement div = doc.createElement(u"div"_s);
            div.setAttribute(u"class"_s, u"item"_s);
            td.appendChild(div);
            QDomElement el = AddTextNode(u"a"_s, iIndex.first, div);
            el.setAttribute(u"class"_s, u"block"_s);
            AddTextNode(u"div"_s, QString::number(iIndex.second) + u" "_s + tr("authors beginning with") + u" '"_s +
                        iIndex.first + u"'"_s, div);
            if(iIndex.second == 1)
            {
                QString lowerIndex = iIndex.first.toCaseFolded();
                for(auto idAuthor :stIdAuthors)
                {
                    const auto &author = pLib->authors.at(idAuthor);
                    if(author.getName().left(iIndex.first.size()).toCaseFolded() == lowerIndex)
                    {
                        el.setAttribute(u"href"_s, sLibUrl + u"/author/"_s + QString::number(idAuthor) + sSesionQuery);
                        break;
                    }
                }
            }
            else
            {
                el.setAttribute(u"href"_s, sLibUrl + u"/authorsindex/"_s+ QString::fromUtf8(QUrl::toPercentEncoding(iIndex.first, ""_ba, "."_ba).constData()) +
                                (setAuthors.contains(iIndex.first) ?u"/books"_s :u""_s) + sSesionQuery);

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
        std::vector<uint> vAuthorId;
        for(auto idAuthor :stIdAuthors)
        {
            const auto &author = pLib->authors.at(idAuthor);
            if(author.getName().left(sIndex.length()).toCaseFolded() == sLowerIndex)
            {
                vAuthorId.push_back(idAuthor);
            }
        }
        std::sort(vAuthorId.begin(), vAuthorId.end(), [pLib](uint id1, uint id2)
        {
            return pLib->authors.at(id1).getName() < pLib->authors.at(id2).getName();
        });
        for(uint iIndex: vAuthorId)
        {
            uint nBooksCount = 0;
            auto range = pLib->authorBooksLink.equal_range(iIndex);
            for (auto it = range.first; it != range.second; ++it) {
                auto &book = pLib->books.at(it->second);
                if(!book.bDeleted)
                    if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == pLib->vLaguages[book.idLanguage])
                        nBooksCount++;
            }

            QDomElement div = doc.createElement(u"DIV"_s);
            div.setAttribute(u"class"_s, u"item"_s);
            feed.appendChild(div);
            QDomElement el = AddTextNode(u"a"_s, pLib->authors.at(iIndex).getName(), div);
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
    const SLib *pLib = getLib(idLib, u"/opds"_s, &sLibUrl);
    if(pLib == nullptr)
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSesionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    QDomElement feed = docHeaderOPDS(tr("Books by authors"), u"tag:root:authors"_s, sLibUrl, sSesionQuery);

    std::map<QString, int, LocaleAwareQStringComparator> mCount;

    std::unordered_set <QString> setAuthors;
    std::unordered_set<uint>stIdAuthors;
    int count = 0;
    QString sLowerIndex = sIndex.toCaseFolded();
    for(const auto &iBook :pLib->books){
        if(!iBook.second.bDeleted){
            if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == pLib->vLaguages[iBook.second.idLanguage])
                for(auto idAuthor :iBook.second.vIdAuthors)
                    stIdAuthors.insert(idAuthor);
        }
    }
    for(auto idAuthor :stIdAuthors)
    {
        const auto &author  = pLib->authors.at(idAuthor);
        QString sAuthorName = author.getName();
        if(sAuthorName.left(sIndex.length()).toCaseFolded() == sLowerIndex)
        {
            count++;
            QString sNewIndex = sAuthorName.left(sIndex.length() + 1).toCaseFolded();

            sNewIndex[0] = sNewIndex[0].toUpper();
            ++mCount[sNewIndex];
            if(sNewIndex.length() == sAuthorName.length())
                setAuthors.insert(sNewIndex);
        }
    }

    if(count>30 && !bByBooks)
    {
        QDomElement tag_table;
        QDomElement tag_tr;
//        int nCurrentColumn = 0;

        for(const auto &iIndex :mCount)
        {
            QDomElement entry = doc.createElement(u"entry"_s);
            feed.appendChild(entry);
            AddTextNode(u"updated"_s, QDateTime::currentDateTimeUtc().toString(Qt::ISODate), entry);
            AddTextNode(u"id"_s, u"tag:authors:"_s + iIndex.first, entry);
            AddTextNode(u"title"_s, iIndex.first, entry);
            QDomElement el = AddTextNode(u"content"_s, QString::number(iIndex.second) + u" "_s + tr("authors beginning with") +
                                                           u" '"_s + iIndex.first + u"'"_s, entry);
            el.setAttribute(u"type"_s, u"text"_s);
            el = AddTextNode(u"link"_s, u""_s, entry);
            if(iIndex.second == 1)
            {
                QString lowerIndex = iIndex.first.toCaseFolded();
                for(auto idAuthor :stIdAuthors)
                {
                    const auto &author = pLib->authors.at(idAuthor);
                    if(author.getName().left(iIndex.first.size()).toCaseFolded() == lowerIndex)
                    {
                        el.setAttribute(u"href"_s, sLibUrl + u"/author/"_s + QString::number(idAuthor) + sSesionQuery);
                        break;
                    }
                }
            }
            else
            {
                el.setAttribute(u"href"_s, sLibUrl + u"/authorsindex/"_s + QString::fromUtf8(QUrl::toPercentEncoding(iIndex.first, ""_ba, "."_ba).constData()) +
                                               (setAuthors.contains(iIndex.first) && iIndex.second<30 ?u"/books"_s :u""_s) + sSesionQuery);
            }
            el.setAttribute(u"type"_s, u"application/atom+xml;profile=opds-catalog"_s);
        }
    }
    else
    {
        std::vector<uint> vAuthorId;
        for(auto idAuthor :stIdAuthors)
        {
            const auto &author = pLib->authors.at(idAuthor);
            if(author.getName().left(sIndex.length()).toCaseFolded() == sLowerIndex)
            {
                vAuthorId.push_back(idAuthor);
            }
        }
        std::sort(vAuthorId.begin(), vAuthorId.end(), [pLib](uint id1, uint id2) {return pLib->authors.at(id1).getName() < pLib->authors.at(id2).getName();});
        for(uint iIndex: vAuthorId)
        {
            uint nBooksCount = 0;
            auto range = pLib->authorBooksLink.equal_range(iIndex);
            for (auto it = range.first; it != range.second; ++it) {
                auto &book = pLib->books.at(it->second);
                if(!book.bDeleted)
                    if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == pLib->vLaguages[book.idLanguage])
                        nBooksCount++;
            }

            QDomElement entry = doc.createElement(u"entry"_s);
            feed.appendChild(entry);
            AddTextNode(u"updated"_s, QDateTime::currentDateTimeUtc().toString(Qt::ISODate), entry);
            AddTextNode(u"id"_s, u"tag:author:"_s + QString::number(iIndex), entry);
            AddTextNode(u"title"_s, pLib->authors.at(iIndex).getName(), entry);
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
    const SLib *pLib = getLib(idLib, u"/http"_s, &sLibUrl);
    if(pLib == nullptr || !pLib->authors.contains(idAuthor))
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSesionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    QString sAuthor = pLib->authors.at(idAuthor).getName();
    QString sIdAuthor = QString::number(idAuthor);
    QDomElement feed = docHeaderHTTP(sSesionQuery, pLib->name, sLibUrl);
    QDomElement div_auth = doc.createElement(u"DIV"_s);;
    div_auth.setAttribute(u"class"_s, u"item"_s);
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
    const SLib *pLib = getLib(idLib, u"/opds"_s, &sLibUrl);
    if(pLib == nullptr || !pLib->authors.contains(idAuthor))
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSesionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    QString sAuthor = pLib->authors.at(idAuthor).getName();
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
    SLib *pLib = getLib(idLib, u"/http"_s, &sLibUrl);
    if(pLib == nullptr || !pLib->authors.contains(idAuthor))
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    std::vector<uint> vBooks = book_list(*pLib, idAuthor, 0, 0, u""_s, false);
    return FillPageHTTP(vBooks, *pLib, tr("Books by ABC") + u" ("_s + pLib->authors.at(idAuthor).getName() + u")"_s, sLibUrl, url, false);
}

QHttpServerResponse opds_server::authorBooksOPDS(uint idLib, uint idAuthor, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u"/opds"_s, &sLibUrl);
    if(pLib == nullptr || !pLib->authors.contains(idAuthor))
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    std::vector<uint> vBooks = book_list(*pLib, idAuthor, 0, 0, u""_s, false);
    QString sPage = FillPageOPDS(vBooks, *pLib, tr("Books by ABC") + u" ("_s + pLib->authors.at(idAuthor).getName() + u")"_s, u"id:autorbooks:"_s + QString::number(idAuthor), sLibUrl, url);

    QHttpServerResponse result(sPage);
    return result;
}

struct SerialComparator {
    const std::unordered_map<uint, SSerial>& mSerial;

    SerialComparator(const std::unordered_map<uint, SSerial>& a) : mSerial(a) {}

    bool operator()(uint id1, uint id2) const {
        return QString::localeAwareCompare(mSerial.at(id1).sName, mSerial.at(id2).sName) < 0;
    }
};

QHttpServerResponse opds_server::authorSequencesHTTP(uint idLib, uint idAuthor, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    const SLib *pLib = getLib(idLib, u"/http"_s, &sLibUrl);
    if(pLib == nullptr || !pLib->authors.contains(idAuthor))
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSesionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    QDomElement feed = docHeaderHTTP(sSesionQuery, pLib->name, sLibUrl);
    QDomElement div = AddTextNode(u"DIV"_s, tr("Book sequences") + u" "_s + pLib->authors.at(idAuthor).getName(), feed);
    div.setAttribute(u"class"_s, u"caption"_s);

    SerialComparator comporator(pLib->serials);
    std::map<uint, uint, SerialComparator> mCountBooks(comporator);
    auto range = pLib->authorBooksLink.equal_range(idAuthor);
    for (auto it = range.first; it != range.second; ++it) {
        const SBook& book = pLib->books.at(it->second);
        if(!book.bDeleted && book.idSerial > 0)
            if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == pLib->vLaguages[book.idLanguage])
                ++mCountBooks[book.idSerial];
    }

    QString sIdAuthor = QString::number(idAuthor);
    for(const auto &iSeria :mCountBooks)
    {
        QDomElement entry = doc.createElement(u"div"_s);
        entry.setAttribute(u"class"_s, u"item"_s);
        feed.appendChild(entry);
        QDomElement el = AddTextNode(u"a"_s, pLib->serials.at(iSeria.first).sName, entry);
        el.setAttribute(u"class"_s, u"block"_s);
        el.setAttribute(u"href"_s, sLibUrl % u"/authorsequence/"_s % sIdAuthor % u"/"_s % QString::number(iSeria.first) % sSesionQuery);
        AddTextNode(u"div"_s, QString::number(iSeria.second) % u" "_s % tr("books in sequence"), entry);
    }
    return responseHTTP();
}

QHttpServerResponse opds_server::authorSequencesOPDS(uint idLib, uint idAuthor, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    const SLib *pLib = getLib(idLib, u"/opds"_s, &sLibUrl);
    if(pLib == nullptr || !pLib->authors.contains(idAuthor))
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSesionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    QString sIdAuthor = QString::number(idAuthor);
    QString sCurrentDateTime = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    QDomElement feed = docHeaderOPDS(tr("Book sequences") + u" "_s + pLib->authors.at(idAuthor).getName(), u"tag:author:"_s + sIdAuthor, sLibUrl, sSesionQuery);

    SerialComparator comporator(pLib->serials);
    std::map<uint, uint, SerialComparator> mCountBooks(comporator);
    auto range = pLib->authorBooksLink.equal_range(idAuthor);
    for (auto it = range.first; it != range.second; ++it) {
        const SBook& book = pLib->books.at(it->second);
        if(!book.bDeleted && book.idSerial > 0){
            if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == pLib->vLaguages[book.idLanguage])
                ++mCountBooks[book.idSerial];
        }
    }

    for(const auto &iSeria :mCountBooks)
    {
        QString sIdSerial = QString::number(iSeria.first);
        QDomElement entry = doc.createElement(u"entry"_s);
        feed.appendChild(entry);
        AddTextNode(u"updated"_s, sCurrentDateTime, entry);
        AddTextNode(u"id"_s, u"tag:author:"_s % sIdAuthor % u":sequence:"_s % sIdSerial, entry);
        AddTextNode(u"title"_s, pLib->serials.at(iSeria.first).sName, entry);
        QDomElement el = AddTextNode(u"content"_s, QString::number(iSeria.first) % u" "_s % tr("books in sequence"), entry);
        el.setAttribute(u"type"_s, u"text"_s);
        el = AddTextNode(u"link"_s, u""_s, entry);
        el.setAttribute(u"href"_s, sLibUrl % u"/authorsequence/"_s % sIdAuthor + u"/"_s % sIdSerial % sSesionQuery);
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
    SLib *pLib = getLib(idLib, u"/http"_s, &sLibUrl);
    if(pLib == nullptr || !pLib->authors.contains(idAuthor) || !pLib->serials.contains(idSequence))
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    std::vector<uint> vBooks = book_list(*pLib, idAuthor, idSequence, 0, u""_s);
    return FillPageHTTP(vBooks, *pLib, tr("Books of sequence") + u" ("_s + pLib->serials[idSequence].sName + u")"_s, sLibUrl, url, false);
}

QHttpServerResponse opds_server::authorSequencesOPDS(uint idLib, uint idAuthor, uint idSequence, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u"/opds"_s, &sLibUrl);
    if(pLib == nullptr || !pLib->authors.contains(idAuthor) || !pLib->serials.contains(idSequence))
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    std::vector<uint> vBooks = book_list(*pLib, idAuthor, idSequence, 0, u""_s);
    QString sPage = FillPageOPDS(vBooks, *pLib, tr("Books of sequence") % u" ("_s % pLib->serials[idSequence].sName % u")"_s,
                                 u"tag:author:"_s % QString::number(idAuthor) % u":sequence:"_s % QString::number(idSequence), sLibUrl, url);
    QHttpServerResponse result(sPage);
    return result;
}

QHttpServerResponse opds_server::authorSequencelessHTTP(uint idLib, uint idAuthor, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u"/http"_s, &sLibUrl);
    if(pLib == nullptr || !pLib->authors.contains(idAuthor))
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    std::vector<uint> vBooks = book_list(*pLib, idAuthor, 0, 0, u""_s, true);
    return FillPageHTTP(vBooks, *pLib, tr("Books without sequence") % u" ("_s % pLib->authors[idAuthor].getName() % u")"_s, sLibUrl, url, false);
}

QHttpServerResponse opds_server::authorSequencelessOPDS(uint idLib, uint idAuthor, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u"/opds"_s, &sLibUrl);
    if(pLib == nullptr || !pLib->authors.contains(idAuthor))
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    std::vector<uint> vBooks = book_list(*pLib, idAuthor, 0, 0, u""_s, true);
    QString sPage = FillPageOPDS(vBooks, *pLib, tr("Books without sequence") % u" ("_s % pLib->authors[idAuthor].getName() % u")"_s,
                                 u"tag:author:"_s % QString::number(idAuthor) % u":sequenceless"_s, sLibUrl, url);
    QHttpServerResponse result(sPage);
    return result;
}

QHttpServerResponse opds_server::sequencesIndexHTTP(uint idLib, const QString &sIndex, bool bByBooks, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    const SLib *pLib = getLib(idLib, u"/http"_s, &sLibUrl);
    if(pLib == nullptr)
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSesionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    QDomElement feed;
    feed = docHeaderHTTP(sSesionQuery, pLib->name, sLibUrl);
    std::map<QString, int, LocaleAwareQStringComparator> mCount;

    std::unordered_set<QString> stSerials;
    std::unordered_set<uint>stIdSerials;
    int count = 0;
    QString sLowerIndex = sIndex.toCaseFolded();
    for(const auto &iBook :pLib->books){
        if(iBook.second.idSerial != 0 && !iBook.second.bDeleted){
            if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == pLib->vLaguages[iBook.second.idLanguage])
                stIdSerials.insert(iBook.second.idSerial);
        }
    }
    for(auto &idSerial :stIdSerials){
        const auto &serial = pLib->serials.at(idSerial);
        if(serial.sName.left(sIndex.length()).toCaseFolded() == sLowerIndex){
            count++;
            QString sNewIndex = serial.sName.left(sIndex.length()+1).toCaseFolded();
            sNewIndex[0] = sNewIndex[0].toUpper();
            ++mCount[sNewIndex];
            if(sNewIndex.length() == serial.sName.length())
                stSerials.insert(sNewIndex);
        }
    }

    if(count > 30 && !bByBooks)
    {
        QDomElement tag_table;
        QDomElement tag_tr;
        int current_column = 0;
        tag_table = doc.createElement(u"TABLE"_s);
        feed.appendChild(tag_table);
        for(const auto &iIndex :mCount)
        {
            if(iIndex.first.trimmed().isEmpty() || iIndex.first[0] == '\0')
                continue;

            if(current_column == 0)
            {
                tag_tr = doc.createElement(u"tr"_s);
                tag_table.appendChild(tag_tr);
            }
            QDomElement td = doc.createElement(u"td"_s);
            tag_tr.appendChild(td);

            QDomElement div = doc.createElement(u"DIV"_s);
            div.setAttribute(u"class"_s, u"item"_s);
            td.appendChild(div);
            QDomElement el = AddTextNode(u"a"_s, iIndex.first, div);
            el.setAttribute(u"class"_s, u"block"_s);
            AddTextNode(u"div"_s, QString::number(iIndex.second) + u" "_s + tr("series beginning with") + u" '"_s +
                        iIndex.first + u"'"_s, div);
            if(iIndex.second == 1)
            {
                for(auto &idSerial :stIdSerials){
                    const auto &serial = pLib->serials.at(idSerial);
                    if(serial.sName.left(iIndex.first.size()).toCaseFolded() == iIndex.first.toCaseFolded())
                    {
                        el.setAttribute(u"href"_s, sLibUrl % u"/sequencebooks/"_s % QString::number(idSerial) % sSesionQuery);
                        break;
                    }
                }
            }
            else
            {
                el.setAttribute(u"href"_s, sLibUrl % u"/sequencesindex/"_s % QString::fromUtf8(QUrl::toPercentEncoding(iIndex.first, ""_ba, "."_ba).constData()) %
                                (stSerials.contains(iIndex.first) && iIndex.second<30 ?u"/books"_s :u""_s) % sSesionQuery );

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
        std::vector<uint> vSerialId;
        for(auto &idSerial :stIdSerials)
        {
            const auto &serial = pLib->serials.at(idSerial);
            if(serial.sName.left((sIndex.length())).toCaseFolded() == sLowerIndex)
            {
                vSerialId.push_back(idSerial);
            }
        }
        std::sort(vSerialId.begin(), vSerialId.end(),[pLib](uint id1, uint id2) {return pLib->serials.at(id1).sName < pLib->serials.at(id2).sName;});

        for(auto idSerial: vSerialId)
        {
            uint nBooksCount = 0;
            for(const auto &iBook :pLib->books)
            {
                if(!iBook.second.bDeleted && iBook.second.idSerial == idSerial)
                    if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == pLib->vLaguages[iBook.second.idLanguage])
                        nBooksCount++;
            }
            QDomElement div = doc.createElement(u"DIV"_s);
            div.setAttribute(u"class"_s, u"item"_s);
            feed.appendChild(div);
            QDomElement el = AddTextNode(u"a"_s, pLib->serials.at(idSerial).sName, div);
            el.setAttribute(u"class"_s, u"block"_s);
            el.setAttribute(u"href"_s ,sLibUrl + u"/sequencebooks/"_s + QString::number(idSerial) + sSesionQuery );
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
    const SLib *pLib = getLib(idLib, u"/opds"_s, &sLibUrl);
    if(pLib == nullptr)
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSesionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    QDomElement feed = docHeaderOPDS(tr("Books by sequences"), u"tag:root:sequences"_s, sLibUrl, sSesionQuery);

    std::map<QString, int, LocaleAwareQStringComparator> mCount;
    std::unordered_set<QString> stSerials;
    std::unordered_set<uint>stIdSerials;
    int count = 0;
    QString sLowerIndex = sIndex.toCaseFolded();

    for(const auto &iBook :pLib->books){
        if(iBook.second.idSerial != 0 && !iBook.second.bDeleted){
            if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == pLib->vLaguages[iBook.second.idLanguage])
                stIdSerials.insert(iBook.second.idSerial);
        }
    }
    for(auto &idSerial :stIdSerials){
        const auto &serial = pLib->serials.at(idSerial);
        if(serial.sName.left(sIndex.length()).toCaseFolded() == sLowerIndex){
            count++;
            QString sNewIndex = serial.sName.left(sIndex.length()+1).toCaseFolded();
            sNewIndex[0] = sNewIndex[0].toUpper();
            ++mCount[sNewIndex];
            if(sNewIndex.length() == serial.sName.length())
                stSerials.insert(sNewIndex);
        }
    }

    if(count > 30 && !bByBooks)
    {
        QDomElement tag_table;
        QDomElement tag_tr;
        for(const auto &iIndex :mCount)
        {
            if(iIndex.first.trimmed().isEmpty() || iIndex.first[0] == '\0')
                continue;

            QDomElement entry = doc.createElement(u"entry"_s);
            feed.appendChild(entry);
            AddTextNode(u"updated"_s, QDateTime::currentDateTimeUtc().toString(Qt::ISODate), entry);
            AddTextNode(u"id"_s, u"tag:sequences:"_s + iIndex.first, entry);
            AddTextNode(u"title"_s, iIndex.first, entry);
            QDomElement el = AddTextNode(u"content"_s, QString::number(iIndex.second) + u" "_s + tr("series beginning with") +
                                         u" '"_s + iIndex.first + u"'"_s, entry);
            el.setAttribute(u"type"_s, u"text"_s);
            el = AddTextNode(u"link"_s, u""_s, entry);
            if(iIndex.second == 1)
            {
                for(auto &idSerial :stIdSerials){
                    const auto &serial = pLib->serials.at(idSerial);
                    if(stSerials.contains(iIndex.first) ?serial.sName.toCaseFolded() == iIndex.first.toCaseFolded()
                            :serial.sName.left(iIndex.first.size()).toCaseFolded() == iIndex.first.toCaseFolded())
                    {
                        el.setAttribute(u"href"_s, sLibUrl + u"/sequencebooks/"_s + QString::number(idSerial) + sSesionQuery);
                        break;
                    }
                }
            }
            else
            {
                el.setAttribute(u"href"_s, sLibUrl + u"/sequencesindex/"_s +  QString::fromUtf8(QUrl::toPercentEncoding(iIndex.first, ""_ba, "."_ba).constData())
                                + (stSerials.contains(iIndex.first) ?u"/books"_s :u""_s) + sSesionQuery);
            }
            el.setAttribute(u"type"_s, u"application/atom+xml;profile=opds-catalog"_s);
        }
    }
    else
    {
        std::vector<uint> vSerialId;
        for(auto &idSerial :stIdSerials)
        {
            const auto &serial = pLib->serials.at(idSerial);
            if(serial.sName.left((sIndex.length())).toCaseFolded() == sLowerIndex)
            {
                vSerialId.push_back(idSerial);
            }
        }
        std::sort(vSerialId.begin(), vSerialId.end(), [pLib](uint id1, uint id2) {return pLib->serials.at(id1).sName < pLib->serials.at(id2).sName;});

        for(auto iIndex: vSerialId)
        {
            uint nBooksCount = 0;
            for(const auto &iBook :pLib->books)
            {
                if(!iBook.second.bDeleted && iBook.second.idSerial == iIndex)
                    if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == pLib->vLaguages[iBook.second.idLanguage])
                        nBooksCount++;
            }
            QDomElement entry = doc.createElement(u"entry"_s);
            feed.appendChild(entry);
            AddTextNode(u"updated"_s, QDateTime::currentDateTimeUtc().toString(Qt::ISODate), entry);
            AddTextNode(u"id"_s, u"tag:sequences:"_s + QString::number(iIndex), entry);
            AddTextNode(u"title"_s, pLib->serials.at(iIndex).sName, entry);
            QDomElement el = AddTextNode(u"content"_s, QString::number(nBooksCount) + u" "_s + tr("books"), entry);
            el.setAttribute(u"type"_s, u"text"_s);
            el = AddTextNode(u"link"_s, u""_s, entry);
            el.setAttribute(u"href"_s, sLibUrl % u"/sequencebooks/"_s % QString::number(iIndex) % sSesionQuery);
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
    SLib *pLib = getLib(idLib, u"/http"_s, &sLibUrl);
    if(pLib == nullptr || !pLib->serials.contains(idSequence))
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    std::vector<uint> vBooks = book_list(*pLib, 0, idSequence, 0, u""_s);
    return FillPageHTTP(vBooks, *pLib, tr("Books of sequence") % u" ("_s % pLib->serials[idSequence].sName % u")"_s, sLibUrl, url, false);
}

QHttpServerResponse opds_server::sequenceBooksOPDS(uint idLib, uint idSequence, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u"/opds"_s, &sLibUrl);
    if(pLib == nullptr || !pLib->serials.contains(idSequence))
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    std::vector<uint> vBooks = book_list(*pLib, 0, idSequence, 0, u""_s);
    QString sPage = FillPageOPDS(vBooks, *pLib, tr("Books of sequence") % u" ("_s % pLib->serials[idSequence].sName % u")"_s, u"tag:sequences:"_s + QString::number(idSequence), sLibUrl, url);
    QHttpServerResponse result(sPage);
    return result;
}

QHttpServerResponse opds_server::bookHTTP(uint idLib, uint idBook, const QString &sFormat)
{
    return convert(idLib, idBook, sFormat, false);
}

QHttpServerResponse opds_server::bookOPDS(uint idLib, uint idBook, const QString &sFormat)
{
    return convert(idLib, idBook, sFormat, true);
}

QHttpServerResponse opds_server::genresHTTP(uint idLib, ushort idParentGenre, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u"/http"_s, &sLibUrl);
    if(pLib == nullptr || (idParentGenre!=0 && !genres.contains(idParentGenre)))
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSesionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    Q_ASSERT(!pLib->serials.contains(0));
    std::vector<ushort> vIdGenres;
    std::unordered_map<ushort, uint> mCounts;
    if(idParentGenre != 0)
    {
        if(genres[idParentGenre].idParrentGenre > 0){
            std::vector<uint> vBooks = book_list(*pLib, 0, 0, idParentGenre, u""_s);
            return FillPageHTTP(vBooks, *pLib, tr("Books by ABC"), sLibUrl, url, true);
        }
        for(const auto &iGenre :genres){
            if(iGenre.second.idParrentGenre == idParentGenre)
                vIdGenres.push_back(iGenre.first);
        }
        for(const auto &iBook :pLib->books){
            if(!iBook.second.bDeleted){
                for(auto iGenre: iBook.second.vIdGenres){
                    if(genres[iGenre].idParrentGenre == idParentGenre){
                        if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == pLib->vLaguages[iBook.second.idLanguage])
                            ++mCounts[iGenre];
                    }
                }
            }
        }
    }
    else
    {
        for(const auto &iGenre :genres){
            if(iGenre.second.idParrentGenre == 0)
                vIdGenres.push_back(iGenre.first);
        }
    }
    std::sort(vIdGenres.begin(), vIdGenres.end(), [&](ushort id1, ushort id2){return genres.at(id1).sName < genres.at(id2).sName;});
    QDomElement feed;
    feed = docHeaderHTTP(sSesionQuery, pLib->name, sLibUrl);

    for(auto idGenre: vIdGenres)
    {
        uint nCount = mCounts[idGenre];
        if(nCount == 0 && idParentGenre != 0)
            continue;
        QDomElement div = doc.createElement(u"DIV"_s);
        feed.appendChild(div);
        div.setAttribute(u"class"_s, u"item"_s);
        QDomElement el = AddTextNode(u"A"_s, genres[idGenre].sName, div);
        el.setAttribute(u"class"_s, u"block"_s);
        el.setAttribute(u"href"_s, sLibUrl % u"/genres/"_s % QString::number(idGenre) % sSesionQuery );
        if(idParentGenre != 0)
        {
            QDomElement el = AddTextNode(u"div"_s, QString::number(nCount) + u" "_s + tr("books"), div);
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
    SLib *pLib = getLib(idLib, u"/opds"_s, &sLibUrl);
    if(pLib == nullptr || (idParentGenre!=0 && !genres.contains(idParentGenre)))
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSesionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;


    std::vector<ushort> vIdGenres;
    QMap<ushort, uint> mCounts;
    if(idParentGenre != 0)
    {
        if(genres[idParentGenre].idParrentGenre > 0){
            std::vector<uint> vBooks = book_list(*pLib, 0, 0, idParentGenre, u""_s);
            QString sPage =  FillPageOPDS(vBooks, *pLib, tr("Books by ABC"), u""_s, sLibUrl, url);
            QHttpServerResponse result(sPage);
            return result;
        }
        for(const auto &iGenre :genres){
            if(iGenre.second.idParrentGenre == idParentGenre)
                vIdGenres.push_back(iGenre.first);
        }
        for(const auto &iBook :pLib->books){
            if(!iBook.second.bDeleted){
                for(auto iGenre: iBook.second.vIdGenres){
                    if(genres[iGenre].idParrentGenre == idParentGenre){
                        if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == pLib->vLaguages[iBook.second.idLanguage])
                            ++mCounts[iGenre];
                    }
                }
            }
        }
    }
    else
    {
        for(const auto &iGenre :genres){
            if(iGenre.second.idParrentGenre == 0)
                vIdGenres.push_back(iGenre.first);
        }
    }

    std::sort(vIdGenres.begin(), vIdGenres.end(), [&](ushort id1, ushort id2){return genres.at(id1).sName < genres.at(id2).sName;});
    QString sCurrentDateTime = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    QDomElement feed;
    feed = docHeaderOPDS(tr("Books by genre"), u"tag:root:genre"_s, sLibUrl, sSesionQuery);

    for(auto idGenre: vIdGenres)
    {
        uint nCount = mCounts[idGenre];
        if(nCount == 0 && idParentGenre != 0)
            continue;
        QDomElement entry = doc.createElement(u"entry"_s);
        feed.appendChild(entry);
        AddTextNode(u"updated"_s, sCurrentDateTime, entry);
        AddTextNode(u"id"_s, u"tag:root:genre:"_s + genres[idGenre].sName, entry);
        AddTextNode(u"title"_s, genres[idGenre].sName, entry);
        if(idParentGenre != 0)
        {
            QDomElement el = AddTextNode(u"content"_s, QString::number(mCounts[idGenre]) + u" "_s + tr("books"), entry);
            el.setAttribute(u"type"_s, u"text"_s);
        }
        else
        {
            QDomElement el = AddTextNode(u"content"_s, tr("Books of genre") + u" "_s + genres[idGenre].sName, entry);
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
    SLib *pLib = getLib(idLib, u"/http"_s, &sLibUrl);
    if(pLib == nullptr)
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sSearchString = urlquery.queryItemValue(u"search_string"_s);

    std::vector<uint> vBooks = book_list(*pLib, 0, 0, 0, sSearchString.replace(u'+', u' '));
    return FillPageHTTP(vBooks, *pLib, tr("Books search"), sLibUrl, url, true);
}

std::vector<uint> opds_server::searchAuthors(const SLib &lib, const QStringView sSearch)
{
    std::vector<uint> vResult;
    auto sListSearch = sSearch.split(u"+"_s, Qt::SkipEmptyParts);
    if(sListSearch.isEmpty())
        return vResult;
    for(const auto &iAuthor :lib.authors){
        bool bMatch = true;
        bool bMatchF = false;
        bool bMatchM = false;
        bool bMatchL = false;
        for(const auto &sSubSearch :sListSearch){
            bool bMatchAny = false;
            if(!bMatchF && iAuthor.second.sFirstName.startsWith(sSubSearch, Qt::CaseInsensitive)){
                bMatchF = true;
                bMatchAny = true;
            }
            if(!bMatchAny && !bMatchM && iAuthor.second.sMiddleName.startsWith(sSubSearch, Qt::CaseInsensitive)){
                bMatchM = true;
                bMatchAny = true;
            }
            if(!bMatchAny && !bMatchL && iAuthor.second.sLastName.startsWith(sSubSearch, Qt::CaseInsensitive)){
                bMatchL = true;
                bMatchAny = true;
            }
            if(!bMatchAny){
                bMatch = false;
                break;
            }
        }
        if(bMatch)
            vResult.push_back(iAuthor.first);
    }
    std::sort(vResult.begin(), vResult.end(), [&lib](uint id1, uint id2)
              { return QString::localeAwareCompare(lib.authors.at(id1).getName(), lib.authors.at(id2).getName()) < 0; });
    return vResult;
}

QHttpServerResponse opds_server::searchAuthorHTTP(uint idLib, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u"/http"_s, &sLibUrl);
    if(pLib == nullptr)
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sSearchString = urlquery.queryItemValue(u"search_string"_s);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSesionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    auto vAuthors = searchAuthors(*pLib, sSearchString);
    QDomElement feed;
    feed = docHeaderHTTP(sSesionQuery, pLib->name, sLibUrl);

    attachSearchFormHTTP(feed, tr("Search authors: "), sLibUrl + u"/searchauthor"_s, sSearchString, sSession);

    for(auto idAuthor :vAuthors){
        uint nBooksCount = 0;
        auto range = pLib->authorBooksLink.equal_range(idAuthor);
        for (auto it = range.first; it != range.second; ++it) {
            auto &book = pLib->books.at(it->second);
            if(!book.bDeleted)
                if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == pLib->vLaguages[book.idLanguage])
                    nBooksCount++;
        }

        QDomElement div = doc.createElement(u"DIV"_s);
        div.setAttribute(u"class"_s, u"item"_s);
        feed.appendChild(div);
        QDomElement el = AddTextNode(u"a"_s, pLib->authors.at(idAuthor).getName(), div);
        el.setAttribute(u"class"_s, u"block"_s);
        el.setAttribute(u"href"_s, sLibUrl + u"/author/"_s + QString::number(idAuthor) + sSesionQuery);
        AddTextNode(u"div"_s, QString::number(nBooksCount) + u" "_s + tr("books"), div);
    }

    return responseHTTP();
}

QHttpServerResponse opds_server::searchOPDS(uint idLib, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QUrlQuery urlquery(url);
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u"/opds"_s, &sLibUrl);
    if(pLib == nullptr)
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QString sSearchString = urlquery.queryItemValue(u"search_string"_s);
    std::vector<uint> vBooks = book_list(*pLib, 0, 0, 0, sSearchString.replace(u'+', u' '));
    return FillPageOPDS(vBooks, *pLib, tr("Books search"), u""_s, sLibUrl, url);
}

QHttpServerResponse opds_server::convert(uint idLib, uint idBook, const QString &sFormat, bool opds)
{
    QByteArray baContentType;
    QString sContentDisposition;
    SLib *pLib = getLib(idLib, u"/http"_s);
    if(pLib == nullptr || !pLib->books.contains(idBook))
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
    QByteArray baBook;
    BookFile bookFile(pLib, idBook);
    auto &book = pLib->books[idBook];
    baBook = bookFile.data();

    ExportOptions *pExportOptions = nullptr;
    int count = options.vExportOptions.size();
    for(int i=0; i<count; i++)
    {
        if((options.vExportOptions[i].sOutputFormat.toLower() == sFormat) || (sFormat == u"fb2"_s && options.vExportOptions[i].sOutputFormat == u"-"_s && pLib->books[idBook].sFormat == u"fb2"_s))
        {
            pExportOptions = &options.vExportOptions[i];
            break;
        }
    }

    if(baBook.size() != 0)
    {
        QString sBookFileName;
        if(pExportOptions != nullptr)
            sBookFileName = pExportOptions->sExportFileName;
        if(sBookFileName.isEmpty())
            sBookFileName = QString(ExportOptions::sDefaultEexpFileName);
        sBookFileName = pLib->fillParams(sBookFileName, idBook) + u"."_s + (sFormat == u"download"_s ? book.sFormat :sFormat);
        if(pExportOptions != nullptr && pExportOptions->bTransliteration)
            sBookFileName = Transliteration(sBookFileName);
        sBookFileName.replace(u' ', u'_');
        sBookFileName.replace(u'\"', u'_');
        sBookFileName.replace(u'\'', u'_');
        sBookFileName.replace(u',', u'_');
        sBookFileName.replace(u"__"_s, u"_"_s);
        //book_file_name.replace("\\","_");
        //book_file_name.replace("/","_");
        QFileInfo book_file(sBookFileName);
        sBookFileName = book_file.fileName();
        if(pExportOptions != nullptr && pExportOptions->bOriginalFileName)
            sBookFileName = book.sFile + u"."_s + sFormat;;
        if(sFormat == u"epub"_s || sFormat == u"mobi"_s || sFormat == u"azw3"_s)
        {
            QFile file;
            file.setFileName(QDir::tempPath() + u"/freeLib/book0."_s + book.sFormat);
            file.open(QFile::WriteOnly);
            file.write(baBook);
            file.close();
            QFileInfo fi(file);

            fb2mobi conv(pExportOptions, idLib);
            QString sOutFile = conv.convert(QStringList() << fi.absoluteFilePath(), idBook);
            file.setFileName(sOutFile);
            file.open(QFile::ReadOnly);
            baBook = file.readAll();
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
    QHttpServerResponse result(baBook);
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

void opds_server::setLanguageFilter(const QString &sLanguage)
{
    sLanguageFilter_ = sLanguage.toLower();
    if(sLanguageFilter_ == u"*")
        sLanguageFilter_ = u""_s;
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
    bMobile_ = false;
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
            bMobile_ = (tokens.contains(QStringLiteral("Mobile"), Qt::CaseInsensitive));
//        if(tokens[0] == "Cookie:")
//            bMobile_=(tokens.contains("PWDCHECK=1",Qt::CaseInsensitive));
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
                    auto hashPassword = passwordToHash(auth_str.at(1), options.baOpdsPasswordSalt);
                    auth = (options.sOpdsUser == auth_str.at(0) && options.baOpdsPasswordHash == hashPassword);
                }
            }
            if(TCPtokens.at(1).contains(QLatin1String("session=")))
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
                    sessions.insert({session, QDateTime::currentDateTime()});
                }
            }
            auto timeExpire = QDateTime::currentDateTime().addSecs(-60*60);
            erase_if(sessions, [&timeExpire](const auto &it) { return it.second < timeExpire; });
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
    SLib &lib = libs[idLib];
    QByteArray baBook;
    if(idBook == 0)
    {
        fi_book.setFile(file_name);
        QFile file(file_name);
        file.open(QFile::ReadOnly);
        outbuff.setData(file.readAll());
    }
    else
    {
        BookFile bookFile(&lib, idBook);
        auto &book = lib.books[idBook];
        baBook = bookFile.data();

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
