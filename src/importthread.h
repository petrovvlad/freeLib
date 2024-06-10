#ifndef IMPORTTHREAD_H
#define IMPORTTHREAD_H

#include <QSqlDatabase>
#include <QSqlQuery>

#include "library.h"

//способы обновления
#define UT_FULL 10
#define UT_DEL_AND_NEW 11
#define UT_NEW 12


class ImportThread : public QObject
{
    Q_OBJECT
public:
    explicit ImportThread(QObject *parent = 0);
    void init(uint id, const SLib &lib, uchar nUpdateType);
    void init(uint id, const SLib &lib, const QStringList &files);
signals:
    void progress(uint nAddedBooks, float fProgress);
    void End();
public slots:
    void process();
    void break_import();
protected:
    void importBooksFromPath(const QString &sPath);
    void importBooksFromList(const QFileInfoList &listFiles);
    void importBooksFromZip(const QString &sPath, const QString &sArchName, uint &nBooksCount);

    void readFB2(const QByteArray &ba, QString file_name, QString arh_name,qint32 file_size=0);
    void readEPUB(const QByteArray &ba, QString file_name, QString arh_name,qint32 file_size=0);
    void readFB2_test(const QByteArray& ba, QString file_name, QString arh_name);
private:
    QString sInpxFile_;
    QString sName_;
    QString sPath_;
    uint  idLib_;
    uchar nUpdateType_;
    bool bFirstAuthorOnly_;
    bool bWoDeleted_;
    std::atomic_bool stopped_;
    std::vector<uint> vIdBookInLib_;
    QStringList listFiles_;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    std::unordered_map<SAuthor, uint> hashAuthors_;
#endif
    QSqlQuery query_;
    QSqlQuery queryInsertBook_;
    QSqlQuery queryInsertAuthor_;
    QSqlQuery queryInsertBookAuthor_;
    QSqlQuery queryInsertBookGenre_;
    QSqlQuery queryInsertSeria_;

    std::unordered_map <QString, ushort> mGenreKeys_;
    uint AddSeria(const QString &str, qlonglong libID, const QVariantList *pTags = nullptr);
    uint addAuthor(const SAuthor &author, uint libID, uint idBook, bool bFirstAuthor, const QVariantList *pTags = nullptr);
    uint AddBook(qlonglong star, const QString &name, qlonglong id_seria, int num_in_seria, const QString &file,
                 int size, int IDinLib, bool deleted, const QString &format, QDate date, const QString &language, const QString &keys, qlonglong id_lib, const QString &archive, const QVariantList *pTags = nullptr);

    void AddGenre(uint idBook, const QString &sGenre, uint idLib);

};

#endif // IMPORTTHREAD_H
