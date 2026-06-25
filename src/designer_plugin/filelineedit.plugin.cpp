#include "filelineedit.plugin.h"
#include "../filelineedit.h"
#include <QIcon>
#include <QFileDialog>

FileLineEditPlugin::FileLineEditPlugin(QObject *parent) 
    : QObject(parent) 
{
}

QString FileLineEditPlugin::name() const 
{
    return QStringLiteral("FileLineEdit");
}

QString FileLineEditPlugin::includeFile() const 
{
    return QStringLiteral("filelineedit.h");
}

QString FileLineEditPlugin::group() const 
{
    return QStringLiteral("Custom Widgets");
}

QIcon FileLineEditPlugin::icon() const 
{
    return QIcon();
}

QString FileLineEditPlugin::toolTip() const 
{
    return QStringLiteral("Line edit with file browser button");
}

QString FileLineEditPlugin::whatsThis() const 
{
    return QStringLiteral("A QLineEdit with a button that opens file dialog");
}

bool FileLineEditPlugin::isContainer() const 
{
    return false;  // Ваш виджет не контейнер
}

QWidget* FileLineEditPlugin::createWidget(QWidget *parent) 
{
    return new FileLineEdit(parent);
}

QString FileLineEditPlugin::domXml() const 
{
    // XML-описание для Qt Designer (значения по умолчанию)
    return QString::fromUtf8(
        "<ui language=\"c++\">\n"
        " <widget class=\"FileLineEdit\" name=\"fileLineEdit\">\n"
        "  <property name=\"geometry\">\n"
        "   <rect>\n"
        "    <x>0</x>\n"
        "    <y>0</y>\n"
        "    <width>200</width>\n"
        "    <height>25</height>\n"
        "   </rect>\n"
        "  </property>\n"
        "  <property name=\"nameFilter\">\n"
        "   <string>All files (*.*)</string>\n"
        "  </property>\n"
        "  <property name=\"buttonText\">\n"
        "   <string>…</string>\n"
        "  </property>\n"
        " </widget>\n"
        "</ui>\n");
}