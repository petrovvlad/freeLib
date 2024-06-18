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

    std::vector<uint> book_list(const SLib& lib, uint idAuthor, uint idSeria, ushort idGenre,  const QString &sSearch, bool sequenceless);
    void stop_server();

#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
    QHttpServerResponse FillPageHTTP(const std::vector<uint> &vBooks, SLib &lib, const QString &sTitle, const QString &sLibUrl, const QUrl &url, bool bShowAuthor);
    QString FillPageOPDS(const std::vector<uint> &vBooks, SLib &lib, const QString &sTitle, const QString &sId, const QString &sLibUrl, const QUrl &url);

    bool checkAuth(const QHttpServerRequest &request, QUrl &url);
    QDomElement docHeaderHTTP(const QString &sSesionQuery, const QString &sLibName, const QString &sLibUrl);
    QDomElement docHeaderOPDS(const QString &sTitle, const QString &sID, const QString &sLibUrl, const QString &sSesionQuery);

    SLib* getLib(uint &idLib, const QString &sTypeServer = u"opds"_s, QString *pLibUrl = nullptr);
    QHttpServerResponse responseHTTP();
    QHttpServerResponse responseUnauthorized();
    QHttpServerResponse rootHTTP(uint idLib, const QHttpServerRequest &request);
    QHttpServerResponse rootOPDS(uint idLib, const QHttpServerRequest &request);
    QByteArray image(const QString &sFile);
    QHttpServerResponse authorsIndexHTTP(uint idLib, const QString &sIndex, bool bByBooks, const QHttpServerRequest &request);
    QHttpServerResponse authorsIndexOPDS(uint idLib, const QString &sIndex, bool bByBooks, const QHttpServerRequest &request);
    QHttpServerResponse authorHTTP(uint idLib, uint idAuthor, const QHttpServerRequest &request);
    QHttpServerResponse authorOPDS(uint idLib, uint idAuthor, const QHttpServerRequest &request);
    QHttpServerResponse authorBooksHTTP(uint idLib, uint idAuthor, const QHttpServerRequest &request);
    QHttpServerResponse authorBooksOPDS(uint idLib, uint idAuthor, const QHttpServerRequest &request);
    QHttpServerResponse authorSequencesHTTP(uint idLib, uint idAuthor, const QHttpServerRequest &request);
    QHttpServerResponse authorSequencesOPDS(uint idLib, uint idAuthor, const QHttpServerRequest &request);
    QHttpServerResponse authorSequencesHTTP(uint idLib, uint idAuthor, uint idSequence, const QHttpServerRequest &request);
    QHttpServerResponse authorSequencesOPDS(uint idLib, uint idAuthor, uint idSequence, const QHttpServerRequest &request);
    QHttpServerResponse authorSequencelessHTTP(uint idLib, uint idAuthor, const QHttpServerRequest &request);
    QHttpServerResponse authorSequencelessOPDS(uint idLib, uint idAuthor, const QHttpServerRequest &request);
    QHttpServerResponse sequencesIndexHTTP(uint idLib, const QString &sIndex, bool bByBooks, const QHttpServerRequest &request);
    QHttpServerResponse sequencesIndexOPDS(uint idLib, const QString &sIndex, bool bByBooks, const QHttpServerRequest &request);
    QHttpServerResponse sequenceBooksHTTP(uint idLib, uint idSequence, const QHttpServerRequest &request);
    QHttpServerResponse sequenceBooksOPDS(uint idLib, uint idSequence, const QHttpServerRequest &request);
    QHttpServerResponse bookHTTP(uint idLib, uint idBook, const QString &sFormat);
    QHttpServerResponse bookOPDS(uint idLib, uint idBook, const QString &sFormat);
    QHttpServerResponse genresHTTP(uint idLib, ushort idParentGenre, const QHttpServerRequest &request);
    QHttpServerResponse genresOPDS(uint idLib, ushort idParentGenre, const QHttpServerRequest &request);
    QHttpServerResponse searchHTTP(uint idLib, const QHttpServerRequest &request);
    QHttpServerResponse searchOPDS(uint idLib, const QHttpServerRequest &request);

    QByteArray cover(uint id, uint idBook);
    QHttpServerResponse convert(uint idLib, uint idBook, const QString &sFormat, bool opds);
#else
    QDomElement doc_header(const QString &session, bool html=false, const QString &lib_name = QString(), const QString &lib_url = QString());
    QString FillPage(std::vector<uint> listBooks, SLib& lib, const QString &sTitle, const QString &lib_url, const QString &current_url, QTextStream& ts, bool opds, uint nPage, const QString &session, bool bShowAuthor);
    void convert(uint idLib, uint idBook, const QString &format, const QString &file_name, bool opds, QTextStream &ts);
    QString WriteSuccess(const QString &contentType = QStringLiteral("text/html;charset=utf-8"), bool isGZip=false);
#endif

    bool for_preview;
    bool for_mobile;
    int port;
    QDomDocument doc;
    int OPDS_server_status;
    std::unordered_map<QString, QDateTime> sessions;
    ExportOptions *pExportOptions_;
    QString sLanguageFilter_;
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
    QHttpServer httpServer_;
#else
    QStringList sesions_auth;
    QMap<qintptr, QTcpSocket *> OPDS_clients;
    QMap<QString, QString> params;
    QTcpServer OPDS_server;
#endif
};

#endif // OPDS_SERVER_H
