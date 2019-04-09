#include "aboutdialog.h"
#include "ui_aboutdialog.h"
#include "common.h"
#include "build_number.h"


AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent,Qt::Window | Qt::FramelessWindowHint),
    ui(new Ui::AboutDialog)
{
    //qDebug()<<app->devicePixelRatio();
    ui->setupUi(this);
    //setFixedSize(400,200);
    ui->version->setText(PROG_VERSION);

    connect(ui->toolButton,SIGNAL(clicked()),this,SLOT(CloseBtn()));
 //   connect(ui->donate,SIGNAL(clicked()),this,SLOT());
    QPixmap pix(QString(":/splash%1.png").arg(app->devicePixelRatio()>=2?"@2x":""));
    pix.setDevicePixelRatio(app->devicePixelRatio());
    ui->label->setPixmap(pix);
    pix.load(QString(":/icons/img/icons/close%1.png").arg(app->devicePixelRatio()>=2?"@2x":""));
    QFont f(ui->version->font());
    f.setPointSize(VERSION_FONT);
    ui->version->setFont(f);
  //  QIcon icon(pix);
   // pix.setDevicePixelRatio(app->devicePixelRatio());
   // QIcon icon;
   // icon.addFile(":/icons/img/icons/close@2x.png");
   // icon.addFile(":/icons/img/icons/close.png");
   // ui->toolButton->setIconSize(QSize(32,32));
   // ui->toolButton->setIcon(icon);
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

//void AboutDialog::Donate()
//{
//    DoDonate();
//    accept();
//}

void AboutDialog::CloseBtn()
{
    accept();
    //close();
}
