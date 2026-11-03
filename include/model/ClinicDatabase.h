#pragma once
#include "model/Appointment.h"
#include "model/ClinicService.h"
#include "model/PatientRecord.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace scapms::model {

// Owns all clinic data: the patients (std::vector of heap-owned polymorphic objects),
// the service catalogue, and the history of appointments that belonged to deleted patients.
// Other classes get controlled access; the GUI pages do not modify these collections directly.
class ClinicDatabase {
public:
    using PatientPtr = std::unique_ptr<PatientRecord>;

    // ---- patients ----
    const std::vector<PatientPtr>& patients() const { return m_patients; }
    std::size_t patientCount() const { return m_patients.size(); }

    // Increases whenever a patient is added, removed or replaced. Lists that hold pointers to patients
    // (the search/sort view) compare it to know when they must be rebuilt.
    unsigned long patientsVersion() const { return m_version; }

    // Plain lookups used internally (duplicate-ID checks, edit, delete).
    // The Linear, Binary and Recursive searches are implemented in SearchEngine.
    PatientRecord* findPatient(const std::string& id);
    const PatientRecord* findPatient(const std::string& id) const;

    void addPatient(PatientPtr patient);                       // throws on null or duplicate ID
    PatientPtr removePatient(const std::string& id);           // returns the removed object
    PatientPtr replacePatient(const std::string& id, PatientPtr replacement);  // same ID required

    // Called with a patient's ID just before that object is destroyed or replaced,
    // so holders of plain pointers (the patient cache) can drop theirs.
    void setInvalidationCallback(std::function<void(const std::string&)> callback);

    // ---- services ----
    const std::vector<ClinicService>& services() const { return m_services; }
    void addService(ClinicService service);                    // throws on blank or duplicate ID
    ClinicService* findService(const std::string& id);
    const ClinicService* findService(const std::string& id) const;

    // ---- history of appointments from deleted patients ----
    const std::vector<Appointment>& history() const { return m_history; }
    void archiveAppointments(const std::vector<Appointment>& appointments);
    void removeFromHistory(const std::vector<std::string>& appointmentIds);

    void clear();

    // Replaces everything with the contents of `other` (used by Load, New and demo data after the new data
    // has been fully built and checked). Tells the invalidation callback about every old patient first.
    void replaceContentsWith(ClinicDatabase&& other);

private:
    void notifyInvalidation(const std::string& id);

    unsigned long m_version = 0;
    std::vector<PatientPtr> m_patients;
    std::vector<ClinicService> m_services;
    std::vector<Appointment> m_history;
    std::function<void(const std::string&)> m_onInvalidate;
};

} // namespace scapms::model
