#ifndef OPDS_SERVER_H
#define OPDS_SERVER_H

#include <vector>
#include <QObject>
#include <QTcpServer>
#include <QDomDocument>
#include <QDateTime>
#include <QHttpServer>

#include "library.h"
#include "options.h"

class opds_server : public QObject
{
    Q_OBJECT
public:
    explicit opds_server(QObject *parent = nullptr);
    void server_run();

    void setLanguageFilter(const QString &sLanguage);

private:
    QDomElement AddTextNode(const QString &name, const QString &text, QDomNode &node);

    void addTextNode(QDomNode &node, const QString &sName, const QString &sText, const QString &sClass);
    void addHRefNode(QDomNode &node, const QString &sText, const QString &sHRef, const QString &sClass);
    void addDownloadItem(QDomNode &node, const QString &sText, const QString &sHRef);
    static void addNavigation(QJsonArray &navigation, const QString &sTitle, const QString &sHRef, uint nCount=0);
    static void addLink(QJsonArray &links, const QString &sType, const QString &sHRef, const QString &sRel);
    void addLink(QDomNode &node, const QString &sType, const QString &sHRef, const QString &sRel, const QString &sTitle);
    void addLink(QDomNode &node, const QString &sType, const QString &sHRef, const QString &sRel);
    void addLink(QDomNode &node, const QString &sType, const QString &sHRef);
    void addEntry(QDomElement &feed, const QString &sId, const QString &sHRef, const QString &sTitle, const QString &sContent);

    std::vector<uint> book_list(const SLib& lib, uint idAuthor, uint idSeria, bool sequenceless);
    std::vector<uint> listGenreBooks(const SLib& lib, ushort idGenre);
    std::unordered_map<ushort, uint> countBooksByGenre(const SLib &lib, ushort idParentGenre);
    std::vector<uint> searchTitle(const SLib &lib, const QString &sTitle);
    std::vector<uint> searchBooks(const SLib& lib, const QString &sAuthor, const QString &sTitle);
    static std::vector<uint> searchAuthors(const SLib &lib, const QString &sAuthor);

    void stop_server();

    auto searchSequence(const SLib& lib, const QString &sSequence);

    static void loadAnnotations(const std::vector<uint> &vBooks, SLib &lib, uint begin, uint end);
    QHttpServerResponse generatePageHTML(const std::vector<uint> &vBooks, SLib &lib, const QString &sTitle, const QString &sLibUrl, const QUrl &url, bool bShowAuthor);
    void fillPageHTML(const std::vector<uint> &vBooks, SLib &lib, QDomElement &feed, const QString &sLibUrl, const QString &sSessionQuery, const QUrl &url, bool bShowAuthor);
    QString generatePageOPDS(const std::vector<uint> &vBooks, SLib &lib, const QString &sTitle, const QString &sId, const QString &sLibUrl, const QUrl &url);
    QHttpServerResponse generatePageOPDS2(const std::vector<uint> &vBooks, SLib &lib, const QString &sTitle, const QString &sLibUrl, const QUrl &url);

    bool checkAuth(const QHttpServerRequest &request, QUrl &url);
    QDomElement docHeaderHTML(const QString &sSesionQuery, const QString &sLibName, const QString &sLibUrl);
    QDomElement docHeaderOPDS(const QString &sTitle, const QString &sID, const QString &sLibUrl, const QString &sSession);
    QJsonObject docHeaderOPDS2(const QString &sTitle, const QString &sLibUrl, const QString &sSession);

