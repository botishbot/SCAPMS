#include "model/PatientRecord.h"

#include "utils/ClinicException.h"
#include "utils/Validation.h"

#include <utility>

namespace scapms::model {

using utils::ClinicException;
using utils::ErrorCode;
namespace v = utils::validation;

PatientRecord::PatientRecord(std::string id, std::string name, int age, std::string phone,
                             std::string email)
    : m_id(std::move(id)), m_age(age) {
    if (v::trim(m_id).empty())
        throw ClinicException(ErrorCode::InvalidPatient, "Patient ID must not be blank.");
    v::requireName(name);
    v::requireAge(age);
    v::requireContact(phone, email);
    m_name = v::collapseSpaces(name);
    m_phone = v::trim(phone);
    m_email = v::trim(email);
}

std::string PatientRecord::contactSummary() const {
    if (!m_phone.empty() && !m_email.empty()) return m_phone + " / " + m_email;
    return m_phone.empty() ? m_email : m_phone;
}

void PatientRecord::setAppointments(std::vector<Appointment> appointments) {
    m_appointments = std::move(appointments);
    // Appointments of a live patient always show that patient's current name (e.g. after a rename).
    for (auto& a : m_appointments) a.setPatient(m_id, m_name);
}

bool PatientRecord::hasPendingAppointments() const { return pendingCount() > 0; }

int PatientRecord::pendingCount() const {
    int n = 0;
    for (const auto& a : m_appointments)
        if (a.status() == AppointmentStatus::Pending) ++n;
    return n;
}

int PatientRecord::completedCount() const {
    int n = 0;
    for (const auto& a : m_appointments)
        if (a.status() == AppointmentStatus::Completed) ++n;
    return n;
}

bool PatientRecord::hasAppointmentAt(const std::string& dateTime) const {
    for (const auto& a : m_appointments)
        if (a.status() != AppointmentStatus::Cancelled && a.dateTime() == dateTime) return true;
    return false;
}

const Appointment* PatientRecord::findAppointment(const std::string& appointmentId) const {
    for (const auto& a : m_appointments)
        if (a.id() == appointmentId) return &a;
    return nullptr;
}

bool PatientRecord::completeAppointment(const std::string& appointmentId, const std::string& outcomeNote) {
    for (auto& a : m_appointments) {
        if (a.id() == appointmentId) {
            if (a.status() != AppointmentStatus::Pending) return false;
            a.setStatus(AppointmentStatus::Completed);
            a.setOutcomeNote(v::collapseSpaces(outcomeNote));
            return true;
        }
    }
    return false;
}

std::optional<Appointment> PatientRecord::removePendingAppointment(const std::string& appointmentId) {
    for (std::size_t i = 0; i < m_appointments.size(); ++i) {
        if (m_appointments[i].id() == appointmentId) {
            if (m_appointments[i].status() != AppointmentStatus::Pending) return std::nullopt;
            Appointment removed = m_appointments[i];
            m_appointments.erase(m_appointments.begin() + static_cast<std::ptrdiff_t>(i));
            return removed;
        }
    }
    return std::nullopt;
}

void PatientRecord::addLoadedAppointment(Appointment appointment) {
    for (const auto& existing : m_appointments) {
        if (existing.id() == appointment.id())
            throw ClinicException(ErrorCode::DuplicateId, "Appointment ID already exists: " + appointment.id());
        if (existing.status() != AppointmentStatus::Cancelled && appointment.status() != AppointmentStatus::Cancelled &&
            existing.serviceId() == appointment.serviceId() && existing.dateTime() == appointment.dateTime())
            throw ClinicException(ErrorCode::DuplicateAppointment,
                                  "Two appointments for the same service at " + appointment.dateTime());
    }
    appointment.setPatient(m_id, m_name);
    m_appointments.push_back(std::move(appointment));
}

void PatientRecord::restoreRemovedAppointment(const Appointment& appointment) {
    if (appointment.status() != AppointmentStatus::Pending)
        throw ClinicException(ErrorCode::UndoUnavailable, "Only a pending appointment can be restored.");
    for (const auto& existing : m_appointments) {
        if (existing.id() == appointment.id())
            throw ClinicException(ErrorCode::UndoUnavailable, "The appointment already exists.");
        if (existing.status() != AppointmentStatus::Cancelled && existing.serviceId() == appointment.serviceId() &&
            existing.dateTime() == appointment.dateTime())
            throw ClinicException(ErrorCode::UndoUnavailable,
                                  "The same service has since been booked at that date and time.");
    }
    Appointment restored = appointment;
    restored.setPatient(m_id, m_name);
    m_appointments.push_back(std::move(restored));
}

void PatientRecord::reopenCancelledAppointment(const std::string& appointmentId) {
    Appointment* target = nullptr;
    for (auto& a : m_appointments)
        if (a.id() == appointmentId) { target = &a; break; }
    if (!target) throw ClinicException(ErrorCode::UndoUnavailable, "The appointment no longer exists.");
    if (target->status() != AppointmentStatus::Cancelled)
        throw ClinicException(ErrorCode::UndoUnavailable, "The appointment is no longer cancelled.");
    for (const auto& other : m_appointments)
        if (&other != target && other.status() != AppointmentStatus::Cancelled &&
            other.serviceId() == target->serviceId() && other.dateTime() == target->dateTime())
            throw ClinicException(ErrorCode::UndoUnavailable,
                                  "The same service has since been booked at that date and time.");
    target->setStatus(AppointmentStatus::Pending);
}

void PatientRecord::requireValidNow(const std::string& nowDateTime) {
    if (!v::isValidDateTime(nowDateTime))
        throw ClinicException(ErrorCode::InvalidAppointment, "The current date/time is not valid.");
}

void PatientRecord::attachAppointment(const Appointment& appointment) {
    if (appointment.status() != AppointmentStatus::Pending)
        throw ClinicException(ErrorCode::InvalidAppointment, "A new appointment must start as Pending.");
    for (const auto& existing : m_appointments) {
        if (existing.id() == appointment.id())
            throw ClinicException(ErrorCode::InvalidAppointment, "Appointment ID already exists for this patient.");
        if (existing.status() != AppointmentStatus::Cancelled &&
            existing.serviceId() == appointment.serviceId() &&
            existing.dateTime() == appointment.dateTime())
            throw ClinicException(ErrorCode::DuplicateAppointment,
                                  "This patient already has the same service booked at that date and time.");
    }
    Appointment stored = appointment;
    stored.setPatient(m_id, m_name);
    m_appointments.push_back(std::move(stored));
}

bool PatientRecord::cancelPending(const std::string& appointmentId) {
    for (auto& a : m_appointments) {
        if (a.id() == appointmentId) {
            if (a.status() != AppointmentStatus::Pending) return false;
            a.setStatus(AppointmentStatus::Cancelled);
            return true;
        }
    }
    return false;
}

} // namespace scapms::model
