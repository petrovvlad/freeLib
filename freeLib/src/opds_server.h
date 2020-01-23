#ifndef OPDS_SERVER_H
#define OPDS_SERVER_H

#include <QObject>
#include <QTcpServer>
#include <QDomDocument>
#include <QDateTime>

#include "common.h"

class opds_server : public QObject
{
    Q_OBJECT
public:
    explicit opds_server(QObject *parent = nullptr);
    void process(QString url,QTextStream& ts,QString session);

    void server_run(int _port=-1);
signals:

private slots:
    void new_connection();
    void slotRead();

private:
    QMap<QString,QString> params;
    bool for_preview;
    bool for_mobile;
    QDomDocument doc;
    QTcpServer OPDS_server;
    int OPDS_server_status;
    QMap<qintptr,QTcpSocket *> OPDS_clients;
    QStringList sesions_auth;
    QDomElement AddTextNode(QString name, QString text, QDomNode &node);
    QString WriteSuccess(QString contentType = "text/html;charset=utf-8", bool isGZip=false);
    int port;
    void stop_server();
    //QString books_list(QString lib_url, QString current_url, QString id_lib, QString author, QString seria, QString ganre,  QTextStream& ts, bool opds, QString lib_name, QString session, bool all=false, QString search="");
    QList<uint> book_list(SLib& lib, uint idAuthor, uint idSeria, uint idGenre,  QString sSearch, bool sequenceless);
    QString FillPage(QList<uint> listBooks, SLib& lib, QString sTitle, QString lib_url, QString current_url, QTextStream& ts, bool opds, uint nPage, QString session, bool bShowAuthor);
    QDomElement doc_header(QString session, bool html=false, QString lib_name=QString(), QString lib_url=QString());
    void convert(QString id,QString format,QString file_name,bool opds, QTextStream &ts);
    QMap<QString,QDateTime> sessions;
};

#endif // OPDS_SERVER_H
