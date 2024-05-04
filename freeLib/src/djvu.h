#ifndef DJVU_H
#define DJVU_H

#include <libdjvu/ddjvuapi.h>

class DjVu
{
public:
    DjVu();
    ~DjVu();
    bool openDocument(const QString &sFileName);
    QImage getCover();

private:
    void waitForDdjvuMessage( /*ddjvu_context_t *ctx, */ddjvu_message_tag_t tag );

    DDJVUAPI ddjvu_context_t  * pContext_;    // This object holds global data structures such as the cache of decoded pages, or the list of pending event messages
    DDJVUAPI ddjvu_document_t * pDocument_;   // is a subclass of <ddjvu_job_t>
};

#endif // DJVU_H
