#include "utils/IDGenerator.h"

#include <cstdio>
#include <stdexcept>

namespace scapms::utils {

namespace {
std::string format(char prefix, int number, int width) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%c-%0*d", prefix, width, number);
    return buf;
}
} // namespace

std::string IDGenerator::peekPatientId() const { return format('P', m_patient + 1, 4); }
std::string IDGenerator::nextPatientId() { return format('P', ++m_patient, 4); }
std::string IDGenerator::peekAppointmentId() const { return format('A', m_appointment + 1, 4); }
std::string IDGenerator::nextAppointmentId() { return format('A', ++m_appointment, 4); }
std::string IDGenerator::nextServiceId() { return format('S', ++m_service, 3); }

void IDGenerator::observe(const std::string& id) {
    if (id.size() < 3 || id[1] != '-') return;
    int n = 0;
    try {
        std::size_t used = 0;
        n = std::stoi(id.substr(2), &used);
        if (used != id.size() - 2) return;
    } catch (const std::exception&) {
        return;
    }
    switch (id[0]) {
        case 'P': if (n > m_patient) m_patient = n; break;
        case 'A': if (n > m_appointment) m_appointment = n; break;
        case 'S': if (n > m_service) m_service = n; break;
        default: break;
    }
}

void IDGenerator::reset() { m_patient = m_appointment = m_service = 0; }

} // namespace scapms::utils
