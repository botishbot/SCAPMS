#include "controller/ClinicManager.h"
#include "controller/DemoData.h"
#include "controller/ReportGenerator.h"
#include "controller/UndoManager.h"
#include "model/ClinicDatabase.h"
#include "test_support.h"
#include "utils/FileHandler.h"
#include "utils/IDGenerator.h"
#include "utils/Validation.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <functional>
#include <string>

using namespace scapms;
using controller::DemoData;
using controller::ReportGenerator;
using model::ClinicDatabase;
using utils::FileHandler;
using utils::IDGenerator;
namespace v = utils::validation;

static const std::string TODAY = "2026-10-28";

namespace {

// Everything in the database as text, so two databases can be compared exactly.
std::string describe(const ClinicDatabase& db) {
    std::string s;
    for (const auto& svc : db.services())
        s += "S|" + svc.id + "|" + svc.name + "|" + svc.description + "|" + (svc.active ? "1" : "0") + "\n";
    auto appt = [](const model::Appointment& a) {
        return a.id() + "|" + a.patientId() + "|" + a.patientNameSnapshot() + "|" + a.serviceId() + "|" +
               a.serviceNameSnapshot() + "|" + a.dateTime() + "|" + a.reason() + "|" + model::statusName(a.status()) +
               "|" + a.outcomeNote();
    };
    for (const auto& p : db.patients()) {
        s += "P|" + controller::ClinicManager::processPatient(*p) + "|" + std::to_string(p->age()) + "|" + p->phone() +
             "|" + p->email() + "\n";
        for (const auto& a : p->appointments()) s += "A|" + appt(a) + "\n";
    }
    for (const auto& a : db.history()) s += "H|" + appt(a) + "\n";
    return s;
}

struct Demo {
    ClinicDatabase db;
    IDGenerator ids;
    controller::UndoManager undo{db};
    controller::ClinicManager mgr{db, ids, undo};
    Demo() { DemoData::populate(db, ids, TODAY); }
};

// Valid JSON with services, patients, appointments and history (Tunde deleted -> history).
QByteArray validJson() {
    Demo d;
    d.mgr.deletePatient("P-0002");
    d.db.setInvalidationCallback(nullptr);
    return FileHandler::toJson(d.db);
}

QByteArray mutate(const QByteArray& json, const std::function<void(QJsonObject&)>& change) {
    QJsonObject root = QJsonDocument::fromJson(json).object();
    change(root);
    return QJsonDocument(root).toJson();
}

QJsonObject patientAt(QJsonObject& root, int i) { return root["patients"].toArray().at(i).toObject(); }
void setPatient(QJsonObject& root, int i, const QJsonObject& p) {
    QJsonArray a = root["patients"].toArray();
    a.replace(i, p);
    root["patients"] = a;
}

// load must fail with `code`, mention `text`, and leave the live data and ID counters untouched
#define CHECK_LOAD_FAILS(jsonBytes, expected, text)                                                  \
    do {                                                                                             \
        Demo live;                                                                                   \
        const std::string before = describe(live.db);                                                \
        const std::string nextBefore = live.ids.peekPatientId();                                     \
        bool ok_ = false; std::string msg_;                                                          \
        try { FileHandler::fromJson(jsonBytes, live.db, live.ids); }                                 \
        catch (const utils::ClinicException& e_) {                                                   \
            ok_ = (e_.code() == utils::ErrorCode::expected); msg_ = e_.what();                       \
        } catch (...) {}                                                                             \
        testing::report(ok_, "load fails with " #expected, __FILE__, __LINE__);                      \
        testing::report(msg_.find(text) != std::string::npos, "message contains \"" text "\" but was: " + msg_, __FILE__, __LINE__); \
        testing::report(describe(live.db) == before, "live data unchanged after failed load", __FILE__, __LINE__); \
        testing::report(live.ids.peekPatientId() == nextBefore, "ID counters unchanged after failed load", __FILE__, __LINE__); \
    } while (0)

} // namespace

static void testDateArithmetic() {
    CHECK_EQ(v::addDays("2026-10-28", 0), std::string("2026-10-28"));
    CHECK_EQ(v::addDays("2026-10-28", 3), std::string("2026-10-31"));
    CHECK_EQ(v::addDays("2026-10-28", 4), std::string("2026-11-01"));       // month end
    CHECK_EQ(v::addDays("2026-12-31", 1), std::string("2027-01-01"));       // year end
    CHECK_EQ(v::addDays("2028-02-28", 1), std::string("2028-02-29"));       // leap day
    CHECK_EQ(v::addDays("2027-03-01", -1), std::string("2027-02-28"));
    CHECK_EQ(v::addDays("2026-10-28", -30), std::string("2026-09-28"));
    CHECK_EQ(v::addDays("2026-10-28", 365), std::string("2027-10-28"));
    CHECK_THROWS(v::addDays("2026-02-30", 1), InvalidAppointment);
}

static void testDemoDataIsValid() {
    Demo d;
    CHECK_EQ(d.db.patientCount(), std::size_t(10));
    CHECK_EQ(d.db.services().size(), std::size_t(4));
    CHECK_EQ(d.mgr.countByCategory(model::PatientCategory::General), 5);
    CHECK_EQ(d.mgr.countByCategory(model::PatientCategory::Emergency), 2);
    CHECK_EQ(d.mgr.countByCategory(model::PatientCategory::FollowUp), 3);
    // the demo must survive the loader's strict checks
    ClinicDatabase copy; IDGenerator ids;
    FileHandler::fromJson(FileHandler::toJson(d.db), copy, ids);
    CHECK(describe(copy) == describe(d.db));
    // works for any "today", e.g. across a year end
    ClinicDatabase other; IDGenerator ids2;
    DemoData::populate(other, ids2, "2026-12-30");
    CHECK_EQ(other.patientCount(), std::size_t(10));
    FileHandler::fromJson(FileHandler::toJson(other), copy, ids);
}

static void testRoundTrip() {
    Demo d;
    d.mgr.deletePatient("P-0002");                                    // creates history
    controller::PatientInput edit; edit.name = "Amara Okafor-Eze"; edit.age = 35; edit.phone = "0803 555 0101";
    d.mgr.editPatient("P-0001", edit);
    d.db.findService("S-003")->active = false;                        // an inactive service

    const QByteArray json = FileHandler::toJson(d.db);
    ClinicDatabase loaded; IDGenerator ids;
    FileHandler::fromJson(json, loaded, ids);
    CHECK(describe(loaded) == describe(d.db));
    CHECK_EQ(loaded.history().size(), d.db.history().size());
    CHECK(loaded.history().size() >= 1);
    CHECK(!loaded.findService("S-003")->active);
    CHECK_EQ(loaded.patientCount(), std::size_t(9));

    // category-specific data survived
    CHECK_EQ(loaded.findPatient("P-0006")->summary(), std::string("Triage level 2 (Urgent)"));
    CHECK(loaded.findPatient("P-0003")->summary().find("Blood pressure review") != std::string::npos);
    CHECK(loaded.findPatient("P-0001")->category() == model::PatientCategory::General);

    // new IDs continue after the highest loaded ones (no clashes)
    CHECK_EQ(ids.nextPatientId(), std::string("P-0011"));
    CHECK_EQ(ids.nextServiceId(), std::string("S-005"));
    CHECK_EQ(ids.nextAppointmentId(), std::string("A-0023"));   // the demo uses A-0001 .. A-0022

    // saving the loaded data gives identical JSON (stable format)
    CHECK(FileHandler::toJson(loaded) == json);
}

static void testLoadReplacesEverything() {
    Demo live;                                                         // has 10 patients
    std::vector<std::string> invalidated;
    live.db.setInvalidationCallback([&](const std::string& id) { invalidated.push_back(id); });
    const unsigned long versionBefore = live.db.patientsVersion();

    ClinicDatabase small; IDGenerator smallIds;
    controller::UndoManager u(small);
    controller::ClinicManager m(small, smallIds, u);
    controller::PatientInput in; in.name = "Only One"; in.age = 40; in.phone = "08031112222";
    m.registerPatient(in);
    FileHandler::fromJson(FileHandler::toJson(small), live.db, live.ids);

    CHECK_EQ(live.db.patientCount(), std::size_t(1));                  // replaced, NOT merged
    CHECK_EQ(live.db.services().size(), std::size_t(0));
    CHECK_EQ(invalidated.size(), std::size_t(10));                     // cache/search views were told first
    CHECK(live.db.patientsVersion() > versionBefore);
    CHECK_EQ(live.ids.peekPatientId(), std::string("P-0002"));
}

static void testBadFilesAreRejectedSafely() {
    const QByteArray good = validJson();

    CHECK_LOAD_FAILS(QByteArray("this is not json {"), FileError, "not a valid SCAPMS data file");
    CHECK_LOAD_FAILS(QByteArray("[1,2,3]"), FileError, "not a valid SCAPMS data file");
    CHECK_LOAD_FAILS(QByteArray(""), FileError, "not a valid SCAPMS data file");
    CHECK_LOAD_FAILS(mutate(good, [](QJsonObject& r) { r["format"] = "OTHER"; }), FileError, "not created by SCAPMS");
    CHECK_LOAD_FAILS(mutate(good, [](QJsonObject& r) { r.remove("schemaVersion"); }), FileError, "no schema version");
    CHECK_LOAD_FAILS(mutate(good, [](QJsonObject& r) { r["schemaVersion"] = 99; }), FileError, "newer version");
    CHECK_LOAD_FAILS(mutate(good, [](QJsonObject& r) { r["schemaVersion"] = 0; }), FileError, "invalid schema version");
    CHECK_LOAD_FAILS(mutate(good, [](QJsonObject& r) { r.remove("patients"); }), FileError, "\"patients\" is missing");
    CHECK_LOAD_FAILS(mutate(good, [](QJsonObject& r) { r.remove("services"); }), FileError, "\"services\" is missing");
    CHECK_LOAD_FAILS(mutate(good, [](QJsonObject& r) { r.remove("history"); }), FileError, "\"history\" is missing");

    // patient problems: the message says which patient, and which rule was broken
    CHECK_LOAD_FAILS(mutate(good, [](QJsonObject& r) { auto p = patientAt(r, 0); p["age"] = 150; setPatient(r, 0, p); }),
                     InvalidAge, "between 0 and 120");
    CHECK_LOAD_FAILS(mutate(good, [](QJsonObject& r) { auto p = patientAt(r, 0); p["age"] = "forty"; setPatient(r, 0, p); }),
                     InvalidAge, "whole number");
    CHECK_LOAD_FAILS(mutate(good, [](QJsonObject& r) { auto p = patientAt(r, 0); p["age"] = 30.5; setPatient(r, 0, p); }),
                     InvalidAge, "whole number");
    CHECK_LOAD_FAILS(mutate(good, [](QJsonObject& r) { auto p = patientAt(r, 0); p["name"] = "   "; setPatient(r, 0, p); }),
                     FileError, "\"name\" is empty");
    CHECK_LOAD_FAILS(mutate(good, [](QJsonObject& r) { auto p = patientAt(r, 0); p["category"] = "VIP"; setPatient(r, 0, p); }),
                     FileError, "unknown patient category");
    CHECK_LOAD_FAILS(mutate(good, [](QJsonObject& r) { auto p = patientAt(r, 0); p["phone"] = ""; p["email"] = ""; setPatient(r, 0, p); }),
                     InvalidContact, "contact");
    CHECK_LOAD_FAILS(mutate(good, [](QJsonObject& r) { auto p = patientAt(r, 0); p.remove("id"); setPatient(r, 0, p); }),
                     FileError, "\"id\" is missing");
    CHECK_LOAD_FAILS(mutate(good, [](QJsonObject& r) { auto p = patientAt(r, 0); p.remove("appointments"); setPatient(r, 0, p); }),
                     FileError, "\"appointments\" is missing");
    CHECK_LOAD_FAILS(mutate(good, [](QJsonObject& r) { auto p = patientAt(r, 4); p["triageLevel"] = 9; setPatient(r, 4, p);   /* index 4 = Chinedu, Emergency */ }),
                     InvalidPatient, "Triage level");
    CHECK_LOAD_FAILS(mutate(good, [](QJsonObject& r) { auto p = patientAt(r, 1); p["previousVisitDate"] = "2026-02-31"; setPatient(r, 1, p);   /* index 1 = Ngozi, Follow-Up */ }),
                     InvalidPatient, "Previous visit date");

    // duplicate patient ID
    CHECK_LOAD_FAILS(mutate(good, [](QJsonObject& r) { auto p = patientAt(r, 1); p["id"] = "P-0001"; setPatient(r, 1, p); }),
                     DuplicateId, "Patient ID already exists: P-0001");

    // appointment problems
    CHECK_LOAD_FAILS(mutate(good, [](QJsonObject& r) {
        auto p = patientAt(r, 0); auto a = p["appointments"].toArray(); auto o = a.at(0).toObject();
        o["status"] = "Done"; a.replace(0, o); p["appointments"] = a; setPatient(r, 0, p); }), FileError, "unknown appointment status");
    CHECK_LOAD_FAILS(mutate(good, [](QJsonObject& r) {
        auto p = patientAt(r, 0); auto a = p["appointments"].toArray(); auto o = a.at(0).toObject();
        o["dateTime"] = "tomorrow"; a.replace(0, o); p["appointments"] = a; setPatient(r, 0, p); }), InvalidAppointment, "date/time");
    CHECK_LOAD_FAILS(mutate(good, [](QJsonObject& r) {
        auto p = patientAt(r, 0); auto a = p["appointments"].toArray(); auto o = a.at(0).toObject();
        o["serviceId"] = "S-099"; a.replace(0, o); p["appointments"] = a; setPatient(r, 0, p); }), FileError, "unknown service");
    CHECK_LOAD_FAILS(mutate(good, [](QJsonObject& r) {   // same appointment ID used by two different patients
        auto p0 = patientAt(r, 0); auto a0 = p0["appointments"].toArray(); QString id = a0.at(0).toObject()["id"].toString();
        auto p1 = patientAt(r, 1); auto a1 = p1["appointments"].toArray(); auto o = a1.at(0).toObject();
        o["id"] = id; a1.replace(0, o); p1["appointments"] = a1; setPatient(r, 1, p1); }), DuplicateId, "Appointment ID already exists");
    CHECK_LOAD_FAILS(mutate(good, [](QJsonObject& r) {   // two live appointments, same service and time
        auto p = patientAt(r, 0); auto a = p["appointments"].toArray(); auto o = a.at(0).toObject();
        auto dup = o; dup["id"] = "A-9001"; dup["status"] = "Pending"; o["status"] = "Pending";
        a.replace(0, o); a.append(dup); p["appointments"] = a; setPatient(r, 0, p); }), DuplicateAppointment, "same service");
    CHECK_LOAD_FAILS(mutate(good, [](QJsonObject& r) {   // history entry without its patient ID
        auto h = r["history"].toArray(); auto o = h.at(0).toObject(); o.remove("patientId"); h.replace(0, o); r["history"] = h; }),
                     FileError, "\"patientId\" is missing");
    CHECK_LOAD_FAILS(mutate(good, [](QJsonObject& r) {   // service problems
        auto s = r["services"].toArray(); auto o = s.at(0).toObject(); o["active"] = "yes"; s.replace(0, o); r["services"] = s; }),
                     FileError, "true or false");
    CHECK_LOAD_FAILS(mutate(good, [](QJsonObject& r) {
        auto s = r["services"].toArray(); auto o = s.at(1).toObject(); o["id"] = "S-001"; s.replace(1, o); r["services"] = s; }),
                     DuplicateId, "Service ID already exists");
}

static void testFiles() {
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString path = dir.path() + "/nested/folder/clinic.json";   // folders are created when saving
    Demo d;
    FileHandler::save(d.db, path);
    CHECK(QFile::exists(path));

    ClinicDatabase loaded; IDGenerator ids;
    FileHandler::load(path, loaded, ids);
    CHECK(describe(loaded) == describe(d.db));

    // overwrite with different data: file always contains complete, valid JSON
    d.mgr.deletePatient("P-0002");
    FileHandler::save(d.db, path);
    FileHandler::load(path, loaded, ids);
    CHECK_EQ(loaded.patientCount(), std::size_t(9));

    // problems with files
    ClinicDatabase untouched; IDGenerator uIds;
    DemoData::populate(untouched, uIds, TODAY);
    const std::string before = describe(untouched);
    CHECK_THROWS(FileHandler::load(dir.path() + "/missing.json", untouched, uIds), FileError);
    QFile bad(dir.path() + "/garbage.json");
    CHECK(bad.open(QIODevice::WriteOnly)); bad.write("{\"format\": \"SCAPMS\", \"schemaVersion\": 1, "); bad.close();   // cut off
    CHECK_THROWS(FileHandler::load(dir.path() + "/garbage.json", untouched, uIds), FileError);
    CHECK(describe(untouched) == before);                                                           // still intact

    QFile blocker(dir.path() + "/afile");
    CHECK(blocker.open(QIODevice::WriteOnly)); blocker.write("x"); blocker.close();
    CHECK_THROWS(FileHandler::save(d.db, dir.path() + "/afile/clinic.json"), FileError);           // parent is a file
    CHECK_THROWS(FileHandler::save(d.db, dir.path()), FileError);                                  // path is a directory

    // the bundled sample file format loads through a path too
    FileHandler::writeTextFile(dir.path() + "/note.txt", "hello");
    QFile note(dir.path() + "/note.txt");
    CHECK(note.open(QIODevice::ReadOnly)); CHECK_EQ(note.readAll().toStdString(), std::string("hello"));
    CHECK_THROWS(FileHandler::writeTextFile(dir.path() + "/afile/x.txt", "x"), FileError);
}

static void testCsvExport() {
    ClinicDatabase db; IDGenerator ids;
    controller::UndoManager undo(db);
    controller::ClinicManager mgr(db, ids, undo);
    controller::PatientInput a; a.name = "Okafor, Amara \"Ama\""; a.age = 34; a.phone = "0803 555 0101";
    mgr.registerPatient(a);
    controller::PatientInput b; b.category = model::PatientCategory::Emergency; b.name = "Tunde Bakare"; b.age = 8;
    b.email = "tunde@example.com"; b.triageLevel = 1;
    mgr.registerPatient(b);

    const QString csv = QString::fromUtf8(FileHandler::patientsCsv(db));
    const QStringList lines = csv.split('\n', Qt::SkipEmptyParts);
    CHECK_EQ(lines.size(), 3);                                                  // header + 2 patients
    CHECK(lines[0].startsWith("Patient ID,Name,Age,Category,Priority"));
    CHECK(lines[1].startsWith("P-0001,\"Okafor, Amara \"\"Ama\"\"\",34,General,Normal,0803 555 0101"));   // quoted correctly
    CHECK(lines[2].contains("Emergency,High Priority"));
    CHECK(lines[2].contains("Triage level 1 (Critical)"));

    QTemporaryDir dir;
    FileHandler::exportPatientsCsv(db, dir.path() + "/patients.csv");
    QFile f(dir.path() + "/patients.csv");
    CHECK(f.open(QIODevice::ReadOnly));
    CHECK_EQ(f.readAll(), FileHandler::patientsCsv(db));
}

static void testReports() {
    Demo d;
    ReportGenerator rep(d.db);

    // 1. most frequently attended: completed counts, highest first, ties by name
    auto freq = rep.mostFrequentlyAttended();
    CHECK_EQ(freq.size(), std::size_t(5));
    CHECK_EQ(freq[0].name, std::string("Amara Okafor"));  CHECK_EQ(freq[0].completed, 3);
    CHECK_EQ(freq[1].name, std::string("Ibrahim Musa"));  CHECK_EQ(freq[1].completed, 2);   // 2 completed, tie ordered by name
    CHECK_EQ(freq[2].name, std::string("Ngozi Eze"));     CHECK_EQ(freq[2].completed, 2);
    CHECK_EQ(freq[3].name, std::string("Tunde Bakare"));  CHECK_EQ(freq[3].completed, 2);
    for (std::size_t i = 1; i < freq.size(); ++i) CHECK(freq[i - 1].completed >= freq[i].completed);
    CHECK_EQ(rep.mostFrequentlyAttended(2).size(), std::size_t(2));
    CHECK_EQ(rep.mostFrequentlyAttended(100).size(), std::size_t(6));                      // only patients with completed visits

    // 2. pending appointments: next appointment first
    auto pending = rep.patientsWithPending();
    CHECK_EQ(pending.size(), std::size_t(9));
    CHECK_EQ(pending[0].name, std::string("Zainab Lawal"));  CHECK_EQ(pending[0].nextDateTime, std::string("2026-10-28 09:30"));
    for (std::size_t i = 1; i < pending.size(); ++i) CHECK(pending[i - 1].nextDateTime <= pending[i].nextDateTime);
    for (const auto& r : pending) CHECK(r.pending >= 1);
    int totalPending = 0; for (const auto& r : pending) totalPending += r.pending;
    CHECK_EQ(totalPending, d.mgr.pendingAppointmentCount());

    // 3. category distribution: General 5 / Emergency 2 / Follow-Up 3 of 10
    auto dist = rep.categoryDistribution();
    CHECK_EQ(dist.size(), std::size_t(3));
    CHECK_EQ(dist[0].name, std::string("General Patients"));   CHECK_EQ(dist[0].count, 5);  CHECK_EQ(dist[0].percent, 50);
    CHECK_EQ(dist[1].name, std::string("Emergency Patients")); CHECK_EQ(dist[1].count, 2);  CHECK_EQ(dist[1].percent, 20);
    CHECK_EQ(dist[2].name, std::string("Follow-Up Patients")); CHECK_EQ(dist[2].count, 3);  CHECK_EQ(dist[2].percent, 30);
    ClinicDatabase empty;
    for (const auto& r : ReportGenerator(empty).categoryDistribution()) CHECK(r.count == 0 && r.percent == 0);

    // 4. daily summary
    auto today = rep.dailySummary(TODAY);
    CHECK_EQ(today.total, 4);  CHECK_EQ(today.completed, 1);  CHECK_EQ(today.pending, 3);  CHECK_EQ(today.cancelled, 0);
    auto past = rep.dailySummary("2026-10-24");                                           // today-4: a cancelled one
    CHECK_EQ(past.total, 1);  CHECK_EQ(past.cancelled, 1);
    CHECK_EQ(rep.dailySummary("2030-01-01").total, 0);
    // appointments of a deleted patient still count for their day (history)
    d.mgr.deletePatient("P-0002");                                                         // Tunde: completed today
    auto after = rep.dailySummary(TODAY);
    CHECK_EQ(after.total, 4);  CHECK_EQ(after.completed, 1);
    CHECK_EQ(rep.mostFrequentlyAttended(100).size(), std::size_t(5));                      // but he leaves the live patient report

    // 5. polymorphic overview
    auto overview = rep.patientOverview();
    CHECK_EQ(overview.size(), std::size_t(9));
    CHECK_EQ(overview[0], std::string("[P-0001] Amara Okafor (General, Normal) - Routine patient"));

    // text export
    const std::string text = rep.renderText(TODAY);
    CHECK(text.find("1. Most Frequently Attended Patients") != std::string::npos);
    CHECK(text.find("1. Amara Okafor (P-0001, General) - 3 completed") != std::string::npos);
    CHECK(text.find("General Patients: 5 (56%)") != std::string::npos);                    // 9 patients now: 5/9
    CHECK(text.find("Total Appointments: 4") != std::string::npos);
    CHECK(text.find("Daily Appointment Summary for 2026-10-28") != std::string::npos);
    CHECK(text.find("Emergency, High Priority") != std::string::npos);
    CHECK(ReportGenerator(empty).renderText(TODAY).find("(no completed appointments yet)") != std::string::npos);
}

static void testUndoDescriptions() {
    Demo d;
    CHECK(d.undo.descriptions().empty());
    d.mgr.deletePatient("P-0002");
    controller::PatientInput edit; edit.name = "Amara Okafor-Eze"; edit.age = 35; edit.phone = "0803 555 0101";
    d.mgr.editPatient("P-0001", edit);
    auto list = d.undo.descriptions();
    CHECK_EQ(list.size(), std::size_t(2));
    CHECK_EQ(list[0], std::string("Edit patient P-0001 (Amara Okafor)"));                   // newest first
    CHECK_EQ(list[1], std::string("Delete patient P-0002 (Tunde Bakare)"));
    d.undo.undoLast();
    CHECK_EQ(d.undo.descriptions().size(), std::size_t(1));
    d.undo.clear();
    CHECK(d.undo.descriptions().empty());
}

void runFileReportTests() {
    testDateArithmetic();
    testDemoDataIsValid();
    testRoundTrip();
    testLoadReplacesEverything();
    testBadFilesAreRejectedSafely();
    testFiles();
    testCsvExport();
    testReports();
    testUndoDescriptions();
}
