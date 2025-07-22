#define QT_USE_QSTRINGBUILDER
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

#define MAX_COLUMN_COUNT    3

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

struct SerialComparator {
    const std::unordered_map<uint, SSerial>& mSerial;

    SerialComparator(const std::unordered_map<uint, SSerial>& a) : mSerial(a) {}

    bool operator()(uint id1, uint id2) const {
        return localeStringCompare(mSerial.at(id1).sName, mSerial.at(id2).sName);
    }
};

opds_server::opds_server(QObject *parent) :
    QObject(parent)
{
    httpServer_.route(u"/<arg>"_s, QHttpServerRequest::Method::Get, [this](uint idLib, const QHttpServerRequest &request)
    {
        return rootHTML(idLib, request);
    });

    httpServer_.route(u"/opds/<arg>"_s, QHttpServerRequest::Method::Get, [this](uint idLib, const QHttpServerRequest &request)
    { return rootOPDS(idLib, request); });

    httpServer_.route(u"/opds2/<arg>"_s, QHttpServerRequest::Method::Get, [this](uint idLib, const QHttpServerRequest &request)
                      { return rootOPDS2(idLib, request); });

    httpServer_.route(u"/<arg>.png"_s, QHttpServerRequest::Method::Get, [](const QString &sUrl)
    {
        QString ico = u":/xsl/opds/"_s % sUrl % u".png"_s;
        QFile file(ico);
        file.open(QFile::ReadOnly);
        QByteArray ba = file.readAll();
        if(ba.isEmpty())
            return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
        QHttpServerResponse response("image/png"_ba, ba);
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
        QHttpHeaders headers;
        headers.append("Cache-Control"_ba,"max-age=3600"_ba);
        response.setHeaders(std::move(headers));
#else
        response.addHeader("Cache-Control"_ba,"max-age=3600"_ba);
#endif
        return response;
    });

    httpServer_.route(u"/<arg>.svg"_s, QHttpServerRequest::Method::Get, [](const QString &sUrl)
    {
        QString sFile = u":/xsl/opds/"_s % sUrl % u".svg"_s;
        QFile file(sFile);
        file.open(QFile::ReadOnly);
        QByteArray ba = file.readAll();
        if(ba.isEmpty())
            return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
        QHttpServerResponse response("image/svg+xml"_ba, ba);
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
        QHttpHeaders headers = response.headers();;
        headers.append("Cache-Control"_ba,"max-age=3600"_ba);
        response.setHeaders(std::move(headers));
#else
        response.addHeader("Cache-Control"_ba,"max-age=3600"_ba);
#endif
        return response;
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
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
        QHttpHeaders headers = response.headers();
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
        return authorsIndexHTML(idLib, u""_s, false, request);
    });

    httpServer_.route(u"/opds/<arg>/authorsindex"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QHttpServerRequest &request)
    { return authorsIndexOPDS(idLib, u""_s, false, request); });

    httpServer_.route(u"/opds2/<arg>/authorsindex"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QHttpServerRequest &request)
                      { return authorsIndexOPDS2(idLib, u""_s, false, request); });

    httpServer_.route(u"/<arg>/authorsindex/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return authorsIndexHTML(idLib, sIndex, false, request); });

    httpServer_.route(u"/opds/<arg>/authorsindex/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return authorsIndexOPDS(idLib, sIndex, false, request); });

    httpServer_.route(u"/opds2/<arg>/authorsindex/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
                      { return authorsIndexOPDS2(idLib, sIndex, false, request); });

    httpServer_.route(u"/<arg>/authorsindex/<arg>/books"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return authorsIndexHTML(idLib, sIndex, true, request); });

    httpServer_.route(u"/opds/<arg>/authorsindex/<arg>/books"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return authorsIndexOPDS(idLib, sIndex, true, request); });

    httpServer_.route(u"/opds2/<arg>/authorsindex/<arg>/books"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return authorsIndexOPDS2(idLib, sIndex, true, request); });

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

    httpServer_.route(u"/<arg>/authorsequences/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idAuthor, const QHttpServerRequest &request)
    { return authorSequencesHTML(idLib, idAuthor, request); });

    httpServer_.route(u"/opds/<arg>/authorsequences/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idAuthor, const QHttpServerRequest &request)
    { return authorSequencesOPDS(idLib, idAuthor, request); });

    httpServer_.route(u"/opds2/<arg>/authorsequences/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idAuthor, const QHttpServerRequest &request)
    { return authorSequencesOPDS2(idLib, idAuthor, request); });

    httpServer_.route(u"/<arg>/authorsequence/<arg>/<arg>"_s, QHttpServerRequest::Method::Get, [this](uint idLib, uint idAuthor, uint idSequence, const QHttpServerRequest &request)
    { return authorSequencesHTML(idLib, idAuthor, idSequence, request); });

    httpServer_.route(u"/opds/<arg>/authorsequence/<arg>/<arg>"_s, QHttpServerRequest::Method::Get, [this](uint idLib, uint idAuthor, uint idSequence, const QHttpServerRequest &request)
    { return authorSequencesOPDS(idLib, idAuthor, idSequence, request); });

    httpServer_.route(u"/opds2/<arg>/authorsequence/<arg>/<arg>"_s, QHttpServerRequest::Method::Get, [this](uint idLib, uint idAuthor, uint idSequence, const QHttpServerRequest &request)
    { return authorSequencesOPDS2(idLib, idAuthor, idSequence, request); });

    httpServer_.route(u"/<arg>/authorsequenceless/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idAuthor, const QHttpServerRequest &request)
    { return authorSequencelessHTML(idLib, idAuthor, request); });

    httpServer_.route(u"/opds/<arg>/authorsequenceless/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idAuthor, const QHttpServerRequest &request)
    { return authorSequencelessOPDS(idLib, idAuthor, request); });

    httpServer_.route(u"/opds2/<arg>/authorsequenceless/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idAuthor, const QHttpServerRequest &request)
    { return authorSequencelessOPDS2(idLib, idAuthor, request); });

    httpServer_.route(u"/<arg>/sequencesindex"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QHttpServerRequest &request)
    { return sequencesIndexHTML(idLib, u""_s, false, request); });

    httpServer_.route(u"/opds/<arg>/sequencesindex"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QHttpServerRequest &request)
    { return sequencesIndexOPDS(idLib, u""_s, false, request); });

    httpServer_.route(u"/opds2/<arg>/sequencesindex"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QHttpServerRequest &request)
    { return sequencesIndexOPDS2(idLib, u""_s, false, request); });

    httpServer_.route(u"/<arg>/sequencesindex/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return sequencesIndexHTML(idLib, sIndex, false, request); });

    httpServer_.route(u"/opds/<arg>/sequencesindex/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return sequencesIndexOPDS(idLib, sIndex, false, request); });

    httpServer_.route(u"/opds2/<arg>/sequencesindex/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return sequencesIndexOPDS2(idLib, sIndex, false, request); });

    httpServer_.route(u"/<arg>/sequencesindex/<arg>/books"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return sequencesIndexHTML(idLib, sIndex, true, request); });

    httpServer_.route(u"/opds/<arg>/sequencesindex/<arg>/books"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return sequencesIndexOPDS(idLib, sIndex, true, request); });

    httpServer_.route(u"/opds2/<arg>/sequencesindex/<arg>/books"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QString &sIndex, const QHttpServerRequest &request)
    { return sequencesIndexOPDS2(idLib, sIndex, true, request); });

    httpServer_.route(u"/<arg>/sequencebooks/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idSequence, const QHttpServerRequest &request)
    { return sequenceBooksHTML(idLib, idSequence, request); });

    httpServer_.route(u"/opds/<arg>/sequencebooks/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idSequence, const QHttpServerRequest &request)
    { return sequenceBooksOPDS(idLib, idSequence, request); });

    httpServer_.route(u"/opds2/<arg>/sequencebooks/<arg>"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, uint idSequence, const QHttpServerRequest &request)
    { return sequenceBooksOPDS2(idLib, idSequence, request); });

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

    httpServer_.route(u"/<arg>/searchsequence"_s, QHttpServerRequest::Method::Get,  [this](uint idLib, const QHttpServerRequest &request)
                      { return searchSequenceHTML(idLib, request); });

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

QDomElement opds_server::AddTextNode(const QString &name, const QString &text, QDomNode &node)
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

void opds_server::addHRefNode(QDomNode &node, const QString &sText, const QString &sHRef, const QString &sClass)
{
    QDomElement el = doc_.createElement(u"a"_s);
    node.appendChild(el);
    el.setAttribute(u"href"_s, sHRef);
    el.setAttribute(u"class"_s, sClass);
    QDomText txt = doc_.createTextNode(sText);
    el.appendChild(txt);
}

void opds_server::addDownloadItem(QDomNode &node, const QString &sText, const QString &sHRef)
{
    QDomElement el = doc_.createElement(u"a"_s);
    el.setAttribute(u"href"_s, sHRef);
    el.setAttribute(u"class"_s, u"download"_s);
    QDomElement img = doc_.createElement(u"img"_s);
    img.setAttribute(u"class"_s, u"download"_s);
    img.setAttribute(u"src"_s, u"/download.svg"_s);
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
    AddTextNode(u"updated"_s, QDateTime::currentDateTimeUtc().toString(Qt::ISODate), entry);
    AddTextNode(u"id"_s, sId, entry);
    AddTextNode(u"title"_s, sTitle, entry);
    if(!sContent.isEmpty()){
        QDomElement el = AddTextNode(u"content"_s, sContent, entry);
        el.setAttribute(u"type"_s, u"text"_s);
    }
    addLink(entry, u"application/atom+xml;profile=opds-catalog"_s, sHRef);
}

QString simplifySearchString(const QString &str)
{
    QString sSimplified = str;
    const static QRegularExpression simpl(u"[.,«»?!-\"'()]"_s);
    sSimplified = sSimplified.replace(simpl, u" "_s).simplified().toCaseFolded();

    return sSimplified;
}

