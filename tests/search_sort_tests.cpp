#include "controller/ClinicManager.h"
#include "controller/PatientCache.h"
#include "controller/SearchEngine.h"
#include "controller/SortEngine.h"
#include "controller/UndoManager.h"
#include "model/ClinicDatabase.h"
#include "test_support.h"
#include "utils/IDGenerator.h"
#include "utils/Validation.h"

#include <algorithm>  // std::sort is used only in these tests, as a reference to check the hand-written sorts against
#include <string>
#include <vector>

using namespace scapms;
using controller::PatientCache;
using controller::PatientInput;
using controller::PatientList;
using controller::SearchAlgorithm;
using controller::SearchEngine;
using controller::SearchField;
using controller::SortAlgorithm;
using controller::SortEngine;
using controller::SortField;
using model::PatientCategory;
namespace v = utils::validation;

namespace {

struct Fixture {
    model::ClinicDatabase db;
    utils::IDGenerator ids;
    controller::UndoManager undo{db};
    controller::ClinicManager mgr{db, ids, undo};

    void add(const std::string& name, int age, PatientCategory cat = PatientCategory::General) {
        PatientInput in;
        in.category = cat; in.name = name; in.age = age; in.phone = "08035550101";
        in.triageLevel = 2; in.previousVisitDate = "2026-01-01";
        mgr.registerPatient(in);
    }
    PatientList list() const {
        PatientList l;
        for (const auto& p : db.patients()) l.push_back(p.get());
        return l;
    }
    // 12 patients: duplicate ages (8, 30), duplicate names (Ada Obi x2), mixed case, extra spaces
    void fillSample() {
        add("Ada Obi", 30);                                  // P-0001
        add("adebayo Ade", 8, PatientCategory::Emergency);   // P-0002
        add("Chi Eze", 52, PatientCategory::FollowUp);       // P-0003
        add("Bayo Ade", 8, PatientCategory::Emergency);      // P-0004
        add("Dayo Bello", 67);                               // P-0005
        add("Efe Okoro", 41);                                // P-0006
        add("Funmi Alade", 29);                              // P-0007
        add("Gbenga Ola", 30, PatientCategory::FollowUp);    // P-0008
        add("Hauwa Musa", 8);                                // P-0009
        add("Ife   Ajayi", 30);                              // P-0010
        add("ADA Obi", 31);                                  // P-0011
        add("Zainab Yusuf", 0);                              // P-0012
    }
};

std::vector<std::string> ids(const PatientList& l) {
    std::vector<std::string> out;
    for (auto* p : l) out.push_back(p->id());
    return out;
}

// reference ordering used to check the results
bool refLess(const model::PatientRecord* a, const model::PatientRecord* b, SortField f) {
    if (f == SortField::Age) { if (a->age() != b->age()) return a->age() < b->age(); }
    else if (f == SortField::Name) {
        const auto x = v::normalizedName(a->name()), y = v::normalizedName(b->name());
        if (x != y) return x < y;
    }
    return a->id() < b->id();
}

} // namespace

static void testStringHelpers() {
    CHECK_EQ(v::toLower("AdA-P"), std::string("ada-p"));
    CHECK_EQ(v::normalizedName("  Ife   AJAYI "), std::string("ife ajayi"));
    CHECK(v::startsWith("ada obi", "ada"));
    CHECK(v::startsWith("ada", "ada"));
    CHECK(v::startsWith("ada", ""));
    CHECK(!v::startsWith("ad", "ada"));
    CHECK(!v::startsWith("bada", "ada"));

    CHECK(v::contains("emeka obi", "obi"));
    CHECK(v::contains("emeka obi", "e"));
    CHECK(v::contains("emeka obi", "emeka obi"));
    CHECK(v::contains("abc", ""));
    CHECK(!v::contains("abc", "abcd"));
    CHECK(!v::contains("abc", "ac"));
    CHECK(v::contains("aab", "ab"));                 // the match may start after a partial match

    CHECK_EQ(v::normalizedIdKey("3"), std::string("p-0003"));
    CHECK_EQ(v::normalizedIdKey("0003"), std::string("p-0003"));
    CHECK_EQ(v::normalizedIdKey("p3"), std::string("p-0003"));
    CHECK_EQ(v::normalizedIdKey(" P-3 "), std::string("p-0003"));
    CHECK_EQ(v::normalizedIdKey("P-0123"), std::string("p-0123"));
    CHECK_EQ(v::normalizedIdKey("P-12345"), std::string("p-12345"));
    CHECK_EQ(v::normalizedIdKey("abc"), std::string("abc"));
    CHECK_EQ(v::normalizedIdKey("P-1a"), std::string("p-1a"));
    CHECK_EQ(v::normalizedIdKey(""), std::string(""));
}

