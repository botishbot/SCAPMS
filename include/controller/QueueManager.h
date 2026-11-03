#pragma once
#include "model/ClinicDatabase.h"

#include <optional>
#include <queue>
#include <string>
#include <vector>

namespace scapms::controller {

// One waiting patient. Stored by ID instead of by pointer, so it stays valid when records change.
struct QueueEntry {
    std::string patientId;
    std::string appointmentId;
    std::string joinedAt;  // "YYYY-MM-DD HH:MM", used to show how long they have waited
};

// FIFO waiting queue built on std::queue. First to join is first to be called; emergency
// patients are highlighted by the GUI but do not jump the queue.
// The queue lives in memory only (it is not saved to the data file).
class QueueManager {
public:
    explicit QueueManager(const model::ClinicDatabase& database) : m_database(database) {}

    // Adds the patient who owns this Pending appointment. Throws ClinicException on bad requests.
    void join(const std::string& appointmentId, const std::string& nowDateTime);

    QueueEntry callNext();   // front of the queue becomes "now serving"
    void release();          // stop serving without completing (appointment stays Pending)
    void remove(const std::string& patientId);  // a waiting patient leaves the queue
    void clear();

    // Called when an appointment is cancelled, completed or removed: drops it from the queue
    // or from "now serving", so the queue only refers to open appointments.
    void onAppointmentClosed(const std::string& appointmentId);

    std::vector<QueueEntry> waiting() const;  // snapshot, front first
    std::size_t size() const { return m_queue.size(); }
    const std::optional<QueueEntry>& nowServing() const { return m_serving; }
    bool isWaiting(const std::string& patientId) const;
    bool isBeingServed(const std::string& patientId) const;
    int position(const std::string& patientId) const;  // 1-based, 0 if not waiting

private:
    const model::ClinicDatabase& m_database;
    std::queue<QueueEntry> m_queue;
    std::optional<QueueEntry> m_serving;
};

} // namespace scapms::controller
