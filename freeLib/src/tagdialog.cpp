#include "tagdialog.h"
#include "ui_tagdialog.h"
#include "common.h"

TagDialog::TagDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TagDialog)
{
    ui->setupUi(this);
    int size =ui->listWidget->style()->pixelMetric(QStyle::PM_SmallIconSize);
    QSqlQuery query(QSqlDatabase::database("libdb"));
    query.exec("SELECT color,name,id from favorite");
    ui->listWidget->clear();
    int con=1;
    while(query.next())
    {

        QListWidgetItem *item=new QListWidgetItem(GetTag(QColor(query.value(0).toString().trimmed()),size),query.value(1).toString().trimmed(),ui->listWidget);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        item->setData(Qt::UserRole,query.value(2).toString());
        con++;
    }
    connect(ui->buttonBox,SIGNAL(accepted()),this,SLOT(ok_btn()));
}

TagDialog::~TagDialog()
{
    delete ui;
}

void TagDialog::ok_btn()
{
    QSqlQuery query(QSqlDatabase::database("libdb"));
    for(int i=0;i<ui->listWidget->count();i++)
    {
        query.exec(QString("UPDATE favorite SET name='%1' WHERE id=%2").
                   arg(ui->listWidget->item(i)->text(),ui->listWidget->item(i)->data(Qt::UserRole).toString()));
    }
}
