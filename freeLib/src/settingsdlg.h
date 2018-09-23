#ifndef SETTINGSDLG_H
#define SETTINGSDLG_H

#include <QDialog>
#include <qitemdelegate.h>
#include <QToolButton>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QWidget>
#include <QEvent>
#include <QDebug>
#include <QFileDialog>
#include <QSettings>
#include "fontframe.h"

class FileItemDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    FileItemDelegate(QObject *parent = 0):QItemDelegate(parent)
    {

    }

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &/*option*/, const QModelIndex &/*index*/) const
    {
        QWidget *frame=new QWidget(parent);
        frame->setAttribute(Qt::WA_TranslucentBackground);
        QHBoxLayout *layout=new QHBoxLayout(frame);
        QLineEdit *editor=new QLineEdit(frame);
        editor->setObjectName("editor");
        QToolButton *button=new QToolButton(frame);
        button->setText("...");
        layout->addWidget(editor,1);
        editor->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        layout->addWidget(button,0);
        //layout->setStretch(1,0);
        layout->setSpacing(0);
        layout->setMargin(0);
        connect(editor,SIGNAL(editingFinished()),this,SLOT(editingFinished()));
        connect(button,SIGNAL(clicked()),this,SLOT(SelectFile()));

        return frame;
    }
    void setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const
    {
        QLineEdit *editor_file=editor->findChild<QLineEdit*>("editor");
        model->setData(index,editor_file->text());
    }
    void setEditorData(QWidget * editor, const QModelIndex & index) const
    {
        QLineEdit *editor_file=editor->findChild<QLineEdit*>("editor");
        editor_file->setText(index.data().toString());
    }
    bool eventFilter(QObject* object, QEvent* event)
    {
        if(event->type()==QEvent::FocusIn)
        {
            QLineEdit *editor=object->findChild<QLineEdit*>("editor");
            editor->setFocus();
        }
       // qDebug()<<event->type();
        return true;
    }
private slots:
    void editingFinished()
    {
        commitData((QWidget*)sender()->parent());
        //emit closeEditor((QWidget*)sender()->parent());
    }
    void SelectFile()
    {
        QLineEdit *editor_file=sender()->parent()->findChild<QLineEdit*>("editor");
        QFileInfo fi(editor_file->text());
        QString file_name=QFileDialog::getOpenFileName((QWidget*)sender(),tr("Select application"),fi.absolutePath());
        if(!file_name.isEmpty())
        {
            editor_file->setText(file_name);
            editor_file->setFocus();
        }
    }
};

namespace Ui {
class SettingsDlg;
}

class SettingsDlg : public QDialog
{
    Q_OBJECT
    
public:
    explicit SettingsDlg(QWidget *parent = 0);
    ~SettingsDlg();
    
private:
    Ui::SettingsDlg *ui;
    void LoadSettings();
private slots:
    void btnOK();
    void SaveTools(QSettings *settings=0);
    void AddExt();
    void DelExt();
    void AddApp();
    void DelApp();
    void ChangePort(int i=-1);
    void ChangeLanguage();
    void on_AddExport_clicked();
    void ExportNameChanged();
    void on_DelExport_clicked();
    void on_ExportName_currentIndexChanged(int index);
    void on_ChangeExportFrameTab(int tab_id,int page_id);

    void on_DefaultExport_clicked();
    void btnDBPath();
    void btnDirPath();
    void on_btnDefaultSettings_clicked();
    void on_tabWidget_currentChanged(int index);
    void UpdateWebExportList();

    void on_proxy_type_currentIndexChanged(int index);

    void on_browseDir_stateChanged(int checked);

    void on_trayIcon_currentIndexChanged(int index);

    void on_tray_color_currentIndexChanged(int index);

    void on_HTTP_need_pasword_clicked();

    void on_btnSaveExport_clicked();

    void on_btnOpenExport_clicked();

signals:
    void ChangingPort(int i);
    void ChangingLanguage();
    void ChangingExportFrameTab(int tab_id,int page_id);
    void NeedUpdateTools();
    void ChangingTrayIcon(int index,int color);
};

#endif // SETTINGSDLG_H
