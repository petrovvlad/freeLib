#include "bookfile.h"

#include <QDir>
#include <QFont>
#include <QPainter>
#include <QStringBuilder>
#include <QTextDocument>
#ifdef QUAZIP_STATIC
#include "quazip/quazip/quazipfile.h"
#else
#include <quazip/quazipfile.h>
#endif

#include "utilites.h"
#include "options.h"
#ifdef USE_DEJVULIBRE
#include "djvu.h"
#endif

BookFile::BookFile(uint idLib, uint idBook)
    :bOpen_(false)
{
    pLib_ = &g::libs[idLib];
    idBook_ = idBook;
}

BookFile::BookFile(SLib *pLib, uint idBook)
    :bOpen_(false)
{
    pLib_ = pLib;
    idBook_ = idBook;
}

BookFile::~BookFile()
{
}

void BookFile::open()
{
    SBook &book = pLib_->books[idBook_];
    QString sLibPath = RelativeToAbsolutePath(pLib_->path);
    if(book.sArchive.isEmpty()){
        QString sFile = u"%1/%2.%3"_s.arg(sLibPath, book.sFile, book.sFormat);
        file_.setFileName(sFile);
        if(!file_.open(QFile::ReadOnly))
        {
            LogWarning << "Error open file!" << sFile;
            return;
        }
        bOpen_ = true;
        QFileInfo fi(file_);
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
        timeBirth_ =  fi.birthTime();
#else
        timeBirth_ = fi.created();
#endif
    }else{
        QString sFile = u"%1.%2"_s.arg(book.sFile, book.sFormat);
        QString sArchive = sLibPath % u"/"_s % book.sArchive;
        sArchive.replace(u".inp"_s, u".zip"_s);

        bool bZipInZip = book.sArchive.contains(u".zip/"_s);

        QuaZip uz;
        QuaZipFile zipFile;
        if(bZipInZip){
            data_ = openZipInZip(sArchive, sFile);
        }else{
            uz.setZipName(sArchive);
            if( !uz.open(QuaZip::mdUnzip) ) [[unlikely]]
            {
                LogWarning << "Error open archive!" << sArchive;
                return;
            }
            setCurrentZipFileName(&uz, sFile);
            zipFile.setZip(&uz);
            if(!zipFile.open(QIODevice::ReadOnly)) [[unlikely]]
            {
                LogWarning << "Error open file:" << sFile;
            }else {
                data_ = zipFile.readAll();
                QuaZipFileInfo64 fiZip;
                if(uz.getCurrentFileInfo(&fiZip))
                    timeBirth_ =  fiZip.dateTime;
                zipFile.close();
                bOpen_ = true;
            }
            uz.close();
        }
    }
}

QImage BookFile::cover()
{
    QString sImg = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) % u"/covers/"_s % QString::number(idBook_) % u".jpg"_s;
    QImage img;
    if(QFile::exists(sImg))
        return QImage(sImg);

    SBook &book = pLib_->books[idBook_];
    if(!bOpen_)
        open();
    QString sFormat = book.sFormat;
    if(pEpub_ != nullptr || sFormat == u"epub")
        img = coverEpub();
    else if(book.sArchive.contains(u".zip/"_s))
        img = coverZip();
    else if(sFormat == u"fb2")
        img = coverFb2();
#ifdef USE_DEJVULIBRE
    else if(sFormat == u"djvu" || sFormat == u"djv")
        img = coverDjvu();
#endif
    else if(sFormat.endsWith(u"zip"))
            img = coverZip();

    if(!img.isNull() && img.width() < img.height()*2){
        img.save(sImg);
        cleanCoversCache();
    }else
        img = createCover();
    return img;
}

QImage BookFile::coverFb2()
{
    SBook &book = pLib_->books[idBook_];
    QDomDocument doc;

    if(book.sArchive.isEmpty()){
        if(file_.isOpen()){
            file_.reset();
            doc.setContent(&file_);
        }
    }else{
        if(data_.size() != 0)
            doc.setContent(data_);
    }

    return coverFb2(doc);
}

