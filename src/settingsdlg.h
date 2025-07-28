#ifndef SETTINGSDLG_H
#define SETTINGSDLG_H

#include <QDialog>
#include <QItemDelegate>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QWidget>
#include <QEvent>
#include <QDebug>
#include <QFileDialog>
#include <QSettings>
#include <QToolButton>
#include <QTreeWidgetItem>

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
        QWidget *frame = new QWidget(parent);
        frame->setAttribute(Qt::WA_TranslucentBackground);
        QHBoxLayout *layout=new QHBoxLayout(frame);
        QLineEdit *editor=new QLineEdit(frame);
        editor->setObjectName(u"editor"_s);
        QToolButton *button = new QToolButton(frame);
        button->setText(u"..."_s);
        layout->addWidget(editor,1);
        editor->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        layout->addWidget(button, 0);
        //layout->setStretch(1,0);
        layout->setSpacing(0);
        layout->setContentsMargins(0, 0, 0, 0);
        connect(editor, &QLineEdit::editingFinished, this, &FileItemDelegate::editingFinished);
        connect(button, &QAbstractButton::clicked, this, &FileItemDelegate::SelectFile);

        return frame;
    }

    void setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const
    {
        QLineEdit *editor_file = editor->findChild<QLineEdit* > (u"editor"_s);
        model->setData(index,editor_file->text());
    }

    void setEditorData(QWidget * editor, const QModelIndex & index) const
    {
        QLineEdit *editor_file = editor->findChild<QLineEdit*>(u"editor"_s);
        editor_file->setText(index.data().toString());
    }

    bool eventFilter(QObject* object, QEvent* event)
    {
        if(event->type() == QEvent::FocusIn)
        {
            QLineEdit *editor = object->findChild<QLineEdit*>(u"editor"_s);
            editor->setFocus();
        }
        return true;
    }

private slots:
    void editingFinished()
    {
        emit commitData((QWidget*)sender()->parent());
        //emit closeEditor((QWidget*)sender()->parent());
    }
    void SelectFile()
    {
        QLineEdit *editor_file=sender()->parent()->findChild<QLineEdit*>(u"editor"_s);
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
    QTreeWidgetItem *itemConversion;

    void LoadSettings();
    void updateKindelegenWarring(int iExportOpton);
    void showConversionNameItem(int nIndex, bool bShow);

public slots:
private slots:
    void btnOK();
    void SaveTools();
    void AddExt();
    void DelExt();
    void AddApp();
    void DelApp();
    void ChangeLanguage();
    void ExportNameChanged();
    void onAddExport();
    void onDelExport();
    void onExportNameCurrentIndexChanged(int index);
    void onChangeConversionFrameTab(int index);
    void onBtnDefaultSettingsClicked();
    void onTrayIconCurrentIndexChanged(int index);
    void onTrayColorCurrentIndexChanged(int index);
    void onBtnSaveExportClicked();
    void onBtnOpenExportClicked();
    void onChangeAlphabetCombobox(int index);
    void btnDBPath();
    void onChangePage();
    void onExportFormatChanged();
#ifdef USE_HTTSERVER
    void onUseForHttpChanged(int state);
    void onProxyTypeCurrentIndexChanged(int index);

#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    void onHttpNeedPaswordChanged(int state);
    void onOpdsEnable(int state);
#else
    void onHttpNeedPaswordChanged(Qt::CheckState state);
    void onOpdsEnable(Qt::CheckState state);
#endif //QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
#endif //USE_HTTSERVER

#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    void onDefaultExportChanged(int state);
#else
    void onDefaultExportChanged(Qt::CheckState state);
#endif //QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    void onUseSystemFontsChanged(int state);
    void onSidebarFontChanged(const QFont &font);
    void onSidebarSizeFontChanged(int i);
    void onListFontChanged(const QFont &font);
    void onListSizeFontChanged(int i);
    void onAnnotationFontChanged(const QFont &font);
    void onAnnotationSizeFontChanged(int i);
    virtual void reject();

signals:
    void ChangingPort(int i);
    void ChangingLanguage();
    void ChangeAlphabet(const QString &str);
    void ChangingConversionFrameTab(int index);
    void NeedUpdateTools();
    void ChangingTrayIcon(int index, int color);
    void ChangeSidebarFont(const QFont &font);
    void ChangeListFont(const QFont &font);
    void ChangeAnnotationFont(const QFont &font);
};

#endif // SETTINGSDLG_H
