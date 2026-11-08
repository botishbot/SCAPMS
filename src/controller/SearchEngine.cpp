#include "controller/SearchEngine.h"

#include "utils/ClinicException.h"
#include "utils/Validation.h"

namespace scapms::controller {

using model::PatientCategory;
using model::PatientRecord;
using utils::ClinicException;
using utils::ErrorCode;
namespace v = utils::validation;

namespace {

// A validated, normalised search request.
struct Query {
    SearchField field;
    std::string key;                                  // lower-case ID or normalised name prefix
    PatientCategory category = PatientCategory::General;
};

PatientCategory parseCategory(const std::string& text) {
    const std::string t = v::toLower(v::collapseSpaces(text));
    if (t == "general") return PatientCategory::General;
    if (t == "emergency") return PatientCategory::Emergency;
    if (t == "follow-up" || t == "followup" || t == "follow up") return PatientCategory::FollowUp;
    throw ClinicException(ErrorCode::InvalidSearch,
                          "Unknown category \"" + text + "\". Choose General, Emergency or Follow-Up.");
}

Query makeQuery(SearchField field, const std::string& keyword) {
    Query q;
    q.field = field;
    if (field == SearchField::Category) {
        q.category = parseCategory(keyword);
        return q;
    }
    if (v::trim(keyword).empty())
        throw ClinicException(ErrorCode::InvalidSearch, "Enter a search keyword first.");
    q.key = (field == SearchField::Id) ? v::normalizedIdKey(keyword) : v::normalizedName(keyword);
    return q;
}

// Does this one patient match? (callers count it as one comparison)
bool matches(const PatientRecord& p, const Query& q) {
    switch (q.field) {
        case SearchField::Id: return v::toLower(p.id()) == q.key;
        case SearchField::Name: return v::contains(v::normalizedName(p.name()), q.key);
        case SearchField::Category: return p.category() == q.category;
    }
    return false;
}

// Recursive helper: look at the range [lo, hi). One patient -> check it. More -> split in half and
// search the left half, then the right half (so matches stay in list order).
void searchRange(const PatientList& list, std::size_t lo, std::size_t hi, const Query& q, SearchResult& out) {
    if (lo >= hi) return;                        // base case: empty range
    if (hi - lo == 1) {                          // base case: a single patient
        ++out.comparisons;
        if (matches(*list[lo], q)) out.matches.push_back(list[lo]);
        return;
    }
    const std::size_t mid = lo + (hi - lo) / 2;
    searchRange(list, lo, mid, q, out);
    searchRange(list, mid, hi, q, out);
}

} // namespace

bool SearchEngine::isAlgorithmValid(SearchAlgorithm algorithm, SearchField field, std::optional<SortField> sortedBy) {
    if (algorithm != SearchAlgorithm::Binary) return true;
    if (!sortedBy) return false;
    return field == SearchField::Id && *sortedBy == SortField::Id;  // only an exact key such as the patient ID can be halved
}

SearchAlgorithm SearchEngine::suggest(SearchField field, std::optional<SortField> sortedBy) {
    if (isAlgorithmValid(SearchAlgorithm::Binary, field, sortedBy)) return SearchAlgorithm::Binary;
    return field == SearchField::Name ? SearchAlgorithm::Recursive : SearchAlgorithm::Linear;
}

std::string SearchEngine::whySuggested(SearchField field, std::optional<SortField> sortedBy) {
    switch (suggest(field, sortedBy)) {
        case SearchAlgorithm::Binary:
            return "The list is sorted by Patient ID, so Binary Search can halve the search area at every step.";
        case SearchAlgorithm::Recursive:
            return "A name can be typed in part, so every patient has to be checked. Recursive Search does this by "
                   "splitting the list in halves. Binary Search is only possible for an exact key such as the Patient ID.";
        case SearchAlgorithm::Linear:
            break;
    }
    if (field == SearchField::Category)
        return "Category matches many patients and Binary Search does not apply, so Linear Search checks them one by one.";
    return "The list is not sorted by ID, so Linear Search checks the patients one by one. "
           "Sort by Patient ID first to enable Binary Search.";
}

SearchResult SearchEngine::linearSearch(const PatientList& list, SearchField field, const std::string& keyword) {
    const Query q = makeQuery(field, keyword);
    SearchResult result;
    result.algorithm = SearchAlgorithm::Linear;
    for (const PatientRecord* p : list) {
        ++result.comparisons;
        if (matches(*p, q)) {
            result.matches.push_back(p);
            if (field == SearchField::Id) break;  // IDs are unique: stop at the first hit
        }
    }
    return result;
}

SearchResult SearchEngine::recursiveSearch(const PatientList& list, SearchField field, const std::string& keyword) {
    const Query q = makeQuery(field, keyword);
    SearchResult result;
    result.algorithm = SearchAlgorithm::Recursive;
    searchRange(list, 0, list.size(), q, result);
    return result;
}

SearchResult SearchEngine::binarySearch(const PatientList& list, SearchField field, const std::string& keyword,
                                        std::optional<SortField> sortedBy) {
    if (field != SearchField::Id)
        throw ClinicException(ErrorCode::InvalidSearch,
                              "Binary Search can only be used to search by Patient ID (an exact match). "
                              "Names and categories are matched in part, so they use Linear or Recursive Search.");
    if (!isAlgorithmValid(SearchAlgorithm::Binary, field, sortedBy))
        throw ClinicException(ErrorCode::InvalidSearch,
                              "Binary Search needs the list sorted by Patient ID first. Use the Sort section, then search again.");
    const Query q = makeQuery(field, keyword);
    SearchResult result;
    result.algorithm = SearchAlgorithm::Binary;

    // exact match: halve the range until the ID is found or the range is empty
    std::size_t lo = 0, hi = list.size();
    while (lo < hi) {
        const std::size_t mid = lo + (hi - lo) / 2;
        ++result.comparisons;
        const int c = q.key.compare(v::toLower(list[mid]->id()));
        if (c == 0) { result.matches.push_back(list[mid]); break; }
        if (c < 0) hi = mid; else lo = mid + 1;
    }
    return result;
}

SearchResult SearchEngine::search(const PatientList& list, SearchField field, const std::string& keyword,
                                  SearchAlgorithm algorithm, std::optional<SortField> sortedBy) {
    switch (algorithm) {
        case SearchAlgorithm::Linear: return linearSearch(list, field, keyword);
        case SearchAlgorithm::Recursive: return recursiveSearch(list, field, keyword);
        case SearchAlgorithm::Binary: return binarySearch(list, field, keyword, sortedBy);
    }
    throw ClinicException(ErrorCode::InvalidSearch, "Unknown search algorithm.");
}

std::string SearchEngine::name(SearchAlgorithm a) {
    switch (a) {
        case SearchAlgorithm::Linear: return "Linear Search";
        case SearchAlgorithm::Binary: return "Binary Search";
        case SearchAlgorithm::Recursive: return "Recursive Search";
    }
    return "";
}

std::string SearchEngine::fieldName(SearchField f) {
    switch (f) {
        case SearchField::Id: return "Patient ID";
        case SearchField::Name: return "Patient name";
        case SearchField::Category: return "Category";
    }
    return "";
}

} // namespace scapms::controller
