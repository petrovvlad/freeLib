#define QT_USE_QSTRINGBUILDER
#include "fb2mobi.h"

#include <QProcess>
#include <QUuid>
#include <QPainter>
#include <QDir>
#include <QDebug>
#include <QRegularExpression>

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
    header = false;
    inline_image_mode = false;
    current_header_level = 0;
    current_section_level = 0;
    first_header_in_body = false;
    nodropcaps = QStringLiteral("'\"-.…0123456789‒–—");
    toc_index = 0;
    toctitle = tr("Contents");
    no_paragraph = false;
    dodropcaps = false;
    first_chapter_line = false;
    subheader = false;
    annotation = false;
    toc_max_level = 1000000;
    if(QStandardPaths::standardLocations(QStandardPaths::TempLocation).count() > 0)
        tmp_dir = QStandardPaths::standardLocations(QStandardPaths::TempLocation).at(0);
    tmp_dir += QStringLiteral("/freeLib");
    QDir dir(tmp_dir);
    dir.mkpath(dir.path());
    annotation_title = tr("Abstract");
    notes_title = tr("Notes");

    if(!pExportOptions_->sBookSeriesTitle.isEmpty())
        bookseriestitle = pExportOptions_->sBookSeriesTitle;
    else
        bookseriestitle = ExportOptions::sDefaultBookTitle;
    if(! pExportOptions_->sAuthorSring.isEmpty())
        authorstring = pExportOptions_->sAuthorSring;
    else
        authorstring = ExportOptions::sDefaultAuthorName;
    notes_bodies << QStringLiteral("notes") << QStringLiteral("comments");
    join_seria = false;
}

QString test_language(QString language)
{
    QStringList l;
    l<<QStringLiteral("af")<<QStringLiteral("sq")<<QStringLiteral("ar")<<QStringLiteral("hy")<<QStringLiteral("az")<<QStringLiteral("eu")<<QStringLiteral("be")<<QStringLiteral("bn")<<
       QStringLiteral("bg")<<QStringLiteral("ca")<<QStringLiteral("zh")<<QStringLiteral("hr")<<QStringLiteral("cs")<<QStringLiteral("da")<<QStringLiteral("nl")<<QStringLiteral("en")<<
       QStringLiteral("et")<<QStringLiteral("fo")<<QStringLiteral("fa")<<QStringLiteral("fi")<<QStringLiteral("fr")<<QStringLiteral("ka")<<QStringLiteral("de")<<QStringLiteral("gu")<<
       QStringLiteral("he")<<QStringLiteral("hi")<<QStringLiteral("hu")<<QStringLiteral("is")<<QStringLiteral("id")<<QStringLiteral("it")<<QStringLiteral("ja")<<QStringLiteral("kn")<<
       QStringLiteral("kk")<<QStringLiteral("x-kok")<<QStringLiteral("ko")<<QStringLiteral("lv")<<QStringLiteral("lt")<<QStringLiteral("mk")<<QStringLiteral("ms")<<QStringLiteral("ml")<<
       QStringLiteral("mt")<<QStringLiteral("mr")<<QStringLiteral("ne")<<QStringLiteral("no")<<QStringLiteral("or")<<QStringLiteral("pl")<<QStringLiteral("pt")<<QStringLiteral("pa")<<
       QStringLiteral("rm")<<QStringLiteral("ro")<<QStringLiteral("ru")<<QStringLiteral("sz")<<QStringLiteral("sa")<<QStringLiteral("sr")<<QStringLiteral("sk")<<QStringLiteral("sl")<<
       QStringLiteral("sb")<<QStringLiteral("es")<<QStringLiteral("sx")<<QStringLiteral("sw")<<QStringLiteral("sv")<<QStringLiteral("ta")<<QStringLiteral("tt")<<QStringLiteral("te")<<
       QStringLiteral("th")<<QStringLiteral("ts")<<QStringLiteral("tn")<<QStringLiteral("tr")<<QStringLiteral("uk")<<QStringLiteral("ur")<<QStringLiteral("uz")<<QStringLiteral("vi")<<
       QStringLiteral("xh")<<QStringLiteral("zu");
    if(l.contains(language.toLower()))
        return language;
    for(const auto &str: l)
    {
        if(language.startsWith(str))
            return str;
    }
    return QStringLiteral("en");
}

QString HTMLHEAD = QStringLiteral("<html xmlns=\"http://www.w3.org/1999/xhtml\" xmlns:epub=\"http://www.idpf.org/2007/ops\">"
            "<head>"
                "<title>freeLib</title>"
                "<link rel=\"stylesheet\" type=\"text/css\" href=\"css/main.css\"/>"
                "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"/>"
            "</head>"
            "<body>");


QString HTMLFOOT = QStringLiteral("</body>"
              "</html>");

void fb2mobi::parse_description(const QDomNode &elem)
{
    if(join_seria)
    {
        pBook->sName = pBook->idSerial==0 ?u""_s :libs[idLib_].serials[pBook->idSerial].sName;
        //pBook->idSerial = 0;
    }
    book_author = authorstring;
    book_author = libs[idLib_].fillParams(book_author, idBook_);
    if(pExportOptions_->bAuthorTranslit)
        book_author = Transliteration(book_author);
    isbn = pBook->sIsbn;
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
                         buf_current = &buf_annotation;
                         *buf_current = HTMLHEAD;
                         *buf_current += u"<div class=\"annotation\"><div class=\"h1\">%1</div>"_s.arg(annotation_title);
                         annotation = true;
                         parse_format(ti.childNodes().at(t), u"div"_s);
                         annotation = false;
                         *buf_current += u"</div>"_s;
                         *buf_current += HTMLFOOT;
                         buf_current=&html_files.last().content;
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
    hyphenator.init(test_language(libs[idLib_].vLaguages[pBook->idLanguage]));
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
        dir.mkpath(tmp_dir + u"/OEBPS/img%1/"_s.arg(current_book));
        QFile file(tmp_dir + u"/OEBPS/img%1/"_s.arg(current_book) + filename);
        file.open(QIODevice::WriteOnly);
        file.write(QByteArray::fromBase64((elem.toElement().text().toStdString().c_str())));
        file.close();
        image_list << (u"img%1/"_s.arg(current_book) + filename);
    }
}

void fb2mobi::parse_body(const QDomNode &elem)
{
    body_name = QStringLiteral("");
    if(!elem.attributes().namedItem(QStringLiteral("name")).isNull())
        body_name = elem.attributes().namedItem(QStringLiteral("name")).toAttr().value();
    current_header_level = 0;
    first_header_in_body = true;
    parse_format(elem);
    first_body = false;
}

