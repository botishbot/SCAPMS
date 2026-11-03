#pragma once
#include "model/PatientRecord.h"

namespace scapms::model {

// Urgent patient with a triage level (1 = critical, 2 = urgent, 3 = less urgent).
// Special rule: may be booked for any time today, including an earlier time (walk-in).
class EmergencyPatient : public PatientRecord {
public:
    EmergencyPatient(std::string id, std::string name, int age, std::string phone,
                     std::string email, int triageLevel);

    int triageLevel() const { return m_triage; }
    static const char* triageName(int level);

    PatientCategory category() const override { return PatientCategory::Emergency; }
    std::string categoryName() const override { return "Emergency"; }
    std::string priorityLabel() const override { return "High Priority"; }
    std::string summary() const override;
    std::unique_ptr<PatientRecord> clone() const override;

    void scheduleAppointment(const Appointment& appointment, const std::string& nowDateTime) override;
    bool cancelAppointment(const std::string& appointmentId) override;

private:
    int m_triage;
};

} // namespace scapms::model
