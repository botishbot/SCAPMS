#include "controller/AppointmentController.h"
#include "controller/ClinicManager.h"
#include "controller/QueueManager.h"
#include "controller/UndoManager.h"
#include "model/ClinicDatabase.h"
#include "test_support.h"
#include "utils/IDGenerator.h"
#include "utils/Validation.h"

using namespace scapms;
using controller::AppointmentController;
using controller::ClinicManager;
using controller::PatientInput;
using controller::QueueManager;
using controller::UndoManager;
using model::AppointmentStatus;
using model::PatientCategory;

static const std::string NOW = "2026-10-24 10:00";

namespace {
struct Fixture {
    model::ClinicDatabase db;
    utils::IDGenerator ids;
    UndoManager undo{db};
    ClinicManager patients{db, ids, undo};
    AppointmentController appts{db, ids, undo};
    QueueManager queue{db};
    std::string ada, bayo, chi;       // patient IDs: General, Emergency, Follow-Up
    std::string consult, xray;        // service IDs
    std::string adaAppt, bayoAppt, chiAppt;

    Fixture() {
        appts.setAppointmentClosedCallback([this](const std::string& id) { queue.onAppointmentClosed(id); });
        PatientInput g; g.name = "Ada Obi"; g.age = 30; g.phone = "08035550101";
        ada = patients.registerPatient(g);
        PatientInput e; e.category = PatientCategory::Emergency; e.name = "Bayo Ade"; e.age = 8;
        e.phone = "08031234567"; e.triageLevel = 1;
        bayo = patients.registerPatient(e);
        PatientInput f; f.category = PatientCategory::FollowUp; f.name = "Chi Eze"; f.age = 52;
        f.email = "chi@example.com"; f.previousVisitDate = "2026-10-01";
        chi = patients.registerPatient(f);
        consult = appts.addService("Consultation", "");
        xray = appts.addService("X-Ray", "");
        adaAppt = appts.schedule(ada, consult, "2026-10-25 09:00", "", NOW);
        bayoAppt = appts.schedule(bayo, consult, "2026-10-24 10:30", "", NOW);
        chiAppt = appts.schedule(chi, xray, "2026-10-26 09:00", "", NOW);
    }
};
} // namespace

// ---------- time helper ----------
static void testMinutesBetween() {
    namespace v = utils::validation;
    CHECK_EQ(v::minutesBetween("2026-10-24 10:00", "2026-10-24 10:45"), 45L);
    CHECK_EQ(v::minutesBetween("2026-10-24 23:50", "2026-10-25 00:10"), 20L);       // across midnight
    CHECK_EQ(v::minutesBetween("2028-02-28 12:00", "2028-03-01 12:00"), 2880L);     // leap year: 2 days
    CHECK_EQ(v::minutesBetween("2026-12-31 23:59", "2027-01-01 00:00"), 1L);        // across a year
    CHECK_EQ(v::minutesBetween("2026-10-24 10:00", "2026-10-24 09:00"), -60L);
    CHECK_THROWS(v::minutesBetween("garbage", NOW), InvalidAppointment);
}

