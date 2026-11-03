#pragma once
// Tiny test helpers so the project needs no external test framework.
#include "utils/ClinicException.h"

#include <iostream>
#include <string>

namespace testing {
inline int& passed() { static int n = 0; return n; }
inline int& failed() { static int n = 0; return n; }
inline void report(bool ok, const std::string& what, const char* file, int line) {
    if (ok) { ++passed(); return; }
    ++failed();
    std::cerr << "FAIL " << file << ":" << line << "  " << what << "\n";
}
} // namespace testing

#define CHECK(cond) testing::report((cond), #cond, __FILE__, __LINE__)
#define CHECK_EQ(a, b) testing::report(((a) == (b)), #a " == " #b, __FILE__, __LINE__)

// Passes only if `code` throws ClinicException with the expected ErrorCode.
#define CHECK_THROWS(stmt, expected)                                                        \
    do {                                                                                    \
        bool ok_ = false;                                                                   \
        try { stmt; } catch (const scapms::utils::ClinicException& e_) {                    \
            ok_ = (e_.code() == scapms::utils::ErrorCode::expected);                        \
        } catch (...) {}                                                                    \
        testing::report(ok_, "throws " #expected ": " #stmt, __FILE__, __LINE__);           \
    } while (0)
