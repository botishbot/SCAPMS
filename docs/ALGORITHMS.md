# SCAPMS - Algorithms explained

All algorithms below are written by hand in `src/controller/` (`SortEngine.cpp`, `SearchEngine.cpp`,
`PatientCache.cpp`). None of them uses `std::sort`, `std::find`, `std::binary_search` or similar.
(`std::vector`, `std::queue` and `std::stack` are used only to store data.)
`n` means the number of patients.

## Ordering used by both sorts
Patients are compared on the chosen field (name, ID or age). If two patients are equal on that field,
the patient ID breaks the tie. IDs are unique, so there is always exactly one correct order, and both
sorts give the same answer. Names are compared in lower case with extra spaces removed, so "ada  OBI"
and "Ada Obi" are the same name.

## Sorting

### Selection Sort - `SortEngine::selectionSort`
Idea: repeatedly pick the smallest patient that is still unsorted and swap it to the front of the unsorted part.

    for each position i from the start:
        smallest = i
        for each position j after i:
            if patient j goes before patient smallest: smallest = j
        swap patient i and patient smallest

- Comparisons: always n(n-1)/2, even if the list is already sorted.
- Swaps: at most n-1.
- Why it is appropriate: moving records is the expensive part, and Selection Sort moves the fewest.
  It is also the easiest to follow step by step.

### Insertion Sort - `SortEngine::insertionSort`
Idea: keep the left part of the list sorted. Take the next patient and slide it left until it sits in the right place.

    for each patient i from the second one:
        current = patient i
        while the patient before it goes AFTER current:
            shift that patient one place to the right
        put current in the gap

- Already sorted list: only n-1 comparisons and no moves.
- Reverse-sorted list (worst case): n(n-1)/2 comparisons and moves.
- Why it is appropriate: a clinic's list is usually nearly sorted (new patients are added a few at a time),
  which is exactly where Insertion Sort is fastest.

Both are O(n*n). That is fine for hundreds of patients. For a very large database Merge Sort or Quick Sort
(O(n log n)) would be the better choice.

## Searching
Name search matches the START of the name, ignoring case and extra spaces ("ad" finds "Ada Obi").
ID search is an exact match, ignoring case. Category search matches General, Emergency or Follow-Up.

### Linear Search - `SearchEngine::linearSearch`
Look at every patient from the first to the last. Works on any field and any order.
For an ID it stops at the first match (IDs are unique). Comparisons: up to n.

### Binary Search - `SearchEngine::binarySearch`
Only works if the list is sorted by the field being searched (ID or name). Look at the middle patient:
if what you want comes before it, keep only the left half, otherwise the right half. Repeat.
- By ID: stop when the middle patient is the one you want.
- By name prefix: find the FIRST patient whose name is not before the prefix, then step forward while names
  still start with the prefix (all such names sit next to each other in sorted order).
Comparisons: about log2(n) (200 patients need about 8 instead of 200).
The program refuses to run it on an unsorted list.

### Recursive Search - `SearchEngine::recursiveSearch`
Divide and conquer. To search a range of the list:
- empty range: nothing to find (base case)
- one patient: check that patient (base case)
- otherwise: split the range in half and search the left half, then the right half (recursive calls)
Works on any field and any order. It checks every patient once, but the recursion is only about log2(n)
levels deep, so it never runs out of stack space.

### Which search is used?
The Search page offers only the searches that are possible for the current state of the data, and pre-selects
the best one:
| Field | List sorted by that field | List not sorted |
|---|---|---|
| ID | Binary Search | Linear Search |
| Name | Binary Search | Recursive Search |
| Category | Linear Search | Linear Search |

## Frequently-accessed cache - `PatientCache`
A fixed array of 10 slots. Opening a patient's record adds one "hit" to their slot. If all slots are full,
the patient with the fewest hits is replaced. When a patient is deleted or replaced, the database tells the
cache first so no slot ever points at a patient that no longer exists.