QImage BookFile::coverFb2(const QDomDocument &doc)
{
    QImage img;
    if(doc.isDocument()){
        QDomElement title_info = doc.elementsByTagName(u"title-info"_s).at(0).toElement();
        auto elmImg =  doc.elementsByTagName(u"title-info"_s).at(0).toElement().elementsByTagName(u"coverpage"_s).at(0).toElement().elementsByTagName(u"image"_s).at(0).attributes();
        QString sCover  = elmImg.namedItem(u"l:href"_s).toAttr().value();
        if(sCover.isEmpty())
            sCover  = elmImg.namedItem(u"xlink:href"_s).toAttr().value();
        if(!sCover.isEmpty() && sCover.at(0) == u'#')
        {
            sCover = sCover.right(sCover.length() - 1);
            QDomNodeList binarys = doc.elementsByTagName(u"binary"_s);
            for(int i=0; i<binarys.count(); i++)
            {
                if(binarys.at(i).attributes().namedItem(u"id"_s).toAttr().value() == sCover)
                {
                    QByteArray ba64;
                    ba64.append(binarys.at(i).toElement().text().toLatin1());
                    QByteArray ba = QByteArray::fromBase64(ba64);
                    img = QImage::fromData(ba);
                    break;
                }
            }
        }

    }
    return img;
}

QImage BookFile::coverEpub()
{
    QImage cover;
    initializeEpub();
    if(pEpub_ != nullptr)
        cover = pEpub_->readCover();

    return cover;
}

void BookFile::annotationEpub()
{
    initializeEpub();
    if(pEpub_ != nullptr)
        sAnnotation_ = pEpub_->readAnnotation();
}

#ifdef USE_DEJVULIBRE
QImage BookFile::coverDjvu()
{
    QImage img;
    static DjVu djvu;
    if(bOpen_ && djvu.loadLibrary()){
        SBook &book = pLib_->books[idBook_];
        QString sDjVuFile;
        QFile fileDjVu;
        if(book.sArchive.isEmpty()){
            sDjVuFile = pLib_->path % u"/"_s % book.sFile % u"."_s % book.sFormat;
        }else{
            sDjVuFile = QDir::tempPath() % u"/freeLib/"_s % QString::number(idBook_) % u".djvu"_s;
            fileDjVu.setFileName(sDjVuFile);
            if(!fileDjVu.open(QIODevice::ReadWrite))
                return img;
            fileDjVu.write(data_);
            fileDjVu.close();
        }

        if(djvu.openDocument(sDjVuFile))
            img = djvu.getCover();
        if(!fileDjVu.fileName().isEmpty())
            fileDjVu.remove();
    }
    return img;
}
#endif

