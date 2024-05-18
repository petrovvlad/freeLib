#ifndef DJVU_H
#define DJVU_H

#include <QLibrary>
#include <libdjvu/ddjvuapi.h>

class DjVu
{    
    typedef ddjvu_context_t* (*DjvuContextCreate)(const char *programname);
    typedef void (*DjvuContextRelease)(ddjvu_context_t *context);
    typedef ddjvu_document_t* (*DjvuDocumentCreateByFilenameUtf8)(ddjvu_context_t *context, const char *filename, int cache);
    typedef ddjvu_job_t* (*DjvuDocumentJob)(ddjvu_document_t *document);
    typedef ddjvu_status_t (*DjvuJobStatus)(ddjvu_job_t *job);
    typedef void (*DjvuJobRelease)(ddjvu_job_t *job);
    typedef ddjvu_message_t* (*DjvuMessageWait)(ddjvu_context_t *context);
    typedef ddjvu_message_t* (*DjvuMessagePeek)(ddjvu_context_t *context);
    typedef void (*DjvuMessagePop)(ddjvu_context_t *context);
    typedef ddjvu_page_t* (*DjvuPageCreateByPageno)(ddjvu_document_t *document, int pageno);
    typedef ddjvu_job_t* (*DjvuPageJob)(ddjvu_page_t *page);
    typedef int (*DjvuPageRender)(ddjvu_page_t *page, const ddjvu_render_mode_t mode, const ddjvu_rect_t *pagerect, const ddjvu_rect_t *renderrect, const ddjvu_format_t *pixelformat, unsigned long rowsize, char *imagebuffer );
    typedef int (*DjvuPageGetWidth)(ddjvu_page_t *page);
    typedef int (*DjvuPageGetHeight)(ddjvu_page_t *page);
    typedef ddjvu_format_t* (*DjvuFormatCreate)(ddjvu_format_style_t style, int nargs, unsigned int *args);
    typedef void (*DjvuFormatRelease)(ddjvu_format_t *format);
    typedef void (*DjvuFormatSetRowOrder)(ddjvu_format_t *format, int top_to_bottom);

public:
    DjVu();
    ~DjVu();
    bool loadLibrary();
    bool openDocument(const QString &sFileName);
    QImage getCover();

private:
    void waitForDdjvuMessage(ddjvu_message_tag_t tag );

    DDJVUAPI ddjvu_context_t  * pContext_;    // This object holds global data structures such as the cache of decoded pages, or the list of pending event messages
    DDJVUAPI ddjvu_document_t * pDocument_;   // is a subclass of <ddjvu_job_t>

    QLibrary libDjVu;
    DjvuContextCreate djvu_context_create;
    DjvuContextRelease djvu_context_release;
    DjvuDocumentCreateByFilenameUtf8 djvu_document_create_by_filename_utf8;
    DjvuDocumentJob djvu_document_job;
    DjvuJobStatus djvu_job_status;
    DjvuJobRelease djvu_job_release;
    DjvuMessageWait djvu_message_wait;
    DjvuMessagePeek djvu_message_peek;
    DjvuMessagePop djvu_message_pop;
    DjvuPageCreateByPageno djvu_page_create_by_pageno;
    DjvuPageJob djvu_page_job;
    DjvuPageRender djvu_page_render;
    DjvuPageGetWidth djvu_page_get_width;
    DjvuPageGetHeight djvu_page_get_height;
    DjvuFormatCreate djvu_format_create;
    DjvuFormatRelease djvu_format_release;
    DjvuFormatSetRowOrder djvu_format_set_row_order;


#define djvu_document_decoding_status(document) \
    djvu_job_status(djvu_document_job(document))
#define djvu_document_decoding_done(document) \
        (djvu_document_decoding_status(document) >= DDJVU_JOB_OK)
#define djvu_document_decoding_error(document) \
        (djvu_document_decoding_status(document) >= DDJVU_JOB_FAILED)
#define djvu_document_release(document) \
        djvu_job_release(djvu_document_job(document))


#define djvu_page_decoding_status(page) \
            djvu_job_status(djvu_page_job(page))
#define djvu_page_decoding_done(page) \
        (djvu_page_decoding_status(page) >= DDJVU_JOB_OK)
#define djvu_page_decoding_error(page) \
        (djvu_page_decoding_status(page) >= DDJVU_JOB_FAILED)
#define djvu_page_release(page) \
        djvu_job_release(djvu_page_job(page))
};

#endif // DJVU_H
