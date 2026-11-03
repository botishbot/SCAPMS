#include "controller/UndoCommands.h"

#include "utils/ClinicException.h"

#include <utility>

namespace scapms::controller {

using utils::ClinicException;
using utils::ErrorCode;

// ---------------------------------------------------------------- delete patient
UndoDeletePatient::UndoDeletePatient(std::unique_ptr<model::PatientRecord> patient,
                                     std::vector<std::string> archivedAppointmentIds)
    : m_patient(std::move(patient)), m_archivedIds(std::move(archivedAppointmentIds)) {
    m_patientId = m_patient->id();
    m_name = m_patient->name();
}

std::string UndoDeletePatient::description() const {
    return "Delete patient " + m_patientId + " (" + m_name + ")";
}

void UndoDeletePatient::undo(model::ClinicDatabase& database) {
    if (database.findPatient(m_patientId))
        throw ClinicException(ErrorCode::UndoUnavailable, "A patient with this ID exists again.");
    database.addPatient(std::move(m_patient));
    database.removeFromHistory(m_archivedIds);  // the appointments live on the patient again
}

// ---------------------------------------------------------------- edit patient
UndoEditPatient::UndoEditPatient(std::unique_ptr<model::PatientRecord> previous) : m_previous(std::move(previous)) {
    m_patientId = m_previous->id();
    m_name = m_previous->name();
}

std::string UndoEditPatient::description() const { return "Edit patient " + m_patientId + " (" + m_name + ")"; }

void UndoEditPatient::undo(model::ClinicDatabase& database) {
    const model::PatientRecord* current = database.findPatient(m_patientId);
    if (!current) throw ClinicException(ErrorCode::UndoUnavailable, "The patient no longer exists.");
    if (current->category() != m_previous->category())
        throw ClinicException(ErrorCode::UndoUnavailable, "The patient's category has changed.");
    m_previous->setAppointments(current->appointments());  // keep the current appointments
    database.replacePatient(m_patientId, std::move(m_previous));
}

// ---------------------------------------------------------------- remove appointment
UndoRemoveAppointment::UndoRemoveAppointment(model::Appointment appointment) : m_appointment(std::move(appointment)) {}

std::string UndoRemoveAppointment::description() const {
    return "Remove appointment " + m_appointment.id() + " (" + m_appointment.patientNameSnapshot() + ")";
}

void UndoRemoveAppointment::undo(model::ClinicDatabase& database) {
    model::PatientRecord* owner = database.findPatient(m_appointment.patientId());
    if (!owner) throw ClinicException(ErrorCode::UndoUnavailable, "The patient no longer exists.");
    owner->restoreRemovedAppointment(m_appointment);
}

// ---------------------------------------------------------------- cancel appointment
UndoCancelAppointment::UndoCancelAppointment(std::string appointmentId, std::string patientId, std::string patientName)
    : m_appointmentId(std::move(appointmentId)), m_patientId(std::move(patientId)), m_patientName(std::move(patientName)) {}

std::string UndoCancelAppointment::description() const {
    return "Cancel appointment " + m_appointmentId + " (" + m_patientName + ")";
}

void UndoCancelAppointment::undo(model::ClinicDatabase& database) {
    model::PatientRecord* owner = database.findPatient(m_patientId);
    if (!owner) throw ClinicException(ErrorCode::UndoUnavailable, "The patient no longer exists.");
    owner->reopenCancelledAppointment(m_appointmentId);
}

} // namespace scapms::controller
