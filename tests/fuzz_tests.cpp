// Randomised stress test: performs many random operations with a fixed seed and checks the
// consistency rules of the data after every step.
#include "controller/AppointmentController.h"
#include "controller/ClinicManager.h"
#include "controller/PatientCache.h"
#include "controller/QueueManager.h"
#include "controller/SearchEngine.h"
#include "controller/SortEngine.h"
#include "controller/UndoManager.h"
#include "model/ClinicDatabase.h"
#include "test_support.h"
#include "utils/FileHandler.h"
#include "utils/IDGenerator.h"
#include "utils/Validation.h"

#include <cstdio>
#include <random>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace scapms;
using controller::PatientInput;
using controller::PatientList;
using controller::SearchAlgorithm;
using controller::SearchEngine;
using controller::SearchField;
using controller::SortAlgorithm;
using controller::SortEngine;
using controller::SortField;
using model::AppointmentStatus;
using model::PatientCategory;
namespace v = utils::validation;

namespace {

const std::string NOW = "2026-10-24 10:00";

struct World {
    model::ClinicDatabase db;
    utils::IDGenerator ids;
    controller::UndoManager undo{db};
    controller::ClinicManager mgr{db, ids, undo};
    controller::AppointmentController appts{db, ids, undo};
    controller::QueueManager queue{db};
    controller::PatientCache cache;
    int serviceCount = 0;

