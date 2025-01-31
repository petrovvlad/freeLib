#ifndef STARSCOMBOBOX_H
#define STARSCOMBOBOX_H

#include <QComboBox>
#include <QObject>

class StarsComboBox : public QComboBox
{
    Q_OBJECT
public:
    explicit StarsComboBox(QWidget *parent = nullptr);

protected:
    // void paintEvent(QPaintEvent *event) override;
};

#endif // STARSCOMBOBOX_H