void fb2mobi::parse_p(const QDomNode &elem)
{
    QString pcss;
    QString ptag = QStringLiteral("p");
    if(header)
        pcss = QStringLiteral("css");
    else
        pcss = QStringLiteral("text");
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
    parse_format(elem, QStringLiteral("div"), QStringLiteral("section"));
    if(body_name.isEmpty() && pExportOptions_->nVignette > 0 && need_end_chapter_vignette)
    {
        QString level = QStringLiteral("h%1").arg(current_header_level <= 6 ?current_header_level :6);
        QString vignet = get_vignette(level, VIGNETTE_CHAPTER_END);
        if(!vignet.isEmpty())
            *buf_current += vignet;
        need_end_chapter_vignette = false;
    }
    current_header_level--;
    if(!parsing_note)
    {
        while(current_section_level > 0)
        {
            *buf_current += QStringLiteral("</div>");
            current_section_level--;
        }
        if(pExportOptions_->bSplitFile )
        {

            html_files.last().content += HTMLFOOT;
            html_files << html_content(QStringLiteral("section%1.html").arg(html_files.count()));
            html_files.last().content=HTMLHEAD;
            if(!annotation)
                buf_current = &html_files.last().content;
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
        if(QFileInfo::exists(tmp_dir + QStringLiteral("/OEBPS/pic/") + level.toLower() + type))
            result = QStringLiteral("");
        if(QFileInfo::exists(tmp_dir + QStringLiteral("/OEBPS/pic/") + level.toLower() + type + QStringLiteral(".png")))
            result = level.toLower() + type + QStringLiteral(".png");
        if(QFileInfo::exists(tmp_dir + QStringLiteral("/OEBPS/pic/") + type + QStringLiteral(".png")))
            result = type + QStringLiteral(".png");
        if(!result.isEmpty())
        {
            if(type == VIGNETTE_CHAPTER_END)
            {
                result = QStringLiteral("<div class=\"vignette_chapter_end\"><img  alt=\"\" src=\"pic/%1\"/></div>").arg(result);
            }
            else if(type == VIGNETTE_TITLE_BEFORE)
            {
                result = QStringLiteral("<div class=\"vignette_title_before\"><img  alt=\"\" src=\"pic/%1\"/></div>").arg(result);
            }
            else if(type == VIGNETTE_TITLE_AFTER)
            {
                result = QStringLiteral("<div class=\"vignette_title_after\"><img  alt=\"\" src=\"pic/%1\"/></div>").arg(result);
            }
        }
    }
    else if(pExportOptions_->nVignette == 2)
    {
        QString file;
        QString txt_dir = QStringLiteral(":/xsl/img/");
        //проверить: похоже лишние строки
//        if(QFileInfo::exists(txt_dir + level.toLower() + type))
//            file = QLatin1String("");
        if(QFileInfo::exists(txt_dir + level.toLower() + type + QStringLiteral(".txt")))
            file = txt_dir + level.toLower() + type + QStringLiteral(".txt");
        if( QFileInfo::exists(txt_dir + type + QStringLiteral(".txt")) )
            file = txt_dir + type + QStringLiteral(".txt");
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
                result = QStringLiteral("<div class=\"vignette_chapter_end\">%1</div>").arg(result);
            }
            else if(type == VIGNETTE_TITLE_BEFORE)
            {
                result = QStringLiteral("<div class=\"vignette_title_before\">%1</div>").arg(result);
            }
            else if(type == VIGNETTE_TITLE_AFTER)
            {
                result = QStringLiteral("<div class=\"vignette_title_after\">%1</div>").arg(result);
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
                  QString* cur = buf_current;
                  buf_current = &sNoteText;
                  parse_format(elem);
                  buf_current = cur;

           }
           notes_dict.append(QPair<QString, QStringList>(id, QStringList() << sNoteTitle.replace(u"&nbsp;"_s, u" "_s).trimmed() << sNoteText.replace(u"&nbsp;"_s, u" "_s).trimmed()));
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
    notes_dict.clear();
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
    QString toc_ref_id = QStringLiteral("tocref%1").arg(toc_index);
    if(parsing_note)
    {
        return;
    }

    QString toc_title;
    QString str;
    QTextStream stream(&str);
    elem.save(stream, 4);
    static const QRegularExpression re1(QStringLiteral("</p>|<br>|<br/>"), QRegularExpression::CaseInsensitiveOption);
    str.replace(re1, QStringLiteral(" "));

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
        *buf_current += u"<div style=\"page-break-before:always;\"></div>"_s;
        need_page_break = false;
    }
    if(body_name.isEmpty() || first_header_in_body || first_body)
    {
        header = true;
        first_chapter_line = true;
        *buf_current += QStringLiteral("<div class=\"titleblock\" id=\"%1\">").arg(toc_ref_id);
        if((body_name.isEmpty() || first_body) && first_header_in_body)
        {
            if(pExportOptions_->nVignette > 0 && !toc_title.isEmpty())
            {
                QString vignet = get_vignette(QStringLiteral("h0"), VIGNETTE_TITLE_BEFORE);
                if(!vignet.isEmpty())
                    *buf_current += vignet;
            }
            parse_format(elem, QStringLiteral("div"), QStringLiteral("h0"));
            current_header_level = 0;
            if(pExportOptions_->nVignette > 0 && !toc_title.isEmpty())
            {
                QString vignet = get_vignette(QStringLiteral("h0"), VIGNETTE_TITLE_AFTER);
                if(!vignet.isEmpty())
                    *buf_current += vignet;
            }
        }
        else
        {
            QString level = QStringLiteral("h%1").arg(current_header_level <= 6 ?current_header_level :6);
            if(pExportOptions_->nVignette > 0 && !toc_title.isEmpty())
            {
                QString vignet = get_vignette(level, VIGNETTE_TITLE_BEFORE);
                if(!vignet.isEmpty())
                    *buf_current += vignet;
            }
            parse_format(elem, QStringLiteral("div"), level);
            if(pExportOptions_->nVignette > 0 && !toc_title.isEmpty())
            {
                QString vignet = get_vignette(level, VIGNETTE_TITLE_AFTER);
                if(!vignet.isEmpty())
                    *buf_current += vignet;
            }
        }
        if(!toc_title.isEmpty())
        {
            STOC c_toc = {QStringLiteral("%1#%2").arg(html_files.last().file_name, toc_ref_id), toc_title, current_header_level, body_name, QStringLiteral("")};
            toc.push_back(c_toc);
        }
    }
    else
    {
        *buf_current += QStringLiteral("<div class=\"titleblock\" id=\"%1\">").arg(toc_ref_id);
        parse_format(elem, QStringLiteral("div"));
    }

    *buf_current += QStringLiteral("</div>\n");

    toc_index++;
    first_header_in_body = false;
    header = false;
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
            image = image.replace(QStringLiteral("#"), QStringLiteral(""));
            if(!image.isEmpty())
            {
                QFileInfo fi(image);
                if(fi.suffix().isEmpty())
                    image += QStringLiteral(".jpg");
            }
        }
        else if(elem.attributes().item(i).toAttr().name() == QStringLiteral("id"))
            img_id = elem.attributes().item(i).toAttr().value();
    }
    if(inline_image_mode)
    {
        if(!img_id.isEmpty())
            *buf_current += QStringLiteral("<img id=\"%1\" class=\"inlineimage\" src=\"img%3/%2\" alt=\"%2\"/>").arg(img_id, image, QString::number(current_book));
        else
            *buf_current += QStringLiteral("<img class=\"inlineimage\" src=\"img%2/%1\" alt=\"%1\"/>").arg(image, QString::number(current_book));
    }
    else
    {
        if(!img_id.isEmpty())
            *buf_current += QStringLiteral("<div id=\"%1\" class=\"image\">").arg(img_id);
        else
            *buf_current += QStringLiteral("<div class=\"image\">");
        *buf_current += QStringLiteral("<img src=\"img%2/%1\" alt=\"%1\"/>").arg(image, QString::number(current_book));
        *buf_current += QStringLiteral("</div>");
    }
    parse_format(elem);
}

void fb2mobi::parse_emptyline(const QDomNode&)
{
    *buf_current += QStringLiteral("<br/>");
}

void fb2mobi::parse_epigraph(const QDomNode &elem)
{
    no_paragraph = true;
    parse_format(elem, QStringLiteral("div"), QStringLiteral("epigraph"));
    no_paragraph = false;
}

void fb2mobi::parse_annotation(const QDomNode &elem)
{
    no_paragraph = true;
    parse_format(elem, QStringLiteral("div"), QStringLiteral("annotation"));
    no_paragraph = false;
}

void fb2mobi::parse_a(const QDomNode &elem)
{
    for(int j=0; j<elem.attributes().count(); j++)
    {
        if(elem.attributes().item(j).nodeName().right(href_pref.length()) == href_pref)
        {
             parse_format(elem, QStringLiteral("a"), QStringLiteral("anchor"), elem.attributes().item(j).toAttr().value().toHtmlEscaped());
             break;
        }
    }
}

void fb2mobi::parse_emphasis(const QDomNode &elem)
{
    parse_span(QStringLiteral("emphasis"), elem);
}

void fb2mobi::parse_strong(const QDomNode &elem)
{
    parse_span(QStringLiteral("strong"), elem);
}

