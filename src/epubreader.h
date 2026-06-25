#ifndef EPUBREADER_H
#define EPUBREADER_H

#pragma once

#include <QByteArray>
#include <QImage>
#include <QString>
#include <QStringList>
#include <QDomDocument>
#include <QBuffer>
#include <vector>

#include "qarchive.h"
#include "library.h"

class EpubReader {
public:
    struct BookMetadata {
        QString title;
        QString language;
        std::vector<SAuthor> authors;
        std::vector<QString> genres;
        bool isValid() const { return !title.isEmpty() && !authors.empty(); }
    };

    explicit EpubReader(const QByteArray data);

    bool readMetadata(BookMetadata &metadata);
    QImage readCover();
    QString readAnnotation();
    QString normalizePath(const QString &path, const QString &basePath) const;
    QDomNode findElementWithOptionalNamespace(const QDomNode &parent, const QString &name) const;

private:
    bool openOpf();
    bool readContainer(QDomDocument &doc) const;
    bool findOpfPath(const QDomDocument &container, QString &opfPath) const;
    bool parseMetadata(BookMetadata &metadata) const;
    QImage parseCover() const;
    QString parseAnnotation() const;
    QString extractImageFromHtml(const QString &htmlPath, const QString &relPath) const;

    QByteArray data_;
    mutable QuaZip zip_;
    mutable QArchive arc_;
    QDomDocument opf_;
    QString opfPath_;
};

#endif // EPUBREADER_H
