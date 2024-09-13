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

struct HtmlContent
{
    QString sFileName;
    QString sContent;
    HtmlContent(const QString &fn)
    {
        sFileName = fn;
    }
};

struct CrossRef
{
    QString from;
    QString to;
};

class fb2mobi:public QObject
{
    Q_OBJECT
public:
    fb2mobi(const ExportOptions *pExportOptions, uint idLib);
    QString convert(const std::vector<QString> &files, uint idBook);
    QString convert(uint idBook);
    void generate_html(QFile *file);
    //QWebView *pdf;
private:
    QString GenerateAZW3(const QString &file);
    QString GenerateMOBI7(const QString &file);
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
    void parse_format(const QDomNode &elem, QString tag=u""_s , QString css=u""_s, QString href=u""_s);
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
    QString *pBufCurrent_;
    QString buf;
    QString sBufAnnotation_;
    QDomDocument doc;
    QString book_author;
    bool first_body;
    bool bHeader_; //Признак формирования заголовка
    bool bInlineImageMode_; //Индикатор режима вставки картинок (inline)
    bool bSubHeader_; //Признак формирования подзаголовка
    QString sNoDropcaps_; //Строка символов, для исключения буквицы
    int current_header_level;  //Уровень текущего заголовка
    int current_section_level;
    QString body_name; //Имя текущего раздела body, например notes
    SBook *pBook_;
    uint idBook_;
    uint idLib_;
    QString sFileCover_;
    QString bookseriestitle;
    QString authorstring;
    QString isbn;
    QString sAnnotationTitle_; //Заголовок для раздела аннотации
    QString sNotesTitle_; //Dictionary of note body titles
    QString sBookName_;
    QString sBookAnntotation_;
    static const QString sHtmlHead_;
    static const QString sHtmlFoot_;


    bool need_page_break;
    bool bAannotation_;
    bool first_header_in_body; //Признак первого заголовка в секции body
    bool first_chapter_line; //Признак первой строки в главе (секции) - для расстановки dropcaps
    int toc_index; //Текущий номер раздела содержания
    bool bNoParagraph_; //Индикатор, что последующий парагаф находится в эпиграфе, аннотации и т.п.
    QString toctitle; //Заголовок для раздела содержания
    int toc_max_level; //Максимальный уровень заголовка (секции) для помещения в содержание (toc.xhtml). В toc.ncx помещаются все уровни
    bool dodropcaps; //Признак вставки стилей буквицы (dropcaps)
    bool parsing_note;
    std::vector<std::vector<QString>> vCurrentNotes_; //Переменная для хранения текущей сноски
    std::vector<std::pair<QString, std::vector<QString>> > vNotesDict; //Словарь со сносками и комментариями
    std::vector<HtmlContent> vHtmlFiles_;
    std::unordered_map<QString, QString> refFiles_; //соответствие id элемента файлу
    std::unordered_map<QString, CrossRef> crossingRef_; //перекрестные ссылки

    QFileInfo fb2file;

    QString href_pref;

    std::vector<STOC> toc;
    std::vector<QString> vImageList_; //Массив для хранения списка картинок
    QString sTmpDir_;

    hyphenations hyphenator;

    bool join_seria;
    int current_book;
    ExportFormat outputFormat_;

    void InsertSeriaNumberToCover(const QString &number, CreateCover create_cover);
    bool need_end_chapter_vignette;
    const ExportOptions *pExportOptions_;
//private slots:
//    void OnLoad(bool ok);
};

#endif // FB2MOBI_H