void fb2mobi::parse_strikethrough(const QDomNode &elem)
{
    parse_span(QStringLiteral("strike"), elem);
}

void fb2mobi::parse_span(const QString &span, const QDomNode &elem)
{
    parse_format(elem, QStringLiteral("span"), span);
}

void fb2mobi::parse_textauthor(const QDomNode &elem)
{
    no_paragraph = true;
    parse_format(elem, QStringLiteral("div"), QStringLiteral("text-author"));
    no_paragraph = false;
}

void fb2mobi::parse_v(const QDomNode &elem)
{
    parse_format(elem, QStringLiteral("p"));
}

void fb2mobi::parse_poem(const QDomNode &elem)
{
    no_paragraph = true;
    parse_format(elem, QStringLiteral("div"), QStringLiteral("poem"));
    no_paragraph = false;
}

void fb2mobi::parse_stanza(const QDomNode &elem)
{
    parse_format(elem, QStringLiteral("div"), QStringLiteral("stanza"));
}

void fb2mobi::parse_table(const QDomNode &elem)
{
    *buf_current += QStringLiteral("<table class=\"table\"");
    for(int i=0; i<elem.attributes().count(); i++)
    {
        *buf_current += QStringLiteral(" %1=\"%2\"").arg(elem.attributes().item(i).toAttr().name(), elem.attributes().item(i).toAttr().value());
    }
    *buf_current += QStringLiteral(">");
    parse_format(elem);
    *buf_current += QStringLiteral("</table>");
}

void fb2mobi::parse_table_element(const QDomNode &elem)
{
    *buf_current += "<" + elem.toElement().tagName();

    for(int i=0; i<elem.attributes().count(); i++)
    {
        *buf_current += QStringLiteral(" %1=\"%2\"").arg(elem.attributes().item(i).toAttr().name(), elem.attributes().item(i).toAttr().value());
    }

    *buf_current += QStringLiteral(">");
    parse_format(elem);
    *buf_current += QStringLiteral("</") + elem.toElement().tagName() + QStringLiteral(">");
}

void fb2mobi::parse_code(const QDomNode &elem)
{
    parse_format(elem, QStringLiteral("code"));
}

void fb2mobi::parse_date(const QDomNode &elem)
{
    parse_format(elem, QStringLiteral("time"));
}

void fb2mobi::parse_subtitle(const QDomNode &elem)
{
    if(parsing_note)
        return;
    subheader = true;
    parse_format(elem, QStringLiteral("p"), QStringLiteral("subtitle"));
    subheader = false;
}

void fb2mobi::parse_style(const QDomNode&)
{
    return;
}

void fb2mobi::parse_cite(const QDomNode &elem)
{
    parse_format(elem, QStringLiteral("div"), QStringLiteral("cite"));
}

QString fb2mobi::save_html(const QString &str)
{
    return QString(str).toHtmlEscaped();
}

