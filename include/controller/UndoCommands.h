#pragma once
#include "controller/UndoManager.h"
#include "model/Appointment.h"
#include "model/PatientRecord.h"

#include <memory>
#include <string>
#include <vector>

namespace scapms::controller {

// Delete patient: puts the patient back and removes the history copies made at deletion.
class UndoDeletePatient : public UndoCommand {
public:
    UndoDeletePatient(std::unique_ptr<model::PatientRecord> patient, std::vector<std::string> archivedAppointmentIds);
    std::string description() const override;
    void undo(model::ClinicDatabase& database) override;

private:
    std::unique_ptr<model::PatientRecord> m_patient;
    std::vector<std::string> m_archivedIds;
    std::string m_patientId;
    std::string m_name;
};

// Edit patient: restores the previous personal data. Appointments are NOT rolled back,
// so bookings made after the edit are kept.
class UndoEditPatient : public UndoCommand {
public:
    explicit UndoEditPatient(std::unique_ptr<model::PatientRecord> previous);
    std::string description() const override;
    void undo(model::ClinicDatabase& database) override;

private:
    std::unique_ptr<model::PatientRecord> m_previous;
    std::string m_patientId;
    std::string m_name;
};

// Remove appointment: puts the removed Pending appointment back (not back into the waiting queue).
class UndoRemoveAppointment : public UndoCommand {
public:
    explicit UndoRemoveAppointment(model::Appointment appointment);
    std::string description() const override;
    void undo(model::ClinicDatabase& database) override;

private:
    model::Appointment m_appointment;
};

// Cancel appointment: sets it back to Pending (not back into the waiting queue).
class UndoCancelAppointment : public UndoCommand {
public:
    UndoCancelAppointment(std::string appointmentId, std::string patientId, std::string patientName);
    std::string description() const override;
    void undo(model::ClinicDatabase& database) override;

private:
    std::string m_appointmentId;
    std::string m_patientId;
    std::string m_patientName;
};

} // namespace scapms::controller
