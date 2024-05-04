#include "djvu.h"

#include <algorithm>
#include <libdjvu/miniexp.h>

DjVu::DjVu()
    :pDocument_(nullptr),
     pContext_(nullptr)
{
}

DjVu::~DjVu()
{
    if(pDocument_)
        ddjvu_document_release(pDocument_);
    if(pContext_)
        ddjvu_context_release (pContext_);
}

void DjVu::waitForDdjvuMessage(ddjvu_message_tag_t tag )
{
    ddjvu_message_wait(pContext_);
    const ddjvu_message_t *msg;
    while ( (msg = ddjvu_message_peek(pContext_)) && (msg->m_any.tag != tag) )
    {
        if(msg->m_any.tag == DDJVU_ERROR)
            return;
        ddjvu_message_pop(pContext_);
    }
}

bool DjVu::openDocument(const QString &sFileName)
{
    QFileInfo info(sFileName);
    if ( !info.isReadable() )
    {
        qWarning() << "cannot read file: " << sFileName;
        return false;
    }

    if( !pContext_ ){
        pContext_ = ddjvu_context_create("freeLib");
        if ( !pContext_ )
        {
            qWarning() << "cannot create context";
            return false;
        }
    }

    if(pDocument_)
        ddjvu_document_release(pDocument_);
    pDocument_ = ddjvu_document_create_by_filename_utf8(pContext_, sFileName.toUtf8(), 0);
    if ( !pDocument_ )
    {
        qWarning() << "cannot create decoder";
        return false;
    }
    waitForDdjvuMessage(DDJVU_DOCINFO);

    if(ddjvu_document_decoding_error(pDocument_))
    {
        ddjvu_document_release(pDocument_);
        pDocument_ = nullptr;
        return false;
    }

    return true;
}

QImage DjVu::getCover()
{
    QImage   image;
    ddjvu_page_t *pPage = ddjvu_page_create_by_pageno ( pDocument_, 0 ); // pages 0
    if ( !pPage )
        return image;

    while( !ddjvu_page_decoding_done(pPage) )
        waitForDdjvuMessage(/*pContext_,*/ DDJVU_PAGEINFO);

    ddjvu_rect_t pageRect, rendRect;
    pageRect.w = ddjvu_page_get_width(pPage);
    pageRect.h = ddjvu_page_get_height(pPage);

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
        ddjvu_format_t *pFormat = ddjvu_format_create(DDJVU_FORMAT_RGBMASK32, 4, masks);
        if( pFormat )
        {
            ddjvu_format_set_row_order(pFormat, 1);
            image = QImage( pageRect.w, pageRect.h, QImage::Format_RGB32 );
            if ( !ddjvu_page_render(pPage, DDJVU_RENDER_COLOR, &pageRect, &rendRect, pFormat, image.bytesPerLine(), reinterpret_cast<char*>(image.bits())) )
                image = QImage();
            ddjvu_format_release(pFormat);
        }
    }
    ddjvu_page_release( pPage );
    return image;
}

