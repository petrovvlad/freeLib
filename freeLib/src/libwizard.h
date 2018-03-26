#ifndef LIBWIZARD_H
#define LIBWIZARD_H

#include <QWizard>

namespace Ui {
class LibWizard;
}

class LibWizard : public QWizard
{
    Q_OBJECT

public:
    explicit LibWizard(QWidget *parent = 0);
    ~LibWizard();
    bool validateCurrentPage();
    QString name;
    QString dir;
    QString inpx;
    int mode;
    int AddLibMode();
private:
    Ui::LibWizard *ui;
    QString find_inpx();
//    int AddLib;
private slots:
    void SetDir(int page);
    void SelectBooksDir();
    void InputINPX();
};

#endif // LIBWIZARD_H
