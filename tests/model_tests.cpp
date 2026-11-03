#include "controller/ClinicManager.h"
#include "controller/UndoManager.h"
#include "model/ClinicDatabase.h"
#include "model/EmergencyPatient.h"
#include "model/FollowUpPatient.h"
#include "model/GeneralPatient.h"
#include "test_support.h"
#include "utils/IDGenerator.h"
#include "utils/Validation.h"

using namespace scapms;
using model::Appointment;
using model::AppointmentStatus;
using model::PatientCategory;
using controller::PatientInput;
namespace v = utils::validation;

static const std::string NOW = "2026-10-24 10:00";

static PatientInput general(const std::string& name = "Ada Obi") {
    PatientInput in;
    in.category = PatientCategory::General;
    in.name = name; in.age = 30; in.phone = "0803 555 0101"; in.email = "";
    return in;
}

static Appointment appt(const std::string& id, const std::string& when, const std::string& service = "S-001") {
    return Appointment(id, service, "Consultation", when, "check-up");
}

// ---------- validation ----------
static void testValidation() {
    CHECK_THROWS(v::requireName(""), InvalidPatient);
    CHECK_THROWS(v::requireName("   "), InvalidPatient);
    CHECK_EQ(v::collapseSpaces("  Ada   Obi "), std::string("Ada Obi"));

    v::requireAge(0);
    v::requireAge(120);
    CHECK_THROWS(v::requireAge(-1), InvalidAge);
    CHECK_THROWS(v::requireAge(121), InvalidAge);
    CHECK_EQ(v::parseAge(" 45 "), 45);
    CHECK_THROWS(v::parseAge("abc"), InvalidAge);
    CHECK_THROWS(v::parseAge("12.5"), InvalidAge);
    CHECK_THROWS(v::parseAge(""), InvalidAge);
    CHECK_THROWS(v::parseAge("-3"), InvalidAge);
    CHECK_THROWS(v::parseAge("999999999999"), InvalidAge);

    CHECK_THROWS(v::requireContact("", ""), InvalidContact);
    CHECK_THROWS(v::requireContact("12", ""), InvalidContact);
    CHECK_THROWS(v::requireContact("", "not-an-email"), InvalidContact);
    v::requireContact("+234 803 555 0101", "");
    v::requireContact("", "ada@example.com");
    v::requireContact("08035550101", "ada@example.com");

    CHECK(v::isValidDate("2026-02-28"));
    CHECK(!v::isValidDate("2026-02-30"));
    CHECK(v::isValidDate("2028-02-29"));   // leap year
    CHECK(!v::isValidDate("2026-13-01"));
    CHECK(!v::isValidDate("26-1-1"));
    CHECK(v::isValidDateTime("2026-10-24 23:59"));
    CHECK(!v::isValidDateTime("2026-10-24 24:00"));
    CHECK(!v::isValidDateTime("2026-10-24T10:00"));
}

// ---------- ID generator ----------
static void testIds() {
    utils::IDGenerator g;
    CHECK_EQ(g.nextPatientId(), std::string("P-0001"));
    CHECK_EQ(g.nextPatientId(), std::string("P-0002"));
    CHECK_EQ(g.nextAppointmentId(), std::string("A-0001"));
    CHECK_EQ(g.nextServiceId(), std::string("S-001"));
    g.observe("P-0040");
    CHECK_EQ(g.nextPatientId(), std::string("P-0041"));
    g.observe("garbage");
    g.observe("P-12x");
    CHECK_EQ(g.peekPatientId(), std::string("P-0042"));
}

