#include "webpage.h"

#include <QApplication>
#include <QStyle>
#include <QStandardPaths>


WebPage::WebPage(QObject* parent)
    : QWebEnginePage(parent)
{

}

void WebPage::setHtml(const QString &html)
{
    QPalette palette = QApplication::style()->standardPalette();
    setBackgroundColor(palette.color(QPalette::Base));
    QString sHtml = html;

    sHtml.replace(QLatin1String("#identifikator_background#"), palette.color(QPalette::AlternateBase).name()).
            replace(QLatin1String("#color#"), palette.color(QPalette::WindowText).name()).
            replace(QLatin1String("#a_color#"), palette.color(QPalette::Link).name());

    QWebEnginePage::setHtml(sHtml,QUrl("file://" + QStandardPaths::writableLocation(QStandardPaths::TempLocation)));
}
