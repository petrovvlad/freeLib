#ifndef IMPORTTHREAD_H
#define IMPORTTHREAD_H

#include <QThread>
#include <QSqlQuery>
#include <QStringList>

//способы обновления
#define UT_FULL 10
#define UT_DEL_AND_NEW 11
#define UT_NEW 12



void ClearLib(qlonglong existingID, bool delete_only);


class ImportThread : public QObject
{
    Q_OBJECT
public:
    explicit ImportThread(QObject *parent = 0);
    void start(QString fileName, QString name, QString path, long ID, int update_type, bool save_only=false, bool firstAuthor=false);
    //void SaveLibrary();
    bool loop;
signals:
    void Message(QString str);
    void End();
public slots:
    void process();
    void break_import();
protected:
    void importFB2(QString path,int &count);
    void importFB2_main(QString path);

    void readFB2(const QByteArray &ba,QString file_name, QString arh_name,qint32 file_size=0);
    void readEPUB(const QByteArray &ba,QString file_name, QString arh_name,qint32 file_size=0);
    void readFB2_test(const QByteArray& ba,QString file_name,QString arh_name);
private:
    QString _fileName;
    QString _name;
    QString _path;
    bool _save_only;
    int _update_type;
    bool _firstAuthorOnly;
    long existingID;
    QSqlQuery *query;
    qlonglong AddSeria(QString str,qlonglong libID,int tag);
    qlonglong AddAuthor(QString str,qlonglong libID,qlonglong id_book,bool first_author,QString language,int tag);
    qlonglong AddBook(qlonglong star, QString name, qlonglong id_seria, int num_in_seria, QString file,
                 int size, int IDinLib, bool deleted, QString format, QDate date, QString language, QString keys, qlonglong id_lib, QString archive, int tag);

    qlonglong AddJanre(qlonglong id_book,QString janre,qlonglong id_lib,QString language);

};

#endif // IMPORTTHREAD_H
