/****************************************************************************
** Meta object code from reading C++ file 'QueuePage.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../include/gui/QueuePage.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'QueuePage.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN6scapms3gui9QueuePageE_t {};
} // unnamed namespace

template <> constexpr inline auto scapms::gui::QueuePage::qt_create_metaobjectdata<qt_meta_tag_ZN6scapms3gui9QueuePageE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "scapms::gui::QueuePage",
        "queueChanged",
        "",
        "appointmentsChanged",
        "statusMessage",
        "onPatientComboChanged",
        "onAddClicked",
        "onCallNextClicked",
        "onRemoveClicked",
        "onCompleteClicked",
        "onReleaseClicked",
        "onSelectionChanged",
        "onClockTick"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'queueChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'appointmentsChanged'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'statusMessage'
        QtMocHelpers::SignalData<void(const QString &)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 2 },
        }}),
        // Slot 'onPatientComboChanged'
        QtMocHelpers::SlotData<void()>(5, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onAddClicked'
        QtMocHelpers::SlotData<void()>(6, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onCallNextClicked'
        QtMocHelpers::SlotData<void()>(7, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onRemoveClicked'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onCompleteClicked'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onReleaseClicked'
        QtMocHelpers::SlotData<void()>(10, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSelectionChanged'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onClockTick'
        QtMocHelpers::SlotData<void()>(12, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<QueuePage, qt_meta_tag_ZN6scapms3gui9QueuePageE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject scapms::gui::QueuePage::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6scapms3gui9QueuePageE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6scapms3gui9QueuePageE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN6scapms3gui9QueuePageE_t>.metaTypes,
    nullptr
} };

void scapms::gui::QueuePage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<QueuePage *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->queueChanged(); break;
        case 1: _t->appointmentsChanged(); break;
        case 2: _t->statusMessage((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->onPatientComboChanged(); break;
        case 4: _t->onAddClicked(); break;
        case 5: _t->onCallNextClicked(); break;
        case 6: _t->onRemoveClicked(); break;
        case 7: _t->onCompleteClicked(); break;
        case 8: _t->onReleaseClicked(); break;
        case 9: _t->onSelectionChanged(); break;
        case 10: _t->onClockTick(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (QueuePage::*)()>(_a, &QueuePage::queueChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (QueuePage::*)()>(_a, &QueuePage::appointmentsChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (QueuePage::*)(const QString & )>(_a, &QueuePage::statusMessage, 2))
            return;
    }
}

const QMetaObject *scapms::gui::QueuePage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *scapms::gui::QueuePage::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6scapms3gui9QueuePageE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int scapms::gui::QueuePage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 11)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 11;
    }
    return _id;
}

// SIGNAL 0
void scapms::gui::QueuePage::queueChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void scapms::gui::QueuePage::appointmentsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void scapms::gui::QueuePage::statusMessage(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}
QT_WARNING_POP