static void testSortsMatchReference() {
    Fixture f; f.fillSample();
    for (auto alg : {SortAlgorithm::Selection, SortAlgorithm::Insertion}) {
        for (auto field : {SortField::Name, SortField::Id, SortField::Age}) {
            PatientList mine = f.list();
            PatientList ref = f.list();
            SortEngine::sort(mine, alg, field);
            std::sort(ref.begin(), ref.end(), [&](auto* a, auto* b) { return refLess(a, b, field); });
            CHECK(ids(mine) == ids(ref));
            CHECK(SortEngine::isSortedBy(mine, field));
            CHECK_EQ(mine.size(), std::size_t(12));            // nothing lost or duplicated
        }
    }
    // both algorithms give the identical order (ties are broken by ID)
    PatientList a = f.list(), b = f.list();
    SortEngine::selectionSort(a, SortField::Age);
    SortEngine::insertionSort(b, SortField::Age);
    CHECK(ids(a) == ids(b));
    CHECK_EQ(a.front()->name(), std::string("Zainab Yusuf"));  // age 0 first
    CHECK_EQ(ids(a)[1], std::string("P-0002"));                // age 8 ties in ID order: P-0002, P-0004, P-0009
    CHECK_EQ(ids(a)[2], std::string("P-0004"));
    CHECK_EQ(ids(a)[3], std::string("P-0009"));

    // names compare case-insensitively with extra spaces collapsed: "Ada Obi" (P-0001) before "ADA Obi" (P-0011)
    PatientList n = f.list();
    SortEngine::insertionSort(n, SortField::Name);
    CHECK_EQ(ids(n)[0], std::string("P-0001"));
    CHECK_EQ(ids(n)[1], std::string("P-0011"));
    CHECK_EQ(ids(n)[2], std::string("P-0002"));                // adebayo
}

static void testSortStats() {
    Fixture f; f.fillSample();
    const long n = 12;

    PatientList sorted = f.list();
    SortEngine::insertionSort(sorted, SortField::Id);          // registration order IS ID order
    PatientList again = sorted;
    auto ins = SortEngine::insertionSort(again, SortField::Id);
    CHECK_EQ(ins.comparisons, n - 1);                          // already sorted: one comparison per element
    CHECK_EQ(ins.moves, 0L);
    again = sorted;
    auto sel = SortEngine::selectionSort(again, SortField::Id);
    CHECK_EQ(sel.comparisons, n * (n - 1) / 2);                // selection always does n(n-1)/2
    CHECK_EQ(sel.moves, 0L);                                   // ...and no swaps when already sorted

    PatientList reversed = f.list();
    for (std::size_t i = 0; i < reversed.size() / 2; ++i) std::swap(reversed[i], reversed[reversed.size() - 1 - i]);
    PatientList r2 = reversed;
    auto insRev = SortEngine::insertionSort(reversed, SortField::Id);
    CHECK_EQ(insRev.comparisons, n * (n - 1) / 2);             // worst case for insertion sort
    CHECK_EQ(insRev.moves, n * (n - 1) / 2);
    auto selRev = SortEngine::selectionSort(r2, SortField::Id);
    CHECK(selRev.moves <= n - 1);                              // selection: at most n-1 swaps
    CHECK_EQ(selRev.comparisons, n * (n - 1) / 2);

    // edge cases
    PatientList empty, one;
    one.push_back(f.list().front());
    CHECK_EQ(SortEngine::selectionSort(empty, SortField::Age).comparisons, 0L);
    CHECK_EQ(SortEngine::insertionSort(empty, SortField::Age).comparisons, 0L);
    CHECK_EQ(SortEngine::selectionSort(one, SortField::Age).comparisons, 0L);
    CHECK_EQ(SortEngine::insertionSort(one, SortField::Age).comparisons, 0L);
    CHECK(SortEngine::isSortedBy(empty, SortField::Name));

    CHECK(SortEngine::explain(SortAlgorithm::Insertion, 12, true).find("11 comparisons") != std::string::npos);
    CHECK(SortEngine::explain(SortAlgorithm::Selection, 12, true).find("66 comparisons") != std::string::npos);
    CHECK_EQ(SortEngine::name(SortAlgorithm::Selection), std::string("Selection Sort"));
    CHECK_EQ(SortEngine::fieldName(SortField::Age), std::string("Age"));
}

