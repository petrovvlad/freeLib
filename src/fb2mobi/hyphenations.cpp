#include "hyphenations.h"

#include <QApplication>
#include <QTextStream>
#include <QtAlgorithms>
#include <QStringBuilder>
#include <QFile>

#include "utilites.h"

hyphenations::hyphenations()
{
}

bool pattern_compare(const pattern_t* a, const pattern_t* b)
{
    bool first = a->str.length() < b->str.length();
    int min_size = first ? a->str.length() : b->str.length();
    for (int i = 0; i < min_size; ++i)
    {
            if (a->str.at(i) < b->str.at(i))
                    return true;
            else if (a->str.at(i) > b->str.at(i))
                    return false;
    }
    return first;
}

void hyphenations::init(const QString &language)
{
    QFile file(u":/xsl/hyphenations/"_s % language.toLower() % u".txt"_s);
    if(!file.open(QFile::ReadOnly))
    {
        LogWarning << "Error open file" << file.fileName();
        return;
    }
    QTextStream ts(&file);
    pattern = ts.readAll().trimmed().replace('\n', ' ').split(QStringLiteral(" "));
    file.close();
//    file.setFileName(QApplication::applicationDirPath()+"/xsl/hyphenations/"+language.toLower()+"_ex.txt");
//    if(!file.open(QFile::ReadOnly))
//    {
//        qDebug()<<"Error open file "+file.fileName();
//    }
//    //    exceptions=ts.readAll().trimmed().replace("\n"," ").split(" ");
//    file.close();
//    file.setFileName(QApplication::applicationDirPath()+"/xsl/hyphenations/"+language.toLower()+"_abc.txt");
//    if(!file.open(QFile::ReadOnly))
//    {
//        qDebug()<<"Error open file "+file.fileName();
//    }
//    abc=ts.readLine();
//    abc_glasnie=ts.readLine();
    //file.close();
    if(language == u"ru")
    {
      //  file.setFileName(QApplication::applicationDirPath()+"/xsl/hyphenations/en.txt");
  //      if(!file.open(QFile::ReadOnly))
  //      {
  //          qDebug()<<"Error open file "+file.fileName();
  //      }
  //      pattern+=ts.readAll().trimmed().replace("\n"," ").split(" ");
  //      file.close();
     //   file.setFileName(QApplication::applicationDirPath()+"/xsl/hyphenations/en_ex.txt");
  //      if(!file.open(QFile::ReadOnly))
  //      {
  //          qDebug()<<"Error open file "+file.fileName();
  //      }
  //      exceptions+=ts.readAll().trimmed().replace("\n"," ").split(" ");
  //      file.close();
    }
    for(const QString &str: std::as_const(pattern))
    {
        if(!str.trimmed().isEmpty())
        {
            pattern_t* p = new pattern_t;
            bool wait_digit = true;
            for(int i=0; i<str.length(); i++)
            {
                QChar c = str[i];
                if(c.isDigit())
                {
                    p->levels.push_back(c.digitValue());
                    //p->str.push_back(c);
                    wait_digit = false;
                }
                else
                {
                    if(c != '.' && wait_digit)
                        p->levels.push_back(0);
                    p->str.push_back(c);
                    wait_digit = true;
                }
            }
            if(wait_digit)
                p->levels.push_back(0);
            if(!p->str.isEmpty())
                plist.list.push_back(p);
        }
     }
     std::sort(plist.list.begin(), plist.list.end(), pattern_compare);
}


QString hyphenations::hyphenate_word(QString word_original, const QString &separator,bool /*hyphenation_only*/)
{
    if(plist.list.count() == 0)
        return word_original;
    //qDebug()<<"1";
    QString result_word;
    QStringList words = word_original.split('-');
    //qDebug()<<words;
    bool last_word_empty = false;
    for(QString &word: words)
    {
        //qDebug()<<word;
        if(word.length() < 3 /*&& hyphenation_only*/)
        {
            result_word += ((last_word_empty || !result_word.isEmpty()) ?u"-"_s :u""_s) + word;
            last_word_empty = word.isEmpty();
            continue;
        }
        //qDebug()<<last_word_empty;

    //    if(word.length()<4 && hyphenation_only)
    //        return word;
        //qDebug()<<"2";
        QString begin_points, end_points;
        for(int i=0; i < word.length(); ++i)
        {
            //if(!abc.contains(word.mid(i,1),Qt::CaseInsensitive))
            if(!word[i].isLetter())
            {
                begin_points = word.left(i + 1);
            }
            else
            {
                break;
            }
        }
        word = word.right(word.length()-begin_points.length());
        for(int i = word.length()-1; i>=0 ;--i)
        {
            if(!word[i].isLetter())
            {
                end_points = word.right(word.length()-i);
            }
            else
            {
                break;
            }
        }
        word = word.left(word.length() - end_points.length());
        if(begin_points == word)
            return word;
        //qDebug()<<"3";

        QString word_string = QStringLiteral(".") + word.toLower() + QStringLiteral(".");
        QVector<int> levels;
        levels.fill(0, word_string.size());
        //qDebug()<<plist.list.count();
        for(int i=0; i<word_string.length()-2; ++i)
        {
            QVector<pattern_t*>::iterator pattern_iter = plist.list.begin();
            for(int count=1; count<=word_string.length()-i; ++count)
            {
                pattern_t pattern_from_word;
                pattern_from_word.str = word_string.mid(i, count);
                if (pattern_compare(&pattern_from_word, *pattern_iter))
                        continue;
                pattern_iter = std::lower_bound(pattern_iter, plist.list.end(), &pattern_from_word, pattern_compare);
                if (pattern_iter == plist.list.end())
                        break;
                if (!pattern_compare(&pattern_from_word, *pattern_iter))
                {
                        for (int level_i = 0; level_i < (*pattern_iter)->levels.size(); ++level_i)
                        {
                                int l = (*pattern_iter)->levels[level_i];
                                if (l > levels[i+level_i])
                                        levels[i+level_i] = l;
                        }
                }
            }
        }
        wh.fill(0, levels.count()-2);
        for(int i=0; i<wh.count(); ++i)
        {
            if(levels[i+1]%2 && i)
                wh[i] = 1;
            else
                wh[i] = 0;
         }
        QString result;
        int len = word.length();
        //bool slog_yest_glasnaya=false;
        QString slog;
        for(int i=0; i<len; i++)
        {
                //qDebug()<<"dd";
                if(!(!wh[i] /*|| (!slog_yest_glasnaya)*/))
                {
                    result += ((result.isEmpty() || (result.length()<2 /*&& hyphenation_only*/)) ?u""_s :separator);
                    result += slog;
                   // slog_yest_glasnaya=false;
                    slog = u""_s;
                }
            //}
            slog += word[i];
           // if(abc_glasnie.contains(word[i]))
            //    slog_yest_glasnaya=true;
        }
        result += ((result.isEmpty()|| (result.length()<2 /*&& hyphenation_only*/) || slog.length()<2)?QStringLiteral(""):separator);
        result += slog;

        result_word += ((result_word.length()>0 || last_word_empty) ?u"-"_s :u""_s) % begin_points+result % end_points;
        last_word_empty = false;
    }
    return result_word;
}
