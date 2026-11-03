#pragma once
#include "model/PatientRecord.h"

#include <string>
#include <vector>

namespace scapms::controller {

// Small cache of "Frequently Accessed Patient Records": a fixed-size array of 10 slots.
// Every time a patient's record is opened, their hit count goes up. When all 10 slots are full
// and a new patient is opened, the least-used patient is replaced.
// The slots hold non-owning pointers, so the database tells the cache (invalidate) just before
// a patient object is deleted or replaced.
class PatientCache {
public:
    static constexpr int kSize = 10;

    struct Entry {
        const model::PatientRecord* patient = nullptr;
        int hits = 0;
    };

    void recordAccess(const model::PatientRecord& patient);
    void invalidate(const std::string& patientId);  // forget one patient (deleted/replaced)
    void clear();

    int count() const;                               // slots in use
    bool contains(const std::string& patientId) const;
    int hitsFor(const std::string& patientId) const;  // 0 if not cached

    std::vector<Entry> mostAccessed() const;         // used slots, highest hit count first

private:
    Entry m_slots[kSize];  // the fixed-size array
};

} // namespace scapms::controller
