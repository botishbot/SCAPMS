#pragma once
#include <stdexcept>
#include <string>

namespace scapms::utils {

// Categories of recoverable domain/file errors. GUI handlers catch
// ClinicException and show e.what() to the user.
enum class ErrorCode {
    InvalidPatient,
    InvalidAge,
    InvalidContact,
    DuplicateId,
    PatientNotFound,
    PendingAppointments,
    InvalidAppointment,
    AppointmentNotFound,
    DuplicateAppointment,
    InvalidSearch,
    FileError,
    UndoUnavailable,
    InvalidQueue,
    InvalidService
};

class ClinicException : public std::runtime_error {
public:
    ClinicException(ErrorCode code, const std::string& message)
        : std::runtime_error(message), m_code(code) {}

    ErrorCode code() const noexcept { return m_code; }

private:
    ErrorCode m_code;
};

} // namespace scapms::utils