// ---------- queue ----------
static void testQueueFifo() {
    Fixture f;
    f.queue.join(f.adaAppt, "2026-10-24 10:00");
    f.queue.join(f.bayoAppt, "2026-10-24 10:05");     // emergency joins SECOND -> stays second (FIFO)
    f.queue.join(f.chiAppt, "2026-10-24 10:07");
    CHECK_EQ(f.queue.size(), std::size_t(3));
    auto w = f.queue.waiting();
    CHECK_EQ(w[0].patientId, f.ada);
    CHECK_EQ(w[1].patientId, f.bayo);
    CHECK_EQ(w[2].patientId, f.chi);
    CHECK_EQ(f.queue.position(f.bayo), 2);
    CHECK_EQ(f.queue.position("P-9999"), 0);
    CHECK_EQ(f.queue.waiting().size(), std::size_t(3));   // reading the queue does not consume it

    CHECK_EQ(f.queue.callNext().patientId, f.ada);        // first in, first out
    CHECK(f.queue.nowServing().has_value());
    CHECK(f.queue.isBeingServed(f.ada));
    CHECK(!f.queue.isWaiting(f.ada));
    CHECK_EQ(f.queue.size(), std::size_t(2));
    CHECK_THROWS(f.queue.callNext(), InvalidQueue);       // must finish or release first

    f.queue.release();                                    // appointment stays Pending, patient not queued
    CHECK(!f.queue.nowServing().has_value());
    CHECK(f.patients.getPatient(f.ada).hasPendingAppointments());
    CHECK_THROWS(f.queue.release(), InvalidQueue);

    CHECK_EQ(f.queue.callNext().patientId, f.bayo);
    f.queue.release();
    CHECK_EQ(f.queue.callNext().patientId, f.chi);
    f.queue.release();
    CHECK_THROWS(f.queue.callNext(), InvalidQueue);       // empty
}

static void testQueueRules() {
    Fixture f;
    CHECK_THROWS(f.queue.join("A-9999", NOW), AppointmentNotFound);
    CHECK_THROWS(f.queue.join(f.adaAppt, "bad time"), InvalidQueue);
    f.queue.join(f.adaAppt, NOW);
    CHECK_THROWS(f.queue.join(f.adaAppt, NOW), InvalidQueue);              // already waiting
    f.queue.callNext();
    const auto second = f.appts.schedule(f.ada, f.xray, "2026-10-27 09:00", "", NOW);
    CHECK_THROWS(f.queue.join(second, NOW), InvalidQueue);                 // already being served

    f.appts.complete(f.chiAppt, "");
    CHECK_THROWS(f.queue.join(f.chiAppt, NOW), InvalidQueue);              // completed: not allowed
    f.appts.cancel(f.bayoAppt);
    CHECK_THROWS(f.queue.join(f.bayoAppt, NOW), InvalidQueue);             // cancelled: not allowed
}

static void testQueueRemoveKeepsOrder() {
    Fixture f;
    f.queue.join(f.adaAppt, "2026-10-24 10:00");
    f.queue.join(f.bayoAppt, "2026-10-24 10:05");
    f.queue.join(f.chiAppt, "2026-10-24 10:07");
    f.queue.remove(f.bayo);                                // leaves from the middle
    auto w = f.queue.waiting();
    CHECK_EQ(w.size(), std::size_t(2));
    CHECK_EQ(w[0].patientId, f.ada);
    CHECK_EQ(w[1].patientId, f.chi);
    CHECK_EQ(w[1].joinedAt, std::string("2026-10-24 10:07"));
    CHECK_THROWS(f.queue.remove(f.bayo), InvalidQueue);    // not in the queue any more
    f.queue.clear();
    CHECK_EQ(f.queue.size(), std::size_t(0));
}

static void testQueueFollowsAppointmentLifecycle() {
    Fixture f;
    f.queue.join(f.adaAppt, "2026-10-24 10:00");
    f.queue.join(f.bayoAppt, "2026-10-24 10:05");
    f.queue.join(f.chiAppt, "2026-10-24 10:07");

    f.appts.cancel(f.bayoAppt);                            // cancelled while waiting -> leaves the queue
    CHECK_EQ(f.queue.size(), std::size_t(2));
    CHECK(!f.queue.isWaiting(f.bayo));

    f.queue.callNext();                                    // Ada now being served
    f.appts.complete(f.adaAppt, "Seen");                   // completing clears "now serving"
    CHECK(!f.queue.nowServing().has_value());
    CHECK(!f.patients.getPatient(f.ada).hasPendingAppointments());

    f.appts.remove(f.chiAppt);                             // removed while waiting -> leaves the queue
    CHECK_EQ(f.queue.size(), std::size_t(0));
}

