#pragma once
#include "model/PatientRecord.h"

namespace scapms::model {

// Routine patient. Standard rule: an appointment must be later than "now".
class GeneralPatient : public PatientRecord {
public:
    using PatientRecord::PatientRecord;

    PatientCategory category() const override { return PatientCategory::General; }
    std::string categoryName() const override { return "General"; }
    std::string priorityLabel() const override { return "Normal"; }
    std::string summary() const override;
    std::unique_ptr<PatientRecord> clone() const override;

    void scheduleAppointment(const Appointment& appointment, const std::string& nowDateTime) override;
    bool cancelAppointment(const std::string& appointmentId) override;
};

} // namespace scapms::model
