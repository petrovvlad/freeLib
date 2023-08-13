#ifndef FB2MOBI_H
#define FB2MOBI_H

#include <QTextStream>
#include <QXmlStreamReader>
#include <QDomDocument>

#include "../library.h"
#include "hyphenations.h"
#include "options.h"

struct STOC
{
    QString href;
    QString title;
    int level;
    QString body_name;
    QString html;
};

#define VIGNETTE_TITLE_BEFORE   QStringLiteral("tb")
#define VIGNETTE_TITLE_AFTER    QStringLiteral("ta")
#define VIGNETTE_CHAPTER_END    QStringLiteral("te")

enum CreateCover{cc_no,cc_if_not_exists,cc_always};

struct html_content
{
    QString file_name;
    QString content;
    html_content(const QString &fn)
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
    fb2mobi(const ExportOptions *pExportOptions, uint idLib);
    QString convert(QStringList files, uint idBook);
    QString convert(uint idBook);
    void generate_html(QFile *file);
    //QWebView *pdf;
private:
    QString GenerateAZW3(QString file);
    QString GenerateMOBI7(QString file);
    void parse_title(const QDomNode &elem);
    void parse_subtitle(const QDomNode &elem);
    void parse_epigraph(const QDomNode &elem);
    void parse_annotation(const QDomNode &elem);
    void parse_section(const QDomNode &elem);
    void parse_strong(const QDomNode &elem);
    void parse_emphasis(const QDomNode &elem);
    void parse_strikethrough(const QDomNode &elem);
    void parse_style(const QDomNode &elem);
    void parse_a(const QDomNode &elem);
    void parse_image(const QDomNode &elem);
    void parse_p(const QDomNode &elem);
    void parse_poem(const QDomNode &elem);
    void parse_stanza(const QDomNode &elem);
    void parse_v(const QDomNode &elem);
    void parse_cite(const QDomNode &elem);
    void parse_emptyline(const QDomNode &elem);
    void parse_textauthor(const QDomNode &elem);
    void parse_table(const QDomNode &elem);
    void parse_code(const QDomNode &elem);
    void parse_date(const QDomNode &elem);
    void parse_other(const QDomNode &elem);

    QString save_html(const QString &str);

    void parse_body(const QDomNode &elem);
    void parse_description(const QDomNode &child);
    void parse_binary(const QDomNode &elem);
    void parse_format(const QDomNode &elem, QString tag=QStringLiteral("") , QString css=QStringLiteral(""), QString href=QStringLiteral(""));
    void parse_span(const QString &span, const QDomNode &elem);

    void generate_toc();
    void generate_ncx_epub();
    void generate_ncx();
    void generate_opf();
    void generate_opf_epub();
    void generate_mime();
    void generate_container();
    void parse_note_elem(const QDomNode &elem);
    void parse_table_element(const QDomNode &elem);

    void get_notes_dict(const QString &body_names);
    QString get_vignette(const QString &level, const QString &type);
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
    QString nodropcaps;
    int current_header_level;
    int current_section_level;
    QString body_name;
    SBook *pBook;
    uint idBook_;
    uint idLib_;
    QString book_cover;
    QString bookseriestitle;
    QString authorstring;
    QString isbn;
    QString annotation_title;
    QString notes_title;
    QString book_anntotation;

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
    QString tmp_dir;

    hyphenations hyphenator;

    bool join_seria;
    int current_book;
    QString outputFormat;

    void InsertSeriaNumberToCover(const QString &number, CreateCover create_cover);
    bool need_end_chapter_vignette;
    const ExportOptions *pExportOptions_;
//private slots:
//    void OnLoad(bool ok);
};

#endif // FB2MOBI_H
