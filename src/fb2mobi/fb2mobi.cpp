#include "fb2mobi.h"

#include <QProcess>
#include <QUuid>
#include <QPainter>
#include <QDir>
#include <QDebug>
#include <QRegularExpression>
#include <QStringBuilder>
#include <unordered_set>

#ifdef QUAZIP_STATIC
#include "../quazip/quazip/quazip.h"
#include "../quazip/quazip/quazipfile.h"
#else
#include <quazip/quazip.h>
#include <quazip/quazipfile.h>
#endif

#include "../mobiEdit/mobiedit.h"
#include "config-freelib.h"
#include "utilites.h"

fb2mobi::fb2mobi(const ExportOptions *pExportOptions, uint idLib)
{
    pExportOptions_ = pExportOptions;
    idLib_= idLib;
    need_page_break = false;
    first_body = true;
    bHeader_ = false;
    bInlineImageMode_ = false;
    current_header_level = 0;
    current_section_level = 0;
    first_header_in_body = false;
    sNoDropcaps_ = u"'\"-.…0123456789‒–—"_s;
    toc_index = 0;
    toctitle = tr("Contents");
    bNoParagraph_ = false;
    dodropcaps = false;
    first_chapter_line = false;
    bSubHeader_ = false;
    bAannotation_ = false;
    toc_max_level = 1000000;
    sTmpDir_ = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + u"/freeLib"_s;
    QDir dir(sTmpDir_);
    dir.mkpath(dir.path());
    sAnnotationTitle_ = tr("Abstract");
    sNotesTitle_ = tr("Notes");

    if(!pExportOptions_->sBookSeriesTitle.isEmpty())
        bookseriestitle = pExportOptions_->sBookSeriesTitle;
    else
        bookseriestitle = ExportOptions::sDefaultBookTitle;
    if(! pExportOptions_->sAuthorSring.isEmpty())
        authorstring = pExportOptions_->sAuthorSring;
    else
        authorstring = ExportOptions::sDefaultAuthorName;
    join_seria = false;
}

QString test_language(QString language)
{
    static std::unordered_set<QString> l = {u"af"_s, u"sq"_s, u"ar"_s, u"hy"_s, u"az"_s, u"eu"_s, u"be"_s, u"bn"_s,
                                            u"bg"_s, u"ca"_s, u"zh"_s, u"hr"_s, u"cs"_s, u"da"_s, u"nl"_s, u"en"_s,
                                            u"et"_s, u"fo"_s, u"fa"_s, u"fi"_s, u"fr"_s, u"ka"_s, u"de"_s, u"gu"_s,
                                            u"he"_s, u"hi"_s, u"hu"_s, u"is"_s, u"id"_s, u"it"_s, u"ja"_s, u"kn"_s,
                                            u"kk"_s, u"x-kok"_s, u"ko"_s, u"lv"_s, u"lt"_s, u"mk"_s, u"ms"_s, u"ml"_s,
                                            u"mt"_s, u"mr"_s, u"ne"_s, u"no"_s, u"or"_s, u"pl"_s, u"pt"_s, u"pa"_s,
                                            u"rm"_s, u"ro"_s, u"ru"_s, u"sz"_s, u"sa"_s, u"sr"_s, u"sk"_s, u"sl"_s,
                                            u"sb"_s, u"es"_s, u"sx"_s, u"sw"_s, u"sv"_s, u"ta"_s, u"tt"_s, u"te"_s,
                                            u"th"_s, u"ts"_s, u"tn"_s, u"tr"_s, u"uk"_s, u"ur"_s, u"uz"_s, u"vi"_s,
                                            u"xh"_s, u"zu"_s};
    if(l.contains(language.toLower()))
        return language;
    for(const auto &str: l)
        if(language.startsWith(str))
            return str;
    return u"en"_s;
}

const QString fb2mobi::sHtmlHead_ = u"<html xmlns=\"http://www.w3.org/1999/xhtml\" xmlns:epub=\"http://www.idpf.org/2007/ops\">"
            "<head>"
                "<title>freeLib</title>"
                "<link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\"/>"
                "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"/>"
            "</head>"
            "<body>"_s;

const QString fb2mobi::sHtmlFoot_ = u"</body></html>"_s;

void fb2mobi::parse_description(const QDomNode &elem)
{
    if(join_seria)
        sBookName_ = pBook_->mSequences.empty() ?u""_s :g::libs[idLib_].serials[pBook_->mSequences.begin()->first].sName;
    else
        sBookName_ = pBook_->sName;
    book_author = authorstring;
    book_author = g::libs[idLib_].fillParams(book_author, idBook_);
    if(pExportOptions_->bAuthorTranslit)
        book_author = Transliteration(book_author);
    isbn = pBook_->sIsbn;
    for(int e=0; e<elem.childNodes().count(); e++)
    {
         if(elem.childNodes().at(e).toElement().tagName() == u"title-info")
         {
             QDomNode ti=elem.childNodes().at(e);
             for(int t=0; t<ti.childNodes().count(); t++)
             {
                 if(ti.childNodes().at(t).toElement().tagName() == u"annotation")
                 {
                 //    if(!hide_annotation)
                     {
                         pBufCurrent_ = &sBufAnnotation_;
                         *pBufCurrent_ = sHtmlHead_;
                         *pBufCurrent_ += u"<div class=\"annotation\"><div class=\"h1\">%1</div>"_s.arg(sAnnotationTitle_);
                         bAannotation_ = true;
                         parse_format(ti.childNodes().at(t), u"div"_s);
                         bAannotation_ = false;
                         *pBufCurrent_ += u"</div>"_s;
                         *pBufCurrent_ += sHtmlFoot_;
                         pBufCurrent_ = &vHtmlFiles_.back().sContent;
                     }

                 }
                 else if(ti.childNodes().at(t).toElement().tagName() == u"coverpage")
                 {
                     QDomNode image = ti.childNodes().at(t);
                     for(int i=0; i<image.childNodes().count(); i++)
                     {
                         if(image.childNodes().at(i).toElement().tagName() == u"image")
                         {
                             for(int j=0; j<image.childNodes().at(i).attributes().count(); j++)
                             {
                                 if(image.childNodes().at(i).attributes().item(j).nodeName().right(href_pref.length()) == href_pref)
                                 {
                                     sFileCover_ = u"img%1/"_s.arg(current_book) +
                                             image.childNodes().at(i).attributes().item(j).toAttr().value().replace('#', u""_s);
                                 }
                             }
                             break;
                         }
                     }
                 }
             }
         }
         else if(elem.childNodes().at(e).toElement().tagName() == u"publish-info")
         {
             QDomNode ti = elem.childNodes().at(e);
             for(int t=0; t<ti.childNodes().count(); t++)
             {
                 if(ti.childNodes().at(t).toElement().tagName() == u"isbn")
                 {
                    isbn = ti.childNodes().at(t).toElement().toElement().text().trimmed().replace(u"-"_s, u""_s);
                 }
             }
         }
    }
    hyphenator.init(test_language(g::libs[idLib_].vLaguages[pBook_->idLanguage]));
}

void fb2mobi::parse_binary(const QDomNode &elem)
{
    QString filename = elem.attributes().namedItem(u"id"_s).toAttr().value();
    if(!filename.isEmpty())
    {
        QFileInfo fi(filename);
        if(fi.suffix().isEmpty())
        {
            if(sFileCover_.right(filename.length()) == filename)
                 sFileCover_ += u".jpg"_s;
            filename += u".jpg"_s;
        }
        QDir dir;
        dir.mkpath(sTmpDir_ + u"/OEBPS/img%1/"_s.arg(current_book));
        QFile file(sTmpDir_ % u"/OEBPS/img%1/"_s.arg(current_book) % filename);
        file.open(QIODevice::WriteOnly);
        file.write(QByteArray::fromBase64((elem.toElement().text().toStdString().c_str())));
        file.close();
        vImageList_.emplace_back(u"img%1/"_s.arg(current_book) + filename);
    }
}

void fb2mobi::parse_body(const QDomNode &elem)
{
    body_name = u""_s;
    if(!elem.attributes().namedItem(u"name"_s).isNull())
        body_name = elem.attributes().namedItem(u"name"_s).toAttr().value();
    current_header_level = 0;
    first_header_in_body = true;
    parse_format(elem);
    first_body = false;
}

void fb2mobi::parse_p(const QDomNode &elem)
{
    QString pcss;
    QString ptag = u"p"_s;
    if(bHeader_)
        pcss = u"css"_s;
    else
        pcss = u"text"_s;
    parse_format(elem, ptag, pcss);
}

void fb2mobi::parse_other(const QDomNode &elem)
{
    parse_format(elem,elem.toElement().tagName());
}

void fb2mobi::parse_section(const QDomNode &elem)
{
    current_header_level++;
    current_section_level++;
    need_end_chapter_vignette = true;
    parse_format(elem, u"div"_s, u"section"_s);
    if(body_name.isEmpty() && pExportOptions_->nVignette > 0 && need_end_chapter_vignette)
    {
        QString level = u"h%1"_s.arg(current_header_level <= 6 ?current_header_level :6);
        QString vignet = get_vignette(level, VIGNETTE_CHAPTER_END);
        if(!vignet.isEmpty())
            *pBufCurrent_ += vignet;
        need_end_chapter_vignette = false;
    }
    current_header_level--;
    if(!parsing_note)
    {
        while(current_section_level > 0)
        {
            *pBufCurrent_ += u"</div>"_s;
            current_section_level--;
        }
        if(pExportOptions_->bSplitFile )
        {

            vHtmlFiles_.back().sContent += sHtmlFoot_;
            vHtmlFiles_.emplace_back(u"section%1.html"_s.arg(vHtmlFiles_.size()));
            vHtmlFiles_.back().sContent = sHtmlHead_;
            if(!bAannotation_)
                pBufCurrent_ = &vHtmlFiles_.back().sContent;
        }
    }
    else if(parsing_note)
    {
        current_section_level = 0;
    }

}

