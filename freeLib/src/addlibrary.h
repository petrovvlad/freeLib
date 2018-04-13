#ifndef ADDLIBRARY_H
#define ADDLIBRARY_H

#include <QDialog>
#include "importthread.h"
#include "common.h"

namespace Ui {
class AddLibrary;
}

class AddLibrary : public QDialog
{
    Q_OBJECT

public:
    explicit AddLibrary(QWidget *parent = 0);
    ~AddLibrary();
    //int exec();
    void AddNewLibrary(QString _name, QString _path, QString _fileName);
    void StartImport(SLib &Lib);
    void SelectLibrary(int idLib);
    bool bLibChanged;
    
private:
    Ui::AddLibrary *ui;
    ImportThread *imp_tr;
    QThread *thread;
    void UpdateLibList();
    void SaveLibrary(int idLib, SLib& Lib);
//    QMap<int,SLib> mapLib;
    int idCurrentLib_;
private slots:
    void LogMessage(QString msg);
    void InputINPX();
    void SelectBooksDir();
    void StartImport();
    void SelectLibrary();
    //void Ok();
    void DeleteLibrary();
    void Add_Library();
    void EndUpdate();
    void terminateImport();
    void reject();
    void ExistingLibsChanged();
    void ExportLib();


signals:
    void break_import();
};

#endif // ADDLIBRARY_H
