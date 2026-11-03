#pragma once
#include "model/ClinicDatabase.h"
#include "utils/IDGenerator.h"

#include <string>

namespace scapms::controller {

// Fictional sample clinic data for demonstrations and screenshots.
// Appointment dates are calculated from `today` ("YYYY-MM-DD") so the demo always looks current:
// some completed visits in the past, some today, some upcoming, and a cancelled one.
class DemoData {
public:
    static void populate(model::ClinicDatabase& database, utils::IDGenerator& ids, const std::string& today);
};

} // namespace scapms::controller