QString fb2mobi::get_vignette(const QString &level, const QString &type)
{
    QString result;
    if(pExportOptions_->nVignette == 1)
    {
        if(QFileInfo::exists(sTmpDir_ % u"/OEBPS/pic/"_s % level.toLower() % type))
            result = u""_s;
        if(QFileInfo::exists(sTmpDir_ % u"/OEBPS/pic/"_s % level.toLower() % type % u".png"_s))
            result = level.toLower() % type % u".png"_s;
        if(QFileInfo::exists(sTmpDir_ % u"/OEBPS/pic/"_s % type % u".png"_s))
            result = type + u".png"_s;
        if(!result.isEmpty())
        {
            if(type == VIGNETTE_CHAPTER_END)
            {
                result = u"<div class=\"vignette_chapter_end\"><img  alt=\"\" src=\"pic/%1\"/></div>"_s.arg(result);
            }
            else if(type == VIGNETTE_TITLE_BEFORE)
            {
                result = u"<div class=\"vignette_title_before\"><img  alt=\"\" src=\"pic/%1\"/></div>"_s.arg(result);
            }
            else if(type == VIGNETTE_TITLE_AFTER)
            {
                result = u"<div class=\"vignette_title_after\"><img  alt=\"\" src=\"pic/%1\"/></div>"_s.arg(result);
            }
        }
    }
    else if(pExportOptions_->nVignette == 2)
    {
        QString file;
        QString txt_dir = u":/xsl/img/"_s;
        //проверить: похоже лишние строки
//        if(QFileInfo::exists(txt_dir + level.toLower() + type))
//            file = QLatin1String("");
        if(QFileInfo::exists(txt_dir % level.toLower() % type % u".txt"_s))
            file = txt_dir % level.toLower() % type % u".txt"_s;
        if( QFileInfo::exists(txt_dir % type % u".txt"_s) )
            file = txt_dir % type % u".txt"_s;
        if(!file.isEmpty())
        {
            QFile vig_file(file);
            if(vig_file.open(QFile::ReadOnly))
                result = QString::fromUtf8(vig_file.readAll());
        }
        if(!result.isEmpty())
        {
            if(type == VIGNETTE_CHAPTER_END)
            {
                result = u"<div class=\"vignette_chapter_end\">%1</div>"_s.arg(result);
            }
            else if(type == VIGNETTE_TITLE_BEFORE)
            {
                result = u"<div class=\"vignette_title_before\">%1</div>"_s.arg(result);
            }
            else if(type == VIGNETTE_TITLE_AFTER)
            {
                result = u"<div class=\"vignette_title_after\">%1</div>"_s.arg(result);
            }
        }
    }
    return result;
}

void fb2mobi::parse_note_elem(const QDomNode &elem)
{
       QString sNoteTitle;

       if(elem.toElement().tagName() == u"section" &&  elem.attributes().contains(u"id"_s))
       {
           QString id = elem.attributes().namedItem(u"id"_s).toAttr().value();
           QString sNoteText;
           for(int e=0; e<elem.childNodes().count(); e++)
           {
               QString str;
               QTextStream stream(&str);
               elem.childNodes().at(e).save(stream, 4);
               static const QRegularExpression re(u"<[^>]*>"_s);
               str.replace(re, u" "_s);
               if(elem.childNodes().at(e).toElement().tagName() == u"title" || elem.childNodes().at(e).toElement().tagName() == u"subtitle")
               {
                  sNoteTitle = str.trimmed();
               }
               else
               {
                   if(pExportOptions_->nFootNotes == 1)
                       sNoteText += str.trimmed();
               }
           }
           if(pExportOptions_->nFootNotes != 1)
           {
                  QString* cur = pBufCurrent_;
                  pBufCurrent_ = &sNoteText;
                  parse_format(elem);
                  pBufCurrent_ = cur;

           }
           vNotesDict.push_back({id, {sNoteTitle.replace(u"&nbsp;"_s, u" "_s).trimmed(), sNoteText.replace(u"&nbsp;"_s, u" "_s).trimmed()}});
       }
       else
       {
           for(int e=0; e<elem.childNodes().count(); e++)
              parse_note_elem(elem.childNodes().at(e));
       }
}

void fb2mobi::get_notes_dict(const QString &body_names)
{
    parsing_note = true;
    vNotesDict.clear();
    QDomNode root = doc.documentElement();

    for(int i=0; i<root.childNodes().count();i++)
    {
        QDomNode item = root.childNodes().at(i);
        if(item.toElement().tagName() == u"body")
        {
            if(item.attributes().contains(u"name"_s))
            {
                for(int e=0; e<item.childNodes().count(); e++)
                    parse_note_elem(item.childNodes().at(e));
            }
        }
    }
    parsing_note = false;
}

void fb2mobi::parse_title(const QDomNode &elem)
{
    QString toc_ref_id = u"tocref%1"_s.arg(toc_index);
    if(parsing_note)
    {
        return;
    }

    QString toc_title;
    QString str;
    QTextStream stream(&str);
    elem.save(stream, 4);
    static const QRegularExpression re1(u"</p>|<br>|<br/>"_s, QRegularExpression::CaseInsensitiveOption);
    str.replace(re1, u" "_s);

    static const QRegularExpression re2(u"<[^>]*>"_s);
    QStringList list = str.replace(u"\n"_s , u""_s).
                           replace(u"\r"_s, u""_s).
                           replace(re2, u""_s).
                           split(u" "_s);
    for(const QString &i: std::as_const(list))
    {
        toc_title += (i.trimmed().isEmpty() ?u""_s :QString(i.trimmed() + u" "_s));
    }
    toc_title = toc_title.trimmed();

    if(pExportOptions_->bBreakAfterCupture && need_page_break && !pExportOptions_->bSplitFile  && !parsing_note)
    {
        *pBufCurrent_ += u"<div style=\"page-break-before:always;\"></div>"_s;
        need_page_break = false;
    }
    if(body_name.isEmpty() || first_header_in_body || first_body)
    {
        bHeader_ = true;
        first_chapter_line = true;
        *pBufCurrent_ += u"<div class=\"titleblock\" id=\"%1\">"_s.arg(toc_ref_id);
        if((body_name.isEmpty() || first_body) && first_header_in_body)
        {
            if(pExportOptions_->nVignette > 0 && !toc_title.isEmpty())
            {
                QString vignet = get_vignette(u"h0"_s, VIGNETTE_TITLE_BEFORE);
                if(!vignet.isEmpty())
                    *pBufCurrent_ += vignet;
            }
            parse_format(elem, u"div"_s, u"h0"_s);
            current_header_level = 0;
            if(pExportOptions_->nVignette > 0 && !toc_title.isEmpty())
            {
                QString vignet = get_vignette(u"h0"_s, VIGNETTE_TITLE_AFTER);
                if(!vignet.isEmpty())
                    *pBufCurrent_ += vignet;
            }
        }
        else
        {
            QString level = u"h%1"_s.arg(current_header_level <= 6 ?current_header_level :6);
            if(pExportOptions_->nVignette > 0 && !toc_title.isEmpty())
            {
                QString vignet = get_vignette(level, VIGNETTE_TITLE_BEFORE);
                if(!vignet.isEmpty())
                    *pBufCurrent_ += vignet;
            }
            parse_format(elem, u"div"_s, level);
            if(pExportOptions_->nVignette > 0 && !toc_title.isEmpty())
            {
                QString vignet = get_vignette(level, VIGNETTE_TITLE_AFTER);
                if(!vignet.isEmpty())
                    *pBufCurrent_ += vignet;
            }
        }
        if(!toc_title.isEmpty())
        {
            STOC c_toc = {u"%1#%2"_s.arg(vHtmlFiles_.back().sFileName, toc_ref_id), toc_title, current_header_level, body_name, u""_s};
            toc.push_back(std::move(c_toc));
        }
    }
    else
    {
        *pBufCurrent_ += u"<div class=\"titleblock\" id=\"%1\">"_s.arg(toc_ref_id);
        parse_format(elem, u"div"_s);
    }

    *pBufCurrent_ += u"</div>\n"_s;

    toc_index++;
    first_header_in_body = false;
    bHeader_ = false;
}

void fb2mobi::parse_image(const QDomNode &elem)
{
    QString image;
    QString img_id;
    for(int i=0; i<elem.attributes().count(); i++)
    {
        if(elem.attributes().item(i).toAttr().name().right(href_pref.length()) == href_pref)
        {
            image = elem.attributes().item(i).toAttr().value();
            image = image.replace(u"#"_s, u""_s);
            if(!image.isEmpty())
            {
                QFileInfo fi(image);
                if(fi.suffix().isEmpty())
                    image += u".jpg"_s;
            }
        }
        else if(elem.attributes().item(i).toAttr().name() == u"id"_s)
            img_id = elem.attributes().item(i).toAttr().value();
    }
    if(bInlineImageMode_)
    {
        if(!img_id.isEmpty())
            *pBufCurrent_ += u"<img id=\"%1\" class=\"inlineimage\" src=\"img%3/%2\" alt=\"%2\"/>"_s.arg(img_id, image, QString::number(current_book));
        else
            *pBufCurrent_ += u"<img class=\"inlineimage\" src=\"img%2/%1\" alt=\"%1\"/>"_s.arg(image, QString::number(current_book));
    }
    else
    {
        if(!img_id.isEmpty())
            *pBufCurrent_ += u"<div id=\"%1\" class=\"image\">"_s.arg(img_id);
        else
            *pBufCurrent_ += u"<div class=\"image\">"_s;
        *pBufCurrent_ += u"<img src=\"img%2/%1\" alt=\"%1\"/>"_s.arg(image, QString::number(current_book));
        *pBufCurrent_ += u"</div>"_s;
    }
    parse_format(elem);
}

void fb2mobi::parse_emptyline(const QDomNode&)
{
    *pBufCurrent_ += u"<br/>"_s;
}

void fb2mobi::parse_epigraph(const QDomNode &elem)
{
    bNoParagraph_ = true;
    parse_format(elem, u"div"_s, u"epigraph"_s);
    bNoParagraph_ = false;
}

void fb2mobi::parse_annotation(const QDomNode &elem)
{
    bNoParagraph_ = true;
    parse_format(elem, u"div"_s, u"annotation"_s);
    bNoParagraph_ = false;
}

void fb2mobi::parse_a(const QDomNode &elem)
{
    for(int j=0; j<elem.attributes().count(); j++)
    {
        if(elem.attributes().item(j).nodeName().right(href_pref.length()) == href_pref)
        {
             parse_format(elem, u"a"_s, u"anchor"_s, elem.attributes().item(j).toAttr().value().toHtmlEscaped());
             break;
        }
    }
}

void fb2mobi::parse_emphasis(const QDomNode &elem)
{
    parse_span(u"emphasis"_s, elem);
}

