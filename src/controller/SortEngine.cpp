#include "controller/SortEngine.h"

#include "utils/Validation.h"

namespace scapms::controller {

using model::PatientRecord;

namespace {

// <0 if a sorts before b, 0 if equal, >0 if after (on the chosen field only).
int compareField(const PatientRecord& a, const PatientRecord& b, SortField field) {
    switch (field) {
        case SortField::Name: {
            const std::string x = utils::validation::normalizedName(a.name());
            const std::string y = utils::validation::normalizedName(b.name());
            return x.compare(y);
        }
        case SortField::Id:
            return a.id().compare(b.id());
        case SortField::Age:
            if (a.age() < b.age()) return -1;
            if (a.age() > b.age()) return 1;
            return 0;
    }
    return 0;
}

} // namespace

bool SortEngine::lessThan(const PatientRecord& a, const PatientRecord& b, SortField field, long& comparisons) {
    ++comparisons;
    const int c = compareField(a, b, field);
    if (c != 0) return c < 0;
    return a.id() < b.id();  // tie-break: IDs are unique, so the order is total
}

// Selection Sort: repeatedly find the smallest remaining patient and swap it to the front of the
// unsorted part. At most one swap per position, always about n*n/2 comparisons.
SortStats SortEngine::selectionSort(PatientList& list, SortField field) {
    SortStats stats;
    const std::size_t n = list.size();
    for (std::size_t i = 0; i + 1 < n; ++i) {
        std::size_t smallest = i;
        for (std::size_t j = i + 1; j < n; ++j)
            if (lessThan(*list[j], *list[smallest], field, stats.comparisons)) smallest = j;
        if (smallest != i) {
            const PatientRecord* tmp = list[i];
            list[i] = list[smallest];
            list[smallest] = tmp;
            ++stats.moves;
        }
    }
    return stats;
}

// Insertion Sort: take the next patient and slide it left past every larger patient until it
// sits in the right place in the already-sorted part. Fast when the list is nearly sorted.
SortStats SortEngine::insertionSort(PatientList& list, SortField field) {
    SortStats stats;
    for (std::size_t i = 1; i < list.size(); ++i) {
        const PatientRecord* current = list[i];
        std::size_t j = i;
        while (j > 0 && lessThan(*current, *list[j - 1], field, stats.comparisons)) {
            list[j] = list[j - 1];  // shift the larger patient one place right
            ++stats.moves;
            --j;
        }
        list[j] = current;
    }
    return stats;
}

SortStats SortEngine::sort(PatientList& list, SortAlgorithm algorithm, SortField field) {
    return algorithm == SortAlgorithm::Selection ? selectionSort(list, field) : insertionSort(list, field);
}

bool SortEngine::isSortedBy(const PatientList& list, SortField field) {
    long ignored = 0;
    for (std::size_t i = 1; i < list.size(); ++i)
        if (lessThan(*list[i], *list[i - 1], field, ignored)) return false;
    return true;
}

std::string SortEngine::name(SortAlgorithm a) {
    return a == SortAlgorithm::Selection ? "Selection Sort" : "Insertion Sort";
}

std::string SortEngine::fieldName(SortField f) {
    switch (f) {
        case SortField::Name: return "Patient name";
        case SortField::Id: return "Patient ID";
        case SortField::Age: return "Age";
    }
    return "";
}

std::string SortEngine::explain(SortAlgorithm algorithm, std::size_t n, bool alreadySorted) {
    std::string text;
    if (algorithm == SortAlgorithm::Insertion) {
        text = "Insertion Sort places one patient at a time into the sorted part. It is quick when records are "
               "already nearly in order, which suits a clinic where new patients are added a few at a time.";
        if (alreadySorted)
            text += " The list is already sorted by this field, so it only needs about " +
                    std::to_string(n > 0 ? n - 1 : 0) + " comparisons.";
    } else {
        text = "Selection Sort swaps the smallest remaining patient into place, so it makes at most one swap per "
               "patient. It suits cases where moving a record is costly, and always does about the same number of "
               "comparisons.";
        if (alreadySorted && n > 1)
            text += " Here that is about " + std::to_string(n * (n - 1) / 2) + " comparisons.";
    }
    return text;
}

} // namespace scapms::controller
