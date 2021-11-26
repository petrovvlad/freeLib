#include "mobiedit.h"
#include <QDebug>
#include <QFile>
#include <QUuid>

#define MOBI_VERSION    36
#define MOBI_HEADER_BASE    16
#define MOBI_HEADER_LENGTH  20
#define UNIQUE_ID_SEED  68
#define NUMBER_OF_PDB_RECORDS   76
#define FIRST_PDB_RECORD    78
#define FIRST_IMAGE_RECORD  108
#define LAST_CONTENT_INDEX  194
#define TITLE_OFFSET    84
#define KF8_LAST_CONTENT_INDEX  192
#define fcis_index  200
#define flis_index  208
#define datp_index  256
#define hufftbloff  120

#define srcs_index  224
#define srcs_count  228

quint32 GetInt32(QByteArray &ba,int offset=0)
{
    return (quint8(ba[offset+0])*256+quint8(ba[offset+1]))*256*256+quint8(ba[offset+2])*256+quint8(ba[offset+3]);
}

quint16 GetInt16(QByteArray &ba,int offset=0)
{
    return quint8(ba[offset+0])*256+quint8(ba[offset+1]);
}

quint8 GetInt8(QByteArray &ba,int offset=0)
{
    return ba[offset];
}

QByteArray Int32ToBa(quint32 x)
{
    QByteArray result;
    result.append(((quint8*)(&x))[3]).append(((quint8*)(&x))[2]).append(((quint8*)(&x))[1]).append(((quint8*)(&x))[0]);
    return result;
}
QByteArray Int16ToBa(quint16 x)
{
    QByteArray result;
    result.append(((quint8*)(&x))[1]).append(((quint8*)(&x))[0]);
    return result;
}

mobiEdit::mobiEdit(QString filename):filename(filename)
{
    QFile file(filename);
    if(!file.open(QFile::ReadOnly))
    {
        qDebug()<<"error open mobi file "<<filename;
        return;
    }
    file_data=file.readAll();
    file.close();
/*
    palmheader=file_data.left(FIRST_PDB_RECORD);
    palmname=QString::fromUtf8(palmheader.left(32));
    ident=QString::fromUtf8(palmheader.mid(0x3C,8));
    num_section=GetInt16(file_data,NUMBER_OF_PDB_RECORDS);
    filelenght=file_data.size();
    for(int i=0;i<num_section;i++)
    {
        QByteArray sectionsdata=file_data.mid(i*8+palmheader.size(),8);
        sections<<section_mobi(GetInt32(sectionsdata),
                               GetInt8(sectionsdata,4),
                               GetInt8(sectionsdata,7));
    }
    sections<<section_mobi(filelenght,0,0);
    sections.last().description="File length only";
    for(int i=0;i<num_section;i++)
        sections[i].length=sections[i+1].offset-sections[i].offset;
        */
    //dumpsectionsinfo();
}

mobiEdit::~mobiEdit()
{

}

void mobiEdit::GetExthParams(QByteArray &rec0,quint32 &ebase,quint32 &elen,quint32 &ecount)
{
    ebase=MOBI_HEADER_BASE+GetInt32(rec0,MOBI_HEADER_LENGTH);
    elen=GetInt32(rec0,ebase+4);
    ecount=GetInt32(rec0,ebase+8);
}

QList<QByteArray> mobiEdit::ReadExth(QByteArray &rec0, quint32 exth_num)
{
    quint32 ebase;
    quint32 elen;
    quint32 ecount;
    GetExthParams(rec0,ebase,elen,ecount);
    QList<QByteArray> val;
    ebase+=12;
    while(ecount>0)
    {
        quint32 exth_id=GetInt32(rec0,ebase);
        if(exth_id==exth_num)
        {
            val<<rec0.mid(ebase+8,GetInt32(rec0,ebase+4)-8);
        }
        ecount--;
        ebase=ebase+GetInt32(rec0,ebase+4);
    }
    return val;
}

