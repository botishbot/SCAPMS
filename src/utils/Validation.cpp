#include "utils/Validation.h"
#include "utils/ClinicException.h"

#include <cctype>
#include <cstdio>
#include <stdexcept>

namespace scapms::utils::validation {

namespace {
bool isDigit(char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; }
bool isSpace(char c) { return std::isspace(static_cast<unsigned char>(c)) != 0; }

bool isLeap(int y) { return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0; }
int daysInMonth(int y, int m) {
    static const int d[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return (m == 2 && isLeap(y)) ? 29 : d[m - 1];
}
int num(const std::string& s, std::size_t pos, std::size_t len) {
    return std::stoi(s.substr(pos, len));
}
} // namespace

std::string trim(const std::string& s) {
    std::size_t b = 0, e = s.size();
    while (b < e && isSpace(s[b])) ++b;
    while (e > b && isSpace(s[e - 1])) --e;
    return s.substr(b, e - b);
}

std::string collapseSpaces(const std::string& s) {
    std::string out;
    bool pendingSpace = false;
    for (char c : trim(s)) {
        if (isSpace(c)) {
            pendingSpace = true;
        } else {
            if (pendingSpace) out.push_back(' ');
            pendingSpace = false;
            out.push_back(c);
        }
    }
    return out;
}

std::string toLower(const std::string& s) {
    std::string out = s;
    for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

std::string normalizedName(const std::string& name) { return toLower(collapseSpaces(name)); }

bool startsWith(const std::string& text, const std::string& prefix) {
    if (prefix.size() > text.size()) return false;
    for (std::size_t i = 0; i < prefix.size(); ++i)
        if (text[i] != prefix[i]) return false;
    return true;
}

bool contains(const std::string& text, const std::string& needle) {
    if (needle.size() > text.size()) return false;
    for (std::size_t start = 0; start + needle.size() <= text.size(); ++start) {
        std::size_t k = 0;
        while (k < needle.size() && text[start + k] == needle[k]) ++k;
        if (k == needle.size()) return true;
    }
    return false;
}

std::string normalizedIdKey(const std::string& text) {
    std::string t = toLower(trim(text));
    std::size_t i = 0;
    if (i < t.size() && t[i] == 'p') ++i;
    if (i < t.size() && t[i] == '-') ++i;
    if (i == t.size() || t.size() - i > 9) return t;
    for (std::size_t k = i; k < t.size(); ++k)
        if (!isDigit(t[k])) return t;
    char buf[32];
    std::snprintf(buf, sizeof buf, "p-%04d", std::stoi(t.substr(i)));
    return buf;
}

bool isValidDate(const std::string& s) {
    if (s.size() != 10 || s[4] != '-' || s[7] != '-') return false;
    for (std::size_t i : {0u, 1u, 2u, 3u, 5u, 6u, 8u, 9u})
        if (!isDigit(s[i])) return false;
    int y = num(s, 0, 4), m = num(s, 5, 2), d = num(s, 8, 2);
    if (y < 1900 || m < 1 || m > 12 || d < 1) return false;
    return d <= daysInMonth(y, m);
}

bool isValidDateTime(const std::string& s) {
    if (s.size() != 16 || s[10] != ' ' || s[13] != ':') return false;
    if (!isValidDate(s.substr(0, 10))) return false;
    if (!isDigit(s[11]) || !isDigit(s[12]) || !isDigit(s[14]) || !isDigit(s[15])) return false;
    return num(s, 11, 2) < 24 && num(s, 14, 2) < 60;
}

std::string datePart(const std::string& dateTime) {
    return dateTime.size() >= 10 ? dateTime.substr(0, 10) : dateTime;
}

namespace {
// Days since 1970-01-01 for a civil date (standard days-from-civil algorithm).
long daysFromCivil(int y, int m, int d) {
    y -= m <= 2;
    const long era = (y >= 0 ? y : y - 399) / 400;
    const long yoe = y - era * 400;
    const long doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}
long toMinutes(const std::string& dt) {
    const long days = daysFromCivil(num(dt, 0, 4), num(dt, 5, 2), num(dt, 8, 2));
    return days * 1440 + num(dt, 11, 2) * 60 + num(dt, 14, 2);
}
} // namespace

namespace {
// Inverse of daysFromCivil: civil date for a day count since 1970-01-01.
void civilFromDays(long z, int& y, int& m, int& d) {
    z += 719468;
    const long era = (z >= 0 ? z : z - 146096) / 146097;
    const long doe = z - era * 146097;
    const long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const long mp = (5 * doy + 2) / 153;
    d = static_cast<int>(doy - (153 * mp + 2) / 5 + 1);
    m = static_cast<int>(mp < 10 ? mp + 3 : mp - 9);
    y = static_cast<int>(yoe + era * 400 + (m <= 2));
}
} // namespace

std::string addDays(const std::string& date, int days) {
    if (!isValidDate(date)) throw ClinicException(ErrorCode::InvalidAppointment, "Invalid date.");
    int y = 0, m = 0, d = 0;
    civilFromDays(daysFromCivil(num(date, 0, 4), num(date, 5, 2), num(date, 8, 2)) + days, y, m, d);
    char buf[16];
    std::snprintf(buf, sizeof buf, "%04d-%02d-%02d", y, m, d);
    return buf;
}

long minutesBetween(const std::string& earlier, const std::string& later) {
    if (!isValidDateTime(earlier) || !isValidDateTime(later))
        throw ClinicException(ErrorCode::InvalidAppointment, "Invalid date/time.");
    return toMinutes(later) - toMinutes(earlier);
}

void requireName(const std::string& name) {
    const std::string n = collapseSpaces(name);
    if (n.empty())
        throw ClinicException(ErrorCode::InvalidPatient, "Patient name must not be blank.");
    if (n.size() > 80)
        throw ClinicException(ErrorCode::InvalidPatient, "Patient name must not exceed 80 characters.");
    for (char c : n)
        if (std::iscntrl(static_cast<unsigned char>(c)))
            throw ClinicException(ErrorCode::InvalidPatient, "Patient name contains invalid characters.");
}

void requireAge(int age) {
    if (age < kMinAge || age > kMaxAge)
        throw ClinicException(ErrorCode::InvalidAge, "Patient age must be between 0 and 120.");
}

int parseAge(const std::string& text) {
    const std::string t = trim(text);
    const std::string msg = "Patient age must be a whole number between 0 and 120.";
    if (t.empty()) throw ClinicException(ErrorCode::InvalidAge, msg);
    int value = 0;
    try {
        std::size_t used = 0;
        value = std::stoi(t, &used);
        if (used != t.size()) throw ClinicException(ErrorCode::InvalidAge, msg);
    } catch (const ClinicException&) {
        throw;
    } catch (const std::exception&) {  // invalid_argument / out_of_range
        throw ClinicException(ErrorCode::InvalidAge, msg);
    }
    requireAge(value);
    return value;
}

bool isValidPhone(const std::string& s) {
    const std::string t = trim(s);
    if (t.empty()) return false;
    int digits = 0;
    for (std::size_t i = 0; i < t.size(); ++i) {
        char c = t[i];
        if (isDigit(c)) ++digits;
        else if (c == '+' && i == 0) continue;
        else if (c == ' ' || c == '-' || c == '(' || c == ')') continue;
        else return false;
    }
    return digits >= 7 && digits <= 15;
}

bool isValidEmail(const std::string& s) {
    const std::string t = trim(s);
    const auto at = t.find('@');
    if (at == std::string::npos || at == 0 || t.find('@', at + 1) != std::string::npos) return false;
    for (char c : t)
        if (isSpace(c)) return false;
    const std::string domain = t.substr(at + 1);
    const auto dot = domain.find('.');
    return dot != std::string::npos && dot > 0 && domain.back() != '.';
}

void requireContact(const std::string& phone, const std::string& email) {
    const bool hasPhone = !trim(phone).empty();
    const bool hasEmail = !trim(email).empty();
    if (!hasPhone && !hasEmail)
        throw ClinicException(ErrorCode::InvalidContact,
                              "Enter at least one contact method (phone number or email address).");
    if (hasPhone && !isValidPhone(phone))
        throw ClinicException(ErrorCode::InvalidContact,
                              "Phone number is not valid. Use 7 to 15 digits (spaces, dashes and a leading + are allowed).");
    if (hasEmail && !isValidEmail(email))
        throw ClinicException(ErrorCode::InvalidContact, "Email address is not valid.");
}

} // namespace scapms::utils::validation
