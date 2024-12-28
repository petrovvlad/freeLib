#ifndef LIBRARIESDLG_H
#define LIBRARIESDLG_H

#include <QDialog>

#include "importthread.h"
#include "library.h"

namespace Ui {
class LibrariesDlg;
}

class LibrariesDlg : public QDialog
{
    Q_OBJECT

public:
    explicit LibrariesDlg(QWidget *parent = 0);
    ~LibrariesDlg();
    void StartImport(SLib &lib);
    bool bLibChanged;
    
private:
    void UpdateLibList();
    void SaveLibrary(const SLib &lib);

    Ui::LibrariesDlg *ui;
    ImportThread *pImportThread_;
    QThread *pThread_;
    uint idCurrentLib_;

private slots:
    void progressImport(uint nBooksAdded, float fProgress);
    void InputINPX();
    void SelectBooksDir();
    void StartImport();
    void SelectLibrary();
    void DeleteLibrary();
    void onAddLibrary();
    void EndUpdate();
    void terminateImport();
    void reject();
    void onBreak();
    void ExistingLibsChanged();
    void ExportLib();
    void addBook();
    void onComboboxLibraryChanged(int index);
    QString UniqueName(const QString &sName);

signals:
    void break_import();
};

#endif // LIBRARIESDLG_H
