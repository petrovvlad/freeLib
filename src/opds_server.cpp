#include "opds_server.h"

#include <algorithm>
#include <ranges>
#include <unordered_set>
#include <QJsonArray>
#include <QJsonObject>
#include <QSettings>
#include <QNetworkProxy>
#include <QStringBuilder>
#include <QDir>
#include <QtConcurrent/QtConcurrentMap>
#include <QRegularExpression>
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
#include <QHttpHeaders>
#endif

#include "config-freelib.h"
#include "fb2mobi/fb2mobi.h"
#include "utilites.h"
#include "bookfile.h"

QString mime(QStringView sFormat)
{
    if(sFormat == u"djv" || sFormat == u"djvu")
        return u"image/vnd-djvu"_s;
    if(sFormat == u"doc")
        return u"application/msword"_s;
    if(sFormat == u"rar")
        return u"application/vnd.rar"_s;
    if(sFormat == u"txt")
        return u"text/plain"_s;
    return u"application/"_s.append(sFormat);
}

struct SeriesComparator {
    const std::unordered_map<uint, SSeries>& mSeries;

    SeriesComparator(const std::unordered_map<uint, SSeries>& a) : mSeries(a) {}

    bool operator()(uint id1, uint id2) const {
        return localeStringCompare(mSeries.at(id1).sName, mSeries.at(id2).sName);
    }
};

void setQueryItem(QUrlQuery &query, const QString &key, const QString &value) {
    if (query.hasQueryItem(key)) {
        query.removeQueryItem(key);
    }
    if(!value.isEmpty())
        query.addQueryItem(key, value);
}

opds_server::opds_server(QObject *parent) :
    QObject(parent)
{
    httpServer_.route(u"/"_s, QHttpServerRequest::Method::Get, [this](const QHttpServerRequest &request)
    {
        if(g::libs.size() == 1){
            auto idLib = g::libs.begin()->first;
            return rootHTML(idLib, request);
        }else
            return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
    });

    httpServer_.route(u"/<arg>"_s, QHttpServerRequest::Method::Get, [this](uint idLib, const QHttpServerRequest &request)
    {
        return rootHTML(idLib, request);
    });

    httpServer_.route(u"/opds/<arg>"_s, QHttpServerRequest::Method::Get, [this](uint idLib, const QHttpServerRequest &request)
    { return rootOPDS(idLib, request); });

    httpServer_.route(u"/opds>"_s, QHttpServerRequest::Method::Get, [this](uint idLib, const QHttpServerRequest &request)
    {
        if(g::libs.size() == 1){
            auto idLib = g::libs.begin()->first;
            return rootOPDS(idLib, request);
        }else
            return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
    });


    httpServer_.route(u"/opds2/<arg>"_s, QHttpServerRequest::Method::Get, [this](uint idLib, const QHttpServerRequest &request)
                      { return rootOPDS2(idLib, request); });

    httpServer_.route(u"/opds2"_s, QHttpServerRequest::Method::Get, [this](uint idLib, const QHttpServerRequest &request)
    {
        if(g::libs.size() == 1){
            auto idLib = g::libs.begin()->first;
            return rootOPDS2(idLib, request);
        }else
            return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
    });


    httpServer_.route(u"/assets/<arg>"_s, QHttpServerRequest::Method::Get, [](const QString &sFile)
    {
        QStringView sFormat;
        auto lastDotIndex = sFile.lastIndexOf(u'.');
        if (lastDotIndex != -1 && lastDotIndex < sFile.size() - 1)
            sFormat = sFile.mid(lastDotIndex + 1);

        QByteArray contentType;
        if(sFormat == u"css")
            contentType =  "text/css"_ba;
        else if(sFormat == u"png")
            contentType = "image/png"_ba;
        if(sFormat == u"svg")
            contentType = "image/svg+xml"_ba;
        if(sFormat == u"js")
            contentType = "application/javascript"_ba;

        if(contentType.isEmpty())
            return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
        QString sAssetsFile = u":/xsl/opds/"_s + sFile;
        QFile file(sAssetsFile);
        QByteArray ba;
        if(file.open(QFile::ReadOnly))
            ba = file.readAll();
        if(ba.isEmpty())
            return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
        QHttpServerResponse response(contentType, ba);
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
        QHttpHeaders headers = response.headers();;
        headers.append("Cache-Control"_ba,"max-age=3600"_ba);
        response.setHeaders(std::move(headers));
#else
        response.addHeader("Cache-Control"_ba,"max-age=3600"_ba);
#endif
        return response;
    });

    httpServer_.route(u"/<arg>/covers/<arg>/cover.jpg"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idBook/*, const QHttpServerRequest &request*/)
    {
        QByteArray ba = cover(idLib, idBook);
        if(ba.isEmpty()) [[unlikely]]
            return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
        QHttpServerResponse response("image/jpeg"_ba, ba);
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
        QHttpHeaders headers = response.headers();
        headers.append("Cache-Control"_ba,"max-age=3600"_ba);
        response.setHeaders(std::move(headers));
#else
        response.addHeader("Cache-Control"_ba,"max-age=3600"_ba);
#endif
        return response;
    });

    httpServer_.route(u"/opds/<arg>/covers/<arg>/cover.jpg"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idBook/*, const QHttpServerRequest &request*/)
    {
        QByteArray ba = cover(idLib, idBook);
        if(ba.isEmpty()) [[unlikely]]
            return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
        return QHttpServerResponse("image/jpeg"_ba, ba);
    });

    httpServer_.route(u"/opds2/<arg>/covers/<arg>/cover.jpg"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idBook/*, const QHttpServerRequest &request*/)
    {
        QByteArray ba = cover(idLib, idBook);
        if(ba.isEmpty()) [[unlikely]]
            return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
        return QHttpServerResponse("image/jpeg"_ba, ba);
    });

    httpServer_.route(u"/<arg>/authorsindex"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QHttpServerRequest &request)
    {
        return authorsIndexHTML(idLib, u""_s, request);
    });

    httpServer_.route(u"/opds/<arg>/authorsindex"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QHttpServerRequest &request)
    { return authorsIndexOPDS(idLib, u""_s, request); });

    httpServer_.route(u"/opds2/<arg>/authorsindex"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QHttpServerRequest &request)
                      { return authorsIndexOPDS2(idLib, u""_s, request); });

    httpServer_.route(u"/<arg>/authorsindex/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return authorsIndexHTML(idLib, sIndex, request); });

    httpServer_.route(u"/opds/<arg>/authorsindex/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return authorsIndexOPDS(idLib, sIndex, request); });

    httpServer_.route(u"/opds2/<arg>/authorsindex/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
                      { return authorsIndexOPDS2(idLib, sIndex, request); });

    httpServer_.route(u"/<arg>/authorsindex/<arg>/books"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return authorsIndexHTML(idLib, sIndex, request); });

    httpServer_.route(u"/opds/<arg>/authorsindex/<arg>/books"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return authorsIndexOPDS(idLib, sIndex, request); });

    httpServer_.route(u"/opds2/<arg>/authorsindex/<arg>/books"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return authorsIndexOPDS2(idLib, sIndex, request); });

    httpServer_.route(u"/<arg>/author/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idAuthor, const QHttpServerRequest &request)
    { return authorHTML(idLib, idAuthor, request); });

    httpServer_.route(u"/opds/<arg>/author/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idAuthor, const QHttpServerRequest &request)
    { return authorOPDS(idLib, idAuthor, request); });

    httpServer_.route(u"/opds2/<arg>/author/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idAuthor, const QHttpServerRequest &request)
    { return authorOPDS2(idLib, idAuthor, request); });

    httpServer_.route(u"/<arg>/authorbooks/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idAuthor, const QHttpServerRequest &request)
    { return authorBooksHTML(idLib, idAuthor, request); });

    httpServer_.route(u"/opds/<arg>/authorbooks/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idAuthor, const QHttpServerRequest &request)
    { return authorBooksOPDS(idLib, idAuthor, request); });

    httpServer_.route(u"/opds2/<arg>/authorbooks/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idAuthor, const QHttpServerRequest &request)
    { return authorBooksOPDS2(idLib, idAuthor, request); });

    httpServer_.route(u"/<arg>/authorseries/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idAuthor, const QHttpServerRequest &request)
    { return authorSeriesHTML(idLib, idAuthor, request); });

    httpServer_.route(u"/opds/<arg>/authorseries/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idAuthor, const QHttpServerRequest &request)
    { return authorSeriesOPDS(idLib, idAuthor, request); });

    httpServer_.route(u"/opds2/<arg>/authorseries/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idAuthor, const QHttpServerRequest &request)
    { return authorSeriesOPDS2(idLib, idAuthor, request); });

    httpServer_.route(u"/<arg>/authorseries/<arg>/<arg>"_s, QHttpServerRequest::Method::Get, [this](uint idLib, uint idAuthor, uint idSeries, const QHttpServerRequest &request)
    { return authorSeriesHTML(idLib, idAuthor, idSeries, request); });

    httpServer_.route(u"/opds/<arg>/authorseries/<arg>/<arg>"_s, QHttpServerRequest::Method::Get, [this](uint idLib, uint idAuthor, uint idSeries, const QHttpServerRequest &request)
    { return authorSeriesOPDS(idLib, idAuthor, idSeries, request); });

    httpServer_.route(u"/opds2/<arg>/authorseries/<arg>/<arg>"_s, QHttpServerRequest::Method::Get, [this](uint idLib, uint idAuthor, uint idSeries, const QHttpServerRequest &request)
    { return authorSeriesOPDS2(idLib, idAuthor, idSeries, request); });

    httpServer_.route(u"/<arg>/authorseriesless/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idAuthor, const QHttpServerRequest &request)
    { return authorSerieslessHTML(idLib, idAuthor, request); });

    httpServer_.route(u"/opds/<arg>/authorseriesless/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idAuthor, const QHttpServerRequest &request)
    { return authorSerieslessOPDS(idLib, idAuthor, request); });

    httpServer_.route(u"/opds2/<arg>/authorseriesless/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idAuthor, const QHttpServerRequest &request)
    { return authorSerieslessOPDS2(idLib, idAuthor, request); });

    httpServer_.route(u"/<arg>/seriesindex"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QHttpServerRequest &request)
    { return seriesIndexHTML(idLib, u""_s, request); });

    httpServer_.route(u"/opds/<arg>/seriesindex"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QHttpServerRequest &request)
    { return seriesIndexOPDS(idLib, u""_s, request); });

    httpServer_.route(u"/opds2/<arg>/seriesindex"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QHttpServerRequest &request)
    { return seriesIndexOPDS2(idLib, u""_s, request); });

    httpServer_.route(u"/<arg>/seriesindex/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return seriesIndexHTML(idLib, sIndex, request); });

    httpServer_.route(u"/opds/<arg>/seriesindex/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return seriesIndexOPDS(idLib, sIndex, request); });

    httpServer_.route(u"/opds2/<arg>/seriesindex/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return seriesIndexOPDS2(idLib, sIndex, request); });

    httpServer_.route(u"/<arg>/seriesindex/<arg>/books"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return seriesIndexHTML(idLib, sIndex, request); });

    httpServer_.route(u"/opds/<arg>/seriesindex/<arg>/books"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return seriesIndexOPDS(idLib, sIndex, request); });

    httpServer_.route(u"/opds2/<arg>/seriesindex/<arg>/books"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return seriesIndexOPDS2(idLib, sIndex, request); });

    httpServer_.route(u"/<arg>/seriesbooks/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idSeries, const QHttpServerRequest &request)
    { return seriesBooksHTML(idLib, idSeries, request); });

    httpServer_.route(u"/opds/<arg>/seriesbooks/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idSeries, const QHttpServerRequest &request)
    { return seriesBooksOPDS(idLib, idSeries, request); });

    httpServer_.route(u"/opds2/<arg>/seriesbooks/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idSeries, const QHttpServerRequest &request)
    { return seriesBooksOPDS2(idLib, idSeries, request); });

    httpServer_.route(u"/<arg>/genres"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QHttpServerRequest &request)
    { return genresHTML(idLib, 0, request); });

    httpServer_.route(u"/opds/<arg>/genres"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QHttpServerRequest &request)
    { return genresOPDS(idLib, 0, request); });

    httpServer_.route(u"/opds2/<arg>/genres"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QHttpServerRequest &request)
                      { return genresOPDS2(idLib, 0, request); });

    httpServer_.route(u"/<arg>/genres/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idGenre, const QHttpServerRequest &request)
    {        return genresHTML(idLib, idGenre, request);    });

    httpServer_.route(u"/opds/<arg>/genres/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idGenre, const QHttpServerRequest &request)
    { return genresOPDS(idLib, idGenre, request); });

    httpServer_.route(u"/opds2/<arg>/genres/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idGenre, const QHttpServerRequest &request)
                      { return genresOPDS2(idLib, idGenre, request); });

    httpServer_.route(u"/<arg>/book/<arg>/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idBook, const QString &sFormat)
    { return bookHTML(idLib, idBook, sFormat); });

    httpServer_.route(u"/opds/<arg>/book/<arg>/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idBook, const QString &sFormat)
    { return bookOPDS(idLib, idBook, sFormat); });

    httpServer_.route(u"/opds2/<arg>/book/<arg>/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idBook, const QString &sFormat)
    { return bookOPDS(idLib, idBook, sFormat); });

    httpServer_.route(u"/<arg>/searchtitle"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QHttpServerRequest &request)
    { return searchTitleHTML(idLib, request); });

    httpServer_.route(u"/<arg>/searchauthor"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QHttpServerRequest &request)
                      { return searchAuthorHTML(idLib, request); });

    httpServer_.route(u"/<arg>/searchseries"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QHttpServerRequest &request)
                      { return searchSeriesHTML(idLib, request); });

    httpServer_.route(u"/opds/<arg>/search"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QHttpServerRequest &request)
    { return searchOPDS(idLib, request); });

    httpServer_.route(u"/opds2/<arg>/search"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QHttpServerRequest &request)
    { return searchOPDS2(idLib, request); });

    httpServer_.route(u"/opds/<arg>/opensearch.xml"_s, QHttpServerRequest::Method::Get,  [](uint /*idLib*/, const QHttpServerRequest &request){
        QString sUrl = request.url().toString();
        sUrl.chop(u"opensearch.xml"_s.size());
        QString sTemplate = sUrl + u"search?q={searchTerms}&amp;author={atom:author}&amp;title={atom:title}"_s;
        QString result = u"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
               "<OpenSearchDescription xmlns=\"http://a9.com/-/spec/opensearch/1.1/\">"
                "<ShortName>freeLib</ShortName>"
                "<Description>Search on freeLib</Description>"
                "<InputEncoding>UTF-8</InputEncoding>"
                "<OutputEncoding>UTF-8</OutputEncoding>"
                "<Url type=\"application/atom+xml\" xmlns:atom=\"http://www.w3.org/2005/Atom\" template=\""_s % sTemplate % u"\"/>"_s
               u"<Url type=\"application/atom+xml;profile=opds-catalog;kind=acquisition\" template=\""_s % sTemplate % u"\"/>"_s
               u"</OpenSearchDescription>"_s;
        return result;
    });

    status_ = Status::stoped;
    nPort_ = 0;
}

QDomElement opds_server::addNode(QDomNode &parrentNode, const QString &name, const QString &sClass)
{
    QDomElement el = doc_.createElement(name);
    el.setAttribute(u"class"_s, sClass);
    parrentNode.appendChild(el);
    return el;
}

QDomElement opds_server::addTextNode(QDomNode &node, const QString &name, const QString &text)
{
    QDomElement el = doc_.createElement(name);
    node.appendChild(el);
    if(!text.isEmpty()) [[likely]]
    {
        QDomText txt = doc_.createTextNode(text);
        el.appendChild(txt);
    }
    return el;
}

void opds_server::addTextNode(QDomNode &node, const QString &sName, const QString &sText, const QString &sClass)
{
    QDomElement el = doc_.createElement(sName);
    node.appendChild(el);
    el.setAttribute(u"class"_s, sClass);
    if(!sText.isEmpty()) [[likely]]
    {
        if (sText.contains(u"<p>")) {
            QDomDocument fragmentDoc;
            QString wrappedText = u"<root>"_s % sText % u"</root>"_s;
            if (fragmentDoc.setContent(wrappedText)) {
                QDomElement root = fragmentDoc.documentElement();
                QDomNode child = root.firstChild();
                while (!child.isNull()) {
                    QDomNode importedNode = doc_.importNode(child, true);
                    el.appendChild(importedNode);
                    child = child.nextSibling();
                }
            } else {
                // Если парсинг не удался, добавляем текст как текстовый узел
                QDomText txt = doc_.createTextNode(sText);
                el.appendChild(txt);
            }
        }else {
            QDomText txt = doc_.createTextNode(sText);
            el.appendChild(txt);
        }
    }
}

QDomElement opds_server::addHRefNode(QDomNode &node, const QString &sText, const QString &sHRef, const QString &sClass)
{
    QDomElement el = doc_.createElement(u"a"_s);
    node.appendChild(el);
    el.setAttribute(u"href"_s, sHRef);
    if(!sClass.isEmpty())
        el.setAttribute(u"class"_s, sClass);
    QDomText txt = doc_.createTextNode(sText);
    el.appendChild(txt);
    return el;
}

