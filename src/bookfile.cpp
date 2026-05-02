#include "bookfile.h"

#include <QDir>
#include <QFont>
#include <QPainter>
#include <QStringBuilder>
#include <QTextDocument>

#include "utilites.h"
#include "options.h"
#ifdef USE_DEJVULIBRE
#include "djvu.h"
#endif
#ifdef USE_POPPLER
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <poppler/qt5/poppler-qt5.h>
#else //QT_VERSION
#include <poppler/qt6/poppler-qt6.h>
#endif//QT_VERSION
#endif //USE_POPPLER

#include "qarchive.h"

BookFile::BookFile(uint idLib, uint idBook)
    :BookFile(g::libs[idLib], idBook)
{
}

BookFile::BookFile(const SLib &lib, uint idBook)
    :bOpen_(false)
{
    bOpen_ = false;
    sLibPath_ = RelativeToAbsolutePath(lib.path) + u"/"_s;
    idBook_ = idBook;
    const SBook &book = lib.books.at(idBook_);
    sFile_ = book.sFile;
    sFormat_ = book.sFormat;
    sArchive_ = book.sArchive;
    sBookName_ = book.sName;
    sAuthorName_ = lib.authors.at(book.idFirstAuthor).getName();
    if(!book.series.empty()){
        const auto &series = book.series.begin();
        sSeries_ = lib.series.at(series->first).sName % (series->second>0 ?u"\n"_s % QString::number(series->second) :u""_s);
    }
}

BookFile::~BookFile()
{
}

void BookFile::open()
{
    if(sArchive_.isEmpty()){
        QString sFile = sLibPath_ % sFile_ % u"."_s % sFormat_;
        file_.setFileName(sFile);
        if(!file_.open(QFile::ReadOnly))
        {
            LogWarning << "Error open file!" << sFile;
            return;
        }
        QFileInfo fi(file_);
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
        timeBirth_ =  fi.birthTime();
#else
        timeBirth_ = fi.created();
#endif
        bOpen_ = true;
    }else{
        QString sArchive = sLibPath_ % sArchive_;
        QString sFile = u"%1.%2"_s.arg(sFile_, sFormat_);
        sArchive.replace(u".inp"_s, u".zip"_s);
        bool bArcInArc = std::ranges::any_of(QArchive::formats, [this](const QString& format){return sArchive_.contains(u"."_s % format % u"/"_s);});

        if(bArcInArc){
            data_ = openArcInArc(sArchive, sFile);
            QString sFbdFile = sFile_ + u".fbd"_s;
            QByteArray ba = openArcInArc(sArchive, sFbdFile);
            if(!ba.isEmpty())
                doc_.setContent(ba);
            bOpen_ = !data_.isEmpty();
        }else{
            QArchive ar(sArchive);
            ar.setCurrentFile(sFile);
            data_ = ar.readFile();
            timeBirth_ = ar.getMTime();
            bOpen_ = true;
        }
    }
    if(sFormat_ == u"fb2")
    {
        if(!doc_.isDocument()){
            if(data_.size() != 0)
                doc_.setContent(data_);
            else if(file_.isOpen())
            {
                file_.reset();
                doc_.setContent(&file_);
            }
        }
    }else if(sFormat_ == u"epub")
        openEpub();
    bool bArch = std::ranges::any_of(QArchive::formats, [this](const QString& suffix)
        { return sFormat_.endsWith(suffix); });
    if(bArch)
    {
        if(data_.size() != 0){
            QArchive a(data_);
            auto fileList = a.fileList();
            for( const auto &sFile :fileList){
                if(sFile.endsWith(u".fbd") && !doc_.isDocument()){
                    QByteArray ba = a.readFile(sFile);
                    doc_.setContent(ba);
                }else
                if(sFile.endsWith(u".epub") && pEpub_ == nullptr){
                    QByteArray ba = a.readFile(sFile);
                    pEpub_ = std::make_unique<EpubReader>(ba);
                }
            }
        }
    }
}