QImage BookFile::coverZip()
{
    QImage img;
    SBook &book = pLib_->books[idBook_];
    bool bZipInZip = book.sArchive.contains(u".zip/"_s);
    if(bZipInZip){
        QString sLibPath = RelativeToAbsolutePath(pLib_->path);
        QString sArchive = sLibPath % u"/"_s % book.sArchive;
        QString sFile = book.sFile + u".fbd"_s;
        QByteArray ba = openZipInZip(sArchive, sFile);
        QDomDocument doc;
        doc.setContent(ba);
        return coverFb2(doc);
    }

    QBuffer buffer;
    QuaZip zip;
    if(data_.size() != 0){
        buffer.setData(data_);
        zip.setIoDevice(&buffer);
    }
    zip.open(QuaZip::mdUnzip);
    auto listFi = zip.getFileInfoList64();
    for(const auto &fi :std::as_const(listFi)){
        if(fi.name.endsWith(u".fbd")){
            zip.setCurrentFile(fi.name);
            QuaZipFile zipFile(&zip);
            zipFile.open(QIODevice::ReadOnly);
            QByteArray ba = zipFile.readAll();
            zipFile.close();
            QDomDocument doc;
            doc.setContent(ba);
            return coverFb2(doc);
        }
    }
    for(const auto &fi :std::as_const(listFi)){
#ifdef USE_DEJVULIBRE
        if(fi.name.endsWith(u".djvu")){
            zip.setCurrentFile(fi.name);
            QuaZipFile zipFile(&zip);
            zipFile.open(QIODevice::ReadOnly);
            QByteArray data = zipFile.readAll();
            zipFile.close();
            QString sDjVuFile = QDir::tempPath() % u"/freeLib/"_s % QString::number(idBook_) % u".djvu"_s;
            QFile fileDjVu;
            fileDjVu.setFileName(sDjVuFile);
            if(!fileDjVu.open(QIODevice::ReadWrite))
                break;
            fileDjVu.write(data);
            fileDjVu.close();
            static DjVu djvu;
            if(djvu.loadLibrary() && djvu.openDocument(sDjVuFile))
                img = djvu.getCover();
            if(!fileDjVu.fileName().isEmpty())
                fileDjVu.remove();
        } else
#endif
            if(fi.name.endsWith(u".epub")){
                zip.setCurrentFile(fi.name);
                QuaZipFile zipFile(&zip);
                zipFile.open(QIODevice::ReadOnly);
                QByteArray data = zipFile.readAll();
                zipFile.close();
                pEpub_ = std::make_unique<EpubReader>(data);
                img = pEpub_->readCover();
                return img;
            }
    }

    zip.close();
    return img;
}

void paintText(QPainter* painter, QRect rect, int flags, const QString &sText)
{
    QFont font(painter->font());
    QRect bound;
    do
    {
        font.setPixelSize( font.pixelSize() - 1);
        painter->setFont(font);
        bound = painter->boundingRect(rect, flags, sText);
    }while((bound.width()>rect.width() || bound.height()>rect.height()) && font.pixelSize()>5);
    painter->drawText(rect, flags, sText);
}

QImage BookFile::createCover()
{
    SBook &book = pLib_->books[idBook_];

    QImage img(u":/xsl/img/cover.jpg"_s);
    img = img.convertToFormat(QImage::Format_RGB32);
    QPainter* painter = new QPainter(&img);
    QFont font;
    int delta = img.rect().height() / 50;
    int delta2 = img.rect().height() / 100;
    int r_width = img.rect().width()-delta * 2;
    int r_heigth = img.rect().height()-delta * 2;
    int r_heigthTopBottom = r_heigth / 4;

    font.setPixelSize(img.height() / 15);
    painter->setFont(font);
    paintText(painter, QRect(delta, delta, r_width, r_heigthTopBottom-delta2), Qt::AlignHCenter|Qt::AlignTop|Qt::TextWordWrap,
              pLib_->authors[book.idFirstAuthor].getName());

    font.setPixelSize(img.height() / 12);
    font.setBold(true);
    painter->setFont(font);
    paintText(painter, QRect(delta, delta+r_heigthTopBottom+delta2, r_width, r_heigth-r_heigthTopBottom*2-delta2*2),
              Qt::AlignHCenter|Qt::AlignVCenter|Qt::TextWordWrap, book.sName);

    if(!book.mSequences.empty()){
        font.setBold(false);
        font.setPixelSize(img.height() / 17);
        painter->setFont(font);
        const auto &sequence = book.mSequences.begin();
        QString sSequence = pLib_->serials[sequence->first].sName;
        paintText(painter, QRect(delta, delta+r_heigth-r_heigthTopBottom+delta2, r_width, r_heigthTopBottom-delta2),
                  Qt::AlignHCenter|Qt::AlignBottom|Qt::TextWordWrap,
                  (sSequence % (sequence->second>0 ?u"\n"_s % QString::number(sequence->second) :u""_s)));
    }
    delete painter;
    return img;
}


