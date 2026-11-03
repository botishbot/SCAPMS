#pragma once
#include "model/ClinicDatabase.h"

#include <string>
#include <vector>

namespace scapms::controller {

struct FrequentPatientRow { std::string id, name, category; int completed = 0; };
struct PendingPatientRow { std::string id, name, category; int pending = 0; std::string nextDateTime; };
struct CategoryRow { std::string name; int count = 0; int percent = 0; };
struct DailySummary { std::string date; int total = 0, completed = 0, pending = 0, cancelled = 0; };

// Builds the clinic reports from the data. Reports are always calculated fresh, never stored.
class ReportGenerator {
public:
    explicit ReportGenerator(const model::ClinicDatabase& database) : m_database(database) {}

    // 1. Patients with the most completed appointments (highest first, ties by name). Patients with none are left out.
    std::vector<FrequentPatientRow> mostFrequentlyAttended(std::size_t limit = 5) const;

    // 2. Patients with at least one pending appointment, ordered by their next appointment.
    std::vector<PendingPatientRow> patientsWithPending() const;

    // 3. How many patients are in each category.
    std::vector<CategoryRow> categoryDistribution() const;

    // 4. Appointments on one day ("YYYY-MM-DD"), including those of deleted patients. "Pending" includes overdue ones.
    DailySummary dailySummary(const std::string& date) const;

    // 5. One line per patient, produced by calling the same virtual functions on every patient type.
    std::vector<std::string> patientOverview() const;

    // All reports as plain text (for export).
    std::string renderText(const std::string& date) const;

private:
    const model::ClinicDatabase& m_database;
};

} // namespace scapms::controller