static void testLinearSearch() {
    Fixture f; f.fillSample();
    PatientList l = f.list();

    auto byId = SearchEngine::linearSearch(l, SearchField::Id, "p-0003");     // case-insensitive
    CHECK_EQ(byId.matches.size(), std::size_t(1));
    CHECK_EQ(byId.matches[0]->name(), std::string("Chi Eze"));
    CHECK_EQ(byId.comparisons, 3L);                                            // stopped at the 3rd patient
    CHECK(byId.algorithm == SearchAlgorithm::Linear);

    auto miss = SearchEngine::linearSearch(l, SearchField::Id, "P-9999");
    CHECK(miss.matches.empty());
    CHECK_EQ(miss.comparisons, 12L);                                           // had to look at everyone

    auto byName = SearchEngine::linearSearch(l, SearchField::Name, "  ADA ");  // any case, trimmed
    CHECK_EQ(byName.matches.size(), std::size_t(2));                           // Ada Obi and ADA Obi
    CHECK_EQ(byName.matches[0]->id(), std::string("P-0001"));
    CHECK_EQ(byName.matches[1]->id(), std::string("P-0011"));
    CHECK_EQ(SearchEngine::linearSearch(l, SearchField::Name, "ife a").matches.size(), std::size_t(1));  // spaces collapsed

    // a name is matched anywhere, so the surname or a single letter can be typed
    CHECK_EQ(SearchEngine::linearSearch(l, SearchField::Name, "obi").matches.size(), std::size_t(2));    // Ada Obi, ADA Obi
    CHECK_EQ(SearchEngine::linearSearch(l, SearchField::Name, "EZE").matches.size(), std::size_t(1));    // Chi Eze
    CHECK_EQ(SearchEngine::linearSearch(l, SearchField::Name, "ade").matches.size(), std::size_t(3));    // adebayo Ade, Bayo Ade, Funmi Alade
    CHECK_EQ(SearchEngine::linearSearch(l, SearchField::Name, "z").matches.size(), std::size_t(2));      // Chi Eze, Zainab Yusuf
    CHECK_EQ(SearchEngine::linearSearch(l, SearchField::Name, "o").matches.size(), std::size_t(7));
    CHECK_EQ(SearchEngine::linearSearch(l, SearchField::Name, "yusuf").matches[0]->id(), std::string("P-0012"));
    CHECK(SearchEngine::linearSearch(l, SearchField::Name, "nobody").matches.empty());
    CHECK_EQ(SearchEngine::linearSearch(l, SearchField::Name, "o").comparisons, 12L);   // a name search checks every patient

    // an ID can be typed in short forms
    for (const char* key : {"3", "p3", "P-3", "0003", "p-0003", " P-0003 "})
        CHECK_EQ(SearchEngine::linearSearch(l, SearchField::Id, key).matches.size(), std::size_t(1));
    CHECK(SearchEngine::linearSearch(l, SearchField::Id, "99").matches.empty());
    CHECK(SearchEngine::linearSearch(l, SearchField::Id, "0").matches.empty());

    CHECK_EQ(SearchEngine::linearSearch(l, SearchField::Category, "emergency").matches.size(), std::size_t(2));
    CHECK_EQ(SearchEngine::linearSearch(l, SearchField::Category, "Follow-Up").matches.size(), std::size_t(2));
    CHECK_EQ(SearchEngine::linearSearch(l, SearchField::Category, "GENERAL").matches.size(), std::size_t(8));

    CHECK_THROWS(SearchEngine::linearSearch(l, SearchField::Name, "   "), InvalidSearch);
    CHECK_THROWS(SearchEngine::linearSearch(l, SearchField::Id, ""), InvalidSearch);
    CHECK_THROWS(SearchEngine::linearSearch(l, SearchField::Category, "surgery"), InvalidSearch);

    PatientList empty;
    CHECK(SearchEngine::linearSearch(empty, SearchField::Name, "a").matches.empty());
}