std::vector<uint> opds_server::book_list(const SLib &lib, uint idAuthor, uint idSeria, bool sequenceless = false)
{
    std::vector<uint> vBooks;
    if(idAuthor !=0 && idSeria != 0){
        auto range = lib.authorBooksLink.equal_range(idAuthor);
        for (auto it = range.first; it != range.second; ++it) {
            auto &book = lib.books.at(it->second);
            if(book.bDeleted) [[unlikely]]
                continue;
            if(book.mSequences.contains(idSeria))
                if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == lib.vLaguages[book.idLanguage])
                    vBooks.push_back(it->second);
        }
        std::sort(vBooks.begin(), vBooks.end(),[&lib](uint id1, uint id2)
        {
            const auto &sequence1 = lib.books.at(id1).mSequences;
            const auto &sequence2 = lib.books.at(id2).mSequences;
            uint nNumInSequence1 = sequence1.empty() ?0 : sequence1.begin()->second;
            uint nNumInSequence2 = sequence1.empty() ?0 : sequence2.begin()->second;
            return nNumInSequence1 < nNumInSequence2;
        });

    }
    if(idAuthor != 0 && idSeria == 0){
        auto range = lib.authorBooksLink.equal_range(idAuthor);
        if(sequenceless){
            for (auto it = range.first; it != range.second; ++it) {
                auto &book = lib.books.at(it->second);
                if(book.bDeleted) [[unlikely]]
                    continue;
                if(book.mSequences.empty())
                    if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == lib.vLaguages[book.idLanguage])
                        vBooks.push_back(it->second);
            }
        }else{
            for (auto it = range.first; it != range.second; ++it) {
                auto &book = lib.books.at(it->second);
                if(!book.bDeleted) [[likely]]
                    if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == lib.vLaguages[book.idLanguage])
                        vBooks.push_back(it->second);
            }
        }
        std::sort(vBooks.begin(), vBooks.end(), [&lib](uint lhs, uint rhs){ return localeStringCompare(lib.books.at(lhs).sName, lib.books.at(rhs).sName); });
    }
    if(idAuthor == 0 && idSeria != 0){
        for(const auto &[idBook, book] :lib.books){
            if(book.bDeleted) [[unlikely]]
                continue;
            if(book.mSequences.contains(idSeria))
                if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == lib.vLaguages[book.idLanguage])
                    vBooks.push_back(idBook);
        }
        std::sort(vBooks.begin(), vBooks.end(),[&lib](uint id1, uint id2)
        {
            const auto &sequence1 = lib.books.at(id1).mSequences;
            const auto &sequence2 = lib.books.at(id2).mSequences;
            uint nNumInSequence1 = sequence1.empty() ?0 : sequence1.begin()->second;
            uint nNumInSequence2 = sequence2.empty() ?0 : sequence2.begin()->second;
            return nNumInSequence1 < nNumInSequence2;
        });
    }
    return vBooks;
}

std::vector<uint> opds_server::listGenreBooks(const SLib &lib, ushort idGenre)
{
    std::vector<uint> vBooks;
    for(const auto &[idBook, book] :lib.books){
        if(book.bDeleted) [[unlikely]]
            continue;
        for(auto iGenre: book.vIdGenres){
            if(iGenre == idGenre){
                if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == lib.vLaguages[book.idLanguage]){
                    vBooks.push_back(idBook);
                    break;
                }
            }
        }
    }
    std::sort(g::executionpolicy, vBooks.begin(), vBooks.end(), [&lib](uint lhs, uint rhs){
        return localeStringCompare(lib.books.at(lhs).sName,lib.books.at(rhs).sName);
    });

    return vBooks;
}

std::vector<uint> opds_server::searchTitle(const SLib &lib, const QString &sTitle)
{
    QString sSearch = simplifySearchString(sTitle);
    std::vector<uint> vBooks;
    if(!sSearch.isEmpty()){
        vBooks = blockingFiltered(lib.books, [&](const auto &book){
            if(!book.bDeleted) [[likely]]{
                QString sName = simplifySearchString(book.sName);
                if(sName.contains(sSearch)){
                    if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == lib.vLaguages[book.idLanguage])
                        return true;
                }
            }
            return false;
        });

        std::sort(g::executionpolicy, vBooks.begin(), vBooks.end(), [&lib](uint lhs, uint rhs){
                return localeStringCompare(lib.books.at(lhs).sName, lib.books.at(rhs).sName);
        });
    }
    return vBooks;
}

std::vector<uint> opds_server::searchBooks(const SLib &lib, const QString &sAuthor, const QString &sTitle)
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
                    if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == lib.vLaguages[book.idLanguage])
                        return true;
            }
        }
        return false;
    });

    std::sort(g::executionpolicy, vBooks.begin(), vBooks.end(), [&lib](uint id1, uint id2){
        return localeStringCompare(lib.books.at(id1).sName, lib.books.at(id2).sName);
    });

    return vBooks;
}

auto opds_server::searchSequence(const SLib &lib, const QString &sSequence)
{
    QString sSearchSimplefied = simplifySearchString(sSequence);
    SerialComparator comporator(lib.serials);
    std::map<uint, uint, SerialComparator> mSequence(comporator);

    std::vector<uint> v;
    v.reserve(lib.books.size());
    for(const auto &it :lib.books)
        v.push_back(it.first);
    std::mutex m;
    QtConcurrent::blockingMap(v, [&](auto id){
        auto &book = lib.books.at(id);
        if(!book.mSequences.empty() && !book.bDeleted){
            if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == lib.vLaguages[book.idLanguage]){
                for(const auto &iSequance :book.mSequences){
                    const auto &secuence = lib.serials.at(iSequance.first);
                    QString sName  = simplifySearchString(secuence.sName);
                    if(sName.contains(sSearchSimplefied)){
                        std::lock_guard<std::mutex> guard(m);
                        mSequence[iSequance.first]++;
                    }
                }
            }
        }
    });

    return mSequence;
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

QDomElement opds_server::docHeaderHTML(const QString &sSessionQuery, const QString &sLibName, const QString &sLibUrl)
{
    doc_ = QDomDocument(u"HTML"_s);
    QDomElement html = doc_.createElement(u"html"_s);
    doc_.appendChild(html);
    QDomElement head = doc_.createElement(u"head"_s);
    html.appendChild(head);
    AddTextNode(u"title"_s, sLibName, head);

    QDomElement link;
    link = doc_.createElement(u"link"_s);
    link.setAttribute(u"rel"_s, u"stylesheet"_s);
    link.setAttribute(u"href"_s, u"/styles.css"_s);
    head.appendChild(link);


    QDomElement meta = doc_.createElement(u"META"_s);
    meta.setAttribute(u"charset"_s, u"utf-8"_s);
    head.appendChild(meta);

    if(!g::options.sBaseUrl.isEmpty()){
        QDomElement baseUrl = doc_.createElement(u"base"_s);
        baseUrl.setAttribute(u"href"_s, g::options.sBaseUrl);
        head.appendChild(baseUrl);
    }

    QString sIconFile = u"/icon_256x256.png"_s;

    addLink(head, u"image/png"_s, sIconFile, u"icon"_s);
    addLink(head, u"image/png"_s, sIconFile, u"shortcut icon"_s);
    addLink(head, u"image/png"_s, sIconFile, u"apple-touch-icon"_s);

    QDomElement body = doc_.createElement(u"body"_s);
    html.appendChild(body);
    QDomElement a = AddTextNode(u"a"_s, u""_s, body);
    QString sHref = (sLibUrl.isEmpty() ?u"/"_s :sLibUrl) + sSessionQuery;
    a.setAttribute(u"class"_s, u"lib"_s);
    a.setAttribute(u"href"_s, sHref);
    QDomElement svg = doc_.createElement(u"svg"_s);
    svg.setAttribute(u"xmlns"_s, u"http://www.w3.org/2000/svg"_s);
    svg.setAttribute(u"class"_s, u"home"_s);
    svg.setAttribute(u"viewBox"_s, u"0 0 32 32"_s);
    a.appendChild(svg);
    QDomElement path = doc_.createElement(u"path"_s);
    path.setAttribute(u"d"_s, u"M 1,17 16,1 31,17 h -4 v 14 h -8 v -9 h -6 v 9 h -8 V 17 Z"_s);
    svg.appendChild(path);

    addHRefNode(body, sLibName, sHref, u"lib"_s);

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

    AddTextNode(u"updated"_s, QDateTime::currentDateTimeUtc().toString(Qt::ISODate), feed);
    AddTextNode(u"icon"_s, u"/icon_256x256.png"_s, feed);
    if(!sID.isEmpty())
        AddTextNode(u"id"_s, sID, feed);
    AddTextNode(u"title"_s, sTitle, feed);

    addLink(feed, u"application/atom+xml;profile=opds-catalog;kind=navigation"_s, sLibUrl, u"start"_s , u"Home"_s);
    addLink(feed, u"application/opensearchdescription+xml"_s, sLibUrl + u"/opensearch.xml"_s, u"search"_s);
    QString sHref = sLibUrl + u"/search?q={searchTerms}&author={atom:author}&title={atom:title}"_s;
    if(!sSession.isEmpty())
        sHref += u"&session="_s + sSession;
    addLink(feed, u"application/atom+xml"_s, sHref, u"search"_s);
    return feed;
}

QJsonObject opds_server::docHeaderOPDS2(const QString &sTitle, const QString &sLibUrl, const QString &sSession)
{
    QString sSessionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;
    QJsonObject root;

    QJsonObject metadata;
    metadata[u"title"] = sTitle;
    root[u"metadata"] = metadata;

    QJsonArray links;
    addLink(links, u"application/opds+json"_s, sLibUrl + sSessionQuery, u"self"_s);

    QJsonObject linkSearch;
    linkSearch[u"rel"] = u"search"_s;
    linkSearch[u"href"] = QString(sLibUrl % u"/search{?query,title,author}"_s % (sSession.isEmpty() ?u""_s :u"&session="_s % sSession));
    linkSearch[u"type"] = u"application/opds+json"_s;
    linkSearch[u"templated"] = true;
    links.push_back(std::move(linkSearch));

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
    auto listQueryItems = urlquery.queryItems();
    int i;
    for(i=0; i<listQueryItems.size(); i++){
        if(listQueryItems[i].first == u"page"_s){
            listQueryItems[i].second = QString::number(nPage);
            break;
        }
    }
    if(i >= listQueryItems.size())
        listQueryItems << QPair<QString, QString> (u"page"_s, QString::number(nPage));
    urlquery.setQueryItems(listQueryItems);
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
            BookFile file(&lib, idBook);
            book.sAnnotation = file.annotation();
        });
    }
}

