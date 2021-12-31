#ifndef TAGDIALOG_H
#define TAGDIALOG_H

#include <QDialog>
#include <QMap>


namespace Ui {
class TagDialog;
}

class TagDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TagDialog(QWidget *parent = 0);
    ~TagDialog();

private:
    Ui::TagDialog *ui;
    QMap<uint, QIcon> icons_;
    QList<uint> listIdDeleted_;

private slots:
    void btnOk();
    void onAddRow();
    void onDeleteRow();
};

#endif // TAGDIALOG_H
