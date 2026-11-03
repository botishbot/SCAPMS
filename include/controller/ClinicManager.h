#pragma once
#include "controller/UndoManager.h"
#include "model/ClinicDatabase.h"
#include "model/PatientRecord.h"
#include "utils/IDGenerator.h"

#include <memory>
#include <string>
#include <vector>

namespace scapms::controller {

// Plain data the GUI collects from the registration/edit form.
struct PatientInput {
    model::PatientCategory category = model::PatientCategory::General;
    std::string name;
    int age = 0;
    std::string phone;
    std::string email;
    int triageLevel = 2;             // Emergency only
    std::string previousVisitDate;   // Follow-Up only, "YYYY-MM-DD"
    std::string followUpReason;      // Follow-Up only
};

// Business rules for patients: register, edit, delete, query.
// The GUI calls this class; it never touches the database collections itself.
class ClinicManager {
public:
    using PatientPtr = model::ClinicDatabase::PatientPtr;

    ClinicManager(model::ClinicDatabase& database, utils::IDGenerator& idGenerator, UndoManager& undo);

    // Returns the new patient's ID. Throws ClinicException when input is invalid.
    std::string registerPatient(const PatientInput& input);

    // Replaces the patient with a validated version (same ID, appointments kept).
    // The category cannot change after registration. Pushes an undo entry.
    void editPatient(const std::string& id, const PatientInput& input);

    // Blocked while any appointment is Pending. Completed/cancelled appointments are copied to
    // the history (with the patient's name snapshot) first. Pushes an undo entry.
    void deletePatient(const std::string& id);

    const model::PatientRecord& getPatient(const std::string& id) const;  // throws if missing
    const std::vector<PatientPtr>& patients() const { return m_database.patients(); }

    // Runtime polymorphism: works on ANY patient type through the base-class reference.
    static std::string processPatient(const model::PatientRecord& patient);
    std::vector<std::string> processAllPatients() const;

    // Dashboard numbers
    int patientCount() const { return static_cast<int>(m_database.patientCount()); }
    unsigned long patientsVersion() const { return m_database.patientsVersion(); }
    int countByCategory(model::PatientCategory category) const;
    int pendingAppointmentCount() const;

private:
    static PatientPtr build(const std::string& id, const PatientInput& input);

    model::ClinicDatabase& m_database;
    utils::IDGenerator& m_ids;
    UndoManager& m_undo;
};

} // namespace scapms::controller
