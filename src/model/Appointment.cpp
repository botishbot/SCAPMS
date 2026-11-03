#include "model/Appointment.h"

#include "utils/ClinicException.h"
#include "utils/Validation.h"

#include <utility>

namespace scapms::model {

using utils::ClinicException;
using utils::ErrorCode;

const char* statusName(AppointmentStatus s) {
    switch (s) {
        case AppointmentStatus::Pending: return "Pending";
        case AppointmentStatus::Completed: return "Completed";
        case AppointmentStatus::Cancelled: return "Cancelled";
    }
    return "Unknown";
}

Appointment::Appointment(std::string id, std::string serviceId, std::string serviceName,
                         std::string dateTime, std::string reason)
    : m_id(std::move(id)),
      m_serviceId(std::move(serviceId)),
      m_serviceName(std::move(serviceName)),
      m_dateTime(std::move(dateTime)),
      m_reason(std::move(reason)) {
    if (utils::validation::trim(m_id).empty())
        throw ClinicException(ErrorCode::InvalidAppointment, "Appointment ID must not be blank.");
    if (utils::validation::trim(m_serviceId).empty())
        throw ClinicException(ErrorCode::InvalidAppointment, "Please select a clinic service.");
    if (!utils::validation::isValidDateTime(m_dateTime))
        throw ClinicException(ErrorCode::InvalidAppointment,
                              "Appointment date/time must look like 2026-12-31 14:30.");
}

void Appointment::setPatient(const std::string& patientId, const std::string& patientName) {
    m_patientId = patientId;
    m_patientName = patientName;
}

} // namespace scapms::model