    World() {
        appts.setAppointmentClosedCallback([this](const std::string& id) { queue.onAppointmentClosed(id); });
        db.setInvalidationCallback([this](const std::string& id) { cache.invalidate(id); });
    }
};

struct Context { unsigned seed; int step; };

std::vector<std::string> g_trace;   // the most recent operations, shown if a check fails
int g_reported = 0;

std::string where(const Context& c) { return "seed " + std::to_string(c.seed) + ", step " + std::to_string(c.step); }

void require(bool ok, const std::string& what, const Context& c) {
    testing::report(ok, what + " (" + where(c) + ")", __FILE__, __LINE__);
    if (!ok && g_reported++ < 2) {
        std::cerr << "  recent operations:\n";
        for (std::size_t i = g_trace.size() > 8 ? g_trace.size() - 8 : 0; i < g_trace.size(); ++i) std::cerr << "    " << g_trace[i] << "\n";
    }
}

// ------------------------------------------------------------------ consistency rules
void checkInvariants(World& w, const Context& c) {
    std::set<std::string> patientIds, appointmentIds;
    for (const auto& p : w.db.patients()) {
        require(patientIds.insert(p->id()).second, "patient IDs are unique", c);
        for (const auto& a : p->appointments()) {
            require(appointmentIds.insert(a.id()).second, "appointment IDs are unique", c);
            require(a.patientId() == p->id() && a.patientNameSnapshot() == p->name(), "appointment shows its owner", c);
        }
        const auto& list = p->appointments();
        for (std::size_t i = 0; i < list.size(); ++i)
            for (std::size_t j = i + 1; j < list.size(); ++j)
                require(!(list[i].status() != AppointmentStatus::Cancelled && list[j].status() != AppointmentStatus::Cancelled &&
                          list[i].serviceId() == list[j].serviceId() && list[i].dateTime() == list[j].dateTime()),
                        "no two live appointments for the same service and time", c);
    }
    for (const auto& a : w.db.history())
        require(appointmentIds.insert(a.id()).second, "history appointment IDs are unique", c);

    // waiting queue: each entry refers to an existing patient with a pending appointment; nobody twice
    std::set<std::string> queued;
    auto checkEntry = [&](const controller::QueueEntry& e) {
        const model::PatientRecord* p = w.db.findPatient(e.patientId);
        require(p != nullptr, "queue entry refers to an existing patient", c);
        if (!p) return;
        const model::Appointment* a = p->findAppointment(e.appointmentId);
        require(a != nullptr, "queue entry refers to an existing appointment (" + e.appointmentId + " of " + e.patientId + ")", c);
        require(a == nullptr || a->status() == AppointmentStatus::Pending,
                std::string("queue entry refers to a pending appointment (status is ") + (a ? model::statusName(a->status()) : "?") + ")", c);
        require(queued.insert(e.patientId).second, "a patient is in the queue only once", c);
    };
    for (const auto& e : w.queue.waiting()) checkEntry(e);
    if (w.queue.nowServing()) checkEntry(*w.queue.nowServing());

    // cache: every cached pointer is a patient that still exists
    for (const auto& entry : w.cache.mostAccessed()) {
        const model::PatientRecord* p = w.db.findPatient(entry.patient->id());
        require(p == entry.patient, "cache points to a live patient", c);
    }
}

// ------------------------------------------------------------------ random helpers
struct Rng {
    std::mt19937 gen;
    explicit Rng(unsigned seed) : gen(seed) {}
    int upTo(int n) { return static_cast<int>(gen() % static_cast<unsigned>(n)); }  // 0 .. n-1
    bool chance(int percent) { return upTo(100) < percent; }
};

PatientInput randomInput(Rng& r, bool allowInvalid) {
    static const char* names[] = {"Ada Obi", "Bayo Ade", "Chi Eze", "Dayo Bello", "Efe Okoro", "Funmi Alade", "Gbenga Ola", "Hauwa Musa",
                                  "Ife Ajayi", "Jide Cole", "Kemi Bello", "Lola Eze"};
    PatientInput in;
    switch (r.upTo(3)) {
        case 0: in.category = PatientCategory::General; break;
        case 1: in.category = PatientCategory::Emergency; break;
        default: in.category = PatientCategory::FollowUp; break;
    }
    in.name = names[r.upTo(12)];
    in.age = r.upTo(100);
    in.phone = "0803555" + std::to_string(1000 + r.upTo(8000));
    in.triageLevel = 1 + r.upTo(3);
    in.previousVisitDate = "2026-09-" + std::string(r.chance(50) ? "10" : "20");
    in.followUpReason = r.chance(50) ? "Review" : "";
    if (allowInvalid && r.chance(15)) {
        switch (r.upTo(4)) {
            case 0: in.age = 121 + r.upTo(50); break;
            case 1: in.name = "  "; break;
            case 2: in.phone = ""; in.email = ""; break;
            default: in.triageLevel = 9; break;
        }
    }
    return in;
}

std::string randomDateTime(Rng& r) {
    const int day = r.upTo(14) - 3;  // 3 days ago .. 10 days ahead
    char buf[32];
    std::snprintf(buf, sizeof buf, " %02d:%02d", 8 + r.upTo(10), 15 * r.upTo(4));
    return v::addDays("2026-10-24", day) + buf;
}

const model::PatientRecord* randomPatient(World& w, Rng& r) {
    if (w.db.patients().empty()) return nullptr;
    return w.db.patients()[static_cast<std::size_t>(r.upTo(static_cast<int>(w.db.patients().size())))].get();
}

// a random appointment of a live patient; status filter: -1 = any
const model::Appointment* randomAppointment(World& w, Rng& r, int status) {
    std::vector<const model::Appointment*> pool;
    for (const auto& p : w.db.patients())
        for (const auto& a : p->appointments())
            if (status < 0 || static_cast<int>(a.status()) == status) pool.push_back(&a);
    return pool.empty() ? nullptr : pool[static_cast<std::size_t>(r.upTo(static_cast<int>(pool.size())))];
}

std::vector<std::string> idsOf(const std::vector<const model::PatientRecord*>& list) {
    std::vector<std::string> out;
    for (auto* p : list) out.push_back(p->id());
    return out;
}

// sort the patients with a random algorithm, then compare all the searches that are possible
void sortAndSearch(World& w, Rng& r, const Context& c) {
    PatientList list;
    for (const auto& p : w.db.patients()) list.push_back(p.get());
    const SortField sortField = static_cast<SortField>(r.upTo(3));
    SortEngine::sort(list, r.chance(50) ? SortAlgorithm::Selection : SortAlgorithm::Insertion, sortField);
    require(SortEngine::isSortedBy(list, sortField), "list is sorted after sorting", c);
    require(list.size() == w.db.patientCount(), "sorting keeps every patient", c);

    const SearchField field = static_cast<SearchField>(r.upTo(3));
    std::string keyword;
    if (field == SearchField::Category) {
        static const char* cats[] = {"general", "Emergency", "FOLLOW-UP"};
        keyword = cats[r.upTo(3)];
    } else if (const model::PatientRecord* p = randomPatient(w, r)) {
        keyword = field == SearchField::Id ? p->id() : p->name().substr(0, 1 + static_cast<std::size_t>(r.upTo(4)));
    } else {
        keyword = "x";
    }
    const std::optional<SortField> sorted = sortField;
    const auto linear = SearchEngine::linearSearch(list, field, keyword);
    const auto recursive = SearchEngine::recursiveSearch(list, field, keyword);
    require(idsOf(linear.matches) == idsOf(recursive.matches), "linear and recursive search agree", c);
    if (SearchEngine::isAlgorithmValid(SearchAlgorithm::Binary, field, sorted)) {
        const auto binary = SearchEngine::binarySearch(list, field, keyword, sorted);
        require(idsOf(linear.matches) == idsOf(binary.matches), "linear and binary search agree", c);
    }
}

// ------------------------------------------------------------------ one random operation
void note(const std::string& text) {
    g_trace.push_back(text);
    if (g_trace.size() > 40) g_trace.erase(g_trace.begin());
}

void step(World& w, Rng& r, const Context& c) {
    try {
        const int op = r.upTo(20);
        note("op " + std::to_string(op));
        switch (op) {
            case 0: case 1: case 2: w.mgr.registerPatient(randomInput(r, true)); break;
            case 3:
                if (const auto* p = randomPatient(w, r)) {
                    PatientInput in = randomInput(r, true);
                    in.category = p->category();
                    w.mgr.editPatient(p->id(), in);
                }
                break;
            case 4:
                if (r.chance(40))
                    if (const auto* p = randomPatient(w, r)) w.mgr.deletePatient(p->id());
                break;
            case 5: w.appts.addService("Service " + std::to_string(++w.serviceCount), "generated"); break;
            case 6:
                if (!w.db.services().empty())
                    w.appts.setServiceActive(w.db.services()[static_cast<std::size_t>(r.upTo(static_cast<int>(w.db.services().size())))].id, r.chance(70));
                break;
            case 7: case 8: case 9: case 10:
                if (const auto* p = randomPatient(w, r); p && !w.db.services().empty())
                    w.appts.schedule(p->id(), w.db.services()[static_cast<std::size_t>(r.upTo(static_cast<int>(w.db.services().size())))].id,
                                     randomDateTime(r), "visit", NOW);
                break;
            case 11: if (const auto* a = randomAppointment(w, r, -1)) { note("cancel " + a->id()); w.appts.cancel(a->id()); } break;
            case 12: if (const auto* a = randomAppointment(w, r, -1)) { note("complete " + a->id()); w.appts.complete(a->id(), "done"); } break;
            case 13: if (r.chance(50)) if (const auto* a = randomAppointment(w, r, -1)) { note("remove " + a->id()); w.appts.remove(a->id()); } break;
            case 14: case 15:
                if (const auto* a = randomAppointment(w, r, static_cast<int>(AppointmentStatus::Pending))) { note("join " + a->id() + " of " + a->patientId()); w.queue.join(a->id(), NOW); }
                break;
            case 16:
                switch (r.upTo(3)) {
                    case 0: w.queue.callNext(); break;
                    case 1: w.queue.release(); break;
                    default:
                        if (const auto* p = randomPatient(w, r)) w.queue.remove(p->id());
                        break;
                }
                break;
            case 17: note("undo " + w.undo.nextDescription()); w.undo.undoLast(); break;
            case 18: if (const auto* p = randomPatient(w, r)) w.cache.recordAccess(*p); break;
            default: sortAndSearch(w, r, c); break;
        }
    } catch (const utils::ClinicException&) {
        // a refused operation (invalid input, a rule that does not allow it) is normal; the data must still be consistent
    } catch (...) {
        require(false, "only ClinicException may be thrown", c);
    }
}

void saveLoadRoundTrip(World& w, const Context& c) {
    const QByteArray json = utils::FileHandler::toJson(w.db);
    model::ClinicDatabase copy;
    utils::IDGenerator copyIds;
    utils::FileHandler::fromJson(json, copy, copyIds);
    require(utils::FileHandler::toJson(copy) == json, "save then load gives identical data", c);
    // the ID counters continue after the loaded IDs
    std::set<std::string> used;
    for (const auto& p : copy.patients()) used.insert(p->id());
    require(used.count(copyIds.peekPatientId()) == 0, "next patient ID is unused after loading", c);

    // occasionally replace the live data with the loaded copy, as File > Load does
    if (c.step % 3 == 0) {
        utils::FileHandler::fromJson(json, w.db, w.ids);
        w.undo.clear();
        w.queue.clear();
        w.cache.clear();
    }
}

void runSeed(unsigned seed, int steps) {
    World w;
    Rng r(seed);
    Context c{seed, 0};
    for (int i = 0; i < steps; ++i) {
        c.step = i;
        step(w, r, c);
        checkInvariants(w, c);
        if (i % 150 == 149) saveLoadRoundTrip(w, c);
    }
    require(w.db.patientCount() > 0 || steps < 50, "the run produced patients", c);
}

} // namespace

void runFuzzTests() {
    for (unsigned seed = 1; seed <= 6; ++seed) runSeed(seed, 1500);
}
