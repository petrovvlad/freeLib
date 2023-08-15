#ifndef FONTFRAME_H
#define FONTFRAME_H

#include <QFrame>

namespace Ui {
class FontFrame;
}

class FontFrame : public QFrame
{
    Q_OBJECT

public:
    explicit FontFrame(bool use=true, int tag=0, const QString &font=QLatin1String(""), const QString &font_b=QLatin1String(""), const QString &font_i=QLatin1String(""), const QString &font_bi=QStringLiteral(""),int fontSize=100, QWidget *parent = 0);
    ~FontFrame();
    bool use();
    quint8 tag();
    int fontSize();
    //bool italic();
    //bool bold();
    QString font();
    QString font_b();
    QString font_i();
    QString font_bi();
private:
    Ui::FontFrame *ui;
    int current_font;
    int current_font_b;
    int current_font_i;
    int current_font_bi;
private slots:
    void UseChange(bool state);
    void DelPress();
    void UpPress();
    void DownPress();
    void FontSelected(const QString &str);
    void onTagCurrentIndexChanged(int index);

signals:
    void remove_font(QWidget* widget);
    void move_font(QWidget* widget, int direction);
};

#endif // FONTFRAME_H
