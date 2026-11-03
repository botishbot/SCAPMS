#include "controller/ReportGenerator.h"

#include "controller/ClinicManager.h"

#include <utility>

namespace scapms::controller {

using model::AppointmentStatus;
using model::PatientCategory;

namespace {

// Stable insertion sort used to order report rows.
template <class T, class Before>
void insertionSort(std::vector<T>& rows, Before before) {
    for (std::size_t i = 1; i < rows.size(); ++i) {
        T key = rows[i];
        std::size_t j = i;
        while (j > 0 && before(key, rows[j - 1])) {
            rows[j] = rows[j - 1];
            --j;
        }
        rows[j] = key;
    }
}

} // namespace

std::vector<FrequentPatientRow> ReportGenerator::mostFrequentlyAttended(std::size_t limit) const {
    std::vector<FrequentPatientRow> rows;
    for (const auto& p : m_database.patients())
        if (p->completedCount() > 0) rows.push_back({p->id(), p->name(), p->categoryName(), p->completedCount()});
    insertionSort(rows, [](const FrequentPatientRow& a, const FrequentPatientRow& b) {
        if (a.completed != b.completed) return a.completed > b.completed;
        return a.name < b.name;
    });
    if (rows.size() > limit) rows.resize(limit);
    return rows;
}

std::vector<PendingPatientRow> ReportGenerator::patientsWithPending() const {
    std::vector<PendingPatientRow> rows;
    for (const auto& p : m_database.patients()) {
        if (!p->hasPendingAppointments()) continue;
        std::string next;
        for (const auto& a : p->appointments())
            if (a.status() == AppointmentStatus::Pending && (next.empty() || a.dateTime() < next)) next = a.dateTime();
        rows.push_back({p->id(), p->name(), p->categoryName(), p->pendingCount(), next});
    }
    insertionSort(rows, [](const PendingPatientRow& a, const PendingPatientRow& b) {
        if (a.nextDateTime != b.nextDateTime) return a.nextDateTime < b.nextDateTime;
        return a.name < b.name;
    });
    return rows;
}

std::vector<CategoryRow> ReportGenerator::categoryDistribution() const {
    int counts[3] = {0, 0, 0};
    for (const auto& p : m_database.patients()) {
        switch (p->category()) {
            case PatientCategory::General: ++counts[0]; break;
            case PatientCategory::Emergency: ++counts[1]; break;
            case PatientCategory::FollowUp: ++counts[2]; break;
        }
    }
    const int total = counts[0] + counts[1] + counts[2];
    const char* names[3] = {"General Patients", "Emergency Patients", "Follow-Up Patients"};
    std::vector<CategoryRow> rows;
    for (int i = 0; i < 3; ++i)
        rows.push_back({names[i], counts[i], total > 0 ? (counts[i] * 100 + total / 2) / total : 0});
    return rows;
}

DailySummary ReportGenerator::dailySummary(const std::string& date) const {
    DailySummary s;
    s.date = date;
    auto count = [&](const model::Appointment& a) {
        if (a.dateTime().compare(0, 10, date) != 0) return;
        ++s.total;
        switch (a.status()) {
            case AppointmentStatus::Completed: ++s.completed; break;
            case AppointmentStatus::Pending: ++s.pending; break;
            case AppointmentStatus::Cancelled: ++s.cancelled; break;
        }
    };
    for (const auto& p : m_database.patients())
        for (const auto& a : p->appointments()) count(a);
    for (const auto& a : m_database.history()) count(a);  // patients deleted since still count for that day
    return s;
}

std::vector<std::string> ReportGenerator::patientOverview() const {
    std::vector<std::string> lines;
    for (const auto& p : m_database.patients())
        lines.push_back(ClinicManager::processPatient(*p));  // runtime polymorphism: one call, three behaviours
    return lines;
}

std::string ReportGenerator::renderText(const std::string& date) const {
    std::string out = "SCAPMS CLINIC REPORTS\n=====================\n\n";

    out += "1. Most Frequently Attended Patients\n";
    const auto freq = mostFrequentlyAttended(10);
    if (freq.empty()) out += "   (no completed appointments yet)\n";
    int rank = 0;
    for (const auto& r : freq)
        out += "   " + std::to_string(++rank) + ". " + r.name + " (" + r.id + ", " + r.category + ") - " +
               std::to_string(r.completed) + " completed\n";

    out += "\n2. Patients with Pending Appointments\n";
    const auto pending = patientsWithPending();
    if (pending.empty()) out += "   (none)\n";
    for (const auto& r : pending)
        out += "   " + r.name + " (" + r.id + ") - " + std::to_string(r.pending) + " pending, next " + r.nextDateTime + "\n";

    out += "\n3. Patient Category Distribution\n";
    for (const auto& r : categoryDistribution())
        out += "   " + r.name + ": " + std::to_string(r.count) + " (" + std::to_string(r.percent) + "%)\n";

    const DailySummary d = dailySummary(date);
    out += "\n4. Daily Appointment Summary for " + d.date + "\n";
    out += "   Total Appointments: " + std::to_string(d.total) + "\n";
    out += "   Completed: " + std::to_string(d.completed) + "\n";
    out += "   Pending: " + std::to_string(d.pending) + "\n";
    out += "   Cancelled: " + std::to_string(d.cancelled) + "\n";

    out += "\n5. Patient Overview (processed through the common PatientRecord interface)\n";
    const auto overview = patientOverview();
    if (overview.empty()) out += "   (no patients)\n";
    for (const auto& line : overview) out += "   " + line + "\n";
    return out;
}

} // namespace scapms::controller