void opds_server::addDownloadItem(QDomNode &node, const QString &sText, const QString &sHRef)
{
    QDomElement el = doc_.createElement(u"a"_s);
    el.setAttribute(u"href"_s, sHRef);
    el.setAttribute(u"class"_s, u"download"_s);
    QDomElement img = doc_.createElement(u"img"_s);
    img.setAttribute(u"class"_s, u"download"_s);
    img.setAttribute(u"src"_s, u"/assets/download.svg"_s);
    el.appendChild(img);
    node.appendChild(el);
    QDomText txt = doc_.createTextNode(sText);
    el.appendChild(txt);
}


void opds_server::addNavigation(QJsonArray &navigation, const QString &sTitle, const QString &sHRef, uint nCount)
{
    QJsonObject entry;
    entry[u"title"] = sTitle;
    entry[u"href"] = sHRef;
    entry[u"type"] = u"application/opds+json"_s;
    if(nCount > 0){
        QJsonObject properties;
        properties[u"numberOfItems"] = static_cast<int>(nCount);
        entry[u"properties"] = properties;
    }
    navigation.push_back(std::move(entry));
}

void opds_server::addLink(QJsonArray &links, const QString &sType, const QString &sHRef, const QString &sRel)
{
    QJsonObject link;
    link[u"rel"] = sRel;
    link[u"type"] = sType;
    link[u"href"] = sHRef;
    links.push_back(std::move(link));
}

void opds_server::addLink(QDomNode &node, const QString &sType, const QString &sHRef, const QString &sRel , const QString &sTitle)
{
    QDomElement link = doc_.createElement(u"link"_s);
    node.appendChild(link);
    link.setAttribute(u"title"_s, sTitle);
    link.setAttribute(u"rel"_s, sRel);
    link.setAttribute(u"type"_s, sType);
    link.setAttribute(u"href"_s, sHRef);
    node.appendChild(link);
}

void opds_server::addLink(QDomNode &node, const QString &sType, const QString &sHRef, const QString &sRel)
{
    QDomElement link = doc_.createElement(u"link"_s);
    node.appendChild(link);
    link.setAttribute(u"rel"_s, sRel);
    link.setAttribute(u"type"_s, sType);
    link.setAttribute(u"href"_s, sHRef);
    node.appendChild(link);
}

void opds_server::addLink(QDomNode &node, const QString &sType, const QString &sHRef)
{
    QDomElement link = doc_.createElement(u"link"_s);
    node.appendChild(link);
    link.setAttribute(u"type"_s, sType);
    link.setAttribute(u"href"_s, sHRef);
    node.appendChild(link);
}

