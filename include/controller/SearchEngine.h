#pragma once
#include "controller/SortEngine.h"

#include <optional>
#include <string>
#include <vector>

namespace scapms::controller {

enum class SearchField { Id, Name, Category };
enum class SearchAlgorithm { Linear, Binary, Recursive };

struct SearchResult {
    std::vector<const model::PatientRecord*> matches;  // in list order
    long comparisons = 0;                              // how many patients/probes were compared
    SearchAlgorithm algorithm = SearchAlgorithm::Linear;
};

// Hand-written searching algorithms (no std::find / std::binary_search).
//
//   Id        exact match, case-insensitive; "3", "p3" and "P-3" all mean P-0003
//   Name      case-insensitive match anywhere in the name ("eze" finds "Ngozi Eze", "ob" finds "Emeka Obi")
//   Category  General / Emergency / Follow-Up
//
//   Linear     any field, any list order
//   Recursive  any field, any list order (divide and conquer)
//   Binary     Id only (an exact key), and only if the list is currently sorted by ID
class SearchEngine {
public:
    // sortedBy tells the engine what order the list is in right now (empty = unsorted/unknown).
    // Throws ClinicException(InvalidSearch) for a blank keyword, an unknown category,
    // or an algorithm that cannot be used on this field / list state.
    static SearchResult search(const PatientList& list, SearchField field, const std::string& keyword,
                               SearchAlgorithm algorithm, std::optional<SortField> sortedBy);

    static SearchResult linearSearch(const PatientList& list, SearchField field, const std::string& keyword);
    static SearchResult binarySearch(const PatientList& list, SearchField field, const std::string& keyword,
                                     std::optional<SortField> sortedBy);
    static SearchResult recursiveSearch(const PatientList& list, SearchField field, const std::string& keyword);

    // Which algorithms may be used right now, and which one fits the data best.
    static bool isAlgorithmValid(SearchAlgorithm algorithm, SearchField field, std::optional<SortField> sortedBy);
    static SearchAlgorithm suggest(SearchField field, std::optional<SortField> sortedBy);

    static std::string name(SearchAlgorithm algorithm);
    static std::string fieldName(SearchField field);
    static std::string whySuggested(SearchField field, std::optional<SortField> sortedBy);
};

} // namespace scapms::controller