QHttpServerResponse opds_server::generatePageHTML(const std::vector<uint> &vBooks, SLib &lib, const QString &sTitle, const QString &sLibUrl, const QUrl &url, bool bShowAuthor)
{
    QUrlQuery urlquery(url);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSessionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    QDomElement feed;
    feed = docHeaderHTML(sSessionQuery, lib.name, sLibUrl);
    addTextNode(feed, u"div"_s, sTitle, u"caption"_s);

    fillPageHTML(vBooks, lib, feed, sLibUrl, sSessionQuery, url, bShowAuthor);
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

void opds_server::fillPageHTML(const std::vector<uint> &vBooks, SLib &lib, QDomElement &feed, const QString &sLibUrl, const QString &sSessionQuery, const QUrl &url, bool bShowAuthor)
{
    auto nMaxBooksPerPage = g::options.nOpdsBooksPerPage;
    if(nMaxBooksPerPage == 0)
        nMaxBooksPerPage = std::numeric_limits<typeof(nMaxBooksPerPage)>::max();

    QUrlQuery urlquery(url);
    uint nPage = urlquery.queryItemValue(u"page"_s).toUInt();
    if(nPage == 0)
        nPage = 1;

    uint iBookBegin = (nPage-1)*nMaxBooksPerPage;
    uint iBookEnd = std::min(static_cast<uint>(vBooks.size()), nPage*nMaxBooksPerPage);
    loadAnnotations(vBooks, lib, iBookBegin, iBookEnd);

    auto convertFormats =  convertFormatAvaible();

    for(uint iBook = iBookBegin; iBook < iBookEnd; ++iBook){
        uint idBook = vBooks.at(iBook);
        SBook& book = lib.books.at(idBook);
        QString sIdBook = QString::number(idBook);
        QDomElement entry = doc_.createElement(u"div"_s);
        entry.setAttribute(u"class"_s, u"item"_s);
        feed.appendChild(entry);

        QDomElement table = doc_.createElement(u"table"_s);
        table.setAttribute(u"width"_s, u"100%"_s);
        entry.appendChild(table);
        QDomElement tr = doc_.createElement(u"tr"_s);
        table.appendChild(tr);
        entry = doc_.createElement(u"td"_s);
        tr.appendChild(entry);

        if(g::options.bOpdsShowCover)
        {
            QDomElement el = doc_.createElement(u"img"_s);
            entry.appendChild(el);
            el.setAttribute(u"src"_s, sLibUrl % u"/covers/"_s % sIdBook % u"/cover.jpg"_s);
            el.setAttribute(u"class"_s, u"cover"_s);
        }

        QString sSerial = book.mSequences.empty() ?u""_s :lib.serials.at(book.mSequences.begin()->first).sName;
        uint numInSequence = book.mSequences.empty() ?0 :book.mSequences.begin()->second;
        QString sText = book.sName % (numInSequence==0  ?u""_s :(u" ("_s % sSerial % u"["_s % QString::number(numInSequence) % u"])"_s));
        addTextNode(entry, u"div"_s, sText, u"book"_s);
        if(bShowAuthor)
            addHRefNode(entry, lib.authors.at(book.idFirstAuthor).getName(), sLibUrl % u"/author/"_s % QString::number(book.idFirstAuthor) % sSessionQuery, u"author"_s);
        QDomElement br = doc_.createElement(u"br"_s);
        entry.appendChild(br);

        if(book.sFormat == u"fb2"_s)
        {
            addDownloadItem(entry, u"fb2"_s, sLibUrl % u"/book/"_s % sIdBook % u"/fb2"_s % sSessionQuery);
            if(convertFormats & epub)
                addDownloadItem(entry, u"epub"_s, sLibUrl % u"/book/"_s % sIdBook % u"/epub"_s % sSessionQuery);
            if(convertFormats & mobi)
                addDownloadItem(entry, u"mobi"_s, sLibUrl % u"/book/"_s % sIdBook % u"/mobi"_s % sSessionQuery);
            if(convertFormats & azw3)
                addDownloadItem(entry, u"azw3"_s, sLibUrl % u"/book/"_s % sIdBook % u"/azw3"_s % sSessionQuery);
        }
        else if(book.sFormat == u"epub"_s)
        {
            addDownloadItem(entry, u"epub"_s, sLibUrl % u"/book/"_s % sIdBook % u"/epub"_s);
            if(convertFormats & mobi)
                addDownloadItem(entry, u"mobi"_s, sLibUrl % u"/book/"_s % sIdBook % u"/mobi"_s % sSessionQuery);
        }
        else if(book.sFormat == u"mobi"_s)
            addDownloadItem(entry, u"mobi"_s, sLibUrl % u"/book/"_s % sIdBook % u"/mobi"_s % sSessionQuery);
        else
            addDownloadItem(entry, book.sFormat, sLibUrl % u"/book/"_s % sIdBook % u"/download"_s % sSessionQuery);

        if(g::options.bOpdsShowAnotation)
            addTextNode(entry, u"div"_s, book.sAnnotation, u"annotation"_s);
    }

    uint nPageCount = vBooks.size()>0 ?(vBooks.size()-1) / nMaxBooksPerPage + 1 :0;
    if(nPageCount > 1){
        QDomElement pageBar = doc_.createElement(u"div"_s);
        pageBar.setAttribute(u"class"_s, u"page-bar"_s);
        feed.appendChild(pageBar);


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
            addHRefNode(pageBar, QString::number(nPageCount),hrefOfPage(url, nPageCount), u"page"_s);
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
        AddTextNode(u"updated"_s, book.date.toString(Qt::ISODate), entry);
        AddTextNode(u"id"_s, u"tag:book:"_s + QString::number(idBook), entry);
        QString sSequence = book.mSequences.empty() ?u""_s :lib.serials[book.mSequences.begin()->first].sName;
        AddTextNode(u"title"_s, book.sName % (sSequence.isEmpty() ?u""_s :u" ("_s % sSequence % u")"_s), entry);
        for(uint idAuthor: book.vIdAuthors){
            QDomElement author = doc_.createElement(u"author"_s);
            entry.appendChild(author);
            AddTextNode(u"name"_s, lib.authors[idAuthor].getName(), author);
        }
        for(auto idGenre: book.vIdGenres){
            QDomElement category = doc_.createElement(u"category"_s);
            entry.appendChild(category);
            category.setAttribute(u"term"_s, g::genres[idGenre].sName);
            category.setAttribute(u"label"_s, g::genres[idGenre].sName);
        }
        QDomElement el;
        if(book.sFormat == u"fb2"_s)
        {
            addLink(entry, u"application/x-fictionbook+xml"_s, sLibUrl % u"/book/"_s % sIdBook % u"/fb2"_s % sSessionQuery, u"http://opds-spec.org/acquisition/open-access"_s);
            if(convertFormats & epub)
                addLink(entry, u"application/epub+zip"_s, sLibUrl % u"/book/"_s % sIdBook % u"/epub"_s % sSessionQuery, u"http://opds-spec.org/acquisition/open-access"_s);
            if(convertFormats & mobi)
                addLink(entry, u"application/x-mobipocket-ebook"_s, sLibUrl % u"/book/"_s % sIdBook % u"/mobi"_s % sSessionQuery, u"http://opds-spec.org/acquisition/open-access"_s);
            if(convertFormats & azw3)
                addLink(entry, u"application/x-mobi8-ebook"_s, sLibUrl % u"/book/"_s % sIdBook % u"/azw3"_s % sSessionQuery, u"http://opds-spec.org/acquisition/open-access"_s);
        }
        else if(book.sFormat == u"epub"_s){
            addLink(entry, u"application/epub+zip"_s, sLibUrl % u"/book/"_s % sIdBook % u"/download"_s % sSessionQuery, u"http://opds-spec.org/acquisition/open-access"_s);
            if(convertFormats & mobi)
                addLink(entry, u"application/x-mobipocket-ebook"_s, sLibUrl % u"/book/"_s % sIdBook % u"/mobi"_s % sSessionQuery, u"http://opds-spec.org/acquisition/open-access"_s);
            if(convertFormats & azw3)
                addLink(entry, u"application/x-mobi8-ebook"_s, sLibUrl % u"/book/"_s % sIdBook % u"/azw3"_s % sSessionQuery, u"http://opds-spec.org/acquisition/open-access"_s);
        }else if(book.sFormat == u"mobi"_s)
            addLink(entry, u"application/x-mobipocket-ebook"_s, sLibUrl % u"/book/"_s % sIdBook % u"/download"_s % sSessionQuery, u"http://opds-spec.org/acquisition/open-access"_s);
        else
            addLink(entry, mime(book.sFormat), sLibUrl % u"/book/"_s % sIdBook % u"/download"_s % sSessionQuery, u"alternate"_s, tr("Download"));

        if(g::options.bOpdsShowCover)
        {
            addLink(entry, u"image/jpeg"_s, sLibUrl % u"/covers/"_s % sIdBook % u"/cover.jpg"_s, u"http://opds-spec.org/image"_s);
            addLink(entry, u"image/jpeg"_s, sLibUrl % u"/covers/"_s % sIdBook % u"/cover.jpg"_s, u"x-stanza-cover-image"_s);
            addLink(entry, u"image/jpeg"_s, sLibUrl % u"/covers/"_s % sIdBook % u"/cover.jpg"_s, u"http://opds-spec.org/thumbnail"_s);
            addLink(entry, u"image/jpeg"_s, sLibUrl % u"/covers/"_s % sIdBook % u"/cover.jpg"_s, u"x-stanza-cover-image-thumbnail"_s);
        }
        AddTextNode(u"dc:language"_s, lib.vLaguages[book.idLanguage], entry);
        AddTextNode(u"dc:format"_s, book.sFormat, entry);

        if(g::options.bOpdsShowAnotation)
        {
            if(book.sAnnotation.isEmpty()){
                BookFile file(&lib, idBook);
                book.sAnnotation = file.annotation();
            }
            el = AddTextNode(u"content"_s, book.sAnnotation, entry);
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
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSessionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    uint iBookBegin = (nPage-1)*nMaxBooksPerPage;
    uint iBookEnd = std::min(static_cast<uint>(vBooks.size()), nPage*nMaxBooksPerPage);
    loadAnnotations(vBooks, lib, iBookBegin, iBookEnd);

    auto root = docHeaderOPDS2(sTitle, sLibUrl, sSession);
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
        QString sSequence = book.mSequences.empty() ?u""_s :lib.serials[book.mSequences.begin()->first].sName;
        metadata[u"@type"] = u"http://schema.org/Book"_s;
        metadata[u"title"] = QString(book.sName % (sSequence.isEmpty() ?u""_s :u" ("_s % sSequence % u")"_s));
        if(g::options.bOpdsShowAnotation)
        {
            if(book.sAnnotation.isEmpty()){
                BookFile file(&lib, idBook);
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
                link[u"href"] = QString(sLibUrl % u"/author/"_s % QString::number(idAuthor) % sSessionQuery);
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
            link[u"href"] = QString(sLibUrl % u"/genres/"_s % QString::number(idGenre) % sSessionQuery);
            links.push_back(std::move(link));
            genre[u"links"] = links;
            subject.push_back(std::move(genre));
        }
        metadata[u"subject"] = subject;
        entry[u"metadata"] = metadata;

        QJsonArray links;
        if(book.sFormat == u"fb2")
        {
            addLink(links, u"application/x-fictionbook+xml"_s, sLibUrl % u"/book/"_s % sIdBook % u"/fb2"_s % sSessionQuery, u"http://opds-spec.org/acquisition/open-access"_s);
            if(convertFormats & epub)
                addLink(links, u"application/epub+zip"_s, sLibUrl % u"/book/"_s % sIdBook % u"/epub"_s % sSessionQuery, u"http://opds-spec.org/acquisition/open-access"_s);
            if(convertFormats & mobi)
                addLink(links, u"application/x-mobipocket-ebook"_s, sLibUrl % u"/book/"_s % sIdBook % u"/mobi"_s % sSessionQuery, u"http://opds-spec.org/acquisition/open-access"_s);
            if(convertFormats & azw3)
                addLink(links, u"application/x-mobi8-ebook"_s, sLibUrl % u"/book/"_s % sIdBook % u"/azw3"_s % sSessionQuery, u"http://opds-spec.org/acquisition/open-access"_s);
        }
        else if(book.sFormat == u"epub"){
            addLink(links, u"application/epub+zip"_s, sLibUrl % u"/book/"_s % sIdBook % u"/download"_s % sSessionQuery, u"http://opds-spec.org/acquisition/open-access"_s);
            if(convertFormats & mobi)
                addLink(links, u"application/x-mobipocket-ebook"_s, sLibUrl % u"/book/"_s % sIdBook % u"/mobi"_s % sSessionQuery, u"http://opds-spec.org/acquisition/open-access"_s);
            if(convertFormats & azw3)
                addLink(links, u"application/x-mobi8-ebook"_s, sLibUrl % u"/book/"_s % sIdBook % u"/azw3"_s % sSessionQuery, u"http://opds-spec.org/acquisition/open-access"_s);
        }else if(book.sFormat == u"mobi")
            addLink(links, u"application/x-mobipocket-ebook"_s, sLibUrl % u"/book/"_s % sIdBook % u"/download"_s % sSessionQuery, u"http://opds-spec.org/acquisition/open-access"_s);
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
    const SLib *pLib = getLib(idLib, u""_s, &sLibUrl);
    if(pLib == nullptr)
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSessionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;
    QDomElement feed = docHeaderHTML(sSessionQuery, pLib->name, sLibUrl);
    QDomElement div = doc_.createElement(u"div"_s);
    feed.appendChild(div);
    QDomElement el;

    el = AddTextNode(u"a"_s, tr("Finding books by authors"), div);
    el.setAttribute(u"href"_s, sLibUrl % u"/authorsindex"_s % sSessionQuery );
    div = doc_.createElement(u"div"_s);
    feed.appendChild(div);
    el = AddTextNode(u"a"_s, tr("Finding books by sequences"), div);
    el.setAttribute(u"href"_s, sLibUrl % u"/sequencesindex"_s % sSessionQuery);
    div = doc_.createElement(u"div"_s);
    feed.appendChild(div);
    el = AddTextNode(u"a"_s, tr("Finding books by genre"), div);
    el.setAttribute(u"href"_s, sLibUrl % u"/genres"_s % sSessionQuery);

    QDomElement hr = doc_.createElement(u"HR"_s);
    feed.appendChild(hr);
    attachSearchFormHTML(feed, tr("Finding books by title") + u": "_s, sLibUrl + u"/searchtitle"_s, u""_s, sSession);

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
    addEntry(feed, u"tag:root:sequences"_s, sLibUrl % u"/sequencesindex"_s % sSessionQuery, tr("Books by sequences"), tr("Finding books by sequences"));
    addEntry(feed, u"tag:root:genre"_s, sLibUrl % u"/genres"_s % sSessionQuery, tr("Books by genre"), tr("Finding books by genre"));

    QHttpServerResponse result("application/atom+xml;charset=utf-8"_ba, doc_.toByteArray());
    return result;
}

QHttpServerResponse opds_server::rootOPDS2(uint idLib, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QUrlQuery urlquery(url);
    QString sLibUrl;
    const SLib *pLib = getLib(idLib, u"/opds2"_s, &sLibUrl);
    if(pLib == nullptr)
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSessionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    QJsonObject root = docHeaderOPDS2(pLib->name, sLibUrl, sSession);

    QJsonArray navigation;
    addNavigation(navigation, tr("Books by authors"), sLibUrl % u"/authorsindex"_s % sSessionQuery);
    addNavigation(navigation, tr("Books by sequences"), sLibUrl % u"/sequencesindex"_s % sSessionQuery);
    addNavigation(navigation, tr("Books by genre"), sLibUrl % u"/genres"_s % sSessionQuery);
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
        QImage img = file.cover();
        QBuffer buffer(&baResult);
        buffer.open(QIODevice::WriteOnly);
        img.save(&buffer, "jpg");
    }
    return baResult;
}

struct LocaleAwareQStringComparator {
    bool operator()(const QString& str1, const QString& str2) const {
        return localeStringCompare(str1, str2);
    }
};

void opds_server::attachSearchFormHTML(QDomElement &feed, const QString &sTitle, const QString &sAction, const QString &sSearch, const QString &sSession)
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

    if(!sSession.isEmpty()){
        el = doc_.createElement(u"input"_s);
        el.setAttribute(u"type"_s, u"hidden"_s);
        el.setAttribute(u"name"_s, u"session"_s);
        el.setAttribute(u"value"_s, sSession);
        div.appendChild(el);
    }

    el = doc_.createElement(u"input"_s);
    el.setAttribute(u"type"_s, u"submit"_s);
    el.setAttribute(u"value"_s, tr("Find"));
    div.appendChild(el);
}

QHttpServerResponse opds_server::authorsIndexHTML(uint idLib, const QString &sIndex, bool bByBooks, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    const SLib *pLib = getLib(idLib, u""_s, &sLibUrl);
    if(pLib == nullptr)
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSessionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    QDomElement feed = docHeaderHTML(sSessionQuery, pLib->name, sLibUrl);

    std::map<QString, int, LocaleAwareQStringComparator> mCount;

    std::unordered_set<QString> setAuthors;
    std::unordered_set<uint>stIdAuthors;

    int count = 0;
    QString sLowerIndex = sIndex.toCaseFolded();
    for(const auto &iBook :pLib->books){
        if(!iBook.second.bDeleted) [[likely]]{
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
        attachSearchFormHTML(feed, tr("Finding authors") + u": "_s, sLibUrl + u"/searchauthor"_s, sIndex, sSession);

        QDomElement tag_table;
        QDomElement tag_tr;
        int nCurrentColumn = 0;
        tag_table = doc_.createElement(u"TABLE"_s);
        feed.appendChild(tag_table);

        for(const auto &iIndex :mCount)
        {
            if(nCurrentColumn == 0)
            {
                tag_tr = doc_.createElement(u"tr"_s);
                tag_table.appendChild(tag_tr);
            }
            QDomElement td = doc_.createElement(u"td"_s);
            tag_tr.appendChild(td);
            QDomElement div = doc_.createElement(u"div"_s);
            div.setAttribute(u"class"_s, u"item"_s);
            td.appendChild(div);
            QDomElement el = AddTextNode(u"a"_s, iIndex.first, div);
            el.setAttribute(u"class"_s, u"block"_s);
            AddTextNode(u"div"_s, QString::number(iIndex.second) % u" "_s % tr("authors beginning with") % u" '"_s %
                        iIndex.first % u"'"_s, div);
            if(iIndex.second == 1)
            {
                QString lowerIndex = iIndex.first.toCaseFolded();
                for(auto idAuthor :stIdAuthors)
                {
                    const auto &author = pLib->authors.at(idAuthor);
                    if(author.getName().left(iIndex.first.size()).toCaseFolded() == lowerIndex)
                    {
                        el.setAttribute(u"href"_s, sLibUrl % u"/author/"_s % QString::number(idAuthor) % sSessionQuery);
                        break;
                    }
                }
            }
            else
            {
                el.setAttribute(u"href"_s, sLibUrl % u"/authorsindex/"_s % QString::fromUtf8(QUrl::toPercentEncoding(iIndex.first, ""_ba, "."_ba).constData()) %
                                (setAuthors.contains(iIndex.first) ?u"/books"_s :u""_s) % sSessionQuery);
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
        std::sort(g::executionpolicy, vAuthorId.begin(), vAuthorId.end(), [pLib](uint id1, uint id2)
        {
            return pLib->authors.at(id1).getName() < pLib->authors.at(id2).getName();
        });
        for(uint iIndex: vAuthorId)
        {
            uint nBooksCount = 0;
            auto range = pLib->authorBooksLink.equal_range(iIndex);
            for (auto it = range.first; it != range.second; ++it) {
                auto &book = pLib->books.at(it->second);
                if(!book.bDeleted) [[likely]]
                    if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == pLib->vLaguages[book.idLanguage])
                        nBooksCount++;
            }

            QDomElement div = doc_.createElement(u"DIV"_s);
            div.setAttribute(u"class"_s, u"item"_s);
            feed.appendChild(div);
            addHRefNode(div, pLib->authors.at(iIndex).getName(), sLibUrl % u"/author/"_s % QString::number(iIndex) % sSessionQuery, u"block"_s);
            AddTextNode(u"div"_s, QString::number(nBooksCount) % u" "_s % tr("books"), div);
        }
    }

    return responseHTML();
}

QHttpServerResponse opds_server::authorsIndexOPDS(uint idLib, const QString &sIndex, bool bByBooks, const QHttpServerRequest &request)
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

    std::map<QString, int, LocaleAwareQStringComparator> mCount;

    std::unordered_set <QString> setAuthors;
    std::unordered_set<uint>stIdAuthors;
    int count = 0;
    QString sLowerIndex = sIndex.toCaseFolded();
    for(const auto &iBook :pLib->books){
        if(!iBook.second.bDeleted) [[likely]]{
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

        for(const auto &iIndex :mCount)
        {
            QString sContent = QString::number(iIndex.second) % u" "_s % tr("authors beginning with") % u" '"_s % iIndex.first % u"'"_s;
            QString sHRef;
            if(iIndex.second == 1)
            {
                QString lowerIndex = iIndex.first.toCaseFolded();
                for(auto idAuthor :stIdAuthors)
                {
                    const auto &author = pLib->authors.at(idAuthor);
                    if(author.getName().left(iIndex.first.size()).toCaseFolded() == lowerIndex)
                    {
                        sHRef = sLibUrl % u"/author/"_s % QString::number(idAuthor) % sSessionQuery;
                        break;
                    }
                }
            }
            else
            {
                sHRef = sLibUrl % u"/authorsindex/"_s % QString::fromUtf8(QUrl::toPercentEncoding(iIndex.first, ""_ba, "."_ba).constData()) %
                                                       (setAuthors.contains(iIndex.first) && iIndex.second<30 ?u"/books"_s :u""_s) % sSessionQuery;
            }
            addEntry(feed, u"tag:authors:"_s + iIndex.first, sHRef, iIndex.first, sContent);
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
                if(!book.bDeleted) [[likely]]
                    if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == pLib->vLaguages[book.idLanguage])
                        nBooksCount++;
            }

            addEntry(feed, u"tag:author:"_s + QString::number(iIndex), sLibUrl % u"/author/"_s % QString::number(iIndex) % sSessionQuery,
                     pLib->authors.at(iIndex).getName(), QString::number(nBooksCount) % u" "_s % tr("books"));
        }
    }

    QHttpServerResponse result(doc_.toString());
    return result;
}

QHttpServerResponse opds_server::authorsIndexOPDS2(uint idLib, const QString &sIndex, bool bByBooks, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    const SLib *pLib = getLib(idLib, u"/opds2"_s, &sLibUrl);
    if(pLib == nullptr) [[unlikely]]
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSessionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    QJsonObject root = docHeaderOPDS2(tr("Books by authors"), sLibUrl, sSession);

    std::map<QString, int, LocaleAwareQStringComparator> mCount;

    std::unordered_set <QString> setAuthors;
    std::unordered_set<uint>stIdAuthors;
    int count = 0;
    QString sLowerIndex = sIndex.toCaseFolded();
    for(const auto &iBook :pLib->books){
        if(!iBook.second.bDeleted) [[likely]]{
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

    QJsonArray navigation;
    if(count>30 && !bByBooks)
    {
        for(const auto &iIndex :mCount)
        {
            QString sHRef;
            if(iIndex.second == 1)
            {
                QString lowerIndex = iIndex.first.toCaseFolded();
                for(auto idAuthor :stIdAuthors)
                {
                    const auto &author = pLib->authors.at(idAuthor);
                    if(author.getName().left(iIndex.first.size()).toCaseFolded() == lowerIndex)
                    {
                        sHRef = sLibUrl % u"/author/"_s % QString::number(idAuthor) % sSessionQuery;
                        break;
                    }
                }
            }
            else
            {
                sHRef = sLibUrl % u"/authorsindex/"_s % QString::fromUtf8(QUrl::toPercentEncoding(iIndex.first, ""_ba, "."_ba).constData()) %
                                                    (setAuthors.contains(iIndex.first) && iIndex.second<30 ?u"/books"_s :u""_s) % sSessionQuery;

            }
            addNavigation(navigation, iIndex.first, sHRef,  iIndex.second);
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
                if(!book.bDeleted) [[likely]]
                    if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == pLib->vLaguages[book.idLanguage])
                        nBooksCount++;
            }
            addNavigation(navigation, pLib->authors.at(iIndex).getName(), sLibUrl % u"/author/"_s % QString::number(iIndex) % sSessionQuery, nBooksCount);
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
    const SLib *pLib = getLib(idLib, u""_s, &sLibUrl);
    if(pLib == nullptr || !pLib->authors.contains(idAuthor))
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSessionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    QString sAuthor = pLib->authors.at(idAuthor).getName();
    QString sIdAuthor = QString::number(idAuthor);
    QDomElement feed = docHeaderHTML(sSessionQuery, pLib->name, sLibUrl);
    QDomElement divAuth = doc_.createElement(u"DIV"_s);;
    divAuth.setAttribute(u"class"_s, u"item"_s);
    feed.appendChild(divAuth);
    addTextNode(divAuth, u"div"_s, tr("Books by") % u" "_s % sAuthor, u"caption"_s);

    QDomElement div = doc_.createElement(u"div"_s);
    divAuth.appendChild(div);
    addHRefNode(div, tr("Books by sequences"), sLibUrl % u"/authorsequences/"_s % sIdAuthor % sSessionQuery, u"block"_s);

    div = doc_.createElement(u"div"_s);
    divAuth.appendChild(div);
    addHRefNode(div, tr("Books without sequence"), sLibUrl % u"/authorsequenceless/"_s % sIdAuthor % sSessionQuery, u"block"_s);

    div = doc_.createElement(u"div"_s);
    divAuth.appendChild(div);
    addHRefNode(div, tr("All books"), sLibUrl % u"/authorbooks/"_s % sIdAuthor % sSessionQuery, u"block"_s);

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

    addEntry(feed, u"tag:author:"_s % sIdAuthor % u":sequences"_s, sLibUrl % u"/authorsequences/"_s % sIdAuthor % sSessionQuery, tr("Books by sequences"), u""_s);
    addEntry(feed, u"tag:author:"_s % sIdAuthor % u":sequenceless"_s, sLibUrl % u"/authorsequenceless/"_s % sIdAuthor % sSessionQuery, tr("Books without sequence"), u""_s);
    addEntry(feed, u"tag:author:"_s % sIdAuthor % u":sequences"_s, sLibUrl % u"/authorbooks/"_s % sIdAuthor % sSessionQuery, tr("All books"), u""_s);

    QHttpServerResponse result(doc_.toString());
    return result;
}

QHttpServerResponse opds_server::authorOPDS2(uint idLib, uint idAuthor, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    const SLib *pLib = getLib(idLib, u"/opds2"_s, &sLibUrl);
    if(pLib == nullptr || !pLib->authors.contains(idAuthor)) [[unlikely]]
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSessionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    QString sAuthor = pLib->authors.at(idAuthor).getName();
    QString sIdAuthor = QString::number(idAuthor);
    QJsonObject root = docHeaderOPDS2(tr("Books by") % u" "_s % sAuthor, sLibUrl, sSession);

    QJsonArray navigation;
    addNavigation(navigation, tr("Books by sequences"), sLibUrl % u"/authorsequences/"_s % sIdAuthor % sSessionQuery);
    addNavigation(navigation, tr("Books without sequence"), sLibUrl % u"/authorsequenceless/"_s % sIdAuthor % sSessionQuery);
    addNavigation(navigation, tr("All books"), sLibUrl % u"/authorbooks/"_s % sIdAuthor % sSessionQuery);
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

    std::vector<uint> vBooks = book_list(*pLib, idAuthor, 0, false);
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

    std::vector<uint> vBooks = book_list(*pLib, idAuthor, 0, false);
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

    std::vector<uint> vBooks = book_list(*pLib, idAuthor, 0, false);
    QHttpServerResponse result = generatePageOPDS2(vBooks, *pLib, tr("Books by ABC") % u" ("_s % pLib->authors.at(idAuthor).getName() % u")"_s, sLibUrl, url);
    return result;
}

QHttpServerResponse opds_server::authorSequencesHTML(uint idLib, uint idAuthor, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    const SLib *pLib = getLib(idLib, u""_s, &sLibUrl);
    if(pLib == nullptr || !pLib->authors.contains(idAuthor)) [[unlikely]]
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSessionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    QDomElement feed = docHeaderHTML(sSessionQuery, pLib->name, sLibUrl);
    addTextNode(feed, u"div"_s, tr("Book sequences") % u" "_s % pLib->authors.at(idAuthor).getName(), u"caption"_s);

    SerialComparator comporator(pLib->serials);
    std::map<uint, uint, SerialComparator> mCountBooks(comporator);
    auto range = pLib->authorBooksLink.equal_range(idAuthor);
    for (auto it = range.first; it != range.second; ++it) {
        const SBook& book = pLib->books.at(it->second);
        if(book.bDeleted) [[unlikely]]
            continue;
        if(!book.mSequences.empty())
            if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == pLib->vLaguages[book.idLanguage])
                for(const auto &iSequence :book.mSequences)
                    ++mCountBooks[iSequence.first];
    }

    QString sIdAuthor = QString::number(idAuthor);
    for(const auto &iSeria :mCountBooks)
    {
        QDomElement entry = doc_.createElement(u"div"_s);
        entry.setAttribute(u"class"_s, u"item"_s);
        feed.appendChild(entry);
        addHRefNode(entry, pLib->serials.at(iSeria.first).sName, sLibUrl % u"/authorsequence/"_s % sIdAuthor % u"/"_s % QString::number(iSeria.first) % sSessionQuery, u"block"_s);
        AddTextNode(u"div"_s, QString::number(iSeria.second) % u" "_s % tr("books in sequence"), entry);
    }
    return responseHTML();
}

QHttpServerResponse opds_server::authorSequencesOPDS(uint idLib, uint idAuthor, const QHttpServerRequest &request)
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
    QDomElement feed = docHeaderOPDS(tr("Book sequences") % u" "_s % pLib->authors.at(idAuthor).getName(), u"tag:author:"_s + sIdAuthor, sLibUrl, sSession);

    SerialComparator comporator(pLib->serials);
    std::map<uint, uint, SerialComparator> mCountBooks(comporator);
    auto range = pLib->authorBooksLink.equal_range(idAuthor);
    for (auto it = range.first; it != range.second; ++it) {
        const SBook& book = pLib->books.at(it->second);
        if(book.bDeleted) [[unlikely]]
            continue;
        if(!book.mSequences.empty()){
            if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == pLib->vLaguages[book.idLanguage])
                for(const auto &iSequenc :book.mSequences)
                    ++mCountBooks[iSequenc.first];
        }
    }

    for(const auto &iSeria :mCountBooks)
    {
        QString sIdSerial = QString::number(iSeria.first);
        addEntry(feed, u"tag:author:"_s % sIdAuthor % u":sequence:"_s % sIdSerial, sLibUrl % u"/authorsequence/"_s % sIdAuthor % u"/"_s % sIdSerial % sSessionQuery,
                 pLib->serials.at(iSeria.first).sName, QString::number(iSeria.second) % u" "_s % tr("books in sequence"));
    }
    QHttpServerResponse result(doc_.toString());
    return result;
}

QHttpServerResponse opds_server::authorSequencesOPDS2(uint idLib, uint idAuthor, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    const SLib *pLib = getLib(idLib, u"/opds2"_s, &sLibUrl);
    if(pLib == nullptr || !pLib->authors.contains(idAuthor)) [[unlikely]]
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSessionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    QString sIdAuthor = QString::number(idAuthor);
    auto root = docHeaderOPDS2(tr("Book sequences") % u" "_s % pLib->authors.at(idAuthor).getName(), sLibUrl, sSession);

    SerialComparator comporator(pLib->serials);
    std::map<uint, uint, SerialComparator> mCountBooks(comporator);
    auto range = pLib->authorBooksLink.equal_range(idAuthor);
    for (auto it = range.first; it != range.second; ++it) {
        const SBook& book = pLib->books.at(it->second);
        if(book.bDeleted) [[unlikely]]
            continue;
        if(!book.mSequences.empty()){
            if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == pLib->vLaguages[book.idLanguage])
                for(const auto &iSequenc :book.mSequences)
                    ++mCountBooks[iSequenc.first];
        }
    }

    QJsonArray navigation;
    for(const auto &iSeria :mCountBooks)
    {
        QString sIdSerial = QString::number(iSeria.first);
        addNavigation(navigation, pLib->serials.at(iSeria.first).sName, sLibUrl % u"/authorsequence/"_s % sIdAuthor % u"/"_s % sIdSerial % sSessionQuery, iSeria.second);
    }
    root[u"navigation"] = navigation;

    QJsonDocument doc(root);
    QHttpServerResponse result("application/opds+json; charset=utf-8"_ba, doc.toJson(QJsonDocument::Compact));
    return result;
}

