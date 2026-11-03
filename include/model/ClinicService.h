#pragma once
#include <string>

namespace scapms::model {

// A bookable clinic service. Services are deactivated, never erased, so old
// appointments keep a valid service reference.
struct ClinicService {
    std::string id;
    std::string name;
    std::string description;
    bool active = true;
};

} // namespace scapms::model
