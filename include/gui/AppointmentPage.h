#pragma once
#include "controller/AppointmentController.h"
#include "controller/ClinicManager.h"

#include <QSet>
#include <QString>
#include <QWidget>
#include <functional>
#include <string>

class QFrame;
class QTableWidget;
class QComboBox;
class QDateTimeEdit;
class QLineEdit;
class QLabel;
class QPushButton;
class QTimer;

namespace scapms::gui {

// Appointments page: table of all appointments (left) and a card with the booking form
// and actions for the selected appointment (right). Rules live in AppointmentController.
class AppointmentPage : public QWidget {
    Q_OBJECT
public:
    using Clock = std::function<std::string()>;  // current time as "YYYY-MM-DD HH:MM"

    AppointmentPage(controller::ClinicManager& patients, controller::AppointmentController& appointments,
                    QWidget* parent = nullptr, Clock clock = {});

    void refresh();  // reload patient list, service list and table
    void showPatient(const QString& patientId);  // list only this patient's appointments and select them in the booking form

    static constexpr int kReminderMinutes = 15;  // pending appointments starting within this time trigger a reminder

signals:
    void appointmentsChanged();  // an appointment or service was changed
    void statusMessage(const QString&);

private slots:
    void onSelectionChanged();
    void onScheduleClicked();
    void onCancelClicked();
    void onCompleteClicked();
    void onRemoveClicked();
    void onManageServicesClicked();
    void onClockTick();

private:
    void buildUi();
    void fillPatientCombo();
    void fillServiceCombo();
    void fillTable();
    void checkReminders();
    void updateFilterBanner();
    void updateActionButtons();
    std::string now() const;
    bool runGuarded(const std::function<void()>& action);

    controller::ClinicManager& m_patients;
    controller::AppointmentController& m_appointments;
    Clock m_clock;
    QString m_selectedId;
    QString m_selectedStatus;
    QString m_patientFilter;   // empty = all patients
    QFrame* m_filterBanner = nullptr;
    QLabel* m_filterLabel = nullptr;
    QSet<QString> m_reminded;  // appointments already announced (ID and time)

    QTableWidget* m_table = nullptr;
    QComboBox* m_filter = nullptr;
    QComboBox* m_patient = nullptr;
    QComboBox* m_service = nullptr;
    QDateTimeEdit* m_when = nullptr;
    QLineEdit* m_reason = nullptr;
    QLabel* m_hint = nullptr;
    QLabel* m_errorLabel = nullptr;
    QLabel* m_selectedInfo = nullptr;
    QPushButton* m_scheduleBtn = nullptr;
    QPushButton* m_manageBtn = nullptr;
    QPushButton* m_completeBtn = nullptr;
    QPushButton* m_cancelBtn = nullptr;
    QPushButton* m_removeBtn = nullptr;
    QTimer* m_timer = nullptr;
};

} // namespace scapms::gui