QHttpServerResponse opds_server::authorSequencesHTML(uint idLib, uint idAuthor, uint idSequence, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u""_s, &sLibUrl);
    if(pLib == nullptr || !pLib->authors.contains(idAuthor) || !pLib->serials.contains(idSequence))
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    std::vector<uint> vBooks = book_list(*pLib, idAuthor, idSequence, 0);
    return generatePageHTML(vBooks, *pLib, tr("Books of sequence") % u" ("_s % pLib->serials[idSequence].sName % u")"_s, sLibUrl, url, false);
}

QHttpServerResponse opds_server::authorSequencesOPDS(uint idLib, uint idAuthor, uint idSequence, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u"/opds"_s, &sLibUrl);
    if(pLib == nullptr || !pLib->authors.contains(idAuthor) || !pLib->serials.contains(idSequence)) [[unlikely]]
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    std::vector<uint> vBooks = book_list(*pLib, idAuthor, idSequence, 0);
    QString sPage = generatePageOPDS(vBooks, *pLib, tr("Books of sequence") % u" ("_s % pLib->serials[idSequence].sName % u")"_s,
                                 u"tag:author:"_s % QString::number(idAuthor) % u":sequence:"_s % QString::number(idSequence), sLibUrl, url);
    QHttpServerResponse result(sPage);
    return result;
}