void opds_server::addEntry(QDomElement &feed, const QString &sId, const QString &sHRef, const QString &sTitle, const QString &sContent)
{
    QDomElement entry = doc_.createElement(u"entry"_s);
    feed.appendChild(entry);
    addTextNode(entry, u"updated"_s, QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    addTextNode(entry, u"id"_s, sId);
    addTextNode(entry, u"title"_s, sTitle);
    if(!sContent.isEmpty()){
        QDomElement el = addTextNode(entry, u"content"_s, sContent);
        el.setAttribute(u"type"_s, u"text"_s);
    }
    addLink(entry, u"application/atom+xml;profile=opds-catalog"_s, sHRef);
}

QString opds_server::query(const QUrlQuery &urlQuery)
{
    QString sQuery = urlQuery.toString();
    if(!sQuery.isEmpty())
        sQuery.insert(0, u'?');
    return sQuery;
}

QString simplifySearchString(const QString &str)
{
    QString sSimplified = str;
    const static QRegularExpression simpl(u"[.,«»?!-\"'()]"_s);
    sSimplified = sSimplified.replace(simpl, u" "_s).simplified().toCaseFolded();

    return sSimplified;
}

std::vector<uint> opds_server::listBooks(const SLib &lib, uint idAuthor, uint idSeries, const QStringView sLanguageFilter, bool bSeriesless)
{
    std::vector<uint> vBooks;
    if(idAuthor !=0 && idSeries != 0){
        auto [begin, end] = lib.authorBooksLink.equal_range(idAuthor);
        for (auto it = begin; it != end; ++it) {
            uint idBook = it->second;
            auto &book = lib.books.at(idBook);
            if(book.bDeleted) [[unlikely]]
                continue;
            if(book.series.contains(idSeries))
                if(sLanguageFilter.isEmpty() || sLanguageFilter == lib.vLaguages[book.idLanguage])
                    vBooks.push_back(idBook);
        }
        sort(vBooks, [&lib](uint id1, uint id2)
        {
            const auto &series1 = lib.books.at(id1).series;
            const auto &series2 = lib.books.at(id2).series;
            uint nNumInSeries1 = series1.empty() ?0 : series1.begin()->second;
            uint nNumInSeries2 = series1.empty() ?0 : series2.begin()->second;
            return nNumInSeries1 < nNumInSeries2;
        });

    }
    if(idAuthor != 0 && idSeries == 0){
        auto [begin, end] = lib.authorBooksLink.equal_range(idAuthor);
        if(bSeriesless){
            for (auto it = begin; it != end; ++it) {
                uint idBook = it->second;
                auto &book = lib.books.at(idBook);
                if(book.bDeleted) [[unlikely]]
                    continue;
                if(book.series.empty())
                    if(sLanguageFilter.isEmpty() || sLanguageFilter == lib.vLaguages[book.idLanguage])
                        vBooks.push_back(idBook);
            }
        }else{
            for (auto it = begin; it != end; ++it) {
                uint idBook = it->second;
                auto &book = lib.books.at(idBook);
                if(!book.bDeleted) [[likely]]
                    if(sLanguageFilter.isEmpty() || sLanguageFilter == lib.vLaguages[book.idLanguage])
                        vBooks.push_back(idBook);
            }
        }
        sort(vBooks, [&lib](uint lhs, uint rhs){ return localeStringCompare(lib.books.at(lhs).sName, lib.books.at(rhs).sName); });
    }
    if(idAuthor == 0 && idSeries != 0){
        for(const auto &[idBook, book] :lib.books){
            if(book.bDeleted) [[unlikely]]
                continue;
            if(book.series.contains(idSeries))
                if(sLanguageFilter.isEmpty() || sLanguageFilter == lib.vLaguages[book.idLanguage])
                    vBooks.push_back(idBook);
        }
        sort(vBooks, [&lib](uint id1, uint id2)
        {
            const auto &series1 = lib.books.at(id1).series;
            const auto &series2 = lib.books.at(id2).series;
            uint nNumInSeries1 = series1.empty() ?0 : series1.begin()->second;
            uint nNumInSeries2 = series2.empty() ?0 : series2.begin()->second;
            return nNumInSeries1 < nNumInSeries2;
        });
    }
    return vBooks;
}

std::vector<uint> opds_server::listGenreBooks(const SLib &lib, ushort idGenre, const QStringView sLanguageFilter)
{
    std::vector<uint> vBooks;
    for(const auto &[idBook, book] :lib.books){
        if(book.bDeleted) [[unlikely]]
            continue;
        for(auto iGenre: book.vIdGenres){
            if(iGenre == idGenre){
                if(sLanguageFilter.isEmpty() || sLanguageFilter == lib.vLaguages[book.idLanguage]){
                    vBooks.push_back(idBook);
                    break;
                }
            }
        }
    }

    sort(vBooks, [&lib](uint lhs, uint rhs){
        return localeStringCompare(lib.books.at(lhs).sName,lib.books.at(rhs).sName);
    });

    return vBooks;
}

std::vector<uint> opds_server::searchTitle(const SLib &lib, const QString &sTitle, const QStringView sLanguageFilter)
{
    QString sSearch = simplifySearchString(sTitle);
    std::vector<uint> vBooks;
    if(!sSearch.isEmpty()){
        vBooks = blockingFiltered(lib.books, [&](const auto &book){
            if(!book.bDeleted) [[likely]]{
                QString sName = simplifySearchString(book.sName);
                if(sName.contains(sSearch)){
                    if(sLanguageFilter.isEmpty() || sLanguageFilter == lib.vLaguages[book.idLanguage])
                        return true;
                }
            }
            return false;
        });

        sort(vBooks, [&lib](uint lhs, uint rhs){
            return localeStringCompare(lib.books.at(lhs).sName, lib.books.at(rhs).sName);
        });
    }
    return vBooks;
}

std::vector<uint> opds_server::searchBooks(const SLib &lib, const QString &sAuthor, const QString &sTitle, const QStringView sLanguageFilter)
{
    auto vAuthors = searchAuthors(lib, sAuthor);
    QString sSimplifiedTitle = simplifySearchString(sTitle);
    std::unordered_set<uint>stIdAuthors(vAuthors.begin(), vAuthors.end());
    std::vector<uint> vBooks;

    vBooks = blockingFiltered(lib.books, [&](const auto &book){
        if(!book.bDeleted) [[likely]]{
            QString sName = simplifySearchString(book.sName);
            if(sName.contains(sSimplifiedTitle)){
                if(sAuthor.isEmpty() || std::any_of(book.vIdAuthors.begin(), book.vIdAuthors.end(), [&stIdAuthors](auto idAuthor){return stIdAuthors.contains(idAuthor);}))
                    if(sLanguageFilter.isEmpty() || sLanguageFilter == lib.vLaguages[book.idLanguage])
                        return true;
            }
        }
        return false;
    });

    sort(vBooks, [&lib](uint id1, uint id2){
        return localeStringCompare(lib.books.at(id1).sName, lib.books.at(id2).sName);
    });

    return vBooks;
}

auto opds_server::searchSeries(const SLib &lib, const QString &sSeries, const QStringView sLanguageFilter)
{
    QString sSearchSimplefied = simplifySearchString(sSeries);
    SeriesComparator comporator(lib.series);
    std::map<uint, uint, SeriesComparator> mSeries(comporator);

    std::mutex m;
    blockingMap(lib.books, [&](auto &book){
        if(!book.series.empty() && !book.bDeleted){
            if(sLanguageFilter.isEmpty() || sLanguageFilter == lib.vLaguages[book.idLanguage]){
                for(const auto &iSeries :book.series){
                    const auto &series = lib.series.at(iSeries.first);
                    QString sName  = simplifySearchString(series.sName);
                    if(sName.contains(sSearchSimplefied)){
                        std::lock_guard<std::mutex> guard(m);
                        mSeries[iSeries.first]++;
                    }
                }
            }
        }
    });
    return mSeries;
}

std::vector<uint> opds_server::searchAuthors(const SLib &lib, const QString &sAuthor)
{
    std::vector<uint> vResult;
    auto sListSearch = simplifySearchString(sAuthor).split(u" "_s, Qt::SkipEmptyParts);
    if(sListSearch.isEmpty())
        return vResult;

    vResult = blockingFiltered(lib.authors, [&sListSearch](const auto &author){
        bool bMatch = true;
        bool bMatchF = false;
        bool bMatchM = false;
        bool bMatchL = false;
        for(const auto &sSubSearch :std::as_const(sListSearch)){
            bool bMatchAny = false;
            if(!bMatchF && author.sFirstName.toCaseFolded().startsWith(sSubSearch)){
                bMatchF = true;
                bMatchAny = true;
            }
            if(!bMatchAny && !bMatchM && author.sMiddleName.toCaseFolded().startsWith(sSubSearch)){
                bMatchM = true;
                bMatchAny = true;
            }
            if(!bMatchAny && !bMatchL && author.sLastName.toCaseFolded().startsWith(sSubSearch)){
                bMatchL = true;
                bMatchAny = true;
            }
            if(!bMatchAny){
                bMatch = false;
                break;
            }
        }
        return bMatch;
    });

    return vResult;
}

bool opds_server::checkAuth(const QHttpServerRequest &request, QUrl &url)
{
    auto timeExpire = QDateTime::currentDateTime().addSecs(-60*60);
    erase_if(sessions_, [&timeExpire](const auto &it) { return it.second < timeExpire; });

    bool bResult = false;
    url = request.url();
    if(g::options.bOpdsNeedPassword){
        QUrlQuery urlquery(url);
        QString sSession = urlquery.queryItemValue(u"session"_s);
        if(!sSession.isEmpty() && sessions_.contains(sSession)){
            sessions_[sSession] = QDateTime::currentDateTime();
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
                    if(!g::options.baOpdsPasswordSalt.isEmpty() && !g::options.baOpdsPasswordHash.isEmpty()){
                        auto hashPassword = Options::passwordToHash(password, g::options.baOpdsPasswordSalt);

                        if (sUser == g::options.sOpdsUser && hashPassword == g::options.baOpdsPasswordHash){
                            bResult = true;
                            static QString chars = u"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"_s;
                            const int sessionNumberLen = 16;
                            sSession = u""_s;
                            srand(time(nullptr));
                            for(int i=0; i<sessionNumberLen; i++)
                                sSession += chars[rand() % chars.size()];
                            sessions_.insert({sSession, QDateTime::currentDateTime()});
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

QDomElement opds_server::docHeaderHTML(SLib &lib, QUrlQuery urlQuery, const QString &sLibUrl)
{
    urlQuery.removeQueryItem(u"page"_s);
    urlQuery.removeQueryItem(u"search_string"_s);
    QString sQuery = query(urlQuery);

    doc_ = QDomDocument(u"HTML"_s);
    QDomElement html = doc_.createElement(u"html"_s);
    doc_.appendChild(html);
    QDomElement head = doc_.createElement(u"head"_s);
    html.appendChild(head);
    addTextNode(head, u"title"_s, lib.name);

    QDomElement script = doc_.createElement(u"script"_s);
    script.setAttribute(u"src"_s, u"/assets/scripts.js"_s);
    script.setAttribute(u"defer"_s, u""_s);
    script.appendChild(doc_.createTextNode(u""_s));
    head.appendChild(script);

    QDomElement link;
    link = doc_.createElement(u"link"_s);
    link.setAttribute(u"rel"_s, u"stylesheet"_s);
    link.setAttribute(u"href"_s, u"/assets/styles.css"_s);
    head.appendChild(link);


    QDomElement meta = doc_.createElement(u"META"_s);
    meta.setAttribute(u"charset"_s, u"utf-8"_s);
    meta.setAttribute(u"name"_s, u"viewport"_s);
    meta.setAttribute(u"content"_s, u"width=device-width, initial-scale=1.0"_s);
    head.appendChild(meta);

    if(!g::options.sBaseUrl.isEmpty()){
        QDomElement baseUrl = doc_.createElement(u"base"_s);
        baseUrl.setAttribute(u"href"_s, g::options.sBaseUrl);
        head.appendChild(baseUrl);
    }

    QString sIconFile = u"/assets/icon_256x256.png"_s;

    addLink(head, u"image/png"_s, sIconFile, u"icon"_s);
    addLink(head, u"image/png"_s, sIconFile, u"shortcut icon"_s);
    addLink(head, u"image/png"_s, sIconFile, u"apple-touch-icon"_s);

    QDomElement body = doc_.createElement(u"body"_s);
    html.appendChild(body);

    auto divHat  = addNode(body, u"div"_s, u"hat"_s);
    auto divHome = addNode(divHat, u"div"_s, u"home"_s);
    QString sHref = (sLibUrl.isEmpty() ?u"/"_s :sLibUrl) + sQuery;
    QDomElement a = addHRefNode(divHome, u""_s, sHref, u"lib"_s);
    QDomElement svg = addNode(a, u"svg"_s, u"home"_s);
    svg.setAttribute(u"xmlns"_s, u"http://www.w3.org/2000/svg"_s);
    svg.setAttribute(u"viewBox"_s, u"0 0 32 32"_s);
    QDomElement path = doc_.createElement(u"path"_s);
    path.setAttribute(u"d"_s, u"M 1,17 16,1 31,17 h -4 v 14 h -8 v -9 h -6 v 9 h -8 V 17 Z"_s);
    svg.appendChild(path);

    addHRefNode(divHome, lib.name, sHref, u"lib"_s);

    QString sLangFilter = urlQuery.queryItemValue(u"lang"_s);
    if(lib.vSortedLaguages.empty())
        lib.sortLanguages();

    auto divlang = addNode(divHat,u"div"_s, u"languageSelector"_s);
    auto divTxt = addTextNode(divlang, u"div"_s, tr("Book language:"));

    QDomElement select = doc_.createElement(u"select"_s);
    select.setAttribute(u"id"_s, u"language-filter"_s);
    divlang.appendChild(select);
    QDomElement option = addTextNode(select, u"option"_s, tr("All"));
    option.setAttribute(u"value"_s, u""_s);
    for(size_t iLang = 0; iLang<lib.vSortedLaguages.size(); iLang++){
        QString sAbbrLanguage = lib.vSortedLaguages[iLang];
        if(!sAbbrLanguage.isEmpty() && sAbbrLanguage != u"un"){
            QString sLanguage;
            if(g::languges.contains(sAbbrLanguage))
                sLanguage = g::languges.at(sAbbrLanguage);
            else
                sLanguage = sAbbrLanguage;

            QDomElement option = addTextNode(select, u"option"_s, sLanguage);
            option.setAttribute(u"value"_s, sAbbrLanguage);
            if(sLangFilter == sAbbrLanguage)
                option.setAttribute(u"selected"_s, u""_s);
        }
    }
    QDomElement hr = doc_.createElement(u"hr"_s);
    body.appendChild(hr);

    return body;
}

QDomElement opds_server::docHeaderOPDS(const QString &sTitle, const QString &sID, const QString &sLibUrl, const QString &sSession)
{
    doc_.clear();
    QDomProcessingInstruction xmlProcessingInstruction = doc_.createProcessingInstruction(u"xml"_s, u"version=\"1.0\" encoding=\"utf-8\""_s);
    doc_.appendChild(xmlProcessingInstruction);
    QDomElement feed = doc_.createElement(u"feed"_s);
    doc_.appendChild(feed);
    feed.setAttribute(u"xmlns"_s, u"http://www.w3.org/2005/Atom"_s);
    feed.setAttribute(u"xmlns:dc"_s, u"http://purl.org/dc/terms/"_s);
    feed.setAttribute(u"xmlns:os"_s, u"http://a9.com/-/spec/opensearch/1.1/"_s);
    feed.setAttribute(u"xmlns:opds"_s, u"http://opds-spec.org/2010/catalog"_s);

    addTextNode(feed, u"updated"_s, QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    addTextNode(feed, u"icon"_s, u"/assets/icon_256x256.png"_s);
    if(!sID.isEmpty())
        addTextNode(feed, u"id"_s, sID);
    addTextNode(feed, u"title"_s, sTitle);

    addLink(feed, u"application/atom+xml;profile=opds-catalog;kind=navigation"_s, sLibUrl, u"start"_s , u"Home"_s);
    addLink(feed, u"application/opensearchdescription+xml"_s, sLibUrl + u"/opensearch.xml"_s, u"search"_s);
    QString sHref = sLibUrl + u"/search?q={searchTerms}&author={atom:author}&title={atom:title}"_s;
    if(!sSession.isEmpty())
        sHref += u"&session="_s + sSession;
    addLink(feed, u"application/atom+xml"_s, sHref, u"search"_s);
    return feed;
}

QJsonObject opds_server::docHeaderOPDS2(SLib &lib, const QString &sTitle, const QString &sLibUrl, const QUrl &url)
{
     QUrlQuery urlquery(url);
    QString sQuery = query(urlquery);
    QString sLangFilter = urlquery.queryItemValue(u"lang"_s);

    QJsonObject root;
    QJsonObject metadata;
    metadata[u"title"] = sTitle;
    root[u"metadata"] = std::move(metadata);

    QJsonArray links;
    addLink(links, u"application/opds+json"_s, sLibUrl % sQuery, u"self"_s);

    QJsonObject linkSearch;
    linkSearch[u"rel"] = u"search"_s;
    QString sHref = sLibUrl % u"/search{?query,title,author}"_s;
    if(!sQuery.isEmpty())
        sHref += u"&"_s + sQuery;
    linkSearch[u"href"] = sHref;
    linkSearch[u"type"] = u"application/opds+json"_s;
    linkSearch[u"templated"] = true;
    links.push_back(std::move(linkSearch));

    QJsonArray facets;
    QJsonObject languageFilter;
    metadata[u"title"] = tr("Book language:");
    languageFilter[u"metadata"] = std::move(metadata);

    QJsonArray linksLanguage;
    QJsonObject linkLanguage;

    if(lib.vSortedLaguages.empty())
        lib.sortLanguages();
    QUrl newUrl;
    if(g::options.sBaseUrl.isEmpty())
        newUrl = url;
    else{
        newUrl = g::options.sBaseUrl;
        newUrl.setPath(url.path());
    }

    linkLanguage[u"type"] = u"application/opds+json"_s;
    linkLanguage[u"title"] = tr("All");
    auto tmpQuery = urlquery;
    setQueryItem(tmpQuery, u"lang"_s, u""_s);
    sHref = tmpQuery.toString();
    linkLanguage[u"href"] = u"?"_s + sHref;
    linksLanguage.push_back(linkLanguage);

    for(size_t iLang = 0; iLang<lib.vSortedLaguages.size(); iLang++){
        QString sAbbrLanguage = lib.vSortedLaguages[iLang];
        if(!sAbbrLanguage.isEmpty() && sAbbrLanguage != u"un"){
            QString sLanguage;
            if(g::languges.contains(sAbbrLanguage))
                sLanguage = g::languges.at(sAbbrLanguage);
            else
                sLanguage = sAbbrLanguage;
            linkLanguage[u"title"] = sLanguage;
            setQueryItem(tmpQuery, u"lang"_s, sAbbrLanguage);
            newUrl.setQuery(tmpQuery);
            linkLanguage[u"href"] = newUrl.toString();
            linkLanguage[u"active"] = sLangFilter==sAbbrLanguage ?u"true"_s :u"false"_s;
            linksLanguage.push_back(linkLanguage);
        }
    }

    languageFilter[u"links"] = linksLanguage;
    facets.push_back(std::move(languageFilter));
    root[u"facets"] = facets;


    root[u"links"] = links;
    return root;
}

SLib* opds_server::getLib(uint &idLib, const QString &sTypeServer, QString *pLibUrl)
{
    if(idLib == 0)
        idLib = g::idCurrentLib;
    if(pLibUrl != nullptr)
        *pLibUrl = g::options.sBaseUrl % sTypeServer % u"/"_s % QString::number(idLib);
    if(!g::libs.contains(idLib) || !g::libs.at(idLib).bLoaded)
        loadLibrary(idLib);
    if(!g::libs.contains(idLib)) [[unlikely]]
        return nullptr;
    SLib &lib = g::libs[idLib];
    lib.timeHttp = std::chrono::system_clock::now();
    return &lib;
}

QHttpServerResponse opds_server::responseHTML()
{
    QHttpServerResponse result("text/html"_ba, doc_.toByteArray(2));
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    QHttpHeaders headers = result.headers();
    headers.append("Server"_ba, "freeLib "_ba + FREELIB_VERSION);
    headers.append("Connection"_ba, "keep-alive"_ba);
    headers.append("Pragma"_ba, "no-cache"_ba);
    result.setHeaders(std::move(headers));
#else
    result.addHeader("Server"_ba, "freeLib "_ba + FREELIB_VERSION);
    result.addHeader("Connection"_ba, "keep-alive"_ba);
    result.addHeader("Pragma"_ba, "no-cache"_ba);
#endif

    return result;
}

QHttpServerResponse opds_server::responseUnauthorized()
{
    QHttpServerResponse result("text/html"_ba, "HTTP/1.1 401 Authorization Required"_ba, QHttpServerResponder::StatusCode::Unauthorized);
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    QHttpHeaders headers  = result.headers();;
    headers.append("WWW-Authenticate"_ba, "Basic"_ba);
    headers.append("Content-Type"_ba, "text/html;charset=utf-8");
    headers.append("Connection"_ba, "close"_ba);
    result.setHeaders(std::move(headers));
#else
    result.addHeader("WWW-Authenticate"_ba, "Basic"_ba);
    result.addHeader("Content-Type"_ba, "text/html;charset=utf-8");
    result.addHeader("Connection"_ba, "close"_ba);
#endif
    return result;
}

QString hrefOfPage(const QUrl &url, uint nPage)
{
    QUrlQuery urlquery(url);
    setQueryItem(urlquery, u"page"_s, QString::number(nPage));
    QUrl newUrl;
    if(g::options.sBaseUrl.isEmpty())
        newUrl = url;
    else{
        newUrl = g::options.sBaseUrl;
        newUrl.setPath(url.path());
    }
    newUrl.setQuery(urlquery);
    QString result = newUrl.toString();
    return result;
}

void opds_server::loadAnnotations(const std::vector<uint> &vBooks, SLib &lib, uint begin, uint end)
{
    if(g::options.bOpdsShowAnotation)
    {
        std::vector<uint> vBooksNeedAnnotations;
        for(uint iBook = begin; iBook < end; ++iBook){
            uint idBook = vBooks.at(iBook);
            SBook& book = lib.books.at(idBook);
            if(book.sAnnotation.isEmpty())
                vBooksNeedAnnotations.push_back(idBook);
        }

        QtConcurrent::blockingMap(vBooksNeedAnnotations, [&](auto idBook){
            SBook& book = lib.books.at(idBook);
            BookFile file(lib, idBook);
            file.open();
            book.sAnnotation = file.annotation();
        });
    }
}

QHttpServerResponse opds_server::generatePageHTML(const std::vector<uint> &vBooks, SLib &lib, const QString &sTitle, const QString &sLibUrl, const QUrl &url, bool bShowAuthor)
{
    QUrlQuery urlquery(url);

    QDomElement feed;
    feed = docHeaderHTML(lib, urlquery, sLibUrl);
    addTextNode(feed, u"div"_s, sTitle, u"caption"_s);

    fillPageHTML(vBooks, lib, feed, sLibUrl, url, bShowAuthor);
    return responseHTML();
}

int convertFormatAvaible()
{
    int result = 0;
    bool bKindleInstallsed = kindlegenInstalled();
    int count = g::options.vExportOptions.size();
    for(int i=0; i<count; i++)
    {
        auto &exportOptions = g::options.vExportOptions[i];
        if(exportOptions.bUseForHttp){
            if(exportOptions.format == epub){
                result |= epub;
                continue;
            }
            if(bKindleInstallsed){
                if(exportOptions.format == mobi)
                    result |= mobi;
                if(exportOptions.format == azw3)
                    result |= azw3;
            }
        }
    }
    return result;
}

void opds_server::fillPageHTML(const std::vector<uint> &vBooks, SLib &lib, QDomElement &feed, const QString &sLibUrl, const QUrl &url, bool bShowAuthor)
{
    auto nMaxBooksPerPage = g::options.nOpdsBooksPerPage;
    if(nMaxBooksPerPage == 0)
        nMaxBooksPerPage = std::numeric_limits<typeof(nMaxBooksPerPage)>::max();

    QUrlQuery urlquery(url);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSessionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;
    QString sQuery = query(urlquery);

    uint nPage = urlquery.queryItemValue(u"page"_s).toUInt();
    if(nPage == 0)
        nPage = 1;


    uint iBookBegin = (nPage-1)*nMaxBooksPerPage;
    uint iBookEnd = std::min(static_cast<uint>(vBooks.size()), nPage*nMaxBooksPerPage);
    loadAnnotations(vBooks, lib, iBookBegin, iBookEnd);

    auto convertFormats =  convertFormatAvaible();
    auto divList = addNode(feed, u"div"_s, u"bookslist"_s);

    for(uint iBook = iBookBegin; iBook < iBookEnd; ++iBook){
        uint idBook = vBooks.at(iBook);
        SBook& book = lib.books.at(idBook);
        QString sIdBook = QString::number(idBook);
        QDomElement entry = addNode(divList, u"div"_s, u"bookcard"_s);

        auto divCover = addNode(entry, u"div"_s, u"cover"_s);

        if(g::options.bOpdsShowCover)
        {
            QDomElement el = addNode(divCover, u"img"_s, u"cover"_s);
            el.setAttribute(u"src"_s, sLibUrl % u"/covers/"_s % sIdBook % u"/cover.jpg"_s);
        }

        QDomElement divBook = addNode(entry, u"div"_s, u"book"_s);
        QString sSeries = book.series.empty() ?u""_s :lib.series.at(book.series.begin()->first).sName;
        uint numInSeries = book.series.empty() ?0 :book.series.begin()->second;
        QString sName = book.sName % (numInSeries==0  ?u""_s :(u" ("_s % sSeries % u"["_s % QString::number(numInSeries) % u"])"_s));
        addTextNode(divBook, u"div"_s, sName, u"bookname"_s);
        if(bShowAuthor)
            addHRefNode(divBook, lib.authors.at(book.idFirstAuthor).getName(), sLibUrl % u"/author/"_s % QString::number(book.idFirstAuthor) % sQuery, u"author"_s);
        QDomElement br = doc_.createElement(u"br"_s);
        divBook.appendChild(br);
        auto divDownload = addNode(divBook, u"div"_s, u"download"_s);

        if(book.sFormat == u"fb2" || book.sFormat == u"fb2.zip")
        {
            addDownloadItem(divDownload, u"fb2"_s, sLibUrl % u"/book/"_s % sIdBook % u"/fb2"_s % sSessionQuery);
            if(convertFormats & epub)
                addDownloadItem(divDownload, u"epub"_s, sLibUrl % u"/book/"_s % sIdBook % u"/epub"_s % sSessionQuery);
            if(convertFormats & mobi)
                addDownloadItem(divDownload, u"mobi"_s, sLibUrl % u"/book/"_s % sIdBook % u"/mobi"_s % sSessionQuery);
            if(convertFormats & azw3)
                addDownloadItem(divDownload, u"azw3"_s, sLibUrl % u"/book/"_s % sIdBook % u"/azw3"_s % sSessionQuery);
        }
        else if(book.sFormat == u"epub" || book.sFormat == u"epub.zip" || (book.sFormat == u"zip" && book.sName.endsWith(u"(epub)")))
        {
            addDownloadItem(divDownload, u"epub"_s, sLibUrl % u"/book/"_s % sIdBook % u"/epub"_s % sSessionQuery);
            if(convertFormats & mobi)
                addDownloadItem(divDownload, u"mobi"_s, sLibUrl % u"/book/"_s % sIdBook % u"/mobi"_s % sSessionQuery);
        }
        else if(book.sFormat == u"mobi"_s)
            addDownloadItem(divDownload, u"mobi"_s, sLibUrl % u"/book/"_s % sIdBook % u"/mobi"_s % sSessionQuery);
        else if(book.sFormat == u"pdf" || book.sFormat == u"pdf.zip" || (book.sFormat == u"zip" && book.sName.endsWith(u"(pdf)")))
            addDownloadItem(divDownload, u"pdf"_s, sLibUrl % u"/book/"_s % sIdBook % u"/pdf"_s % sSessionQuery);
        else if(book.sFormat == u"djvu" || book.sFormat == u"djvu.zip" || (book.sFormat == u"zip" && book.sName.endsWith(u"(djvu)")))
            addDownloadItem(divDownload, u"djvu"_s, sLibUrl % u"/book/"_s % sIdBook % u"/djvu"_s % sSessionQuery);
        else
            addDownloadItem(divDownload, book.sFormat, sLibUrl % u"/book/"_s % sIdBook % u"/download"_s % sSessionQuery);

        if(g::options.bOpdsShowAnotation)
            addTextNode(divBook, u"div"_s, book.sAnnotation, u"annotation"_s);

    }

    uint nPageCount = vBooks.size()>0 ?(vBooks.size()-1) / nMaxBooksPerPage + 1 :0;
    if(nPageCount > 1){
        QDomElement pageBar = addNode(feed, u"div"_s, u"page-bar"_s);

        if(nPage > 1){
            addHRefNode(pageBar, u"<"_s, hrefOfPage(url, nPage-1), u"page"_s);
            addHRefNode(pageBar, u"1"_s, hrefOfPage(url, 1), u"page"_s);
        }
        else{
            addTextNode(pageBar, u"span"_s, u"<"_s, u"arrow"_s);
        }
        if(nPage > 4)
            addTextNode(pageBar, u"span"_s, u"…"_s, u"page"_s);
        for(auto i = std::max(2u, nPage-2); i <nPage; ++i ){
            QString sPageNumber = QString::number(i);
            addHRefNode(pageBar, sPageNumber, hrefOfPage(url, i), u"page"_s);
        }
        addTextNode(pageBar, u"span"_s, QString::number(nPage), u"page-current"_s);

        for(auto i=nPage+1u; i < nPage+3 && i < nPageCount; ++i){
            QString sPageNumber = QString::number(i);
            addHRefNode(pageBar, sPageNumber, hrefOfPage(url, i), u"page"_s);
        }

        if(nPage+3 < nPageCount)
            addTextNode(pageBar, u"span"_s, u"…"_s, u"page"_s);

        if(nPage < nPageCount){
            addHRefNode(pageBar, QString::number(nPageCount), hrefOfPage(url, nPageCount), u"page"_s);
            addHRefNode(pageBar, u">"_s, hrefOfPage(url, nPage+1), u"page"_s);
        }else
            addTextNode(pageBar, u"span"_s, u">"_s, u"arrow"_s);
    }
}

QString opds_server::generatePageOPDS(const std::vector<uint> &vBooks, SLib &lib, const QString &sTitle,const QString &sId, const QString &sLibUrl, const QUrl &url)
{
    auto nMaxBooksPerPage = g::options.nOpdsBooksPerPage;
    if(nMaxBooksPerPage == 0)
        nMaxBooksPerPage = std::numeric_limits<typeof(nMaxBooksPerPage)>::max();

    QUrlQuery urlquery(url);
    uint nPage = urlquery.queryItemValue(u"page"_s).toUInt();
    if(nPage == 0)
        nPage = 1;
    uint nPageCount = (vBooks.size()-1) / nMaxBooksPerPage + 1;
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSessionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    uint iBookBegin = (nPage-1)*nMaxBooksPerPage;
    uint iBookEnd = std::min(static_cast<uint>(vBooks.size()), nPage*nMaxBooksPerPage);
    loadAnnotations(vBooks, lib, iBookBegin, iBookEnd);

    QDomElement feed = docHeaderOPDS(sTitle, sId, sLibUrl, sSession);
    if(nPage>1)
        addLink(feed, u"application/atom+xml;profile=opds-catalog"_s, hrefOfPage(url, nPage-1), u"previous"_s);
    if(nPage < nPageCount)
        addLink(feed, u"application/atom+xml;profile=opds-catalog"_s, hrefOfPage(url, nPage+1), u"next"_s);
    if(nPageCount>1){
        if(nPage>1)
            addLink(feed, u"application/atom+xml;profile=opds-catalog"_s, hrefOfPage(url, 1), u"first"_s);
        if(nPage < nPageCount)
            addLink(feed, u"application/atom+xml;profile=opds-catalog"_s, hrefOfPage(url, nPageCount), u"last"_s);
        QDomElement meta = doc_.createElement(u"metadata"_s);
        meta.setAttribute(u"numberOfItems"_s, static_cast<uint>(vBooks.size()));
        meta.setAttribute(u"itemsPerPage"_s, nMaxBooksPerPage);
        meta.setAttribute(u"currentPage"_s, nPage);
        feed.appendChild(meta);

        addLink(feed, u"application/atom+xml;profile=opds-catalog"_s, hrefOfPage(url, nPage), u"self"_s);
    }
    auto convertFormats =  convertFormatAvaible();


    for(uint iBook = iBookBegin; iBook < iBookEnd; ++iBook){
        uint idBook = vBooks.at(iBook);
        SBook& book = lib.books[idBook];
        QString sIdBook = QString::number(idBook);
        QDomElement entry = doc_.createElement(u"entry"_s);
        feed.appendChild(entry);
        addTextNode(entry, u"updated"_s, book.date.toString(Qt::ISODate));
        addTextNode(entry, u"id"_s, u"tag:book:"_s + QString::number(idBook));
        QString sSeries = book.series.empty() ?u""_s :lib.series[book.series.begin()->first].sName;
        addTextNode(entry, u"title"_s, book.sName % (sSeries.isEmpty() ?u""_s :u" ("_s % sSeries % u")"_s));
        for(uint idAuthor: book.vIdAuthors){
            QDomElement author = doc_.createElement(u"author"_s);
            entry.appendChild(author);
            addTextNode(author, u"name"_s, lib.authors[idAuthor].getName());
        }
        for(auto idGenre: book.vIdGenres){
            QDomElement category = doc_.createElement(u"category"_s);
            entry.appendChild(category);
            category.setAttribute(u"term"_s, g::genres[idGenre].sName);
            category.setAttribute(u"label"_s, g::genres[idGenre].sName);
        }
        QDomElement el;
        if(book.sFormat == u"fb2" || book.sFormat == u"fb2.zip")
        {
            addLink(entry, u"application/x-fictionbook+xml"_s, sLibUrl % u"/book/"_s % sIdBook % u"/fb2"_s % sSessionQuery, u"http://opds-spec.org/acquisition/open-access"_s);
            if(convertFormats & epub)
                addLink(entry, u"application/epub+zip"_s, sLibUrl % u"/book/"_s % sIdBook % u"/epub"_s % sSessionQuery, u"http://opds-spec.org/acquisition/open-access"_s);
            if(convertFormats & mobi)
                addLink(entry, u"application/x-mobipocket-ebook"_s, sLibUrl % u"/book/"_s % sIdBook % u"/mobi"_s % sSessionQuery, u"http://opds-spec.org/acquisition/open-access"_s);
            if(convertFormats & azw3)
                addLink(entry, u"application/x-mobi8-ebook"_s, sLibUrl % u"/book/"_s % sIdBook % u"/azw3"_s % sSessionQuery, u"http://opds-spec.org/acquisition/open-access"_s);
        }
        else if(book.sFormat == u"epub" || book.sFormat == u"epub.zip" || (book.sFormat == u"zip" && book.sName.endsWith(u"(epub)")))
        {
            addLink(entry, u"application/epub+zip"_s, sLibUrl % u"/book/"_s % sIdBook % u"/download"_s % sSessionQuery, u"http://opds-spec.org/acquisition/open-access"_s);
            if(convertFormats & mobi)
                addLink(entry, u"application/x-mobipocket-ebook"_s, sLibUrl % u"/book/"_s % sIdBook % u"/mobi"_s % sSessionQuery, u"http://opds-spec.org/acquisition/open-access"_s);
            if(convertFormats & azw3)
                addLink(entry, u"application/x-mobi8-ebook"_s, sLibUrl % u"/book/"_s % sIdBook % u"/azw3"_s % sSessionQuery, u"http://opds-spec.org/acquisition/open-access"_s);
        }else if(book.sFormat == u"mobi")
            addLink(entry, u"application/x-mobipocket-ebook"_s, sLibUrl % u"/book/"_s % sIdBook % u"/download"_s % sSessionQuery, u"http://opds-spec.org/acquisition/open-access"_s);
        else if(book.sFormat == u"pdf" || book.sFormat == u"pdf.zip" || (book.sFormat == u"zip" && book.sName.endsWith(u"(pdf)")))
            addLink(entry, u"application/pdf"_s, sLibUrl % u"/book/"_s % sIdBook % u"/pdf"_s % sSessionQuery, u"http://opds-spec.org/acquisition/open-access"_s);
        else if(book.sFormat == u"djvu" || book.sFormat == u"djvu.zip" || (book.sFormat == u"zip" && book.sName.endsWith(u"(djvu)")))
            addLink(entry, u"mage/vnd-djvu"_s, sLibUrl % u"/book/"_s % sIdBook % u"/djvu"_s % sSessionQuery, u"http://opds-spec.org/acquisition/open-access"_s);
        else
            addLink(entry, mime(book.sFormat), sLibUrl % u"/book/"_s % sIdBook % u"/download"_s % sSessionQuery, u"alternate"_s, tr("Download"));

        if(g::options.bOpdsShowCover)
        {
            addLink(entry, u"image/jpeg"_s, sLibUrl % u"/covers/"_s % sIdBook % u"/cover.jpg"_s, u"http://opds-spec.org/image"_s);
            addLink(entry, u"image/jpeg"_s, sLibUrl % u"/covers/"_s % sIdBook % u"/cover.jpg"_s, u"x-stanza-cover-image"_s);
            addLink(entry, u"image/jpeg"_s, sLibUrl % u"/covers/"_s % sIdBook % u"/cover.jpg"_s, u"http://opds-spec.org/thumbnail"_s);
            addLink(entry, u"image/jpeg"_s, sLibUrl % u"/covers/"_s % sIdBook % u"/cover.jpg"_s, u"x-stanza-cover-image-thumbnail"_s);
        }
        addTextNode(entry, u"dc:language"_s, lib.vLaguages[book.idLanguage]);
        addTextNode(entry, u"dc:format"_s, book.sFormat);

        if(g::options.bOpdsShowAnotation)
        {
            if(book.sAnnotation.isEmpty()){
                BookFile file(lib, idBook);
                book.sAnnotation = file.annotation();
            }
            el = addTextNode(entry, u"content"_s, book.sAnnotation);
            el.setAttribute(u"type"_s, u"text/html"_s);
        }
    }

    return doc_.toString();
}

QHttpServerResponse opds_server::generatePageOPDS2(const std::vector<uint> &vBooks, SLib &lib, const QString &sTitle, const QString &sLibUrl, const QUrl &url)
{
    auto nMaxBooksPerPage = g::options.nOpdsBooksPerPage;
    if(nMaxBooksPerPage == 0)
        nMaxBooksPerPage = std::numeric_limits<typeof(nMaxBooksPerPage)>::max();

    QUrlQuery urlquery(url);
    uint nPage = urlquery.queryItemValue(u"page"_s).toUInt();
    if(nPage == 0)
        nPage = 1;
    uint nPageCount = (vBooks.size()-1) / nMaxBooksPerPage + 1;
    QString sQuery = query(urlquery);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSessionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    uint iBookBegin = (nPage-1)*nMaxBooksPerPage;
    uint iBookEnd = std::min(static_cast<uint>(vBooks.size()), nPage*nMaxBooksPerPage);
    loadAnnotations(vBooks, lib, iBookBegin, iBookEnd);

    auto root = docHeaderOPDS2(lib, lib.name, sLibUrl, url);
    QJsonArray links = root[u"links"].toArray();
    if(nPage>1)
        addLink(links, u"application/opds+json"_s, hrefOfPage(url, nPage-1), u"previous"_s);
    if(nPage < nPageCount)
        addLink(links, u"application/opds+json"_s, hrefOfPage(url, nPage+1), u"next"_s);
    if(nPageCount>1){
        if(nPage>1)
            addLink(links, u"application/opds+json"_s, hrefOfPage(url, 1), u"first"_s);
        if(nPage < nPageCount)
            addLink(links, u"application/opds+json"_s, hrefOfPage(url, nPageCount), u"last"_s);
        root[u"links"] = links;
        auto metadata = root[u"metadata"].toObject();
        metadata[u"numberOfItems"] = static_cast<int>(vBooks.size());
        metadata[u"currentPage"] = static_cast<int>(nPage);
        metadata[u"itemsPerPage"] = static_cast<int>(nMaxBooksPerPage);
        root[u"metadata"] = metadata;
    }
    auto convertFormats =  convertFormatAvaible();

    QJsonArray publications;
    for(uint iBook = iBookBegin; iBook < iBookEnd; ++iBook){
        QJsonObject entry;
        uint idBook = vBooks.at(iBook);
        SBook& book = lib.books[idBook];
        QString sIdBook = QString::number(idBook);
        QJsonObject metadata;
        QString sSeries = book.series.empty() ?u""_s :lib.series[book.series.begin()->first].sName;
        metadata[u"@type"] = u"http://schema.org/Book"_s;
        metadata[u"title"] = QString(book.sName % (sSeries.isEmpty() ?u""_s :u" ("_s % sSeries % u")"_s));
        if(g::options.bOpdsShowAnotation)
        {
            if(book.sAnnotation.isEmpty()){
                BookFile file(lib, idBook);
                book.sAnnotation = file.annotation();
            }
            metadata[u"description"] = book.sAnnotation;
        }

        QJsonArray authors;
        for(uint idAuthor: book.vIdAuthors){
            QJsonObject joAuthor;
            joAuthor[u"name"] = lib.authors[idAuthor].getName();
            {
                QJsonArray links;
                QJsonObject link;
                link[u"type"] = u"application/opds+json"_s;
                link[u"href"] = QString(sLibUrl % u"/author/"_s % QString::number(idAuthor) % sQuery);
                links.push_back(std::move(link));
                joAuthor[u"links"] = links;
            }
            authors.push_back(std::move(joAuthor));
        }
        metadata[u"author"] = authors;
        metadata[u"language"] = lib.vLaguages[book.idLanguage];

        QJsonArray subject;
        for(auto idGenre: book.vIdGenres){
            QJsonObject genre;
            genre[u"name"] = g::genres[idGenre].sName;
            QJsonArray links;
            QJsonObject link;
            link[u"type"] = u"application/opds+json"_s;
            link[u"href"] = QString(sLibUrl % u"/genres/"_s % QString::number(idGenre) % sQuery);
            links.push_back(std::move(link));
            genre[u"links"] = links;
            subject.push_back(std::move(genre));
        }
        metadata[u"subject"] = subject;
        entry[u"metadata"] = metadata;

        QJsonArray links;
        if(book.sFormat == u"fb2" || book.sFormat == u"fb2.zip")
        {
            addLink(links, u"application/x-fictionbook+xml"_s, sLibUrl % u"/book/"_s % sIdBook % u"/fb2"_s % sSessionQuery, u"http://opds-spec.org/acquisition/open-access"_s);
            if(convertFormats & epub)
                addLink(links, u"application/epub+zip"_s, sLibUrl % u"/book/"_s % sIdBook % u"/epub"_s % sSessionQuery, u"http://opds-spec.org/acquisition/open-access"_s);
            if(convertFormats & mobi)
                addLink(links, u"application/x-mobipocket-ebook"_s, sLibUrl % u"/book/"_s % sIdBook % u"/mobi"_s % sSessionQuery, u"http://opds-spec.org/acquisition/open-access"_s);
            if(convertFormats & azw3)
                addLink(links, u"application/x-mobi8-ebook"_s, sLibUrl % u"/book/"_s % sIdBook % u"/azw3"_s % sSessionQuery, u"http://opds-spec.org/acquisition/open-access"_s);
        }
        else if(book.sFormat == u"epub" || book.sFormat == u"epub.zip" || (book.sFormat == u"zip" && book.sName.endsWith(u"(epub)")))
        {
            addLink(links, u"application/epub+zip"_s, sLibUrl % u"/book/"_s % sIdBook % u"/epub"_s % sSessionQuery, u"http://opds-spec.org/acquisition/open-access"_s);
            if(convertFormats & mobi)
                addLink(links, u"application/x-mobipocket-ebook"_s, sLibUrl % u"/book/"_s % sIdBook % u"/mobi"_s % sSessionQuery, u"http://opds-spec.org/acquisition/open-access"_s);
            if(convertFormats & azw3)
                addLink(links, u"application/x-mobi8-ebook"_s, sLibUrl % u"/book/"_s % sIdBook % u"/azw3"_s % sSessionQuery, u"http://opds-spec.org/acquisition/open-access"_s);
        }else if(book.sFormat == u"mobi")
            addLink(links, u"application/x-mobipocket-ebook"_s, sLibUrl % u"/book/"_s % sIdBook % u"/download"_s % sSessionQuery, u"http://opds-spec.org/acquisition/open-access"_s);
        else if(book.sFormat == u"pdf" || book.sFormat == u"pdf.zip" || (book.sFormat == u"zip" && book.sName.endsWith(u"(pdf)")))
            addLink(links, u"application/pdf"_s, sLibUrl % u"/book/"_s % sIdBook % u"/pdf"_s % sSessionQuery, u"http://opds-spec.org/acquisition/open-access"_s);
        else if(book.sFormat == u"djvu" || book.sFormat == u"djvu.zip" || (book.sFormat == u"zip" && book.sName.endsWith(u"(djvu)")))
            addLink(links, u"image/vnd-djvu"_s, sLibUrl % u"/book/"_s % sIdBook % u"/djvu"_s % sSessionQuery, u"http://opds-spec.org/acquisition/open-access"_s);
        else
            addLink(links,  mime(book.sFormat), sLibUrl % u"/book/"_s % sIdBook % u"/download"_s % sSessionQuery, u"http://opds-spec.org/acquisition/open-access"_s);
        entry[u"links"] = links;

        if(g::options.bOpdsShowCover)
        {
            QJsonArray images;
            QJsonObject image;
            image[u"type"] = u"image/jpeg"_s;
            image[u"href"] = QString(sLibUrl % u"/covers/"_s % sIdBook % u"/cover.jpg"_s);
            images.push_back(std::move(image));
            entry[u"images"] = images;
        }
        publications.push_back(std::move(entry));
    }
    root[u"publications"] = publications;

    QJsonDocument doc(root);
    QHttpServerResponse result("application/opds+json; charset=utf-8"_ba, doc.toJson(QJsonDocument::Compact));
    return result;
}

void opds_server::stop_server()
{
    if(status_ == Status::run)
    {
        status_ = Status::stoped;
        const auto listTcpServsrs = httpServer_.servers();
        for(auto &server: listTcpServsrs) {
            server->close();
        }
    }
}

QHttpServerResponse opds_server::rootHTML(uint idLib, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();

    QString sLibUrl;
    SLib *pLib = getLib(idLib, u""_s, &sLibUrl);
    if(pLib == nullptr)
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sQuery = query(urlquery);

    QDomElement feed = docHeaderHTML(*pLib, urlquery, sLibUrl);
    QDomElement div = doc_.createElement(u"div"_s);
    feed.appendChild(div);
    QDomElement el;

    addHRefNode(div, tr("Finding books by authors"), sLibUrl % u"/authorsindex"_s % sQuery, u""_s);
    div = doc_.createElement(u"div"_s);
    feed.appendChild(div);
    addHRefNode(div, tr("Finding books by series"), sLibUrl % u"/seriesindex"_s % sQuery, u""_s);
    div = doc_.createElement(u"div"_s);
    feed.appendChild(div);
    addHRefNode(div, tr("Finding books by genre"), sLibUrl % u"/genres"_s % sQuery, u""_s);

    QDomElement hr = doc_.createElement(u"HR"_s);
    feed.appendChild(hr);
    attachSearchFormHTML(feed, tr("Finding books by title") + u": "_s, sLibUrl + u"/searchtitle"_s, u""_s, urlquery);

    return responseHTML();
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
    QString sSessionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    QDomElement feed = docHeaderOPDS(pLib->name, u"tag:root"_s, sLibUrl, sSession);

    addEntry(feed, u"tag:root:authors"_s, sLibUrl % u"/authorsindex"_s % sSessionQuery, tr("Books by authors"), tr("Finding books by authors"));
    addEntry(feed, u"tag:root:series"_s, sLibUrl % u"/seriesindex"_s % sSessionQuery, tr("Books by series"), tr("Finding books by series"));
    addEntry(feed, u"tag:root:genre"_s, sLibUrl % u"/genres"_s % sSessionQuery, tr("Books by genre"), tr("Finding books by genre"));

    QHttpServerResponse result("application/atom+xml;charset=utf-8"_ba, doc_.toByteArray());
    return result;
}

QHttpServerResponse opds_server::rootOPDS2(uint idLib, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u"/opds2"_s, &sLibUrl);
    if(pLib == nullptr)
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sQuery = query(urlquery);
    QJsonObject root = docHeaderOPDS2(*pLib, pLib->name, sLibUrl, url);

    QJsonArray navigation;
    addNavigation(navigation, tr("Books by authors"), sLibUrl % u"/authorsindex"_s % sQuery);
    addNavigation(navigation, tr("Books by series"), sLibUrl % u"/seriesindex"_s % sQuery);
    addNavigation(navigation, tr("Books by genre"), sLibUrl % u"/genres"_s % sQuery);
    root[u"navigation"] = navigation;

    QJsonDocument doc(root);
    QHttpServerResponse result("application/opds+json; charset=utf-8"_ba, doc.toJson(QJsonDocument::Compact));
    return result;
}

QByteArray opds_server::cover(uint idLib, uint idBook)
{
    QByteArray baResult;
    if(g::libs.contains(idLib) && g::libs.at(idLib).books.contains(idBook)){
        BookFile file(idLib, idBook);
        file.open();
        QImage img = file.cover();
        QBuffer buffer(&baResult);
        buffer.open(QIODevice::WriteOnly);
        img.save(&buffer, "jpg");
    }
    return baResult;
}

void opds_server::attachSearchFormHTML(QDomElement &feed, const QString &sTitle, const QString &sAction, const QString &sSearch, const QUrlQuery &urlQuery)
{
    QDomElement div = doc_.createElement(u"div"_s);

    QDomElement form = doc_.createElement(u"form"_s);
    form.setAttribute(u"method"_s, u"get"_s);
    form.setAttribute(u"action"_s, sAction);
    feed.appendChild(form);
    form.appendChild(div);

    QDomElement el;
    addTextNode(div, u"div"_s, sTitle, u"search"_s);

    el = doc_.createElement(u"input"_s);
    el.setAttribute(u"type"_s, u"search"_s);
    el.setAttribute(u"name"_s, u"search_string"_s);
    if(!sSearch.isEmpty())
        el.setAttribute(u"value"_s, sSearch);
    div.appendChild(el);

    QString sSession = urlQuery.queryItemValue(u"session"_s);
    if(!sSession.isEmpty()){
        el = doc_.createElement(u"input"_s);
        el.setAttribute(u"type"_s, u"hidden"_s);
        el.setAttribute(u"name"_s, u"session"_s);
        el.setAttribute(u"value"_s, sSession);
        div.appendChild(el);
    }

    QString sLanguageFilter = urlQuery.queryItemValue(u"lang"_s);
    if(!sLanguageFilter.isEmpty()){
        el = doc_.createElement(u"input"_s);
        el.setAttribute(u"type"_s, u"hidden"_s);
        el.setAttribute(u"name"_s, u"lang"_s);
        el.setAttribute(u"value"_s, sLanguageFilter);
        div.appendChild(el);
    }

    el = doc_.createElement(u"input"_s);
    el.setAttribute(u"type"_s, u"submit"_s);
    el.setAttribute(u"value"_s, tr("Find"));
    div.appendChild(el);
}

QHttpServerResponse opds_server::authorsIndexHTML(uint idLib, const QString &sIndex, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u""_s, &sLibUrl);
    if(pLib == nullptr)
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sLanguageFilter = urlquery.queryItemValue(u"lang"_s);
    QString sQuery = query(urlquery);

    QDomElement feed = docHeaderHTML(*pLib, urlquery, sLibUrl);

    std::unordered_map<QString, uint> mCount;
    std::vector<QString> vIndex;
    std::unordered_set<QString> setAuthors;
    std::unordered_set<uint>setIdAuthors;
    std::unordered_map<QString, uint> mIdIndex;

    int count = 0;
    QString sLowerIndex = sIndex.toCaseFolded();
    for(const auto &iBook :pLib->books){
        if(!iBook.second.bDeleted) [[likely]]{
            if(sLanguageFilter.isEmpty() || sLanguageFilter == pLib->vLaguages[iBook.second.idLanguage])
                for(auto idAuthor :iBook.second.vIdAuthors)
                    setIdAuthors.insert(idAuthor);
        }
    }
    for(auto idAuthor :setIdAuthors)
    {
        const auto &author  = pLib->authors.at(idAuthor);
        QString sAuthorName = author.getName();
        if(sAuthorName.left(sIndex.length()).toCaseFolded() == sLowerIndex)
        {
            count++;
            QString sNewIndex = sAuthorName.left(sIndex.length() + 1).toCaseFolded();

            sNewIndex[0] = sNewIndex[0].toUpper();
            ++mCount[sNewIndex];
            if(!mIdIndex.contains(sNewIndex)){
                mIdIndex[sNewIndex] = idAuthor;
                vIndex.push_back(sNewIndex);
            }
            if(sNewIndex.length() == sAuthorName.length())
                setAuthors.insert(sNewIndex);
        }
    }


    if(count>30)
    {
        attachSearchFormHTML(feed, tr("Finding authors") + u": "_s, sLibUrl + u"/searchauthor"_s, sIndex, urlquery);
        auto divGrid = addNode(feed, u"div"_s, u"griditems"_s);

        std::ranges::sort(vIndex, [](const auto &s1, const auto &s2){return localeStringCompare(s1, s2);});

        for(const auto &sIndex :vIndex)
        {
            QString sHref;
            auto nCount = mCount.at(sIndex);
            if(nCount == 1)
            {
                auto idAuthor = mIdIndex[sIndex];
                sHref = sLibUrl % u"/author/"_s % QString::number(idAuthor) % sQuery;
            }else{
                sHref = sLibUrl % u"/authorsindex/"_s % QString::fromUtf8(QUrl::toPercentEncoding(sIndex, ""_ba, "."_ba)) %
                        (setAuthors.contains(sIndex) ?u"/books"_s :u""_s) % sQuery;
            }
            auto div = addNode(divGrid, u"div"_s, u"item"_s);
            addHRefNode(div, sIndex, sHref, u"block"_s);
            addTextNode(div, u"div"_s, QString::number(nCount) % u" "_s % tr("authors beginning with") % u" '"_s % sIndex % u"'"_s );
        }

    }
    else
    {
        std::vector<uint> vAuthorId;
        for(auto idAuthor :setIdAuthors)
        {
            const auto &author = pLib->authors.at(idAuthor);
            if(author.getName().left(sIndex.length()).toCaseFolded() == sLowerIndex)
                vAuthorId.push_back(idAuthor);
        }
        std::ranges::sort(vAuthorId, [pLib](uint id1, uint id2)
                          { return localeStringCompare(pLib->authors.at(id1).getName(),  pLib->authors.at(id2).getName()); });
        auto divList = addNode(feed, u"div"_s, u"listitems"_s);
        for(uint idAuthor: vAuthorId)
        {
            auto [begin, end] = pLib->authorBooksLink.equal_range(idAuthor);
            uint nBooksCount = std::count_if(begin, end, [&pLib, &sLanguageFilter](const auto& pair) {
                const auto& book = pLib->books.at(pair.second);
                return !book.bDeleted && (sLanguageFilter.isEmpty() || sLanguageFilter == pLib->vLaguages[book.idLanguage]);
            });

            QDomElement div = addNode(divList, u"div"_s, u"item"_s);
            addHRefNode(div, pLib->authors.at(idAuthor).getName(), sLibUrl % u"/author/"_s % QString::number(idAuthor) % sQuery, u"block"_s);
            addTextNode(div, u"div"_s, QString::number(nBooksCount) % u" "_s % tr("books"));
        }
    }

    return responseHTML();
}

QHttpServerResponse opds_server::authorsIndexOPDS(uint idLib, const QString &sIndex, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    const SLib *pLib = getLib(idLib, u"/opds"_s, &sLibUrl);
    if(pLib == nullptr) [[unlikely]]
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSessionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    QDomElement feed = docHeaderOPDS(tr("Books by authors"), u"tag:root:authors"_s, sLibUrl, sSession);

    std::unordered_map<QString, uint> mCount;
    std::vector<QString> vIndex;
    std::unordered_set <QString> setAuthors;
    std::unordered_set<uint> setIdAuthors;
    std::unordered_map<QString, uint> mIdIndex;

    int count = 0;
    QString sLowerIndex = sIndex.toCaseFolded();
    for(const auto &iBook :pLib->books){
        if(!iBook.second.bDeleted) [[likely]]{
            if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == pLib->vLaguages[iBook.second.idLanguage])
                for(auto idAuthor :iBook.second.vIdAuthors)
                    setIdAuthors.insert(idAuthor);
        }
    }
    for(auto idAuthor :setIdAuthors)
    {
        const auto &author  = pLib->authors.at(idAuthor);
        QString sAuthorName = author.getName();
        if(sAuthorName.left(sIndex.length()).toCaseFolded() == sLowerIndex)
        {
            count++;
            QString sNewIndex = sAuthorName.left(sIndex.length() + 1).toCaseFolded();

            sNewIndex[0] = sNewIndex[0].toUpper();
            ++mCount[sNewIndex];
            if(!mIdIndex.contains(sNewIndex)){
                mIdIndex[sNewIndex] = idAuthor;
                vIndex.push_back(sNewIndex);
            }
            if(sNewIndex.length() == sAuthorName.length())
                setAuthors.insert(sNewIndex);
        }
    }

    if(count>30)
    {
        std::ranges::sort(vIndex, [](const auto &s1, const auto &s2){return localeStringCompare(s1, s2);});
        for(const auto &sIndex :vIndex)
        {
            QString sHref;
            auto nCount = mCount.at(sIndex);
            QString sContent = QString::number(nCount) % u" "_s % tr("authors beginning with") % u" '"_s % sIndex % u"'"_s;
            if(nCount == 1)
            {
                auto idAuthor = mIdIndex[sIndex];
                sHref = sLibUrl % u"/author/"_s % QString::number(idAuthor) % sSessionQuery;
            }else{
                sHref = sLibUrl % u"/authorsindex/"_s % QString::fromUtf8(QUrl::toPercentEncoding(sIndex, ""_ba, "."_ba)) %
                        (setAuthors.contains(sIndex) ?u"/books"_s :u""_s) % sSessionQuery;
            }
            addEntry(feed, u"tag:authors:"_s + sIndex, sHref, sIndex, sContent);
        }
    }
    else
    {
        std::vector<uint> vAuthorId;
        for(auto idAuthor :setIdAuthors)
        {
            const auto &author = pLib->authors.at(idAuthor);
            if(author.getName().left(sIndex.length()).toCaseFolded() == sLowerIndex)
                vAuthorId.push_back(idAuthor);
        }
        std::ranges::sort(vAuthorId, [pLib](uint id1, uint id2)
                          { return localeStringCompare(pLib->authors.at(id1).getName(),  pLib->authors.at(id2).getName()); });

        for(uint idAuthor: vAuthorId)
        {
            auto [begin, end] = pLib->authorBooksLink.equal_range(idAuthor);
            uint nBooksCount = std::count_if(begin, end, [&pLib, this](const auto& pair) {
                const auto& book = pLib->books.at(pair.second);
                return !book.bDeleted && (sLanguageFilter_.isEmpty() || sLanguageFilter_ == pLib->vLaguages[book.idLanguage]);
            });

            addEntry(feed, u"tag:author:"_s + QString::number(idAuthor), sLibUrl % u"/author/"_s % QString::number(idAuthor) % sSessionQuery,
                     pLib->authors.at(idAuthor).getName(), QString::number(nBooksCount) % u" "_s % tr("books"));
        }
    }

    QHttpServerResponse result(doc_.toString());
    return result;
}

QHttpServerResponse opds_server::authorsIndexOPDS2(uint idLib, const QString &sIndex, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u"/opds2"_s, &sLibUrl);
    if(pLib == nullptr) [[unlikely]]
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sQuery = query(urlquery);
    QString sLanguageFilter = urlquery.queryItemValue(u"lang"_s);

    QJsonObject root = docHeaderOPDS2(*pLib, tr("Books by authors"), sLibUrl, url);

    std::unordered_map<QString, uint> mCount;
    std::vector<QString> vIndex;
    std::unordered_set <QString> setAuthors;
    std::unordered_set<uint>setIdAuthors;
    std::unordered_map<QString, uint> mIdIndex;

    int count = 0;
    QString sLowerIndex = sIndex.toCaseFolded();
    for(const auto &iBook :pLib->books){
        if(!iBook.second.bDeleted) [[likely]]{
            if(sLanguageFilter.isEmpty() || sLanguageFilter == pLib->vLaguages[iBook.second.idLanguage])
                for(auto idAuthor :iBook.second.vIdAuthors)
                    setIdAuthors.insert(idAuthor);
        }
    }
    for(auto idAuthor :setIdAuthors)
    {
        const auto &author  = pLib->authors.at(idAuthor);
        QString sAuthorName = author.getName();
        if(sAuthorName.left(sIndex.length()).toCaseFolded() == sLowerIndex)
        {
            count++;
            QString sNewIndex = sAuthorName.left(sIndex.length() + 1).toCaseFolded();

            sNewIndex[0] = sNewIndex[0].toUpper();
            ++mCount[sNewIndex];
            if(!mIdIndex.contains(sNewIndex)){
                mIdIndex[sNewIndex] = idAuthor;
                vIndex.push_back(sNewIndex);
            }
            if(sNewIndex.length() == sAuthorName.length())
                setAuthors.insert(sNewIndex);
        }
    }

    QJsonArray navigation;
    if(count>30)
    {
        std::ranges::sort(vIndex, [](const auto &s1, const auto &s2){return localeStringCompare(s1, s2);});
        for(const auto &sIndex :vIndex)
        {
            QString sHref;
            auto nCount = mCount.at(sIndex);
            if(nCount == 1)
            {
                auto idAuthor = mIdIndex[sIndex];
                sHref = sLibUrl % u"/author/"_s % QString::number(idAuthor) % sQuery;
            }else{
                sHref = sLibUrl % u"/authorsindex/"_s % QString::fromUtf8(QUrl::toPercentEncoding(sIndex, ""_ba, "."_ba)) %
                        (setAuthors.contains(sIndex) ?u"/books"_s :u""_s) % sQuery;
            }
            addNavigation(navigation, sIndex, sHref,  nCount);
        }
    }
    else
    {
        std::vector<uint> vAuthorId;
        for(auto idAuthor :setIdAuthors)
        {
            const auto &author = pLib->authors.at(idAuthor);
            if(author.getName().left(sIndex.length()).toCaseFolded() == sLowerIndex)
                vAuthorId.push_back(idAuthor);
        }
        std::ranges::sort(vAuthorId, [pLib](uint id1, uint id2)
                          { return localeStringCompare(pLib->authors.at(id1).getName(),  pLib->authors.at(id2).getName()); });
        for(uint idAuthor: vAuthorId)
        {
            auto [begin, end] = pLib->authorBooksLink.equal_range(idAuthor);
            uint nBooksCount = std::count_if(begin, end, [&pLib, &sLanguageFilter](const auto& pair) {
                const auto& book = pLib->books.at(pair.second);
                return !book.bDeleted && (sLanguageFilter.isEmpty() || sLanguageFilter == pLib->vLaguages[book.idLanguage]);
            });
            addNavigation(navigation, pLib->authors.at(idAuthor).getName(), sLibUrl % u"/author/"_s % QString::number(idAuthor) % sQuery, nBooksCount);
        }
    }
    root[u"navigation"] = navigation;

    QJsonDocument doc(root);
    QHttpServerResponse result("application/opds+json; charset=utf-8"_ba, doc.toJson(QJsonDocument::Compact));
    return result;
}

QHttpServerResponse opds_server::authorHTML(uint idLib, uint idAuthor, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u""_s, &sLibUrl);
    if(pLib == nullptr || !pLib->authors.contains(idAuthor))
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sQuery = query(urlquery);

    QString sAuthor = pLib->authors.at(idAuthor).getName();
    QString sIdAuthor = QString::number(idAuthor);
    QDomElement feed = docHeaderHTML(*pLib, urlquery, sLibUrl);
    QDomElement divAuth = addNode(feed, u"div"_s, u"item"_s);
    addTextNode(divAuth, u"div"_s, tr("Books by") % u" "_s % sAuthor, u"caption"_s);

    QDomElement div = doc_.createElement(u"div"_s);
    divAuth.appendChild(div);
    addHRefNode(div, tr("Books by series"), sLibUrl % u"/authorseries/"_s % sIdAuthor % sQuery, u"block"_s);

    div = doc_.createElement(u"div"_s);
    divAuth.appendChild(div);
    addHRefNode(div, tr("Books without series"), sLibUrl % u"/authorseriesless/"_s % sIdAuthor % sQuery, u"block"_s);

    div = doc_.createElement(u"div"_s);
    divAuth.appendChild(div);
    addHRefNode(div, tr("All books"), sLibUrl % u"/authorbooks/"_s % sIdAuthor % sQuery, u"block"_s);

    return responseHTML();
}

QHttpServerResponse opds_server::authorOPDS(uint idLib, uint idAuthor, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    const SLib *pLib = getLib(idLib, u"/opds"_s, &sLibUrl);
    if(pLib == nullptr || !pLib->authors.contains(idAuthor)) [[unlikely]]
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSessionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    QString sAuthor = pLib->authors.at(idAuthor).getName();
    QString sIdAuthor = QString::number(idAuthor);
    QDomElement feed = docHeaderOPDS(tr("Books by") % u" "_s % sAuthor, u"tag:author:"_s + sIdAuthor, sLibUrl, sSession);

    addEntry(feed, u"tag:author:"_s % sIdAuthor % u":series"_s, sLibUrl % u"/authorseries/"_s % sIdAuthor % sSessionQuery, tr("Books by series"), u""_s);
    addEntry(feed, u"tag:author:"_s % sIdAuthor % u":seriesless"_s, sLibUrl % u"/authorseriesless/"_s % sIdAuthor % sSessionQuery, tr("Books without series"), u""_s);
    addEntry(feed, u"tag:author:"_s % sIdAuthor % u":series"_s, sLibUrl % u"/authorbooks/"_s % sIdAuthor % sSessionQuery, tr("All books"), u""_s);

    QHttpServerResponse result(doc_.toString());
    return result;
}

QHttpServerResponse opds_server::authorOPDS2(uint idLib, uint idAuthor, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u"/opds2"_s, &sLibUrl);
    if(pLib == nullptr || !pLib->authors.contains(idAuthor)) [[unlikely]]
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sQuery = query(urlquery);

    QString sAuthor = pLib->authors.at(idAuthor).getName();
    QString sIdAuthor = QString::number(idAuthor);
    QJsonObject root = docHeaderOPDS2(*pLib, tr("Books by") % u" "_s % sAuthor, sLibUrl, url);

    QJsonArray navigation;
    addNavigation(navigation, tr("Books by series"), sLibUrl % u"/authorseries/"_s % sIdAuthor % sQuery);
    addNavigation(navigation, tr("Books without series"), sLibUrl % u"/authorseriesless/"_s % sIdAuthor % sQuery);
    addNavigation(navigation, tr("All books"), sLibUrl % u"/authorbooks/"_s % sIdAuthor % sQuery);
    root[u"navigation"] = navigation;

    QJsonDocument doc(root);
    QHttpServerResponse result("application/opds+json; charset=utf-8"_ba, doc.toJson(QJsonDocument::Compact));
    return result;
}

QHttpServerResponse opds_server::authorBooksHTML(uint idLib, uint idAuthor, const QHttpServerRequest &request)
{
    QString sLibUrl;
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    SLib *pLib = getLib(idLib, u""_s, &sLibUrl);
    if(pLib == nullptr || !pLib->authors.contains(idAuthor)) [[unlikely]]
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
    QUrlQuery urlquery(url);
    QString sLangFilter = urlquery.queryItemValue(u"lang"_s);


    std::vector<uint> vBooks = listBooks(*pLib, idAuthor, 0, sLangFilter, false);
    return generatePageHTML(vBooks, *pLib, tr("Books by ABC") % u" ("_s % pLib->authors.at(idAuthor).getName() % u")"_s, sLibUrl, url, false);
}

QHttpServerResponse opds_server::authorBooksOPDS(uint idLib, uint idAuthor, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u"/opds"_s, &sLibUrl);
    if(pLib == nullptr || !pLib->authors.contains(idAuthor)) [[unlikely]]
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
    QUrlQuery urlquery(url);
    QString sLangFilter = urlquery.queryItemValue(u"lang"_s);

    std::vector<uint> vBooks = listBooks(*pLib, idAuthor, 0, sLangFilter, false);
    QString sPage = generatePageOPDS(vBooks, *pLib, tr("Books by ABC") % u" ("_s % pLib->authors.at(idAuthor).getName() % u")"_s, u"id:autorbooks:"_s + QString::number(idAuthor), sLibUrl, url);

    QHttpServerResponse result(sPage);
    return result;
}

