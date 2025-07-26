#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QToolButton>
#include <QSqlQuery>
#include <QtWidgets/QTreeWidgetItem>
#include <QBuffer>
#include <QMenu>
#ifdef USE_KStatusNotifier
#include <KStatusNotifierItem>
#else
#include <QSystemTrayIcon>
#endif

#include <QList>

#include "helpdialog.h"
#include "options.h"
#include "coverlabel.h"
#ifdef USE_HTTSERVER
#include "opds_server.h"
#endif
#include "importthread.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    bool error_quit;
    
private:
    Ui::MainWindow *ui;
    void fillLanguages();
    void UpdateTags();
    void SaveLibPosition();

    HelpDialog *pHelpDlg;
    QString last_search_symbol;
    QMenu TagMenu;
    QObject* currentListForTag_;
    std::unordered_map<uint, QIcon> iconsTags_;

    void uncheckBooks(const std::vector<uint> &vBooks);
    QToolButton *FirstButton;
    QToolButton *btn_Hash;
    void UpdateExportMenu();
    void FillAuthors();
    void FillSerials();
    void FillGenres();
    void FillListBooks();
    void FillListBooks(const std::vector<uint> &vBooks, const std::vector<uint> &vCheckedBooks, uint idCurrentAuthor);
    void FillAlphabet(const QString &sAlphabetName);
    bool IsBookInList(const SBook &book);
    void checkLetter(const QChar cLetter);
    std::vector<uint> getCheckedBooks(bool bCheckedOnly = false);
    void FillCheckedItemsBookList(const QTreeWidgetItem *item, bool send_all, std::vector<uint> *pList);
    QList<QTreeWidgetItem *> checkedItemsBookList(const QTreeWidgetItem *item = nullptr);
    void setTagAuthor(uint idTag, uint idAuthor, bool bSet);
    void setTagBook(uint idTag, uint idBook, bool bSet);
    void setTagSequence(uint idTag, uint idSequence, bool bSet);
    void updateIcons();
    void updateItemIcon(QTreeWidgetItem *item);
    QIcon getTagIcon(const std::unordered_set<uint> &vIdTags);
    void updateTitle();


#ifdef USE_KStatusNotifier
    KStatusNotifierItem *statusNotifierItem_;
#else
    QSystemTrayIcon *pTrayIcon_;
#endif
    QMenu *pTrayMenu_;
    QAction *pHideAction_;
    QAction *pShowAction_;
    QActionGroup *pLibGroup_;
    int idCurrentLanguage_;
    uint idCurrentAuthor_;
    ushort idCurrentGenre_;
    uint idCurrentSerial_;
    uint idCurrentBook_;
    std::vector<uint> vBooks_;
    std::vector<uint> vFoundBooks_;
    CoverLabel *pCover;
    bool bTreeView_;
    bool bCollapsed_;
    QByteArray aHeadersTree_;
    QByteArray aHeadersList_;
    enum TabIndex{TabAuthors = 0, TabSeries = 1, TabGenres = 2, TabSearch = 3};
#ifdef USE_HTTSERVER
    std::unique_ptr<opds_server> pOpds_;
#endif
    ImportThread *pImportThread_;
    QThread *pThread_;

protected:
    void closeEvent(QCloseEvent *event) override;
    void FillBookList(QSqlQuery &query);
    void CheckParent(QTreeWidgetItem* parent);
    void CheckChild(QTreeWidgetItem* parent);

    void ExportBookListBtn(bool Enable);
    void FillLibrariesMenu();
    void SendMail(const ExportOptions &exportOptions);
    void SendToDevice(const ExportOptions &exportOptions);
#ifndef USE_KStatusNotifier
    void changeEvent(QEvent *event) override;
#endif
    void onSetRating(QTreeWidgetItem* item, uchar nRating);

private slots:
    void ExportAction();
    void ManageLibrary();
    void onStatistics();
    void onAddBooks();
    void addBooksFinished();
    void CheckBooks();
    void EditBooks();
    void Settings();
    void onSerachAuthorsChanded(const QString& str);
    void onSerachSeriesChanded(const QString& str);
    void btnSearch();
    void SelectAuthor();
    void selectBook();
    void SelectGenre();
    void SelectSeria();
    void onItemChanged(QTreeWidgetItem*,int);
    void BookDblClick();
    void About();
//    void LanguageChange();
    void onStartSearch();
    void onClearSearch();
    void HelpDlg();
    void ContextMenu(QPoint point);
    void HeaderContextMenu(QPoint point);
    void MoveToAuthor(uint id, const QString &FirstLetter);
    void MoveToGenre(uint id);
    void MoveToSeria(uint id, const QString &FirstLetter);
    void onTagFilterChanged(int index);
    void onSetTag();
    void ChangingLanguage();
    void ReviewLink(const QUrl &url);
    void SelectLibrary();
    void onTabWidgetChanged(int index);
    void onLanguageFilterChanged(int index);
    void onChangeAlpabet(const QString &sAlphabetName);
    void onTreeView();
    void onListView();
    void onCollapseAll();
    void onExpandAll();
    void onUpdateSidebarFont(const QFont &font);
    void onUpdateListFont(const QFont &font);
    void onUpdateAnnotationFont(const QFont &font);

    void changingTrayIcon(int index, bool bColor);
#ifdef USE_KStatusNotifier
    void hideEvent(QHideEvent *ev) override;
    void showEvent(QShowEvent *ev) override;
    void handleTrayActivation(bool active, const QPoint &pos);
#else
    void handleTrayActivation(QSystemTrayIcon::ActivationReason reason);
    void MinimizeWindow();
    void hide();
public slots:
    void show();
#endif



signals:
    void window_loaded();
};

#endif // MAINWINDOW_H
