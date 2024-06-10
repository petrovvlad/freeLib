#ifndef EXPORTFRAME_H
#define EXPORTFRAME_H

#include <QFrame>
#include <QSettings>

#include "fontframe.h"
#include "options.h"

namespace Ui {
class ExportFrame;
}

class ExportFrame : public QFrame
{
    Q_OBJECT

public:
    explicit ExportFrame(QWidget *parent = 0);
    ~ExportFrame();
    QStringList Save(ExportOptions *pExportOptions);
    void Load(const ExportOptions *pExportOptions);
signals:
    void ChangeTabIndex(int tab_id,int page_id);
public slots:
    void SetTabIndex(int tab_id,int page_id);
    void UpdateToolComboBox(const QString &sCurrentTool = QStringLiteral(""));
private slots:
    void onRadioDeviceToggled(bool checked);
    void onRadioEmailToggled(bool checked);
    void onOutputFormatChanged(int index);
    void onConnectionTypeChanged(int index);
    void onTabWidgetCurrentChanged(int);
    void onAddCoverLabelClicked();
    void onCreateCaverAlwaysClicked();
    void onCreateCoverClicked();
    void onOriginalFileNameClicked();
    void onMlTocClicked();
    void onPostprocessingCopyClicked();
    void onUserCSSclicked();
    void onBtnDefaultCSSclicked();
    void validateEmail(QLineEdit* leEmail);

    void FontMove(QWidget *font_widget, int direction);
    void RemoveFont(QWidget *font_widget);
    FontFrame *AddFont(bool use=true, int tag=0, const QString &font=QLatin1String(""), const QString &font_b=QLatin1String(""), const QString &font_i=QLatin1String(""), const QString &font_bi=QLatin1String(""), int fontSize=100);
    void btnPath();

private:
    Ui::ExportFrame *ui;
    QRegularExpressionValidator validatorEMail;

    void set_userCSS_clicked();
};

#endif // EXPORTFRAME_H
