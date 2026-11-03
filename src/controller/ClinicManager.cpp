#include "controller/ClinicManager.h"

#include "model/EmergencyPatient.h"
#include "model/FollowUpPatient.h"
#include "model/GeneralPatient.h"
#include "controller/UndoCommands.h"
#include "utils/ClinicException.h"

#include <utility>

namespace scapms::controller {

using model::PatientCategory;
using model::PatientRecord;
using utils::ClinicException;
using utils::ErrorCode;

ClinicManager::ClinicManager(model::ClinicDatabase& database, utils::IDGenerator& idGenerator, UndoManager& undo)
    : m_database(database), m_ids(idGenerator), m_undo(undo) {}

ClinicManager::PatientPtr ClinicManager::build(const std::string& id, const PatientInput& in) {
    // The constructors validate every field, so a bad input throws here and nothing is stored.
    switch (in.category) {
        case PatientCategory::General:
            return std::make_unique<model::GeneralPatient>(id, in.name, in.age, in.phone, in.email);
        case PatientCategory::Emergency:
            return std::make_unique<model::EmergencyPatient>(id, in.name, in.age, in.phone, in.email,
                                                             in.triageLevel);
        case PatientCategory::FollowUp:
            return std::make_unique<model::FollowUpPatient>(id, in.name, in.age, in.phone, in.email,
                                                            in.previousVisitDate, in.followUpReason);
    }
    throw ClinicException(ErrorCode::InvalidPatient, "Unknown patient category.");
}

std::string ClinicManager::registerPatient(const PatientInput& input) {
    // Peek first so a failed registration does not use up an ID.
    const std::string id = m_ids.peekPatientId();
    PatientPtr patient = build(id, input);
    m_database.addPatient(std::move(patient));
    m_ids.nextPatientId();  // commit the ID
    return id;
}

void ClinicManager::editPatient(const std::string& id, const PatientInput& input) {
    const PatientRecord* current = m_database.findPatient(id);
    if (!current) throw ClinicException(ErrorCode::PatientNotFound, "Patient not found: " + id);
    if (current->category() != input.category)
        throw ClinicException(ErrorCode::InvalidPatient,
                              "A patient's category cannot be changed. Delete and re-register instead.");

    PatientPtr updated = build(id, input);
    updated->setAppointments(current->appointments());  // keep the patient's appointments
    PatientPtr previous = m_database.replacePatient(id, std::move(updated));
    m_undo.push(std::make_unique<UndoEditPatient>(std::move(previous)));
}

void ClinicManager::deletePatient(const std::string& id) {
    const PatientRecord* patient = m_database.findPatient(id);
    if (!patient) throw ClinicException(ErrorCode::PatientNotFound, "Patient not found: " + id);
    if (patient->hasPendingAppointments())
        throw ClinicException(ErrorCode::PendingAppointments,
                              "Cannot delete " + patient->name() + ": " +
                                  std::to_string(patient->pendingCount()) +
                                  " pending appointment(s). Cancel or complete them first.");

    // Only Completed/Cancelled appointments remain: keep them as history with a name snapshot.
    std::vector<model::Appointment> history = patient->appointments();
    for (auto& a : history) a.setPatient(patient->id(), patient->name());
    std::vector<std::string> archivedIds;
    for (const auto& a : history) archivedIds.push_back(a.id());
    m_database.archiveAppointments(history);
    PatientPtr removed = m_database.removePatient(id);
    m_undo.push(std::make_unique<UndoDeletePatient>(std::move(removed), std::move(archivedIds)));
}

const PatientRecord& ClinicManager::getPatient(const std::string& id) const {
    const PatientRecord* p = m_database.findPatient(id);
    if (!p) throw ClinicException(ErrorCode::PatientNotFound, "Patient not found: " + id);
    return *p;
}

std::string ClinicManager::processPatient(const PatientRecord& patient) {
    // categoryName(), priorityLabel() and summary() are virtual: the version that runs
    // depends on the real object type (General / Emergency / Follow-Up).
    return "[" + patient.id() + "] " + patient.name() + " (" + patient.categoryName() + ", " +
           patient.priorityLabel() + ") - " + patient.summary();
}

std::vector<std::string> ClinicManager::processAllPatients() const {
    std::vector<std::string> lines;
    for (const auto& p : m_database.patients()) lines.push_back(processPatient(*p));
    return lines;
}

int ClinicManager::countByCategory(PatientCategory category) const {
    int n = 0;
    for (const auto& p : m_database.patients())
        if (p->category() == category) ++n;
    return n;
}

int ClinicManager::pendingAppointmentCount() const {
    int n = 0;
    for (const auto& p : m_database.patients()) n += p->pendingCount();
    return n;
}

} // namespace scapms::controller
