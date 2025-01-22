#include "starscombobox.h"
#include "starsdelegate.h"
#include "utilites.h"

StarsComboBox::StarsComboBox(QWidget *parent)
    : QComboBox(parent)
{
    auto delegate = new StarsDelegate(this);
    delegate->setAlinment(Qt::AlignLeft | Qt::AlignVCenter);
    setItemDelegate(delegate);

    for(int i=0; i<=5; i++){
        insertItem(i, u""_s, i);
    }
}

void StarsComboBox::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    QStyleOptionComboBox opt;
    initStyleOption(&opt);

    // Отрисовка рамки и фона
    style()->drawComplexControl(QStyle::CC_ComboBox, &opt, &painter, this);
    if (currentIndex() >= 0)
    {
        // Создаем QStyleOptionViewItem и копируем доступные данные из QStyleOptionComboBox
        QStyleOptionViewItem itemOption;
        itemOption.rect = opt.rect;
        itemOption.state = opt.state;
        itemOption.palette = opt.palette;
        itemOption.fontMetrics = opt.fontMetrics;
        itemOption.widget = this;

        // Получаем данные текущего элемента и вызываем делегат для отрисовки
        QModelIndex index = model()->index(currentIndex(), modelColumn(), rootModelIndex());
        itemDelegate()->paint(&painter, itemOption, index);
    }
    else
    {
        QComboBox::paintEvent(event);
    }
}