void fb2mobi::parse_strong(const QDomNode &elem)
{
    parse_span(QStringLiteral("strong"), elem);
}

void fb2mobi::parse_strikethrough(const QDomNode &elem)
{
    parse_span(u"strike"_s, elem);
}

void fb2mobi::parse_span(const QString &span, const QDomNode &elem)
{
    parse_format(elem, u"span"_s, span);
}

void fb2mobi::parse_textauthor(const QDomNode &elem)
{
    bNoParagraph_ = true;
    parse_format(elem, u"div"_s, u"text-author"_s);
    bNoParagraph_ = false;
}

void fb2mobi::parse_v(const QDomNode &elem)
{
    parse_format(elem, u"p"_s);
}

void fb2mobi::parse_poem(const QDomNode &elem)
{
    bNoParagraph_ = true;
    parse_format(elem, u"div"_s, u"poem"_s);
    bNoParagraph_ = false;
}

void fb2mobi::parse_stanza(const QDomNode &elem)
{
    parse_format(elem, u"div"_s, u"stanza"_s);
}

void fb2mobi::parse_table(const QDomNode &elem)
{
    *pBufCurrent_ += u"<table class=\"table\""_s;
    for(int i=0; i<elem.attributes().count(); i++)
    {
        *pBufCurrent_ += u" %1=\"%2\""_s.arg(elem.attributes().item(i).toAttr().name(), elem.attributes().item(i).toAttr().value());
    }
    *pBufCurrent_ += u">"_s;
    parse_format(elem);
    *pBufCurrent_ += u"</table>"_s;
}

void fb2mobi::parse_table_element(const QDomNode &elem)
{
    *pBufCurrent_ += u"<"_s + elem.toElement().tagName();

    for(int i=0; i<elem.attributes().count(); i++)
    {
        *pBufCurrent_ += u" %1=\"%2\""_s.arg(elem.attributes().item(i).toAttr().name(), elem.attributes().item(i).toAttr().value());
    }

    *pBufCurrent_ += u">"_s;
    parse_format(elem);
    *pBufCurrent_ += u"</"_s % elem.toElement().tagName() % u">"_s;
}

void fb2mobi::parse_code(const QDomNode &elem)
{
    parse_format(elem, u"code"_s);
}

void fb2mobi::parse_date(const QDomNode &elem)
{
    parse_format(elem, u"time"_s);
}

void fb2mobi::parse_subtitle(const QDomNode &elem)
{
    if(parsing_note)
        return;
    bSubHeader_ = true;
    parse_format(elem, u"p"_s, u"subtitle"_s);
    bSubHeader_ = false;
}

void fb2mobi::parse_style(const QDomNode&)
{
    return;
}

void fb2mobi::parse_cite(const QDomNode &elem)
{
    parse_format(elem, u"div"_s, u"cite"_s);
}

QString fb2mobi::save_html(const QString &str)
{
    return QString(str).toHtmlEscaped();
}

void fb2mobi::parse_format(const QDomNode &elem, QString tag , QString css, QString href)
{
    if(!tag.isEmpty())
    {
        dodropcaps = false;
        if(pExportOptions_->bDropCaps && first_chapter_line && !(bHeader_ || bSubHeader_) && body_name.isEmpty() && tag.toLower() == u"p")
        {
            if(!bNoParagraph_)
            {
                if(sNoDropcaps_.indexOf(elem.toElement().text()) < 0)
                {
                    dodropcaps = true;
                    css = u"dropcaps"_s;
                }
                first_chapter_line = false;
            }
        }

        if(pExportOptions_->nFootNotes>0 && tag.toLower() == u"a" && pExportOptions_->nFootNotes != 3)
        {
            QString note_id = href.right(href.length()-1);
            for(int i=0; i<vNotesDict.size(); i++)
            {
                if(vNotesDict.at(i).first == note_id){
                    vCurrentNotes_.push_back(vNotesDict.at(i).second);
                    tag = u"span"_s;
                    css = (pExportOptions_->nFootNotes==1 ?u"inline"_s :u"block"_s) + u"anchor"_s;
                    href = u""_s;
                    break;
                }
            }
        }

        QString str;
        str = u"<"_s + tag;
        if(!css.isEmpty())
            str += u" class=\"%1\""_s.arg(css);
        if(pExportOptions_->nFootNotes==3 && tag.toLower() == u"a" && outputFormat_ == epub)
        {
            str += u" epub:type=\"noteref\""_s;
        }
        if(!href.isEmpty() && elem.attributes().namedItem(u"id"_s).isNull())
        {
            elem.toElement().setAttribute(u"id"_s, u"tocref%1"_s.arg(toc_index));
            toc_index++;
        }
        if(!elem.attributes().namedItem(u"id"_s).isNull())
        {
            QString id = elem.attributes().namedItem(u"id"_s).toAttr().value();
            QString sFileName = vHtmlFiles_.back().sFileName;
            str += u" id=\"%1\""_s.arg(id);
            refFiles_[id] = sFileName + (id.left(1)==u"#" ?u""_s :u"#"_s) + id;
            crossingRef_[id].from = sFileName + (id.left(1)==u"#" ?u""_s :u"#"_s) + id;
            crossingRef_[id].to = href;
        }
        if(!href.isEmpty())
            str += u" href=\"%1\""_s.arg(href);
        *pBufCurrent_ += str;
        if(bAannotation_)
        {
            sBookAnntotation_ += str;
        }
    }
    if(!tag.isEmpty())
    {
        *pBufCurrent_ += u">"_s;
        if(bAannotation_)
        {
            sBookAnntotation_ += u">"_s;
        }
        if(tag == u"p")
            bInlineImageMode_ = true;
    }

    if(!elem.isElement())
    {
        if(!(bHeader_ || bSubHeader_))
        {
            need_page_break = true;
        }
        QString hstring;
        QString elem_text = save_html(elem.nodeValue());
        if(pExportOptions_->nHyphenate>0 && !(bHeader_ || bSubHeader_))
        {

            QStringList sl = elem_text.split(u" "_s);
            for(const QString &str: std::as_const(sl))
            {
                QString hyp = hyphenator.hyphenate_word(str, (pExportOptions_->nHyphenate==1 ?SOFT_HYPHEN :CHILD_HYPHEN), pExportOptions_->nHyphenate == 1);
                hstring += u" "_s + hyp;
            }
            hstring.remove(0, 1);
        }
        else
        {
            hstring = elem_text;
        }

        if(dodropcaps)
        {
            int len = 1;
            if(len < hstring.length())
            {
                while(!hstring[len-1].isLetter())
                {
                    if(len > 1)
                    {
                        if(hstring[len-2].isDigit() && hstring[len-1] == ' ')
                            break;
                    }
                    if(hstring.mid(len-1).startsWith(u"&quot;", Qt::CaseInsensitive))
                        len += 6;
                    else
                        len++;
                    if(len >= hstring.length())
                        break;
                }
            }
            else
                len = hstring.length();
            len = hstring.left(len).trimmed().length();
            *pBufCurrent_ += u"<span class=\"dropcaps\">%1</span>"_s.arg(hstring.left(len)) + hstring.right(hstring.length() - len);
            dodropcaps = false;
        }
        else
        {
            if(hstring.length() > 2)
            {
                if((hstring[0] == '-' || hstring[0] == u'—' || hstring[0] == u'—') && hstring[1] == ' ')
                {
                    hstring = hstring[0] % u"&#8197;"_s % hstring.right(hstring.length() - 2);
                }
            }
            *pBufCurrent_ += hstring;
        }
        if(bAannotation_)
        {
            sBookAnntotation_ += hstring;
        }
    }
    for(int e=0; e<elem.childNodes().count(); e++)
    {
        if(elem.childNodes().at(e).toElement().tagName() == u"title")
            parse_title(elem.childNodes().at(e));
        else if(elem.childNodes().at(e).toElement().tagName() == u"subtitle")
            parse_subtitle(elem.childNodes().at(e));
        else if(elem.childNodes().at(e).toElement().tagName() == u"epigraph")
            parse_epigraph(elem.childNodes().at(e));
        else if(elem.childNodes().at(e).toElement().tagName() == u"annotation")
            parse_annotation(elem.childNodes().at(e));
        else if(elem.childNodes().at(e).toElement().tagName() == u"section")
            parse_section(elem.childNodes().at(e));
        else if(elem.childNodes().at(e).toElement().tagName() == u"strong")
            parse_strong(elem.childNodes().at(e));
        else if(elem.childNodes().at(e).toElement().tagName() == u"emphasis")
            parse_emphasis(elem.childNodes().at(e));
        else if(elem.childNodes().at(e).toElement().tagName() == u"strikethrough")
            parse_strikethrough(elem.childNodes().at(e));
        else if(elem.childNodes().at(e).toElement().tagName() == u"style")
            parse_style(elem.childNodes().at(e));
        else if(elem.childNodes().at(e).toElement().tagName() == u"a")
           parse_a(elem.childNodes().at(e));
        else if(elem.childNodes().at(e).toElement().tagName() == u"image")
            parse_image(elem.childNodes().at(e));
        else if(elem.childNodes().at(e).toElement().tagName() == u"p")
            parse_p(elem.childNodes().at(e));
        else if(elem.childNodes().at(e).toElement().tagName() == u"poem")
            parse_poem(elem.childNodes().at(e));
        else if(elem.childNodes().at(e).toElement().tagName() == u"stanza")
            parse_stanza(elem.childNodes().at(e));
        else if(elem.childNodes().at(e).toElement().tagName() == u"v")
            parse_v(elem.childNodes().at(e));
        else if(elem.childNodes().at(e).toElement().tagName() == u"cite")
            parse_cite(elem.childNodes().at(e));
        else if(elem.childNodes().at(e).toElement().tagName() == u"empty-line")
            parse_emptyline(elem.childNodes().at(e));
        else if(elem.childNodes().at(e).toElement().tagName() == u"text-author")
            parse_textauthor(elem.childNodes().at(e));
        else if(elem.childNodes().at(e).toElement().tagName() == u"table")
            parse_table(elem.childNodes().at(e));
        else if(elem.childNodes().at(e).toElement().tagName() == u"code")
            parse_code(elem.childNodes().at(e));
        else if(elem.childNodes().at(e).toElement().tagName() == u"date")
            parse_date(elem.childNodes().at(e));
        else if(elem.childNodes().at(e).toElement().tagName() == u"tr")
            parse_table_element(elem.childNodes().at(e));
        else if(elem.childNodes().at(e).toElement().tagName() == u"td")
            parse_table_element(elem.childNodes().at(e));
        else if(elem.childNodes().at(e).toElement().tagName() == u"th")
            parse_table_element(elem.childNodes().at(e));
        else
            parse_other(elem.childNodes().at(e));
    }

    if(!tag.isEmpty())
    {
        if(elem.toElement().tagName() != u"section")
            *pBufCurrent_ += u"</%1>"_s.arg(tag);
        if(bAannotation_)
        {
            sBookAnntotation_ += u"</%1>"_s.arg(tag);
        }
        if(tag == u"p")
            bInlineImageMode_ = false;
        if(vCurrentNotes_.size() >0 )
        {
            if(pExportOptions_->nFootNotes == 1) //inline
            {
                *pBufCurrent_ += u"<span class=\"inlinenote\">%1</span>"_s.arg(vCurrentNotes_.at(0).at(1));
                vCurrentNotes_.clear();
            }
            else if(pExportOptions_->nFootNotes == 2 && tag == u"p")
            {
                *pBufCurrent_ += u"<div class=\"blocknote\">"_s;
                for(const auto &note: vCurrentNotes_)
                {
                    if(!note[1].isEmpty())
                        *pBufCurrent_ += u"<p><span class=\"notenum\">%1</span>&#160;%2</p>"_s.arg(note[0], note[1]);
                }
                *pBufCurrent_ += u"</div>"_s;
                vCurrentNotes_.clear();
            }
        }
    }
}

