#ifndef EXPORTDLG_H
#define EXPORTDLG_H

#include <QDialog>

#include "exportthread.h"

namespace Ui {
class ExportDlg;
}

class ExportDlg : public QDialog
{
    Q_OBJECT
    
public:
    explicit ExportDlg(QWidget *parent = 0);
    ~ExportDlg();
    void exec(const std::vector<uint> &vBooks, SendType send, qlonglong id_author, const ExportOptions &exportOptions);
    void exec(uint idLib, const QString &path);
    int exec();
    std::vector<uint> vSuccessfulExportBooks;
private:
    Ui::ExportDlg *ui;
    ExportThread *worker;
    QThread* thread;
    bool itsOkToClose;
    QString SelectDir();
    const ExportOptions* pExportOptions_;
private slots:
    void EndExport();
    void BreakExport();
    void Process(int Procent, int count);
    void reject();
signals:
    void break_exp();
};

#endif // EXPORTDLG_H
