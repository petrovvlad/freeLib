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
    void start(const QString &_export_dir, const std::vector<uint> &vBooks, SendType send, qlonglong id_author);

    void start(QString _export_dir, const QStringList &list_books, SendType send);
    void start(uint idLib, const QString &path);
    std::vector<uint> vSuccessfulExportBooks;

 signals:
    void End();
    void Progress(int procent,int count);
//protected:
    //void run();
private:
    void export_books();
    void export_lib();
    bool convert(const std::vector<QBuffer*> &vOutBuff, uint idLib, const QString &file_name, int count,  uint idBook );

    std::vector<uint> vBbooks_;
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
