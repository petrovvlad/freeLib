/****************************************************************************
** Meta object code from reading C++ file 'fontframe.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.9.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../src/fontframe.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'fontframe.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.9.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_FontFrame_t {
    QByteArrayData data[16];
    char stringdata0[152];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_FontFrame_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_FontFrame_t qt_meta_stringdata_FontFrame = {
    {
QT_MOC_LITERAL(0, 0, 9), // "FontFrame"
QT_MOC_LITERAL(1, 10, 11), // "remove_font"
QT_MOC_LITERAL(2, 22, 0), // ""
QT_MOC_LITERAL(3, 23, 8), // "QWidget*"
QT_MOC_LITERAL(4, 32, 6), // "widget"
QT_MOC_LITERAL(5, 39, 9), // "move_font"
QT_MOC_LITERAL(6, 49, 9), // "direction"
QT_MOC_LITERAL(7, 59, 9), // "UseChange"
QT_MOC_LITERAL(8, 69, 5), // "state"
QT_MOC_LITERAL(9, 75, 8), // "DelPress"
QT_MOC_LITERAL(10, 84, 7), // "UpPress"
QT_MOC_LITERAL(11, 92, 9), // "DownPress"
QT_MOC_LITERAL(12, 102, 12), // "FontSelected"
QT_MOC_LITERAL(13, 115, 3), // "str"
QT_MOC_LITERAL(14, 119, 26), // "on_tag_currentIndexChanged"
QT_MOC_LITERAL(15, 146, 5) // "index"

    },
    "FontFrame\0remove_font\0\0QWidget*\0widget\0"
    "move_font\0direction\0UseChange\0state\0"
    "DelPress\0UpPress\0DownPress\0FontSelected\0"
    "str\0on_tag_currentIndexChanged\0index"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_FontFrame[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   54,    2, 0x06 /* Public */,
       5,    2,   57,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       7,    1,   62,    2, 0x08 /* Private */,
       9,    0,   65,    2, 0x08 /* Private */,
      10,    0,   66,    2, 0x08 /* Private */,
      11,    0,   67,    2, 0x08 /* Private */,
      12,    1,   68,    2, 0x08 /* Private */,
      14,    1,   71,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 3, QMetaType::Int,    4,    6,

 // slots: parameters
    QMetaType::Void, QMetaType::Bool,    8,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   13,
    QMetaType::Void, QMetaType::Int,   15,

       0        // eod
};

void FontFrame::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        FontFrame *_t = static_cast<FontFrame *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->remove_font((*reinterpret_cast< QWidget*(*)>(_a[1]))); break;
        case 1: _t->move_font((*reinterpret_cast< QWidget*(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 2: _t->UseChange((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 3: _t->DelPress(); break;
        case 4: _t->UpPress(); break;
        case 5: _t->DownPress(); break;
        case 6: _t->FontSelected((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 7: _t->on_tag_currentIndexChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QWidget* >(); break;
            }
            break;
        case 1:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QWidget* >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (FontFrame::*_t)(QWidget * );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&FontFrame::remove_font)) {
                *result = 0;
                return;
            }
        }
        {
            typedef void (FontFrame::*_t)(QWidget * , int );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&FontFrame::move_font)) {
                *result = 1;
                return;
            }
        }
    }
}

const QMetaObject FontFrame::staticMetaObject = {
    { &QFrame::staticMetaObject, qt_meta_stringdata_FontFrame.data,
      qt_meta_data_FontFrame,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *FontFrame::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *FontFrame::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_FontFrame.stringdata0))
        return static_cast<void*>(const_cast< FontFrame*>(this));
    return QFrame::qt_metacast(_clname);
}

int FontFrame::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QFrame::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void FontFrame::remove_font(QWidget * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void FontFrame::move_font(QWidget * _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
