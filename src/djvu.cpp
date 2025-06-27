#include "djvu.h"

#include <QImage>
#include <algorithm>
#include <libdjvu/miniexp.h>

#include "utilites.h"

DjVu::DjVu()
    :pDocument_(nullptr),
     pContext_(nullptr),
    libDjVu(u"djvulibre"_s),
    djvu_context_create(nullptr),
    djvu_context_release(nullptr),
    djvu_document_create_by_filename_utf8(nullptr),
    djvu_document_job(nullptr),
    djvu_job_status(nullptr),
    djvu_job_release(nullptr),
    djvu_message_wait(nullptr),
    djvu_message_peek(nullptr),
    djvu_message_pop(nullptr),
    djvu_page_create_by_pageno(nullptr),
    djvu_page_job(nullptr),
    djvu_page_get_width(nullptr),
    djvu_page_get_height(nullptr),
    djvu_format_create(nullptr),
    djvu_format_release(nullptr),
    djvu_format_set_row_order(nullptr),
    djvu_page_render(nullptr)
{
}

DjVu::~DjVu()
{
    if(pDocument_ && djvu_job_release && djvu_document_job)
        djvu_document_release(pDocument_);
    if(pContext_ && djvu_context_release)
        djvu_context_release(pContext_);
}

bool DjVu::loadLibrary()
{

    if ( !djvu_context_create && !(djvu_context_create = (DjvuContextCreate)libDjVu.resolve("ddjvu_context_create")) ) [[unlikely]]
        return false;
    if ( !djvu_context_release && !(djvu_context_release = (DjvuContextRelease)libDjVu.resolve("ddjvu_context_release")) ) [[unlikely]]
        return false;
    if( !djvu_document_create_by_filename_utf8 && !(djvu_document_create_by_filename_utf8 = (DjvuDocumentCreateByFilenameUtf8)libDjVu.resolve("ddjvu_document_create_by_filename_utf8")) ) [[unlikely]]
        return false;
    if( !djvu_document_job && !(djvu_document_job = (DjvuDocumentJob)libDjVu.resolve("ddjvu_document_job")) ) [[unlikely]]
        return false;
    if( !djvu_job_status && !(djvu_job_status = (DjvuJobStatus)libDjVu.resolve("ddjvu_job_status")) ) [[unlikely]]
        return false;
    if( !djvu_job_release && !(djvu_job_release = (DjvuJobRelease)libDjVu.resolve("ddjvu_job_release")) ) [[unlikely]]
        return false;
    if( !djvu_message_wait && !(djvu_message_wait = (DjvuMessageWait)libDjVu.resolve("ddjvu_message_wait")) ) [[unlikely]]
        return false;
    if( !djvu_message_peek && !(djvu_message_peek = (DjvuMessagePeek)libDjVu.resolve("ddjvu_message_peek")) ) [[unlikely]]
        return false;
    if( !djvu_message_pop && !(djvu_message_pop =(DjvuMessagePop)libDjVu.resolve("ddjvu_message_pop")) ) [[unlikely]]
        return false;
    if( !djvu_page_create_by_pageno && !(djvu_page_create_by_pageno = (DjvuPageCreateByPageno)libDjVu.resolve("ddjvu_page_create_by_pageno")) ) [[unlikely]]
        return false;
    if( !djvu_page_job && !(djvu_page_job = (DjvuPageJob)libDjVu.resolve("ddjvu_page_job")) ) [[unlikely]]
        return false;
    if( !djvu_page_render && !(djvu_page_render = (DjvuPageRender)libDjVu.resolve("ddjvu_page_render")) ) [[unlikely]]
        return false;
    if( !djvu_page_get_width && !(djvu_page_get_width = (DjvuPageGetWidth)libDjVu.resolve("ddjvu_page_get_width")) ) [[unlikely]]
        return false;
    if( !djvu_page_get_height && !(djvu_page_get_height = (DjvuPageGetHeight)libDjVu.resolve("ddjvu_page_get_height")) ) [[unlikely]]
        return false;
    if( !djvu_format_create && !(djvu_format_create = (DjvuFormatCreate)libDjVu.resolve("ddjvu_format_create")) ) [[unlikely]]
        return false;
    if( !djvu_format_release && !(djvu_format_release = (DjvuFormatRelease)libDjVu.resolve("ddjvu_format_release")) ) [[unlikely]]
        return false;
    if( !djvu_format_set_row_order && !(djvu_format_set_row_order = (DjvuFormatSetRowOrder)libDjVu.resolve("ddjvu_format_set_row_order")) ) [[unlikely]]
        return false;

    return true;
}

