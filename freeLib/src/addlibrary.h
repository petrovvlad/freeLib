#ifndef ADDLIBRARY_H
#define ADDLIBRARY_H

#include <QDialog>

#include "importthread.h"
#include "library.h"

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
    void AddNewLibrary(SLib &lib);
    void StartImport(SLib &Lib);
    bool bLibChanged;
    
private:
    void UpdateLibList();
    void SaveLibrary(uint idLib, const SLib &Lib);

    Ui::AddLibrary *ui;
    ImportThread *imp_tr;
    QThread *thread;
    uint idCurrentLib_;

private slots:
    void LogMessage(const QString &msg);
    void InputINPX();
    void SelectBooksDir();
    void StartImport();
    void SelectLibrary();
    void DeleteLibrary();
    void Add_Library();
    void EndUpdate();
    void terminateImport();
    void reject();
    void ExistingLibsChanged();
    void ExportLib();
    void onComboboxLibraryChanged(int index);
    QString UniqueName(const QString &sName);

signals:
    void break_import();
};

#endif // ADDLIBRARY_H
