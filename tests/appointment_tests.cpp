#include "controller/AppointmentController.h"
#include "controller/ClinicManager.h"
#include "controller/UndoManager.h"
#include "model/ClinicDatabase.h"
#include "test_support.h"
#include "utils/IDGenerator.h"

using namespace scapms;
using controller::AppointmentController;
using controller::ClinicManager;
using controller::PatientInput;
using model::AppointmentStatus;
using model::PatientCategory;

static const std::string NOW = "2026-10-24 10:00";

namespace {
struct Fixture {
    model::ClinicDatabase db;
    utils::IDGenerator ids;
    controller::UndoManager undo{db};
    ClinicManager patients{db, ids, undo};
    AppointmentController appts{db, ids, undo};
    std::string general, emergency, followUp, consult, xray;

    Fixture() {
        PatientInput g; g.name = "Ada Obi"; g.age = 30; g.phone = "08035550101";
        general = patients.registerPatient(g);
        PatientInput e; e.category = PatientCategory::Emergency; e.name = "Bayo Ade"; e.age = 8;
        e.phone = "08031234567"; e.triageLevel = 1;
        emergency = patients.registerPatient(e);
        PatientInput f; f.category = PatientCategory::FollowUp; f.name = "Chi Eze"; f.age = 52;
        f.email = "chi@example.com"; f.previousVisitDate = "2026-10-20"; f.followUpReason = "BP review";
        followUp = patients.registerPatient(f);
        consult = appts.addService("General Consultation", "Routine visit");
        xray = appts.addService("X-Ray", "");
    }
};
} // namespace

static void testServices() {
    Fixture f;
    CHECK_EQ(f.consult, std::string("S-001"));
    CHECK_EQ(f.xray, std::string("S-002"));
    CHECK_THROWS(f.appts.addService("   ", ""), InvalidService);
    CHECK_THROWS(f.appts.addService("general   CONSULTATION", ""), InvalidService);   // duplicate, case-insensitive
    CHECK_EQ(f.appts.services().size(), std::size_t(2));
    CHECK_EQ(f.appts.addService("Lab Test", "Blood work"), std::string("S-003"));      // failed adds used no IDs

    f.appts.setServiceActive(f.xray, false);
    CHECK_EQ(f.appts.activeServices().size(), std::size_t(2));
    CHECK_THROWS(f.appts.setServiceActive("S-999", true), InvalidService);
}

static void testScheduleAndPolymorphism() {
    Fixture f;
    // General: must be in the future
    CHECK_THROWS(f.appts.schedule(f.general, f.consult, "2026-10-24 09:00", "", NOW), InvalidAppointment);
    CHECK_EQ(f.appts.schedule(f.general, f.consult, "2026-10-25 09:00", " check  up ", NOW), std::string("A-0001"));
    CHECK_EQ(f.patients.getPatient(f.general).appointments().size(), std::size_t(1));
    const auto& a = f.patients.getPatient(f.general).appointments()[0];
    CHECK_EQ(a.serviceNameSnapshot(), std::string("General Consultation"));
    CHECK_EQ(a.reason(), std::string("check up"));
    CHECK_EQ(a.patientNameSnapshot(), std::string("Ada Obi"));

    // Emergency: earlier today is fine. Follow-Up: not before the previous visit.
    CHECK_EQ(f.appts.schedule(f.emergency, f.consult, "2026-10-24 08:00", "", NOW), std::string("A-0002"));
    CHECK_THROWS(f.appts.schedule(f.followUp, f.consult, "2026-10-20 12:00", "", NOW), InvalidAppointment);
    CHECK_EQ(f.appts.schedule(f.followUp, f.consult, "2026-10-26 12:00", "", NOW), std::string("A-0003"));

    // failures: unknown patient/service, inactive service, bad date text. None used an ID.
    CHECK_THROWS(f.appts.schedule("P-9999", f.consult, "2026-10-25 09:00", "", NOW), PatientNotFound);
    CHECK_THROWS(f.appts.schedule(f.general, "S-999", "2026-10-25 09:00", "", NOW), InvalidService);
    CHECK_THROWS(f.appts.schedule(f.general, "", "2026-10-25 09:00", "", NOW), InvalidService);
    f.appts.setServiceActive(f.xray, false);
    CHECK_THROWS(f.appts.schedule(f.general, f.xray, "2026-10-25 09:00", "", NOW), InvalidService);
    CHECK_THROWS(f.appts.schedule(f.general, f.consult, "tomorrow", "", NOW), InvalidAppointment);
    CHECK_EQ(f.appts.schedule(f.general, f.consult, "2026-10-27 09:00", "", NOW), std::string("A-0004"));

    // existing appointments keep working after their service is deactivated
    CHECK_EQ(f.patients.getPatient(f.general).appointments()[0].serviceNameSnapshot(), std::string("General Consultation"));
}