QByteArray mobiEdit::DeleteSectionRange(QByteArray &data, quint32 first, quint32 last)
{
    QByteArray dataout;
    quint16 nsec=GetInt16(data,NUMBER_OF_PDB_RECORDS);
    dataout.append(data.left(UNIQUE_ID_SEED));
    dataout.append(Int32ToBa(2*(nsec-(last-first+1))+1));
    dataout.append(data.mid(UNIQUE_ID_SEED+4,4));
    dataout.append(Int16ToBa(nsec-(last-first+1)));
    for(quint32 i=0;i<first;i++)
    {
        quint32 ofs=GetInt32(data,FIRST_PDB_RECORD+i*8);
        dataout.append(Int32ToBa(ofs-8*(last-first+1)));
        dataout.append(data.mid(FIRST_PDB_RECORD+4+i*8,4));
    }

    quint32 firstsecstart;
    quint32 firstsecend;
    GetSecAddr(data, first,firstsecstart,firstsecend);
    quint32 lastsecstart;
    quint32 lastsecend;
    GetSecAddr(data, last,lastsecstart,lastsecend);
    quint32 dif=lastsecend-firstsecstart+8*(last-first+1);
    for(int i=last+1;i<nsec;i++)
    {
        quint32 ofs=GetInt32(data,FIRST_PDB_RECORD+i*8);
        dataout.append(Int32ToBa(ofs-dif));
        dataout.append(data.mid(FIRST_PDB_RECORD+4+i*8,4));
    }
    quint32 zerrosecstart;
    quint32 zerrosecend;
    GetSecAddr(data, 0,zerrosecstart,zerrosecend);
    qint32 newstart=zerrosecstart-8*(last-first+1);
    quint32 lpad=newstart-(FIRST_PDB_RECORD+8*(nsec-(last-first+1)));
    if(lpad>0)
        dataout.append(QByteArray(lpad,'\0'));
    dataout.append(data.mid(zerrosecstart,firstsecstart-zerrosecstart));
    dataout.append(data.mid(lastsecend));
    return dataout;
}

QByteArray mobiEdit::IntsertSectionRange(QByteArray &data, quint32 first, quint32 last,QByteArray &sectiontarget,quint32 targetsec)
{
    QByteArray dataout=sectiontarget;
    for(int idx=(int)last;idx>=(int)first;idx--)
    {
        dataout=IntsertSection(dataout,targetsec,ReadSection(data,idx));
    }
    return dataout;
}
QByteArray mobiEdit::ReadSection(QByteArray &data, quint32 secno)
{
    quint32 secstart;
    quint32 secend;
    GetSecAddr(data, secno,secstart,secend);
    return data.mid(secstart,secend-secstart);
}

void mobiEdit::GetSecAddr(QByteArray &data, quint32 secno,quint32 &secstart,quint32 &secend)
{
    quint16 nsec=GetInt16(data,NUMBER_OF_PDB_RECORDS);
    secstart=GetInt32(data,FIRST_PDB_RECORD+secno*8);
    if(nsec-1==secno)
        secend=data.length();
    else
        secend=GetInt32(data,FIRST_PDB_RECORD+(secno+1)*8);
}

