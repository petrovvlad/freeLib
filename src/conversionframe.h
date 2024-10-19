#ifndef CONVERSIONFRAME_H
#define CONVERSIONFRAME_H

#include <QFrame>

#include "fontframe.h"
#include "options.h"

namespace Ui {
class ConversionFrame;
}

class ConversionFrame : public QFrame
{
    Q_OBJECT

public:
    explicit ConversionFrame(QWidget *parent = nullptr);
    ~ConversionFrame();
    void Load(const ExportOptions *pExportOptions);
    QStringList Save(ExportOptions *pExportOptions);
    void setCurrentTab(int index);

signals:
    void changeTabIndex(int index);
private:
    Ui::ConversionFrame *ui;
    void userCssChanged(int state);

private slots:
    void onAddCoverLabelChanged(int state);
    void onCreateCaverAlwaysChanged(int state);
    void onMlTocChanged(int state);
    void onUserCssChanged(int state);
    void onBtnDefaultCSSclicked();
    void onTabWidgetCurrentChanged(int index);

    void FontMove(QWidget *font_widget, int direction);
    void RemoveFont(QWidget *font_widget);
    FontFrame *AddFont(bool use=true, int tag=0, const QString &font=u""_s, const QString &font_b=u""_s, const QString &font_i=u""_s, const QString &font_bi=u""_s, int fontSize=100);

};

#endif // CONVERSIONFRAME_H