// ---------- undo ----------
static void testUndoBasics() {
    Fixture f;
    CHECK(!f.undo.canUndo());
    CHECK_EQ(f.undo.nextDescription(), std::string(""));
    CHECK_THROWS(f.undo.undoLast(), UndoUnavailable);

    int changes = 0;
    f.undo.setChangedCallback([&] { ++changes; });
    f.appts.cancel(f.adaAppt);
    CHECK_EQ(f.undo.size(), std::size_t(1));
    CHECK_EQ(changes, 1);
    CHECK_EQ(f.undo.nextDescription(), std::string("Cancel appointment A-0001 (Ada Obi)"));
    CHECK_EQ(f.undo.undoLast(), std::string("Cancel appointment A-0001 (Ada Obi)"));
    CHECK_EQ(changes, 2);
    CHECK(!f.undo.canUndo());

    f.appts.complete(f.chiAppt, "");                       // completing is not an undoable operation
    CHECK(!f.undo.canUndo());
    f.appts.cancel(f.adaAppt);
    f.undo.clear();                                        // New / Load clear the stack
    CHECK(!f.undo.canUndo());
}

static void testUndoCancelAndRemove() {
    Fixture f;
    f.appts.cancel(f.adaAppt);
    f.undo.undoLast();
    CHECK(f.patients.getPatient(f.ada).findAppointment(f.adaAppt)->status() == AppointmentStatus::Pending);

    f.appts.remove(f.adaAppt);
    CHECK(f.patients.getPatient(f.ada).findAppointment(f.adaAppt) == nullptr);
    f.undo.undoLast();
    const auto* back = f.patients.getPatient(f.ada).findAppointment(f.adaAppt);
    CHECK(back != nullptr);
    CHECK(back->status() == AppointmentStatus::Pending);
    CHECK_EQ(back->dateTime(), std::string("2026-10-25 09:00"));
    CHECK_EQ(back->patientNameSnapshot(), std::string("Ada Obi"));
}

static void testUndoLifoOrder() {
    Fixture f;
    f.appts.cancel(f.adaAppt);                             // 1st
    f.appts.remove(f.chiAppt);                             // 2nd
    PatientInput edit; edit.name = "Bayo Adeyemi"; edit.age = 9; edit.phone = "08031234567";
    edit.category = PatientCategory::Emergency; edit.triageLevel = 1;
    f.patients.editPatient(f.bayo, edit);                  // 3rd
    CHECK_EQ(f.undo.size(), std::size_t(3));

    CHECK_EQ(f.undo.undoLast(), std::string("Edit patient P-0002 (Bayo Ade)"));       // newest first
    CHECK_EQ(f.patients.getPatient(f.bayo).name(), std::string("Bayo Ade"));
    CHECK_EQ(f.patients.getPatient(f.bayo).age(), 8);
    CHECK_EQ(f.undo.undoLast(), std::string("Remove appointment A-0003 (Chi Eze)"));
    CHECK_EQ(f.undo.undoLast(), std::string("Cancel appointment A-0001 (Ada Obi)"));
    CHECK(!f.undo.canUndo());
}

static void testUndoEditKeepsNewAppointments() {
    Fixture f;
    PatientInput edit; edit.name = "Ada Obi-Nwosu"; edit.age = 31; edit.phone = "08035550101";
    f.patients.editPatient(f.ada, edit);
    // booked after the edit (scheduling itself is not undoable)
    const auto later = f.appts.schedule(f.ada, f.xray, "2026-10-28 09:00", "", NOW);
    CHECK_EQ(f.patients.getPatient(f.ada).appointments().size(), std::size_t(2));

    f.undo.undoLast();                                     // restores name and age...
    CHECK_EQ(f.patients.getPatient(f.ada).name(), std::string("Ada Obi"));
    CHECK_EQ(f.patients.getPatient(f.ada).age(), 30);
    CHECK_EQ(f.patients.getPatient(f.ada).appointments().size(), std::size_t(2));   // ...but keeps both bookings
    CHECK(f.patients.getPatient(f.ada).findAppointment(later) != nullptr);
    CHECK_EQ(f.patients.getPatient(f.ada).appointments()[0].patientNameSnapshot(), std::string("Ada Obi"));
}