void fb2mobi::parse_format(const QDomNode &elem, QString tag , QString css, QString href)
{
    QStringList note;
    //bool need_popup=false;
    if(!tag.isEmpty())
    {
        dodropcaps = false;
        if(pExportOptions_->bDropCaps && first_chapter_line && !(header || subheader) && body_name.isEmpty() && tag.toLower() == u"p")
        {
            if(!no_paragraph)
            {
                if(nodropcaps.indexOf(elem.toElement().text()) < 0)
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
            note.clear();
            for(int i=0; i<notes_dict.count(); i++)
            {
                if(notes_dict.at(i).first == note_id)
                    note = notes_dict.at(i).second;
            }
            if(note.count() > 0)
            {
                current_notes << note;
                tag = QStringLiteral("span");
                css = u"%1anchor"_s.arg(pExportOptions_->nFootNotes==1 ?u"inline"_s :u"block"_s);
                href = QStringLiteral("");
            }
        }

        QString str;
        str += u"<%1"_s.arg(tag);
        if(!css.isEmpty())
            str += u" class=\"%1\""_s.arg(css);
        if(pExportOptions_->nFootNotes==3 && tag.toLower() == u"a" && sOutputFormat_ == u"EPUB")
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
            str += u" id=\"%1\""_s.arg(id);
            ref_files[id] = html_files.last().file_name + (id.left(1)==u"#" ?u""_s :u"#"_s) + id;
            crossing_ref[id].from = html_files.last().file_name + (id.left(1)==u"#" ?u""_s :u"#"_s) + id;
            crossing_ref[id].to = href;
        }
        if(!href.isEmpty())
            str += u" href=\"%1\""_s.arg(href);
        *buf_current += str;
        if(annotation)
        {
            book_anntotation += str;
        }
    }
    if(!tag.isEmpty())
    {
        *buf_current += u">"_s;
        if(annotation)
        {
            book_anntotation += u">"_s;
        }
        if(tag == u"p")
            inline_image_mode = true;
    }

    if(!elem.isElement())
    {
        if(!(header || subheader))
        {
            need_page_break = true;
        }
        QString hstring;
        QString elem_text = save_html(elem.nodeValue());
        if(pExportOptions_->nHyphenate>0 && !(header || subheader))
        {

            QStringList sl = elem_text.split(u" "_s);
            for(const QString &str: std::as_const(sl))
            {
                QString hyp = hyphenator.hyphenate_word(str, (pExportOptions_->nHyphenate==1 ?SOFT_HYPHEN :CHILD_HYPHEN), pExportOptions_->nHyphenate == 1);
                hstring += QStringLiteral(" ") + hyp;
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
            *buf_current += u"<span class=\"dropcaps\">%1</span>"_s.arg(hstring.left(len)) + hstring.right(hstring.length() - len);
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
            *buf_current += hstring;
        }
        if(annotation)
        {
            book_anntotation += hstring;
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
            *buf_current += u"</%1>"_s.arg(tag);
        if(annotation)
        {
            book_anntotation += u"</%1>"_s.arg(tag);
        }
        if(tag == u"p")
            inline_image_mode = false;
        if(current_notes.count() >0 )
        {
            if(pExportOptions_->nFootNotes == 1) //inline
            {
                *buf_current += u"<span class=\"inlinenote\">%1</span>"_s.arg(current_notes.at(0).at(1));
                current_notes.clear();
            }
            else if(pExportOptions_->nFootNotes == 2 && tag == u"p")
            {
                *buf_current += u"<div class=\"blocknote\">"_s;
                for(const QStringList &note: current_notes)
                {
                    if(!note[1].isEmpty())
                        *buf_current += u"<p><span class=\"notenum\">%1</span>&#160;%2</p>"_s.arg(note[0], note[1]);
                }
                *buf_current += u"</div>"_s;
                current_notes.clear();
            }
        }
    }
}

void fb2mobi::generate_toc()
{
    buf = HTMLHEAD;
    buf += QStringLiteral("<div class=\"toc\">");
    buf += QStringLiteral("<div class=\"h1\" id=\"toc\">%1</div>").arg(toctitle);
    if(!buf_annotation.isEmpty() && !pExportOptions_->bAnnotation)
    {
        buf += QStringLiteral("<div class=\"indent0\"><a href=\"%1\">%2</a></div>").arg("annotation.html", annotation_title);
    }
    for(const STOC &item: toc)
    {
        if(item.level <= toc_max_level)
        {
            if(item.body_name.isEmpty())
            {
                int indent = (item.level <=6 ?item.level :6);
                if(indent == 0)
                {
                    QStringList lines = item.title.split(u"\n"_s);
                    buf += QStringLiteral("<div class=\"indent0\"><a href=\"%1\">").arg(item.href);
                    for(const QString &line: std::as_const(lines))
                    {
                        if(!line.trimmed().isEmpty())
                            buf += save_html(line).trimmed() + QStringLiteral("<br/>");
                    }
                    buf += QStringLiteral("</a></div>");
                }
                else
                    buf += QStringLiteral("<div class=\"indent%1\"><a href=\"%2\">%3</a></div>").arg(QString::number(indent), item.href, save_html(item.title));
            }
            else
                buf += QStringLiteral("<div class=\"indent0\"><a href=\"%1\">%2</a></div>").arg(item.href, save_html(item.title));
        }
    }


    buf += QStringLiteral("</div>"
                          "<div class=\"indent0\"><a href=\"%1\">%2</a></div>").arg(QStringLiteral("annotation.html"), notes_title);
    buf += HTMLFOOT;
}


void fb2mobi::generate_ncx()
{
    buf = u"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                         "<ncx>"
                         "<head></head>"
                         "<docTitle>"
                            "<text>freeLib</text>"
                         "</docTitle>"
                         "<navMap>"_s;
    int i = 1;
    if(!buf_annotation.isEmpty() && !pExportOptions_->bAnnotation)
    {
        buf += QStringLiteral("<navPoint id=\"annotation\" playOrder=\"%1\">").arg(i) +
               QStringLiteral(  "<navLabel><text>%1</text></navLabel>").arg(annotation_title) +
               QStringLiteral(   "<content src=\"annotation.html\" />"
                              "</navPoint>");
        i++;
    }
    if(pExportOptions_->nContentPlacement == 1)
    {
        // Включим содержание в навигацию джойстиком
        buf += QStringLiteral("<navPoint id=\"navpoint%1\" playOrder=\"%1\">").arg(i) +
               QStringLiteral(  "<navLabel><text>%1</text></navLabel>").arg(toctitle) +
               QStringLiteral(   "<content src=\"toc.html\" />"
                              "</navPoint>");
        i++;
    }
    int current_level = -1;
    std::vector<STOC> tmp_toc = toc;
    if(pExportOptions_->bMlToc)
    {
        //считаем количество заголовков каждого уровня
        QMap<int, int> LevelsCount;
        for(const STOC &item: tmp_toc)
        {
            if(LevelsCount.contains(item.level))
                LevelsCount[item.level]++;
            else
                LevelsCount.insert(item.level, 1);
        }

        //находим maxLevel уровней с максимальным количеством заголовков
        QList<int> levelsList;
        for(int i=0; i<pExportOptions_->nMaxCaptionLevel; i++)
        {
            int curMaxCount = 0;
            int maxKey = 0;
            QMapIterator<int, int> ref(LevelsCount);
            while(ref.hasNext())
            {
                ref.next();
                if(ref.value()>curMaxCount)
                {
                    curMaxCount = ref.value();
                    maxKey = ref.key();
                }
            }
            if(curMaxCount > 0)
            {
                LevelsCount[maxKey] = 0;
                levelsList << maxKey;
            }
        }
        std::sort(levelsList.begin(), levelsList.end());
        for(int i=levelsList.count()-1; i>=0; i--)
        {
            for(auto &iTmpToc :tmp_toc)
            {
               if(iTmpToc.level >= levelsList[i])
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
                buf += QStringLiteral("</navPoint>");
                current_level--;
            }
            if(current_level == item.level)
                buf += QStringLiteral("</navPoint>\n");
        }
        buf += QStringLiteral("<navPoint id=\"navpoint%1\" playOrder=\"%1\">").arg(i) +
               QStringLiteral("<navLabel><text>%1</text></navLabel>").arg(save_html(item.title)) +
               QStringLiteral("<content src=\"%1\" />").arg(item.href);
        if(!pExportOptions_->bMlToc)
            buf += QStringLiteral("</navPoint>\n");
        current_level = item.level;
        i++;
    }
    while(current_level >= 0 && pExportOptions_->bMlToc)
    {
        buf += QStringLiteral("</navPoint>");
        current_level--;
    }


    if(pExportOptions_->nContentPlacement == 2)
    {
        // Включим содержание в навигацию джойстиком
        if( i > 1)
        {
            buf += QStringLiteral("<navPoint id=\"navpoint%1\" playOrder=\"%1\">").arg(i) +
                   QStringLiteral("<navLabel><text>%1</text></navLabel>").arg(toctitle) +
                   QStringLiteral(  "<content src=\"toc.html\" />"
                                  "</navPoint>");
            i++;
        }
    }

    buf += QStringLiteral("</navMap></ncx>");
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
    if(!buf_annotation.isEmpty() && !pExportOptions_->bAnnotation)
    {
        buf += u"<navPoint id=\"annotation\" playOrder=\"%1\">\n"_s.arg(i) +
               u"<navLabel><text>%1</text></navLabel>\n"_s.arg(annotation_title) +
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
    buf = QStringLiteral("application/epub+zip");
}

void fb2mobi::generate_container()
{
    buf = QStringLiteral("<?xml version=\"1.0\"?>"
            "<container version=\"1.0\" xmlns=\"urn:oasis:names:tc:opendocument:xmlns:container\">"
              "<rootfiles>"
                "<rootfile full-path=\"OEBPS/book.opf\" "
                 "media-type=\"application/oebps-package+xml\" />"
              "</rootfiles>"
            "</container>");
}

QString MIME_TYPE(QString type)
{
    type=type.toLower();
    if(type == u"jpg")
        return QStringLiteral("image/jpeg");
    if(type == u"ttf")
    {
        return QStringLiteral("application/octet-stream");
        //return "application/x-font-ttf";
    }
    return QStringLiteral("image/") + type;
}

void fb2mobi::generate_opf_epub()
{
    buf = u"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"_s;
    buf += u"<package xmlns=\"http://www.idpf.org/2007/opf\" xmlns:dc=\"http://purl.org/dc/elements/1.1/\" unique-identifier=\"bookid\" version=\"2.0\">\n"
                          "<metadata xmlns:opf=\"http://www.idpf.org/2007/opf\">\n"_s;
    if(pBook->idSerial == 0)
        buf += u"<dc:title>%1</dc:title>\n"_s.arg(pBook->sName);
    else
    {

        QString title = bookseriestitle;
        title = libs[idLib_].fillParams(title, idBook_);
        if(pExportOptions_->bSeriaTranslit)
            title=Transliteration(title);
        buf += u"<dc:title>%1</dc:title>\n"_s.arg(title);
     }
    buf += u"<dc:language>%1</dc:language>\n"_s.arg(test_language(libs[idLib_].vLaguages[pBook->idLanguage]));
    buf += u"<dc:identifier id=\"bookid\">%1</dc:identifier>\n"_s.arg(isbn.isEmpty() ?QUuid::createUuid().toString() :isbn);
    buf += u"<dc:creator opf:role=\"aut\">%1</dc:creator>\n"_s.arg(book_author);

    if(!pBook->vIdGenres.empty())
        buf += u"<dc:subject>%1</dc:subject>\n"_s.arg(genres[pBook->vIdGenres.at(0)].sName);
    if(!book_anntotation.isEmpty()){
        static const QRegularExpression re(QStringLiteral("<[^>]*>"));
        buf += u"<dc:description>%1</dc:description>\n"_s.arg(book_anntotation.replace(re, u""_s).trimmed());
    }
    if(!sFileCover_.isEmpty())
        buf += u"<meta name=\"cover\" content=\"cover-image\" />\n"_s;
    buf += u"</metadata>\n"
            "<manifest>\n"
            "<item id=\"ncx\" media-type=\"application/x-dtbncx+xml\" href=\"toc.ncx\"/>\n"_s;
    if(!pExportOptions_->bAnnotation && !buf_annotation.isEmpty())
        buf += u"<item id=\"annotation\" media-type=\"application/xhtml+xml\" href=\"annotation.html\"/>\n"_s;
    if(!sFileCover_.isEmpty()){
        buf += u"<item id=\"cover-image\" media-type=\"%2\" href=\"%1\"/>\n"_s.arg(sFileCover_, MIME_TYPE(QFileInfo(sFileCover_).suffix()));
        buf += u"<item id=\"cover-page\" media-type=\"application/xhtml+xml\" href=\"cover.xhtml\"/>\n";
    }
    QString spine_files;
    int i = 0;
    for (const html_content &str: std::as_const(html_files))
    {
        buf += u"<item id=\"content%2\" media-type=\"application/xhtml+xml\" href=\"%1\"/>\n"_s.arg(str.file_name, QString::number(i));
        //if(!str.file_name.startsWith("footnotes",Qt::CaseInsensitive))
        {
            spine_files += u"<itemref idref=\"content%2\"/>\n"_s.arg(i);
        }
        i++;
    }
    if(pExportOptions_->nContentPlacement != 0)
    {
        buf += u"<item id=\"toc\" media-type=\"application/xhtml+xml\" href=\"toc.html\"/>\n"
                "<item id=\"css\" href=\"css/main.css\" media-type=\"text/css\"/>\n"_s;
    }

    QFileInfoList fonts = QDir(tmp_dir + u"/OEBPS/fonts"_s).entryInfoList(QDir::NoDotAndDotDot | QDir::Files);
    for(const QFileInfo &font: std::as_const(fonts))
    {
        buf += u"<item id=\"%1\" media-type=\"%2\" href=\"fonts/%1\"/>\n"_s.arg(font.fileName(), MIME_TYPE(font.suffix()));
    }
    QFileInfoList pics = QDir(tmp_dir + u"/OEBPS/pic"_s).entryInfoList(QDir::NoDotAndDotDot | QDir::Files);
    for(const QFileInfo &pic: std::as_const(pics))
    {
        buf += u"<item id=\"%1\" media-type=\"%2\" href=\"pic/%1\"/\n>"_s.arg(pic.fileName(), MIME_TYPE(pic.suffix()));
    }
    int img_count = 0;
    for(const QString &str: std::as_const(image_list))
    {
        if(str == sFileCover_)
            continue;
        buf += u"<item id=\"img_id_%1\" href=\"%2\" media-type=\"%3\"/\n>"_s.arg(QString::number(img_count), str, MIME_TYPE(QFileInfo(str).suffix()));
        img_count++;
    }

    buf += u"</manifest>\n"
            "<spine toc=\"ncx\">\n"_s %
            (sFileCover_.isEmpty() ?u""_s :u"<itemref idref=\"cover-page\"/>\n"_s) %
            (pExportOptions_->nContentPlacement == 1 ?u"<itemref idref=\"toc\"/>\n"_s :u""_s) %
            ((buf_annotation.isEmpty() || pExportOptions_->bAnnotation) ?u""_s :u"<itemref idref=\"annotation\"/>\n"_s) %
            spine_files %
            (pExportOptions_->nContentPlacement == 2 ?u"<itemref idref=\"toc\"/>\n"_s :u""_s) %
            u"</spine>\n"
             "</package>\n"_s;
}

void fb2mobi::generate_opf()
{
    Q_CHECK_PTR(pExportOptions_);
    SLib& lib = libs[idLib_];
    buf = u"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"_s;
    buf += u"<package><metadata><dc-metadata xmlns:dc=\"http://\">"_s;
    if(pBook->idSerial == 0)
        buf += u"<dc:Title>%1</dc:Title>"_s.arg(pBook->sName);
    else
    {
        QString title = bookseriestitle;
        title = lib. fillParams(title, idBook_);
        if(pExportOptions_->bSeriaTranslit)
            title = Transliteration(title);

        buf += QStringLiteral("<dc:Title>%1</dc:Title>").arg(title);
     }
    buf += u"<dc:Language>%1</dc:Language>"_s.arg(test_language(libs[idLib_].vLaguages[pBook->idLanguage]));
    buf += u"<dc:Creator>%1</dc:Creator>"_s.arg(book_author);
    buf += u"<dc:identifier id=\"BookId\" opf:scheme=\"ISBN\">%1</dc:identifier>"_s.arg(isbn.isEmpty() ?QUuid::createUuid().toString() :isbn);
    buf += u"<dc:Publisher /><dc:date /><x-metadata>"_s;
    if(!sFileCover_.isEmpty())
        buf += u"<EmbeddedCover>%1</EmbeddedCover>"_s.arg(sFileCover_);
    buf += u"</x-metadata></dc-metadata></metadata>"
            "<manifest>"
            "<item id=\"ncx\" media-type=\"application/x-dtbncx+xml\" href=\"toc.ncx\"/>"_s +
            ((buf_annotation.isEmpty() || pExportOptions_->bAnnotation) ?u""_s :u"<item id=\"annotation\" media-type=\"text/x-oeb1-document\" href=\"annotation.html\"/>"_s);

    int i = 0;
    QString spine_files;
    for(const html_content &str: std::as_const(html_files))
    {
        buf += u"<item id=\"text%2\" media-type=\"text/x-oeb1-document\" href=\"%1\"/>"_s.arg(str.file_name, QString::number(i));
        spine_files += u"<itemref idref=\"text%1\"/>"_s.arg(i);
        i++;
    }

    buf += (pExportOptions_->nContentPlacement != 0 ?u"<item id=\"content\" media-type=\"text/x-oeb1-document\" href=\"toc.html\"/>"_s :u""_s) %
                    u"</manifest>"
                     "<spine toc=\"ncx\">"_s %
                     (pExportOptions_->nContentPlacement == 1 ?u"<itemref idref=\"content\"/>"_s :u""_s) %
                     ((buf_annotation.isEmpty() || pExportOptions_->bAnnotation) ?u""_s :u"<itemref idref=\"annotation\"/>"_s) %
                     spine_files %
                     (pExportOptions_->nContentPlacement == 2 ?u"<itemref idref=\"content\"/>"_s :u""_s) %
                     QStringLiteral("</spine>"
                     "<guide>") %
                     ((buf_annotation.isEmpty() || pExportOptions_->bAnnotation) ?u""_s :u"<reference type=\"other.intro\" title=\"Annotation\" href=\"annotation.html\"/>"_s) %
                     (pExportOptions_->nContentPlacement == 1 ?u"<itemref idref=\"content\"/>"_s :u""_s) %
                     u"<reference type=\"text\" title=\"Book\" href=\"%1\"/>"_s.arg(html_files.first().file_name) %
                     (pExportOptions_->nContentPlacement == 2 ?u"<itemref idref=\"content\"/>"_s :u""_s) %
                     u"</guide>"
                     "</package>"_s;
}

void fb2mobi::generate_html(QFile *file)
{
    html_files << html_content(QFileInfo(file->fileName()).completeBaseName() + u".html"_s);
    buf_current = &html_files.last().content;

    *buf_current = HTMLHEAD;
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
    *buf_current += HTMLFOOT;
    for(int i=0; i<html_files.count(); i++)
    {
        if(html_files.at(i).content == (HTMLHEAD + HTMLFOOT))
        {
            html_files.removeAt(i);
            i--;
        }
    }

    if((pExportOptions_->nFootNotes == 0 || pExportOptions_->nFootNotes == 3) && notes_dict.count() > 0)
    {
        int index=0;
        QString tmp = QStringLiteral("footnotes%1.html");
        bool loop = true;
        while(loop)
        {
            loop = false;
            for(int i=0; i<html_files.count(); i++)
            {
                if(html_files.at(i).file_name == tmp.arg(QString::number(index)))
                {
                    index++;
                    loop = true;
                    break;
                }
            }
        }
        html_files << html_content(tmp.arg(QString::number(index)));
        STOC c_toc = {QStringLiteral("%1#%2").arg(html_files.last().file_name, QStringLiteral("fn%1").arg(toc_index)), tr("Footnotes"), 1, body_name, ""};
        toc.push_back(c_toc);
        QString* str = &html_files.last().content;
        *str += HTMLHEAD;
        *str += QStringLiteral("<a name='fn%1'></a>").arg(QString::number(toc_index));
        toc_index++;
        for(int i=0; i<notes_dict.count(); i++)
        {
            QString id = notes_dict.at(i).first;
            ref_files[id] = html_files.last().file_name + (id.left(1) == u"#" ?u""_s :u"#"_s) + id;
            QMapIterator<QString, cross_ref> ref(crossing_ref);
            QString href;
            while(ref.hasNext())
            {
                ref.next();
                if(((ref.value().to.left(1)==u"#" ?QStringLiteral("") :QStringLiteral("#")) + ref.value().to) == ((id.left(1)==u"#" ?QStringLiteral("") :QStringLiteral("#")) + id))
                {
                    href = ref.value().from;
                    break;
                }
            }
            QString title = notes_dict.at(i).second[0].isEmpty() ?QStringLiteral("^") :notes_dict.at(i).second[0];
            if(pExportOptions_->nFootNotes == 3)
            {
                if(sOutputFormat_ == u"EPUB")
                    *str += QStringLiteral("<div epub:type=\"footnote\" id=\"%1\">").arg(id);
                else
                    *str += QStringLiteral("<div id=\"%1\"><div class=\"titlenotes\"><a href=\"%2\">[%3] </a></div>").arg(id,href,title);
            }
            else
            {
                if(sOutputFormat_ == u"EPUB")
                    *str += QStringLiteral("<div class=\"titlenotes\" id=\"%1\"><a href=\"%2\">[%3] </a></div>").arg(id,href,title);
                else
                    *str += QStringLiteral("<div class=\"titlenotes\" id=\"%1\">%2</div>").arg(id,title);
            }
            *str += notes_dict.at(i).second[1];
            if(pExportOptions_->nFootNotes == 3)
                *str += QStringLiteral("</div>");
            *str += QStringLiteral("<div style=\"page-break-before:always;\"></div>");
            if(str->length() > 10000)
            {
                *str += HTMLFOOT;
                //i++;
                html_files << html_content(tmp.arg(QString::number(i+1)));
                str = &html_files.last().content;
                *str += HTMLHEAD;
            }
        }

        *str += HTMLFOOT;
    }

    QMapIterator<QString, QString> ref(ref_files);
    while(ref.hasNext())
    {
        ref.next();
        for(int i=0; i<html_files.count(); i++)
        {
            html_files[i].content.replace(QStringLiteral("href=\"") + ref.key() + QStringLiteral("\""), QStringLiteral("href=\"") + ref.value() + QStringLiteral("\""));
            html_files[i].content.replace(QStringLiteral("href=\"#") + ref.key() + QStringLiteral("\""), QStringLiteral("href=\"") + ref.value() + QStringLiteral("\""));
        }
    }
}

QString fb2mobi::GenerateAZW3(QString file)
{
    Q_CHECK_PTR(pExportOptions_);
    mobiEdit me(file);
    QString azw3File = file.left(file.length()-4) + QStringLiteral("azw3");
    if(!me.SaveAZW(azw3File, pExportOptions_->bRemovePersonal, pExportOptions_->bRepairCover))
        return file;
    QFile().remove(file);
    return azw3File;
}

QString fb2mobi::GenerateMOBI7(QString file)
{
    Q_CHECK_PTR(pExportOptions_);
    mobiEdit me(file);
    QString mobi7File = file.left(file.length()-4) + QStringLiteral("mobi");
    if(!me.SaveMOBI7(mobi7File, pExportOptions_->bRemovePersonal, pExportOptions_->bRepairCover))
        return file;
    return mobi7File;
}

void PaintText(QPainter* painter, QRect rect, int to, const QString &text, QRect* br=0)
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
            image_list << sFileCover_;
        }
        else
        {
            QFile::remove(tmp_dir % u"/OEBPS/"_s % sFileCover_);
        }
        img.save(tmp_dir % u"/OEBPS/"_s % sFileCover_);
        create_new=true;
    }

    QImage img(tmp_dir % u"/OEBPS/"_s % sFileCover_);
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
        PaintText(painter, QRect(delta, delta, r_width, r_heigthTopBottom-delta2), Qt::AlignHCenter|Qt::AlignTop|Qt::TextWordWrap, libs[idLib_].authors[pBook->idFirstAuthor].getName());

        font.setPixelSize(img.height() / 12);
        font.setBold(true);
        painter->setFont(font);
        PaintText(painter, QRect(delta, delta+r_heigthTopBottom+delta2, r_width, r_heigth-r_heigthTopBottom*2-delta2*2), Qt::AlignHCenter|Qt::AlignVCenter|Qt::TextWordWrap, pBook->sName);

        font.setBold(false);
        font.setPixelSize(img.height() / 17);
        painter->setFont(font);
        PaintText(painter,QRect(delta,delta+r_heigth-r_heigthTopBottom+delta2,r_width,r_heigthTopBottom-delta2),Qt::AlignHCenter|Qt::AlignBottom|Qt::TextWordWrap,
                  (pBook->idSerial ==0 ?QStringLiteral("") :libs[idLib_].serials[pBook->idSerial].sName) +
                  (pBook->numInSerial>0 ?QStringLiteral("\n") + QString::number(pBook->numInSerial) :QStringLiteral("")));
    }
    img.save(tmp_dir + QStringLiteral("/OEBPS/") + sFileCover_);
    delete painter;
}

