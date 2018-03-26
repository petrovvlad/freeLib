/****************************************************************************
** Meta object code from reading C++ file 'smtpclient.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.9.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../src/SmtpClient/smtpclient.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'smtpclient.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.9.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_SmtpClient_t {
    QByteArrayData data[12];
    char stringdata0[170];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_SmtpClient_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_SmtpClient_t qt_meta_stringdata_SmtpClient = {
    {
QT_MOC_LITERAL(0, 0, 10), // "SmtpClient"
QT_MOC_LITERAL(1, 11, 18), // "socketStateChanged"
QT_MOC_LITERAL(2, 30, 0), // ""
QT_MOC_LITERAL(3, 31, 28), // "QAbstractSocket::SocketState"
QT_MOC_LITERAL(4, 60, 5), // "state"
QT_MOC_LITERAL(5, 66, 11), // "socketError"
QT_MOC_LITERAL(6, 78, 28), // "QAbstractSocket::SocketError"
QT_MOC_LITERAL(7, 107, 5), // "error"
QT_MOC_LITERAL(8, 113, 15), // "socketReadyRead"
QT_MOC_LITERAL(9, 129, 15), // "sslErrorHandler"
QT_MOC_LITERAL(10, 145, 16), // "QList<QSslError>"
QT_MOC_LITERAL(11, 162, 7) // "errlist"

    },
    "SmtpClient\0socketStateChanged\0\0"
    "QAbstractSocket::SocketState\0state\0"
    "socketError\0QAbstractSocket::SocketError\0"
    "error\0socketReadyRead\0sslErrorHandler\0"
    "QList<QSslError>\0errlist"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SmtpClient[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,   34,    2, 0x09 /* Protected */,
       5,    1,   37,    2, 0x09 /* Protected */,
       8,    0,   40,    2, 0x09 /* Protected */,
       9,    1,   41,    2, 0x09 /* Protected */,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 6,    7,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 10,   11,

       0        // eod
};

void SmtpClient::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        SmtpClient *_t = static_cast<SmtpClient *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->socketStateChanged((*reinterpret_cast< QAbstractSocket::SocketState(*)>(_a[1]))); break;
        case 1: _t->socketError((*reinterpret_cast< QAbstractSocket::SocketError(*)>(_a[1]))); break;
        case 2: _t->socketReadyRead(); break;
        case 3: _t->sslErrorHandler((*reinterpret_cast< QList<QSslError>(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QAbstractSocket::SocketState >(); break;
            }
            break;
        case 1:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QAbstractSocket::SocketError >(); break;
            }
            break;
        case 3:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QList<QSslError> >(); break;
            }
            break;
        }
    }
}

const QMetaObject SmtpClient::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_SmtpClient.data,
      qt_meta_data_SmtpClient,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *SmtpClient::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SmtpClient::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SmtpClient.stringdata0))
        return static_cast<void*>(const_cast< SmtpClient*>(this));
    return QObject::qt_metacast(_clname);
}

int SmtpClient::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
