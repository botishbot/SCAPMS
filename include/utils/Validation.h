#pragma once
#include <string>

namespace scapms::utils::validation {

// Text helpers
std::string trim(const std::string& s);
std::string collapseSpaces(const std::string& s);  // trim + single inner spaces
std::string toLower(const std::string& s);          // ASCII lower-case copy
std::string normalizedName(const std::string& name);  // collapseSpaces + toLower: the key used to sort and search names
bool startsWith(const std::string& text, const std::string& prefix);  // plain character-by-character check

// Date/time format used everywhere in the model: "YYYY-MM-DD" and "YYYY-MM-DD HH:MM".
// ISO order means plain string comparison is also chronological comparison.
bool isValidDate(const std::string& s);
bool isValidDateTime(const std::string& s);
std::string datePart(const std::string& dateTime);

// The date `days` days after `date` (negative = before). Both "YYYY-MM-DD". Throws on an invalid date.
std::string addDays(const std::string& date, int days);

// Minutes from `earlier` to `later` (both "YYYY-MM-DD HH:MM"). Negative if later is before earlier.
// Throws ClinicException(InvalidAppointment) if either value is not a valid date/time.
long minutesBetween(const std::string& earlier, const std::string& later);

// Field checks: each throws ClinicException with a user-readable message.
void requireName(const std::string& name);
void requireAge(int age);
int parseAge(const std::string& text);  // "abc", "12.5", "-3", "200" all throw
void requireContact(const std::string& phone, const std::string& email);

bool isValidPhone(const std::string& s);
bool isValidEmail(const std::string& s);

constexpr int kMinAge = 0;
constexpr int kMaxAge = 120;

} // namespace scapms::utils::validation