QHttpServerResponse opds_server::authorBooksOPDS2(uint idLib, uint idAuthor, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u"/opds2"_s, &sLibUrl);
    if(pLib == nullptr || !pLib->authors.contains(idAuthor)) [[unlikely]]
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
    QUrlQuery urlquery(url);
    QString sLanguageFilter = urlquery.queryItemValue(u"lang"_s);

    std::vector<uint> vBooks = listBooks(*pLib, idAuthor, 0, sLanguageFilter, false);
    QHttpServerResponse result = generatePageOPDS2(vBooks, *pLib, tr("Books by ABC") % u" ("_s % pLib->authors.at(idAuthor).getName() % u")"_s, sLibUrl, url);
    return result;
}

QHttpServerResponse opds_server::authorSeriesHTML(uint idLib, uint idAuthor, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u""_s, &sLibUrl);
    if(pLib == nullptr || !pLib->authors.contains(idAuthor)) [[unlikely]]
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sLanguageFilter = urlquery.queryItemValue(u"lang"_s);
    QString sQuery = query(urlquery);

    QDomElement feed = docHeaderHTML(*pLib, urlquery, sLibUrl);
    addTextNode(feed, u"div"_s, tr("Book series") % u" "_s % pLib->authors.at(idAuthor).getName(), u"caption"_s);

    SeriesComparator comporator(pLib->series);
    std::map<uint, uint, SeriesComparator> mCountBooks(comporator);
    auto [begin, end] = pLib->authorBooksLink.equal_range(idAuthor);
    for (auto it = begin; it != end; ++it) {
        const SBook& book = pLib->books.at(it->second);
        if(book.bDeleted) [[unlikely]]
            continue;
        if(!book.series.empty())
            if(sLanguageFilter.isEmpty() || sLanguageFilter == pLib->vLaguages[book.idLanguage])
                for(const auto &iSeries :book.series)
                    ++mCountBooks[iSeries.first];
    }

    auto divList = addNode(feed, u"div"_s, u"listitems"_s);
    QString sIdAuthor = QString::number(idAuthor);
    for(auto [idSeries, count] :mCountBooks)
    {
        QDomElement entry = addNode(divList, u"div"_s, u"item"_s);
        addHRefNode(entry, pLib->series.at(idSeries).sName, sLibUrl % u"/authorseries/"_s % sIdAuthor % u"/"_s % QString::number(idSeries) % sQuery, u"block"_s);
        addTextNode(entry, u"div"_s, QString::number(count) % u" "_s % tr("books in series"));
    }
    return responseHTML();
}

