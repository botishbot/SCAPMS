#include "model/EmergencyPatient.h"

#include "utils/ClinicException.h"
#include "utils/Validation.h"

namespace scapms::model {

using utils::ClinicException;
using utils::ErrorCode;

EmergencyPatient::EmergencyPatient(std::string id, std::string name, int age, std::string phone,
                                   std::string email, int triageLevel)
    : PatientRecord(std::move(id), std::move(name), age, std::move(phone), std::move(email)),
      m_triage(triageLevel) {
    if (triageLevel < 1 || triageLevel > 3)
        throw ClinicException(ErrorCode::InvalidPatient, "Triage level must be 1, 2 or 3.");
}

const char* EmergencyPatient::triageName(int level) {
    switch (level) {
        case 1: return "Critical";
        case 2: return "Urgent";
        case 3: return "Less urgent";
        default: return "Unknown";
    }
}

std::string EmergencyPatient::summary() const {
    return "Triage level " + std::to_string(m_triage) + " (" + triageName(m_triage) + ")";
}

std::unique_ptr<PatientRecord> EmergencyPatient::clone() const {
    return std::make_unique<EmergencyPatient>(*this);
}

void EmergencyPatient::scheduleAppointment(const Appointment& appointment, const std::string& nowDateTime) {
    requireValidNow(nowDateTime);
    // Walk-in rule: today (any time) or later is fine; earlier days are not.
    if (utils::validation::datePart(appointment.dateTime()) < utils::validation::datePart(nowDateTime))
        throw ClinicException(ErrorCode::InvalidAppointment,
                              "Emergency appointments can be booked for today or a later date only.");
    attachAppointment(appointment);
}

bool EmergencyPatient::cancelAppointment(const std::string& appointmentId) {
    return cancelPending(appointmentId);
}

} // namespace scapms::model
