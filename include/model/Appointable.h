#pragma once
#include <string>

namespace scapms::model {

class Appointment;

// Interface-like abstract class: operations for anything that can hold appointments.
// Every concrete patient type implements these with its OWN scheduling rules,
// which is where runtime polymorphism shows up in the scheduling workflow.
class Appointable {
public:
    virtual ~Appointable() = default;

    // Adds a Pending appointment if this patient type's rules allow it.
    // nowDateTime is the current time as "YYYY-MM-DD HH:MM" (passed in so it can be tested).
    // Throws ClinicException(InvalidAppointment / DuplicateAppointment) on failure.
    virtual void scheduleAppointment(const Appointment& appointment, const std::string& nowDateTime) = 0;

    // Cancels a Pending appointment. Returns false if it is missing or not Pending.
    virtual bool cancelAppointment(const std::string& appointmentId) = 0;
};

} // namespace scapms::model
