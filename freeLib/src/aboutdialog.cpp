#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include "build_number.h"


AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent,Qt::Window | Qt::FramelessWindowHint),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    //setFixedSize(400,200);
    ui->version->setText(PROG_VERSION);

    connect(ui->toolButton, &QAbstractButton::clicked, this, &AboutDialog::CloseBtn);
    QPixmap pix(QStringLiteral(":/splash%1.png").arg(devicePixelRatio()>=2 ?QStringLiteral("@2x") :QLatin1String("")));
    pix.setDevicePixelRatio(devicePixelRatio());
    ui->label->setPixmap(pix);
    pix.load(QStringLiteral(":/icons/img/icons/close%1.png").arg(devicePixelRatio()>=2 ?QStringLiteral("@2x"): QLatin1String("")));
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

void AboutDialog::CloseBtn()
{
    accept();
    //close();
}
