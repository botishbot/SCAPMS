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

void DemoData::populate(model::ClinicDatabase& db, utils::IDGenerator& ids, const std::string& today, bool large) {
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

    if (!large) return;

    // ---- 20 more patients (P-0011 to P-0030) ----
    auto general = [&](const char* name, int age, const char* phone, const char* email) {
        return add(std::make_unique<model::GeneralPatient>(pid(), name, age, phone, email));
    };
    auto emergency = [&](const char* name, int age, const char* phone, int triage) {
        return add(std::make_unique<model::EmergencyPatient>(pid(), name, age, phone, "", triage));
    };
    auto followUp = [&](const char* name, int age, const char* phone, int daysSinceVisit, const char* reason) {
        return add(std::make_unique<model::FollowUpPatient>(pid(), name, age, phone, "", v::addDays(today, -daysSinceVisit), reason));
    };
    auto* adaeze = general("Adaeze Okonkwo", 27, "0803 555 0111", "adaeze.okonkwo@example.com");
    auto* obinna = general("Obinna Eze", 45, "0803 555 0112", "");
    auto* fatima = general("Fatima Abdullahi", 31, "0803 555 0113", "fatima.abdullahi@example.com");
    auto* seun = general("Oluwaseun Adebayo", 19, "0803 555 0114", "");
    auto* blessing = general("Blessing Udoh", 38, "", "blessing.udoh@example.com");
    auto* kelechi = followUp("Kelechi Obi", 62, "0803 555 0116", 14, "Hypertension follow-up");
    auto* rashida = followUp("Rashida Garba", 55, "0803 555 0117", 6, "Wound dressing");
    auto* tobi = emergency("Tobi Ogunleye", 12, "0803 555 0118", 2);
    auto* yetunde = general("Yetunde Ibrahim", 44, "0803 555 0119", "yetunde.ibrahim@example.com");
    auto* chukwuemeka = followUp("Chukwuemeka Obiora", 71, "0803 555 0120", 25, "Cardiology review");
    auto* maryam = emergency("Maryam Bello", 3, "0803 555 0121", 1);
    auto* daniel = general("Daniel Okeke", 33, "0803 555 0122", "");
    auto* hadiza = general("Hadiza Mohammed", 26, "0803 555 0123", "hadiza.mohammed@example.com");
    auto* samuel = followUp("Samuel Ojo", 49, "0803 555 0124", 11, "Physiotherapy");
    auto* ifeoma = general("Ifeoma Nnamdi", 36, "0803 555 0125", "");
    auto* abdul = emergency("Abdul Yusuf", 58, "0803 555 0126", 3);
    auto* grace = general("Grace Effiong", 22, "0803 555 0127", "grace.effiong@example.com");
    auto* peter = followUp("Peter Danjuma", 64, "0803 555 0128", 40, "Diabetes review");
    general("Nkechi Onuoha", 30, "0803 555 0129", "");   // new patient: no appointments yet
    auto* emmanuel = emergency("Emmanuel Bassey", 9, "0803 555 0130", 2);

    book(adaeze, -12, "09:00", 0, S::Completed, "Check-up", "Healthy");
    book(adaeze, 3, "10:30", 1, S::Pending, "Blood test");
    book(obinna, -20, "11:00", 0, S::Completed, "Back pain", "Physiotherapy advised");
    book(obinna, -6, "14:00", 2, S::Completed, "Spine X-ray", "No fracture");
    book(obinna, 0, "10:00", 0, S::Pending, "Review");
    book(fatima, -15, "08:30", 0, S::Completed, "Antenatal check", "Normal");
    book(fatima, -1, "09:30", 1, S::Completed, "Lab test");
    book(fatima, 4, "09:00", 0, S::Pending, "Antenatal check");
    book(seun, -3, "16:00", 0, S::Cancelled, "Sports injury");
    book(seun, 2, "15:00", 2, S::Pending, "Knee X-ray");
    book(blessing, 0, "11:30", 0, S::Pending, "Headache");
    book(kelechi, -14, "10:00", 0, S::Completed, "Blood pressure", "Medication adjusted");
    book(kelechi, -7, "10:00", 3, S::Completed, "Medication review");
    book(kelechi, 7, "10:00", 0, S::Pending, "Blood pressure follow-up");
    book(rashida, -6, "13:00", 0, S::Completed, "Dressing change");
    book(rashida, 0, "13:30", 0, S::Pending, "Dressing change");
    book(rashida, 6, "13:30", 0, S::Pending, "Dressing change");
    book(tobi, -2, "18:30", 0, S::Completed, "Asthma attack", "Stabilised");
    book(tobi, 0, "09:00", 0, S::Pending, "Asthma review");
    book(yetunde, -30, "09:00", 1, S::Completed, "Lab test");
    book(yetunde, -9, "09:00", 0, S::Completed, "Consultation", "Advised rest");
    book(yetunde, -2, "15:30", 0, S::Cancelled, "Consultation");
    book(yetunde, 5, "12:00", 0, S::Pending, "Consultation");
    book(chukwuemeka, -25, "09:30", 0, S::Completed, "Cardiology", "ECG ordered");
    book(chukwuemeka, -18, "09:30", 1, S::Completed, "ECG");
    book(chukwuemeka, -11, "09:30", 0, S::Completed, "Review", "Stable");
    book(chukwuemeka, -4, "09:30", 3, S::Completed, "Medication refill");
    book(chukwuemeka, 8, "09:30", 0, S::Pending, "Cardiology review");
    book(maryam, -1, "22:10", 0, S::Completed, "High fever", "Treated and discharged");
    book(maryam, 1, "10:00", 0, S::Pending, "Fever review");
    book(daniel, -8, "10:15", 2, S::Completed, "Chest X-ray", "Clear");
    book(daniel, 1, "11:15", 0, S::Pending, "Results");
    book(hadiza, 0, "14:30", 1, S::Pending, "Lab test");
    book(hadiza, 9, "14:30", 1, S::Pending, "Lab test");
    book(samuel, -11, "15:00", 0, S::Completed, "Physiotherapy");
    book(samuel, -4, "15:00", 0, S::Completed, "Physiotherapy");
    book(samuel, 3, "15:00", 0, S::Pending, "Physiotherapy");
    book(ifeoma, -10, "12:00", 3, S::Completed, "Pharmacy");
    book(ifeoma, 0, "16:00", 0, S::Pending, "Check-up");
    book(abdul, -5, "20:00", 0, S::Completed, "Road accident", "Observation");
    book(abdul, -2, "09:00", 2, S::Completed, "Arm X-ray", "Hairline fracture");
    book(abdul, 2, "09:00", 0, S::Pending, "Cast check");
    book(grace, 6, "10:45", 0, S::Pending, "Check-up");
    book(peter, -40, "08:00", 0, S::Completed, "Diabetes", "Diet plan");
    book(peter, -20, "08:00", 1, S::Completed, "Sugar test");
    book(peter, -9, "08:00", 0, S::Completed, "Diabetes review", "Improving");
    book(peter, 0, "08:30", 1, S::Pending, "Sugar test");
    book(emmanuel, -7, "17:00", 0, S::Completed, "Fracture", "Plaster applied");
    book(emmanuel, -1, "10:00", 2, S::Completed, "X-ray follow-up");
    book(emmanuel, 5, "10:00", 0, S::Pending, "Cast removal");
}

} // namespace scapms::controller