static void testDuplicatesAndSameTimeWarning() {
    Fixture f;
    f.appts.schedule(f.general, f.consult, "2026-10-25 09:00", "", NOW);
    CHECK_THROWS(f.appts.schedule(f.general, f.consult, "2026-10-25 09:00", "", NOW), DuplicateAppointment);
    CHECK(!f.appts.hasSameTimeConflict(f.general, f.consult, "2026-10-25 09:00"));  // same service: blocked, not a warning
    CHECK(f.appts.hasSameTimeConflict(f.general, f.xray, "2026-10-25 09:00"));      // different service: warn
    CHECK(!f.appts.hasSameTimeConflict(f.general, f.xray, "2026-10-25 10:00"));
    CHECK(!f.appts.hasSameTimeConflict("P-9999", f.xray, "2026-10-25 09:00"));
    f.appts.schedule(f.general, f.xray, "2026-10-25 09:00", "", NOW);               // allowed after user confirms
    CHECK_EQ(f.patients.getPatient(f.general).pendingCount(), 2);
}

static void testCancel() {
    Fixture f;
    const auto id = f.appts.schedule(f.general, f.consult, "2026-10-25 09:00", "", NOW);
    f.appts.cancel(id);
    CHECK(f.patients.getPatient(f.general).appointments()[0].status() == AppointmentStatus::Cancelled);
    CHECK_EQ(f.patients.getPatient(f.general).appointments().size(), std::size_t(1));   // kept as history
    CHECK_THROWS(f.appts.cancel(id), InvalidAppointment);          // already cancelled
    CHECK_THROWS(f.appts.cancel("A-9999"), AppointmentNotFound);
    // the freed slot can be booked again
    f.appts.schedule(f.general, f.consult, "2026-10-25 09:00", "", NOW);
    CHECK_EQ(f.patients.getPatient(f.general).pendingCount(), 1);
}

static void testComplete() {
    Fixture f;
    const auto id = f.appts.schedule(f.general, f.consult, "2026-10-25 09:00", "", NOW);
    f.appts.complete(id, "  Seen,   advised rest ");
    const auto& a = f.patients.getPatient(f.general).appointments()[0];
    CHECK(a.status() == AppointmentStatus::Completed);
    CHECK_EQ(a.outcomeNote(), std::string("Seen, advised rest"));
    CHECK_EQ(f.patients.getPatient(f.general).completedCount(), 1);
    CHECK_THROWS(f.appts.complete(id, ""), InvalidAppointment);        // already completed
    CHECK_THROWS(f.appts.cancel(id), InvalidAppointment);              // completed cannot be cancelled
    CHECK_THROWS(f.appts.complete("A-9999", ""), AppointmentNotFound);

    const auto cancelled = f.appts.schedule(f.general, f.xray, "2026-10-26 09:00", "", NOW);
    f.appts.cancel(cancelled);
    CHECK_THROWS(f.appts.complete(cancelled, ""), InvalidAppointment); // cancelled cannot be completed
}