QHttpServerResponse opds_server::authorSequencesOPDS2(uint idLib, uint idAuthor, uint idSequence, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u"/opds2"_s, &sLibUrl);
    if(pLib == nullptr || !pLib->authors.contains(idAuthor) || !pLib->serials.contains(idSequence)) [[unlikely]]
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    std::vector<uint> vBooks = book_list(*pLib, idAuthor, idSequence, 0);
    QHttpServerResponse result = generatePageOPDS2(vBooks, *pLib, tr("Books of sequence") % u" ("_s % pLib->serials[idSequence].sName % u")"_s, sLibUrl, url);
    return result;
}

QHttpServerResponse opds_server::authorSequencelessHTML(uint idLib, uint idAuthor, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u""_s, &sLibUrl);
    if(pLib == nullptr || !pLib->authors.contains(idAuthor)) [[unlikely]]
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    std::vector<uint> vBooks = book_list(*pLib, idAuthor, 0, true);
    return generatePageHTML(vBooks, *pLib, tr("Books without sequence") % u" ("_s % pLib->authors[idAuthor].getName() % u")"_s, sLibUrl, url, false);
}

QHttpServerResponse opds_server::authorSequencelessOPDS(uint idLib, uint idAuthor, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u"/opds"_s, &sLibUrl);
    if(pLib == nullptr || !pLib->authors.contains(idAuthor)) [[unlikely]]
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    std::vector<uint> vBooks = book_list(*pLib, idAuthor, 0, true);
    QString sPage = generatePageOPDS(vBooks, *pLib, tr("Books without sequence") % u" ("_s % pLib->authors[idAuthor].getName() % u")"_s,
                                 u"tag:author:"_s % QString::number(idAuthor) % u":sequenceless"_s, sLibUrl, url);
    QHttpServerResponse result(sPage);
    return result;
}