void fb2mobi::generate_toc()
{
    buf = sHtmlHead_ %
          u"<div class=\"toc\">"_s %
          u"<div class=\"h1\" id=\"toc\">%1</div>"_s.arg(toctitle);
    if(!sBufAnnotation_.isEmpty() && !pExportOptions_->bAnnotation)
    {
        buf += u"<div class=\"indent0\"><a href=\"%1\">%2</a></div>"_s.arg(u"annotation.html"_s, sAnnotationTitle_);
    }
    for(const auto &item: toc)
    {
        if(item.level <= toc_max_level)
        {
            if(item.body_name.isEmpty())
            {
                int indent = (item.level <=6 ?item.level :6);
                if(indent == 0)
                {
                    QStringList lines = item.title.split(u"\n"_s);
                    buf += u"<div class=\"indent0\"><a href=\"%1\">"_s.arg(item.href);
                    for(const QString &line: std::as_const(lines))
                    {
                        if(!line.trimmed().isEmpty())
                            buf += save_html(line).trimmed() + u"<br/>"_s;
                    }
                    buf += u"</a></div>"_s;
                }
                else
                    buf += u"<div class=\"indent%1\"><a href=\"%2\">%3</a></div>"_s.arg(QString::number(indent), item.href, save_html(item.title));
            }
            else
                buf += u"<div class=\"indent0\"><a href=\"%1\">%2</a></div>"_s.arg(item.href, save_html(item.title));
        }
    }


    buf += u"</div>"
            "<div class=\"indent0\"><a href=\"%1\">%2</a></div>"_s.arg(u"annotation.html"_s, sNotesTitle_);
    buf += sHtmlFoot_;
}


void fb2mobi::generate_ncx()
{
    buf = u"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                         "<ncx>"
                         "<head></head>\n"
                         "<docTitle>"
                            "<text>freeLib</text>"
                         "</docTitle>"
                         "<navMap>"_s;
    int i = 1;
    if(!sBufAnnotation_.isEmpty() && !pExportOptions_->bAnnotation)
    {
        buf += u"<navPoint id=\"annotation\" playOrder=\"%1\">"_s.arg(i) +
               u"<navLabel><text>%1</text></navLabel>"_s.arg(sAnnotationTitle_) +
               u"<content src=\"annotation.html\" />"
                              "</navPoint>"_s;
        i++;
    }
    if(pExportOptions_->nContentPlacement == 1)
    {
        // Включим содержание в навигацию джойстиком
        buf += u"<navPoint id=\"navpoint%1\" playOrder=\"%1\">"_s.arg(i) %
               u"<navLabel><text>%1</text></navLabel>"_s.arg(toctitle) %
               u"<content src=\"toc.html\" />"
                              "</navPoint>"_s;
        i++;
    }
    int current_level = -1;
    std::vector<STOC> tmp_toc = toc;
    if(pExportOptions_->bMlToc)
    {
        //считаем количество заголовков каждого уровня
        std::unordered_map<int, int> LevelsCount;
        for(const STOC &item: tmp_toc)
        {
            if(LevelsCount.contains(item.level))
                LevelsCount[item.level]++;
            else
                LevelsCount[item.level] = 1;
        }

        //находим maxLevel уровней с максимальным количеством заголовков
        std::vector<int> vLevels;
        for(int i=0; i<pExportOptions_->nMaxCaptionLevel; i++)
        {
            int curMaxCount = 0;
            int maxKey = 0;
            for(const auto &ref :LevelsCount)
            {
                if(ref.second>curMaxCount)
                {
                    curMaxCount = ref.second;
                    maxKey = ref.first;
                }
            }
            if(curMaxCount > 0)
            {
                LevelsCount[maxKey] = 0;
                vLevels.push_back(maxKey);
            }
        }
        std::sort(vLevels.begin(), vLevels.end());
        for(int i=vLevels.size()-1; i>=0; i--)
        {
            for(auto &iTmpToc :tmp_toc)
            {
               if(iTmpToc.level >= vLevels[i])
                {
                    iTmpToc.level=-i;
                }
            }
        }
        for(auto &iTmpToc :tmp_toc)
        {
            if(iTmpToc.level > 0)
                iTmpToc.level = 0;
            else
                iTmpToc.level = -iTmpToc.level;
        }

    }
    for(const STOC &item: tmp_toc)
    {
        if(pExportOptions_->bMlToc)
        {
            while(current_level > item.level)
            {
                buf += u"</navPoint>"_s;
                current_level--;
            }
            if(current_level == item.level)
                buf += u"</navPoint>\n"_s;
        }
        buf += u"<navPoint id=\"navpoint%1\" playOrder=\"%1\">\n"_s.arg(i) %
               u"<navLabel><text>%1</text></navLabel>\n"_s.arg(save_html(item.title)) %
               u"<content src=\"%1\" /\n>"_s.arg(item.href);
        if(!pExportOptions_->bMlToc)
            buf += u"</navPoint>\n"_s;
        current_level = item.level;
        i++;
    }
    while(current_level >= 0 && pExportOptions_->bMlToc)
    {
        buf += u"</navPoint>\n"_s;
        current_level--;
    }


    if(pExportOptions_->nContentPlacement == 2)
    {
        // Включим содержание в навигацию джойстиком
        if( i > 1)
        {
            buf += u"<navPoint id=\"navpoint%1\" playOrder=\"%1\">\n"_s.arg(i) %
                   u"<navLabel><text>%1</text></navLabel>\n"_s.arg(toctitle) %
                   u"<content src=\"toc.html\" /\n>"
                                  "</navPoint\n>"_s;
            i++;
        }
    }

    buf += u"</navMap></ncx>"_s;
}

void fb2mobi::generate_ncx_epub()
{
    buf = u"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                         "<!DOCTYPE ncx PUBLIC \"-//NISO//DTD ncx 2005-1//EN\" \"http://www.daisy.org/z3986/2005/ncx-2005-1.dtd\">\n"
                         "<ncx xmlns=\"http://www.daisy.org/z3986/2005/ncx/\" version=\"2005-1\">\n"
                         "<head>\n"
                            "<meta name=\"dtb:uid\" content=\"123456\"/>\n"
                            "<meta name=\"dtb:depth\" content=\"1\"/>\n"
                            "<meta name=\"dtb:totalPageCount\" content=\"0\"/>\n"
                            "<meta name=\"dtb:maxPageNumber\" content=\"0\"/>\n"
                         "</head>\n"
                         "<docTitle>\n"
                            "<text>FreeLib</text>\n"
                         "</docTitle>\n"
                         "<navMap>"_s;
    int i = 1;
    if(!sBufAnnotation_.isEmpty() && !pExportOptions_->bAnnotation)
    {
        buf += u"<navPoint id=\"annotation\" playOrder=\"%1\">\n"_s.arg(i) +
               u"<navLabel><text>%1</text></navLabel>\n"_s.arg(sAnnotationTitle_) +
               u"<content src=\"annotation.html\"/>\n>"
                              "</navPoint>\n"_s;
        i++;
    }
    // Включим содержание в навигацию джойстиком
    if(pExportOptions_->nContentPlacement == 1)
    {
        buf += u"<navPoint id=\"navpoint%1\" playOrder=\"%1\">\n"_s.arg(i) +
               u"<navLabel><text>Содержание</text></navLabel>\n"
                                "<content src=\"toc.html\" /\n>"
                              "</navPoint>\n"_s;
        i++;
    }
    int current_level = -1;
    for(const STOC &item: toc)
    {
        if(pExportOptions_->bMlToc)
        {
            while(current_level>item.level)
            {
                buf += u"</navPoint>"_s;
                current_level--;
            }
            if(current_level == item.level)
                buf += u"</navPoint>\n"_s;
        }
        buf += u"<navPoint id=\"navpoint%1\" playOrder=\"%1\">"_s.arg(i) +
               u"<navLabel><text>%1</text></navLabel>"_s.arg(save_html(item.title)) +
               u"<content src=\"%1\" />"_s.arg(item.href);
        if(!pExportOptions_->bMlToc)
            buf += u"</navPoint>\n"_s;
        current_level = item.level;
        i++;
    }
    while(current_level >= 0 && pExportOptions_->bMlToc)
    {
        buf += u"</navPoint>"_s;
        current_level--;
    }

    // Включим содержание в навигацию джойстиком
    if(pExportOptions_->nContentPlacement == 2)
    {
        buf += u"<navPoint id=\"navpoint%1\" playOrder=\"%1\">"_s.arg(i) +
               u"<navLabel><text>Содержание</text></navLabel>"
                                "<content src=\"toc.html\" />"
                              "</navPoint>"_s;
        i++;
    }

    buf += u"</navMap></ncx>"_s;
}

