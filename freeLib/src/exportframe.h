#ifndef EXPORTFRAME_H
#define EXPORTFRAME_H

#include <QFrame>
#include <QSettings>
#include "fontframe.h"

namespace Ui {
class ExportFrame;
}

class ExportFrame : public QFrame
{
    Q_OBJECT

public:
    explicit ExportFrame(QWidget *parent = 0);
    ~ExportFrame();
    QStringList Save(QSettings *settings,bool save_passwords=true);
    void Load(QSettings *settings);
signals:
    void ChangeTabIndex(int tab_id,int page_id);
public slots:
    void SetTabIndex(int tab_id,int page_id);
private slots:
    void on_radioDevice_toggled(bool checked);

    void on_radioEmail_toggled(bool checked);

    void on_OutputFormat_currentIndexChanged(const QString &arg1);

    void on_ConnectionType_currentIndexChanged(const QString &arg1);

    void FontMove(QWidget *font_widget, int direction);
    void RemoveFont(QWidget *font_widget);
    FontFrame *AddFont(bool use=true, int tag=0, QString font="", QString font_b="", QString font_i="", QString font_bi="", int fontSize=100);
    void btnPath();
    void on_tabWidget_currentChanged(int);
    void UpdateToolComboBox(QSettings *settings=0);

    void on_addCoverLabel_clicked();

    void on_createCaverAlways_clicked();

    void on_createCover_clicked();


    void on_originalFileName_clicked();

    void on_ml_toc_clicked();

    void on_PostprocessingCopy_clicked();

    void on_userCSS_clicked();

    void on_btnDefaultCSS_clicked();

private:
    Ui::ExportFrame *ui;
    void LoadDefault();
    void set_userCSS_clicked();
};

#endif // EXPORTFRAME_H
