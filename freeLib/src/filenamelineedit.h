#ifndef FILENAMELINEEDIT_H
#define FILENAMELINEEDIT_H

#include <QLineEdit>
#include <QToolButton>
#include <QModelIndex>
#include <QStyleOptionViewItem>
#include <QItemDelegate>

class FilenameLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    FilenameLineEdit(QObject *parent = 0);
    //QString getChoice () {return fileName;}
    protected:
  //  void resizeEvent(QResizeEvent *); //в этой функции делаем так чтобы текст не залезал под кнопку

    private slots:
    //void pressButton ( ); //здесь вызов диалога выбора файла/каталога

    private:
    QToolButton *OpenButton;
    QString fileName;
signals:


};


class CChoicePathDelegate : public QItemDelegate
{
public:
    CChoicePathDelegate (QObject *parent = 0);
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,const QModelIndex &index) const; //создаем редактор делегата - это наш виджет
    void setEditorData(QWidget *editor, const QModelIndex &index) const; //устанавливаем данные в редакторе
    void setModelData(QWidget *editor, QAbstractItemModel *model,const QModelIndex &index) const; //а здесь данные из редактора передаем уже в модель
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

#endif // FILENAMELINEEDIT_H
