#include "filenamelineedit.h"

FilenameLineEdit::FilenameLineEdit(QObject *parent)
{
}


CChoicePathDelegate::CChoicePathDelegate(QObject *parent) :
    QItemDelegate(parent)
{
 //   SQL=sql;
}


QWidget *CChoicePathDelegate::createEditor(QWidget *parent,
     const QStyleOptionViewItem &/* option */,
     const QModelIndex &/* index */) const
 {

     return 0;
 }



void CChoicePathDelegate::setEditorData(QWidget *editor,
                                     const QModelIndex &index) const
 {

 }

void CChoicePathDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                    const QModelIndex &index) const
 {

 }

void CChoicePathDelegate::updateEditorGeometry(QWidget *editor,
    const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}