// ---------- class hierarchy / polymorphism ----------
static void testRegisterAllCategories() {
    model::ClinicDatabase db;
    utils::IDGenerator ids;
    controller::UndoManager undo(db);
    controller::ClinicManager mgr(db, ids, undo);

    PatientInput g = general("Ada Obi");
    PatientInput e; e.category = PatientCategory::Emergency; e.name = "Bayo Ade"; e.age = 8;
    e.phone = "08031234567"; e.triageLevel = 1;
    PatientInput f; f.category = PatientCategory::FollowUp; f.name = "Chi Eze"; f.age = 52;
    f.email = "chi@example.com"; f.previousVisitDate = "2026-09-01"; f.followUpReason = "BP review";

    CHECK_EQ(mgr.registerPatient(g), std::string("P-0001"));
    CHECK_EQ(mgr.registerPatient(e), std::string("P-0002"));
    CHECK_EQ(mgr.registerPatient(f), std::string("P-0003"));
    CHECK_EQ(mgr.patientCount(), 3);

    CHECK(mgr.getPatient("P-0001").category() == PatientCategory::General);
    CHECK(mgr.getPatient("P-0002").category() == PatientCategory::Emergency);
    CHECK(mgr.getPatient("P-0003").category() == PatientCategory::FollowUp);
    CHECK(dynamic_cast<const model::EmergencyPatient*>(&mgr.getPatient("P-0002")) != nullptr);

    // Runtime polymorphism: one function, three different behaviours.
    auto lines = mgr.processAllPatients();
    CHECK_EQ(lines.size(), std::size_t(3));
    CHECK_EQ(lines[0], std::string("[P-0001] Ada Obi (General, Normal) - Routine patient"));
    CHECK_EQ(lines[1], std::string("[P-0002] Bayo Ade (Emergency, High Priority) - Triage level 1 (Critical)"));
    CHECK_EQ(lines[2], std::string("[P-0003] Chi Eze (Follow-Up, Scheduled) - Previous visit 2026-09-01 - BP review"));

    CHECK_EQ(mgr.countByCategory(PatientCategory::General), 1);
    CHECK_EQ(mgr.countByCategory(PatientCategory::Emergency), 1);
    CHECK_EQ(mgr.countByCategory(PatientCategory::FollowUp), 1);
}

static void testRegisterValidationDoesNotConsumeId() {
    model::ClinicDatabase db;
    utils::IDGenerator ids;
    controller::UndoManager undo(db);
    controller::ClinicManager mgr(db, ids, undo);

    PatientInput bad = general(""); 
    CHECK_THROWS(mgr.registerPatient(bad), InvalidPatient);
    PatientInput badAge = general(); badAge.age = 121;
    CHECK_THROWS(mgr.registerPatient(badAge), InvalidAge);
    PatientInput noContact = general(); noContact.phone = ""; noContact.email = "";
    CHECK_THROWS(mgr.registerPatient(noContact), InvalidContact);
    PatientInput badTriage; badTriage.category = PatientCategory::Emergency; badTriage.name = "X Y";
    badTriage.age = 5; badTriage.phone = "08031234567"; badTriage.triageLevel = 4;
    CHECK_THROWS(mgr.registerPatient(badTriage), InvalidPatient);
    PatientInput badDate; badDate.category = PatientCategory::FollowUp; badDate.name = "X Y";
    badDate.age = 5; badDate.phone = "08031234567"; badDate.previousVisitDate = "2026-02-31";
    CHECK_THROWS(mgr.registerPatient(badDate), InvalidPatient);

    CHECK_EQ(mgr.patientCount(), 0);
    CHECK_EQ(mgr.registerPatient(general()), std::string("P-0001"));  // first real ID not wasted
}

static void testDuplicateId() {
    model::ClinicDatabase db;
    db.addPatient(std::make_unique<model::GeneralPatient>("P-0001", "Ada Obi", 30, "08035550101", ""));
    CHECK_THROWS(db.addPatient(std::make_unique<model::GeneralPatient>("P-0001", "Other", 20, "08035550102", "")),
                 DuplicateId);
    CHECK_EQ(db.patientCount(), std::size_t(1));
}

