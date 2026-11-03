#include "model/ClinicDatabase.h"

#include "utils/ClinicException.h"

#include <utility>

namespace scapms::model {

using utils::ClinicException;
using utils::ErrorCode;

PatientRecord* ClinicDatabase::findPatient(const std::string& id) {
    for (auto& p : m_patients)
        if (p->id() == id) return p.get();
    return nullptr;
}

const PatientRecord* ClinicDatabase::findPatient(const std::string& id) const {
    for (const auto& p : m_patients)
        if (p->id() == id) return p.get();
    return nullptr;
}

void ClinicDatabase::addPatient(PatientPtr patient) {
    if (!patient) throw ClinicException(ErrorCode::InvalidPatient, "Cannot add an empty patient record.");
    if (findPatient(patient->id()))
        throw ClinicException(ErrorCode::DuplicateId, "Patient ID already exists: " + patient->id());
    m_patients.push_back(std::move(patient));
    ++m_version;
}

ClinicDatabase::PatientPtr ClinicDatabase::removePatient(const std::string& id) {
    for (std::size_t i = 0; i < m_patients.size(); ++i) {
        if (m_patients[i]->id() == id) {
            notifyInvalidation(id);
            PatientPtr removed = std::move(m_patients[i]);
            m_patients.erase(m_patients.begin() + static_cast<std::ptrdiff_t>(i));
            ++m_version;
            return removed;
        }
    }
    throw ClinicException(ErrorCode::PatientNotFound, "Patient not found: " + id);
}

ClinicDatabase::PatientPtr ClinicDatabase::replacePatient(const std::string& id, PatientPtr replacement) {
    if (!replacement) throw ClinicException(ErrorCode::InvalidPatient, "Cannot replace with an empty record.");
    if (replacement->id() != id)
        throw ClinicException(ErrorCode::InvalidPatient, "A patient's ID cannot be changed.");
    for (auto& slot : m_patients) {
        if (slot->id() == id) {
            notifyInvalidation(id);
            PatientPtr old = std::move(slot);
            slot = std::move(replacement);
            ++m_version;
            return old;
        }
    }
    throw ClinicException(ErrorCode::PatientNotFound, "Patient not found: " + id);
}

void ClinicDatabase::setInvalidationCallback(std::function<void(const std::string&)> callback) {
    m_onInvalidate = std::move(callback);
}

void ClinicDatabase::notifyInvalidation(const std::string& id) {
    if (m_onInvalidate) m_onInvalidate(id);
}

void ClinicDatabase::addService(ClinicService service) {
    if (service.id.empty() || service.name.empty())
        throw ClinicException(ErrorCode::InvalidService, "A service needs an ID and a name.");
    if (findService(service.id))
        throw ClinicException(ErrorCode::DuplicateId, "Service ID already exists: " + service.id);
    m_services.push_back(std::move(service));
}

ClinicService* ClinicDatabase::findService(const std::string& id) {
    for (auto& s : m_services)
        if (s.id == id) return &s;
    return nullptr;
}

const ClinicService* ClinicDatabase::findService(const std::string& id) const {
    for (const auto& s : m_services)
        if (s.id == id) return &s;
    return nullptr;
}

void ClinicDatabase::archiveAppointments(const std::vector<Appointment>& appointments) {
    for (const auto& a : appointments) m_history.push_back(a);
}

void ClinicDatabase::removeFromHistory(const std::vector<std::string>& appointmentIds) {
    std::vector<Appointment> kept;
    for (const auto& a : m_history) {
        bool remove = false;
        for (const auto& id : appointmentIds)
            if (a.id() == id) { remove = true; break; }
        if (!remove) kept.push_back(a);
    }
    m_history = std::move(kept);
}

void ClinicDatabase::replaceContentsWith(ClinicDatabase&& other) {
    for (const auto& p : m_patients) notifyInvalidation(p->id());  // old objects are about to be destroyed
    m_patients = std::move(other.m_patients);
    m_services = std::move(other.m_services);
    m_history = std::move(other.m_history);
    other.m_patients.clear();
    other.m_services.clear();
    other.m_history.clear();
    ++m_version;
}

void ClinicDatabase::clear() {
    for (const auto& p : m_patients) notifyInvalidation(p->id());
    m_patients.clear();
    ++m_version;
    m_services.clear();
    m_history.clear();
}

} // namespace scapms::model