    SLib* getLib(uint &idLib, const QString &sTypeServer = u"opds"_s, QString *pLibUrl = nullptr);
    QHttpServerResponse responseHTML();
    QHttpServerResponse responseUnauthorized();
    QHttpServerResponse rootHTML(uint idLib, const QHttpServerRequest &request);
    QHttpServerResponse rootOPDS(uint idLib, const QHttpServerRequest &request);
    QHttpServerResponse rootOPDS2(uint idLib, const QHttpServerRequest &request);
    QHttpServerResponse authorsIndexHTML(uint idLib, const QString &sIndex, bool bByBooks, const QHttpServerRequest &request);
    QHttpServerResponse authorsIndexOPDS(uint idLib, const QString &sIndex, bool bByBooks, const QHttpServerRequest &request);
    QHttpServerResponse authorsIndexOPDS2(uint idLib, const QString &sIndex, bool bByBooks, const QHttpServerRequest &request);
    QHttpServerResponse authorHTML(uint idLib, uint idAuthor, const QHttpServerRequest &request);
    QHttpServerResponse authorOPDS(uint idLib, uint idAuthor, const QHttpServerRequest &request);
    QHttpServerResponse authorOPDS2(uint idLib, uint idAuthor, const QHttpServerRequest &request);
    QHttpServerResponse authorBooksHTML(uint idLib, uint idAuthor, const QHttpServerRequest &request);
    QHttpServerResponse authorBooksOPDS(uint idLib, uint idAuthor, const QHttpServerRequest &request);
    QHttpServerResponse authorBooksOPDS2(uint idLib, uint idAuthor, const QHttpServerRequest &request);
    QHttpServerResponse authorSequencesHTML(uint idLib, uint idAuthor, const QHttpServerRequest &request);
    QHttpServerResponse authorSequencesOPDS(uint idLib, uint idAuthor, const QHttpServerRequest &request);
    QHttpServerResponse authorSequencesOPDS2(uint idLib, uint idAuthor, const QHttpServerRequest &request);
    QHttpServerResponse authorSequencesHTML(uint idLib, uint idAuthor, uint idSequence, const QHttpServerRequest &request);
    QHttpServerResponse authorSequencesOPDS(uint idLib, uint idAuthor, uint idSequence, const QHttpServerRequest &request);
    QHttpServerResponse authorSequencesOPDS2(uint idLib, uint idAuthor, uint idSequence, const QHttpServerRequest &request);
    QHttpServerResponse authorSequencelessHTML(uint idLib, uint idAuthor, const QHttpServerRequest &request);
    QHttpServerResponse authorSequencelessOPDS(uint idLib, uint idAuthor, const QHttpServerRequest &request);
    QHttpServerResponse authorSequencelessOPDS2(uint idLib, uint idAuthor, const QHttpServerRequest &request);
    QHttpServerResponse sequencesIndexHTML(uint idLib, const QString &sIndex, bool bByBooks, const QHttpServerRequest &request);
    QHttpServerResponse sequencesIndexOPDS(uint idLib, const QString &sIndex, bool bByBooks, const QHttpServerRequest &request);
    QHttpServerResponse sequencesIndexOPDS2(uint idLib, const QString &sIndex, bool bByBooks, const QHttpServerRequest &request);
    QHttpServerResponse sequenceBooksHTML(uint idLib, uint idSequence, const QHttpServerRequest &request);
    QHttpServerResponse sequenceBooksOPDS(uint idLib, uint idSequence, const QHttpServerRequest &request);
    QHttpServerResponse sequenceBooksOPDS2(uint idLib, uint idSequence, const QHttpServerRequest &request);
    QHttpServerResponse bookHTML(uint idLib, uint idBook, const QString &sFormat);
    QHttpServerResponse bookOPDS(uint idLib, uint idBook, const QString &sFormat);
    QHttpServerResponse genresHTML(uint idLib, ushort idParentGenre, const QHttpServerRequest &request);
    void attachSearchFormHTML(QDomElement &feed, const QString &sTitle, const QString &sAction, const QString &sSearch, const QString &sSession);
    QHttpServerResponse genresOPDS(uint idLib, ushort idParentGenre, const QHttpServerRequest &request);
    QHttpServerResponse genresOPDS2(uint idLib, ushort idParentGenre, const QHttpServerRequest &request);
    QHttpServerResponse searchTitleHTML(uint idLib, const QHttpServerRequest &request);
    QHttpServerResponse searchAuthorHTML(uint idLib, const QHttpServerRequest &request);
    QHttpServerResponse searchSequenceHTML(uint idLib, const QHttpServerRequest &request);
    QHttpServerResponse searchOPDS(uint idLib, const QHttpServerRequest &request);
    QHttpServerResponse searchOPDS2(uint idLib, const QHttpServerRequest &request);

    QByteArray cover(uint id, uint idBook);
    QHttpServerResponse convert(uint idLib, uint idBook, const QString &sFormat, bool opds);

    enum Status : int {stoped, run};
    Status status_;
    int nPort_;
    QDomDocument doc_;
    std::unordered_map<QString, QDateTime> sessions_;
    ExportOptions *pExportOptions_;
    QString sLanguageFilter_;
    QHttpServer httpServer_;
    QTcpServer tcpServer_;
};

#endif // OPDS_SERVER_H
