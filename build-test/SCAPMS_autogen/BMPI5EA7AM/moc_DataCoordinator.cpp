/****************************************************************************
** Meta object code from reading C++ file 'DataCoordinator.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../include/gui/DataCoordinator.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DataCoordinator.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.10.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN6scapms3gui15DataCoordinatorE_t {};
} // unnamed namespace

template <> constexpr inline auto scapms::gui::DataCoordinator::qt_create_metaobjectdata<qt_meta_tag_ZN6scapms3gui15DataCoordinatorE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "scapms::gui::DataCoordinator",
        "dataReplaced",
        "",
        "markDirty",
        "dataSaved",
        "pathChanged",
        "statusMessage",
        "text",
        "newData",
        "save",
        "saveAs",
        "load",
        "exportPatientsCsv",
        "loadDemo"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'dataReplaced'
        QtMocHelpers::SignalData<void(bool)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 3 },
        }}),
        // Signal 'dataSaved'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'pathChanged'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'statusMessage'
        QtMocHelpers::SignalData<void(const QString &)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 7 },
        }}),
        // Slot 'newData'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'save'
        QtMocHelpers::SlotData<bool()>(9, 2, QMC::AccessPublic, QMetaType::Bool),
        // Slot 'saveAs'
        QtMocHelpers::SlotData<bool()>(10, 2, QMC::AccessPublic, QMetaType::Bool),
        // Slot 'load'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'exportPatientsCsv'
        QtMocHelpers::SlotData<void()>(12, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'loadDemo'
        QtMocHelpers::SlotData<void()>(13, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<DataCoordinator, qt_meta_tag_ZN6scapms3gui15DataCoordinatorE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject scapms::gui::DataCoordinator::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6scapms3gui15DataCoordinatorE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6scapms3gui15DataCoordinatorE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN6scapms3gui15DataCoordinatorE_t>.metaTypes,
    nullptr
} };

void scapms::gui::DataCoordinator::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<DataCoordinator *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->dataReplaced((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 1: _t->dataSaved(); break;
        case 2: _t->pathChanged(); break;
        case 3: _t->statusMessage((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->newData(); break;
        case 5: { bool _r = _t->save();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 6: { bool _r = _t->saveAs();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 7: _t->load(); break;
        case 8: _t->exportPatientsCsv(); break;
        case 9: _t->loadDemo(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (DataCoordinator::*)(bool )>(_a, &DataCoordinator::dataReplaced, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (DataCoordinator::*)()>(_a, &DataCoordinator::dataSaved, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (DataCoordinator::*)()>(_a, &DataCoordinator::pathChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (DataCoordinator::*)(const QString & )>(_a, &DataCoordinator::statusMessage, 3))
            return;
    }
}

const QMetaObject *scapms::gui::DataCoordinator::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *scapms::gui::DataCoordinator::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6scapms3gui15DataCoordinatorE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int scapms::gui::DataCoordinator::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 10)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 10;
    }
    return _id;
}

// SIGNAL 0
void scapms::gui::DataCoordinator::dataReplaced(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void scapms::gui::DataCoordinator::dataSaved()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void scapms::gui::DataCoordinator::pathChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void scapms::gui::DataCoordinator::statusMessage(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}
QT_WARNING_POP