QByteArray mobiEdit::IntsertSection(QByteArray &data, quint32 secno, QByteArray secdata)
{
    quint16 nsec=GetInt16(data,NUMBER_OF_PDB_RECORDS);
    quint32 secstart;
    quint32 secend;
    GetSecAddr(data, secno,secstart,secend);
    quint32 zerrosecstart;
    quint32 zerrosecend;
    GetSecAddr(data, 0,zerrosecstart,zerrosecend);
    quint32 dif=secdata.size();
    QByteArray dataout;
    dataout.append(data.left(UNIQUE_ID_SEED));
    dataout.append(Int32ToBa(2*(nsec+1)+1));
    dataout.append(data.mid(UNIQUE_ID_SEED+4,4));
    dataout.append(Int16ToBa(nsec+1));
    quint32 newstart=zerrosecstart+8;
    for(quint32 i=0;i<secno;i++)
    {
        quint32 ofs=GetInt32(data,FIRST_PDB_RECORD+i*8)+8;
        dataout.append(Int32ToBa(ofs));
        dataout.append(data.mid(FIRST_PDB_RECORD+4+i*8,4));
    }
    dataout.append(Int32ToBa(secstart+8));
    dataout.append(Int32ToBa(2*secno));
    for(quint32 i=secno;i<nsec;i++)
    {
        quint32 ofs=GetInt32(data,FIRST_PDB_RECORD+i*8)+8+dif;
        dataout.append(Int32ToBa(ofs));
        dataout.append(Int32ToBa(2*(i+1)));
    }
    quint32 lpad=newstart-(FIRST_PDB_RECORD+8*(nsec+1));
    if(lpad>0)
        dataout.append(QByteArray(lpad,'\0'));
    dataout.append(data.mid(zerrosecstart,secstart-zerrosecstart));
    dataout.append(secdata);
    dataout.append(data.mid(secstart));
    return dataout;
}

QByteArray mobiEdit::WriteInt32(QByteArray &data, quint32 ofs, quint32 n)
{
    return data.left(ofs).append(Int32ToBa(n)).append(data.mid(ofs+4));
}
QByteArray mobiEdit::WriteInt16(QByteArray &data, quint32 ofs, quint16 n)
{
    return data.left(ofs).append(Int16ToBa(n)).append(data.mid(ofs+2));
}


QByteArray mobiEdit::WriteExth(QByteArray &data, quint32 exth_num, QByteArray exth_data)
{
    quint32 ebase;
    quint32 elen;
    quint32 ecount;
    GetExthParams(data,ebase,elen,ecount);
    quint32 ebase_idx=ebase+12;
    quint32 enum_idx=ecount;
    while(enum_idx>0)
    {
        quint32 exth_id=GetInt32(data,ebase_idx);
        if(exth_id==exth_num)
        {
            quint32 dif=exth_data.size()+8-GetInt32(data,ebase_idx+4);
            QByteArray newrec=data;
            if(dif!=0)
            {
                newrec=WriteInt32(newrec,TITLE_OFFSET,GetInt32(newrec,TITLE_OFFSET)+dif);
            }
            return newrec.left(ebase+4).append(Int32ToBa(elen+exth_data.size()+8-GetInt32(data,ebase_idx+4))).
                    append(Int32ToBa(ecount)).append(data.mid(ebase+12,ebase_idx+4-(ebase+12))).
                    append(Int32ToBa(exth_data.size()+8)).append(exth_data).
                    append(data.mid(ebase_idx+GetInt32(data,ebase_idx+4)));
        }
        enum_idx--;
        ebase_idx+=GetInt32(data,ebase_idx+4);
    }
    return data;
}

QByteArray mobiEdit::DelExth(QByteArray &data, quint32 exth_num)
{
    quint32 ebase;
    quint32 elen;
    quint32 ecount;
    GetExthParams(data,ebase,elen,ecount);
    quint32 ebase_idx=ebase+12;
    quint32 enum_idx=0;
    while(enum_idx<ecount)
    {
        quint32 exth_id=GetInt32(data,ebase_idx);
        quint32 exth_size=GetInt32(data,ebase_idx+4);
        if(exth_id==exth_num)
        {
            QByteArray newrec=data;
            //quint32 size=newrec.size();
            newrec=WriteInt32(newrec,TITLE_OFFSET,GetInt32(newrec,TITLE_OFFSET)-exth_size);
            newrec=newrec.left(ebase_idx).append(newrec.mid(ebase_idx+exth_size));
            newrec=newrec.left(ebase+4).append(Int32ToBa(elen-exth_size)).append(Int32ToBa(ecount-1)).append(newrec.mid(ebase+12));
            //quint32 size1=newrec.size();
            return newrec;
        }
        enum_idx++;
        ebase_idx+=exth_size;
    }
    return data;
}

