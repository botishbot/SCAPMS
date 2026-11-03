#include "model/FollowUpPatient.h"

#include "utils/ClinicException.h"
#include "utils/Validation.h"

namespace scapms::model {

using utils::ClinicException;
using utils::ErrorCode;
namespace v = utils::validation;

FollowUpPatient::FollowUpPatient(std::string id, std::string name, int age, std::string phone,
                                 std::string email, std::string previousVisitDate,
                                 std::string followUpReason)
    : PatientRecord(std::move(id), std::move(name), age, std::move(phone), std::move(email)),
      m_previousVisit(v::trim(previousVisitDate)),
      m_reason(v::collapseSpaces(followUpReason)) {
    if (!v::isValidDate(m_previousVisit))
        throw ClinicException(ErrorCode::InvalidPatient,
                              "Previous visit date must be a valid date (YYYY-MM-DD).");
}

std::string FollowUpPatient::summary() const {
    std::string s = "Previous visit " + m_previousVisit;
    if (!m_reason.empty()) s += " - " + m_reason;
    return s;
}

std::unique_ptr<PatientRecord> FollowUpPatient::clone() const {
    return std::make_unique<FollowUpPatient>(*this);
}

void FollowUpPatient::scheduleAppointment(const Appointment& appointment, const std::string& nowDateTime) {
    requireValidNow(nowDateTime);
    if (appointment.dateTime() <= nowDateTime)
        throw ClinicException(ErrorCode::InvalidAppointment,
                              "Appointment must be later than the current date and time.");
    if (v::datePart(appointment.dateTime()) <= m_previousVisit)
        throw ClinicException(ErrorCode::InvalidAppointment,
                              "A follow-up must be after the previous visit date (" + m_previousVisit + ").");
    attachAppointment(appointment);
}

bool FollowUpPatient::cancelAppointment(const std::string& appointmentId) {
    return cancelPending(appointmentId);
}

} // namespace scapms::model
