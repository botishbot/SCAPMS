#include "controller/DemoData.h"

#include "model/EmergencyPatient.h"
#include "model/FollowUpPatient.h"
#include "model/GeneralPatient.h"
#include "utils/Validation.h"

#include <memory>
#include <vector>

namespace scapms::controller {

using model::Appointment;
using model::AppointmentStatus;
namespace v = utils::validation;

void DemoData::populate(model::ClinicDatabase& db, utils::IDGenerator& ids, const std::string& today) {
    // ---- services ----
    const std::vector<std::pair<std::string, std::string>> serviceList = {
        {"General Consultation", "Routine consultation with a doctor"},
        {"Laboratory Test", "Blood and urine tests"},
        {"X-Ray", "Radiology imaging"},
        {"Pharmacy Review", "Medication review and refill"}};
    std::vector<model::ClinicService> services;
    for (const auto& s : serviceList) {
        model::ClinicService svc;
        svc.id = ids.nextServiceId();
        svc.name = s.first;
        svc.description = s.second;
        db.addService(svc);
        services.push_back(svc);
    }

    // ---- patients ----
    auto add = [&](std::unique_ptr<model::PatientRecord> p) -> model::PatientRecord* {
        model::PatientRecord* raw = p.get();
        db.addPatient(std::move(p));
        return raw;
    };
    auto pid = [&] { return ids.nextPatientId(); };
    auto* amara = add(std::make_unique<model::GeneralPatient>(pid(), "Amara Okafor", 34, "0803 555 0101", "amara.okafor@example.com"));
    auto* tunde = add(std::make_unique<model::EmergencyPatient>(pid(), "Tunde Bakare", 8, "0803 555 0102", "", 1));
    auto* ngozi = add(std::make_unique<model::FollowUpPatient>(pid(), "Ngozi Eze", 52, "0803 555 0103", "", v::addDays(today, -20), "Blood pressure review"));
    auto* ibrahim = add(std::make_unique<model::GeneralPatient>(pid(), "Ibrahim Musa", 67, "0803 555 0104", ""));
    auto* folake = add(std::make_unique<model::GeneralPatient>(pid(), "Folake Adeyemi", 29, "", "folake.adeyemi@example.com"));
    auto* chinedu = add(std::make_unique<model::EmergencyPatient>(pid(), "Chinedu Nwosu", 41, "0803 555 0106", "", 2));
    auto* zainab = add(std::make_unique<model::GeneralPatient>(pid(), "Zainab Lawal", 5, "0803 555 0107", ""));
    auto* emeka = add(std::make_unique<model::FollowUpPatient>(pid(), "Emeka Obi", 58, "0803 555 0108", "", v::addDays(today, -9), "Diabetes follow-up"));
    auto* halima = add(std::make_unique<model::GeneralPatient>(pid(), "Halima Sani", 23, "0803 555 0109", "halima.sani@example.com"));
    auto* segun = add(std::make_unique<model::FollowUpPatient>(pid(), "Segun Ajayi", 36, "0803 555 0110", "", v::addDays(today, -30), "Post-treatment check"));

    // ---- appointments (any status; dates relative to today) ----
    auto book = [&](model::PatientRecord* p, int dayOffset, const char* time, int service, AppointmentStatus status,
                    const char* reason, const char* outcome = "") {
        const auto& svc = services[static_cast<std::size_t>(service)];
        Appointment a(ids.nextAppointmentId(), svc.id, svc.name, v::addDays(today, dayOffset) + " " + time, reason);
        a.setStatus(status);
        a.setOutcomeNote(outcome);
        p->addLoadedAppointment(a);
    };
    using S = AppointmentStatus;
    // past, completed (these feed "most frequently attended")
    book(amara, -30, "09:00", 0, S::Completed, "Check-up", "Healthy");
    book(amara, -14, "10:30", 1, S::Completed, "Blood work", "Results normal");
    book(amara, -7, "11:00", 0, S::Completed, "Review", "Advised rest");
    book(ibrahim, -21, "08:30", 0, S::Completed, "Chest pain", "Referred for tests");
    book(ibrahim, -10, "09:15", 2, S::Completed, "Chest X-ray", "Clear");
    book(ngozi, -20, "10:00", 0, S::Completed, "Blood pressure", "Medication adjusted");
    book(ngozi, -8, "10:00", 3, S::Completed, "Medication review");
    book(tunde, -3, "07:45", 0, S::Completed, "High fever", "Treated and discharged");
    book(segun, -30, "14:00", 0, S::Completed, "Treatment", "Completed");
    book(emeka, -9, "13:00", 1, S::Completed, "Sugar level test");
    book(folake, -5, "15:30", 0, S::Cancelled, "Check-up");
    book(halima, -4, "12:00", 0, S::Cancelled, "Consultation");
    // today
    book(tunde, 0, "08:00", 2, S::Completed, "Follow-up scan", "Healing well");
    book(zainab, 0, "09:30", 0, S::Pending, "Fever and cough");
    book(chinedu, 0, "11:00", 0, S::Pending, "Injury");
    book(halima, 0, "15:00", 1, S::Pending, "Lab test");
    // upcoming
    book(folake, 2, "10:00", 0, S::Pending, "Check-up");
    book(ngozi, 3, "10:00", 0, S::Pending, "Blood pressure review");
    book(emeka, 5, "09:00", 1, S::Pending, "Diabetes follow-up");
    book(ibrahim, 7, "11:30", 3, S::Pending, "Medication refill");
    book(amara, 10, "14:30", 0, S::Pending, "Routine check");
    book(segun, 12, "16:00", 2, S::Pending, "Final scan");
}

} // namespace scapms::controller