QHttpServerResponse opds_server::authorSeriesOPDS(uint idLib, uint idAuthor, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    const SLib *pLib = getLib(idLib, u"/opds"_s, &sLibUrl);
    if(pLib == nullptr || !pLib->authors.contains(idAuthor)) [[unlikely]]
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSessionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    QString sIdAuthor = QString::number(idAuthor);
    QDomElement feed = docHeaderOPDS(tr("Book series") % u" "_s % pLib->authors.at(idAuthor).getName(), u"tag:author:"_s + sIdAuthor, sLibUrl, sSession);

    SeriesComparator comporator(pLib->series);
    std::map<uint, uint, SeriesComparator> mCountBooks(comporator);
    auto [begin, end] = pLib->authorBooksLink.equal_range(idAuthor);
    for (auto it = begin; it != end; ++it) {
        const SBook& book = pLib->books.at(it->second);
        if(book.bDeleted) [[unlikely]]
            continue;
        if(!book.series.empty()){
            if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == pLib->vLaguages[book.idLanguage])
                for(const auto &iSeries :book.series)
                    ++mCountBooks[iSeries.first];
        }
    }

    for(auto [idSeries, count] :mCountBooks)
    {
        QString sIdSeries = QString::number(idSeries);
        addEntry(feed, u"tag:author:"_s % sIdAuthor % u":series:"_s % sIdSeries, sLibUrl % u"/authorseries/"_s % sIdAuthor % u"/"_s % sIdSeries % sSessionQuery,
                 pLib->series.at(idSeries).sName, QString::number(count) % u" "_s % tr("books in series"));
    }
    QHttpServerResponse result(doc_.toString());
    return result;
}