void recurseAddDir(const QDir &d, QStringList & list)
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
            list << QDir::toNativeSeparators(finfo.filePath());
    }
}

void ZipDir(QuaZip *zip, const QDir &dir)
{
    QFile inFile;
    QStringList sl;
    recurseAddDir(dir, sl);
    QFileInfoList files;
    for(const QString &fn: std::as_const(sl))
        files << QFileInfo(fn);
    QuaZipFile outFile(zip);
    char c;
    for(const QFileInfo &fileInfo: std::as_const(files))
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
            qDebug()<<QStringLiteral("testCreate(): inFile.open(): %1").arg(inFile.errorString().toLocal8Bit().constData());
            return;
        }
        if (!outFile.open(QIODevice::WriteOnly, QuaZipNewInfo(fileNameWithRelativePath, fileInfo.filePath())))
        {
            qDebug()<<QStringLiteral("testCreate(): outFile.open(): %1").arg(outFile.getZipError());
            return;
        }
        while (inFile.getChar(&c) && outFile.putChar(c));
        if (outFile.getZipError() != UNZ_OK)
        {
            qDebug()<<QStringLiteral("testCreate(): outFile.putChar(): %1").arg(outFile.getZipError());
            return;
        }
        outFile.close();
        if (outFile.getZipError() != UNZ_OK)
        {
            qDebug()<<QStringLiteral("testCreate(): outFile.close(): %1").arg(outFile.getZipError());
            return;
        }
        inFile.close();
    }
}

