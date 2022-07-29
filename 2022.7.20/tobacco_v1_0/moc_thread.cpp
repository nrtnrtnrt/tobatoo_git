/****************************************************************************
** Meta object code from reading C++ file 'thread.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.9.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "thread.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'thread.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.9.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_SendThread_t {
    QByteArrayData data[9];
    char stringdata0[83];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_SendThread_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_SendThread_t qt_meta_stringdata_SendThread = {
    {
QT_MOC_LITERAL(0, 0, 10), // "SendThread"
QT_MOC_LITERAL(1, 11, 15), // "send_valve_data"
QT_MOC_LITERAL(2, 27, 0), // ""
QT_MOC_LITERAL(3, 28, 8), // "uint8_t*"
QT_MOC_LITERAL(4, 37, 18), // "send_to_up_machine"
QT_MOC_LITERAL(5, 56, 6), // "float*"
QT_MOC_LITERAL(6, 63, 9), // "send_mask"
QT_MOC_LITERAL(7, 73, 5), // "char*"
QT_MOC_LITERAL(8, 79, 3) // "buf"

    },
    "SendThread\0send_valve_data\0\0uint8_t*\0"
    "send_to_up_machine\0float*\0send_mask\0"
    "char*\0buf"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SendThread[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   29,    2, 0x06 /* Public */,
       4,    1,   32,    2, 0x06 /* Public */,
       6,    1,   35,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    2,
    QMetaType::Void, 0x80000000 | 5,    2,
    QMetaType::Void, 0x80000000 | 7,    8,

       0        // eod
};

void SendThread::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        SendThread *_t = static_cast<SendThread *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->send_valve_data((*reinterpret_cast< uint8_t*(*)>(_a[1]))); break;
        case 1: _t->send_to_up_machine((*reinterpret_cast< float*(*)>(_a[1]))); break;
        case 2: _t->send_mask((*reinterpret_cast< char*(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            typedef void (SendThread::*_t)(uint8_t * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SendThread::send_valve_data)) {
                *result = 0;
                return;
            }
        }
        {
            typedef void (SendThread::*_t)(float * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SendThread::send_to_up_machine)) {
                *result = 1;
                return;
            }
        }
        {
            typedef void (SendThread::*_t)(char * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SendThread::send_mask)) {
                *result = 2;
                return;
            }
        }
    }
}

const QMetaObject SendThread::staticMetaObject = {
    { &QThread::staticMetaObject, qt_meta_stringdata_SendThread.data,
      qt_meta_data_SendThread,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *SendThread::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SendThread::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SendThread.stringdata0))
        return static_cast<void*>(this);
    return QThread::qt_metacast(_clname);
}

int SendThread::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QThread::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void SendThread::send_valve_data(uint8_t * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void SendThread::send_to_up_machine(float * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void SendThread::send_mask(char * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
struct qt_meta_stringdata_SaveThread_t {
    QByteArrayData data[1];
    char stringdata0[11];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_SaveThread_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_SaveThread_t qt_meta_stringdata_SaveThread = {
    {
QT_MOC_LITERAL(0, 0, 10) // "SaveThread"

    },
    "SaveThread"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SaveThread[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void SaveThread::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObject SaveThread::staticMetaObject = {
    { &QThread::staticMetaObject, qt_meta_stringdata_SaveThread.data,
      qt_meta_data_SaveThread,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *SaveThread::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SaveThread::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SaveThread.stringdata0))
        return static_cast<void*>(this);
    return QThread::qt_metacast(_clname);
}

int SaveThread::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QThread::qt_metacall(_c, _id, _a);
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