static void testRecursiveSearch() {
    Fixture f; f.fillSample();
    PatientList l = f.list();
    for (const char* key : {"ada", "a", "z", "nobody", "ife a", "D"}) {
        auto lin = SearchEngine::linearSearch(l, SearchField::Name, key);
        auto rec = SearchEngine::recursiveSearch(l, SearchField::Name, key);
        CHECK(ids(PatientList(rec.matches.begin(), rec.matches.end())) == ids(PatientList(lin.matches.begin(), lin.matches.end())));
        CHECK_EQ(rec.comparisons, 12L);                       // checks every patient once
        CHECK(rec.algorithm == SearchAlgorithm::Recursive);
    }
    auto cat = SearchEngine::recursiveSearch(l, SearchField::Category, "emergency");
    CHECK_EQ(cat.matches.size(), std::size_t(2));
    CHECK_EQ(SearchEngine::recursiveSearch(l, SearchField::Id, "P-0007").matches.size(), std::size_t(1));
    PatientList empty, one{l.front()};
    CHECK(SearchEngine::recursiveSearch(empty, SearchField::Name, "a").matches.empty());
    CHECK_EQ(SearchEngine::recursiveSearch(one, SearchField::Name, "ada").matches.size(), std::size_t(1));
    CHECK_THROWS(SearchEngine::recursiveSearch(l, SearchField::Name, " "), InvalidSearch);
}

static void testBinarySearch() {
    Fixture f; f.fillSample();
    PatientList l = f.list();

    // only an exact key (the patient ID) can use Binary Search
    CHECK_THROWS(SearchEngine::binarySearch(l, SearchField::Name, "ada", SortField::Name), InvalidSearch);
    CHECK_THROWS(SearchEngine::binarySearch(l, SearchField::Category, "emergency", SortField::Name), InvalidSearch);
    CHECK_THROWS(SearchEngine::binarySearch(l, SearchField::Id, "P-0001", std::nullopt), InvalidSearch);   // not sorted
    CHECK_THROWS(SearchEngine::binarySearch(l, SearchField::Id, "P-0001", SortField::Name), InvalidSearch);  // sorted by something else

    PatientList byId = f.list();
    SortEngine::selectionSort(byId, SortField::Id);
    for (int i = 1; i <= 12; ++i) {
        char buf[16]; snprintf(buf, sizeof buf, "p-%04d", i);
        auto r = SearchEngine::binarySearch(byId, SearchField::Id, buf, SortField::Id);
        CHECK_EQ(r.matches.size(), std::size_t(1));
        CHECK_EQ(v::toLower(r.matches[0]->id()), std::string(buf));
        CHECK(r.algorithm == SearchAlgorithm::Binary);
    }
    for (const char* key : {"3", "p3", "P-3", "0003"})                       // short forms of an ID work here too
        CHECK_EQ(SearchEngine::binarySearch(byId, SearchField::Id, key, SortField::Id).matches[0]->id(), std::string("P-0003"));
    CHECK(SearchEngine::binarySearch(byId, SearchField::Id, "P-0099", SortField::Id).matches.empty());
    CHECK(SearchEngine::binarySearch(byId, SearchField::Id, "P-0000", SortField::Id).matches.empty());
    CHECK_THROWS(SearchEngine::binarySearch(byId, SearchField::Id, "  ", SortField::Id), InvalidSearch);

    PatientList empty;
    CHECK(SearchEngine::binarySearch(empty, SearchField::Id, "P-1", SortField::Id).matches.empty());
}

