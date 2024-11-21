#ifndef EXPORTFRAME_H
#define EXPORTFRAME_H

#include <QFrame>
#include <QSettings>
#include <QLineEdit>
#include <QRegularExpressionValidator>

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
    ExportFormat outputFormat();
    bool getUseForHttp();
    void setUseForHttp(bool bUse);

signals:
    void OutputFormatChanged();
    void UseForHttpChanged(int state);
public slots:
    void UpdateToolComboBox(const QString &sCurrentTool = u""_s);
private slots:
    void onUseForHttpChanged(int state);
    void onRadioDeviceToggled(bool checked);
    void onRadioEmailToggled(bool checked);
    void onOutputFormatChanged(int index);
    void onConnectionTypeChanged(int index);
    void onOriginalFileNameChanged(int state);
    void onPostprocessingCopyChanged(int state);
    void validateEmail(QLineEdit* leEmail);

    void btnPath();

private:
    Ui::ExportFrame *ui;
    QRegularExpressionValidator validatorEMail;
};

#endif // EXPORTFRAME_H
