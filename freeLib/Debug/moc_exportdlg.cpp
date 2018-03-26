/****************************************************************************
** Meta object code from reading C++ file 'exportdlg.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.9.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../src/exportdlg.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'exportdlg.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.9.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ExportDlg_t {
    QByteArrayData data[9];
    char stringdata0[72];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ExportDlg_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ExportDlg_t qt_meta_stringdata_ExportDlg = {
    {
QT_MOC_LITERAL(0, 0, 9), // "ExportDlg"
QT_MOC_LITERAL(1, 10, 9), // "break_exp"
QT_MOC_LITERAL(2, 20, 0), // ""
QT_MOC_LITERAL(3, 21, 9), // "EndExport"
QT_MOC_LITERAL(4, 31, 11), // "BreakExport"
QT_MOC_LITERAL(5, 43, 7), // "Process"
QT_MOC_LITERAL(6, 51, 7), // "Procent"
QT_MOC_LITERAL(7, 59, 5), // "count"
QT_MOC_LITERAL(8, 65, 6) // "reject"

    },
    "ExportDlg\0break_exp\0\0EndExport\0"
    "BreakExport\0Process\0Procent\0count\0"
    "reject"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ExportDlg[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   39,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       3,    0,   40,    2, 0x08 /* Private */,
       4,    0,   41,    2, 0x08 /* Private */,
       5,    2,   42,    2, 0x08 /* Private */,
       8,    0,   47,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,    6,    7,
    QMetaType::Void,

       0        // eod
};

void ExportDlg::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        ExportDlg *_t = static_cast<ExportDlg *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->break_exp(); break;
        case 1: _t->EndExport(); break;
        case 2: _t->BreakExport(); break;
        case 3: _t->Process((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 4: _t->reject(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (ExportDlg::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&ExportDlg::break_exp)) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject ExportDlg::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_ExportDlg.data,
      qt_meta_data_ExportDlg,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *ExportDlg::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ExportDlg::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ExportDlg.stringdata0))
        return static_cast<void*>(const_cast< ExportDlg*>(this));
    return QDialog::qt_metacast(_clname);
}

int ExportDlg::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void ExportDlg::break_exp()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