QString fb2mobi::convert(uint idBook)
{
    idBook_ = idBook;
    QDir dir(tmp_dir + QStringLiteral("/OEBPS"));
    dir.removeRecursively();
    dir.mkpath(tmp_dir + QStringLiteral("/OEBPS"));

    QBuffer outbuff;
    sOutputFormat_ = u"EPUB"_s;
    if(libs[idLib_].books[idBook].sFormat != u"fb2")
        return QStringLiteral("");
    SBook book_tmp = libs[idLib_].books[idBook];
    pBook = &book_tmp;

    QFile file;
    QString out_file = tmp_dir + u"/book.fb2"_s;
    QFile::remove(out_file);
    file.setFileName(out_file);
    file.open(QFile::WriteOnly);
    file.write(outbuff.data());
    file.close();


    fb2file.setFile(tmp_dir + u"/book.fb2"_s);
    QFile f(tmp_dir + u"/book.fb2"_s);
    if(!f.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug()<<"Error open fb2 file: "<<f.fileName();
        return u""_s;
    }
    join_seria = false;
    current_book = 1;
    generate_html(&f);
   // return tmp_dir+"/freeLib/"+book_cover;
    f.close();

    QString title = pExportOptions_->sCoverLabel;
    if(title.isEmpty())
        title = ExportOptions::sDefaultCoverLabel;
    if(sFileCover_.isEmpty())
        InsertSeriaNumberToCover(title, cc_if_not_exists);
    if(!sFileCover_.endsWith(u".jpg", Qt::CaseInsensitive))
    {
        QImage img(tmp_dir % u"/OEBPS/"_s % sFileCover_);
        sFileCover_ = sFileCover_.left(sFileCover_.length() - QFileInfo(sFileCover_).suffix().length()) + u"jpg"_s;
        img.save(tmp_dir % u"/OEBPS/"_s % sFileCover_);
    }
    return tmp_dir % u"/OEBPS/"_s % sFileCover_;
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
    QMap<QRgb, QList<QColor> > colors;
    for(int i=0; i<h; i+=step)
    {
        for(int j=0; j<w; j+=step)
        {
            QColor color = img.pixel(j,i);
            color.setBlue(color.blue()/error);
            color.setGreen(color.green() / error);
            color.setRed(color.red()/error);
            colors[color.rgb()] << img.pixel(j,i);
        }
    }
    QMap<QRgb,QList<QColor> >::iterator it = colors.begin();
    QList<QColor> result;
    QList<QColor> result_prev;
    int max = 0;
    for(;it != colors.end(); ++it)
    {
       if(max<it.value().count())
       {
           max = it.value().count();
           result_prev = result;
           result = it.value();
       }
    }
    qlonglong r = 0, g = 0, b = 0;
    for(const QColor &color: result)
    {
        r += color.red();
        g += color.green();
        b += color.blue();
    }
    if(result_prev.count() > 0)
    {
        qlonglong pr = 0, pg = 0, pb = 0;
        for(const QColor &color: result_prev)
        {
            pr += color.red();
            pg += color.green();
            pb += color.blue();
        }
        if((pr + pg + pb) / result_prev.count() > (r + g + b) /result.count())
            return QColor(pr/result_prev.count(), pg/result_prev.count(), pb/result_prev.count());
        else
            return QColor(r/result.count(), g/result.count(), b/result.count());
    }
    return QColor(r/result.count(), g/result.count(), b/result.count());
}

