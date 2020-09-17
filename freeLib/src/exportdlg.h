#ifndef EXPORTDLG_H
#define EXPORTDLG_H

#include <QDialog>

#include "exportthread.h"
#include "library.h"

namespace Ui {
class ExportDlg;
}

class ExportDlg : public QDialog
{
    Q_OBJECT
    
public:
    explicit ExportDlg(QWidget *parent = 0);
    ~ExportDlg();
    void exec(const QList<uint> &list_books,SendType send, qlonglong id_author);
    void exec(qlonglong id_lib, QString path);
    int exec();
    QList<qlonglong> succesfull_export_books;
private:
    Ui::ExportDlg *ui;
    ExportThread *worker;
    QThread* thread;
    bool itsOkToClose;
    QString SelectDir();
private slots:
    void EndExport();
    void BreakExport();
    void Process(int Procent, int count);
    void reject();
signals:
    void break_exp();
};

#endif // EXPORTDLG_H
