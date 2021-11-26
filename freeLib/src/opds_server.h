#ifndef OPDS_SERVER_H
#define OPDS_SERVER_H

#include <QObject>
#include <QTcpServer>
#include <QDomDocument>
#include <QDateTime>

#include "library.h"
#include "options.h"

class opds_server : public QObject
{
    Q_OBJECT
public:
    explicit opds_server(QObject *parent = nullptr);
    void process(QString url, QTextStream& ts, const QString &session);

    void server_run();
signals:

private slots:
    void new_connection();
    void slotRead();

private:
    QDomElement AddTextNode(const QString &name, const QString &text, QDomNode &node);
    QString WriteSuccess(const QString &contentType = QStringLiteral("text/html;charset=utf-8"), bool isGZip=false);
    void convert(uint idLib, uint idBook, const QString &format, const QString &file_name, bool opds, QTextStream &ts);
    QString FillPage(QList<uint> listBooks, SLib& lib, const QString &sTitle, const QString &lib_url, const QString &current_url, QTextStream& ts, bool opds, uint nPage, const QString &session, bool bShowAuthor);
    QList<uint> book_list(SLib& lib, uint idAuthor, uint idSeria, uint idGenre,  const QString &sSearch, bool sequenceless);
    QDomElement doc_header(const QString &session, bool html=false, const QString &lib_name = QString(), const QString &lib_url = QString());
    void stop_server();

    QMap<QString, QString> params;
    bool for_preview;
    bool for_mobile;
    QDomDocument doc;
    QTcpServer OPDS_server;
    int OPDS_server_status;
    QMap<qintptr, QTcpSocket *> OPDS_clients;
    QStringList sesions_auth;
    int port;
    QMap<QString, QDateTime> sessions;
    ExportOptions *pExportOptions_;
};

#endif // OPDS_SERVER_H
