#include "test_support.h"

#include <QCoreApplication>

void runModelTests();
void runAppointmentTests();
void runQueueUndoTests();
void runSearchSortTests();
void runFileReportTests();

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName("COS202");
    QCoreApplication::setApplicationName("SCAPMS");
    runModelTests();
    runAppointmentTests();
    runQueueUndoTests();
    runSearchSortTests();
    runFileReportTests();
    std::cout << testing::passed() << " checks passed, " << testing::failed() << " failed\n";
    return testing::failed() == 0 ? 0 : 1;
}
