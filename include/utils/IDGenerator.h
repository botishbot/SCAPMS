#pragma once
#include <string>

namespace scapms::utils {

// Generates human-readable, zero-padded IDs: P-0001, A-0001, S-001.
// Zero padding keeps text order equal to numeric order (useful for sorting by ID).
// After loading data, call observe() for every loaded ID so new IDs never collide.
class IDGenerator {
public:
    std::string peekPatientId() const;
    std::string nextPatientId();
    std::string peekAppointmentId() const;
    std::string nextAppointmentId();
    std::string nextServiceId();

    void observe(const std::string& id);  // raises the counter if id is P-/A-/S- numbered
    void reset();

private:
    int m_patient = 0;
    int m_appointment = 0;
    int m_service = 0;
};

} // namespace scapms::utils
