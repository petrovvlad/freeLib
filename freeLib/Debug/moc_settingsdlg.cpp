/****************************************************************************
** Meta object code from reading C++ file 'settingsdlg.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.9.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../src/settingsdlg.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'settingsdlg.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.9.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_FileItemDelegate_t {
    QByteArrayData data[4];
    char stringdata0[45];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_FileItemDelegate_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_FileItemDelegate_t qt_meta_stringdata_FileItemDelegate = {
    {
QT_MOC_LITERAL(0, 0, 16), // "FileItemDelegate"
QT_MOC_LITERAL(1, 17, 15), // "editingFinished"
QT_MOC_LITERAL(2, 33, 0), // ""
QT_MOC_LITERAL(3, 34, 10) // "SelectFile"

    },
    "FileItemDelegate\0editingFinished\0\0"
    "SelectFile"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_FileItemDelegate[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   24,    2, 0x08 /* Private */,
       3,    0,   25,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void FileItemDelegate::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        FileItemDelegate *_t = static_cast<FileItemDelegate *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->editingFinished(); break;
        case 1: _t->SelectFile(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObject FileItemDelegate::staticMetaObject = {
    { &QItemDelegate::staticMetaObject, qt_meta_stringdata_FileItemDelegate.data,
      qt_meta_data_FileItemDelegate,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *FileItemDelegate::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *FileItemDelegate::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_FileItemDelegate.stringdata0))
        return static_cast<void*>(const_cast< FileItemDelegate*>(this));
    return QItemDelegate::qt_metacast(_clname);
}