// ---------- edit ----------
static void testEdit() {
    model::ClinicDatabase db;
    utils::IDGenerator ids;
    controller::UndoManager undo(db);
    controller::ClinicManager mgr(db, ids, undo);
    const std::string id = mgr.registerPatient(general());

    // give the patient an appointment so we can check it survives the edit
    db.findPatient(id)->scheduleAppointment(appt("A-0001", "2026-11-01 09:00"), NOW);

    PatientInput edited = general("Ada Obi-Nwosu");
    edited.age = 31;
    mgr.editPatient(id, edited);
    CHECK_EQ(mgr.getPatient(id).name(), std::string("Ada Obi-Nwosu"));
    CHECK_EQ(mgr.getPatient(id).age(), 31);
    CHECK_EQ(mgr.getPatient(id).id(), id);                // ID is stable
    CHECK_EQ(mgr.getPatient(id).appointments().size(), std::size_t(1));
    CHECK_EQ(undo.size(), std::size_t(1));                // the edit can be undone
    CHECK_EQ(undo.nextDescription(), std::string("Edit patient P-0001 (Ada Obi)"));

    PatientInput changeCategory = general(); changeCategory.category = PatientCategory::Emergency;
    CHECK_THROWS(mgr.editPatient(id, changeCategory), InvalidPatient);
    CHECK_THROWS(mgr.editPatient("P-9999", edited), PatientNotFound);
    PatientInput invalid = general(); invalid.age = -5;
    CHECK_THROWS(mgr.editPatient(id, invalid), InvalidAge);
    CHECK_EQ(mgr.getPatient(id).age(), 31);               // failed edit changed nothing
}

// ---------- delete + history ----------
static void testDelete() {
    model::ClinicDatabase db;
    utils::IDGenerator ids;
    controller::UndoManager undo(db);
    controller::ClinicManager mgr(db, ids, undo);
    const std::string id = mgr.registerPatient(general());
    auto* p = db.findPatient(id);

    p->scheduleAppointment(appt("A-0001", "2026-11-01 09:00"), NOW);
    CHECK_THROWS(mgr.deletePatient(id), PendingAppointments);   // pending blocks deletion
    CHECK_EQ(mgr.patientCount(), 1);

    CHECK(p->cancelAppointment("A-0001"));
    p->scheduleAppointment(appt("A-0002", "2026-11-02 09:00"), NOW);
    CHECK(p->completeAppointment("A-0002", "Seen and discharged"));

    mgr.deletePatient(id);
    CHECK_EQ(mgr.patientCount(), 0);
    CHECK_EQ(db.history().size(), std::size_t(2));               // terminal history preserved
    CHECK_EQ(db.history()[0].patientNameSnapshot(), std::string("Ada Obi"));
    CHECK_THROWS(mgr.deletePatient(id), PatientNotFound);
    CHECK_EQ(undo.nextDescription(), std::string("Delete patient P-0001 (Ada Obi)"));
}

// ---------- Appointable rules per type ----------
static void testGeneralScheduling() {
    model::GeneralPatient p("P-0001", "Ada Obi", 30, "08035550101", "");
    p.scheduleAppointment(appt("A-0001", "2026-10-24 10:30"), NOW);
    CHECK_THROWS(p.scheduleAppointment(appt("A-0002", "2026-10-24 09:00"), NOW), InvalidAppointment);  // past
    CHECK_THROWS(p.scheduleAppointment(appt("A-0003", "2026-10-24 10:00"), NOW), InvalidAppointment);  // exactly now
    CHECK_THROWS(p.scheduleAppointment(appt("A-0004", "2026-10-24 10:30"), NOW), DuplicateAppointment);  // same service+time
    p.scheduleAppointment(appt("A-0005", "2026-10-24 10:30", "S-002"), NOW);  // different service: allowed here
    CHECK(p.hasAppointmentAt("2026-10-24 10:30"));                            // controller can warn on this
    CHECK_EQ(p.pendingCount(), 2);
    CHECK_EQ(p.appointments()[0].patientId(), std::string("P-0001"));
    CHECK_EQ(p.appointments()[0].patientNameSnapshot(), std::string("Ada Obi"));
}

static void testEmergencyScheduling() {
    model::EmergencyPatient p("P-0002", "Bayo Ade", 8, "08031234567", "", 1);
    p.scheduleAppointment(appt("A-0001", "2026-10-24 08:00"), NOW);   // earlier today: allowed (walk-in)
    CHECK_THROWS(p.scheduleAppointment(appt("A-0002", "2026-10-23 23:00"), NOW), InvalidAppointment);
    p.scheduleAppointment(appt("A-0003", "2026-10-25 09:00"), NOW);
    CHECK_EQ(p.pendingCount(), 2);
    CHECK_EQ(std::string(model::EmergencyPatient::triageName(3)), std::string("Less urgent"));
}