static void testRemove() {
    Fixture f;
    const auto pending = f.appts.schedule(f.general, f.consult, "2026-10-25 09:00", "", NOW);
    const auto done = f.appts.schedule(f.general, f.consult, "2026-10-26 09:00", "", NOW);
    f.appts.complete(done, "");
    const auto cancelled = f.appts.schedule(f.general, f.xray, "2026-10-27 09:00", "", NOW);
    f.appts.cancel(cancelled);

    CHECK_THROWS(f.appts.remove(done), InvalidAppointment);
    CHECK_THROWS(f.appts.remove(cancelled), InvalidAppointment);
    CHECK_THROWS(f.appts.remove("A-9999"), AppointmentNotFound);
    CHECK_EQ(f.patients.getPatient(f.general).appointments().size(), std::size_t(3));

    f.appts.remove(pending);
    CHECK_EQ(f.patients.getPatient(f.general).appointments().size(), std::size_t(2));
    CHECK(f.patients.getPatient(f.general).findAppointment(pending) == nullptr);
}

static void testOverdueAndQueries() {
    Fixture f;
    const auto early = f.appts.schedule(f.emergency, f.consult, "2026-10-24 08:00", "", NOW);   // earlier today
    const auto later = f.appts.schedule(f.general, f.consult, "2026-10-25 09:00", "", NOW);
    const auto mid = f.appts.schedule(f.followUp, f.xray, "2026-10-24 15:00", "", NOW);

    auto rows = f.appts.allAppointments(NOW);
    CHECK_EQ(rows.size(), std::size_t(3));
    CHECK_EQ(rows[0].appointment.id(), early);                         // sorted earliest first
    CHECK_EQ(rows[1].appointment.id(), mid);
    CHECK_EQ(rows[2].appointment.id(), later);
    CHECK(rows[0].overdue);                                            // pending and already past
    CHECK_EQ(rows[0].displayStatus, std::string("Overdue"));
    CHECK(!rows[1].overdue);
    CHECK_EQ(rows[1].displayStatus, std::string("Pending"));
    CHECK(rows[0].category == PatientCategory::Emergency);

    f.appts.complete(early, "");
    rows = f.appts.allAppointments(NOW);
    CHECK(!rows[0].overdue);                                           // a completed appointment is never overdue
    CHECK_EQ(rows[0].displayStatus, std::string("Completed"));

    CHECK_EQ(f.appts.countOnDate("2026-10-24"), 2);
    f.appts.cancel(mid);
    CHECK_EQ(f.appts.countOnDate("2026-10-24"), 1);                    // cancelled not counted
    CHECK_EQ(f.appts.countOnDate("2026-10-25"), 1);
    CHECK_EQ(f.appts.countOnDate("2026-12-01"), 0);
}

static void testPatientInteractions() {
    Fixture f;
    const auto id = f.appts.schedule(f.general, f.consult, "2026-10-25 09:00", "", NOW);

    // renaming a patient updates the name shown on their appointments
    PatientInput edit; edit.name = "Ada Obi-Nwosu"; edit.age = 31; edit.phone = "08035550101";
    f.patients.editPatient(f.general, edit);
    CHECK_EQ(f.patients.getPatient(f.general).appointments()[0].patientNameSnapshot(), std::string("Ada Obi-Nwosu"));

    // pending appointment blocks deletion; once cancelled it moves to history
    CHECK_THROWS(f.patients.deletePatient(f.general), PendingAppointments);
    f.appts.cancel(id);
    f.patients.deletePatient(f.general);
    CHECK_EQ(f.db.history().size(), std::size_t(1));
    CHECK_EQ(f.db.history()[0].patientNameSnapshot(), std::string("Ada Obi-Nwosu"));
    CHECK_THROWS(f.appts.cancel(id), AppointmentNotFound);             // no longer on a live patient
    CHECK_EQ(f.patients.pendingAppointmentCount(), 0);
}

void runAppointmentTests() {
    testServices();
    testScheduleAndPolymorphism();
    testDuplicatesAndSameTimeWarning();
    testCancel();
    testComplete();
    testRemove();
    testOverdueAndQueries();
    testPatientInteractions();
}