QHttpServerResponse opds_server::authorSequencelessOPDS2(uint idLib, uint idAuthor, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u"/opds2"_s, &sLibUrl);
    if(pLib == nullptr || !pLib->authors.contains(idAuthor)) [[unlikely]]
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    std::vector<uint> vBooks = book_list(*pLib, idAuthor, 0, true);
    QHttpServerResponse result = generatePageOPDS2(vBooks, *pLib, tr("Books without sequence") % u" ("_s % pLib->authors[idAuthor].getName() % u")"_s, sLibUrl, url);
    return result;
}

QHttpServerResponse opds_server::sequencesIndexHTML(uint idLib, const QString &sIndex, bool bByBooks, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    const SLib *pLib = getLib(idLib, u""_s, &sLibUrl);
    if(pLib == nullptr) [[unlikely]]
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSessionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    QDomElement feed;
    feed = docHeaderHTML(sSessionQuery, pLib->name, sLibUrl);
    std::map<QString, int, LocaleAwareQStringComparator> mCount;

    std::unordered_set<QString> stSerials;
    std::unordered_set<uint>stIdSerials;
    int count = 0;
    QString sLowerIndex = sIndex.toCaseFolded();
    for(const auto &iBook :pLib->books){
        const auto &book = iBook.second;
        if(book.bDeleted) [[unlikely]]
            continue;
        if(!book.mSequences.empty()){
            if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == pLib->vLaguages[book.idLanguage])
                for(const auto &iSequenc :book.mSequences)
                    stIdSerials.insert(iSequenc.first);
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
        attachSearchFormHTML(feed, tr("Finding sequence") + u": "_s, sLibUrl + u"/searchsequence"_s, sIndex, sSession);

        QDomElement tag_table;
        QDomElement tag_tr;
        int current_column = 0;
        tag_table = doc_.createElement(u"TABLE"_s);
        feed.appendChild(tag_table);
        for(const auto &iIndex :mCount)
        {
            if(iIndex.first.trimmed().isEmpty() || iIndex.first[0] == '\0')
                continue;

            if(current_column == 0)
            {
                tag_tr = doc_.createElement(u"tr"_s);
                tag_table.appendChild(tag_tr);
            }
            QDomElement td = doc_.createElement(u"td"_s);
            tag_tr.appendChild(td);

            QDomElement div = doc_.createElement(u"DIV"_s);
            div.setAttribute(u"class"_s, u"item"_s);
            td.appendChild(div);
            QDomElement el = AddTextNode(u"a"_s, iIndex.first, div);
            el.setAttribute(u"class"_s, u"block"_s);
            AddTextNode(u"div"_s, QString::number(iIndex.second) % u" "_s % tr("series beginning with") % u" '"_s %
                        iIndex.first % u"'"_s, div);
            if(iIndex.second == 1)
            {
                for(auto &idSerial :stIdSerials){
                    const auto &serial = pLib->serials.at(idSerial);
                    if(serial.sName.left(iIndex.first.size()).toCaseFolded() == iIndex.first.toCaseFolded())
                    {
                        el.setAttribute(u"href"_s, sLibUrl % u"/sequencebooks/"_s % QString::number(idSerial) % sSessionQuery);
                        break;
                    }
                }
            }
            else
            {
                el.setAttribute(u"href"_s, sLibUrl % u"/sequencesindex/"_s % QString::fromUtf8(QUrl::toPercentEncoding(iIndex.first, ""_ba, "."_ba).constData()) %
                                (stSerials.contains(iIndex.first) && iIndex.second<30 ?u"/books"_s :u""_s) % sSessionQuery );
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
                const auto &book = iBook.second;
                if(book.bDeleted) [[unlikely]]
                    continue;
                if(book.mSequences.contains(idSerial))
                    if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == pLib->vLaguages[book.idLanguage])
                        nBooksCount++;
            }
            QDomElement div = doc_.createElement(u"DIV"_s);
            div.setAttribute(u"class"_s, u"item"_s);
            feed.appendChild(div);
            addHRefNode(div, pLib->serials.at(idSerial).sName, sLibUrl % u"/sequencebooks/"_s % QString::number(idSerial) % sSessionQuery, u"block"_s);
            AddTextNode(u"div"_s, QString::number(nBooksCount) % u" "_s % tr("books"), div);
        }
    }

    return responseHTML();
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
    QString sSessionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    QDomElement feed = docHeaderOPDS(tr("Books by sequences"), u"tag:root:sequences"_s, sLibUrl, sSession);

    std::map<QString, int, LocaleAwareQStringComparator> mCount;
    std::unordered_set<QString> stSerials;
    std::unordered_set<uint>stIdSerials;
    int count = 0;
    QString sLowerIndex = sIndex.toCaseFolded();

    for(const auto &iBook :pLib->books){
        const auto &book = iBook.second;
        if(book.bDeleted) [[unlikely]]
            continue;
        if(!book.mSequences.empty()){
            if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == pLib->vLaguages[book.idLanguage])
                for(const auto &iSequence :book.mSequences)
                    stIdSerials.insert(iSequence.first);
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

            QString sContent = QString::number(iIndex.second) % u" "_s % tr("series beginning with") % u" '"_s % iIndex.first % u"'"_s;
            QString sHRef;
            if(iIndex.second == 1)
            {
                for(auto &idSerial :stIdSerials){
                    const auto &serial = pLib->serials.at(idSerial);
                    if(stSerials.contains(iIndex.first) ?serial.sName.toCaseFolded() == iIndex.first.toCaseFolded()
                            :serial.sName.left(iIndex.first.size()).toCaseFolded() == iIndex.first.toCaseFolded())
                    {
                        sHRef = sLibUrl % u"/sequencebooks/"_s % QString::number(idSerial) % sSessionQuery;
                        break;
                    }
                }
            }
            else
            {
                sHRef = sLibUrl % u"/sequencesindex/"_s %  QString::fromUtf8(QUrl::toPercentEncoding(iIndex.first, ""_ba, "."_ba).constData())
                        % (stSerials.contains(iIndex.first) ?u"/books"_s :u""_s) % sSessionQuery;
            }
            addEntry(feed, u"tag:sequences:"_s + iIndex.first, sHRef, iIndex.first, sContent);
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
                const auto &book = iBook.second;
                if(book.bDeleted) [[unlikely]]
                    continue;
                if(book.mSequences.contains(iIndex))
                    if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == pLib->vLaguages[iBook.second.idLanguage])                            
                        nBooksCount++;
            }
            addEntry(feed, u"tag:sequences:"_s + QString::number(iIndex), sLibUrl % u"/sequencebooks/"_s % QString::number(iIndex) % sSessionQuery, pLib->serials.at(iIndex).sName, u""_s);
        }
    }
    QHttpServerResponse result(doc_.toString());
    return result;
}