QString BookFile::annotation()
{
    if(!bOpen_)
        open();
    if(sAnnotation_.isEmpty()){
        SBook &book = pLib_->books[idBook_];
        QString sFormat = book.sFormat;
        if(pEpub_ != nullptr || sFormat == u"epub")
            annotationEpub();
        else if(sFormat == u"fb2"_s)
            annotationFb2();
        else if(sFormat.endsWith(u"zip"))
            annotationZip();

        sAnnotation_ = sAnnotation_.trimmed();
        QDomDocument htmlDoc;
        if(htmlDoc.setContent(u"<html>"_s % sAnnotation_ % u"</html>"_s)){
            // Функция для обработки элементов
            auto processElement = [](QDomElement &element, QDomDocument &doc, auto &self) -> void{
                // Удаляем style
                element.removeAttribute(u"style"_s);

                // Обрабатываем <font face="...">
                if(element.tagName().toLower() == u"font"_s && element.hasAttribute(u"face"_s)){
                    QDomNodeList children = element.childNodes();
                    QDomNode parent = element.parentNode();
                    for (int i = children.count() - 1; i >= 0; --i) {
                        QDomNode child = children.at(i).cloneNode(true);
                        parent.insertBefore(child, element);
                    }
                    parent.removeChild(element);
                }else{
                    // Проверяем текстовые узлы на \n
                    QDomNodeList children = element.childNodes();
                    for (int i = children.count() - 1; i >= 0; --i) {
                        QDomNode child = children.at(i);
                        if (child.isText() && child.nodeValue().contains(u"\n")) {
                            QStringList lines = child.nodeValue().split(u"\n"_s, Qt::SkipEmptyParts);
                            QDomNode parent = child.parentNode();
                            parent.removeChild(child);
                            for (const QString &line : std::as_const(lines)) {
                                QString trimmedLine = line.trimmed();
                                if (!trimmedLine.isEmpty()) {
                                    QDomElement p = doc.createElement(u"p"_s);
                                    p.appendChild(doc.createTextNode(trimmedLine));
                                    parent.insertAfter(p, child.nextSibling());
                                }
                            }
                        } else if (child.isElement()) {
                            QDomElement childEl = child.toElement();
                            self(childEl, doc, self);
                        }
                    }
                }
            };

            // Обрабатываем все элементы
            QDomElement root = htmlDoc.documentElement();
            processElement(root, htmlDoc, processElement);

            // Извлекаем содержимое без <html>
            QString result = htmlDoc.toString(-1).trimmed();
            result.remove(0, result.indexOf(u'>') + 1);
            result.chop(7); // Удаляем </html>
            sAnnotation_ = result.trimmed();
        }else{
            // Fallback: используем QTextDocument
            QTextDocument textDoc;
            textDoc.setHtml(sAnnotation_);
            QString plainText = textDoc.toPlainText().trimmed();
            QStringList lines = plainText.split(u'\n', Qt::SkipEmptyParts);
            QStringList paragraphs;
            for (const QString &line : std::as_const(lines)) {
                QString trimmedLine = line.trimmed();
                if (!trimmedLine.isEmpty()) {
                    paragraphs.append(u"<p>"_s % trimmedLine.toHtmlEscaped() % u"</p>"_s);
                }
            }
            sAnnotation_ = paragraphs.join(u""_s);        }
        if(sAnnotation_.isEmpty())
            sAnnotation_ = u" "_s;
    }
    return sAnnotation_;
}


void BookFile::annotationFb2()
{
    QDomDocument doc;
    SBook &book = pLib_->books[idBook_];

    if(book.sArchive.isEmpty()){
        if(file_.isOpen()){
            file_.reset();
            doc.setContent(&file_);
        }
    }else{
        if(data_.size() != 0)
            doc.setContent(data_);
    }
    annotationFb2(doc);
}