QHttpServerResponse opds_server::authorSeriesOPDS2(uint idLib, uint idAuthor, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u"/opds2"_s, &sLibUrl);
    if(pLib == nullptr || !pLib->authors.contains(idAuthor)) [[unlikely]]
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sQuery = query(urlquery);
    QString sLanguageFilter = urlquery.queryItemValue(u"lang"_s);
    QString sIdAuthor = QString::number(idAuthor);
    auto root = docHeaderOPDS2(*pLib, tr("Book series") % u" "_s % pLib->authors.at(idAuthor).getName(), sLibUrl, url);

    SeriesComparator comporator(pLib->series);
    std::map<uint, uint, SeriesComparator> mCountBooks(comporator);
    auto [begin, end] = pLib->authorBooksLink.equal_range(idAuthor);
    for (auto it = begin; it != end; ++it) {
        const SBook& book = pLib->books.at(it->second);
        if(book.bDeleted) [[unlikely]]
            continue;
        if(!book.series.empty()){
            if(sLanguageFilter.isEmpty() || sLanguageFilter == pLib->vLaguages[book.idLanguage])
                for(const auto &iSeries :book.series)
                    ++mCountBooks[iSeries.first];
        }
    }

    QJsonArray navigation;
    for(auto [idSeries, count] :mCountBooks)
    {
        QString sIdSeries = QString::number(idSeries);
        addNavigation(navigation, pLib->series.at(idSeries).sName, sLibUrl % u"/authorseries/"_s % sIdAuthor % u"/"_s % sIdSeries % sQuery, count);
    }
    root[u"navigation"] = navigation;

    QJsonDocument doc(root);
    QHttpServerResponse result("application/opds+json; charset=utf-8"_ba, doc.toJson(QJsonDocument::Compact));
    return result;
}

QHttpServerResponse opds_server::authorSeriesHTML(uint idLib, uint idAuthor, uint idSeries, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u""_s, &sLibUrl);
    if(pLib == nullptr || !pLib->authors.contains(idAuthor) || !pLib->series.contains(idSeries))
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sLanguageFilter = urlquery.queryItemValue(u"lang"_s);

    std::vector<uint> vBooks = listBooks(*pLib, idAuthor, idSeries, sLanguageFilter, false);
    return generatePageHTML(vBooks, *pLib, tr("Books of series") % u" ("_s % pLib->series[idSeries].sName % u")"_s, sLibUrl, url, false);
}

QHttpServerResponse opds_server::authorSeriesOPDS(uint idLib, uint idAuthor, uint idSeries, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u"/opds"_s, &sLibUrl);
    if(pLib == nullptr || !pLib->authors.contains(idAuthor) || !pLib->series.contains(idSeries)) [[unlikely]]
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
    std::vector<uint> vBooks = listBooks(*pLib, idAuthor, idSeries, sLanguageFilter_, false);
    QString sPage = generatePageOPDS(vBooks, *pLib, tr("Books of series") % u" ("_s % pLib->series[idSeries].sName % u")"_s,
                                 u"tag:author:"_s % QString::number(idAuthor) % u":series:"_s % QString::number(idSeries), sLibUrl, url);
    QHttpServerResponse result(sPage);
    return result;
}

QHttpServerResponse opds_server::authorSeriesOPDS2(uint idLib, uint idAuthor, uint idSeries, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u"/opds2"_s, &sLibUrl);
    if(pLib == nullptr || !pLib->authors.contains(idAuthor) || !pLib->series.contains(idSeries)) [[unlikely]]
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sLanguageFilter = urlquery.queryItemValue(u"lang"_s);

    std::vector<uint> vBooks = listBooks(*pLib, idAuthor, idSeries, sLanguageFilter, false);
    QHttpServerResponse result = generatePageOPDS2(vBooks, *pLib, tr("Books of series") % u" ("_s % pLib->series[idSeries].sName % u")"_s, sLibUrl, url);
    return result;
}

QHttpServerResponse opds_server::authorSerieslessHTML(uint idLib, uint idAuthor, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u""_s, &sLibUrl);
    if(pLib == nullptr || !pLib->authors.contains(idAuthor)) [[unlikely]]
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
    QUrlQuery urlquery(url);
    QString sLangFilter = urlquery.queryItemValue(u"lang"_s);

    std::vector<uint> vBooks = listBooks(*pLib, idAuthor, 0, sLangFilter, true);
    return generatePageHTML(vBooks, *pLib, tr("Books without series") % u" ("_s % pLib->authors[idAuthor].getName() % u")"_s, sLibUrl, url, false);
}

QHttpServerResponse opds_server::authorSerieslessOPDS(uint idLib, uint idAuthor, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u"/opds"_s, &sLibUrl);
    if(pLib == nullptr || !pLib->authors.contains(idAuthor)) [[unlikely]]
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
    QUrlQuery urlquery(url);
    QString sLangFilter = urlquery.queryItemValue(u"lang"_s);

    std::vector<uint> vBooks = listBooks(*pLib, idAuthor, 0, sLangFilter, true);
    QString sPage = generatePageOPDS(vBooks, *pLib, tr("Books without series") % u" ("_s % pLib->authors[idAuthor].getName() % u")"_s,
                                 u"tag:author:"_s % QString::number(idAuthor) % u":seriesless"_s, sLibUrl, url);
    QHttpServerResponse result(sPage);
    return result;
}

QHttpServerResponse opds_server::authorSerieslessOPDS2(uint idLib, uint idAuthor, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u"/opds2"_s, &sLibUrl);
    if(pLib == nullptr || !pLib->authors.contains(idAuthor)) [[unlikely]]
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
    QUrlQuery urlquery(url);
    QString sLangFilter = urlquery.queryItemValue(u"lang"_s);

    std::vector<uint> vBooks = listBooks(*pLib, idAuthor, 0, sLangFilter, true);
    QHttpServerResponse result = generatePageOPDS2(vBooks, *pLib, tr("Books without series") % u" ("_s % pLib->authors[idAuthor].getName() % u")"_s, sLibUrl, url);
    return result;
}

QHttpServerResponse opds_server::seriesIndexHTML(uint idLib, const QString &sIndex, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u""_s, &sLibUrl);
    if(pLib == nullptr) [[unlikely]]
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sLanguageFilter = urlquery.queryItemValue(u"lang"_s);
    QString sQuery = query(urlquery);

    QDomElement feed;
    feed = docHeaderHTML(*pLib, urlquery, sLibUrl);
    std::unordered_map<QString, int> mCount;
    std::vector<QString> vIndex;
    std::unordered_set<QString> setSeries;
    std::unordered_set<uint>setIdSeries;
    std::unordered_map<QString, uint> mIdIndex;

    int count = 0;
    QString sLowerIndex = sIndex.toCaseFolded();
    auto seriesIds = pLib->books | std::views::values |
    std::views::filter([&](const auto& book) {
        return !book.bDeleted &&  !book.series.empty() &&
                (sLanguageFilter.isEmpty() || sLanguageFilter == pLib->vLaguages[book.idLanguage]);
    }) |
    std::views::transform([](const auto& book) { return book.series | std::views::keys; }) |
    std::views::join;
    std::ranges::copy(seriesIds, std::inserter(setIdSeries, setIdSeries.end()));

    for(auto &idSeries :setIdSeries){
        const auto &series = pLib->series.at(idSeries);
        if(series.sName.left(sIndex.length()).toCaseFolded() == sLowerIndex){
            count++;
            QString sNewIndex = series.sName.left(sIndex.length()+1).toCaseFolded();
            sNewIndex[0] = sNewIndex[0].toUpper();
            ++mCount[sNewIndex];
            if(!mIdIndex.contains(sNewIndex)){
                mIdIndex[sNewIndex] = idSeries;
                vIndex.push_back(sNewIndex);
            }
            if(sNewIndex.length() == series.sName.length())
                setSeries.insert(sNewIndex);
        }
    }

    if(count > 30)
    {
        attachSearchFormHTML(feed, tr("Finding series") + u": "_s, sLibUrl + u"/searchseries"_s, sIndex, urlquery);
        auto divGrid = addNode(feed, u"div"_s, u"griditems"_s);
        std::ranges::sort(vIndex, [](const auto &s1, const auto &s2){return localeStringCompare(s1, s2);});

        for(const auto &sIndex :vIndex)
        {
            QString sHref;
            auto nCount = mCount.at(sIndex);
            if(nCount == 1)
            {
                auto idSeries = mIdIndex[sIndex];
                sHref = sLibUrl % u"/seriesbooks/"_s % QString::number(idSeries) % sQuery;
            }else{
                sHref = sLibUrl % u"/seriesindex/"_s % QString::fromUtf8(QUrl::toPercentEncoding(sIndex, ""_ba, "."_ba)) %
                        (setSeries.contains(sIndex) && nCount<30 ?u"/books"_s :u""_s) % sQuery;

            }
            auto div = addNode(divGrid, u"div"_s, u"item"_s);
            addHRefNode(div, sIndex, sHref, u"block"_s);
            addTextNode(div, u"div"_s, QString::number(nCount) % u" "_s % tr("series starting with") % u" '"_s % sIndex % u"'"_s);

        }
    }
    else
    {
        std::vector<uint> vSeriesId;
        auto filtered = setIdSeries | std::views::filter([&](uint idSeries) {
            const auto& series = pLib->series.at(idSeries);
            return series.sName.left(sIndex.length()).toCaseFolded() == sLowerIndex;
        });
        std::ranges::copy(filtered, std::back_inserter(vSeriesId));
        std::ranges::sort(vSeriesId,[pLib](uint id1, uint id2) {return localeStringCompare(pLib->series.at(id1).sName, pLib->series.at(id2).sName);});
        auto divList = addNode(feed, u"div"_s, u"listitems"_s);

        for(auto idSeries: vSeriesId)
        {
            uint nBooksCount = std::ranges::count_if(pLib->books | std::views::values, [pLib, idSeries, &sLanguageFilter](const auto& book) {
                return !book.bDeleted && (sLanguageFilter.isEmpty() || sLanguageFilter == pLib->vLaguages[book.idLanguage]) && book.series.contains(idSeries);
            });
            QDomElement div = addNode(divList, u"div"_s, u"item"_s);
            addHRefNode(div, pLib->series.at(idSeries).sName, sLibUrl % u"/seriesbooks/"_s % QString::number(idSeries) % sQuery, u"block"_s);
            addTextNode(div, u"div"_s, QString::number(nBooksCount) % u" "_s % tr("books"));
        }
    }

    return responseHTML();
}

