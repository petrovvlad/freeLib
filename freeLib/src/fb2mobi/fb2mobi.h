#ifndef FB2MOBI_H
#define FB2MOBI_H

#include "../common.h"
#include <QTextStream>
#include <QXmlStreamReader>
#include <QDomDocument>
#include "hyphenations.h"
//#include <QWebView>

struct STOC
{
    QString href;
    QString title;
    int level;
    QString body_name;
    QString html;
};

#define VIGNETTE_TITLE_BEFORE   "tb"
#define VIGNETTE_TITLE_AFTER    "ta"
#define VIGNETTE_CHAPTER_END    "te"

enum CreateCover{cc_no,cc_if_not_exists,cc_always};

struct html_content
{
    QString file_name;
    QString content;
    html_content(QString fn)
    {
        file_name=fn;
    }
};

struct cross_ref
{
    QString from;
    QString to;
};

class fb2mobi:public QObject
{
    Q_OBJECT
public:
    fb2mobi();
    //QString convert(QString files, bool remove, QString format, QString language);
    QString convert(QStringList files, bool remove,QString format,book_info &bi);
    QString convert(qlonglong id);
    void generate_html(QFile *file);
    //QWebView *pdf;
private:
    QString GenerateAZW3(QString file);
    QString GenerateMOBI7(QString file);
    void parse_title(QDomNode elem);
    void parse_subtitle(QDomNode elem);
    void parse_epigraph(QDomNode elem);
    void parse_annotation(QDomNode elem);
    void parse_section(QDomNode elem);
    void parse_strong(QDomNode elem);
    void parse_emphasis(QDomNode elem);
    void parse_strikethrough(QDomNode elem);
    void parse_style(QDomNode elem);
    void parse_a(QDomNode elem);
    void parse_image(QDomNode elem);
    void parse_p(QDomNode elem);
    void parse_poem(QDomNode elem);
    void parse_stanza(QDomNode elem);
    void parse_v(QDomNode elem);
    void parse_cite(QDomNode elem);
    void parse_emptyline(QDomNode elem);
    void parse_textauthor(QDomNode elem);
    void parse_table(QDomNode elem);
    void parse_code(QDomNode elem);
    void parse_date(QDomNode elem);
    void parse_other(QDomNode elem);

    QString save_html(QString str);

    void parse_body(QDomNode elem);
    void parse_description(QDomNode child);
    void parse_binary(QDomNode elem);
    void parse_format(QDomNode elem, QString tag="" , QString css="", QString href="");
    void parse_span(QString span, QDomNode elem);

    void generate_toc();
    void generate_ncx_epub();
    void generate_ncx();
    void generate_opf();
    void generate_opf_epub();
    void generate_mime();
    void generate_container();
    void parse_note_elem(QDomNode elem);
    void parse_table_element(QDomNode elem);

    void get_notes_dict(QString body_names);
    QString get_vignette(QString level,QString type);
    QString *buf_current;
    QString buf;
    QString buf_annotation;
    //    QXmlStreamReader doc;
    QDomDocument doc;
    QString book_author;
    bool first_body;
    bool header;
    bool inline_image_mode;
    bool subheader;
    bool dropcaps;
    QString nodropcaps;
    int current_header_level;
    int current_section_level;
    bool cut_html;
    QString body_name;
    book_info *book_inf;
    //QString book_series;
    //QString book_title;
    QString book_cover;
    //QString book_lang;
    QString bookseriestitle;
    QString authorstring;
    QString isbn;
    //QString book_series_num;
    QString annotation_title;
    QString notes_title;
    //QString book_genre;
    QString book_anntotation;
    //QString language_from_library;
   // QString book_anntotation_for_opf;

    bool need_page_break;
    bool hide_annotation;
    bool annotation;
    bool first_header_in_body;
    bool first_chapter_line;
    int toc_index;
    bool no_paragraph;
    QString toctitle;
    int toc_max_level;
    bool dodropcaps;
    int notes_mode;
    bool parsing_note;
    QStringList notes_bodies;
    QList<QStringList> current_notes;
    QList<QPair<QString,QStringList> > notes_dict;
    QList<html_content> html_files;
    QMap<QString,QString> ref_files; //соответствие id элемента файлу
    QMap<QString,cross_ref> crossing_ref; //перекрестные ссылки

    QFileInfo fb2file;

    QString href_pref;

    QList<STOC> toc;
    QStringList image_list;
    //QString current_file;
    QString tmp_dir;

    hyphenations hyphenator;
    int hyphenate;
    int vignette;
    bool break_after_cupture;

    bool join_seria;
    int current_book;
    QString outputFormat;

    void InsertSeriaNumberToCover(QString number, CreateCover create_cover);
    bool need_end_chapter_vignette;
    //bool page_load;
//private slots:
//    void OnLoad(bool ok);
};

#endif // FB2MOBI_H
