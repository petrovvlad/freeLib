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

#include <quazip/quazip.h>
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

    explicit EpubReader(QByteArray data/*, QString fileName = QString()*/);
    explicit EpubReader(QIODevice *device/*, QString fileName = QString()*/);

    bool readMetadata(BookMetadata &metadata) const;
    QImage readCover() const;
    QString readAnnotation() const;
    bool readAnnotationAndCover(QString &annotation, QImage &cover) const;
    QString normalizePath(const QString &path, const QString &basePath) const;
    QDomNode findElementWithOptionalNamespace(const QDomNode &parent, const QString &name) const;

private:
    bool initializeZip() const;
    bool readContainer(QDomDocument &doc) const;
    bool findOpfPath(const QDomDocument &container, QString &opfPath) const;
    bool readOpf(const QString &opfPath, QDomDocument &opf) const;
    bool parseMetadata(const QDomDocument &opf, BookMetadata &metadata) const;
    QImage parseCover(const QDomDocument &opf, const QString &relPath) const;
    QString parseAnnotation(const QDomDocument &opf) const;
    QString extractImageFromHtml(const QString &htmlPath, const QString &relPath) const;

    QByteArray data_;
    QBuffer buffer_;
    QIODevice *device_;
    // QString fileName_;
    mutable QuaZip zip_;
};

#endif // EPUBREADER_H