void BookFile::annotationFb2(const QDomDocument &doc)
{
    if(doc.isDocument()){
        QDomElement titleInfo = doc.elementsByTagName(u"title-info"_s).at(0).toElement();
        sAnnotation_ = titleInfo.elementsByTagName(u"annotation"_s).at(0).toElement().text();
    }
}

void BookFile::annotationZip()
{
    SBook &book = pLib_->books[idBook_];
    QBuffer buffer;
    QuaZip zip;
    if(data_.size() != 0){
        buffer.setData(data_);
        zip.setIoDevice(&buffer);
    }
    zip.open(QuaZip::mdUnzip);
    auto listFi = zip.getFileInfoList64();
    for(const auto &fi :std::as_const(listFi)){
        if(fi.name.endsWith(u".fbd")){
            zip.setCurrentFile(fi.name);
            QuaZipFile zipFile(&zip);
            zipFile.open(QIODevice::ReadOnly);
            QByteArray ba = zipFile.readAll();
            zipFile.close();
            QDomDocument doc;
            doc.setContent(ba);
            annotationFb2(doc);
            return;
        }
    }
    for(const auto &fi :std::as_const(listFi)){
        if(fi.name.endsWith(u".epub")){
            zip.setCurrentFile(fi.name);
            QuaZipFile zipFile(&zip);
            zipFile.open(QIODevice::ReadOnly);
            QByteArray data = zipFile.readAll();
            zipFile.close();
            pEpub_ = std::make_unique<EpubReader>(data);
            sAnnotation_ = pEpub_->readAnnotation();
            return;
        }
    }
}

QDateTime BookFile::birthTime() const
{
    return timeBirth_;
}

QString BookFile::fileName() const
{
    QString sFormat;
    const SBook &book = pLib_->books[idBook_];
    if(book.sFormat.endsWith(u".zip") && book.sFormat.size()>4)
        sFormat = book.sFormat.left(book.sFormat.size()-4);
    else
        sFormat = book.sFormat;
    QString sFileName = book.sFile % u"."_s % sFormat;
    return sFileName;
}

QString BookFile::filePath() const
{
    const SBook &book = pLib_->books[idBook_];
    QString sFilePath;
    if(book.sArchive.isEmpty())
        sFilePath = pLib_->path + u"/"_s ;
    else{
        sFilePath = pLib_->path % u"/"_s % book.sArchive;
        sFilePath.replace(u".inp"_s, u".zip"_s);
    }

    return sFilePath;
}

qint64 BookFile::fileSize() const
{
    const SBook &book = pLib_->books[idBook_];
    QString sFile;
    if(book.sArchive.isEmpty()){
        sFile = pLib_->path % u"/"_s % book.sFile % u"."_s % book.sFormat;;
    }else{
        QString  sArchive = book.sArchive;
        sArchive.replace(u".inp"_s, u".zip"_s);
        auto nIndex = sArchive.indexOf(u".zip/"_s);
        if(nIndex >= 0)
            sArchive = sArchive.left(nIndex + 4);
        sFile = pLib_->path % u"/"_s % sArchive;
    }
    QFileInfo fi(sFile);
    qint64 size = fi.size();
    return size;
}

QByteArray BookFile::data()
{
    if(!bOpen_)
        open();
    const SBook &book = pLib_->books[idBook_];
    if(book.sArchive.isEmpty())
        data_ = file_.readAll();
    else{
        if(book.sFormat == u"pdf.zip")
            return dataZip(u".pdf"_s);
        else if(book.sFormat == u"djvu.zip")
            return dataZip(u".djvu"_s);
        else if(book.sFormat == u"epub.zip")
            return dataZip(u".epub"_s);
    }
    return data_;
}

