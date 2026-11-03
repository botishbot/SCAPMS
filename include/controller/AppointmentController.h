#pragma once
#include "controller/UndoManager.h"
#include "model/Appointment.h"
#include "model/ClinicDatabase.h"
#include "model/ClinicService.h"
#include "model/PatientRecord.h"
#include "utils/IDGenerator.h"

#include <functional>
#include <string>
#include <vector>

namespace scapms::controller {

// One line of the appointments table: the appointment plus display information.
struct AppointmentRow {
    model::Appointment appointment;
    model::PatientCategory category;
    bool overdue;                 // Pending and its time has already passed (derived, not stored)
    std::string displayStatus;    // "Pending", "Overdue", "Completed" or "Cancelled"
};

// Business rules for services and appointments. The GUI calls this class.
// Scheduling and cancelling go through the Appointable interface, so each patient
// type applies its own rules (runtime polymorphism).
class AppointmentController {
public:
    AppointmentController(model::ClinicDatabase& database, utils::IDGenerator& idGenerator, UndoManager& undo);

    // ---- services ----
    std::string addService(const std::string& name, const std::string& description);
    void setServiceActive(const std::string& serviceId, bool active);
    const std::vector<model::ClinicService>& services() const { return m_database.services(); }
    std::vector<model::ClinicService> activeServices() const;

    // ---- scheduling ----
    // True when the patient already has a non-cancelled appointment at this exact time
    // for a different service. The GUI asks the user to confirm in that case.
    bool hasSameTimeConflict(const std::string& patientId, const std::string& serviceId,
                             const std::string& dateTime) const;

    // nowDateTime is "YYYY-MM-DD HH:MM". Returns the new appointment ID.
    std::string schedule(const std::string& patientId, const std::string& serviceId,
                         const std::string& dateTime, const std::string& reason,
                         const std::string& nowDateTime);

    // ---- lifecycle ----
    void cancel(const std::string& appointmentId);                       // Pending -> Cancelled (undoable)
    void complete(const std::string& appointmentId, const std::string& outcomeNote);  // Pending -> Completed
    void remove(const std::string& appointmentId);                       // Pending only (undoable)

    // Called after an appointment is cancelled, completed or removed, so the waiting queue
    // can drop it. (Set by the main window; keeps this class independent of the queue.)
    void setAppointmentClosedCallback(std::function<void(const std::string&)> callback) {
        m_onClosed = std::move(callback);
    }

    // ---- queries ----
    std::vector<AppointmentRow> allAppointments(const std::string& nowDateTime) const;  // earliest first
    int countOnDate(const std::string& date) const;                      // non-cancelled, "YYYY-MM-DD"
    static bool isOverdue(const model::Appointment& appointment, const std::string& nowDateTime);
    static std::string displayStatus(const model::Appointment& appointment, const std::string& nowDateTime);

private:
    model::PatientRecord* findOwner(const std::string& appointmentId) const;

    model::ClinicDatabase& m_database;
    utils::IDGenerator& m_ids;
    UndoManager& m_undo;
    std::function<void(const std::string&)> m_onClosed;
};

} // namespace scapms::controller
