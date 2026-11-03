#include "test_support.h"

void runModelTests();
void runAppointmentTests();
void runQueueUndoTests();
void runSearchSortTests();
void runFileReportTests();

int main() {
    runModelTests();
    runAppointmentTests();
    runQueueUndoTests();
    runSearchSortTests();
    runFileReportTests();
    std::cout << testing::passed() << " checks passed, " << testing::failed() << " failed\n";
    return testing::failed() == 0 ? 0 : 1;
}
