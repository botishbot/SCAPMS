#pragma once
#include "model/ClinicDatabase.h"
#include "utils/IDGenerator.h"

#include <string>

namespace scapms::controller {

// Fictional sample clinic data for demonstrations and screenshots.
// Appointment dates are calculated from `today` ("YYYY-MM-DD") so the demo always looks current:
// some completed visits in the past, some today, some upcoming, and some cancelled.
// The basic set has 10 patients; with `large` set it has 30 patients (the first 10 are the same).
class DemoData {
public:
    static void populate(model::ClinicDatabase& database, utils::IDGenerator& ids, const std::string& today, bool large = false);
};

} // namespace scapms::controller
