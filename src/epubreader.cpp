#include "epubreader.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#ifdef QUAZIP_STATIC
#include "quazip/quazip/quazipfile.h"
#else
#include <quazip/quazipfile.h>
#endif


EpubReader::EpubReader(QByteArray data)
    : data_(std::move(data)), buffer_(&data_), device_(&buffer_)
{
}

EpubReader::EpubReader(QIODevice *device)
    : device_(device)
{
    Q_ASSERT(device_ != nullptr);
}

bool EpubReader::openOpf()
{
    if(!initializeZip()) [[unlikely]]
        return false;

    QDomDocument container;
    if(!readContainer(container)) [[unlikely]]
        return false;

    if(!findOpfPath(container, opfPath_)) [[unlikely]]
        return false;

    if(!setCurrentZipFileName(&zip_, opfPath_)) [[unlikely]]
    {
        LogWarning << "Failed to set OPF file:" << opfPath_;
        return false;
    }

    QuaZipFile zipFile(&zip_);
    if(!zipFile.open(QIODevice::ReadOnly)) [[unlikely]]
    {
        LogWarning << "Failed to open OPF file:" << opfPath_;
        return false;
    }

    if(!opf_.setContent(zipFile.readAll())) [[unlikely]]
    {
        LogWarning << "Failed to parse OPF file:" << opfPath_;
        return false;
    }

    return true;
}

bool EpubReader::initializeZip() const
{
    zip_.setIoDevice(device_);
    if (!zip_.open(QuaZip::mdUnzip)) [[unlikely]]
    {
        LogWarning << "Failed to open EPUB as ZIP";
        return false;
    }
    return true;
}

bool EpubReader::readContainer(QDomDocument &doc) const
{
    if (!setCurrentZipFileName(&zip_, u"META-INF/container.xml"_s)) [[unlikely]]
    {
        LogWarning << "No container.xml";
        return false;
    }

    QuaZipFile zipFile(&zip_);
    if (!zipFile.open(QIODevice::ReadOnly)) [[unlikely]]
    {
        LogWarning << "Failed to open META-INF/container.xml";
        return false;
    }

    if (!doc.setContent(zipFile.readAll())) [[unlikely]]
    {
        LogWarning << "Failed to parse container.xml" ;
        return false;
    }
    return true;
}

bool EpubReader::findOpfPath(const QDomDocument &container, QString &opfPath) const
{
    QDomNode root = container.documentElement();
    if (root.isNull()) [[unlikely]]
    {
        LogWarning << "Invalid container.xml";
        return false;
    }

    for (int i = 0; i < root.childNodes().count(); ++i) {
        QDomNode node = root.childNodes().at(i);
        if (node.isNull() || node.nodeName().toLower() != u"rootfiles") continue;

        QDomNode roots = node;
        for (int j = 0; j < roots.childNodes().count(); ++j) {
            QDomNode rootfile = roots.childNodes().at(j);
            if (rootfile.isNull() || rootfile.nodeName().toLower() != u"rootfile") continue;

            QDomNamedNodeMap attrs = rootfile.attributes();
            QDomNode pathAttr = attrs.namedItem(u"full-path"_s);
            if (!pathAttr.isNull()) {
                opfPath = pathAttr.toAttr().value();
                return true;
            }
        }
    }
    return false;
}

QDomNode EpubReader::findElementWithOptionalNamespace(const QDomNode &parent, const QString &name) const
{
    // URI для стандартных пространств имен EPUB
    static const QString opfNs = u"http://www.idpf.org/2007/opf"_s;
    static const QString dcNs = u"http://purl.org/dc/elements/1.1/"_s;

    if(parent.isNull())
        return QDomNode();

    QDomElement parentEl = parent.toElement();
    if (parentEl.isNull())
        return QDomNode();

    QDomNodeList children = parentEl.childNodes();
    for(int i = 0; i < children.count(); ++i) {
        QDomElement child = children.at(i).toElement();
        if (child.isNull())
            continue;

        // Проверяем имя элемента и пространство имен
        if(child.tagName() == name || child.localName() == name) {
            const QString nsUri = child.namespaceURI();
            if(nsUri.isEmpty() || nsUri == opfNs || nsUri == dcNs)
                return child;
        }
    }

    return QDomNode();
}

