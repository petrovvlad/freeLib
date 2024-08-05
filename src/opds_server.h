#ifndef OPDS_SERVER_H
#define OPDS_SERVER_H

#include <QObject>
#include <QTcpServer>
#include <QDomDocument>
#include <QDateTime>
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
#include <QHttpServer>
#endif

#include "library.h"
#include "options.h"

#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
using namespace Qt::Literals::StringLiterals;
#endif

class opds_server : public QObject
{
    Q_OBJECT
public:
    explicit opds_server(QObject *parent = nullptr);
    void server_run();

    void setLanguageFilter(const QString &sLanguage);
#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)

    void process(QString url, QTextStream& ts, const QString &session);

private slots:
    void new_connection();
    void slotRead();
#endif

private:
    QDomElement AddTextNode(const QString &name, const QString &text, QDomNode &node);

#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
    void addTextNode(QDomNode &node, const QString &sName, const QString &sText, const QString &sClass);
    void addHRefNode(QDomNode &node, const QString &sText, const QString &sHRef, const QString &sClass);
    static void addNavigation(QJsonArray &navigation, const QString &sTitle, const QString &sHRef, uint nCount=0);
    static void addLink(QJsonArray &links, const QString sType, const QString &sHRef, const QString &sRel);
    void addLink(QDomNode &node, const QString sType, const QString &sHRef, const QString &sRel, const QString &sTitle);
    void addLink(QDomNode &node, const QString sType, const QString &sHRef, const QString &sRel);
    void addLink(QDomNode &node, const QString sType, const QString &sHRef);
    void addEntry(QDomElement &feed, const QString &sId, const QString &sHRef, const QString &sTitle, const QString &sContent);

#endif
    std::vector<uint> book_list(const SLib& lib, uint idAuthor, uint idSeria, ushort idGenre, const QString &sSearch, bool sequenceless);
    void stop_server();

#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
    std::vector<uint> searchBooks(const SLib& lib, QStringView sAuthor, QStringView sTitle);
    auto searchSequence(const SLib& lib, QStringView sSequence);
    static std::vector<uint> searchAuthors(const SLib &lib, const QStringView sAuthor);

    static void loadAnnotations(const std::vector<uint> &vBooks, SLib &lib, uint begin, uint end);
    QHttpServerResponse FillPageHTML(const std::vector<uint> &vBooks, SLib &lib, const QString &sTitle, const QString &sLibUrl, const QUrl &url, bool bShowAuthor);
    QString fillPageOPDS(const std::vector<uint> &vBooks, SLib &lib, const QString &sTitle, const QString &sId, const QString &sLibUrl, const QUrl &url);
    QHttpServerResponse fillPageOPDS2(const std::vector<uint> &vBooks, SLib &lib, const QString &sTitle, const QString &sLibUrl, const QUrl &url);

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
    QHttpServerResponse searchHTML(uint idLib, const QHttpServerRequest &request);
    QHttpServerResponse searchAuthorHTML(uint idLib, const QHttpServerRequest &request);
    QHttpServerResponse searchSequenceHTML(uint idLib, const QHttpServerRequest &request);
    QHttpServerResponse searchOPDS(uint idLib, const QHttpServerRequest &request);
    QHttpServerResponse searchOPDS2(uint idLib, const QHttpServerRequest &request);

    QByteArray cover(uint id, uint idBook);
    QHttpServerResponse convert(uint idLib, uint idBook, const QString &sFormat, bool opds);
#else
    QDomElement doc_header(const QString &session, bool html=false, const QString &lib_name = QString(), const QString &lib_url = QString());
    QString FillPage(std::vector<uint> listBooks, SLib& lib, const QString &sTitle, const QString &lib_url, const QString &current_url, QTextStream& ts, bool opds, uint nPage, const QString &session, bool bShowAuthor);
    void convert(uint idLib, uint idBook, const QString &format, const QString &file_name, bool opds, QTextStream &ts);
    QString WriteSuccess(const QString &contentType = QStringLiteral("text/html;charset=utf-8"), bool isGZip=false);
#endif

    enum Status : int {stoped, run};
    Status status_;
    int nPort_;
    QDomDocument doc_;
    std::unordered_map<QString, QDateTime> sessions_;
    ExportOptions *pExportOptions_;
    QString sLanguageFilter_;
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
    QHttpServer httpServer_;
#else
    bool bMobile_;
    bool for_preview;
    QStringList sesions_auth;
    QMap<qintptr, QTcpSocket *> OPDS_clients;
    QMap<QString, QString> params;
    QTcpServer OPDS_server;
#endif
};

#endif // OPDS_SERVER_H
