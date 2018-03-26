#ifndef DROPFORM_H
#define DROPFORM_H

#include <QWidget>
#include <QMouseEvent>
#include <QBoxLayout>

namespace Ui {
class DropForm;
}

#define CMD_DEVICE  0
#define CMD_EMAIL   1

class DropForm : public QWidget
{
    Q_OBJECT

public:
    explicit DropForm(QWidget *parent = 0);
    ~DropForm();
    void switch_command(QPoint mouse_pos);
    int get_command(QPoint mouse_pos);
    void AddCommand(QStringList cmd);
private:
    Ui::DropForm *ui;
    QBoxLayout *verticalLayout;
//protected:
//    void mouseMoveEvent(QMouseEvent *e);
};

#endif // DROPFORM_H
