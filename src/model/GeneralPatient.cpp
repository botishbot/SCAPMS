#include "model/GeneralPatient.h"

#include "utils/ClinicException.h"

namespace scapms::model {

using utils::ClinicException;
using utils::ErrorCode;

std::string GeneralPatient::summary() const { return "Routine patient"; }

std::unique_ptr<PatientRecord> GeneralPatient::clone() const {
    return std::make_unique<GeneralPatient>(*this);
}

void GeneralPatient::scheduleAppointment(const Appointment& appointment, const std::string& nowDateTime) {
    requireValidNow(nowDateTime);
    if (appointment.dateTime() <= nowDateTime)
        throw ClinicException(ErrorCode::InvalidAppointment,
                              "Appointment must be later than the current date and time.");
    attachAppointment(appointment);
}

bool GeneralPatient::cancelAppointment(const std::string& appointmentId) {
    return cancelPending(appointmentId);
}

} // namespace scapms::model
