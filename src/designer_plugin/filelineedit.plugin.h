#ifndef FILELINEEDITPLUGIN_H
#define FILELINEEDITPLUGIN_H

#include <QtUiPlugin/customwidget.h>

class FileLineEditPlugin : public QObject, public QDesignerCustomWidgetInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QDesignerCustomWidgetInterface" FILE "filelineedit.json")
    Q_INTERFACES(QDesignerCustomWidgetInterface)

public:
    explicit FileLineEditPlugin(QObject *parent = nullptr);

    // Обязательные методы интерфейса
    QString name() const override;
    QString includeFile() const override;
    QString group() const override;
    QIcon icon() const override;
    QString toolTip() const override;
    QString whatsThis() const override;
    bool isContainer() const override;
    QWidget *createWidget(QWidget *parent) override;
    
    // Дополнительные методы для более тонкой настройки
    QString domXml() const override;
};

#endif // FILELINEEDITPLUGIN_H