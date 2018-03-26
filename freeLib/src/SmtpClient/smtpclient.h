/*
  Copyright (c) 2011-2012 - Tőkés Attila

  This file is part of SmtpClient for Qt.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  See the LICENSE file for more details.
*/

#ifndef SMTPCLIENT_H
#define SMTPCLIENT_H

#include <QObject>
#include <QtNetwork/QSslSocket>
#include <QNetworkReply>

#include "mimemessage.h"
/*
enum SmtpError
{
    ConnectionTimeoutError,
    ResponseTimeoutError,
    AuthenticationFailedError,
    ServerError,    // 4xx smtp error
    ClientError     // 5xx smtp error
};
*/
class SmtpClient : public QObject
{
    Q_OBJECT
public:

    /* [0] Enumerations */

    enum AuthMethod
    {
        AuthPlain,
        AuthLogin
    };


    enum ConnectionType
    {
        TcpConnection,
        SslConnection,
        TlsConnection       // STARTTLS
    };
    /* [0] --- */


    /* [1] Constructors and Destructors */

    SmtpClient(const QString & host = "locahost", int port = 25, ConnectionType ct = TcpConnection);

    ~SmtpClient();

    /* [1] --- */


    /* [2] Getters and Setters */

    const QString& getHost() const;
    void setHost(QString &host);

    int getPort() const;
    void setPort(int port);

    const QString& getName() const;
    void setName(const QString &name);

    ConnectionType getConnectionType() const;
    void setConnectionType(ConnectionType ct);

    const QString & getUser() const;
    void setUser(const QString &host);

    const QString & getPassword() const;
    void setPassword(const QString &password);

    SmtpClient::AuthMethod getAuthMethod() const;
    void setAuthMethod(AuthMethod method);

    const QString & getResponseText() const;
    int getResponseCode() const;

    QTcpSocket* getSocket();


    /* [2] --- */


    /* [3] Public methods */

    bool connectToHost();

    bool login();
    bool login(const QString &user, const QString &password, AuthMethod method = AuthLogin);

    bool sendMail(MimeMessage& email);

    void quit();


    /* [3] --- */

    QString host;
    int port;
protected:

    /* [4] Protected members */

    QTcpSocket *socket;
    QSslSocket *ssl_socket;

    ConnectionType connectionType;
    QString name;

    QString user;
    QString password;
    AuthMethod authMethod;

    int connectionTimeout;
    int responseTimeout;

    QString responseText;
    int responseCode;


    class ResponseTimeoutException {};

    /* [4] --- */


    /* [5] Protected methods */

    void waitForResponse() throw (ResponseTimeoutException);

    void sendMessage(const QString &text);

    /* [5] --- */

//    bool Error(int code);
protected slots:

    /* [6] Protected slots */

    void socketStateChanged(QAbstractSocket::SocketState state);
    void socketError(QAbstractSocket::SocketError error);
    void socketReadyRead();
    void sslErrorHandler(QList<QSslError> errlist);
    /* [6] --- */


signals:

    /* [7] Signals */

  //  void smtpError(SmtpError e);

    /* [7] --- */

};

#endif // SMTPCLIENT_H