void fb2mobi::generate_mime()
{
    buf = u"application/epub+zip"_s;
}

void fb2mobi::generate_container()
{
    buf = u"<?xml version=\"1.0\"?>\n"
            "<container version=\"1.0\" xmlns=\"urn:oasis:names:tc:opendocument:xmlns:container\">\n"
              "<rootfiles>\n"
                "<rootfile full-path=\"OEBPS/book.opf\" "
                 "media-type=\"application/oebps-package+xml\" />\n"
              "</rootfiles>\n"
            "</container>\n"_s;
}

QString MIME_TYPE(QString type)
{
    type = type.toLower();
    if(type == u"jpg")
        return u"image/jpeg"_s;
    if(type == u"ttf")
    {
        return u"application/octet-stream"_s;
        //return "application/x-font-ttf";
    }
    return u"image/"_s + type;
}

void fb2mobi::generate_opf_epub()
{
    buf = u"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"_s;
    buf += u"<package xmlns=\"http://www.idpf.org/2007/opf\" xmlns:dc=\"http://purl.org/dc/elements/1.1/\" unique-identifier=\"bookid\" version=\"2.0\">\n"
                          "<metadata xmlns:opf=\"http://www.idpf.org/2007/opf\">\n"_s;
    if(pBook_->mSequences.empty())
        buf += u"<dc:title>%1</dc:title>\n"_s.arg(sBookName_);
    else
    {

        QString title = bookseriestitle;
        title = g::libs[idLib_].fillParams(title, idBook_);
        if(pExportOptions_->bSeriaTranslit)
            title=Transliteration(title);
        buf += u"<dc:title>%1</dc:title>\n"_s.arg(title);
     }
    buf += u"<dc:language>%1</dc:language>\n"_s.arg(test_language(g::libs[idLib_].vLaguages[pBook_->idLanguage]));
    buf += u"<dc:identifier id=\"bookid\">%1</dc:identifier>\n"_s.arg(isbn.isEmpty() ?QUuid::createUuid().toString() :isbn);
    buf += u"<dc:creator opf:role=\"aut\">%1</dc:creator>\n"_s.arg(book_author);

    if(!pBook_->vIdGenres.empty())
        buf += u"<dc:subject>%1</dc:subject>\n"_s.arg(g::genres[pBook_->vIdGenres.at(0)].sName);
    if(!sBookAnntotation_.isEmpty()){
        static const QRegularExpression re(QStringLiteral("<[^>]*>"));
        buf += u"<dc:description>%1</dc:description>\n"_s.arg(sBookAnntotation_.replace(re, u""_s).trimmed());
    }
    if(!sFileCover_.isEmpty())
        buf += u"<meta name=\"cover\" content=\"cover-image\" />\n"_s;
    buf += u"</metadata>\n"
            "<manifest>\n"
            "<item id=\"ncx\" media-type=\"application/x-dtbncx+xml\" href=\"toc.ncx\"/>\n"_s;
    if(!pExportOptions_->bAnnotation && !sBufAnnotation_.isEmpty())
        buf += u"<item id=\"annotation\" media-type=\"application/xhtml+xml\" href=\"annotation.html\"/>\n"_s;
    if(!sFileCover_.isEmpty()){
        buf += u"<item id=\"cover-image\" media-type=\"%2\" href=\"%1\"/>\n"_s.arg(sFileCover_, MIME_TYPE(QFileInfo(sFileCover_).suffix()));
        buf += u"<item id=\"cover-page\" media-type=\"application/xhtml+xml\" href=\"cover.xhtml\"/>\n"_s;
    }
    QString spine_files;
    int i = 0;
    for (const auto &html: vHtmlFiles_)
    {
        buf += u"<item id=\"content%2\" media-type=\"application/xhtml+xml\" href=\"%1\"/>\n"_s.arg(html.sFileName, QString::number(i));
        //if(!str.file_name.startsWith("footnotes",Qt::CaseInsensitive))
        {
            spine_files += u"<itemref idref=\"content%2\"/>\n"_s.arg(i);
        }
        i++;
    }
    if(pExportOptions_->nContentPlacement != 0)
    {
        buf += u"<item id=\"toc\" media-type=\"application/xhtml+xml\" href=\"toc.html\"/>\n"
               "<item id=\"css\" href=\"style.css\" media-type=\"text/css\"/>\n"_s;
    }

    QFileInfoList fonts = QDir(sTmpDir_ + u"/OEBPS/fonts"_s).entryInfoList(QDir::NoDotAndDotDot | QDir::Files);
    for(const QFileInfo &font: std::as_const(fonts))
    {
        buf += u"<item id=\"%1\" media-type=\"%2\" href=\"fonts/%1\"/>\n"_s.arg(font.fileName(), MIME_TYPE(font.suffix()));
    }
    QFileInfoList pics = QDir(sTmpDir_ + u"/OEBPS/pic"_s).entryInfoList(QDir::NoDotAndDotDot | QDir::Files);
    for(const QFileInfo &pic: std::as_const(pics))
    {
        buf += u"<item id=\"%1\" media-type=\"%2\" href=\"pic/%1\"/>\n"_s.arg(pic.fileName(), MIME_TYPE(pic.suffix()));
    }
    int img_count = 0;
    for(const QString &str: vImageList_)
    {
        if(str == sFileCover_)
            continue;
        buf += u"<item id=\"img_id_%1\" href=\"%2\" media-type=\"%3\"/>\n"_s.arg(QString::number(img_count), str, MIME_TYPE(QFileInfo(str).suffix()));
        img_count++;
    }

    buf += u"</manifest>\n"
            "<spine toc=\"ncx\">\n"_s %
            (sFileCover_.isEmpty() ?u""_s :u"<itemref idref=\"cover-page\"/>\n"_s) %
            (pExportOptions_->nContentPlacement == 1 ?u"<itemref idref=\"toc\"/>\n"_s :u""_s) %
            ((sBufAnnotation_.isEmpty() || pExportOptions_->bAnnotation) ?u""_s :u"<itemref idref=\"annotation\"/>\n"_s) %
            spine_files %
            (pExportOptions_->nContentPlacement == 2 ?u"<itemref idref=\"toc\"/>\n"_s :u""_s) %
            u"</spine>\n"
             "</package>\n"_s;
}

void fb2mobi::generate_opf()
{
    Q_CHECK_PTR(pExportOptions_);
    SLib& lib = g::libs[idLib_];
    buf = u"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"_s;
    buf += u"<package><metadata><dc-metadata xmlns:dc=\"http://\">"_s;
    if(pBook_->mSequences.empty())
        buf += u"<dc:Title>%1</dc:Title>"_s.arg(sBookName_);
    else
    {
        QString title = bookseriestitle;
        title = lib. fillParams(title, idBook_);
        if(pExportOptions_->bSeriaTranslit)
            title = Transliteration(title);

        buf += u"<dc:Title>%1</dc:Title>"_s.arg(title);
     }
    buf += u"<dc:Language>%1</dc:Language>"_s.arg(test_language(lib.vLaguages[pBook_->idLanguage]));
    buf += u"<dc:Creator>%1</dc:Creator>"_s.arg(book_author);
    buf += u"<dc:identifier id=\"BookId\" opf:scheme=\"ISBN\">%1</dc:identifier>"_s.arg(isbn.isEmpty() ?QUuid::createUuid().toString() :isbn);
    buf += u"<dc:Publisher /><dc:date /><x-metadata>"_s;
    if(!sFileCover_.isEmpty())
        buf += u"<EmbeddedCover>%1</EmbeddedCover>"_s.arg(sFileCover_);
    buf += u"</x-metadata></dc-metadata></metadata>"
            "<manifest>"
            "<item id=\"ncx\" media-type=\"application/x-dtbncx+xml\" href=\"toc.ncx\"/>"_s +
            ((sBufAnnotation_.isEmpty() || pExportOptions_->bAnnotation) ?u""_s :u"<item id=\"annotation\" media-type=\"text/x-oeb1-document\" href=\"annotation.html\"/>"_s);

    int i = 0;
    QString spine_files;
    for(const auto &html: vHtmlFiles_)
    {
        buf += u"<item id=\"text%2\" media-type=\"text/x-oeb1-document\" href=\"%1\"/>"_s.arg(html.sFileName, QString::number(i));
        spine_files += u"<itemref idref=\"text%1\"/>"_s.arg(i);
        i++;
    }

    buf += (pExportOptions_->nContentPlacement != 0 ?u"<item id=\"content\" media-type=\"text/x-oeb1-document\" href=\"toc.html\"/>"_s :u""_s) %
                    u"</manifest>"
                     "<spine toc=\"ncx\">"_s %
                     (pExportOptions_->nContentPlacement == 1 ?u"<itemref idref=\"content\"/>"_s :u""_s) %
                     ((sBufAnnotation_.isEmpty() || pExportOptions_->bAnnotation) ?u""_s :u"<itemref idref=\"annotation\"/>"_s) %
                     spine_files %
                     (pExportOptions_->nContentPlacement == 2 ?u"<itemref idref=\"content\"/>"_s :u""_s) %
                     QStringLiteral("</spine>"
                     "<guide>") %
                     ((sBufAnnotation_.isEmpty() || pExportOptions_->bAnnotation) ?u""_s :u"<reference type=\"other.intro\" title=\"Annotation\" href=\"annotation.html\"/>"_s) %
                     (pExportOptions_->nContentPlacement == 1 ?u"<itemref idref=\"content\"/>"_s :u""_s) %
                     u"<reference type=\"text\" title=\"Book\" href=\"%1\"/>"_s.arg(vHtmlFiles_.front().sFileName) %
                     (pExportOptions_->nContentPlacement == 2 ?u"<itemref idref=\"content\"/>"_s :u""_s) %
                     u"</guide>"
                     "</package>"_s;
}

