/****************************************************************************
** Meta object code from reading C++ file 'addlibrary.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.9.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../src/addlibrary.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'addlibrary.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.9.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_AddLibrary_t {
    QByteArrayData data[17];
    char stringdata0[194];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_AddLibrary_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_AddLibrary_t qt_meta_stringdata_AddLibrary = {
    {
QT_MOC_LITERAL(0, 0, 10), // "AddLibrary"
QT_MOC_LITERAL(1, 11, 12), // "break_import"
QT_MOC_LITERAL(2, 24, 0), // ""
QT_MOC_LITERAL(3, 25, 10), // "LogMessage"
QT_MOC_LITERAL(4, 36, 3), // "msg"
QT_MOC_LITERAL(5, 40, 9), // "InputINPX"
QT_MOC_LITERAL(6, 50, 14), // "SelectBooksDir"
QT_MOC_LITERAL(7, 65, 11), // "StartImport"
QT_MOC_LITERAL(8, 77, 13), // "SelectLibrary"
QT_MOC_LITERAL(9, 91, 13), // "UpdateLibList"
QT_MOC_LITERAL(10, 105, 13), // "DeleteLibrary"
QT_MOC_LITERAL(11, 119, 11), // "Add_Library"
QT_MOC_LITERAL(12, 131, 9), // "EndUpdate"
QT_MOC_LITERAL(13, 141, 15), // "terminateImport"
QT_MOC_LITERAL(14, 157, 6), // "reject"
QT_MOC_LITERAL(15, 164, 19), // "ExistingLibsChanged"
QT_MOC_LITERAL(16, 184, 9) // "ExportLib"

    },
    "AddLibrary\0break_import\0\0LogMessage\0"
    "msg\0InputINPX\0SelectBooksDir\0StartImport\0"
    "SelectLibrary\0UpdateLibList\0DeleteLibrary\0"
    "Add_Library\0EndUpdate\0terminateImport\0"
    "reject\0ExistingLibsChanged\0ExportLib"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_AddLibrary[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
      14,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   84,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       3,    1,   85,    2, 0x08 /* Private */,
       5,    0,   88,    2, 0x08 /* Private */,
       6,    0,   89,    2, 0x08 /* Private */,
       7,    0,   90,    2, 0x08 /* Private */,
       8,    0,   91,    2, 0x08 /* Private */,
       9,    0,   92,    2, 0x08 /* Private */,
      10,    0,   93,    2, 0x08 /* Private */,
      11,    0,   94,    2, 0x08 /* Private */,
      12,    0,   95,    2, 0x08 /* Private */,
      13,    0,   96,    2, 0x08 /* Private */,
      14,    0,   97,    2, 0x08 /* Private */,
      15,    0,   98,    2, 0x08 /* Private */,
      16,    0,   99,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void AddLibrary::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        AddLibrary *_t = static_cast<AddLibrary *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->break_import(); break;
        case 1: _t->LogMessage((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 2: _t->InputINPX(); break;
        case 3: _t->SelectBooksDir(); break;
        case 4: _t->StartImport(); break;
        case 5: _t->SelectLibrary(); break;
        case 6: _t->UpdateLibList(); break;
        case 7: _t->DeleteLibrary(); break;
        case 8: _t->Add_Library(); break;
        case 9: _t->EndUpdate(); break;
        case 10: _t->terminateImport(); break;
        case 11: _t->reject(); break;
        case 12: _t->ExistingLibsChanged(); break;
        case 13: _t->ExportLib(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (AddLibrary::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&AddLibrary::break_import)) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject AddLibrary::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_AddLibrary.data,
      qt_meta_data_AddLibrary,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *AddLibrary::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AddLibrary::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_AddLibrary.stringdata0))
        return static_cast<void*>(const_cast< AddLibrary*>(this));
    return QDialog::qt_metacast(_clname);
}

int AddLibrary::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 14)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 14;
    }
    return _id;
}

// SIGNAL 0
void AddLibrary::break_import()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
