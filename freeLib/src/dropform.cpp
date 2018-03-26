#include "dropform.h"
#include "ui_dropform.h"
#include <QLabel>
#include <QDebug>

DropForm::DropForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DropForm)
{
    ui->setupUi(this);
    verticalLayout=new QBoxLayout(QBoxLayout::TopToBottom,this);
    verticalLayout->setMargin(4);
    setMouseTracking(true);
}

DropForm::~DropForm()
{
    delete ui;
}

void DropForm::switch_command(QPoint mouse_pos)
{
    int count=get_command(mouse_pos);
    for(int i=0;i<verticalLayout->count();i++)
    {
        if(i==count)
            ((QLabel*)verticalLayout->itemAt(i)->widget())->setStyleSheet("border:2px solid grey; border-radius: 7px;background-color: rgba(0,0,0,128);");
        else
            ((QLabel*)verticalLayout->itemAt(i)->widget())->setStyleSheet("border:2px solid grey; border-radius: 7px;background-color: rgba(0,0,0,64);");
    }
}

int DropForm::get_command(QPoint mouse_pos)
{
    if(verticalLayout->count()==0)
        return -1;
    QPoint point=mapFromParent(mouse_pos);

    int height_item=rect().height()/verticalLayout->count();
    int count=0;
    if(point.x()<=0 || point.x()>=width())
        return -1;
    while(count*height_item<point.y())
        count++;
    if(count==0 || count>verticalLayout->count())
        return -1;
    return count-1;
}

void DropForm::AddCommand(QStringList cmd)
{
    QFont lab_font(font());
    lab_font.setPixelSize(rect().height()/cmd.count()*0.75);
    QFontMetrics metric(lab_font);
    int max_len=0;
    foreach (QString str, cmd)
    {
        if(metric.width(str)>max_len)
            max_len=metric.width(str);
    }
    if(max_len>rect().width()*0.9)
    {
        lab_font.setPixelSize(lab_font.pixelSize()*(rect().width()*0.9/max_len));
    }
    foreach (QString str, cmd)
    {
        QLabel *lab=new QLabel(str,this);
        lab->setAlignment(Qt::AlignCenter);
        lab->setFont(lab_font);
        lab->setStyleSheet("border:2px solid grey; border-radius: 7px;background-color: rgba(0,0,0,64);");
        lab->setMouseTracking(true);
        verticalLayout->addWidget(lab);
    }
}