QHttpServerResponse opds_server::seriesIndexOPDS(uint idLib, const QString &sIndex, const QHttpServerRequest &request)
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
    QString sSessionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    QDomElement feed = docHeaderOPDS(tr("Books by series"), u"tag:root:series"_s, sLibUrl, sSession);

    std::unordered_map<QString, int> mCount;
    std::vector<QString> vIndex;
    std::unordered_set<QString> setSeries;
    std::unordered_set<uint>setIdSeries;
    std::unordered_map<QString, uint> mIdIndex;

    int count = 0;
    QString sLowerIndex = sIndex.toCaseFolded();

    for(const auto &iBook :pLib->books){
        const auto &book = iBook.second;
        if(book.bDeleted) [[unlikely]]
            continue;
        if(!book.series.empty()){
            if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == pLib->vLaguages[book.idLanguage])
                for(const auto &iSeries :book.series)
                    setIdSeries.insert(iSeries.first);
        }
    }
    for(auto &idSeries :setIdSeries){
        const auto &series = pLib->series.at(idSeries);
        if(series.sName.left(sIndex.length()).toCaseFolded() == sLowerIndex){
            count++;
            QString sNewIndex = series.sName.left(sIndex.length()+1).toCaseFolded();
            sNewIndex[0] = sNewIndex[0].toUpper();
            ++mCount[sNewIndex];
            if(!mIdIndex.contains(sNewIndex)){
                mIdIndex[sNewIndex] = idSeries;
                vIndex.push_back(sNewIndex);
            }
            if(sNewIndex.length() == series.sName.length())
                setSeries.insert(sNewIndex);
        }
    }

    if(count > 30)
    {
        std::ranges::sort(vIndex, [](const auto &s1, const auto &s2){return localeStringCompare(s1, s2);});

        for(const auto &sIndex :vIndex)
        {
            QString sHref;
            auto nCount = mCount.at(sIndex);
            QString sContent = QString::number(nCount) % u" "_s % tr("series starting with") % u" '"_s % sIndex % u"'"_s;
            if(nCount == 1)
            {
                auto idSeries = mIdIndex[sIndex];
                sHref = sLibUrl % u"/seriesbooks/"_s % QString::number(idSeries) % sSessionQuery;
            }else{
                sHref = sLibUrl % u"/seriesindex/"_s %  QString::fromUtf8(QUrl::toPercentEncoding(sIndex, ""_ba, "."_ba).constData())
                % (setSeries.contains(sIndex) ?u"/books"_s :u""_s) % sSessionQuery;
            }
            addEntry(feed, u"tag:series:"_s + sIndex, sHref, sIndex, sContent);
        }
    }
    else
    {
        std::vector<uint> vSeriesId;
        for(auto &idSeries :setIdSeries)
        {
            const auto &series = pLib->series.at(idSeries);
            if(series.sName.left((sIndex.length())).toCaseFolded() == sLowerIndex)
            {
                vSeriesId.push_back(idSeries);
            }
        }
        std::ranges::sort(vSeriesId, [pLib](uint id1, uint id2)
                          {return localeStringCompare(pLib->series.at(id1).sName, pLib->series.at(id2).sName);});

        for(auto idSeries: vSeriesId)
        {
            uint nBooksCount = std::ranges::count_if(pLib->books | std::views::values, [pLib, idSeries, this](const auto& book) {
                return !book.bDeleted && (sLanguageFilter_.isEmpty() || sLanguageFilter_ == pLib->vLaguages[book.idLanguage]) && book.series.contains(idSeries);
            });

            addEntry(feed, u"tag:series:"_s + QString::number(idSeries), sLibUrl % u"/seriesbooks/"_s % QString::number(idSeries) % sSessionQuery,
                     pLib->series.at(idSeries).sName, QString::number(nBooksCount) % u" "_s % tr("books"));
        }
    }
    QHttpServerResponse result(doc_.toString());
    return result;
}

QHttpServerResponse opds_server::seriesIndexOPDS2(uint idLib, const QString &sIndex, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u"/opds2"_s, &sLibUrl);
    if(pLib == nullptr)
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sQuery = query(urlquery);
    QString sLanguageFilter = urlquery.queryItemValue(u"lang"_s);

    auto root = docHeaderOPDS2(*pLib, tr("Books by series"), sLibUrl, url);

    std::unordered_map<QString, int> mCount;
    std::vector<QString> vIndex;
    std::unordered_set<QString> setSeries;
    std::unordered_set<uint>setIdSeries;
    std::unordered_map<QString, uint> mIdIndex;
    int count = 0;
    QString sLowerIndex = sIndex.toCaseFolded();

    for(const auto &iBook :pLib->books){
        const auto &book = iBook.second;
        if(book.bDeleted) [[unlikely]]
            continue;
        if(!book.series.empty()){
            if(sLanguageFilter.isEmpty() || sLanguageFilter == pLib->vLaguages[book.idLanguage])
                for(const auto &iSeries :book.series)
                    setIdSeries.insert(iSeries.first);
        }
    }
    for(auto &idSeries :setIdSeries){
        const auto &series = pLib->series.at(idSeries);
        if(series.sName.left(sIndex.length()).toCaseFolded() == sLowerIndex){
            count++;
            QString sNewIndex = series.sName.left(sIndex.length()+1).toCaseFolded();
            sNewIndex[0] = sNewIndex[0].toUpper();
            ++mCount[sNewIndex];
            if(!mIdIndex.contains(sNewIndex)){
                mIdIndex[sNewIndex] = idSeries;
                vIndex.push_back(sNewIndex);
            }
            if(sNewIndex.length() == series.sName.length())
                setSeries.insert(sNewIndex);
        }
    }

    QJsonArray navigation;
    if(count > 30)
    {
        std::ranges::sort(vIndex, [](const auto &s1, const auto &s2){return localeStringCompare(s1, s2);});
        for(const auto &sIndex :vIndex)
        {
            QString sHref;
            auto nCount = mCount.at(sIndex);
            if(nCount == 1)
            {
                auto idSeries = mIdIndex[sIndex];
                sHref = sLibUrl % u"/seriesbooks/"_s % QString::number(idSeries) % sQuery;
            }else{
                sHref = sLibUrl % u"/seriesindex/"_s %  QString::fromUtf8(QUrl::toPercentEncoding(sIndex, ""_ba, "."_ba))
                             % (setSeries.contains(sIndex) ?u"/books"_s :u""_s) % sQuery;

            }
            addNavigation(navigation, sIndex, sHref, nCount);

        }
    }
    else
    {
        std::vector<uint> vSeriesId;
        auto filtered = setIdSeries | std::views::filter([&](uint idSeries) {
            const auto& series = pLib->series.at(idSeries);
            return series.sName.left(sIndex.length()).toCaseFolded() == sLowerIndex;
        });
        std::ranges::copy(filtered, std::back_inserter(vSeriesId));
        std::ranges::sort(vSeriesId, [pLib](uint id1, uint id2) {return localeStringCompare(pLib->series.at(id1).sName, pLib->series.at(id2).sName);});

        for(auto idSeries: vSeriesId)
        {
            uint nBooksCount = std::ranges::count_if(pLib->books | std::views::values, [&](const auto& book) {
                return !book.bDeleted && (sLanguageFilter.isEmpty() || sLanguageFilter == pLib->vLaguages[book.idLanguage]) && book.series.contains(idSeries);
            });

            addNavigation(navigation, pLib->series.at(idSeries).sName, sLibUrl % u"/seriesbooks/"_s % QString::number(idSeries) % sQuery, nBooksCount);
        }
    }
    root[u"navigation"] = navigation;

    QJsonDocument doc(root);
    QHttpServerResponse result("application/opds+json; charset=utf-8"_ba, doc.toJson(QJsonDocument::Compact));
    return result;
}

QHttpServerResponse opds_server::seriesBooksHTML(uint idLib, uint idSeries, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u""_s, &sLibUrl);
    if(pLib == nullptr || !pLib->series.contains(idSeries))
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
    QUrlQuery urlquery(url);
    QString sLanguageFilter = urlquery.queryItemValue(u"lang"_s);

    std::vector<uint> vBooks = listBooks(*pLib, 0, idSeries, sLanguageFilter, false);
    return generatePageHTML(vBooks, *pLib, tr("Books of series") % u" ("_s % pLib->series[idSeries].sName % u")"_s, sLibUrl, url, true);
}

QHttpServerResponse opds_server::seriesBooksOPDS(uint idLib, uint idSeries, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u"/opds"_s, &sLibUrl);
    if(pLib == nullptr || !pLib->series.contains(idSeries))
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    std::vector<uint> vBooks = listBooks(*pLib, 0, idSeries, sLanguageFilter_, false);
    QString sPage = generatePageOPDS(vBooks, *pLib, tr("Books of series") % u" ("_s % pLib->series[idSeries].sName % u")"_s, u"tag:series:"_s + QString::number(idSeries), sLibUrl, url);
    QHttpServerResponse result(sPage);
    return result;
}

QHttpServerResponse opds_server::seriesBooksOPDS2(uint idLib, uint idSeries, const QHttpServerRequest &request)
{   
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u"/opds2"_s, &sLibUrl);
    if(pLib == nullptr || !pLib->series.contains(idSeries)) [[unlikely]]
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
    QUrlQuery urlquery(url);
    QString sLanguageFilter = urlquery.queryItemValue(u"lang"_s);

    std::vector<uint> vBooks = listBooks(*pLib, 0, idSeries, sLanguageFilter, false);
    QHttpServerResponse result = generatePageOPDS2(vBooks, *pLib, tr("Books of series") % u" ("_s % pLib->series[idSeries].sName % u")"_s, sLibUrl, url);
    return result;
}

QHttpServerResponse opds_server::bookHTML(uint idLib, uint idBook, const QString &sFormat)
{
    return convert(idLib, idBook, sFormat, false);
}

QHttpServerResponse opds_server::bookOPDS(uint idLib, uint idBook, const QString &sFormat)
{
    return convert(idLib, idBook, sFormat, true);
}

std::unordered_map<ushort, uint> opds_server::countBooksByGenre(const SLib &lib, ushort idParentGenre, const QString &sLanguageFilter)
{
    std::unordered_map<ushort, uint> mCounts;
    for(const auto &iBook :lib.books){
        if(!iBook.second.bDeleted) [[likely]]{
            for(auto idGenre: iBook.second.vIdGenres){
                if(g::genres.at(idGenre).idParrentGenre == idParentGenre){
                    if(sLanguageFilter.isEmpty() || sLanguageFilter == lib.vLaguages[iBook.second.idLanguage])
                        ++mCounts[idGenre];
                }
            }
        }
    }
    return mCounts;
}

QHttpServerResponse opds_server::genresHTML(uint idLib, ushort idParentGenre, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u""_s, &sLibUrl);
    if(pLib == nullptr || (idParentGenre!=0 && !g::genres.contains(idParentGenre))) [[unlikely]]
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sLanguageFilter = urlquery.queryItemValue(u"lang"_s);
    QString sQuery = query(urlquery);

    Q_ASSERT(!pLib->series.contains(0));
    std::vector<ushort> vIdGenres;
    std::unordered_map<ushort, uint> mCounts;

    if(idParentGenre != 0)
    {
        if(g::genres[idParentGenre].idParrentGenre > 0){
            std::vector<uint> vBooks = listGenreBooks(*pLib, idParentGenre, sLanguageFilter);
            return generatePageHTML(vBooks, *pLib, tr("Books by genre") % u": " % g::genres[idParentGenre].sName, sLibUrl, url, true);
        }

        mCounts = countBooksByGenre(*pLib, idParentGenre, sLanguageFilter);
        vIdGenres.reserve(mCounts.size());
        std::ranges::copy(mCounts | std::views::keys,  std::back_inserter(vIdGenres));
    }
    else
    {
        auto filtered = g::genres | std::views::filter([](const auto& iGenre) {return iGenre.second.idParrentGenre == 0;}) | std::views::keys;
        vIdGenres.assign(filtered.begin(), filtered.end());
    }
    std::ranges::sort(vIdGenres, [&](ushort id1, ushort id2){return g::genres.at(id1).sName < g::genres.at(id2).sName;});
    QDomElement feed;
    feed = docHeaderHTML(*pLib, urlquery, sLibUrl);
    auto divList = addNode(feed, u"div"_s, u"listitems"_s);

    for(auto idGenre: vIdGenres)
    {
        uint nCount = mCounts[idGenre];
        QDomElement div = addNode(divList, u"div"_s, u"item"_s);
        addHRefNode(div, g::genres[idGenre].sName, sLibUrl % u"/genres/"_s % QString::number(idGenre) % sQuery, u"block"_s);
        if(idParentGenre != 0)
            addTextNode(div, u"div"_s, QString::number(nCount) % u" "_s % tr("books"));
    }

    return responseHTML();
}

QHttpServerResponse opds_server::genresOPDS(uint idLib, ushort idParentGenre, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u"/opds"_s, &sLibUrl);
    if(pLib == nullptr || (idParentGenre!=0 && !g::genres.contains(idParentGenre))) [[unlikely]]
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sQuery = query(urlquery);
    QString sSession = urlquery.queryItemValue(u"session"_s);

    std::vector<ushort> vIdGenres;
    std::unordered_map<ushort, uint> mCounts;
    if(idParentGenre != 0)
    {
        if(g::genres[idParentGenre].idParrentGenre > 0){
            std::vector<uint> vBooks = listGenreBooks(*pLib, idParentGenre, sLanguageFilter_);
            QString sPage =  generatePageOPDS(vBooks, *pLib, tr("Books by genre") % u": " % g::genres[idParentGenre].sName, u""_s, sLibUrl, url);
            QHttpServerResponse result(sPage);
            return result;
        }

        mCounts = countBooksByGenre(*pLib, idParentGenre, sLanguageFilter_);
        vIdGenres.reserve(mCounts.size());
        std::ranges::copy(mCounts | std::views::keys, std::back_inserter(vIdGenres));
    }
    else
    {
        auto filtered = g::genres | std::views::filter([](const auto& iGenre) {return iGenre.second.idParrentGenre == 0;}) | std::views::keys;
        vIdGenres.assign(filtered.begin(), filtered.end());
    }

    std::ranges::sort(vIdGenres, [&](ushort id1, ushort id2){return g::genres.at(id1).sName < g::genres.at(id2).sName;});
    QDomElement feed;
    feed = docHeaderOPDS(tr("Books by genre"), u"tag:root:genre"_s, sLibUrl, sSession);

    for(auto idGenre: vIdGenres)
    {
        QString sContent;
        if(idParentGenre != 0)
            sContent = QString::number(mCounts[idGenre]) % u" "_s % tr("books");
        else
            sContent =  tr("Books of genre") % u" "_s % g::genres[idGenre].sName;
        addEntry(feed, u"tag:root:genre:"_s + g::genres[idGenre].sName, sLibUrl % u"/genres/"_s % QString::number(idGenre) % sQuery,
                 g::genres[idGenre].sName, sContent);
    }

    QHttpServerResponse result(doc_.toString());
    return result;
}

QHttpServerResponse opds_server::genresOPDS2(uint idLib, ushort idParentGenre, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u"/opds2"_s, &sLibUrl);
    if(pLib == nullptr || (idParentGenre!=0 && !g::genres.contains(idParentGenre))) [[unlikely]]
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sLanguageFilter = urlquery.queryItemValue(u"lang"_s);
    QString sQuery = query(urlquery);

    std::vector<ushort> vIdGenres;
    std::unordered_map<ushort, uint> mCounts;
    if(idParentGenre != 0)
    {
        if(g::genres[idParentGenre].idParrentGenre > 0){
            std::vector<uint> vBooks = listGenreBooks(*pLib, idParentGenre, sLanguageFilter);
            QHttpServerResponse result = generatePageOPDS2(vBooks, *pLib, tr("Books by genre") % u": " % g::genres[idParentGenre].sName, sLibUrl, url);
            return result;
        }
        mCounts = countBooksByGenre(*pLib, idParentGenre, sLanguageFilter);
        vIdGenres.reserve(mCounts.size());
        std::ranges::copy(mCounts | std::views::keys, std::back_inserter(vIdGenres));
    }
    else
    {
        auto filtered = g::genres | std::views::filter([](const auto& iGenre) {return iGenre.second.idParrentGenre == 0;}) | std::views::keys;
        vIdGenres.assign(filtered.begin(), filtered.end());
    }

    std::ranges::sort(vIdGenres, [&](ushort id1, ushort id2){return g::genres.at(id1).sName < g::genres.at(id2).sName;});
    QJsonObject root = docHeaderOPDS2(*pLib, tr("Books by genre"), sLibUrl, url);

    QJsonArray navigation;
    for(auto idGenre: vIdGenres)
    {
        uint nCount = idParentGenre != 0 ?mCounts[idGenre] :0;
        addNavigation(navigation, g::genres[idGenre].sName, sLibUrl % u"/genres/"_s % QString::number(idGenre) % sQuery, nCount);
    }
    root[u"navigation"] = navigation;

    QJsonDocument doc(root);
    QHttpServerResponse result("application/opds+json; charset=utf-8"_ba, doc.toJson(QJsonDocument::Compact));
    return result;
}

QHttpServerResponse opds_server::searchTitleHTML(uint idLib, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u""_s, &sLibUrl);
    if(pLib == nullptr) [[unlikely]]
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sSearchString = urlquery.queryItemValue(u"search_string"_s, QUrl::FullyDecoded).replace(u'+', u' ');
    QString sLanguageFilter = urlquery.queryItemValue(u"lang"_s);

    std::vector<uint> vBooks = searchTitle(*pLib, sSearchString, sLanguageFilter);
    QDomElement feed;
    feed = docHeaderHTML(*pLib, urlquery, sLibUrl);
    attachSearchFormHTML(feed, tr("Finding books by title") + u": "_s, sLibUrl + u"/searchtitle"_s, sSearchString, urlquery);

    fillPageHTML(vBooks, *pLib, feed, sLibUrl, url, true);
    return responseHTML();
}

