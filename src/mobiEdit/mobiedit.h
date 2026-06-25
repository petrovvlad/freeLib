#ifndef MOBIEDIT_H
#define MOBIEDIT_H
#include <QString>
#include <QList>


struct section_mobi
{
    quint32 offset;
    quint8 attribute;
    QString description;
    quint32 uid;
    quint32 length;
    section_mobi(quint32 offset,quint8 attribute,quint32 uid):offset(offset),attribute(attribute),uid(uid)
    {
    }
};

class mobiEdit
{
public:
    mobiEdit(const QString &filename);
    ~mobiEdit();
    bool SaveAZW(const QString &azw_file, bool removePersonal, bool repairCover);
    bool SaveMOBI7(const QString &mobi_file, bool removePersonal, bool repairCover);
    // bool AddExthToMobi(quint32 exth_num/*, QByteArray exth_data*/);
    //void dumpsectionsinfo();
private:
    //QByteArray GetSection(int number);
    void GetExthParams(const QByteArray &rec0, quint32 &ebase, quint32 &elen, quint32 &ecount);
    QList<QByteArray> ReadExth(const QByteArray &rec0,quint32 exth_num);
    QByteArray nullsection(const QByteArray &data, quint32 secno);
    QByteArray DelExth(const QByteArray &data, quint32 exth_num);
    QByteArray AddExth(const QByteArray &data, quint32 exth_num, const QByteArray &exth_data);
    QByteArray DeleteSectionRange(const QByteArray &data, quint32 first, quint32 last);
    QByteArray IntsertSectionRange(const QByteArray &data, quint32 first, quint32 last, const QByteArray &sectiontarget, quint32 targetsec);
    QByteArray IntsertSection(const QByteArray &data, quint32 secno, const QByteArray &secdata);
    QByteArray WriteExth(const QByteArray &data, quint32 exth_num, const QByteArray &exth_data);
    QByteArray WriteInt32(const QByteArray &data, quint32 ofs, quint32 n);
    QByteArray WriteInt16(const QByteArray &data, quint32 ofs, quint16 n);
    void GetSecAddr(const QByteArray &data, quint32 secno,quint32 &secstart,quint32 &secend);
    QByteArray ReadSection(const QByteArray &data, quint32 secno);
    QByteArray WriteSection(const QByteArray &data, quint32 secno, const QByteArray &secdata);
    QByteArray file_data;
    QString filename;
    //QByteArray palmheader;
    //QString palmname;
    //QString ident;
    //qint16 num_section;
    //qint64 filelenght;
    //QList<section_mobi> sections;
};

#endif // MOBIEDIT_H
