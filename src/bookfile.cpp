#include "bookfile.h"

#include <QDir>
#include <QDomDocument>
#include <QFont>
#include <QImage>
#include <QPainter>
#include <QStringBuilder>
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
    if(file_.isOpen())
        file_.close();
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
            MyDBG << "Error open file! " << sFile;
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
                MyDBG << "Error open archive! " << sArchive;
                return;
            }
            setCurrentZipFileName(&uz, sFile);
            zipFile.setZip(&uz);
            if(!zipFile.open(QIODevice::ReadOnly)) [[unlikely]]
            {
                MyDBG << "Error open file: " << sFile;
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
    SBook &book = pLib_->books[idBook_];
    QImage img;
    if(QFile::exists(sImg))
        return QImage(sImg);

    if(!bOpen_)
        open();
    QString sFormat = book.sFormat;
    if(book.sArchive.contains(u".zip/"_s))
        img = coverZip();
    else
    if(sFormat == u"fb2")
        img = coverFb2();
    else if(sFormat == u"epub")
        img = coverAnnotationEpub();
#ifdef USE_DEJVULIBRE
    else if(sFormat == u"djvu" || sFormat == u"djv")
        img = coverDjvu();
#endif
    else if(sFormat.endsWith(u"zip"))
            img = coverZip();

    if(!img.isNull()){
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

QImage BookFile::coverAnnotationEpub()
{
    SBook &book = pLib_->books[idBook_];
    QBuffer buffer;
    QuaZip zip;
    if(book.sArchive.isEmpty()){
        if(file_.isOpen()){
            file_.reset();
            zip.setIoDevice(&file_);
        }
    }else{
        if(data_.size() != 0){
            buffer.setData(data_);
            zip.setIoDevice(&buffer);
        }
    }
    return coverAnnotationEpub(zip);
}

QImage BookFile::coverAnnotationEpub(QuaZip &zipEpub)
{
    QImage img;
    zipEpub.open(QuaZip::mdUnzip);
    if(!setCurrentZipFileName(&zipEpub, u"META-INF/container.xml"_s)) [[unlikely]] {
        MyDBG << "Error open file: " << u"META-INF/container.xml"_s;
        return img;
    }
    QuaZipFile zipFile(&zipEpub);
    zipFile.open(QIODevice::ReadOnly);
    QByteArray baInfo = zipFile.readAll();
    zipFile.close();
    QDomDocument doc;
    doc.setContent(baInfo);

    QDomNode root = doc.documentElement();
    bool need_loop = true;
    QString rel_path;
    for(int i=0; i<root.childNodes().count() && need_loop; i++)
    {
        if(root.childNodes().at(i).nodeName().toLower() == u"rootfiles")
        {
            QDomNode roots = root.childNodes().at(i);
            for(int j=0; j<roots.childNodes().count() && need_loop; j++)
            {
                if(roots.childNodes().at(j).nodeName().toLower() == u"rootfile")
                {
                    QString path = roots.childNodes().at(j).attributes().namedItem(u"full-path"_s).toAttr().value();
                    QFileInfo fi(path);
                    rel_path = fi.path();
                    if(rel_path == u".")
                        rel_path = u""_s;
                    else
                        rel_path = rel_path + u"/"_s;
                    setCurrentZipFileName(&zipEpub, path);
                    zipFile.open(QIODevice::ReadOnly);
                    QByteArray baOpf = zipFile.readAll();
                    zipFile.close();

                    QDomDocument opf;
                    QString sCoverImg;
                    opf.setContent(baOpf);
                    QDomNode meta = opf.documentElement().namedItem(u"metadata"_s);
                    for(int m=0; m<meta.childNodes().count(); m++)
                    {
                        if(meta.childNodes().at(m).nodeName().right(11) == u"description")
                        {
                            sAnnotation_ = meta.childNodes().at(m).toElement().text();
                        }
                        else if(meta.childNodes().at(m).nodeName().right(4) == u"meta")
                        {
                            auto itemMetadataAtributes = meta.childNodes().at(m).attributes();
                            if(itemMetadataAtributes.namedItem(u"name"_s).toAttr().value() == u"cover")
                            {
                                QString cover = itemMetadataAtributes.namedItem(u"content"_s).toAttr().value();
                                QDomNode manifest = opf.documentElement().namedItem(u"manifest"_s);
                                for(int man=0; man<manifest.childNodes().count(); man++)
                                {
                                    auto itemManifestAtributes = manifest.childNodes().at(man).attributes();
                                    if(itemManifestAtributes.namedItem(u"id"_s).toAttr().value() == cover)
                                    {
                                        sCoverImg = rel_path +itemManifestAtributes.namedItem(u"href"_s).toAttr().value();
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    if(sCoverImg.isEmpty()){
                        QDomNode manifest = opf.documentElement().namedItem(u"manifest"_s);
                        for(int m=0; m<manifest.childNodes().count(); m++)
                        {
                            auto itemAtribites = manifest.childNodes().at(m).attributes();
                            if(itemAtribites.namedItem(u"id"_s).toAttr().value().startsWith(u"cover"_s) && itemAtribites.namedItem(u"media-type"_s).toAttr().value().startsWith(u"image/"_s))
                            {
                                sCoverImg = rel_path +itemAtribites.namedItem(u"href"_s).toAttr().value();
                                break;
                            }

                        }
                    }
                    if(!sCoverImg.isEmpty()){
                        sCoverImg = sCoverImg.replace(u"%40"_s, u"@"_s);
                        setCurrentZipFileName(&zipEpub, sCoverImg);
                        if(!zipFile.open(QIODevice::ReadOnly)) [[unlikely]]
                            MyDBG << "Error open file: " << sCoverImg;
                        else{
                            QByteArray ba = zipFile.readAll();
                            zipFile.close();
                            img = QImage::fromData(ba);
                        }
                    }
                    need_loop = false;
                }
            }
        }
    }
    zipEpub.close();
    if(sAnnotation_.isEmpty()) [[unlikely]]
        sAnnotation_  = u" "_s;

    return img;
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

        if(djvu.openDocument(sDjVuFile)){
            img = djvu.getCover();
        }
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
            QBuffer buffer;
            buffer.setData(data);
            QuaZip zipEpub(&buffer);
            return coverAnnotationEpub(zipEpub);
        }
    }

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
        if(sFormat == u"fb2"_s)
            annotationFb2();
        else if(sFormat == u"epub")
            coverAnnotationEpub();
        else if(sFormat.endsWith(u"zip"))
            annotationZip();

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
            QBuffer buffer;
            buffer.setData(data);
            QuaZip zipEpub(&buffer);
            coverAnnotationEpub(zipEpub);
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
            MyDBG << "Error open archive! " << sArchive;
            return data;
        }
        zipFile.setZip(&uz);
        setCurrentZipFileName(&uz, sZipChain[i]);
        if(!zipFile.open(QIODevice::ReadOnly)) [[unlikely]]
        {
            MyDBG << "Error open file: " << sZipChain[i];
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
    if(zipFile.open(QIODevice::ReadOnly)){
        data = zipFile.readAll();
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