bool EpubReader::parseMetadata(BookMetadata &metadata) const
{
    metadata = BookMetadata();
    QDomNode meta = opf_.documentElement().namedItem(u"metadata"_s);
    if(meta.isNull()) [[unlikely]]
    {
        LogWarning << "No metadata in OPF";
        return false;
    }

    for(int i = 0; i < meta.childNodes().count(); ++i) {
        QDomNode node = meta.childNodes().at(i);
        if (node.isNull()) continue;
        QString nodeName = node.nodeName().toLower();

        if (nodeName.endsWith(u"title")) {
            metadata.title = node.toElement().text().trimmed();
        } else if (nodeName.endsWith(u"language")) {
            metadata.language = node.toElement().text().trimmed().left(2);
        } else if (nodeName.endsWith(u"creator")) {
            SAuthor author;
            QString creatorText = node.toElement().text().trimmed();
            if (!creatorText.isEmpty()) {
                QStringList names = creatorText.split(u" "_s, Qt::SkipEmptyParts);
                if(names.count() > 0) [[likely]]
                    author.sFirstName = names.at(0);
                if(names.count() > 1)
                    author.sMiddleName = names.at(1);
                if(names.count() > 2)
                    author.sLastName = names.at(2);
                metadata.authors.push_back(author);
            }
        } else if(nodeName.endsWith(u"subject")) {
            QString genre = node.toElement().text().trimmed();
            if (!genre.isEmpty()) {
                metadata.genres.push_back(genre);
            }
        }
    }

    if (!metadata.isValid())
        return false;
    return true;
}

QString EpubReader::normalizePath(const QString &path, const QString &basePath) const
{
    QString cleanPath = path;
    // Удаляем ./ в начале
    if (cleanPath.startsWith(u"./"))
        cleanPath.remove(0, 2);

    // Формируем путь, соединяя basePath и cleanPath
    QString combinedPath = basePath.isEmpty() ? cleanPath : (basePath + cleanPath);
    // Нормализуем путь, обрабатывая ../ и ./
    QString normalizedPath = QDir::cleanPath(combinedPath);
    // Удаляем ведущий /, если есть
    if(normalizedPath.startsWith(u'/'))
        normalizedPath.remove(0, 1);
    return normalizedPath;
}

QString EpubReader::extractImageFromHtml(const QString &htmlPath, const QString &relPath) const
{
    QString cleanHtmlPath = normalizePath(htmlPath, relPath);
    if(!setCurrentZipFileName(&zip_, cleanHtmlPath)) [[unlikely]]
    {
        LogWarning << "Failed to set HTML file:" << cleanHtmlPath;
        return QString();
    }

    QuaZipFile zipFile(&zip_);
    if(!zipFile.open(QIODevice::ReadOnly)) [[unlikely]]
    {
        LogWarning << "Failed to open HTML file:" << cleanHtmlPath;
        return QString();
    }

    QDomDocument xmlDoc;
    if(!xmlDoc.setContent(zipFile.readAll())) [[unlikely]]
    {
        LogWarning << "Failed to parse HTML file:" << cleanHtmlPath;
        return QString();
    }

    // Формируем базовый путь: relPath + директория htmlPath
    QString basePath = relPath;
    if(!basePath.endsWith(u'/'))
        basePath += u"/"_s;
    basePath += QFileInfo(htmlPath).path();
    if(!basePath.endsWith(u'/'))
        basePath += u"/"_s;

    // Поиск <img> в HTML
    QDomNodeList imgNodes = xmlDoc.elementsByTagName(u"img"_s);
    for(int i = 0; i < imgNodes.count(); ++i){
        QDomElement img = imgNodes.at(i).toElement();
        QString src = img.attribute(u"src"_s);
        if(!src.isEmpty())
            return normalizePath(src, basePath);
    }

    // Поиск <image> в SVG
    QDomNodeList imageNodes = xmlDoc.elementsByTagName(u"image"_s);
    for(int i = 0; i < imageNodes.count(); ++i){
        QDomElement image = imageNodes.at(i).toElement();
        QString src = image.attribute(u"xlink:href"_s, image.attribute(u"href"_s));
        if(!src.isEmpty())
            return normalizePath(src, basePath);
    }

    return QString();
}