QHttpServerResponse opds_server::sequencesIndexOPDS2(uint idLib, const QString &sIndex, bool bByBooks, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    const SLib *pLib = getLib(idLib, u"/opds2"_s, &sLibUrl);
    if(pLib == nullptr)
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QUrlQuery urlquery(url);
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSessionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    auto root = docHeaderOPDS2(tr("Books by sequences"), sLibUrl, sSession);

    std::map<QString, int, LocaleAwareQStringComparator> mCount;
    std::unordered_set<QString> stSerials;
    std::unordered_set<uint>stIdSerials;
    int count = 0;
    QString sLowerIndex = sIndex.toCaseFolded();

    for(const auto &iBook :pLib->books){
        const auto &book = iBook.second;
        if(book.bDeleted) [[unlikely]]
            continue;
        if(!book.mSequences.empty()){
            if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == pLib->vLaguages[book.idLanguage])
                for(const auto &iSequence :book.mSequences)
                    stIdSerials.insert(iSequence.first);
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

    QJsonArray navigation;
    if(count > 30 && !bByBooks)
    {
        QDomElement tag_table;
        QDomElement tag_tr;
        for(const auto &iIndex :mCount)
        {
            if(iIndex.first.trimmed().isEmpty() || iIndex.first[0] == '\0')
                continue;

            QString sHRef;
            if(iIndex.second == 1)
            {
                for(auto &idSerial :stIdSerials){
                    const auto &serial = pLib->serials.at(idSerial);
                    if(stSerials.contains(iIndex.first) ?serial.sName.toCaseFolded() == iIndex.first.toCaseFolded()
                            :serial.sName.left(iIndex.first.size()).toCaseFolded() == iIndex.first.toCaseFolded())
                    {
                        sHRef = sLibUrl % u"/sequencebooks/"_s % QString::number(idSerial) % sSessionQuery;
                        break;
                    }
                }
            }
            else
            {
                sHRef = sLibUrl % u"/sequencesindex/"_s %  QString::fromUtf8(QUrl::toPercentEncoding(iIndex.first, ""_ba, "."_ba).constData())
                        % (stSerials.contains(iIndex.first) ?u"/books"_s :u""_s) % sSessionQuery;
            }
            addNavigation(navigation, iIndex.first, sHRef, iIndex.second);
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
                const auto &book = iBook.second;
                if(book.bDeleted) [[unlikely]]
                    continue;
                if(book.mSequences.contains(iIndex))
                    if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == pLib->vLaguages[book.idLanguage])
                        nBooksCount++;
            }
            addNavigation(navigation, pLib->serials.at(iIndex).sName, sLibUrl % u"/sequencebooks/"_s % QString::number(iIndex) % sSessionQuery, nBooksCount);
        }
    }
    root[u"navigation"] = navigation;

    QJsonDocument doc(root);
    QHttpServerResponse result("application/opds+json; charset=utf-8"_ba, doc.toJson(QJsonDocument::Compact));
    return result;
}

QHttpServerResponse opds_server::sequenceBooksHTML(uint idLib, uint idSequence, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u""_s, &sLibUrl);
    if(pLib == nullptr || !pLib->serials.contains(idSequence))
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    std::vector<uint> vBooks = book_list(*pLib, 0, idSequence, 0);
    return generatePageHTML(vBooks, *pLib, tr("Books of sequence") % u" ("_s % pLib->serials[idSequence].sName % u")"_s, sLibUrl, url, false);
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

    std::vector<uint> vBooks = book_list(*pLib, 0, idSequence, 0);
    QString sPage = generatePageOPDS(vBooks, *pLib, tr("Books of sequence") % u" ("_s % pLib->serials[idSequence].sName % u")"_s, u"tag:sequences:"_s + QString::number(idSequence), sLibUrl, url);
    QHttpServerResponse result(sPage);
    return result;
}

QHttpServerResponse opds_server::sequenceBooksOPDS2(uint idLib, uint idSequence, const QHttpServerRequest &request)
{
    QUrl url;
    if(!checkAuth(request, url))
        return responseUnauthorized();
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u"/opds2"_s, &sLibUrl);
    if(pLib == nullptr || !pLib->serials.contains(idSequence)) [[unlikely]]
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    std::vector<uint> vBooks = book_list(*pLib, 0, idSequence, 0);
    QHttpServerResponse result = generatePageOPDS2(vBooks, *pLib, tr("Books of sequence") % u" ("_s % pLib->serials[idSequence].sName % u")"_s, sLibUrl, url);
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

