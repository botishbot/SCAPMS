#include "controller/QueueManager.h"

#include "utils/ClinicException.h"
#include "utils/Validation.h"

#include <utility>

namespace scapms::controller {

using model::AppointmentStatus;
using utils::ClinicException;
using utils::ErrorCode;

void QueueManager::join(const std::string& appointmentId, const std::string& nowDateTime) {
    if (!utils::validation::isValidDateTime(nowDateTime))
        throw ClinicException(ErrorCode::InvalidQueue, "The current date/time is not valid.");

    const model::PatientRecord* owner = nullptr;
    const model::Appointment* appointment = nullptr;
    for (const auto& p : m_database.patients()) {
        if (const model::Appointment* a = p->findAppointment(appointmentId)) {
            owner = p.get();
            appointment = a;
            break;
        }
    }
    if (!owner) throw ClinicException(ErrorCode::AppointmentNotFound, "Appointment not found: " + appointmentId);
    if (appointment->status() != AppointmentStatus::Pending)
        throw ClinicException(ErrorCode::InvalidQueue, "Only a patient with a pending appointment can join the queue.");
    if (isWaiting(owner->id()))
        throw ClinicException(ErrorCode::InvalidQueue, owner->name() + " is already in the waiting queue.");
    if (isBeingServed(owner->id()))
        throw ClinicException(ErrorCode::InvalidQueue, owner->name() + " is being served right now.");

    m_queue.push({owner->id(), appointmentId, nowDateTime});
}

QueueEntry QueueManager::callNext() {
    if (m_serving)
        throw ClinicException(ErrorCode::InvalidQueue,
                              "Finish or release the current consultation before calling the next patient.");
    if (m_queue.empty()) throw ClinicException(ErrorCode::InvalidQueue, "The waiting queue is empty.");
    m_serving = m_queue.front();
    m_queue.pop();
    return *m_serving;
}

void QueueManager::release() {
    if (!m_serving) throw ClinicException(ErrorCode::InvalidQueue, "No patient is being served.");
    m_serving.reset();
}

void QueueManager::remove(const std::string& patientId) {
    // std::queue has no erase: rebuild it without the leaving patient (order of the others is kept).
    std::queue<QueueEntry> rebuilt;
    bool found = false;
    while (!m_queue.empty()) {
        if (m_queue.front().patientId == patientId) found = true;
        else rebuilt.push(m_queue.front());
        m_queue.pop();
    }
    m_queue = std::move(rebuilt);
    if (!found) throw ClinicException(ErrorCode::InvalidQueue, "That patient is not in the waiting queue.");
}

void QueueManager::clear() {
    m_queue = std::queue<QueueEntry>();
    m_serving.reset();
}

void QueueManager::onAppointmentClosed(const std::string& appointmentId) {
    if (m_serving && m_serving->appointmentId == appointmentId) m_serving.reset();
    std::queue<QueueEntry> rebuilt;
    while (!m_queue.empty()) {
        if (m_queue.front().appointmentId != appointmentId) rebuilt.push(m_queue.front());
        m_queue.pop();
    }
    m_queue = std::move(rebuilt);
}

std::vector<QueueEntry> QueueManager::waiting() const {
    std::vector<QueueEntry> out;
    std::queue<QueueEntry> copy = m_queue;  // walk a copy so the real queue is untouched
    while (!copy.empty()) {
        out.push_back(copy.front());
        copy.pop();
    }
    return out;
}

bool QueueManager::isWaiting(const std::string& patientId) const { return position(patientId) > 0; }

bool QueueManager::isBeingServed(const std::string& patientId) const {
    return m_serving && m_serving->patientId == patientId;
}

int QueueManager::position(const std::string& patientId) const {
    int pos = 0;
    for (const auto& e : waiting()) {
        ++pos;
        if (e.patientId == patientId) return pos;
    }
    return 0;
}

} // namespace scapms::controller