QHttpServerResponse opds_server::searchAuthorHTML(uint idLib, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u""_s, &sLibUrl);
    if(pLib == nullptr) [[unlikely]]
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sSearchString = urlquery.queryItemValue(u"search_string"_s, QUrl::FullyDecoded).replace(u'+', u' ');
    QString sQuery = query(urlquery);
    QString sLanguageFilter = urlquery.queryItemValue(u"lang"_s);

    auto vAuthors = searchAuthors(*pLib, sSearchString);
    sort(vAuthors, [pLib](uint id1, uint id2)
              { return localeStringCompare(pLib->authors.at(id1).getName(), pLib->authors.at(id2).getName()); });

    QDomElement feed;
    feed = docHeaderHTML(*pLib, urlquery, sLibUrl);
    attachSearchFormHTML(feed, tr("Finding authors") + u": "_s, sLibUrl + u"/searchauthor"_s, sSearchString, urlquery);
    auto divList = addNode(feed, u"div"_s, u"listitems"_s);

    for(auto idAuthor :vAuthors){
        uint nBooksCount = 0;
        auto [begin, end] = pLib->authorBooksLink.equal_range(idAuthor);
        for (auto it = begin; it != end; ++it) {
            auto &book = pLib->books.at(it->second);
            if(!book.bDeleted) [[likely]]
                if(sLanguageFilter.isEmpty() || sLanguageFilter == pLib->vLaguages[book.idLanguage])
                    nBooksCount++;
        }
        if(nBooksCount >0 ){
            QDomElement div = addNode(divList, u"div"_s, u"item"_s);
            addHRefNode(div, pLib->authors.at(idAuthor).getName(), sLibUrl % u"/author/"_s % QString::number(idAuthor) % sQuery, u"block"_s);
            addTextNode(div, u"div"_s, QString::number(nBooksCount) % u" "_s % tr("books"));
        }
    }

    return responseHTML();
}

QHttpServerResponse opds_server::searchSeriesHTML(uint idLib, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u""_s, &sLibUrl);
    if(pLib == nullptr) [[unlikely]]
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sLanguageFilter = urlquery.queryItemValue(u"lang"_s);
    QString sSearchString = urlquery.queryItemValue(u"search_string"_s, QUrl::FullyDecoded).replace(u'+', u' ');
    QString sQuery = query(urlquery);

    QDomElement feed;
    feed = docHeaderHTML(*pLib, urlquery, sLibUrl);
    attachSearchFormHTML(feed, tr("Finding series") + u": "_s, sLibUrl + u"/searchseries"_s, sSearchString, urlquery);

    auto mSeries  = searchSeries(*pLib, sSearchString, sLanguageFilter);
    auto divList = addNode(feed, u"div"_s, u"listitems"_s);
    for(auto [idSeries, count]: mSeries){
        QDomElement div = addNode(divList, u"div"_s, u"item"_s);
        addHRefNode(div, pLib->series.at(idSeries).sName, sLibUrl % u"/seriesbooks/"_s % QString::number(idSeries) % sQuery, u"block"_s);
        addTextNode(div, u"div"_s, QString::number(count) % u" "_s % tr("books"));
    }

    return responseHTML();
}

QHttpServerResponse opds_server::searchOPDS(uint idLib, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QUrlQuery urlquery(url);
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u"/opds"_s, &sLibUrl);
    if(pLib == nullptr) [[unlikely]]
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QString sSearchString = urlquery.queryItemValue(u"q"_s, QUrl::FullyDecoded).replace(u'+', u' ');
    QString sSearchAuthor = urlquery.queryItemValue(u"author"_s, QUrl::FullyDecoded).replace(u'+', u' ');
    if(sSearchAuthor.startsWith(u"{atom:author}"))
        sSearchAuthor = u""_s;
    QString sSearchTitle = urlquery.queryItemValue(u"title"_s, QUrl::FullyDecoded).replace(u'+', u' ');
    if(sSearchTitle.startsWith(u"{atom:title}"))
        sSearchTitle = u""_s;
    QString sSeries = urlquery.queryItemValue(u"series"_s, QUrl::FullyDecoded).replace(u'+', u' ');

    if(!sSearchString.isEmpty()){
        QString sSession = urlquery.queryItemValue(u"session"_s);
        QDomElement feed = docHeaderOPDS(pLib->name, u"tag:root"_s, sLibUrl, sSession);

        QString sHRef = sLibUrl % u"/search?author="_s % sSearchString;
        if(!sSession.isEmpty())
            sHRef += u"&session="_s + sSession;
        addEntry(feed,  u"tag:search:authors"_s, sHRef, tr("Finding authors"), u""_s);

        sHRef = sLibUrl % u"/search?title="_s % sSearchString;
        if(!sSession.isEmpty())
            sHRef += u"&session="_s + sSession;
        addEntry(feed, u"tag:search:title"_s, sHRef, tr("Finding books by title"), u""_s);

        sHRef = sLibUrl % u"/search?series="_s % sSearchString;
        if(!sSession.isEmpty())
            sHRef += u"&session="_s + sSession;
        addEntry(feed, u"tag:search:series"_s, sHRef, tr("Finding series"), u""_s);

        QHttpServerResponse result("application/atom+xml;charset=utf-8"_ba, doc_.toByteArray());
        return result;
    }
    if(!sSearchAuthor.isEmpty()){
        sSearchAuthor.replace(u'+', u' ');
        auto vAuthors = searchAuthors(*pLib, sSearchAuthor);
        sort(vAuthors, [pLib](uint id1, uint id2)
                  { return localeStringCompare(pLib->authors.at(id1).getName(), pLib->authors.at(id2).getName()) /*< 0*/; });

        QString sSession = urlquery.queryItemValue(u"session"_s);
        QString sSessionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;
        QDomElement feed = docHeaderOPDS(tr("Finding authors"), u"tag:search:authors"_s, sLibUrl, sSession);
        for(auto idAuthor :vAuthors){
            uint nBooksCount = 0;
            auto [begin, end] = pLib->authorBooksLink.equal_range(idAuthor);
            for (auto it = begin; it != end; ++it) {
                auto &book = pLib->books.at(it->second);
                if(!book.bDeleted) [[likely]]
                    if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == pLib->vLaguages[book.idLanguage])
                        nBooksCount++;
            }
            if(nBooksCount >0 ){
                addEntry(feed, u"tag:search:authors:"_s + QString::number(idAuthor), sLibUrl % u"/author/"_s % QString::number(idAuthor) % sSessionQuery,
                         pLib->authors.at(idAuthor).getName(),  QString::number(nBooksCount) % u" "_s % tr("books"));
            }
        }
        QHttpServerResponse result("application/atom+xml;charset=utf-8"_ba, doc_.toByteArray());
        return result;
    }

    if(!sSearchTitle.isEmpty()){
        auto vBooks = searchTitle(*pLib, sSearchTitle, sLanguageFilter_);
        return generatePageOPDS(vBooks, *pLib, tr("Books search"), u""_s, sLibUrl, url);
    }

    if(!sSeries.isEmpty()){
        auto mSeries  = searchSeries(*pLib, sSeries, sLanguageFilter_);
        QString sSession = urlquery.queryItemValue(u"session"_s);
        QString sSessionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;
        QDomElement feed = docHeaderOPDS(tr("Finding series"), u"tag:search:series"_s, sLibUrl, sSession);

        for(auto [idSeries, count]: mSeries)
        {
            addEntry(feed, u"tag:series:"_s + QString::number(idSeries), sLibUrl % u"/seriesbooks/"_s % QString::number(idSeries) % sSessionQuery,
                     pLib->series.at(idSeries).sName, QString::number(count) % u" "_s % tr("books"));
        }
        QHttpServerResponse result("application/atom+xml;charset=utf-8"_ba, doc_.toByteArray());
        return result;
    }
    return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
}


QHttpServerResponse opds_server::searchOPDS2(uint idLib, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u"/opds2"_s, &sLibUrl);
    if(pLib == nullptr) [[unlikely]]
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
    QUrlQuery urlquery(url);
    QString sQuery = query(urlquery);
    QString sLanguageFilter = urlquery.queryItemValue(u"lang"_s);

    QString sSearchString = urlquery.queryItemValue(u"query"_s, QUrl::FullyDecoded).replace(u'+', u' ');
    QString sSearchAuthor = urlquery.queryItemValue(u"author"_s, QUrl::FullyDecoded).replace(u'+', u' ');
    QString sSearchTitle = urlquery.queryItemValue(u"title"_s, QUrl::FullyDecoded).replace(u'+', u' ');
    QString sSeries = urlquery.queryItemValue(u"series"_s, QUrl::FullyDecoded).replace(u'+', u' ');

    if(!sSearchString.isEmpty() && sSearchAuthor.isEmpty() && sSearchTitle.isEmpty()){
        QString sSession = urlquery.queryItemValue(u"session"_s);
        QJsonObject root = docHeaderOPDS2(*pLib, pLib->name, sLibUrl, url);

        QJsonArray navigation;
        urlquery.removeQueryItem(u"query"_s);
        auto querySearchAuthor = urlquery;
        setQueryItem(querySearchAuthor, u"author"_s, sSearchString);
        addNavigation(navigation, tr("Finding authors"), sLibUrl % u"/search?"_s % querySearchAuthor.toString());
        auto querySearchTitle = urlquery;
        setQueryItem(querySearchTitle, u"title"_s, sSearchString);
        addNavigation(navigation, tr("Finding books by title"), sLibUrl % u"/search?" % querySearchTitle.toString());
        auto querySearchSeries = urlquery;
        setQueryItem(querySearchSeries, u"series"_s, sSearchString);
        addNavigation(navigation, tr("Finding series"), sLibUrl % u"/search?" % querySearchSeries.toString());
        root[u"navigation"] = navigation;

        QJsonDocument doc(root);
        QHttpServerResponse result("application/opds+json; charset=utf-8"_ba, doc.toJson(QJsonDocument::Compact));
        return result;
    }

    std::vector<uint> vBooks;
    if(!sSearchTitle.isEmpty()){
        if(!sSearchAuthor.isEmpty()){
            vBooks = searchBooks(*pLib, sSearchAuthor, sSearchTitle, sLanguageFilter);
        }else
            vBooks = searchTitle(*pLib, sSearchTitle, sLanguageFilter);
        return generatePageOPDS2(vBooks, *pLib, tr("Books search"), sLibUrl, url);
    }

    if(!sSearchAuthor.isEmpty()){
        auto vAuthors = searchAuthors(*pLib, sSearchAuthor);
        sort(vAuthors, [pLib](uint id1, uint id2)
                  { return localeStringCompare(pLib->authors.at(id1).getName(), pLib->authors.at(id2).getName()) /*< 0*/; });
        QJsonObject root = docHeaderOPDS2(*pLib, tr("Finding authors"), sLibUrl, url);
        QJsonArray navigation;
        for(auto idAuthor :vAuthors){
            auto [begin, end] = pLib->authorBooksLink.equal_range(idAuthor);
            uint nBooksCount = std::count_if(begin, end, [&pLib, &sLanguageFilter](const auto& pair) {
                const auto& book = pLib->books.at(pair.second);
                return !book.bDeleted && (sLanguageFilter.isEmpty() || sLanguageFilter == pLib->vLaguages[book.idLanguage]);
            });
            if(nBooksCount >0 ) [[likely]]{
                addNavigation(navigation, pLib->authors.at(idAuthor).getName(), sLibUrl % u"/author/"_s % QString::number(idAuthor) % sQuery);
            }
            root[u"navigation"] = navigation;

        }
        QJsonDocument doc(root);
        QHttpServerResponse result("application/opds+json; charset=utf-8"_ba, doc.toJson(QJsonDocument::Compact));
        return result;
    }
    if(!sSeries.isEmpty()){
        auto mSeries  = searchSeries(*pLib, sSeries, sLanguageFilter);
        QJsonObject root = docHeaderOPDS2(*pLib, tr("Finding series"), sLibUrl, url);
        QJsonArray navigation;

        for(auto [idSeries, count]: mSeries)
            addNavigation(navigation, pLib->series.at(idSeries).sName, sLibUrl % u"/seriesbooks/"_s % QString::number(idSeries) % sQuery, count);

        root[u"navigation"] = navigation;

        QJsonDocument doc(root);
        QHttpServerResponse result("application/opds+json; charset=utf-8"_ba, doc.toJson(QJsonDocument::Compact));
        return result;
    }
    return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
}

QHttpServerResponse opds_server::convert(uint idLib, uint idBook, const QString &sFormat, bool opds)
{
    QByteArray baContentType;
    QString sContentDisposition;
    SLib *pLib = getLib(idLib, u""_s);
    if(pLib == nullptr || !pLib->books.contains(idBook))
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
    QByteArray baBook;
    BookFile bookFile(*pLib, idBook);
    auto &book = pLib->books[idBook];
    baBook = bookFile.data();

    ExportOptions *pExportOptions = nullptr;
    int count = g::options.vExportOptions.size();
    ExportFormat format;
    if(sFormat == u"fb2")
         format = fb2;
    else if(sFormat == u"epub")
        format = epub;
    else if(sFormat == u"azw3")
        format = azw3;
    else if(sFormat == u"mobi")
        format = mobi;
    else if(sFormat == u"mobi7")
        format = mobi7;
    else
        format = asis;

    for(int i=0; i<count; i++)
    {
        if((g::options.vExportOptions[i].format == format && g::options.vExportOptions[i].bUseForHttp) ||
            (sFormat == u"fb2"_s && g::options.vExportOptions[i].format == asis && pLib->books[idBook].sFormat == u"fb2"_s))
        {
            pExportOptions = &g::options.vExportOptions[i];
            break;
        }
    }

    if(baBook.size() != 0) [[likely]]
    {
        QString sBookFileName;
        if(pExportOptions != nullptr)
            sBookFileName = pExportOptions->sExportFileName;
        if(sBookFileName.isEmpty())
            sBookFileName = QString(ExportOptions::sDefaultEexpFileName);
        sBookFileName = pLib->fillParams(sBookFileName, idBook) % u"."_s % (sFormat == u"download"_s ? book.sFormat :sFormat);
        if(pExportOptions != nullptr && pExportOptions->bTransliteration)
            sBookFileName = Transliteration(sBookFileName);
        sBookFileName.replace(u' ', u'_');
        sBookFileName.replace(u'\"', u'_');
        sBookFileName.replace(u'\'', u'_');
        sBookFileName.replace(u',', u'_');
        sBookFileName.replace(u"__"_s, u"_"_s);
        QFileInfo book_file(sBookFileName);
        sBookFileName = book_file.fileName();
        if(pExportOptions != nullptr && pExportOptions->bOriginalFileName)
            sBookFileName = book.sFile % u"."_s % sFormat;;
        if(format == epub || format == mobi || format == azw3)
        {
            QString sBookFormat = book.sFormat;
            if(sBookFormat == u"epub.zip" || (sBookFormat == u"zip" && book.sName.endsWith(u"(epub)")))
                sBookFormat = u"epub"_s;
            if(sBookFormat != sFormat){
                QFile file;
                file.setFileName(QDir::tempPath() % u"/freeLib/book0."_s % sBookFormat);
                if(file.open(QFile::WriteOnly)){
                    file.write(baBook);
                    file.close();
                }
                QFileInfo fi(file);

                fb2mobi conv(pExportOptions, idLib);
                QString sOutFile = conv.convert({fi.absoluteFilePath()}, idBook);
                file.setFileName(sOutFile);
                if(file.open(QFile::ReadOnly))
                    baBook = file.readAll();
            }
        }
        switch (format) {
        case fb2:
            baContentType = "application/x-fictionbook+xml"_ba;
            break;
        case epub:
            baContentType = "application/epub+zip"_ba;
            break;
        case azw3:
            baContentType = "application/x-mobi8-ebook"_ba;
            break;
        case mobi:
            baContentType = "application/x-mobipocket-ebook"_ba;
            break;
        default:
            baContentType = mime(sFormat).toUtf8();
            break;
        }
        sContentDisposition = u"attachment; filename=\""_s % sBookFileName % u"\""_s;
    }
    QHttpServerResponse result(baBook);
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    QHttpHeaders headers = result.headers();;
    headers.replaceOrAppend("Content-Type"_ba, baContentType);
    if(!sContentDisposition.isEmpty())
        headers.append("Content-Disposition"_ba, sContentDisposition.toUtf8());
    result.setHeaders(std::move(headers));
#else
    result.setHeader("Content-Type"_ba, baContentType);
    if(!sContentDisposition.isEmpty())
        result.addHeader("Content-Disposition"_ba, sContentDisposition.toUtf8());
#endif
    return result;
}

void opds_server::server_run()
{
    if(g::options.nHttpPort != nPort_ && status_ == Status::run)
    {
        stop_server();
    }
    nPort_ = g::options.nHttpPort;
    if(g::options.bOpdsEnable)
    {
        if(status_ == Status::stoped)
        {
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
            if (!tcpServer_.listen(QHostAddress::Any, nPort_) || !httpServer_.bind(&tcpServer_))
                LogWarning << "Unable to start the server.";
            else
                status_ = Status::run;
#else
            if (!tcpServer_.listen(QHostAddress::Any, nPort_))
                LogWarning << "Unable to start the server.";
            else{
                httpServer_.bind(&tcpServer_);
                status_ = Status::run;
            }
#endif
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
