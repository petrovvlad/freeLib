#include "starscombobox.h"

//#include <QStylePainter>

//#include "starsdelegate.h"
#include "utilites.h"


StarsComboBox::StarsComboBox(QWidget *parent)
    : QComboBox(parent)
{
    // auto delegate = new StarsDelegate(this);
    // delegate->setAlinment(Qt::AlignLeft | Qt::AlignVCenter);
    // setItemDelegate(delegate);

    QString sStars[6] {u"☆☆☆☆☆"_s, u"★☆☆☆☆"_s, u"★★☆☆☆"_s, u"★★★☆☆"_s, u"★★★★☆"_s, u"★★★★★"_s,};

    for(int i=0; i<=5; i++){
        insertItem(i, sStars[i], i);
        // insertItem(i, u""_s, i);
    }
    QFont font;
    font.setPointSize(font.pointSize()+4);
    setFont(font);
}

// void StarsComboBox::paintEvent(QPaintEvent *event)
// {
//     if (currentIndex() >= 0)
//     {
//         QPainter painter(this);
//         // QStylePainter painter(this);
//         QStyleOptionComboBox opt;
//         initStyleOption(&opt);

//         // Отрисовка рамки и фона
//         // style()->drawComplexControl(QStyle::CC_ComboBox, &opt, &painter, this);
//         // painter.drawComplexControl(QStyle::CC_ComboBox, opt);

//         // Создаем QStyleOptionViewItem и копируем доступные данные из QStyleOptionComboBox
//         QStyleOptionViewItem itemOption;
//         itemOption.type = /*QStyleOption::SO_MenuItem;//*/opt.type;
//         itemOption.rect = opt.rect;
//         itemOption.state = opt.state;
//         itemOption.palette = opt.palette;
//         itemOption.fontMetrics = opt.fontMetrics;
//         //itemOption.features = opt.features
//         //itemOption.Type = QStyleOption::SO_ComboBox;
//         itemOption.widget = this;


//         // Получаем данные текущего элемента и вызываем делегат для отрисовки
//         QModelIndex index = model()->index(currentIndex(), modelColumn(), rootModelIndex());
//         itemDelegate()->paint(&painter, itemOption, index);
//     }
//     else
//     {
//         QComboBox::paintEvent(event);
//     }
// }
