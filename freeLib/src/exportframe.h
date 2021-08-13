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
    void UpdateToolComboBox(QSettings *settings=0);
private slots:
    void onRadioDeviceToggled(bool checked);
    void onRadioEmailToggled(bool checked);
    void onOutputFormatChanged(const QString &arg1);
    void onConnectionTypeChanged(const QString &arg1);
    void onTabWidgetCurrentChanged(int);
    void onAddCoverLabelClicked();
    void onCreateCaverAlwaysClicked();
    void onCreateCoverClicked();
    void onOriginalFileNameClicked();
    void onMlTocClicked();
    void onPostprocessingCopyClicked();
    void onUserCSSclicked();
    void onBtnDefaultCSSclicked();

    void FontMove(QWidget *font_widget, int direction);
    void RemoveFont(QWidget *font_widget);
    FontFrame *AddFont(bool use=true, int tag=0, QString font="", QString font_b="", QString font_i="", QString font_bi="", int fontSize=100);
    void btnPath();

private:
    Ui::ExportFrame *ui;
    void LoadDefault();
    void set_userCSS_clicked();
};

#endif // EXPORTFRAME_H
