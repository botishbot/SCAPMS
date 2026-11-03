#pragma once
#include "model/Appointable.h"
#include "model/Appointment.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace scapms::model {

enum class PatientCategory { General, Emergency, FollowUp };

// Abstract base class for all patients. Holds the common data and the patient's
// own appointment list (composition). Pure virtual functions make it abstract.
// Common fields are validated in the constructor, so an invalid patient object
// cannot be created.
class PatientRecord : public Appointable {
public:
    PatientRecord(std::string id, std::string name, int age, std::string phone, std::string email);
    ~PatientRecord() override = default;

    // ---- pure virtual: each patient type must define these ----
    virtual PatientCategory category() const = 0;
    virtual std::string categoryName() const = 0;
    virtual std::string priorityLabel() const = 0;  // "High Priority" / "Normal" / "Scheduled"
    virtual std::string summary() const = 0;        // one line of type-specific detail
    virtual std::unique_ptr<PatientRecord> clone() const = 0;  // deep copy (used for undo snapshots)
    // scheduleAppointment() and cancelAppointment() stay pure virtual (from Appointable).

    // ---- common data ----
    const std::string& id() const { return m_id; }
    const std::string& name() const { return m_name; }
    int age() const { return m_age; }
    const std::string& phone() const { return m_phone; }
    const std::string& email() const { return m_email; }
    std::string contactSummary() const;

    // ---- appointments ----
    const std::vector<Appointment>& appointments() const { return m_appointments; }
    void setAppointments(std::vector<Appointment> appointments);  // used when a record is edited/restored
    bool hasPendingAppointments() const;
    int pendingCount() const;
    int completedCount() const;
    bool hasAppointmentAt(const std::string& dateTime) const;  // any non-cancelled appointment at that time
    const Appointment* findAppointment(const std::string& appointmentId) const;

    // Lifecycle steps shared by every patient type (Pending -> Completed / removed).
    bool completeAppointment(const std::string& appointmentId, const std::string& outcomeNote);  // false unless Pending
    std::optional<Appointment> removePendingAppointment(const std::string& appointmentId);       // empty unless Pending

    // Used when loading saved data (any status, no booking-time rules). Rejects a repeated appointment ID
    // and a second non-cancelled appointment for the same service and time.
    void addLoadedAppointment(Appointment appointment);

    // Used by undo. Both throw ClinicException(UndoUnavailable) if the old state can no longer be restored.
    void restoreRemovedAppointment(const Appointment& appointment);   // puts a removed Pending appointment back
    void reopenCancelledAppointment(const std::string& appointmentId);  // Cancelled -> Pending

protected:
    // Shared steps used by the subclasses after their own rule check.
    void attachAppointment(const Appointment& appointment);  // blocks exact duplicates
    bool cancelPending(const std::string& appointmentId);
    static void requireValidNow(const std::string& nowDateTime);

private:
    std::string m_id;
    std::string m_name;
    int m_age;
    std::string m_phone;
    std::string m_email;
    std::vector<Appointment> m_appointments;
};

} // namespace scapms::model