std::unordered_map<ushort, uint> opds_server::countBooksByGenre(const SLib &lib, ushort idParentGenre)
{
    std::unordered_map<ushort, uint> mCounts;
    for(const auto &iBook :lib.books){
        if(!iBook.second.bDeleted) [[likely]]{
            for(auto iGenre: iBook.second.vIdGenres){
                if(g::genres.at(iGenre).idParrentGenre == idParentGenre){
                    if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == lib.vLaguages[iBook.second.idLanguage])
                        ++mCounts[iGenre];
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
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSessionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    Q_ASSERT(!pLib->serials.contains(0));
    std::vector<ushort> vIdGenres;
    std::unordered_map<ushort, uint> mCounts;

    if(idParentGenre != 0)
    {
        if(g::genres[idParentGenre].idParrentGenre > 0){
            std::vector<uint> vBooks = listGenreBooks(*pLib, idParentGenre);
            return generatePageHTML(vBooks, *pLib, tr("Books by genre") % u": " % g::genres[idParentGenre].sName, sLibUrl, url, true);
        }

        mCounts = countBooksByGenre(*pLib, idParentGenre);
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
    feed = docHeaderHTML(sSessionQuery, pLib->name, sLibUrl);

    for(auto idGenre: vIdGenres)
    {
        uint nCount = mCounts[idGenre];
        QDomElement div = doc_.createElement(u"div"_s);
        feed.appendChild(div);
        div.setAttribute(u"class"_s, u"item"_s);
        addHRefNode(div, g::genres[idGenre].sName, sLibUrl % u"/genres/"_s % QString::number(idGenre) % sSessionQuery, u"block"_s);
        if(idParentGenre != 0)
            AddTextNode(u"div"_s, QString::number(nCount) % u" "_s % tr("books"), div);
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
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSessionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;


    std::vector<ushort> vIdGenres;
    std::unordered_map<ushort, uint> mCounts;
    if(idParentGenre != 0)
    {
        if(g::genres[idParentGenre].idParrentGenre > 0){
            std::vector<uint> vBooks = listGenreBooks(*pLib, idParentGenre);
            QString sPage =  generatePageOPDS(vBooks, *pLib, tr("Books by genre") % u": " % g::genres[idParentGenre].sName, u""_s, sLibUrl, url);
            QHttpServerResponse result(sPage);
            return result;
        }

        mCounts = countBooksByGenre(*pLib, idParentGenre);
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
        addEntry(feed, u"tag:root:genre:"_s + g::genres[idGenre].sName, sLibUrl % u"/genres/"_s % QString::number(idGenre) % sSessionQuery,
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
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSessionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    std::vector<ushort> vIdGenres;
    std::unordered_map<ushort, uint> mCounts;
    if(idParentGenre != 0)
    {
        if(g::genres[idParentGenre].idParrentGenre > 0){
            std::vector<uint> vBooks = listGenreBooks(*pLib, idParentGenre);
            QHttpServerResponse result = generatePageOPDS2(vBooks, *pLib, tr("Books by genre") % u": " % g::genres[idParentGenre].sName, sLibUrl, url);
            return result;
        }
        mCounts = countBooksByGenre(*pLib, idParentGenre);
        vIdGenres.reserve(mCounts.size());
        std::ranges::copy(mCounts | std::views::keys, std::back_inserter(vIdGenres));
    }
    else
    {
        auto filtered = g::genres | std::views::filter([](const auto& iGenre) {return iGenre.second.idParrentGenre == 0;}) | std::views::keys;
        vIdGenres.assign(filtered.begin(), filtered.end());
    }

    std::ranges::sort(vIdGenres, [&](ushort id1, ushort id2){return g::genres.at(id1).sName < g::genres.at(id2).sName;});
    QJsonObject root = docHeaderOPDS2(tr("Books by genre"), sLibUrl, sSession);

    QJsonArray navigation;
    for(auto idGenre: vIdGenres)
    {
        uint nCount = idParentGenre != 0 ?mCounts[idGenre] :0;
        addNavigation(navigation, g::genres[idGenre].sName, sLibUrl % u"/genres/"_s % QString::number(idGenre) % sSessionQuery, nCount);
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
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSessionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    std::vector<uint> vBooks = searchTitle(*pLib, sSearchString);
    QDomElement feed;
    feed = docHeaderHTML(sSessionQuery, pLib->name, sLibUrl);
    attachSearchFormHTML(feed, tr("Finding books by title") + u": "_s, sLibUrl + u"/searchtitle"_s, sSearchString, sSession);

    fillPageHTML(vBooks, *pLib, feed, sLibUrl, sSessionQuery, url, true);
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
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSessionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    auto vAuthors = searchAuthors(*pLib, sSearchString);
    std::sort(vAuthors.begin(), vAuthors.end(), [pLib](uint id1, uint id2)
              { return localeStringCompare(pLib->authors.at(id1).getName(), pLib->authors.at(id2).getName()); });

    QDomElement feed;
    feed = docHeaderHTML(sSessionQuery, pLib->name, sLibUrl);

    attachSearchFormHTML(feed, tr("Finding authors") + u": "_s, sLibUrl + u"/searchauthor"_s, sSearchString, sSession);

    for(auto idAuthor :vAuthors){
        uint nBooksCount = 0;
        auto range = pLib->authorBooksLink.equal_range(idAuthor);
        for (auto it = range.first; it != range.second; ++it) {
            auto &book = pLib->books.at(it->second);
            if(!book.bDeleted) [[likely]]
                if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == pLib->vLaguages[book.idLanguage])
                    nBooksCount++;
        }
        if(nBooksCount >0 ){
            QDomElement div = doc_.createElement(u"DIV"_s);
            div.setAttribute(u"class"_s, u"item"_s);
            feed.appendChild(div);
            addHRefNode(div, pLib->authors.at(idAuthor).getName(), sLibUrl % u"/author/"_s % QString::number(idAuthor) % sSessionQuery, u"block"_s);
            AddTextNode(u"div"_s, QString::number(nBooksCount) % u" "_s % tr("books"), div);
        }
    }

    return responseHTML();
}

QHttpServerResponse opds_server::searchSequenceHTML(uint idLib, const QHttpServerRequest &request)
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
    QString sSession = urlquery.queryItemValue(u"session"_s);
    QString sSessionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;

    QDomElement feed;
    feed = docHeaderHTML(sSessionQuery, pLib->name, sLibUrl);
    attachSearchFormHTML(feed, tr("Finding sequence") + u": "_s, sLibUrl + u"/searchsequence"_s, sSearchString, sSession);

    auto mSequence  = searchSequence(*pLib, sSearchString);
    for(auto iSequence: mSequence){
        QDomElement div = doc_.createElement(u"DIV"_s);
        div.setAttribute(u"class"_s, u"item"_s);
        feed.appendChild(div);
        addHRefNode(div, pLib->serials.at(iSequence.first).sName, sLibUrl % u"/sequencebooks/"_s % QString::number(iSequence.first) % sSessionQuery, u"block"_s);
        AddTextNode(u"div"_s, QString::number(iSequence.second) % u" "_s % tr("books"), div);
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
    QString sSequence = urlquery.queryItemValue(u"sequence"_s, QUrl::FullyDecoded).replace(u'+', u' ');

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

        sHRef = sLibUrl % u"/search?sequence="_s % sSearchString;
        if(!sSession.isEmpty())
            sHRef += u"&session="_s + sSession;
        addEntry(feed, u"tag:search:sequence"_s, sHRef, tr("Finding sequence"), u""_s);

        QHttpServerResponse result("application/atom+xml;charset=utf-8"_ba, doc_.toByteArray());
        return result;
    }
    if(!sSearchAuthor.isEmpty()){
        sSearchAuthor.replace(u'+', u' ');
        auto vAuthors = searchAuthors(*pLib, sSearchAuthor);
        std::sort(vAuthors.begin(), vAuthors.end(), [pLib](uint id1, uint id2)
                  { return localeStringCompare(pLib->authors.at(id1).getName(), pLib->authors.at(id2).getName()) /*< 0*/; });

        QString sSession = urlquery.queryItemValue(u"session"_s);
        QString sSessionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;
        QDomElement feed = docHeaderOPDS(tr("Finding authors"), u"tag:search:authors"_s, sLibUrl, sSession);
        for(auto idAuthor :vAuthors){
            uint nBooksCount = 0;
            auto range = pLib->authorBooksLink.equal_range(idAuthor);
            for (auto it = range.first; it != range.second; ++it) {
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
        auto vBooks = searchTitle(*pLib, sSearchTitle);
        return generatePageOPDS(vBooks, *pLib, tr("Books search"), u""_s, sLibUrl, url);
    }

    if(!sSequence.isEmpty()){
        auto mSequence  = searchSequence(*pLib, sSequence);
        QString sSession = urlquery.queryItemValue(u"session"_s);
        QString sSessionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;
        QDomElement feed = docHeaderOPDS(tr("Finding sequence"), u"tag:search:sequence"_s, sLibUrl, sSession);

        for(auto iSequence: mSequence)
        {
            addEntry(feed, u"tag:sequences:"_s + QString::number(iSequence.first), sLibUrl % u"/sequencebooks/"_s % QString::number(iSequence.first) % sSessionQuery,
                     pLib->serials.at(iSequence.first).sName, QString::number(iSequence.second) % u" "_s % tr("books"));
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
    QUrlQuery urlquery(url);
    QString sLibUrl;
    SLib *pLib = getLib(idLib, u"/opds2"_s, &sLibUrl);
    if(pLib == nullptr) [[unlikely]]
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);

    QString sSearchString = urlquery.queryItemValue(u"query"_s, QUrl::FullyDecoded).replace(u'+', u' ');
    QString sSearchAuthor = urlquery.queryItemValue(u"author"_s, QUrl::FullyDecoded).replace(u'+', u' ');
    QString sSearchTitle = urlquery.queryItemValue(u"title"_s, QUrl::FullyDecoded).replace(u'+', u' ');
    QString sSequence = urlquery.queryItemValue(u"sequence"_s, QUrl::FullyDecoded).replace(u'+', u' ');

    if(!sSearchString.isEmpty() && sSearchAuthor.isEmpty() && sSearchTitle.isEmpty()){
        QString sSession = urlquery.queryItemValue(u"session"_s);
        QJsonObject root = docHeaderOPDS2(pLib->name, sLibUrl, sSession);

        QJsonArray navigation;
        addNavigation(navigation, tr("Finding authors"), sLibUrl % u"/search?author="_s % sSearchString % u"&session="_s + sSession);
        addNavigation(navigation, tr("Finding books by title"), sLibUrl % u"/search?title="_s % sSearchString % u"&session="_s + sSession);
        addNavigation(navigation, tr("Finding sequence"), sLibUrl % u"/search?sequence="_s % sSearchString % u"&session="_s + sSession);
        root[u"navigation"] = navigation;

        QJsonDocument doc(root);
        QHttpServerResponse result("application/opds+json; charset=utf-8"_ba, doc.toJson(QJsonDocument::Compact));
        return result;
    }

    std::vector<uint> vBooks;
    if(!sSearchTitle.isEmpty()){
        if(!sSearchAuthor.isEmpty()){
            vBooks = searchBooks(*pLib, sSearchAuthor, sSearchTitle);
        }else
            vBooks = searchTitle(*pLib, sSearchTitle);
        return generatePageOPDS2(vBooks, *pLib, tr("Books search"), sLibUrl, url);
    }

    if(!sSearchAuthor.isEmpty()){
        auto vAuthors = searchAuthors(*pLib, sSearchAuthor);
        std::sort(vAuthors.begin(), vAuthors.end(), [pLib](uint id1, uint id2)
                  { return localeStringCompare(pLib->authors.at(id1).getName(), pLib->authors.at(id2).getName()) /*< 0*/; });
        QString sSession = urlquery.queryItemValue(u"session"_s);
        QString sSessionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;
        QJsonObject root = docHeaderOPDS2(tr("Finding authors"), sLibUrl, sSession);
        QJsonArray navigation;
        for(auto idAuthor :vAuthors){
            uint nBooksCount = 0;
            auto range = pLib->authorBooksLink.equal_range(idAuthor);
            for (auto it = range.first; it != range.second; ++it) {
                auto &book = pLib->books.at(it->second);
                if(!book.bDeleted) [[likely]]
                    if(sLanguageFilter_.isEmpty() || sLanguageFilter_ == pLib->vLaguages[book.idLanguage])
                        nBooksCount++;
            }
            if(nBooksCount >0 ) [[likely]]{
                addNavigation(navigation, pLib->authors.at(idAuthor).getName(), sLibUrl % u"/author/"_s % QString::number(idAuthor) % sSessionQuery);
            }
            root[u"navigation"] = navigation;

        }
        QJsonDocument doc(root);
        QHttpServerResponse result("application/opds+json; charset=utf-8"_ba, doc.toJson(QJsonDocument::Compact));
        return result;
    }
    if(!sSequence.isEmpty()){
        auto mSequence  = searchSequence(*pLib, sSequence);
        QString sSession = urlquery.queryItemValue(u"session"_s);
        QString sSessionQuery = sSession.isEmpty() ?u""_s :u"?session="_s + sSession;
        QJsonObject root = docHeaderOPDS2(tr("Finding sequence"), sLibUrl, sSession);
        QJsonArray navigation;

        for(auto iSequence: mSequence)
            addNavigation(navigation, pLib->serials.at(iSequence.first).sName, sLibUrl % u"/sequencebooks/"_s % QString::number(iSequence.first) % sSessionQuery, iSequence.second);

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
    BookFile bookFile(pLib, idBook);
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
            if(book.sFormat != sFormat){
                QFile file;
                file.setFileName(QDir::tempPath() % u"/freeLib/book0."_s % book.sFormat);
                file.open(QFile::WriteOnly);
                file.write(baBook);
                file.close();
                QFileInfo fi(file);

                fb2mobi conv(pExportOptions, idLib);
                QString sOutFile = conv.convert({fi.absoluteFilePath()}, idBook);
                file.setFileName(sOutFile);
                file.open(QFile::ReadOnly);
                baBook = file.readAll();
            }
            if(format == epub){
                baContentType = "application/epub+zip"_ba;
            }
            else{
                baContentType = "application/x-mobipocket-ebook"_ba;
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
