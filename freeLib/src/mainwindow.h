#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWebEngineView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QToolButton>
#include <QSqlQuery>
#include <QtWidgets/QTreeWidgetItem>
#include <QBuffer>
#include <QMenu>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QTcpServer>
#include <QSystemTrayIcon>

#include "helpdialog.h"
#include "dropform.h"
#include "opds_server.h"
#include "common.h"

namespace Ui {
class MainWindow;
}

struct Stag
{
    QPixmap pm;
    int id;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    bool error_quit;
    
private:
    Ui::MainWindow *ui;
    QSystemTrayIcon *trIcon;
    void UpdateBooks();
    void UpdateTags();
    DropForm *pDropForm;
    HelpDialog *pHelpDlg;
    QString last_search_symbol;
    QMenu TagMenu;
    QObject* current_list_for_tag;
    void SaveLibPosition();
    QList<Stag> tags_pic;
    QPixmap GetTag(int id);
    void update_list_pix(qlonglong id,int list,int tag_id);
    void uncheck_books(QList<qlonglong> list);
    opds_server opds;
    QToolButton *FirstButton;
    QToolButton *btn_Hash;
    void UpdateExportMenu();
    void DeleteDropForm();
    void FillAuthors();
    void FillSerials();
    void FillGenres();
    void FillListBooks();
    void FillListBooks(QList<uint> listBook, uint idCurrentAuthor);
    bool IsBookInList(const SBook &book);

    int idCurrentLanguage_;
    uint idCurrentAuthor_;
    uint idCurrentGenre_;
    uint idCurrentSerial_;
    uint idCurrentBook_;
    bool bUseTag_;
    bool bShowDeleted_;
    enum TabIndex{TabAuthors = 0, TabSeries = 1, TabGenres = 2, TabSearch = 3};

protected:
    void showEvent(QShowEvent *ev);
    void resizeEvent(QResizeEvent * e);
    void mouseMoveEvent(QMouseEvent *e);
    //void mouseReleaseEvent(QMouseEvent *e);
    void leaveEvent(QEvent *e);
    APP_MODE mode;
    void closeEvent(QCloseEvent *event);
    void FillBookList(QSqlQuery &query);
    void CheckParent(QTreeWidgetItem* parent);
    void CheckChild(QTreeWidgetItem* parent);
    void FillCheckedBookList(QList<uint> &list, QTreeWidgetItem* item=nullptr, bool send_all=false, bool checked_only=false);
    void FillCheckedItemsBookList(QList<uint> &list, QTreeWidgetItem* item, bool send_all);

    void ExportBookListBtn(bool Enable);
    void dragEnterEvent(QDragEnterEvent *ev);
    void dragLeaveEvent(QDragLeaveEvent *);
    void dragMoveEvent(QDragMoveEvent *ev);
    void proc_path(QString path, QStringList *book_list);
    void FillLibrariesMenu();
    void SendMail();
    void SendToDevice();
    void changeEvent(QEvent *event);
    void ShowHeaderCoulmn(int nColumn,QString sSetting,bool bHide);
private slots:
    void ShowDropForm();
    void ExportAction();
    void ManageLibrary();
    void CheckBooks();
    void EditBooks();
    void Settings();
    void searchCanged(QString str);
    void searchClear();
    void btnSearch();
    void DoSearch();
    void SelectAuthor();
    void SelectBook();
    void SelectGenre();
    void SelectSeria();
    void itemChanged(QTreeWidgetItem*,int);
    void BookDblClick();
    void About();
//    void LanguageChange();
    void StartSearch();
    void HelpDlg();
    void ContextMenu(QPoint point);
    void HeaderContextMenu(QPoint point);
    void MoveToAuthor(qlonglong id=-1,QString FirstLetter="");
    void MoveToGenre(qlonglong id);
    void MoveToSeria(qlonglong id=-1,QString FirstLetter="");
    void tag_select(int index);
    void set_tag();
    void ChangingPort(int i);
    void ChangingLanguage(bool change_language=true);
    void ReviewLink(QUrl url);
    void SelectLibrary();
    void on_actionSwitch_to_convert_mode_triggered();
    void on_actionSwitch_to_library_mode_triggered();
    void on_btnSwitchToLib_clicked();
    void on_btnPreference_clicked();
    void onTabWidgetChanged(int index);
    void onLanguageCurrentIndexChanged(int index);


    //void on_splitter_splitterMoved(int pos, int index);

    void ChangingTrayIcon(int index=-1, int color=-1);
    void TrayMenuAction(QSystemTrayIcon::ActivationReason reson);
    void dockClicked();
    void MinimizeWindow();

public slots:
    void newLibWizard(bool AddLibOnly=true);
signals:
    void window_loaded();
};

#endif // MAINWINDOW_H
