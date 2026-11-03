#pragma once
#include "controller/AppointmentController.h"
#include "controller/ClinicManager.h"
#include "controller/QueueManager.h"

#include <QString>
#include <QWidget>
#include <functional>
#include <string>

class QTableWidget;
class QComboBox;
class QLabel;
class QPushButton;
class QTimer;

namespace scapms::gui {

// Waiting Queue page: "Now serving" card + FIFO table of waiting patients (left),
// controls to add, call next and remove (right). Rules live in QueueManager.
class QueuePage : public QWidget {
    Q_OBJECT
public:
    using Clock = std::function<std::string()>;  // current time as "YYYY-MM-DD HH:MM"

    QueuePage(controller::ClinicManager& patients, controller::AppointmentController& appointments,
              controller::QueueManager& queue, QWidget* parent = nullptr, Clock clock = {});

    void refresh();

    static constexpr int kWarnMinutes = 15;      // waiting this long turns amber
    static constexpr int kLongWaitMinutes = 30;  // waiting this long turns red and alerts staff

signals:
    void queueChanged();         // queue contents changed (not saved data)
    void appointmentsChanged();  // an appointment was completed (saved data)
    void statusMessage(const QString&);

private slots:
    void onPatientComboChanged();
    void onAddClicked();
    void onCallNextClicked();
    void onRemoveClicked();
    void onCompleteClicked();
    void onReleaseClicked();
    void onSelectionChanged();
    void onClockTick();

private:
    void buildUi();
    void fillPatientCombo();
    void fillAppointmentCombo();
    void fillServing();
    void fillTable();
    void updateButtons();
    std::string now() const;
    bool runGuarded(const std::function<void()>& action);

    controller::ClinicManager& m_patients;
    controller::AppointmentController& m_appointments;
    controller::QueueManager& m_queue;
    Clock m_clock;
    int m_lastAlertCount = 0;
    QString m_selectedPatientId;

    QLabel* m_servingName = nullptr;
    QLabel* m_servingDetail = nullptr;
    QPushButton* m_completeBtn = nullptr;
    QPushButton* m_releaseBtn = nullptr;
    QTableWidget* m_table = nullptr;
    QLabel* m_hint = nullptr;
    QComboBox* m_patientCombo = nullptr;
    QComboBox* m_appointmentCombo = nullptr;
    QPushButton* m_addBtn = nullptr;
    QPushButton* m_callBtn = nullptr;
    QPushButton* m_removeBtn = nullptr;
    QTimer* m_timer = nullptr;
};

} // namespace scapms::gui