QByteArray mobiEdit::WriteSection(QByteArray &data, quint32 secno, QByteArray &secdata)
{
    QByteArray dataout=DeleteSectionRange(data,secno,secno);
    return IntsertSection(dataout,secno,secdata);
}
QByteArray mobiEdit::AddExth(QByteArray &data, quint32 exth_num, QByteArray exth_data)
{
    quint32 ebase;
    quint32 elen;
    quint32 ecount;
    GetExthParams(data,ebase,elen,ecount);
    quint32 newrecsize=8+exth_data.size();
    QByteArray newrec=data.left(ebase+4).append(Int32ToBa(elen+newrecsize)).append(Int32ToBa(ecount+1)).
            append(Int32ToBa(exth_num)).append(Int32ToBa(newrecsize)).append(exth_data).append(data.mid(ebase+12));
    return WriteInt32(newrec,TITLE_OFFSET,GetInt32(newrec,TITLE_OFFSET)+newrecsize);
}

QByteArray mobiEdit::nullsection(QByteArray &data, quint32 secno)
{
    QByteArray datalst;
    quint16 nsec=GetInt16(data,NUMBER_OF_PDB_RECORDS);
    quint32 secstart;
    quint32 secend;
    GetSecAddr(data, secno,secstart,secend);
    quint32 zerrosecstart;
    quint32 zerrosecend;
    GetSecAddr(data, 0,zerrosecstart,zerrosecend);
    quint32 dif=secend-secstart;
    datalst.append(data.left(FIRST_PDB_RECORD));
    for(quint32 i=0;i<secno+1;i++)
    {
        quint32 ofs=GetInt32(data,FIRST_PDB_RECORD+i*8);
        datalst.append(Int32ToBa(ofs));
        datalst.append(data.mid(FIRST_PDB_RECORD+i*8+4,4));
    }
    for(quint32 i=secno+1;i<nsec;i++)
    {
        quint32 ofs=GetInt32(data,FIRST_PDB_RECORD+i*8);
        ofs=ofs-dif;
        datalst.append(Int32ToBa(ofs));
        datalst.append(data.mid(FIRST_PDB_RECORD+i*8+4,4));
    }
    quint32 lpad=zerrosecstart-(FIRST_PDB_RECORD+8*(nsec));
    if(lpad>0)
        datalst.append(QByteArray(lpad,0));
    datalst.append(data.mid(zerrosecstart,secstart-zerrosecstart));
    datalst.append(data.mid(secend));
    return datalst;
}