void fb2mobi::generate_html(QFile *file)
{
    vHtmlFiles_.emplace_back(QFileInfo(file->fileName()).completeBaseName() + u".html"_s);
    pBufCurrent_ = &vHtmlFiles_.back().sContent;

    *pBufCurrent_ = sHtmlHead_;
    doc.setContent(file);
    QDomNode root = doc.documentElement();
    href_pref = u"href"_s;
    get_notes_dict(u"notes"_s);

    first_body = true;
    for(int i=0; i<root.childNodes().count();i++)
    {
        if(root.childNodes().at(i).toElement().tagName() == u"description" && (!join_seria || current_book == 1))
            parse_description(root.childNodes().at(i));
        else if(root.childNodes().at(i).toElement().tagName() == u"body" && (first_body || !root.childNodes().at(i).toElement().attributes().contains(QStringLiteral("name"))))
        {
            parse_body(root.childNodes().at(i));
        }
        else if(root.childNodes().at(i).toElement().tagName() == u"binary")
            parse_binary(root.childNodes().at(i));
    }
    *pBufCurrent_ += sHtmlFoot_;
    std::erase_if(vHtmlFiles_, [](auto &html){return html.sContent == (sHtmlHead_ + sHtmlFoot_);});

    if((pExportOptions_->nFootNotes == 0 || pExportOptions_->nFootNotes == 3) && vNotesDict.size() > 0)
    {
        int index = 0;
        QString tmp = u"footnotes%1.html"_s;
        bool loop = true;
        while(loop)
        {
            loop = false;
            for(auto &html :vHtmlFiles_)
            {
                if(html.sFileName == tmp.arg(QString::number(index)))
                {
                    index++;
                    loop = true;
                    break;
                }
            }
        }
        vHtmlFiles_.emplace_back(tmp.arg(QString::number(index)));
        STOC c_toc = {u"%1#%2"_s.arg(vHtmlFiles_.back().sFileName, u"fn%1"_s.arg(toc_index)), tr("Footnotes"), 1, body_name, ""};
        toc.push_back(std::move(c_toc));
        QString* str = &vHtmlFiles_.back().sContent;
        *str += sHtmlHead_;
        *str += u"<a name='fn%1'></a>"_s.arg(QString::number(toc_index));
        toc_index++;
        for(int i=0; i<vNotesDict.size(); i++)
        {
            QString id = vNotesDict.at(i).first;
            auto notes = vNotesDict.at(i).second;
            refFiles_[id] = vHtmlFiles_.back().sFileName + (id.left(1) == u"#" ?u""_s :u"#"_s) + id;
            QString href;
            for(const auto &ref :crossingRef_)
            {
                if(((ref.second.to.left(1)==u"#" ?u""_s :u"#"_s) + ref.second.to) == ((id.left(1)==u"#" ?u""_s :u"#"_s) + id))
                {
                    href = ref.second.from;
                    break;
                }
            }
            QString title = notes[0].isEmpty() ?u"^"_s :notes[0];
            if(pExportOptions_->nFootNotes == 3)
            {
                if(outputFormat_ == epub)
                    *str += u"<div epub:type=\"footnote\" id=\"%1\">"_s.arg(id);
                else
                    *str += u"<div id=\"%1\"><div class=\"titlenotes\"><a href=\"%2\">[%3] </a></div>"_s.arg(id,href,title);
            }
            else
            {
                if(outputFormat_ == epub)
                    *str += u"<div class=\"titlenotes\" id=\"%1\"><a href=\"%2\">[%3] </a></div>"_s.arg(id, href, title);
                else
                    *str += u"<div class=\"titlenotes\" id=\"%1\">%2</div>"_s.arg(id, title);
            }
            *str += notes[1];
            if(pExportOptions_->nFootNotes == 3)
                *str += u"</div>"_s;
            *str += u"<div style=\"page-break-before:always;\"></div>"_s;
            if(str->length() > 10000)
            {
                *str += sHtmlFoot_;
                //i++;
                vHtmlFiles_.emplace_back(tmp.arg(QString::number(i+1)));
                str = &vHtmlFiles_.back().sContent;
                *str += sHtmlHead_;
            }
        }

        *str += sHtmlFoot_;
    }

    for(const auto &ref :refFiles_){
        for(int i=0; i<vHtmlFiles_.size(); i++)
        {
            vHtmlFiles_[i].sContent.replace(u"href=\""_s % ref.first % u"\""_s, u"href=\""_s % ref.second % u"\""_s);
            vHtmlFiles_[i].sContent.replace(u"href=\"#"_s % ref.first % u"\""_s, u"href=\""_s % ref.second % u"\""_s);
        }
    }
}

QString fb2mobi::GenerateAZW3(const QString &file)
{
    Q_CHECK_PTR(pExportOptions_);
    mobiEdit me(file);
    QString azw3File = file.left(file.length()-4) + u"azw3"_s;
    if(!me.SaveAZW(azw3File, pExportOptions_->bRemovePersonal, pExportOptions_->bRepairCover))
        return file;
    QFile().remove(file);
    return azw3File;
}

QString fb2mobi::GenerateMOBI7(const QString &file)
{
    Q_CHECK_PTR(pExportOptions_);
    mobiEdit me(file);
    QString mobi7File = file.left(file.length()-4) + u"mobi"_s;
    if(!me.SaveMOBI7(mobi7File, pExportOptions_->bRemovePersonal, pExportOptions_->bRepairCover))
        return file;
    return mobi7File;
}

void PaintText(QPainter* painter, const QRect &rect, int to, const QString &text, QRect *br=nullptr)
{
    QFont font(painter->font());
    QRect bound;
    do
    {
        font.setPixelSize(font.pixelSize()-1);
        painter->setFont(font);
        bound=painter->boundingRect(rect,to,text);
    }while((bound.width()>rect.width() || bound.height()>rect.height()) && font.pixelSize()>5);
    painter->drawText(rect, to, text, br);
}

void fb2mobi::InsertSeriaNumberToCover(const QString &number, CreateCover create_cover)
{
    if(sFileCover_.isEmpty() && cc_no)
        return;
    bool create_new = false;
    if((sFileCover_.isEmpty() && create_cover==cc_if_not_exists) || create_cover==cc_always)
    {
        QImage img(u":/xsl/img/cover.jpg"_s);
        if(sFileCover_.isEmpty())
        {
            sFileCover_ = u"cover.jpg"_s;
            vImageList_.push_back(sFileCover_);
        }
        else
        {
            QFile::remove(sTmpDir_ % u"/OEBPS/"_s % sFileCover_);
        }
        img.save(sTmpDir_ % u"/OEBPS/"_s % sFileCover_);
        create_new=true;
    }

    QImage img(sTmpDir_ % u"/OEBPS/"_s % sFileCover_);
    img = img.convertToFormat(QImage::Format_RGB32);

    QPainter* painter = new QPainter(&img);
    QFont font;
    if(!create_new)
    {
        if(!number.isEmpty())
        {
            painter->setBackgroundMode(Qt::OpaqueMode);
            painter->setBackground(QBrush(QColor(255, 255, 255, 168)));
            font.setPixelSize(img.height() / 15);
            painter->setFont(font);
            PaintText(painter, img.rect(), Qt::AlignTop|Qt::AlignRight, number);
        }
    }
    else
    {
        int delta = img.rect().height() / 50;
        int delta2 = img.rect().height() / 100;
        int r_width = img.rect().width()-delta * 2;
        int r_heigth = img.rect().height()-delta * 2;
        int r_heigthTopBottom = r_heigth / 4;

        font.setPixelSize(img.height() / 15);
        painter->setFont(font);
        PaintText(painter, QRect(delta, delta, r_width, r_heigthTopBottom-delta2), Qt::AlignHCenter|Qt::AlignTop|Qt::TextWordWrap, g::libs[idLib_].authors[pBook_->idFirstAuthor].getName());

        font.setPixelSize(img.height() / 12);
        font.setBold(true);
        painter->setFont(font);
        PaintText(painter, QRect(delta, delta+r_heigthTopBottom+delta2, r_width, r_heigth-r_heigthTopBottom*2-delta2*2), Qt::AlignHCenter|Qt::AlignVCenter|Qt::TextWordWrap, sBookName_);

        if(!pBook_->mSequences.empty()){
            font.setBold(false);
            font.setPixelSize(img.height() / 17);
            painter->setFont(font);
            auto sequence = pBook_->mSequences.begin();
            QString sSequence = g::libs[idLib_].serials[sequence->first].sName;
            uint numInSquence = sequence->second;
            PaintText(painter, QRect(delta, delta+r_heigth-r_heigthTopBottom+delta2, r_width, r_heigthTopBottom-delta2), Qt::AlignHCenter|Qt::AlignBottom|Qt::TextWordWrap,
                      (sSequence % (numInSquence>0 ?u"\n"_s % QString::number(numInSquence) :u""_s)));
        }
    }
    img.save(sTmpDir_ % u"/OEBPS/"_s % sFileCover_);
    delete painter;
}

void recurseAddDir(const QDir &d, std::vector<QString> &list)
{
    QStringList qsl = d.entryList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files);
    for(const QString &file: std::as_const(qsl))
    {
        QFileInfo finfo(u"%1/%2"_s.arg(d.path(), file));
        if (finfo.isSymLink())
            return;
        if (finfo.isDir())
        {
            QDir sd(finfo.filePath());
            recurseAddDir(sd, list);
        }
        else
            list.emplace_back(QDir::toNativeSeparators(finfo.filePath()));
    }
}

void ZipDir(QuaZip *zip, const QDir &dir)
{
    QFile inFile;
    std::vector<QString> sl;
    recurseAddDir(dir, sl);
    std::vector<QFileInfo> files;
    for(const QString &fn: sl)
        files.emplace_back(QFileInfo(fn));
    QuaZipFile outFile(zip);
    char c;
    for(const QFileInfo &fileInfo: files)
    {
        if (!fileInfo.isFile())
            continue;
        // Если файл в поддиректории, то добавляем имя этой поддиректории к именам файлов
        // например: fileInfo.filePath() = "D:\Work\Sources\SAGO\svn\sago\Release\tmp_DOCSWIN\Folder\123.opn"
        // тогда после удаления части строки fileNameWithSubFolders будет равен "Folder\123.opn" и т.д.
        QString fileNameWithRelativePath = fileInfo.filePath().remove(0, QFileInfo(dir.absolutePath()).absolutePath().length() + 1);
        inFile.setFileName(fileInfo.filePath());
        if (!inFile.open(QIODevice::ReadOnly))
        {
            qDebug() << u"testCreate(): inFile.open(): %1"_s.arg(inFile.errorString().toLocal8Bit().constData());
            return;
        }
        if (!outFile.open(QIODevice::WriteOnly, QuaZipNewInfo(fileNameWithRelativePath, fileInfo.filePath())))
        {
            qDebug() << u"testCreate(): outFile.open(): %1"_s.arg(outFile.getZipError());
            return;
        }
        while (inFile.getChar(&c) && outFile.putChar(c));
        if (outFile.getZipError() != UNZ_OK)
        {
            qDebug() << u"testCreate(): outFile.putChar(): %1"_s.arg(outFile.getZipError());
            return;
        }
        outFile.close();
        if (outFile.getZipError() != UNZ_OK)
        {
            qDebug() << u"testCreate(): outFile.close(): %1"_s.arg(outFile.getZipError());
            return;
        }
        inFile.close();
    }
}