static void testUndoDeletePatient() {
    Fixture f;
    f.appts.cancel(f.adaAppt);                             // so Ada can be deleted
    f.appts.schedule(f.ada, f.xray, "2026-10-29 09:00", "", NOW);
    CHECK_THROWS(f.patients.deletePatient(f.ada), PendingAppointments);
    f.appts.cancel("A-0004");
    const std::size_t undoBefore = f.undo.size();
    f.patients.deletePatient(f.ada);
    CHECK_EQ(f.undo.size(), undoBefore + 1);
    CHECK_EQ(f.db.history().size(), std::size_t(2));
    CHECK_THROWS(f.patients.getPatient(f.ada), PatientNotFound);

    CHECK_EQ(f.undo.nextDescription(), std::string("Delete patient P-0001 (Ada Obi)"));
    f.undo.undoLast();
    CHECK_EQ(f.patients.getPatient(f.ada).name(), std::string("Ada Obi"));
    CHECK_EQ(f.patients.getPatient(f.ada).appointments().size(), std::size_t(2));   // appointments came back
    CHECK_EQ(f.db.history().size(), std::size_t(0));                                // no duplicates left in history
    CHECK_EQ(f.patients.patientCount(), 3);
}

static void testUndoCancelBlockedWhenSlotRebooked() {
    Fixture f;
    f.appts.cancel(f.adaAppt);
    f.appts.schedule(f.ada, f.consult, "2026-10-25 09:00", "", NOW);          // same service + time booked again
    const std::size_t before = f.undo.size();
    CHECK_THROWS(f.undo.undoLast(), UndoUnavailable);
    CHECK_EQ(f.undo.size(), before - 1);                                       // failed entry discarded
    CHECK(f.patients.getPatient(f.ada).findAppointment(f.adaAppt)->status() == AppointmentStatus::Cancelled);
}

static void testUndoRemoveBlockedAndDoesNotBlockOlderEntries() {
    Fixture f;
    f.appts.cancel(f.chiAppt);                             // older entry (can be undone)
    f.appts.remove(f.adaAppt);                             // newest entry
    f.appts.schedule(f.ada, f.consult, "2026-10-25 09:00", "", NOW);          // rebooked the removed slot
    CHECK_THROWS(f.undo.undoLast(), UndoUnavailable);       // cannot restore the removed one
    CHECK(f.undo.canUndo());                               // older entry still there
    CHECK_EQ(f.undo.undoLast(), std::string("Cancel appointment A-0003 (Chi Eze)"));
    CHECK(f.patients.getPatient(f.chi).findAppointment(f.chiAppt)->status() == AppointmentStatus::Pending);
}

static void testUndoDoesNotRequeue() {
    Fixture f;
    f.queue.join(f.adaAppt, NOW);
    f.appts.cancel(f.adaAppt);
    CHECK_EQ(f.queue.size(), std::size_t(0));
    f.undo.undoLast();                                     // appointment is Pending again...
    CHECK(f.patients.getPatient(f.ada).hasPendingAppointments());
    CHECK_EQ(f.queue.size(), std::size_t(0));              // ...but the patient is not back in the queue
}

void runQueueUndoTests() {
    testMinutesBetween();
    testQueueFifo();
    testQueueRules();
    testQueueRemoveKeepsOrder();
    testQueueFollowsAppointmentLifecycle();
    testUndoBasics();
    testUndoCancelAndRemove();
    testUndoLifoOrder();
    testUndoEditKeepsNewAppointments();
    testUndoDeletePatient();
    testUndoCancelBlockedWhenSlotRebooked();
    testUndoRemoveBlockedAndDoesNotBlockOlderEntries();
    testUndoDoesNotRequeue();
}