void DjVu::waitForDdjvuMessage(ddjvu_message_tag_t tag )
{
    djvu_message_wait(pContext_);
    const ddjvu_message_t *msg;
    while ( (msg = djvu_message_peek(pContext_)) && (msg->m_any.tag != tag) )
    {
        if(msg->m_any.tag == DDJVU_ERROR) [[unlikely]]{
            if (msg->m_error.filename)
                LogWarning << "ddjvu:" << msg->m_error.message << "ddjvu:" << "'" << msg->m_error.filename << ":" << msg->m_error.lineno;
            else
                LogWarning << "ddjvu:" << msg->m_error.message;
            return;
        }
        djvu_message_pop(pContext_);
    }
}

bool DjVu::openDocument(const QString &sFileName)
{
    QFileInfo info(sFileName);
    if ( !info.isReadable() ) [[unlikely]]
    {
        qWarning() << "cannot read file: " << sFileName;
        return false;
    }

    if( !pContext_ ){
        pContext_ = djvu_context_create("freeLib");
        if ( !pContext_ ) [[unlikely]]
        {
            qWarning() << "cannot create context";
            return false;
        }
    }

    if(pDocument_)
        djvu_document_release(pDocument_);
    pDocument_ = djvu_document_create_by_filename_utf8(pContext_, sFileName.toUtf8(), 0);
    if ( !pDocument_ ) [[unlikely]]
    {
        qWarning() << "cannot create decoder";
        return false;
    }
    while (! djvu_document_decoding_done(pDocument_))
        waitForDdjvuMessage(DDJVU_DOCINFO);

    return true;
}

QImage DjVu::getCover()
{
    QImage   image;
    ddjvu_page_t *pPage = djvu_page_create_by_pageno ( pDocument_, 0 ); // pages 0
    if ( !pPage )
        return image;

    while( !djvu_page_decoding_done(pPage) )
        waitForDdjvuMessage(DDJVU_PAGEINFO);

    ddjvu_rect_t pageRect, rendRect;
    pageRect.w = djvu_page_get_width(pPage);
    pageRect.h = djvu_page_get_height(pPage);

    if ( pageRect.w > 1 && pageRect.h > 1 )
    {
        const float nMaxHeight = 512; //максимальная высота обложки
        float scale = std::min( nMaxHeight, static_cast<float>(pageRect.h) ) / static_cast<float>(pageRect.h);
        pageRect.x = 0;
        pageRect.y = 0;
        pageRect.w = static_cast<int>( pageRect.w * scale );
        pageRect.h = static_cast<int>( pageRect.h * scale );
        rendRect = pageRect;

        static uint masks[4] = { 0xff0000, 0x00ff00, 0x0000ff, 0xff000000 };
        ddjvu_format_t *pFormat = djvu_format_create(DDJVU_FORMAT_RGBMASK32, 4, masks);
        if( pFormat )
        {
            djvu_format_set_row_order(pFormat, 1);
            image = QImage( pageRect.w, pageRect.h, QImage::Format_RGB32 );
            if ( !djvu_page_render(pPage, DDJVU_RENDER_COLOR, &pageRect, &rendRect, pFormat, image.bytesPerLine(), reinterpret_cast<char*>(image.bits())) )
                image = QImage();
            djvu_format_release(pFormat);
        }
    }
    djvu_page_release( pPage );
    return image;
}

