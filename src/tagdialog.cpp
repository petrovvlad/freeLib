#include "tagdialog.h"
#include "ui_tagdialog.h"

#include <algorithm>
#include <QComboBox>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

#include "library.h"
#include "utilites.h"

TagDialog::TagDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TagDialog)
{
    ui->setupUi(this);

    ui->tableWidget->setColumnWidth(0, 140);
    ui->tableWidget->setColumnWidth(1, 30);

    bool darkTheme = palette().color(QPalette::Window).lightness() < 127;

    QSqlQuery query(QSqlDatabase::database(QStringLiteral("libdb")));
    query.exec(QStringLiteral("SELECT id, light_theme, dark_theme FROM icon"));
    while(query.next())
    {
        QPixmap pixIcon;
        uint idIcon = query.value(0).toUInt();
        QByteArray baLightIcon = query.value(1).toByteArray();
        QByteArray baDarkIcon = query.value(2).toByteArray();
        if(darkTheme){
            if(baDarkIcon.isEmpty())
                pixIcon.loadFromData(baLightIcon);
            else
                pixIcon.loadFromData(baDarkIcon);
        }else
            pixIcon.loadFromData(baLightIcon);
        icons_[idIcon] = pixIcon;
    }

    query.exec(QStringLiteral("SELECT id,name,id_icon FROM tag"));
    //                                0  1    2
    int con = 0;
    while(query.next())
    {
        int index = 0;
        uint idTag = query.value(0).toUInt();
        uint idIcon = query.value(2).toUInt();
        QString sName = query.value(1).toString().trimmed();
        bool bLatin1 = true;
        for(int i=0; i<sName.length(); i++){
            if(sName.at(i).unicode()>127){
                bLatin1 = false;
                break;
            }
        }
        if(bLatin1)
            sName = tr(sName.toLatin1().data());
        QTableWidgetItem *itemName = new QTableWidgetItem(sName);
        itemName->setData(Qt::UserRole, idTag);
        ui->tableWidget->insertRow(ui->tableWidget->rowCount());
        ui->tableWidget->setItem(con, 0, itemName);
        QComboBox *combo = new QComboBox(ui->tableWidget);
        for(const auto &iIcon :icons_){
            combo->insertItem(index, iIcon.second, QStringLiteral(""), iIcon.first);
            if(iIcon.first == idIcon)
                combo->setCurrentIndex(index);
            index++;
        }
        ui->tableWidget->setCellWidget(con, 1, combo);
        con++;
    }
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &TagDialog::btnOk);
    connect(ui->pushButtonAddRow, &QPushButton::clicked, this, &TagDialog::onAddRow);
    connect(ui->pushButtonDelRow, &QPushButton::clicked, this, &TagDialog::onDeleteRow);
}

TagDialog::~TagDialog()
{
    delete ui;
}

void TagDialog::btnOk()
{
    QSqlQuery query(QSqlDatabase::database(u"libdb"_s));
    for(auto id :vIdDeleted_){
        for(auto &iLib :g::libs){
            iLib.second.deleteTag(id);
        }
    }
    uint maxId = 0;
    for(int i=0; i<ui->tableWidget->rowCount(); i++)
        maxId = std::max(maxId, ui->tableWidget->item(i, 0)->data(Qt::UserRole).toUInt());
    for(int i=0; i<ui->tableWidget->rowCount(); i++)
    {
        uint id = ui->tableWidget->item(i,0)->data(Qt::UserRole).toUInt();
        if(id == 0){
            id = ++maxId;
            query.prepare(QStringLiteral("INSERT INTO tag (id,name,id_icon) VALUES(:id, :name,:id_icon) "));
        }else{
            query.prepare(QStringLiteral("UPDATE tag SET name=:name, id_icon=:id_icon WHERE id=:id"));
        }
        query.bindValue(QStringLiteral(":id"), id);
        query.bindValue(QStringLiteral(":name"), ui->tableWidget->item(i,0)->text().trimmed());
        QVariant idIcon = qobject_cast<QComboBox*>(ui->tableWidget->cellWidget(i, 1))->currentData();
        query.bindValue(QStringLiteral(":id_icon"), idIcon);
        if(!query.exec()) [[unlikely]]
            LogWarning << query.lastError().text();
    }
}

void TagDialog::onAddRow()
{
    int newRow = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(newRow);
    QTableWidgetItem *itemName = new QTableWidgetItem(tr("Tag") + QString::number(newRow));
    ui->tableWidget->setItem(newRow, 0, itemName);

    QComboBox *combo = new QComboBox(ui->tableWidget);
    int index = 0;
    for(const auto &iIcon :icons_){
        combo->insertItem(index, iIcon.second, u""_s, iIcon.first);
        index++;
    }
    combo->setCurrentIndex(0);
    ui->tableWidget->setCellWidget(newRow, 1, combo);
}

void TagDialog::onDeleteRow()
{
    int index = ui->tableWidget->currentRow();
    if(index>=0){
        QTableWidgetItem *item = ui->tableWidget->item(index, 0);
        uint id = item->data(Qt::UserRole).toUInt();
        QSqlQuery query(QSqlDatabase::database(QStringLiteral("libdb")));
        query.prepare(QStringLiteral("SELECT COUNT(id_tag) cnt FROM book_tag WHERE id_tag=:id_tag"));
        query.bindValue(QStringLiteral(":id_tag"), id);
        query.exec();
        query.first();
        uint nCount = query.value(0).toUInt();
        if(nCount == 0){
            query.prepare(QStringLiteral("SELECT COUNT(id_tag) cnt FROM author_tag WHERE id_tag=:id_tag"));
            query.bindValue(QStringLiteral(":id_tag"), id);
            query.exec();
            query.first();
            nCount = query.value(0).toUInt();
        }
        if(nCount == 0){
            query.prepare(QStringLiteral("SELECT COUNT(id_tag) cnt FROM seria_tag WHERE id_tag=:id_tag"));
            query.bindValue(QStringLiteral(":id_tag"), id);
            query.exec();
            query.first();
            nCount = query.value(0).toUInt();
        }

        if(nCount ==0 || QMessageBox::question(nullptr,tr("Tags"),tr("This tag is used. Do you want to delete this tag anyway?"),QMessageBox::Yes|QMessageBox::No,QMessageBox::No)==QMessageBox::Yes){
            ui->tableWidget->removeRow(index);
            vIdDeleted_.push_back(id);
        }
    }
}

