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
#include "common.h"
#include "options.h"


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
        editor->setObjectName(QStringLiteral("editor"));
        QToolButton *button=new QToolButton(frame);
        button->setText(QStringLiteral("..."));
        layout->addWidget(editor,1);
        editor->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        layout->addWidget(button,0);
        //layout->setStretch(1,0);
        layout->setSpacing(0);
        layout->setContentsMargins(0, 0, 0, 0);
        connect(editor,&QLineEdit::editingFinished, this, &FileItemDelegate::editingFinished);
        connect(button, &QAbstractButton::clicked, this, &FileItemDelegate::SelectFile);

        return frame;
    }

    void setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const
    {
        QLineEdit *editor_file = editor->findChild<QLineEdit* > (QStringLiteral("editor"));
        model->setData(index,editor_file->text());
    }

    void setEditorData(QWidget * editor, const QModelIndex & index) const
    {
        QLineEdit *editor_file = editor->findChild<QLineEdit*>(QStringLiteral("editor"));
        editor_file->setText(index.data().toString());
    }

    bool eventFilter(QObject* object, QEvent* event)
    {
        if(event->type() == QEvent::FocusIn)
        {
            QLineEdit *editor = object->findChild<QLineEdit*>(QStringLiteral("editor"));
            editor->setFocus();
        }
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
        QLineEdit *editor_file=sender()->parent()->findChild<QLineEdit*>(QStringLiteral("editor"));
        QFileInfo fi(editor_file->text());
        QString file_name = QFileDialog::getOpenFileName((QWidget*)sender(), tr("Select application"), fi.absolutePath());
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
    explicit SettingsDlg(QWidget *parent);
    ~SettingsDlg();

    Options options_;

private:
    Ui::SettingsDlg *ui;

    void LoadSettings();
private slots:
    void btnOK();
    void SaveTools();
    void AddExt();
    void DelExt();
    void AddApp();
    void DelApp();
    void ChangePort(int i);
    void ChangeLanguage();
    void ExportNameChanged();
    void onAddExportClicked();
    void onDelExportClicked();
    void onExportNameCurrentIndexChanged(int index);
    void onChangeExportFrameTab(int tab_id, int page_id);
    void onDefaultExportClicked();
    void onBtnDefaultSettingsClicked();
    void onTabWidgetCurrentChanged(int index);
    void onProxyTypeCurrentIndexChanged(int index);
    void onBrowseDirStateChanged(int checked);
    void onTrayIconCurrentIndexChanged(int index);
    void onTrayColorCurrentIndexChanged(int index);
    void onHTTPneedPaswordClicked();
    void onBtnSaveExportClicked();
    void onBtnOpenExportClicked();
    void onChangeAlphabetCombobox(int index);
    void onOpdsEnable(int state);
    void btnDBPath();
    void btnDirPath();
    void UpdateWebExportList();
    virtual void reject();

signals:
    void ChangingPort(int i);
    void ChangingLanguage();
    void ChangeAlphabet(const QString &str);
    void ChangingExportFrameTab(int tab_id, int page_id);
    void NeedUpdateTools();
    void ChangingTrayIcon(int index, int color);
};

#endif // SETTINGSDLG_H
