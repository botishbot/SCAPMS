#pragma once
#include "model/PatientRecord.h"

#include <string>
#include <vector>

namespace scapms::controller {

// A working list of patients (non-owning). The database keeps patients in registration order;
// sorting reorders this list only.
using PatientList = std::vector<const model::PatientRecord*>;

enum class SortAlgorithm { Selection, Insertion };
enum class SortField { Name, Id, Age };

struct SortStats {
    long comparisons = 0;  // how many times two patients were compared
    long moves = 0;        // Selection Sort: swaps. Insertion Sort: places an element was shifted
};

// Hand-written sorting algorithms (no std::sort). Both sort ascending by the chosen field and
// break ties with the patient ID, so the order is always fully determined and both algorithms
// give exactly the same result.
class SortEngine {
public:
    static SortStats sort(PatientList& list, SortAlgorithm algorithm, SortField field);
    static SortStats selectionSort(PatientList& list, SortField field);
    static SortStats insertionSort(PatientList& list, SortField field);

    static bool isSortedBy(const PatientList& list, SortField field);

    // True if a goes before b. Counts one comparison.
    static bool lessThan(const model::PatientRecord& a, const model::PatientRecord& b, SortField field,
                         long& comparisons);

    static std::string name(SortAlgorithm algorithm);
    static std::string fieldName(SortField field);

    // Plain-language reason shown in the GUI: why this algorithm suits this situation.
    static std::string explain(SortAlgorithm algorithm, std::size_t patientCount, bool alreadySortedByField);
};

} // namespace scapms::controller
