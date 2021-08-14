#include <QApplication>
#include <QStyle>
#include <QStandardPaths>

#include "webpage.h"

WebPage::WebPage(QObject* parent)
    : QWebEnginePage(parent)
{

}

void WebPage::setHtml(const QString &html)
{
    QPalette palette = QApplication::style()->standardPalette();
    setBackgroundColor(palette.color(QPalette::Base));
    QString sHtml = html;

    sHtml.replace("#identifikator_background#",palette.color(QPalette::AlternateBase).name()).
            replace("#color#",palette.color(QPalette::WindowText).name()).
            replace("#a_color#",palette.color(QPalette::Link).name());

    QWebEnginePage::setHtml(sHtml,QUrl("file://"+QStandardPaths::writableLocation(QStandardPaths::TempLocation)));
}