struct fontfamily
{
    int font;
    int font_b;
    int font_i;
    int font_bi;
    QMap<QString,QString> tags;
    fontfamily()
    {
        font = -1;
        font_b = -1;
        font_i = -1;
        font_bi = -1;
    }

    bool operator ==(const fontfamily &a) const
    {
        return font == a.font && font_b == a.font_b && font_i == a.font_i && font_bi == a.font_bi;
    }
};

QString fb2mobi::convert(QStringList files, uint idBook)
{
    pBook = &libs[idLib_].books[idBook];
    idBook_ = idBook;
    sOutputFormat_ = pExportOptions_->sOutputFormat;
    if(files.count() == 1)
    {
        fb2file.setFile(files.first());
        QString out_file;
        if(fb2file.suffix().toLower() == u"epub" &&
                (pExportOptions_->sOutputFormat == u"MOBI" ||
                 pExportOptions_->sOutputFormat == u"AZW3"))
        {
            QString sKindlegen = QApplication::applicationDirPath() + QStringLiteral("/kindlegen");
            if(!QFile::exists(sKindlegen))
                sKindlegen = u"kindlegen"_s;
            QProcess::execute(sKindlegen, QStringList() << tmp_dir % u"/"_s % fb2file.fileName());
            out_file = tmp_dir % u"/"_s % fb2file.completeBaseName() % u".mobi"_s;
            if(pExportOptions_->sOutputFormat == u"AZW3")
            {
                out_file = GenerateAZW3(out_file);
            }
            return out_file;
        }
    }
    QString out_file;
    QDir dir(tmp_dir + u"/OEBPS"_s);
    dir.removeRecursively();
    dir.mkpath(tmp_dir + u"/OEBPS"_s);
    dir.mkpath(tmp_dir + u"/OEBPS/css"_s);

    //TODO: Упростить. Копировать дефолтный style.css, только если нет пользовательского
    QFile::copy(u":/xsl/css/style.css"_s, tmp_dir + u"/OEBPS/css/main.css"_s);
    if(pExportOptions_->bUserCSS)
    {
        if(!pExportOptions_->sUserCSS.isEmpty())
        {
            QFile file(tmp_dir + u"/OEBPS/css/main.css"_s);
            file.remove();
            if(file.open(QFile::WriteOnly))
            {
                file.write(pExportOptions_->sUserCSS.toUtf8());
                file.close();
            }
        }
    }

    dir.mkpath(tmp_dir + u"/OEBPS/pic"_s);
    if(pExportOptions_->nVignette > 0)
    {
        dir.setPath(u":/xsl/img"_s);
        QFileInfoList list = dir.entryInfoList();
        for(const QFileInfo &i: std::as_const(list))
        {
            if(i.suffix().toLower() != u"txt")
                QFile::copy(i.absoluteFilePath(), tmp_dir % u"/OEBPS/pic/"_s % i.fileName());
        }
    }

    QString HomeDir;
    if(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).count() > 0)
        HomeDir = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).at(0);
    QString db_path = QFileInfo(options.sDatabasePath).absolutePath() + u"/fonts"_s;


    QFile css(tmp_dir + u"/OEBPS/css/main.css"_s);
    css.open(QFile::Append);
    int count = pExportOptions_->vFontExportOptions.size();
    QStringList fonts;
    QList<fontfamily> fonts_set;
    for(int i=0; i<count; i++)
    {
        const FontExportOptions &fontExportOptions = pExportOptions_->vFontExportOptions.at(i);
        if(fontExportOptions.bUse)
        {
            dir.mkpath(tmp_dir + u"/OEBPS/fonts"_s);
            fontfamily set;
            for(int j=0; j<4; j++)
            {
                QString font_file = j==0 ?fontExportOptions.sFont :j==1 ?fontExportOptions.sFontB :j==2 ?fontExportOptions.sFontI :j==3 ?fontExportOptions.sFontBI :u""_s;
                int index = fonts.indexOf(font_file);
                if(index<0 && !font_file.isEmpty())
                {
                    fonts.append(font_file);
                    index = fonts.count()-1;
                    if(QFile::exists(QApplication::applicationDirPath() % u"/xsl/fonts/"_s % font_file))

                    {
                        QFile::copy(QApplication::applicationDirPath() % u"/xsl/fonts/"_s % font_file, tmp_dir + u"/OEBPS/fonts/font%1.ttf"_s.arg(index));
                    }
                    else
                    {
                        if(QFile::exists(db_path % u"/"_s % font_file))
                        {
                            QFile::copy(db_path % u"/"_s % font_file, tmp_dir + u"/OEBPS/fonts/font%1.ttf"_s.arg(index));
                        }
                        else if(QFile::exists(FREELIB_DATA_DIR % u"/fonts"_s % font_file))
                        {
                            QFile::copy(FREELIB_DATA_DIR % u"/fonts"_s % font_file, tmp_dir + u"/OEBPS/fonts/font%1.ttf"_s.arg(index));
                        }
                        else
                        {
                            QFileInfo fi(font_file);
                            font_file = fi.fileName();
                            QFile::copy(fi.absoluteFilePath(), tmp_dir  + u"/OEBPS/fonts/font%1.ttf"_s.arg(index));
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
            set.tags[vTags[tag_id].css] = sFontSize;
            bool find = false;
            for (int j=0; j<fonts_set.count(); j++)
            {
                if(fonts_set[j] == set)
                {
                    find = true;
                    fonts_set[j].tags[vTags[tag_id].css] = sFontSize;
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
                                             "    src: url(\"../fonts/font%1.ttf\");\n"
                                             "%2"
                                             "%3"
                                             "}\n"_s.arg(
                               (QString::number(j==0?set.font:j==1?set.font_b:j==2?set.font_i:j==3?set.font_bi:0)),
                              (j==1||j==3)?u"    font-weight: bold;\n"_s :u""_s,
                              (j==2||j==3)?u"    font-style: italic;\n"_s :u""_s,
                              QString::number(fonts_set.count())).toUtf8());
                }
                fonts_set << set;
            }
        }
    }
    int font_index = 0;
    for(const fontfamily &set_i: std::as_const(fonts_set))
    {
        QStringList keys = set_i.tags.keys();
        for(const QString &key: std::as_const(keys))
        {
            css.write(u"\n%2 {\n"
                "    font-family: \"font%1\";\n"
                "    font-size: %3%;\n"
                "}\n"_s.arg(font_index).arg(key, set_i.tags[key]).toUtf8());
        }
        font_index++;
    }

    css.close();
    join_seria = files.count() > 1;
    int i = 0;
    for(const QString &file: files)
    {
        fb2file.setFile(file);
        if(fb2file.suffix().toLower() != u"fb2")
            return u""_s;
        QFile f(tmp_dir + u"/book%1.fb2"_s.arg(i));
        if(!f.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            qDebug()<<"Error open fb2 file: "<<f.fileName();
            return u""_s;
        }
        current_book = i + 1;
        generate_html(&f);
        f.close();
        i++;

    }
    QFile f;
    QTextStream ts(&f);
    for(const html_content &html: std::as_const(html_files))
    {
        QString html_file = tmp_dir + u"/OEBPS/%1"_s.arg(html.file_name);
        f.setFileName(html_file);
        f.open(QIODevice::WriteOnly);
        ts << html.content;
        f.close();
    }
    int height{0}, width{0};
    if(!sFileCover_.isEmpty()){
        QImage img(tmp_dir % u"/OEBPS/"_s % sFileCover_);
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
                img.save(tmp_dir % u"/OEBPS/"_s % sFileCover_);
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
                img_new.save(tmp_dir % u"/OEBPS/"_s % sFileCover_);
                width = newWidth;
                height = newHeight;
            }
        }
    }
    if(pExportOptions_->bAddCoverLabel || pExportOptions_->bCreateCover)
    {
        QString abbr = u""_s;
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        if(pBook->idSerial != 0){
            auto partsSequenceName = libs[idLib_].serials[pBook->idSerial].sName.split(' ', Qt::SkipEmptyParts);
            for(const QString &str: std::as_const(partsSequenceName))
                abbr += str.at(0);
        }
#else
        foreach(const QString &str, libs[idLib_].serials[pBook->idSerial].sName.split(' '))
            if(!str.isEmpty())
                abbr += str.at(0);
#endif
        QString title = pExportOptions_->sCoverLabel;
        if(title.isEmpty())
            title = ExportOptions::sDefaultCoverLabel;
        title = libs[idLib_].fillParams(title, idBook_);
        if(pBook->numInSerial == 0 || !pExportOptions_->bAddCoverLabel)
            title = u""_s;
        InsertSeriaNumberToCover(title,
                                 (pExportOptions_->bCreateCoverAlways ?cc_always :(pExportOptions_->bCreateCover ?cc_if_not_exists :cc_no)));
        if(height == 0){
            QImage img(tmp_dir % u"/OEBPS/"_s % sFileCover_);
            height = img.height();
            width = img.width();
        }
    }

    if(!sFileCover_.isEmpty() && pExportOptions_->sOutputFormat == u"EPUB"){
        QString sWidth = QString::number(width);
        QString sHeight = QString::number(height);

        QString sCoverHtml =
            u"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
            "<head>\n"
            "<title/>\n"
            "<link rel=\"stylesheet\" href=\"css/main.css\" type=\"text/css\"/>\n"
            "</head>\n"
            "<body class=\"cover\">\n"
            "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" class=\"cover-svg\" viewBox=\"0 0 "_s % sWidth % u" "_s % sHeight % u"\">\n"
            "<image width=\""_s % sWidth % u"\" height=\""_s % sHeight % u"\" xlink:href=\""_s % sFileCover_ % u"\" xmlns:xlink=\"http://www.w3.org/1999/xlink\"/>\n"
            "</svg>\n"
            "</body>\n"
            "</html>\n"_s.arg(width).arg(height);
        QFile fileCoverHtml(tmp_dir + u"/OEBPS/cover.xhtml"_s);
        if(fileCoverHtml.open(QIODeviceBase::WriteOnly)){
            fileCoverHtml.write(sCoverHtml.toUtf8());
            fileCoverHtml.close();
        }
    }

    generate_toc();
    if(pExportOptions_->nContentPlacement != 0)
    {
        QString toc_file = tmp_dir + u"/OEBPS/toc.html"_s;
        f.setFileName(toc_file);
        f.open(QIODevice::WriteOnly);
        ts << buf;
        f.close();
    }

    if(!buf_annotation.isEmpty() && !pExportOptions_->bAnnotation)
    {
        QString annotation_file = tmp_dir + QStringLiteral("/OEBPS/annotation.html");
        f.setFileName(annotation_file);
        f.open(QIODevice::WriteOnly);
        ts << buf_annotation;
        f.close();
    }

    if(pExportOptions_->sOutputFormat == u"MOBI" ||
            pExportOptions_->sOutputFormat == u"AZW3" ||
            pExportOptions_->sOutputFormat == u"MOBI7")
        generate_ncx();
    else
        generate_ncx_epub();
    QString ncx_file = tmp_dir + u"/OEBPS/toc.ncx"_s;
    f.setFileName(ncx_file);
    f.open(QIODevice::WriteOnly);
    ts << buf;
    f.close();

    if(pExportOptions_->sOutputFormat == u"MOBI" ||
            pExportOptions_->sOutputFormat == u"AZW3" ||
            pExportOptions_->sOutputFormat == u"MOBI7")
        generate_opf();
    else
        generate_opf_epub();
    QString opf_file = tmp_dir + u"/OEBPS/book.opf"_s;
    f.setFileName(opf_file);
    f.open(QIODevice::WriteOnly);
    ts << buf;
    f.close();

    if(pExportOptions_->sOutputFormat == u"MOBI" ||
            pExportOptions_->sOutputFormat == u"AZW3" ||
            pExportOptions_->sOutputFormat == u"MOBI7")
    {
        QString sKindlegen = QApplication::applicationDirPath() + u"/kindlegen"_s;
        if(!QFile::exists(sKindlegen))
            sKindlegen = QStringLiteral("kindlegen");
        QProcess::execute(sKindlegen, QStringList() << opf_file);
        out_file = tmp_dir + u"/OEBPS/book.mobi"_s;
    }
    if(pExportOptions_->sOutputFormat == QStringLiteral("AZW3"))
    {
        out_file = GenerateAZW3(out_file);
    }
    if(pExportOptions_->sOutputFormat == u"MOBI7")
    {
        out_file = GenerateMOBI7(out_file);
    }
    else if(pExportOptions_->sOutputFormat == u"EPUB")
    {
        if(QDir(tmp_dir + u"/OEBPS/pic"_s).entryList(QStringList(),QDir::Files).count() == 0)
        {
            QDir().rmpath(tmp_dir + u"/OEBPS/pic"_s);
        }
        QDir().mkpath(tmp_dir + u"/META-INF"_s);
        generate_mime();
        QString mime_file = tmp_dir + u"/mimetype"_s;;
        f.setFileName(mime_file);
        f.open(QIODevice::WriteOnly);
        ts << buf;
        f.close();
        generate_container();
        QString container_file = tmp_dir + u"/META-INF/container.xml"_s;
        f.setFileName(container_file);
        f.open(QIODevice::WriteOnly);
        ts << buf;
        f.close();

        QuaZip zip(tmp_dir + u"/book.epub"_s);
        zip.open(QuaZip::mdCreate);
        QuaZipFile zip_file(&zip);
        zip_file.open(QIODevice::WriteOnly, QuaZipNewInfo(u"mimetype"_s), 0, 0, 0, 0);
        QFile file(mime_file);
        file.open(QIODevice::ReadOnly);
        zip_file.write(file.readAll());
        zip_file.close();
        ZipDir(&zip, QDir(tmp_dir + u"/OEBPS"_s));
        ZipDir(&zip, QDir(tmp_dir + u"/META-INF"_s));
        zip.close();

        out_file = tmp_dir + u"/book.epub"_s;
    }
    else if(pExportOptions_->sOutputFormat == u"PDF")
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
    return out_file;
}

