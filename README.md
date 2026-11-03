# SCAPMS - Smart Clinic Appointment & Patient Management System

COS202 Programming II lab assessment project. C++17, Qt 6 Widgets, CMake.

## Build and run (Ubuntu)

Run these commands inside this folder:

    sudo apt install cmake qt6-base-dev build-essential
    cmake -S . -B build
    cmake --build build -j
    ./build/SCAPMS

To run the unit tests:

    ctest --test-dir build --output-on-failure

On first use, open Administration and choose "Load demo data" to fill the system with fictional patients,
services and appointments. `data/sample_data.json` is an example data file that can be opened with File > Load.

## Project layout

    include/, src/
      model/        PatientRecord (abstract), GeneralPatient, EmergencyPatient, FollowUpPatient,
                    Appointable (interface), Appointment, ClinicService, ClinicDatabase
      controller/   ClinicManager, AppointmentController, QueueManager, UndoManager, UndoCommands,
                    SortEngine, SearchEngine, PatientCache, ReportGenerator, DemoData
      utils/        ClinicException, Validation, IDGenerator, FileHandler
      gui/          MainWindow, DashboardPage, PatientPage, AppointmentPage, QueuePage, SearchPage,
                    ReportPage, AdminPage, DataCoordinator, ServiceDialog
    resources/      style.qss, resources.qrc
    data/           sample_data.json
    tests/          unit tests for the model, controller and utils classes
    docs/           ALGORITHMS.md (explanation of the sorting, searching and cache algorithms)

Namespaces: `scapms::model`, `scapms::controller`, `scapms::utils`, `scapms::gui`.

## Data file

The data is saved as JSON (schemaVersion 1). It contains the patients with their appointments, the services,
and the appointments of deleted patients (history). The waiting queue and the undo history are not saved.
When a file is loaded, the whole file is checked first. If anything is wrong (invalid JSON, duplicate patient ID,
age outside 0 to 120, unknown service, and so on) an error message is shown and the current data is left unchanged.