static void testFollowUpScheduling() {
    model::FollowUpPatient p("P-0003", "Chi Eze", 52, "", "chi@example.com", "2026-10-30", "BP review");
    CHECK_THROWS(p.scheduleAppointment(appt("A-0001", "2026-10-29 09:00"), NOW), InvalidAppointment);  // before previous visit
    CHECK_THROWS(p.scheduleAppointment(appt("A-0002", "2026-10-30 09:00"), NOW), InvalidAppointment);  // same day as previous visit
    p.scheduleAppointment(appt("A-0003", "2026-10-31 09:00"), NOW);
    CHECK_EQ(p.pendingCount(), 1);
}

static void testCancelAndStatusRules() {
    model::GeneralPatient p("P-0001", "Ada Obi", 30, "08035550101", "");
    p.scheduleAppointment(appt("A-0001", "2026-11-01 09:00"), NOW);
    CHECK(p.cancelAppointment("A-0001"));
    CHECK(!p.cancelAppointment("A-0001"));        // already cancelled
    CHECK(!p.cancelAppointment("A-9999"));        // unknown ID
    CHECK(p.appointments()[0].status() == AppointmentStatus::Cancelled);
    CHECK_EQ(p.appointments().size(), std::size_t(1));   // cancel keeps the record
    // a cancelled slot can be booked again
    p.scheduleAppointment(appt("A-0002", "2026-11-01 09:00"), NOW);
    CHECK_EQ(p.pendingCount(), 1);

    CHECK_THROWS(Appointment("", "S-001", "x", "2026-11-01 09:00", ""), InvalidAppointment);
    CHECK_THROWS(Appointment("A-1", "", "x", "2026-11-01 09:00", ""), InvalidAppointment);
    CHECK_THROWS(Appointment("A-1", "S-001", "x", "tomorrow", ""), InvalidAppointment);
}

// ---------- clone (undo snapshots) ----------
static void testClone() {
    model::EmergencyPatient p("P-0002", "Bayo Ade", 8, "08031234567", "", 2);
    p.scheduleAppointment(appt("A-0001", "2026-10-25 09:00"), NOW);
    auto copy = p.clone();
    CHECK(copy->category() == PatientCategory::Emergency);
    CHECK_EQ(copy->summary(), p.summary());
    CHECK_EQ(copy->appointments().size(), std::size_t(1));
    CHECK(copy.get() != &p);
}

// ---------- invalidation hook for the cache ----------
static void testInvalidationCallback() {
    model::ClinicDatabase db;
    std::vector<std::string> notified;
    db.setInvalidationCallback([&](const std::string& id) { notified.push_back(id); });
    db.addPatient(std::make_unique<model::GeneralPatient>("P-0001", "Ada Obi", 30, "08035550101", ""));
    db.addPatient(std::make_unique<model::GeneralPatient>("P-0002", "Bayo Ade", 30, "08035550102", ""));
    db.replacePatient("P-0001", std::make_unique<model::GeneralPatient>("P-0001", "Ada B", 30, "08035550101", ""));
    db.removePatient("P-0002");
    CHECK_EQ(notified.size(), std::size_t(2));
    CHECK_EQ(notified[0], std::string("P-0001"));
    CHECK_EQ(notified[1], std::string("P-0002"));
    CHECK_THROWS(db.replacePatient("P-0001", std::make_unique<model::GeneralPatient>("P-0009", "Z Z", 30, "08035550101", "")),
                 InvalidPatient);
}

void runModelTests() {
    testValidation();
    testIds();
    testRegisterAllCategories();
    testRegisterValidationDoesNotConsumeId();
    testDuplicateId();
    testEdit();
    testDelete();
    testGeneralScheduling();
    testEmergencyScheduling();
    testFollowUpScheduling();
    testCancelAndStatusRules();
    testClone();
    testInvalidationCallback();
}