static void testBinaryIsFasterOnLargeLists() {
    Fixture f;
    for (int i = 0; i < 200; ++i) f.add("Patient " + std::to_string(i), 20 + i % 50);
    PatientList l = f.list();
    SortEngine::insertionSort(l, SortField::Id);
    auto bin = SearchEngine::binarySearch(l, SearchField::Id, "P-0150", SortField::Id);
    auto lin = SearchEngine::linearSearch(l, SearchField::Id, "P-0150");
    CHECK_EQ(bin.matches.size(), std::size_t(1));
    CHECK_EQ(lin.matches.size(), std::size_t(1));
    CHECK_EQ(lin.comparisons, 150L);                             // linear search walks to position 150
    CHECK(bin.comparisons <= 8);                                 // about log2(200)
    auto miss = SearchEngine::binarySearch(l, SearchField::Id, "P-0999", SortField::Id);
    auto linMiss = SearchEngine::linearSearch(l, SearchField::Id, "P-0999");
    CHECK(miss.matches.empty() && miss.comparisons <= 8);
    CHECK_EQ(linMiss.comparisons, 200L);                         // a miss makes linear search check everyone
}

static void testAlgorithmChoice() {
    using A = SearchAlgorithm; using F = SearchField;
    CHECK(SearchEngine::isAlgorithmValid(A::Linear, F::Name, std::nullopt));
    CHECK(SearchEngine::isAlgorithmValid(A::Recursive, F::Category, std::nullopt));
    CHECK(!SearchEngine::isAlgorithmValid(A::Binary, F::Name, std::nullopt));
    CHECK(!SearchEngine::isAlgorithmValid(A::Binary, F::Name, SortField::Name));   // a name is matched in part
    CHECK(!SearchEngine::isAlgorithmValid(A::Binary, F::Id, std::nullopt));
    CHECK(!SearchEngine::isAlgorithmValid(A::Binary, F::Id, SortField::Name));
    CHECK(SearchEngine::isAlgorithmValid(A::Binary, F::Id, SortField::Id));
    CHECK(!SearchEngine::isAlgorithmValid(A::Binary, F::Category, SortField::Id));

    // the engine picks a technique that fits the state of the data
    CHECK(SearchEngine::suggest(F::Id, std::nullopt) == A::Linear);
    CHECK(SearchEngine::suggest(F::Id, SortField::Age) == A::Linear);
    CHECK(SearchEngine::suggest(F::Id, SortField::Id) == A::Binary);
    CHECK(SearchEngine::suggest(F::Name, std::nullopt) == A::Recursive);
    CHECK(SearchEngine::suggest(F::Name, SortField::Name) == A::Recursive);
    CHECK(SearchEngine::suggest(F::Category, SortField::Id) == A::Linear);
    CHECK(!SearchEngine::whySuggested(F::Name, SortField::Name).empty());
    CHECK(!SearchEngine::whySuggested(F::Id, SortField::Id).empty());

    Fixture f; f.fillSample();
    PatientList l = f.list();
    CHECK_THROWS(SearchEngine::search(l, F::Name, "ada", A::Binary, SortField::Name), InvalidSearch);
    CHECK_THROWS(SearchEngine::search(l, F::Id, "P-0001", A::Binary, std::nullopt), InvalidSearch);
    CHECK_EQ(SearchEngine::search(l, F::Name, "ada", A::Recursive, std::nullopt).matches.size(), std::size_t(2));
    CHECK_EQ(SearchEngine::name(A::Binary), std::string("Binary Search"));
}

