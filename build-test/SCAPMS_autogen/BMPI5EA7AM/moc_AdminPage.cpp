/****************************************************************************
** Meta object code from reading C++ file 'AdminPage.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../include/gui/AdminPage.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'AdminPage.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN6scapms3gui9AdminPageE_t {};
} // unnamed namespace

template <> constexpr inline auto scapms::gui::AdminPage::qt_create_metaobjectdata<qt_meta_tag_ZN6scapms3gui9AdminPageE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "scapms::gui::AdminPage",
        "saveRequested",
        "",
        "saveAsRequested",
        "loadRequested",
        "exportRequested",
        "newRequested",
        "demoRequested",
        "undoRequested"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'saveRequested'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'saveAsRequested'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'loadRequested'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'exportRequested'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'newRequested'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'demoRequested'
        QtMocHelpers::SignalData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'undoRequested'
        QtMocHelpers::SignalData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<AdminPage, qt_meta_tag_ZN6scapms3gui9AdminPageE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject scapms::gui::AdminPage::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6scapms3gui9AdminPageE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6scapms3gui9AdminPageE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN6scapms3gui9AdminPageE_t>.metaTypes,
    nullptr
} };

void scapms::gui::AdminPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<AdminPage *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->saveRequested(); break;
        case 1: _t->saveAsRequested(); break;
        case 2: _t->loadRequested(); break;
        case 3: _t->exportRequested(); break;
        case 4: _t->newRequested(); break;
        case 5: _t->demoRequested(); break;
        case 6: _t->undoRequested(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (AdminPage::*)()>(_a, &AdminPage::saveRequested, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminPage::*)()>(_a, &AdminPage::saveAsRequested, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminPage::*)()>(_a, &AdminPage::loadRequested, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminPage::*)()>(_a, &AdminPage::exportRequested, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminPage::*)()>(_a, &AdminPage::newRequested, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminPage::*)()>(_a, &AdminPage::demoRequested, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminPage::*)()>(_a, &AdminPage::undoRequested, 6))
            return;
    }
}

const QMetaObject *scapms::gui::AdminPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *scapms::gui::AdminPage::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6scapms3gui9AdminPageE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int scapms::gui::AdminPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void scapms::gui::AdminPage::saveRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void scapms::gui::AdminPage::saveAsRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void scapms::gui::AdminPage::loadRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void scapms::gui::AdminPage::exportRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void scapms::gui::AdminPage::newRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void scapms::gui::AdminPage::demoRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void scapms::gui::AdminPage::undoRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}
QT_WARNING_POP