void BookFile::openAsync()
{
    futureOpen_ = QtConcurrent::run([this]() {
        open();
    });
}

QImage BookFile::cover()
{
    if(!cover_.isNull())
        return cover_;
    QString sImg = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) % u"/covers/"_s % QString::number(idBook_) % u".jpg"_s;
    QImage img;
    if(QFile::exists(sImg))
        return QImage(sImg);

    if(!bOpen_)
        return createCover();
    if(doc_.isDocument())
        img = coverFb2();
    if(img.isNull()){
        if(pEpub_ != nullptr)
            img = coverEpub();
#ifdef USE_DEJVULIBRE
        else if(sFormat_ == u"djvu" || sFormat_ == u"djv")
            img = coverDjvu();
#endif //USE_DEJVULIBRE
#ifdef USE_POPPLER
        else if(sFormat_ == u"pdf")
            img = coverPdf();
#endif //USE_POPPLER
        else
        {
            bool bArch = std::ranges::any_of(QArchive::formats, [this](const QString& format)
                       { return sFormat_.endsWith(format) || sArchive_.contains(u"."_s % format % u"/"_s); });
            if(bArch)
                img = coverArchive();
        }
    }

    if(!img.isNull() && img.width() < img.height()*2){
        img.save(sImg);
        cleanCoversCache();
    }else{
        cover_ = createCover();
        return cover_;
    }

    return img;
}

QFuture<QImage> BookFile::coverAsync()
{
    Q_ASSERT(!futureCover_.isRunning());
    futureCover_ = QtConcurrent::run([this]() {
        QString sImg = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) % u"/covers/"_s % QString::number(idBook_) % u".jpg"_s;
        if(QFile::exists(sImg))
            return QImage(sImg);

        futureOpen_.waitForFinished();
        return cover();
    });
    return futureCover_;
}

bool BookFile::isBusy()
{
    return futureOpen_.isRunning() || futureCover_.isRunning() || futureAnnotation_.isRunning();
}

bool BookFile::isCoverCached()
{
    if(!cover_.isNull())
        return true;
    QString sImg = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) % u"/covers/"_s % QString::number(idBook_) % u".jpg"_s;
    return QFile::exists(sImg);
}

