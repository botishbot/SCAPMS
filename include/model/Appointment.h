#pragma once
#include <string>

namespace scapms::model {

enum class AppointmentStatus { Pending, Completed, Cancelled };

const char* statusName(AppointmentStatus s);

// One appointment. Patient and service names are stored as snapshots so history
// stays readable after a patient is deleted or a service is renamed/deactivated.
class Appointment {
public:
    // Throws ClinicException(InvalidAppointment) for blank ids or a bad date/time.
    Appointment(std::string id, std::string serviceId, std::string serviceName,
                std::string dateTime, std::string reason);

    const std::string& id() const { return m_id; }
    const std::string& patientId() const { return m_patientId; }
    const std::string& patientNameSnapshot() const { return m_patientName; }
    const std::string& serviceId() const { return m_serviceId; }
    const std::string& serviceNameSnapshot() const { return m_serviceName; }
    const std::string& dateTime() const { return m_dateTime; }  // "YYYY-MM-DD HH:MM"
    const std::string& reason() const { return m_reason; }
    AppointmentStatus status() const { return m_status; }
    const std::string& outcomeNote() const { return m_outcome; }

    void setPatient(const std::string& patientId, const std::string& patientName);
    void setStatus(AppointmentStatus s) { m_status = s; }
    void setOutcomeNote(const std::string& note) { m_outcome = note; }

private:
    std::string m_id;
    std::string m_patientId;
    std::string m_patientName;
    std::string m_serviceId;
    std::string m_serviceName;
    std::string m_dateTime;
    std::string m_reason;
    std::string m_outcome;
    AppointmentStatus m_status = AppointmentStatus::Pending;
};

} // namespace scapms::model
