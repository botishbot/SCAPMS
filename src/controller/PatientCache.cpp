#include "controller/PatientCache.h"

namespace scapms::controller {

void PatientCache::recordAccess(const model::PatientRecord& patient) {
    int empty = -1, leastUsed = 0;
    for (int i = 0; i < kSize; ++i) {
        if (m_slots[i].patient == &patient) {  // already cached: one more hit
            ++m_slots[i].hits;
            return;
        }
        if (m_slots[i].patient == nullptr) {
            if (empty < 0) empty = i;
        } else if (m_slots[leastUsed].patient == nullptr || m_slots[i].hits < m_slots[leastUsed].hits) {
            leastUsed = i;
        }
    }
    const int target = (empty >= 0) ? empty : leastUsed;  // free slot first, otherwise replace the least used
    m_slots[target].patient = &patient;
    m_slots[target].hits = 1;
}

void PatientCache::invalidate(const std::string& patientId) {
    for (int i = 0; i < kSize; ++i)
        if (m_slots[i].patient && m_slots[i].patient->id() == patientId) m_slots[i] = Entry();
}

void PatientCache::clear() {
    for (int i = 0; i < kSize; ++i) m_slots[i] = Entry();
}

int PatientCache::count() const {
    int n = 0;
    for (int i = 0; i < kSize; ++i)
        if (m_slots[i].patient) ++n;
    return n;
}

bool PatientCache::contains(const std::string& patientId) const { return hitsFor(patientId) > 0; }

int PatientCache::hitsFor(const std::string& patientId) const {
    for (int i = 0; i < kSize; ++i)
        if (m_slots[i].patient && m_slots[i].patient->id() == patientId) return m_slots[i].hits;
    return 0;
}

std::vector<PatientCache::Entry> PatientCache::mostAccessed() const {
    std::vector<Entry> used;
    for (int i = 0; i < kSize; ++i)
        if (m_slots[i].patient) used.push_back(m_slots[i]);
    // insertion sort by hits, highest first (stable: equal hits keep slot order)
    for (std::size_t i = 1; i < used.size(); ++i) {
        Entry key = used[i];
        std::size_t j = i;
        while (j > 0 && used[j - 1].hits < key.hits) {
            used[j] = used[j - 1];
            --j;
        }
        used[j] = key;
    }
    return used;
}

} // namespace scapms::controller