bool mobiEdit::SaveMOBI7(QString mobi_file,bool removePersonal,bool repairCover)
{
    QByteArray datain_rec0=ReadSection(file_data,0);
    qint32 ver=GetInt32(datain_rec0,MOBI_VERSION);
    if(ver==8)
        return false;
    //qDebug()<<ver;
    QList<QByteArray> exth121=ReadExth(datain_rec0,121);
    if(exth121.count()==0)
        return false;
    quint32 datain_kf8=GetInt32(exth121[0],0);
    if(datain_kf8==0xFFFFFFFF)
        return false;

    quint16 num_sec = GetInt16(file_data,NUMBER_OF_PDB_RECORDS);
    QByteArray result_file7 = DeleteSectionRange(file_data,datain_kf8-1,num_sec-2);

    quint32 srcs = GetInt32(datain_rec0,srcs_index);
    quint32 num_srcs = GetInt32(datain_rec0,srcs_count);
    if (srcs != 0xffffffff && num_srcs > 0)
    {
        result_file7 = DeleteSectionRange(result_file7,srcs,srcs+num_srcs-1);
        datain_rec0 = WriteInt32(datain_rec0,srcs_index,0xffffffff);
        datain_rec0 = WriteInt32(datain_rec0,srcs_count,0);
    }
    datain_rec0 = WriteExth(datain_rec0,121, Int32ToBa(0xffffffff));
    datain_rec0 = WriteExth(datain_rec0,129, "");

    quint32 fval=GetInt32(datain_rec0,0x80);
    //fval=fval&0x1FFF;
    fval=fval&0x07FF;
    datain_rec0=datain_rec0.left(0x80).append(Int32ToBa(fval)).append(datain_rec0.mid(0x84));

    if(repairCover)
    {
        QList<QByteArray> ex201=ReadExth(datain_rec0,201);
        if(ex201.count()>0)
        {
            datain_rec0=DelExth(datain_rec0,202);
            datain_rec0=AddExth(datain_rec0,202,ex201[0]);
        }
    }
    if(removePersonal)
    {
        datain_rec0=AddExth(datain_rec0,501,"EBOK");
    }

    result_file7 = WriteSection(result_file7,0,datain_rec0);

    quint32 firstimage=GetInt32(datain_rec0,FIRST_IMAGE_RECORD);
    quint32 lastimage=GetInt16(datain_rec0,LAST_CONTENT_INDEX);
    if (lastimage == 0xffff)
    {
        quint32 ofs_list[]={KF8_LAST_CONTENT_INDEX,fcis_index,flis_index,datp_index,hufftbloff};
        for(quint32 i=0;i<sizeof(ofs_list)/sizeof(quint32);i++)
        {
            quint32 n=GetInt32(datain_rec0,ofs_list[i]);
            if(n>0 && n<lastimage)
                lastimage=n-1;
        }
    }


    for(quint32 i=firstimage;i<lastimage;i++)
    {
        QByteArray imgsec=ReadSection(result_file7,i);
        QString type=QString::fromLatin1(imgsec.mid(0,4).data());
        if(type=="FONT" || type=="RESC")
        {
            result_file7=nullsection(result_file7,i);
        }
    }

    //qDebug()<<mobi_file<<" "<<QString::number(result_file7.size());

    QFile file(mobi_file);
    file.open(QFile::WriteOnly);
    file.write(result_file7);
    file.close();

    return true;

}

