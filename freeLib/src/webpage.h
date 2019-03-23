#ifndef WEBPAGE_H
#define WEBPAGE_H
#include <QWebEnginePage>

class WebPage : public QWebEnginePage
{
        Q_OBJECT
public:
    WebPage(QObject* parent = 0);

    bool acceptNavigationRequest(const QUrl & url, QWebEnginePage::NavigationType type, bool)
    {
        switch(type)
        {
            case QWebEnginePage::NavigationTypeLinkClicked:
                emit linkClicked(url);
                return false;
//            case QWebEnginePage::NavigationTypeTyped:
//                return false;
            default:
                return true;
        }
        return true;
    }

signals:
    void linkClicked(const QUrl&);

};

#endif // WEBPAGE_H
