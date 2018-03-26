#ifndef HYPHENATIONS_H
#define HYPHENATIONS_H
#include "../common.h"
#include <QStringList>

#define SOFT_HYPHEN  "&#173;" //&shy;"
#define CHILD_HYPHEN  "-"
//#define SOFT_HYPHEN  "-"

struct pattern_t
{
    QString str;
    QVector<int> levels;
};

struct pattern_list_t
{
        QVector<pattern_t*> list;
};

class hyphenations
{
public:
    hyphenations();
    void init(QString language);
    QString hyphenate_word(QString word, QString separator="-",bool hyphenation_only=false);
private:
    QStringList pattern;
    //QStringList exceptions;
    QStringList _hyphenate_word(QString word);
    pattern_list_t plist;
    QVector<int> wh;
    //QString abc;
    //QString abc_glasnie;
};

#endif // HYPHENATIONS_H
