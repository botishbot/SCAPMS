#include "controller/AppointmentController.h"

#include "controller/UndoCommands.h"
#include "utils/ClinicException.h"
#include "utils/Validation.h"

#include <cctype>
#include <utility>

namespace scapms::controller {

using model::AppointmentStatus;
using model::PatientRecord;
using utils::ClinicException;
using utils::ErrorCode;
namespace v = utils::validation;

namespace {
std::string lower(const std::string& s) {
    std::string out = s;
    for (auto& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}
} // namespace

AppointmentController::AppointmentController(model::ClinicDatabase& database, utils::IDGenerator& idGenerator,
                                             UndoManager& undo)
    : m_database(database), m_ids(idGenerator), m_undo(undo) {}

// ---------------------------------------------------------------- services
std::string AppointmentController::addService(const std::string& name, const std::string& description) {
    const std::string n = v::collapseSpaces(name);
    if (n.empty()) throw ClinicException(ErrorCode::InvalidService, "Service name must not be blank.");
    if (n.size() > 60) throw ClinicException(ErrorCode::InvalidService, "Service name must not exceed 60 characters.");
    for (const auto& s : m_database.services())
        if (lower(s.name) == lower(n))
            throw ClinicException(ErrorCode::InvalidService, "A service named \"" + s.name + "\" already exists.");

    model::ClinicService service;
    service.id = m_ids.nextServiceId();
    service.name = n;
    service.description = v::collapseSpaces(description);
    service.active = true;
    m_database.addService(service);
    return service.id;
}

void AppointmentController::setServiceActive(const std::string& serviceId, bool active) {
    model::ClinicService* s = m_database.findService(serviceId);
    if (!s) throw ClinicException(ErrorCode::InvalidService, "Service not found: " + serviceId);
    s->active = active;  // existing appointments keep their service reference and name snapshot
}

std::vector<model::ClinicService> AppointmentController::activeServices() const {
    std::vector<model::ClinicService> out;
    for (const auto& s : m_database.services())
        if (s.active) out.push_back(s);
    return out;
}

// ---------------------------------------------------------------- scheduling
bool AppointmentController::hasSameTimeConflict(const std::string& patientId, const std::string& serviceId,
                                                const std::string& dateTime) const {
    const PatientRecord* patient = m_database.findPatient(patientId);
    if (!patient) return false;
    for (const auto& a : patient->appointments())
        if (a.status() != AppointmentStatus::Cancelled && a.dateTime() == dateTime && a.serviceId() != serviceId)
            return true;
    return false;
}

std::string AppointmentController::schedule(const std::string& patientId, const std::string& serviceId,
                                            const std::string& dateTime, const std::string& reason,
                                            const std::string& nowDateTime) {
    PatientRecord* patient = m_database.findPatient(patientId);
    if (!patient) throw ClinicException(ErrorCode::PatientNotFound, "Please select a patient.");

    model::ClinicService* service = m_database.findService(serviceId);
    if (!service) throw ClinicException(ErrorCode::InvalidService, "Please select a clinic service.");
    if (!service->active)
        throw ClinicException(ErrorCode::InvalidService, "The service \"" + service->name + "\" is no longer offered.");

    const std::string id = m_ids.peekAppointmentId();  // peek: a failed booking must not use up an ID
    model::Appointment appointment(id, service->id, service->name, dateTime, v::collapseSpaces(reason));

    // Runtime polymorphism: the actual patient type (General, Emergency or Follow-Up) applies its own rule.
    model::Appointable& appointable = *patient;
    appointable.scheduleAppointment(appointment, nowDateTime);

    m_ids.nextAppointmentId();  // commit
    return id;
}

// ---------------------------------------------------------------- lifecycle
PatientRecord* AppointmentController::findOwner(const std::string& appointmentId) const {
    for (const auto& p : m_database.patients())
        if (p->findAppointment(appointmentId)) return p.get();
    return nullptr;
}

void AppointmentController::cancel(const std::string& appointmentId) {
    PatientRecord* owner = findOwner(appointmentId);
    if (!owner) throw ClinicException(ErrorCode::AppointmentNotFound, "Appointment not found: " + appointmentId);
    model::Appointable& appointable = *owner;
    if (!appointable.cancelAppointment(appointmentId))
        throw ClinicException(ErrorCode::InvalidAppointment, "Only pending appointments can be cancelled.");
    m_undo.push(std::make_unique<UndoCancelAppointment>(appointmentId, owner->id(), owner->name()));
    if (m_onClosed) m_onClosed(appointmentId);
}

void AppointmentController::complete(const std::string& appointmentId, const std::string& outcomeNote) {
    PatientRecord* owner = findOwner(appointmentId);
    if (!owner) throw ClinicException(ErrorCode::AppointmentNotFound, "Appointment not found: " + appointmentId);
    if (!owner->completeAppointment(appointmentId, outcomeNote))
        throw ClinicException(ErrorCode::InvalidAppointment, "Only pending appointments can be marked as completed.");
    if (m_onClosed) m_onClosed(appointmentId);
}

void AppointmentController::remove(const std::string& appointmentIdArg) {
    const std::string appointmentId = appointmentIdArg;  // copy: the caller's string may live inside the appointment that is erased below
    PatientRecord* owner = findOwner(appointmentId);
    if (!owner) throw ClinicException(ErrorCode::AppointmentNotFound, "Appointment not found: " + appointmentId);
    auto removed = owner->removePendingAppointment(appointmentId);
    if (!removed)
        throw ClinicException(ErrorCode::InvalidAppointment,
                              "Only pending appointments can be removed. Completed and cancelled ones stay as history.");
    m_undo.push(std::make_unique<UndoRemoveAppointment>(*removed));
    if (m_onClosed) m_onClosed(appointmentId);
}

// ---------------------------------------------------------------- queries
bool AppointmentController::isOverdue(const model::Appointment& a, const std::string& now) {
    return a.status() == AppointmentStatus::Pending && a.dateTime() < now;
}

std::string AppointmentController::displayStatus(const model::Appointment& a, const std::string& now) {
    return isOverdue(a, now) ? "Overdue" : model::statusName(a.status());
}

std::vector<AppointmentRow> AppointmentController::allAppointments(const std::string& now) const {
    std::vector<AppointmentRow> rows;
    for (const auto& p : m_database.patients()) {
        for (const auto& a : p->appointments())
            rows.push_back({a, p->category(), isOverdue(a, now), displayStatus(a, now)});
    }
    // Earliest first: stable insertion sort (equal times keep their booking order).
    for (std::size_t i = 1; i < rows.size(); ++i) {
        AppointmentRow key = rows[i];
        std::size_t j = i;
        while (j > 0 && rows[j - 1].appointment.dateTime() > key.appointment.dateTime()) {
            rows[j] = rows[j - 1];
            --j;
        }
        rows[j] = key;
    }
    return rows;
}

int AppointmentController::countOnDate(const std::string& date) const {
    int n = 0;
    for (const auto& p : m_database.patients())
        for (const auto& a : p->appointments())
            if (a.status() != AppointmentStatus::Cancelled && v::datePart(a.dateTime()) == date) ++n;
    return n;
}

} // namespace scapms::controller