QImage BookFile::coverFb2()
{
    QImage img;
    if(doc_.isDocument()){
        QDomElement title_info = doc_.elementsByTagName(u"title-info"_s).at(0).toElement();
        auto elmImg =  doc_.elementsByTagName(u"title-info"_s).at(0).toElement().elementsByTagName(u"coverpage"_s).at(0).toElement().elementsByTagName(u"image"_s).at(0).attributes();
        QString sCover  = elmImg.namedItem(u"l:href"_s).toAttr().value();
        if(sCover.isEmpty())
            sCover  = elmImg.namedItem(u"xlink:href"_s).toAttr().value();
        if(!sCover.isEmpty() && sCover.at(0) == u'#')
        {
            sCover = sCover.right(sCover.length() - 1);
            QDomNodeList binarys = doc_.elementsByTagName(u"binary"_s);
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
    if(pEpub_ != nullptr)
        cover = pEpub_->readCover();

    return cover;
}

QString BookFile::annotationEpub()
{
    QString sAnnotation;
    if(pEpub_ != nullptr)
        sAnnotation = pEpub_->readAnnotation();
    return sAnnotation;
}

bool isUniformColor(const QImage& img) {
    if (img.isNull() || img.width() == 0 || img.height() == 0)
        return true;

    const uint32_t* data = reinterpret_cast<const uint32_t*>(img.constBits());
    uint32_t firstPixel = data[0];
    size_t pixelCount = img.width() * img.height();
    size_t differentPixels = 0;
    size_t i = 0;
#ifdef __AVX512F__
    // Векторная обработка с AVX-512 (16 пикселей за раз)
    {
        __m512i firstPixelVec = _mm512_set1_epi32(firstPixel);
        for (; i <= pixelCount - 16; i += 16) {
            __m512i pixels = _mm512_loadu_si512(data + i);
            __mmask16 mask = _mm512_cmpneq_epi32_mask(pixels, firstPixelVec);
            differentPixels += _mm_popcnt_u32(static_cast<uint32_t>(mask));
        }
        // Обработка оставшихся пикселей (меньше 16) с маской
        if (i < pixelCount) {
            size_t nTail = pixelCount - i;
            __mmask16 loadMask = (1U << nTail) - 1;
            __m512i pixels = _mm512_maskz_loadu_epi32(loadMask, data + i);
            __mmask16 mask = _mm512_mask_cmpneq_epi32_mask(loadMask, pixels, firstPixelVec);
            differentPixels += _mm_popcnt_u32(static_cast<uint32_t>(mask));
        }
    }
#else
#ifdef __AVX2__
    // Векторная обработка с AVX2 (8 пикселей за раз)
    {
        __m256i firstPixelVec = _mm256_set1_epi32(firstPixel);
        for (; i <= pixelCount - 8; i += 8) {
            __m256i pixels = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(data + i));
            __m256i cmp = _mm256_cmpeq_epi32(pixels, firstPixelVec);
            uint32_t mask = _mm256_movemask_epi8(cmp);
            differentPixels += _mm_popcnt_u32(~mask) / 4;
        }
    }

#ifdef __clang__ //отключение векторизации и развёртывания следующего цикла
#pragma clang loop vectorize(disable)
#pragma clang loop unroll(disable)
#elif defined(__GNUC__)
#pragma GCC novector
#pragma GCC unroll 0
#endif

#endif //__AVX2__
    // Обработка оставшихся пикселей (скалярный код)
    for (; i < pixelCount; ++i) {
        if (data[i] != firstPixel) {
             ++differentPixels;
        }
    }
#endif //__AVX512F__

    float ratio = static_cast<float>(differentPixels) / static_cast<float>(pixelCount);
    return ratio < 0.01f;
}

#ifdef USE_POPPLER
QImage BookFile::coverPdf()
{
    QImage img;
    std::unique_ptr<Poppler::Document> doc;
    if(sArchive_.isEmpty()){
        if(file_.isOpen()){
            file_.reset();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            doc = std::unique_ptr<Poppler::Document>(Poppler::Document::load(&file_));
#else
            doc = Poppler::Document::load(&file_);
#endif
        }
    }else{
        if(data_.size() != 0){
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            doc = std::unique_ptr<Poppler::Document>(Poppler::Document::loadFromData(data_));
#else
            doc = Poppler::Document::loadFromData(data_);
#endif
        }
    }
    if(doc){
        std::unique_ptr<Poppler::Page> page(doc->page(0));
        if(page){
            img = page->renderToImage();
            if(isUniformColor(img))
                img = QImage();
        }
    }

    return img;
}
#endif //USE_POPPLER

#ifdef USE_DEJVULIBRE
QImage BookFile::coverDjvu()
{
    QImage img;
    static DjVu djvu;
    static std::mutex m;
    if(bOpen_ && djvu.loadLibrary()){       
        QString sDjVuFile;
        QFile fileDjVu;
        if(sArchive_.isEmpty()){
            sDjVuFile = sLibPath_ % sFile_ % u"."_s % sFormat_;
        }else{
            sDjVuFile = QDir::tempPath() % u"/freeLib/"_s % QString::number(idBook_) % u".djvu"_s;
            fileDjVu.setFileName(sDjVuFile);
            if(!fileDjVu.open(QIODevice::ReadWrite))
                return img;
            fileDjVu.write(data_);
            fileDjVu.close();
        }
        {
            std::lock_guard<std::mutex> guard(m);
            if(djvu.openDocument(sDjVuFile))
                img = djvu.getCover();
        }
        if(!fileDjVu.fileName().isEmpty())
            fileDjVu.remove();
        if(isUniformColor(img))
            img = QImage();
    }
    return img;
}
#endif

QImage BookFile::coverArchive()
{
    QImage img;
    QArchive ar;
    if(data_.size() != 0){
        ar.setData(data_);
    }
    auto files = ar.fileList();
    for(auto &sFile :files){
#ifdef USE_DEJVULIBRE
        if(sFile.endsWith(u".djvu")){
            QString sDjVuFile = QDir::tempPath() % u"/freeLib/"_s % QString::number(idBook_) % u".djvu"_s;
            if(!ar.extractFileTo(sFile, sDjVuFile))
                return img;
            static DjVu djvu;
            static std::mutex m;
            if(djvu.loadLibrary()){
                std::lock_guard<std::mutex> guard(m);
                if(djvu.openDocument(sDjVuFile)){
                    img = djvu.getCover();
                }
            }
            QFile::remove(sDjVuFile);
            if(isUniformColor(img))
                img = QImage();
        } else
#endif
#ifdef USE_POPPLER
        if(sFile.endsWith(u".pdf")){
            QByteArray ba = ar.readFile(sFile);
            std::unique_ptr<Poppler::Document> doc(Poppler::Document::loadFromData(ba));
            if(doc){
                std::unique_ptr<Poppler::Page> page(doc->page(0));
                if(page){
                    img = page->renderToImage();
                    if(!isUniformColor(img))
                        return img;
                }
            }
        }
#endif //USE_POPPLER
        if(sFile.endsWith(u".epub")){
            if(pEpub_){
                img = pEpub_->readCover();
                return img;
            }
        }

    }
    return img;
}

void paintText(QPainter &painter, QRect rect, int flags, const QString &sText)
{
    QFont font(painter.font());
    QRect bound;
    do
    {
        font.setPixelSize( font.pixelSize() - 1);
        painter.setFont(font);
        bound = painter.boundingRect(rect, flags, sText);
    }while((bound.width()>rect.width() || bound.height()>rect.height()) && font.pixelSize()>5);
    painter.drawText(rect, flags, sText);
}

QImage BookFile::createCover()
{
    QImage img(u":/xsl/img/cover.jpg"_s);
    QPainter painter(&img);
    QFont font;
    int delta = img.rect().height() / 50;
    int delta2 = img.rect().height() / 100;
    int r_width = img.rect().width() - delta * 2;
    int r_heigth = img.rect().height() - delta * 2;
    int r_heigthTopBottom = r_heigth / 4;

    font.setPixelSize(img.height() / 15);
    painter.setFont(font);
    paintText(painter, QRect(delta, delta, r_width, r_heigthTopBottom-delta2), Qt::AlignHCenter|Qt::AlignTop|Qt::TextWordWrap,
              sAuthorName_);

    font.setPixelSize(img.height() / 12);
    font.setBold(true);
    painter.setFont(font);
    paintText(painter, QRect(delta, delta+r_heigthTopBottom+delta2, r_width, r_heigth-r_heigthTopBottom*2-delta2*2),
              Qt::AlignHCenter|Qt::AlignVCenter|Qt::TextWordWrap, sBookName_);

    if(!sSeries_.isEmpty()){
        font.setBold(false);
        font.setPixelSize(img.height() / 17);
        painter.setFont(font);
        paintText(painter, QRect(delta, delta+r_heigth-r_heigthTopBottom+delta2, r_width, r_heigthTopBottom-delta2),
                  Qt::AlignHCenter|Qt::AlignBottom|Qt::TextWordWrap,
                  sSeries_);
    }
    return img;
}

QString BookFile::annotation()
{
    if(!bOpen_)
        return u" "_s;

    QString sAnnotation;
    if(doc_.isDocument())
        sAnnotation = annotationFb2();

    if(sAnnotation.isEmpty() && pEpub_ != nullptr)
        sAnnotation = annotationEpub();

    sAnnotation = sAnnotation.trimmed();
    if(!sAnnotation.isEmpty()){
        QDomDocument htmlDoc;
        if(htmlDoc.setContent(u"<html>"_s % sAnnotation % u"</html>"_s)){
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
            sAnnotation = result.trimmed();
        }else{
            // Fallback: используем QTextDocument
            sAnnotation.replace(u"<p>"_s, u""_s)
                       .replace(u"</p>"_s, u"<br/>"_s)
                       .replace(u'\n', u"<br/>"_s);
            QTextDocument textDoc;
            textDoc.setHtml(sAnnotation);
            QString plainText = textDoc.toPlainText().trimmed();
            QStringList lines = plainText.split(u'\n', Qt::SkipEmptyParts);
            QStringList paragraphs;
            for (const QString &line : std::as_const(lines)) {
                QString trimmedLine = line.trimmed();
                if (!trimmedLine.isEmpty())
                    paragraphs.append(u"<p>"_s % trimmedLine.toHtmlEscaped() % u"</p>"_s);
            }
            sAnnotation = paragraphs.join(u""_s);
        }
    }else
        sAnnotation = u" "_s;
    return sAnnotation;
}

QFuture<QString> BookFile::annotationAsync()
{
    futureAnnotation_ = QtConcurrent::run([this]() {
        futureOpen_.waitForFinished();
        return annotation();
    });
    return futureAnnotation_;
}

QString BookFile::annotationFb2()
{
    QString sAnnotation;
    if(doc_.isDocument()){
        QDomElement titleInfo = doc_.elementsByTagName(u"title-info"_s).at(0).toElement();
        QDomElement annotation = titleInfo.elementsByTagName(u"annotation"_s).at(0).toElement();

        // Обрабатываем дочерние узлы annotation
        QDomNodeList children = annotation.childNodes();
        for (int i = 0; i < children.count(); ++i) {
            QDomNode node = children.at(i);
            if (node.isElement() && node.nodeName() == u"p"_s) {
                // Добавляем текст параграфа с тегом <p>
                QString sText = node.toElement().text().trimmed();
                if(!sText.isEmpty())
                    sAnnotation += u"<p>"_s % sText % u"</p>"_s;
            }
        }
    }
    return sAnnotation;
}

QDateTime BookFile::birthTime() const
{
    return timeBirth_;
}

QString BookFile::fileName() const
{
    QString sFileName = sFile_ % u"."_s % fileFormat();
    return sFileName;
}

QString BookFile::fileFormat() const
{
    QString sFormat;
    bool bCompoundArchFormat = false;
    if(bOneBookInArchive_){
        for(const auto &sArchFormat :QArchive::formats){
            if(sFormat_.endsWith(sArchFormat) && sFormat_.size()>sArchFormat.size()+1){
                sFormat = sFormat_.left(sFormat_.size() - (sArchFormat.size()+1));
                break;
            }
        }
        if(sFormat.isEmpty()){
            bool bArch = std::ranges::any_of(QArchive::formats, [this](const QString& suffix)
                                             { return sFormat_.endsWith(suffix); });
            if(bArch){
                if(sBookName_.endsWith(u"(pdf)"))
                    sFormat = u"pdf"_s;
                else if(sBookName_.endsWith(u"(djvu)") || sBookName_.endsWith(u"(djv)"))
                    sFormat = u"djvu"_s;
                else if(sBookName_.endsWith(u"(epub)"))
                    sFormat = u"epub"_s;
            }

        }
    }
    if(sFormat.isEmpty())
        sFormat = sFormat_;

    return sFormat;
}

QString BookFile::filePath() const
{
    if(sArchive_.isEmpty())
        return sLibPath_;

    QString sFilePath = sLibPath_ % sArchive_;
    sFilePath.replace(u".inp"_s, u".zip"_s);
    return sFilePath;
}

qint64 BookFile::fileSize() const
{
    QString sFile;
    if(sArchive_.isEmpty()){
        sFile = sLibPath_ % sFile_ % u"."_s % sFormat_;
    }else{
        QString  sArchive = sArchive_;
        sArchive.replace(u".inp"_s, u".zip"_s);
        auto nIndex = sArchive.indexOf(u".zip/"_s);
        if(nIndex >= 0)
            sArchive = sArchive.left(nIndex + 4);
        sFile = sLibPath_ % sArchive;
    }
    QFileInfo fi(sFile);
    qint64 size = fi.size();
    return size;
}

QByteArray BookFile::data()
{
    futureOpen_.waitForFinished();
    if(!bOpen_)
        open();
    if(sArchive_.isEmpty())
        data_ = file_.readAll();
    else{
        for(const auto &sArchFormat :QArchive::formats){
            if(sFormat_ == u"pdf."_s + sArchFormat || (sFormat_ == sArchFormat && sBookName_.endsWith(u"(pdf)")))
                return dataArchive(u".pdf"_s);
            else if(sFormat_ == u"djvu."_s + sArchFormat || (sFormat_ == sArchFormat && (sBookName_.endsWith(u"(djvu)") || sBookName_.endsWith(u"(djv)"))))
                return dataArchive(u".djvu"_s);
            else if(sFormat_ == u"epub."_s + sArchFormat || (sFormat_ == sArchFormat && sBookName_.endsWith(u"(epub)")))
                return dataArchive(u".epub"_s);
        }
    }
    return data_;
}

QByteArray BookFile::dataArchive(const QString &sSubFormat)
{
    QByteArray ba;
    if(data_.size() != 0){
        QArchive a(data_);
        auto fileList = a.fileList();
        int nCount = 0;
        for(const auto &sFile :fileList){
            if(sFile.endsWith(sSubFormat)){
                nCount++;
                if(nCount > 1)
                    continue;
                ba = a.readFile(sFile);
            }
        }
        bOneBookInArchive_ = (nCount == 1);
        if(!bOneBookInArchive_){
            return data_;
        }
    }
    return ba;
}

QByteArray BookFile::openArcInArc(const QString &sArchive, const QString &sFileName)
{
    QByteArray data;
    QStringList sArcChain;
    qsizetype posNext = 0;
    qsizetype posPrev = 0;

    // Собираем цепочку архивов, поддерживая разные форматы
    while (posNext != -1) {
        qsizetype foundPos = -1;
        QString foundExt;

        // Ищем любой из поддерживаемых форматов
        for (const QString &format : QArchive::formats) {
            qsizetype pos = sArchive.indexOf(u"."_s + format, posPrev);
            if (pos != -1 && (foundPos == -1 || pos < foundPos)) {
                foundPos = pos;
                foundExt = format;
            }
        }

        if (foundPos != -1) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            sArcChain << sArchive.sliced(posPrev, foundPos + foundExt.length() + 1 - posPrev);
#else
            QStringRef sArc = sArchive.leftRef(foundPos + foundExt.length() + 1);
            sArcChain << sArc.right(sArc.length() - posPrev).toString();
#endif
            posPrev = foundPos + foundExt.length() + 2; // +2 for '.' and next char
            posNext = foundPos + 1; // Continue search
        } else {
            posNext = -1;
        }
    }


    if (sArcChain.isEmpty()) {
        LogWarning << "No zip chain found in:" << sArchive;
        return data;
    }

    QArchive archive;
    // Первый элемент — реальный файл на диске
    archive.setPath(sArcChain[0]);

    // Для каждого вложенного архива читаем его содержимое и инициализируем QArchive из байтов
    for (int i = 1; i < sArcChain.count(); ++i) {
        const QString &innerArcName = sArcChain[i];
        QByteArray innerData = archive.readFile(innerArcName); // читаем вложенный архив
        if (innerData.isEmpty()) {
            LogWarning << "Error open nested arc:" << innerArcName;
            return data;
        }
        archive.close();
        archive.setData(innerData); // теперь archive работает с in-memory архив
    }

    // Находим и читаем запрошенный файл в последнем (внешнем/вложенном) архиве
    data = archive.readFile(sFileName);
    if (data.isEmpty()) {
        if(!sFileName.endsWith(u".fbd"))
            LogWarning << "Error open file:" << sFileName;
    }

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

void BookFile::openEpub()
{
    if(pEpub_ == nullptr){
        if(sArchive_.isEmpty()){
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
