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
//   Id        exact match, case-insensitive
//   Name      case-insensitive match on the start of the name ("ad" finds "Ada Obi" and "Adebayo Ade")
//   Category  General / Emergency / Follow-Up
//
//   Linear     any field, any list order
//   Recursive  any field, any list order (divide and conquer)
//   Binary     Id or Name only, and only if the list is currently sorted by that field
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