QString fb2mobi::convert(uint idBook)
{
    idBook_ = idBook;
    QDir dir(sTmpDir_ + u"/OEBPS"_s);
    dir.removeRecursively();
    dir.mkpath(sTmpDir_ + u"/OEBPS"_s);

    QBuffer outbuff;
    outputFormat_ = epub;
    auto &book = g::libs[idLib_].books[idBook];
    if(book.sFormat != u"fb2")
        return u""_s;
    pBook_ = &book;

    QFile file;
    QString out_file = sTmpDir_ + u"/book.fb2"_s;
    QFile::remove(out_file);
    file.setFileName(out_file);
    file.open(QFile::WriteOnly);
    file.write(outbuff.data());
    file.close();


    fb2file.setFile(sTmpDir_ + u"/book.fb2"_s);
    QFile f(sTmpDir_ + u"/book.fb2"_s);
    if(!f.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        LogWarning << "Error open fb2 file:" << f.fileName();
        return u""_s;
    }
    join_seria = false;
    current_book = 1;
    generate_html(&f);
    f.close();

    QString title = pExportOptions_->sCoverLabel;
    if(title.isEmpty())
        title = ExportOptions::sDefaultCoverLabel;
    if(sFileCover_.isEmpty())
        InsertSeriaNumberToCover(title, cc_if_not_exists);
    if(!sFileCover_.endsWith(u".jpg", Qt::CaseInsensitive))
    {
        QImage img(sTmpDir_ % u"/OEBPS/"_s % sFileCover_);
        sFileCover_ = sFileCover_.left(sFileCover_.length() - QFileInfo(sFileCover_).suffix().length()) + u"jpg"_s;
        img.save(sTmpDir_ % u"/OEBPS/"_s % sFileCover_);
    }
    return sTmpDir_ % u"/OEBPS/"_s % sFileCover_;
}

struct group_colors
{
    QList<QColor> list;
};

QColor GetColor(const QImage &img)
{
    int h = img.height();
    int w = img.width();
    int step = 10;
    int error = 32;
    std::unordered_map<QRgb, std::vector<QColor> > colors;
    for(int i=0; i<h; i+=step)
    {
        for(int j=0; j<w; j+=step)
        {
            QColor color = img.pixel(j,i);
            color.setBlue(color.blue()/error);
            color.setGreen(color.green() / error);
            color.setRed(color.red()/error);
            colors[color.rgb()].emplace_back(img.pixel(j,i));
        }
    }
    std::vector<QColor> result;
    std::vector<QColor> result_prev;
    int max = 0;
    for(const auto &it :colors)
    {
       if(max<it.second.size())
       {
           max = it.second.size();
           result_prev = std::move(result);
           result = std::move(it.second);
       }
    }
    qlonglong r = 0, g = 0, b = 0;
    for(const QColor &color: result)
    {
        r += color.red();
        g += color.green();
        b += color.blue();
    }
    if(result_prev.size() > 0)
    {
        qlonglong pr = 0, pg = 0, pb = 0;
        for(const QColor &color: result_prev)
        {
            pr += color.red();
            pg += color.green();
            pb += color.blue();
        }
        if((pr + pg + pb) / result_prev.size() > (r + g + b) /result.size())
            return QColor(pr/result_prev.size(), pg/result_prev.size(), pb/result_prev.size());
        else
            return QColor(r/result.size(), g/result.size(), b/result.size());
    }
    return QColor(r/result.size(), g/result.size(), b/result.size());
}

struct FontFamily
{
    int font;
    int font_b;
    int font_i;
    int font_bi;
    std::unordered_map<QString, QString> tags;
    FontFamily()
    {
        font = -1;
        font_b = -1;
        font_i = -1;
        font_bi = -1;
    }

    bool operator ==(const FontFamily &a) const
    {
        return font == a.font && font_b == a.font_b && font_i == a.font_i && font_bi == a.font_bi;
    }
};