int FileItemDelegate::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QItemDelegate::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 2)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 2;
    }
    return _id;
}
struct qt_meta_stringdata_SettingsDlg_t {
    QByteArrayData data[41];
    char stringdata0[673];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_SettingsDlg_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_SettingsDlg_t qt_meta_stringdata_SettingsDlg = {
    {
QT_MOC_LITERAL(0, 0, 11), // "SettingsDlg"
QT_MOC_LITERAL(1, 12, 12), // "ChangingPort"
QT_MOC_LITERAL(2, 25, 0), // ""
QT_MOC_LITERAL(3, 26, 1), // "i"
QT_MOC_LITERAL(4, 28, 16), // "ChangingLanguage"
QT_MOC_LITERAL(5, 45, 22), // "ChangingExportFrameTab"
QT_MOC_LITERAL(6, 68, 6), // "tab_id"
QT_MOC_LITERAL(7, 75, 7), // "page_id"
QT_MOC_LITERAL(8, 83, 15), // "NeedUpdateTools"
QT_MOC_LITERAL(9, 99, 16), // "ChangingTrayIcon"
QT_MOC_LITERAL(10, 116, 5), // "index"
QT_MOC_LITERAL(11, 122, 5), // "color"
QT_MOC_LITERAL(12, 128, 5), // "btnOK"
QT_MOC_LITERAL(13, 134, 9), // "SaveTools"
QT_MOC_LITERAL(14, 144, 10), // "QSettings*"
QT_MOC_LITERAL(15, 155, 8), // "settings"
QT_MOC_LITERAL(16, 164, 6), // "AddExt"
QT_MOC_LITERAL(17, 171, 6), // "DelExt"
QT_MOC_LITERAL(18, 178, 6), // "AddApp"
QT_MOC_LITERAL(19, 185, 6), // "DelApp"
QT_MOC_LITERAL(20, 192, 10), // "ChangePort"
QT_MOC_LITERAL(21, 203, 14), // "ChangeLanguage"
QT_MOC_LITERAL(22, 218, 20), // "on_AddExport_clicked"
QT_MOC_LITERAL(23, 239, 17), // "ExportNameChanged"
QT_MOC_LITERAL(24, 257, 20), // "on_DelExport_clicked"
QT_MOC_LITERAL(25, 278, 33), // "on_ExportName_currentIndexCha..."
QT_MOC_LITERAL(26, 312, 23), // "on_ChangeExportFrameTab"
QT_MOC_LITERAL(27, 336, 24), // "on_DefaultExport_clicked"
QT_MOC_LITERAL(28, 361, 9), // "btnDBPath"
QT_MOC_LITERAL(29, 371, 10), // "btnDirPath"
QT_MOC_LITERAL(30, 382, 29), // "on_btnDefaultSettings_clicked"
QT_MOC_LITERAL(31, 412, 27), // "on_tabWidget_currentChanged"
QT_MOC_LITERAL(32, 440, 19), // "UpdateWebExportList"
QT_MOC_LITERAL(33, 460, 33), // "on_proxy_type_currentIndexCha..."
QT_MOC_LITERAL(34, 494, 25), // "on_browseDir_stateChanged"
QT_MOC_LITERAL(35, 520, 7), // "checked"
QT_MOC_LITERAL(36, 528, 31), // "on_trayIcon_currentIndexChanged"
QT_MOC_LITERAL(37, 560, 33), // "on_tray_color_currentIndexCha..."
QT_MOC_LITERAL(38, 594, 28), // "on_HTTP_need_pasword_clicked"
QT_MOC_LITERAL(39, 623, 24), // "on_btnSaveExport_clicked"
QT_MOC_LITERAL(40, 648, 24) // "on_btnOpenExport_clicked"

    },
    "SettingsDlg\0ChangingPort\0\0i\0"
    "ChangingLanguage\0ChangingExportFrameTab\0"
    "tab_id\0page_id\0NeedUpdateTools\0"
    "ChangingTrayIcon\0index\0color\0btnOK\0"
    "SaveTools\0QSettings*\0settings\0AddExt\0"
    "DelExt\0AddApp\0DelApp\0ChangePort\0"
    "ChangeLanguage\0on_AddExport_clicked\0"
    "ExportNameChanged\0on_DelExport_clicked\0"
    "on_ExportName_currentIndexChanged\0"
    "on_ChangeExportFrameTab\0"
    "on_DefaultExport_clicked\0btnDBPath\0"
    "btnDirPath\0on_btnDefaultSettings_clicked\0"
    "on_tabWidget_currentChanged\0"
    "UpdateWebExportList\0"
    "on_proxy_type_currentIndexChanged\0"
    "on_browseDir_stateChanged\0checked\0"
    "on_trayIcon_currentIndexChanged\0"
    "on_tray_color_currentIndexChanged\0"
    "on_HTTP_need_pasword_clicked\0"
    "on_btnSaveExport_clicked\0"
    "on_btnOpenExport_clicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SettingsDlg[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
      33,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,  179,    2, 0x06 /* Public */,
       4,    0,  182,    2, 0x06 /* Public */,
       5,    2,  183,    2, 0x06 /* Public */,
       8,    0,  188,    2, 0x06 /* Public */,
       9,    2,  189,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      12,    0,  194,    2, 0x08 /* Private */,
      13,    1,  195,    2, 0x08 /* Private */,
      13,    0,  198,    2, 0x28 /* Private | MethodCloned */,
      16,    0,  199,    2, 0x08 /* Private */,
      17,    0,  200,    2, 0x08 /* Private */,
      18,    0,  201,    2, 0x08 /* Private */,
      19,    0,  202,    2, 0x08 /* Private */,
      20,    1,  203,    2, 0x08 /* Private */,
      20,    0,  206,    2, 0x28 /* Private | MethodCloned */,
      21,    0,  207,    2, 0x08 /* Private */,
      22,    0,  208,    2, 0x08 /* Private */,
      23,    0,  209,    2, 0x08 /* Private */,
      24,    0,  210,    2, 0x08 /* Private */,
      25,    1,  211,    2, 0x08 /* Private */,
      26,    2,  214,    2, 0x08 /* Private */,
      27,    0,  219,    2, 0x08 /* Private */,
      28,    0,  220,    2, 0x08 /* Private */,
      29,    0,  221,    2, 0x08 /* Private */,
      30,    0,  222,    2, 0x08 /* Private */,
      31,    1,  223,    2, 0x08 /* Private */,
      32,    0,  226,    2, 0x08 /* Private */,
      33,    1,  227,    2, 0x08 /* Private */,
      34,    1,  230,    2, 0x08 /* Private */,
      36,    1,  233,    2, 0x08 /* Private */,
      37,    1,  236,    2, 0x08 /* Private */,
      38,    0,  239,    2, 0x08 /* Private */,
      39,    0,  240,    2, 0x08 /* Private */,
      40,    0,  241,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,    6,    7,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,   10,   11,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 14,   15,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   10,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,    6,    7,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   10,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   10,
    QMetaType::Void, QMetaType::Int,   35,
    QMetaType::Void, QMetaType::Int,   10,
    QMetaType::Void, QMetaType::Int,   10,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void SettingsDlg::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        SettingsDlg *_t = static_cast<SettingsDlg *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->ChangingPort((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->ChangingLanguage(); break;
        case 2: _t->ChangingExportFrameTab((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 3: _t->NeedUpdateTools(); break;
        case 4: _t->ChangingTrayIcon((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 5: _t->btnOK(); break;
        case 6: _t->SaveTools((*reinterpret_cast< QSettings*(*)>(_a[1]))); break;
        case 7: _t->SaveTools(); break;
        case 8: _t->AddExt(); break;
        case 9: _t->DelExt(); break;
        case 10: _t->AddApp(); break;
        case 11: _t->DelApp(); break;
        case 12: _t->ChangePort((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 13: _t->ChangePort(); break;
        case 14: _t->ChangeLanguage(); break;
        case 15: _t->on_AddExport_clicked(); break;
        case 16: _t->ExportNameChanged(); break;
        case 17: _t->on_DelExport_clicked(); break;
        case 18: _t->on_ExportName_currentIndexChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 19: _t->on_ChangeExportFrameTab((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 20: _t->on_DefaultExport_clicked(); break;
        case 21: _t->btnDBPath(); break;
        case 22: _t->btnDirPath(); break;
        case 23: _t->on_btnDefaultSettings_clicked(); break;
        case 24: _t->on_tabWidget_currentChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 25: _t->UpdateWebExportList(); break;
        case 26: _t->on_proxy_type_currentIndexChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 27: _t->on_browseDir_stateChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 28: _t->on_trayIcon_currentIndexChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 29: _t->on_tray_color_currentIndexChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 30: _t->on_HTTP_need_pasword_clicked(); break;
        case 31: _t->on_btnSaveExport_clicked(); break;
        case 32: _t->on_btnOpenExport_clicked(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 6:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QSettings* >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (SettingsDlg::*_t)(int );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&SettingsDlg::ChangingPort)) {
                *result = 0;
                return;
            }
        }
        {
            typedef void (SettingsDlg::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&SettingsDlg::ChangingLanguage)) {
                *result = 1;
                return;
            }
        }
        {
            typedef void (SettingsDlg::*_t)(int , int );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&SettingsDlg::ChangingExportFrameTab)) {
                *result = 2;
                return;
            }
        }
        {
            typedef void (SettingsDlg::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&SettingsDlg::NeedUpdateTools)) {
                *result = 3;
                return;
            }
        }
        {
            typedef void (SettingsDlg::*_t)(int , int );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&SettingsDlg::ChangingTrayIcon)) {
                *result = 4;
                return;
            }
        }
    }
}

const QMetaObject SettingsDlg::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_SettingsDlg.data,
      qt_meta_data_SettingsDlg,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *SettingsDlg::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SettingsDlg::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SettingsDlg.stringdata0))
        return static_cast<void*>(const_cast< SettingsDlg*>(this));
    return QDialog::qt_metacast(_clname);
}

int SettingsDlg::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 33)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 33;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 33)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 33;
    }
    return _id;
}

// SIGNAL 0
void SettingsDlg::ChangingPort(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void SettingsDlg::ChangingLanguage()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void SettingsDlg::ChangingExportFrameTab(int _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void SettingsDlg::NeedUpdateTools()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void SettingsDlg::ChangingTrayIcon(int _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