bool mobiEdit::SaveAZW(QString azw_file,bool removePersonal,bool repairCover)
{
    QByteArray datain_rec0=ReadSection(file_data,0);
    qint32 ver=GetInt32(datain_rec0,MOBI_VERSION);
    if(ver==8)
        return false;
    //qDebug()<<ver;
    QList<QByteArray> exth121=ReadExth(datain_rec0,121);
    if(exth121.count()==0)
        return false;
    quint32 datain_kf8=GetInt32(exth121[0],0);
    if(datain_kf8==0xFFFFFFFF)
        return false;
    // QByteArray result_file7=DeleteSectionRange(file_data,datain_kf8-1,num_section-2);
    QByteArray datain_kfrec0=ReadSection(file_data,datain_kf8);
    qint32 firstimage=GetInt32(datain_rec0,FIRST_IMAGE_RECORD);
    qint32 lastimage=GetInt16(datain_rec0,LAST_CONTENT_INDEX);

    QByteArray result_file8=DeleteSectionRange(file_data,0,datain_kf8-1);
    //qDebug()<<result_file8.size();
    quint32 target=GetInt32(datain_kfrec0,FIRST_IMAGE_RECORD);
    result_file8=IntsertSectionRange(file_data,firstimage,lastimage,result_file8,target);
    //qDebug()<<result_file8.size()<<firstimage<<lastimage<<target;
    datain_kfrec0=ReadSection(result_file8,0);
    QList<QByteArray> kf8starts=ReadExth(datain_kfrec0,116);
    int kf8start_count=kf8starts.count();
    while(kf8start_count>1)
    {
        kf8start_count--;
        datain_kfrec0=DelExth(datain_kfrec0,116);
        //qDebug()<<"sss";
    }
    datain_kfrec0=WriteExth(datain_kfrec0,125,Int32ToBa(lastimage-firstimage+1));
    quint32 fval=GetInt32(datain_kfrec0,0x80);
    fval=fval&0x1FFF;
    fval|=0x0800;
    datain_kfrec0=datain_kfrec0.left(0x80).append(Int32ToBa(fval)).append(datain_kfrec0.mid(0x84));

    quint32 ofs_list[]={KF8_LAST_CONTENT_INDEX,fcis_index,flis_index,datp_index,hufftbloff};
    for(quint32 i=0;i<sizeof(ofs_list)/sizeof(quint32);i++)
    {
        quint32 n=GetInt32(datain_kfrec0,ofs_list[i]);
        if(n!=0xFFFFFFFF)
            datain_kfrec0=WriteInt32(datain_kfrec0,ofs_list[i],n+lastimage-firstimage+1);

    }
    if(repairCover)
    {
        QList<QByteArray> ex201=ReadExth(datain_kfrec0,201);
        if(ex201.count()>0)
        {
            datain_kfrec0=DelExth(datain_kfrec0,202);
            datain_kfrec0=AddExth(datain_kfrec0,202,ex201[0]);
        }
    }
    if(removePersonal)
    {
        datain_kfrec0=AddExth(datain_kfrec0,501,"EBOK");
    }
    result_file8=WriteSection(result_file8,0,datain_kfrec0);
    QFile file(azw_file);
    file.open(QFile::WriteOnly);
    file.write(result_file8);
    file.close();
    //qDebug()<<"ok";
    return true;
}

bool mobiEdit::AddExthToMobi(quint32 exth_num, QByteArray exth_data)
{
    QByteArray uuid=QUuid::createUuid().toString().toLocal8Bit();
    QByteArray datain_rec0=ReadSection(file_data,0);
    datain_rec0=AddExth(datain_rec0,501,"EBOK");
    datain_rec0=AddExth(datain_rec0,113,uuid);
    datain_rec0=AddExth(datain_rec0,504,uuid);
    QByteArray result_file8=WriteSection(file_data,0,datain_rec0);


    datain_rec0=ReadSection(file_data,0);
    QList<QByteArray> exth121=ReadExth(datain_rec0,121);
    if(exth121.count()==0)
        return false;
    quint32 datain_kf8=GetInt32(exth121[0],0);
    if(datain_kf8==0xFFFFFFFF)
        return false;
    QByteArray datain_kfrec0=ReadSection(result_file8,datain_kf8);
    datain_kfrec0=AddExth(datain_kfrec0,501,"EBOK");
    datain_rec0=AddExth(datain_kfrec0,113,uuid);
    datain_rec0=AddExth(datain_kfrec0,504,uuid);
    result_file8=WriteSection(result_file8,datain_kf8,datain_kfrec0);

    QFile file(filename+"501.mobi");
    file.open(QFile::WriteOnly);
    file.write(result_file8);
    file.close();
    return true;
}

/*
QByteArray mobiEdit::GetSection(int number)
{
    return file_data.mid(sections[number].offset,sections[number+1].offset-sections[number].offset);
}

void mobiEdit::dumpsectionsinfo()
{
    int i=0;
    qDebug()<<QString("Section Offset      Lenght      UID Attribs Description");
    foreach (section_mobi section, sections)
    {
        qDebug()<<QString("%1 %2 %3    %4 %5 %6").arg(QString::number(i),3).
                    arg(QString::number(section.offset,16),10).
                    arg((i+1)<sections.count()?QString::number(section.length,16):"",10).
                    arg(QString::number(section.uid),5).
                    arg(QString::number(section.attribute),5).
                    arg(section.description,-40);
        i++;
    }
}

*/