QString fb2mobi::convert(const std::vector<QString> &files, uint idBook)
{
    pBook_ = &g::libs[idLib_].books[idBook];
    idBook_ = idBook;
    outputFormat_ = pExportOptions_->format;
    if(files.size() == 1)
    {
        fb2file.setFile(files.at(0));
        QString out_file;
        if(fb2file.suffix().toLower() == u"epub" &&
            (outputFormat_ == mobi || outputFormat_ == azw3))
        {
            QString sKindlegen = QApplication::applicationDirPath() + QStringLiteral("/kindlegen");
            if(!QFile::exists(sKindlegen))
                sKindlegen = u"kindlegen"_s;
            QProcess::execute(sKindlegen, {sTmpDir_ % u"/"_s % fb2file.fileName()});
            out_file = sTmpDir_ % u"/"_s % fb2file.completeBaseName() % u".mobi"_s;
            if(pExportOptions_->format == azw3)
            {
                out_file = GenerateAZW3(out_file);
            }
            return out_file;
        }
    }
    QString out_file;
    QDir dir(sTmpDir_ + u"/OEBPS"_s);
    dir.removeRecursively();
    dir.mkpath(sTmpDir_ + u"/OEBPS/css"_s);

    QString sFileCss = sTmpDir_ + u"/OEBPS/style.css"_s;
    if(pExportOptions_->bUserCSS && !pExportOptions_->sUserCSS.isEmpty())
    {
        QFile file(sFileCss);
        if(file.open(QFile::WriteOnly))
        {
            file.write(pExportOptions_->sUserCSS.toUtf8());
            file.close();
        }
    }else{
        QFile::copy(u":/xsl/css/style.css"_s, sFileCss);
        QFile::setPermissions(sFileCss, QFileDevice::WriteOwner | QFileDevice::ReadOwner);
    }

    QString sAppDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    dir.mkpath(sTmpDir_ + u"/OEBPS/pic"_s);
    if(pExportOptions_->nVignette > 0)
    {
        const static QStringList imgs{u"te.png"_s, u"h0tb.png"_s, u"h0ta.png"_s, u"h1tb.png"_s, u"h1ta.png"_s};
        QString sSrcPrefix = sAppDir + u"/xsl/img/"_s;
        const QString sDestPrefix = sTmpDir_ + u"/OEBPS/pic/"_s;
        for(const auto &img :std::as_const(imgs)){
            QString sImagePath = sSrcPrefix + img;
            if( !QFile::exists(sImagePath) )
                sImagePath = u":/xsl/img/"_s + img;
            QString sDestImagePath = sDestPrefix + img;
            QFile::copy(sImagePath, sDestImagePath);
        }
    }

    QString sFontsPath = sAppDir + u"/fonts/"_s;

    QFile css(sFileCss);
    if(!css.open(QFile::Append)){
        LogWarning << css.errorString();
    }
    int count = pExportOptions_->vFontExportOptions.size();
    QStringList fonts;
    std::vector<FontFamily> fonts_set;
    for(int i=0; i<count; i++)
    {
        const FontExportOptions &fontExportOptions = pExportOptions_->vFontExportOptions.at(i);
        if(fontExportOptions.bUse)
        {
            dir.setPath(sTmpDir_ + u"/OEBPS"_s);
            dir.mkpath(sTmpDir_ + u"/OEBPS/fonts"_s);
            FontFamily set;
            for(int j=0; j<4; j++)
            {
                QString sFileFont = j==0 ?fontExportOptions.sFont :j==1 ?fontExportOptions.sFontB :j==2 ?fontExportOptions.sFontI :j==3 ?fontExportOptions.sFontBI :u""_s;
                int index = fonts.indexOf(sFileFont);
                if(index<0 && !sFileFont.isEmpty())
                {
                    fonts.append(sFileFont);
                    index = fonts.count()-1;
                    if(QFile::exists(QApplication::applicationDirPath() % u"/xsl/fonts/"_s % sFileFont))

                    {
                        QFile::copy(QApplication::applicationDirPath() % u"/xsl/fonts/"_s % sFileFont, sTmpDir_ + u"/OEBPS/fonts/font%1.ttf"_s.arg(index));
                    }
                    else
                    {
                        if(QFile::exists(sFontsPath + sFileFont))
                        {
                            QFile::copy(sFontsPath + sFileFont, sTmpDir_ + u"/OEBPS/fonts/font%1.ttf"_s.arg(index));
                        }
                        else if(QFile::exists(FREELIB_DATA_DIR % u"/fonts"_s % sFileFont))
                        {
                            QFile::copy(FREELIB_DATA_DIR % u"/fonts"_s % sFileFont, sTmpDir_ + u"/OEBPS/fonts/font%1.ttf"_s.arg(index));
                        }
                        else
                        {
                            QFileInfo fi(sFileFont);
                            QFile::copy(fi.absoluteFilePath(), sTmpDir_  + u"/OEBPS/fonts/font%1.ttf"_s.arg(index));
                        }
                    }
                }
                if(j == 0)
                    set.font = index;
                else if(j == 1)
                    set.font_b = index;
                else if(j == 2)
                    set.font_i = index;
                else if(j == 3)
                    set.font_bi = index;
            }
            quint8 tag_id = fontExportOptions.nTag;
            QString sFontSize = QString::number(fontExportOptions.nFontSize);
            set.tags[g::vTags[tag_id].css] = sFontSize;
            bool find = false;
            for (int j=0; j<fonts_set.size(); j++)
            {
                if(fonts_set[j] == set)
                {
                    find = true;
                    fonts_set[j].tags[g::vTags[tag_id].css] = sFontSize;
                    break;
                }
            }
            if(!find)
            {
                for(int j=0; j<4; j++)
                {
                    if(j==0 && set.font==-1)
                        continue;
                    if(j==1 && set.font_b==-1)
                        continue;
                    if(j==2 && set.font_i==-1)
                        continue;
                    if(j==3 && set.font_bi==-1)
                        continue;
                    css.write(u"\n@font-face {\n"
                               "    font-family: \"font%4\";\n"
                               "    src: url(\"fonts/font%1.ttf\");\n"
                               "%2"
                               "%3"
                               "}\n"_s.arg(
                               (QString::number(j==0?set.font:j==1?set.font_b:j==2?set.font_i:j==3?set.font_bi:0)),
                              (j==1||j==3)?u"    font-weight: bold;\n"_s :u""_s,
                              (j==2||j==3)?u"    font-style: italic;\n"_s :u""_s,
                              QString::number(fonts_set.size())).toUtf8());
                }
                fonts_set.push_back(std::move(set));
            }
        }
    }
    int font_index = 0;
    for(const auto &set_i: fonts_set)
    {
        for(const auto &tag :set_i.tags)
        {
            css.write(u"\n%2 {\n"
                "    font-family: \"font%1\";\n"
                "    font-size: %3%;\n"
                "}\n"_s.arg(font_index).arg(tag.first, tag.second).toUtf8());
        }
        font_index++;
    }

    css.close();
    join_seria = files.size() > 1;
    int i = 0;
    for(const QString &file: files)
    {
        fb2file.setFile(file);
        if(fb2file.suffix().toLower() != u"fb2")
            return u""_s;
        QFile f(sTmpDir_ + u"/book%1.fb2"_s.arg(i));
        if(!f.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            LogWarning << "Error open fb2 file:" << f.fileName();
            return u""_s;
        }
        current_book = i + 1;
        generate_html(&f);
        f.close();
        i++;

    }
    QFile f;
    QTextStream ts(&f);
    for(const auto &html: vHtmlFiles_)
    {
        QString html_file = sTmpDir_ + u"/OEBPS/%1"_s.arg(html.sFileName);
        f.setFileName(html_file);
        f.open(QIODevice::WriteOnly);
        ts << html.sContent;
        f.close();
    }
    int height{0}, width{0};
    if(!sFileCover_.isEmpty()){
        QImage img(sTmpDir_ % u"/OEBPS/"_s % sFileCover_);
        img = img.convertToFormat(QImage::Format_RGB32);
        height = img.height();
        width = img.width();
        if(pExportOptions_->bRepairCover)
        {
            if(width < 625 || height < 625 || (height < 1000 && width < 1000))
            {
                if(height < 1100)
                {
                    width = width * 1100 / height;
                    height = 1100;
                }
                if(width < 700)
                {
                    height = height * 700 / width;
                    width = 700;
                }
                img = img.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                img.save(sTmpDir_ % u"/OEBPS/"_s % sFileCover_);
            }
            double Aspect = double(width) / double(height);
            if(Aspect <= 0.62 || Aspect >= 0.63)
            {
                int newWidth, newHeight;
                if(Aspect <= 0.62)
                {
                    newWidth = height * 0.6252;
                    newHeight = height;
                }
                else
                {
                    newWidth = width;
                    newHeight = width/0.6252;
                }
                QImage img_new(newWidth, newHeight, QImage::Format_RGB32);
                QPainter painter(&img_new);
                if(Aspect < 0.62)
                {
                    painter.fillRect(0, 0, newWidth/2, newHeight, QBrush(GetColor(img.copy(0, 0, width/5, height))));
                    painter.fillRect(newWidth/2, 0, newWidth/2, newHeight, QBrush(GetColor(img.copy(width - width/5, 0, width/5, height))));
                }
                else
                {
                    painter.fillRect(0, 0, newWidth, newHeight/2, QBrush(GetColor(img.copy(0, 0, width, height/5))));
                    painter.fillRect(0, newHeight/2, newWidth, newHeight/2, QBrush(GetColor(img.copy(0, height - height/5, width, height/5))));
                }
                painter.drawImage((newWidth - width)/2, (newHeight - height)/2, img);
                img_new.save(sTmpDir_ % u"/OEBPS/"_s % sFileCover_);
                width = newWidth;
                height = newHeight;
            }
        }
    }
    if(pExportOptions_->bAddCoverLabel || pExportOptions_->bCreateCover)
    {
        QString title = pExportOptions_->sCoverLabel;
        if(title.isEmpty())
            title = ExportOptions::sDefaultCoverLabel;
        title = g::libs[idLib_].fillParams(title, idBook_);
        if(pBook_->mSequences.empty() || pBook_->mSequences.begin()->second == 0 || !pExportOptions_->bAddCoverLabel)
            title = u""_s;
        InsertSeriaNumberToCover(title, (pExportOptions_->bCreateCoverAlways ?cc_always :(pExportOptions_->bCreateCover ?cc_if_not_exists :cc_no)));
        if(height == 0){
            QImage img(sTmpDir_ % u"/OEBPS/"_s % sFileCover_);
            height = img.height();
            width = img.width();
        }
    }

    if(!sFileCover_.isEmpty() && pExportOptions_->format == epub){
        QString sWidth = QString::number(width);
        QString sHeight = QString::number(height);

        QString sCoverHtml =
            u"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
            "<head>\n"
            "<title/>\n"
            "</head>\n"
            "<body style=\"margin: 0px; padding: 0px; oeb-column-number: 1;\">\n"
            "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" style=\"height: 100%; width: 100%;\" viewBox=\"0 0 "_s % sWidth % u" "_s % sHeight % u"\">\n"
            "<image width=\""_s % sWidth % u"\" height=\""_s % sHeight % u"\" xlink:href=\""_s % sFileCover_ % u"\" xmlns:xlink=\"http://www.w3.org/1999/xlink\"/>\n"
            "</svg>\n"
            "</body>\n"
            "</html>\n"_s;
        QFile fileCoverHtml(sTmpDir_ + u"/OEBPS/cover.xhtml"_s);
        if(fileCoverHtml.open(QIODevice::WriteOnly)){
            fileCoverHtml.write(sCoverHtml.toUtf8());
            fileCoverHtml.close();
        }
    }

    generate_toc();
    if(pExportOptions_->nContentPlacement != 0)
    {
        QString toc_file = sTmpDir_ + u"/OEBPS/toc.html"_s;
        f.setFileName(toc_file);
        f.open(QIODevice::WriteOnly);
        ts << buf;
        f.close();
    }

    if(!sBufAnnotation_.isEmpty() && !pExportOptions_->bAnnotation)
    {
        QString annotation_file = sTmpDir_ + u"/OEBPS/annotation.html"_s;
        f.setFileName(annotation_file);
        f.open(QIODevice::WriteOnly);
        ts << sBufAnnotation_;
        f.close();
    }

    if(pExportOptions_->format == mobi || pExportOptions_->format == azw3 || pExportOptions_->format == mobi7)
        generate_ncx();
    else
        generate_ncx_epub();
    QString ncx_file = sTmpDir_ + u"/OEBPS/toc.ncx"_s;
    f.setFileName(ncx_file);
    f.open(QIODevice::WriteOnly);
    ts << buf;
    f.close();

    if(pExportOptions_->format == mobi || pExportOptions_->format == azw3 || pExportOptions_->format == mobi7)
        generate_opf();
    else
        generate_opf_epub();
    QString opf_file = sTmpDir_ + u"/OEBPS/book.opf"_s;
    f.setFileName(opf_file);
    f.open(QIODevice::WriteOnly);
    ts << buf;
    f.close();

    if(pExportOptions_->format == mobi || pExportOptions_->format == azw3 || pExportOptions_->format == mobi7)
    {
        QString sKindlegen = QApplication::applicationDirPath() + u"/kindlegen"_s;
        if(!QFile::exists(sKindlegen))
            sKindlegen = QStringLiteral("kindlegen");
        QProcess::execute(sKindlegen, {opf_file});
        out_file = sTmpDir_ + u"/OEBPS/book.mobi"_s;
    }
    switch(pExportOptions_->format){
    case azw3:
        out_file = GenerateAZW3(out_file);
        break;
    case mobi7:
        out_file = GenerateMOBI7(out_file);
        break;
    case epub:
    {
        if(QDir(sTmpDir_ + u"/OEBPS/pic"_s).entryList(QStringList(), QDir::Files).count() == 0)
        {
            QDir().rmpath(sTmpDir_ + u"/OEBPS/pic"_s);
        }
        QDir().mkpath(sTmpDir_ + u"/META-INF"_s);
        generate_mime();
        QString mime_file = sTmpDir_ + u"/mimetype"_s;;
        f.setFileName(mime_file);
        f.open(QIODevice::WriteOnly);
        ts << buf;
        f.close();
        generate_container();
        QString container_file = sTmpDir_ + u"/META-INF/container.xml"_s;
        f.setFileName(container_file);
        f.open(QIODevice::WriteOnly);
        ts << buf;
        f.close();

        QuaZip zip(sTmpDir_ + u"/book.epub"_s);
        zip.open(QuaZip::mdCreate);
        QuaZipFile zip_file(&zip);
        zip_file.open(QIODevice::WriteOnly, QuaZipNewInfo(u"mimetype"_s), 0, 0, 0, 0);
        QFile file(mime_file);
        file.open(QIODevice::ReadOnly);
        zip_file.write(file.readAll());
        zip_file.close();
        ZipDir(&zip, QDir(sTmpDir_ + u"/OEBPS"_s));
        ZipDir(&zip, QDir(sTmpDir_ + u"/META-INF"_s));
        zip.close();

        out_file = sTmpDir_ + u"/book.epub"_s;

    }
        break;
    case pdf:
    {

//        QWebEnginePage* pdf = new QWebEnginePage();
//         QEventLoop loop;
//        connect(pdf,&QWebEnginePage::loadFinished,&loop,&QEventLoop::quit);
//        qDebug()<<QUrl::fromLocalFile(tmp_dir + QLatin1String("/OEBPS/") + html_files[0].file_name);
//        pdf->load(QUrl::fromLocalFile(tmp_dir + QLatin1String("/OEBPS/") + html_files[0].file_name));
//        loop.exec();
//        //qDebug()<<pdf->mainFrame()->toHtml();
//        //QPrinter printer;
//        //printer.setPageSize(QPrinter::A4);
//        out_file = tmp_dir + QLatin1String("/book.pdf");
//        QFile::remove(out_file);
//        //printer.setOutputFileName(out_file);
//        //printer.setOutputFormat(QPrinter::NativeFormat);
//        //pdf->print(&printer);
//        pdf->printToPdf(out_file);
//        qDebug()<<out_file<<html_files[0].file_name;
//        delete pdf;
    }
    default:
        break;
    }
    return out_file;
}

