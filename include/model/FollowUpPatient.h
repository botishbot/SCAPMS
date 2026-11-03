#pragma once
#include "model/PatientRecord.h"

namespace scapms::model {

// Returning patient. Stores the date of the previous visit.
// Special rule: a follow-up must be later than "now" AND later than the previous visit date.
class FollowUpPatient : public PatientRecord {
public:
    FollowUpPatient(std::string id, std::string name, int age, std::string phone, std::string email,
                    std::string previousVisitDate, std::string followUpReason);

    const std::string& previousVisitDate() const { return m_previousVisit; }  // "YYYY-MM-DD"
    const std::string& followUpReason() const { return m_reason; }

    PatientCategory category() const override { return PatientCategory::FollowUp; }
    std::string categoryName() const override { return "Follow-Up"; }
    std::string priorityLabel() const override { return "Scheduled"; }
    std::string summary() const override;
    std::unique_ptr<PatientRecord> clone() const override;

    void scheduleAppointment(const Appointment& appointment, const std::string& nowDateTime) override;
    bool cancelAppointment(const std::string& appointmentId) override;

private:
    std::string m_previousVisit;
    std::string m_reason;
};

} // namespace scapms::model