static void testCache() {
    Fixture f; f.fillSample();
    PatientCache cache;
    CHECK_EQ(cache.count(), 0);
    const auto& p1 = *f.db.patients()[0];
    cache.recordAccess(p1);
    cache.recordAccess(p1);
    cache.recordAccess(p1);
    CHECK_EQ(cache.hitsFor("P-0001"), 3);
    CHECK_EQ(cache.count(), 1);                                  // same patient reuses its slot

    for (int i = 1; i < 10; ++i) cache.recordAccess(*f.db.patients()[static_cast<std::size_t>(i)]);   // fills all 10 slots
    CHECK_EQ(cache.count(), 10);
    cache.recordAccess(*f.db.patients()[1]);                     // P-0002 now has 2 hits
    cache.recordAccess(*f.db.patients()[2]);                     // P-0003 now has 2 hits

    // 11th distinct patient: replaces a patient with the fewest hits (1 hit), never P-0001 / P-0002 / P-0003
    cache.recordAccess(*f.db.patients()[10]);                    // P-0011
    CHECK_EQ(cache.count(), 10);
    CHECK(cache.contains("P-0011"));
    CHECK(cache.contains("P-0001") && cache.contains("P-0002") && cache.contains("P-0003"));
    CHECK(!cache.contains("P-0004"));                            // first one with the lowest hits was replaced

    auto top = cache.mostAccessed();
    CHECK_EQ(top.size(), std::size_t(10));
    CHECK_EQ(top[0].patient->id(), std::string("P-0001"));       // 3 hits
    CHECK_EQ(top[0].hits, 3);
    CHECK(top[1].hits == 2 && top[2].hits == 2);
    for (std::size_t i = 1; i < top.size(); ++i) CHECK(top[i - 1].hits >= top[i].hits);

    cache.invalidate("P-0001");
    CHECK(!cache.contains("P-0001"));
    CHECK_EQ(cache.count(), 9);
    cache.invalidate("P-9999");                                   // unknown ID: nothing happens
    cache.clear();
    CHECK_EQ(cache.count(), 0);
    CHECK(cache.mostAccessed().empty());
}

static void testCacheStaysSafeWhenPatientsChange() {
    Fixture f; f.fillSample();
    PatientCache cache;
    f.db.setInvalidationCallback([&](const std::string& id) { cache.invalidate(id); });
    cache.recordAccess(f.mgr.getPatient("P-0003"));
    cache.recordAccess(f.mgr.getPatient("P-0005"));
    CHECK_EQ(cache.count(), 2);

    PatientInput edit; edit.name = "Dayo Bello Jr"; edit.age = 68; edit.phone = "08035550101";
    f.mgr.editPatient("P-0005", edit);                            // the object is replaced -> pointer would dangle
    CHECK(!cache.contains("P-0005"));
    f.mgr.deletePatient("P-0003");                                // deleted -> forgotten
    CHECK(!cache.contains("P-0003"));
    CHECK_EQ(cache.count(), 0);
    CHECK(cache.mostAccessed().empty());                          // no dangling pointers left
}

static void testPatientsVersion() {
    Fixture f;
    const auto v0 = f.db.patientsVersion();
    f.add("Ada Obi", 30);
    const auto v1 = f.db.patientsVersion();
    CHECK(v1 > v0);
    PatientInput edit; edit.name = "Ada Obi-N"; edit.age = 31; edit.phone = "08035550101";
    f.mgr.editPatient("P-0001", edit);
    const auto v2 = f.db.patientsVersion();
    CHECK(v2 > v1);
    f.mgr.deletePatient("P-0001");
    const auto v3 = f.db.patientsVersion();
    CHECK(v3 > v2);
    f.undo.undoLast();                                            // undo delete: patient comes back
    CHECK(f.db.patientsVersion() > v3);
    CHECK_EQ(f.mgr.patientsVersion(), f.db.patientsVersion());
}

void runSearchSortTests() {
    testStringHelpers();
    testSortsMatchReference();
    testSortStats();
    testLinearSearch();
    testRecursiveSearch();
    testBinarySearch();
    testBinaryIsFasterOnLargeLists();
    testAlgorithmChoice();
    testCache();
    testCacheStaysSafeWhenPatientsChange();
    testPatientsVersion();
}