QImage EpubReader::parseCover() const
{
    QFileInfo fi(opfPath_);
    QString relPath = fi.path();
    if(relPath == u'.')
        relPath = u""_s;
    else
        relPath += u"/"_s;

    QDomElement root = opf_.documentElement();
    QDomNode meta = findElementWithOptionalNamespace(root, u"metadata"_s);
    QDomNode manifest = findElementWithOptionalNamespace(root, u"manifest"_s);
    QDomNode guide = findElementWithOptionalNamespace(root, u"guide"_s);
    QDomNode spine = findElementWithOptionalNamespace(root, u"spine"_s);
    if(meta.isNull() || manifest.isNull())
        return QImage();

    QString coverPath;

    // 1. Поиск через <meta>, <opf:meta> или <ns0:meta> с name="cover"
    QString coverId;
    for(int i = 0; i < meta.childNodes().count(); ++i){
        QDomNode node = meta.childNodes().at(i);
        if (node.isNull())
            continue;
        QString nodeName = node.nodeName().toLower();
        if(nodeName == u"meta" || nodeName == u"opf:meta" || nodeName == u"ns0:meta"){
            QDomNamedNodeMap attrs = node.attributes();
            QDomNode nameAttr = attrs.namedItem(u"name"_s);
            if (!nameAttr.isNull() && nameAttr.toAttr().value() == u"cover") {
                coverId = attrs.namedItem(u"content"_s).toAttr().value();
                break;
            }
        }
    }

    if(!coverId.isEmpty()){
        for(int i = 0; i < manifest.childNodes().count(); ++i){
            QDomNode item = manifest.childNodes().at(i);
            if (item.isNull()) continue;
            QDomNamedNodeMap attrs = item.attributes();
            if(attrs.namedItem(u"id"_s).toAttr().value() == coverId){
                QString href = attrs.namedItem(u"href"_s).toAttr().value();
                coverPath = normalizePath(href, relPath);
                // Если это .xhtml или .html, извлекаем изображение
                if(coverPath.endsWith(u".xhtml", Qt::CaseInsensitive) || coverPath.endsWith(u".html", Qt::CaseInsensitive))
                    coverPath = extractImageFromHtml(coverPath, relPath);
                break;
            }
        }
    }

    // 2. Поиск через properties="cover-image" (EPUB 3)
    if(coverPath.isEmpty()){
        for(int i = 0; i < manifest.childNodes().count(); ++i){
            QDomNode item = manifest.childNodes().at(i);
            if (item.isNull()) continue;
            QDomNamedNodeMap attrs = item.attributes();
            QString properties = attrs.namedItem(u"properties"_s).toAttr().value();
            if(properties.contains(u"cover-image")){
                QString href = attrs.namedItem(u"href"_s).toAttr().value();
                coverPath = normalizePath(href, relPath);
                break;
            }
        }
    }

    // 3. Поиск через <guide> <reference type="cover">, затем type="title-page"
    if(coverPath.isEmpty() && !guide.isNull()){
        // Сначала ищем type="cover"
        for(int i = 0; i < guide.childNodes().count(); ++i){
            QDomNode ref = guide.childNodes().at(i);
            if (ref.isNull() || ref.nodeName().toLower() != u"reference"_s)
                continue;
            QDomNamedNodeMap attrs = ref.attributes();
            QString type = attrs.namedItem(u"type"_s).toAttr().value();
            if(type == u"cover") {
                QString href = attrs.namedItem(u"href"_s).toAttr().value();
                if(href.endsWith(u".xhtml", Qt::CaseInsensitive) || href.endsWith(u".html", Qt::CaseInsensitive))
                    coverPath = extractImageFromHtml(normalizePath(href, relPath), relPath);
                else
                    coverPath = normalizePath(href, relPath);
                break;
            }
        }
        // Если не нашли cover, ищем type="title-page"
        if(coverPath.isEmpty()){
            for(int i = 0; i < guide.childNodes().count(); ++i){
                QDomNode ref = guide.childNodes().at(i);
                if(ref.isNull() || ref.nodeName().toLower() != u"reference")
                    continue;
                QDomNamedNodeMap attrs = ref.attributes();
                QString type = attrs.namedItem(u"type"_s).toAttr().value();
                if(type == u"title-page"){
                    QString href = attrs.namedItem(u"href"_s).toAttr().value();
                    if (href.endsWith(u".xhtml", Qt::CaseInsensitive) || href.endsWith(u".html", Qt::CaseInsensitive))
                        coverPath = extractImageFromHtml(href, relPath);
                    else
                        coverPath = normalizePath(href, relPath);
                    break;
                }
            }
        }
    }

    // 4. Поиск через первый элемент <spine>
    if(coverPath.isEmpty() && !spine.isNull()) {
        for(int i = 0; i < spine.childNodes().count(); ++i) {
            QDomNode itemref = spine.childNodes().at(i);
            if(itemref.isNull() || itemref.nodeName().toLower() != u"itemref")
                continue;
            QString idref = itemref.attributes().namedItem(u"idref"_s).toAttr().value();

            for(int j = 0; j < manifest.childNodes().count(); ++j) {
                QDomNode item = manifest.childNodes().at(j);
                if (item.isNull()) continue;
                QDomNamedNodeMap attrs = item.attributes();
                if(attrs.namedItem(u"id"_s).toAttr().value() == idref){
                    QString href = attrs.namedItem(u"href"_s).toAttr().value();
                    if(href.endsWith(u".xhtml", Qt::CaseInsensitive) || href.endsWith(u".html", Qt::CaseInsensitive)) {
                        coverPath = extractImageFromHtml(href, relPath);
                    }
                    break;
                }
            }
            break; // Проверяем только первый itemref
        }
    }

    // 5. Резервный поиск по id, связанным с заголовком книги
    if(coverPath.isEmpty()){
        QString titlePrefix;
        for(int i = 0; i < meta.childNodes().count(); ++i){
            QDomNode node = meta.childNodes().at(i);
            if(node.nodeName().toLower().endsWith(u"title"_s)){
                titlePrefix = node.toElement().text().trimmed().toLower();
                // Извлекаем ключевые слова, например, "price", "paradise"
                QStringList words = titlePrefix.split(QRegularExpression(u"[^\\w]+"_s), Qt::SkipEmptyParts);
                titlePrefix = u""_s;
                if (!words.isEmpty()) {
                    // Выбираем первое слово длиной >= 4
                    for(const QString &word : std::as_const(words)){
                        if(word.length() >= 4){
                            titlePrefix = word;
                            break;
                        }
                    }
                }
                break;
            }
        }

        for(int i = 0; i < manifest.childNodes().count(); ++i){
            QDomNode item = manifest.childNodes().at(i);
            if (item.isNull()) continue;
            QDomNamedNodeMap attrs = item.attributes();
            QString id = attrs.namedItem(u"id"_s).toAttr().value().toLower();
            QString mediaType = attrs.namedItem(u"media-type"_s).toAttr().value();
            if((id.contains(u"cover"_s) || id.contains(u"titlepage") ||
                 id.startsWith(u"frontcover"_s) || id.startsWith(u"cover-image") ||
                 (!titlePrefix.isEmpty() && id.contains(titlePrefix))) &&
                (mediaType.startsWith(u"image/") || mediaType == u"application/xhtml+xml"_s))
            {
                QString href = attrs.namedItem(u"href"_s).toAttr().value();
                if(mediaType == u"application/xhtml+xml"_s){
                    coverPath = extractImageFromHtml(href, relPath);
                }else{
                    // Проверяем размер изображения
                    QString imgPath = normalizePath(href, relPath);
                    if(setCurrentZipFileName(&zip_, imgPath)){
                        QuaZipFile zipFile(&zip_);
                        if(zipFile.open(QIODevice::ReadOnly)){
                            QImage img = QImage::fromData(zipFile.readAll());
                            if(!img.isNull() && img.width() >= 200 && img.height() >= 200 && img.height() > img.width())
                                coverPath = imgPath;
                        }
                    }
                }
                if(!coverPath.isEmpty())
                    break;
            }
        }
    }


    if(coverPath.isEmpty())
        return QImage();

    coverPath.replace(u"%40"_s, u"@"_s);
    coverPath.replace(u"%20"_s, u" "_s);
    if(!setCurrentZipFileName(&zip_, coverPath)) [[unlikely]]
    {
        LogWarning << "Failed to set cover file:" << coverPath;
        return QImage();
    }

    QuaZipFile zipFile(&zip_);
    if(!zipFile.open(QIODevice::ReadOnly)) [[unlikely]]
    {
        LogWarning << "Failed to open cover file:" << coverPath;
        return QImage();
    }

    QImage cover = QImage::fromData(zipFile.readAll());
    if(cover.isNull()) [[unlikely]]
    {
        LogWarning << "Failed to load cover image from:" << coverPath;
        return QImage();
    }
    return cover;
}

QString EpubReader::parseAnnotation() const
{
    QDomNode meta = opf_.documentElement().namedItem(u"metadata"_s);
    if (meta.isNull())
        return u""_s;

    for (int i = 0; i < meta.childNodes().count(); ++i) {
        QDomNode node = meta.childNodes().at(i);
        if (node.isNull()) continue;
        QString nodeName = node.nodeName().toLower();

        if (nodeName.endsWith(u"description"_s))
            return node.toElement().text().trimmed();
    }

    return u""_s; // Аннотация отсутствует
}

bool EpubReader::readMetadata(BookMetadata &metadata)
{
    if(opf_.isNull())
        openOpf();
    return parseMetadata(metadata);
}

QImage EpubReader::readCover()
{
    if(opf_.isNull())
        openOpf();
    return parseCover();
}

QString EpubReader::readAnnotation()
{
    if(opf_.isNull())
        openOpf();
    return parseAnnotation();
}