QByteArray BookFile::dataZip(const QString &sSubFormat)
{
    QBuffer buffer;
    QuaZip zip;
    QByteArray ba;
    if(data_.size() != 0){
        buffer.setData(data_);
        zip.setIoDevice(&buffer);
    }
    zip.open(QuaZip::mdUnzip);
    auto listFi = zip.getFileInfoList64();
    for(const auto &fi :std::as_const(listFi)){
        if(fi.name.endsWith(sSubFormat)){
            zip.setCurrentFile(fi.name);
            QuaZipFile zipFile(&zip);
            zipFile.open(QIODevice::ReadOnly);
            ba = zipFile.readAll();
            zipFile.close();
            break;
        }
    }
    return ba;
}

QByteArray BookFile::openZipInZip(const QString &sArchive, const QString &sFileName)
{
    QByteArray data;
    QBuffer buffer;
    QStringList sZipChain;
    QuaZip uz;
    QuaZipFile zipFile;
    qsizetype posNext = 0;
    qsizetype posPrev = 0;
    while(posNext != -1){
        posNext = sArchive.indexOf(u".zip"_s, posPrev);
        if(posNext != -1){
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            sZipChain << sArchive.sliced(posPrev, posNext + 4 - posPrev);
#else
            QStringRef sZip = sArchive.leftRef(posNext + 4);
            sZipChain << sZip.right(sZip.length() - posPrev).toString();
#endif
            posPrev = posNext + 5;
        }
    }

    std::vector<QByteArray> vData;
    for(int i=1; i<sZipChain.count(); i++){
        QBuffer tmpBuffer;
        if(i==1){
            QString sFirstArchive = sZipChain[0];
            uz.setZipName(sFirstArchive);
        }else{
            tmpBuffer.setData(vData.back());
            uz.setIoDevice(&tmpBuffer);
        }

        if (!uz.open(QuaZip::mdUnzip)) [[unlikely]]
        {
            LogWarning << "Error open archive:" << sArchive;
            return data;
        }
        zipFile.setZip(&uz);
        setCurrentZipFileName(&uz, sZipChain[i]);
        if(!zipFile.open(QIODevice::ReadOnly)) [[unlikely]]
        {
            LogWarning << "Error open file:" << sZipChain[i];
            return data;
        }
        vData.emplace_back(zipFile.readAll());
        zipFile.close();
        uz.close();
    }
    if(!vData.empty()){
        buffer.setData(vData.back());
        uz.setIoDevice(&buffer);
        uz.open(QuaZip::mdUnzip);
    }
    setCurrentZipFileName(&uz, sFileName);
    zipFile.setZip(&uz);
    if(!zipFile.open(QIODevice::ReadOnly))
        LogWarning << "Error open file:" << sFileName;
    else
        data = zipFile.readAll();

    return data;
}

void BookFile::cleanCoversCache()
{
    qint64 nSizeDir = 0;
    static QStringList nameFilters = {u"*.jpg"_s};
    QDir dirCovers(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + u"/covers"_s);
    QFileInfoList fiList = dirCovers.entryInfoList(nameFilters, QDir::Files, QDir::Time);
    uint nCount = fiList.count();
    for(uint i = 0; i<nCount; i++){
        const auto &fi = fiList[i];
        qint64 nNewSizeDir = nSizeDir + fi.size();
        if(nNewSizeDir > g::options.nCacheSize && i>0)
            QFile::remove(fi.absoluteFilePath());
        else
            nSizeDir = nNewSizeDir;
    }
}

void BookFile::initializeEpub()
{
    if(pEpub_ == nullptr){
        SBook &book = pLib_->books[idBook_];
        if(book.sArchive.isEmpty()){
            if(file_.isOpen()){
                file_.reset();
                QByteArray data = file_.readAll();
                pEpub_ = std::make_unique<EpubReader>(data);
            }
        }else{
            if(!data_.isEmpty())
                pEpub_ = std::make_unique<EpubReader>(data_);
        }
    }
}
