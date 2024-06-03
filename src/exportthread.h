#ifndef EXPORTTHREAD_H
#define EXPORTTHREAD_H

#include <QThread>
#include <QBuffer>

#include "options.h"

class ExportThread : public QObject
{
    Q_OBJECT
public:
    explicit ExportThread(const ExportOptions *pExportOptions = nullptr);
    void start(const QString &_export_dir, const QList<uint> &list_books, SendType send, qlonglong id_author);

    void start(QString _export_dir, const QStringList &list_books, SendType send);
    void start(uint idLib, const QString &path);
    QList<qlonglong> successful_export_books;

 signals:
    void End();
    void Progress(int procent,int count);
//protected:
    //void run();
private:
    void export_books();
    void export_lib();
    bool convert(const QList<QBuffer*> &outbuff, uint idLib, const QString &file_name, int count,  uint idBook );

    QList<uint> book_list;
    const ExportOptions* pExportOptions_;
    SendType send_type;
    qlonglong IDauthor;
    QString sExportDir_;
    uint idLib_;
    std::atomic_bool stopped_;
public slots:
    void break_exp();
    void process();

};

#endif // EXPORTTHREAD_H
