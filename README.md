# SCAPMS — Smart Clinic Appointment & Patient Management System

A cross-platform desktop application built with **C++17, Qt 6 Widgets and CMake**. SCAPMS brings patient records, appointment scheduling, a first-in/first-out waiting queue, searching, sorting and clinic reports into one desktop workspace.

**Personal software development project by George Oluwasemilore** — developed to strengthen and demonstrate practical C++, object-oriented programming, data structures, algorithms, GUI development, persistence and testing skills.

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-17-00599C?style=for-the-badge&logo=cplusplus&logoColor=white" alt="C++17">
  <img src="https://img.shields.io/badge/Qt-6-41CD52?style=for-the-badge&logo=qt&logoColor=white" alt="Qt 6">
  <img src="https://img.shields.io/badge/CMake-Build-064F8C?style=for-the-badge&logo=cmake&logoColor=white" alt="CMake">
  <img src="https://img.shields.io/badge/Windows-Linux-blue?style=for-the-badge" alt="Windows and Linux">
</p>

---

## Contents

- [Overview](#overview)
- [Application screenshots](#application-screenshots)
- [Features](#features)
- [Technology](#technology)
- [Architecture and OOP](#architecture-and-object-oriented-design)
- [Data structures and algorithms](#data-structures-and-algorithms)
- [Data storage](#data-storage)
- [Download a release](#download-a-release)
- [Build from source](#build-from-source)
- [Run tests](#run-tests)
- [Project structure](#project-structure)
- [Project notes](#project-notes)
- [Author](#author)

## Overview

SCAPMS is an educational clinic-management application. It provides a graphical environment for maintaining patient records, scheduling appointments, managing a consultation queue and reviewing operational summaries.

The project applies object-oriented programming and separates domain models, application controllers, GUI components and utilities. It also includes manually implemented searching and sorting algorithms, JSON persistence, CSV export, undo support and automated tests.

> **Scope note:** SCAPMS is a learning and portfolio project. It is not a clinically validated electronic medical-record system and must not be used to manage real patient information.

## Application screenshots

The screenshots below show the running Qt application using fictional demonstration data.

| Dashboard | Patient management |
|---|---|
| ![SCAPMS dashboard](screenshots/dashboard.png) | ![Patient management](screenshots/patients.png) |
| **Appointment management** | **Waiting queue** |
| ![Appointment management](screenshots/appointments.png) | ![Waiting queue](screenshots/waiting_queue.png) |
| **Search and sorting** | **Reports** |
| ![Search and sorting](screenshots/search_and_sort.png) | ![Reports](screenshots/reports.png) |
| **Administration** | **Clinic services dialog** |
| ![Administration](screenshots/admin.png) | ![Manage clinic services](screenshots/services.png) |

## Features

- **Patient records:** register and manage General, Emergency and Follow-Up patients.
- **Appointments:** schedule appointments, associate them with patient records and manage appointment states.
- **Waiting queue:** maintain consultation order using FIFO behaviour.
- **Search:** use linear, binary or recursive search where supported by the selected field and current data order.
- **Sorting:** sort patient records by name, ID or age using Selection Sort or Insertion Sort.
- **Dashboard:** view patient, appointment and waiting-queue summaries.
- **Reports:** review frequently attended patients, pending appointments, category distribution and daily appointment summaries.
- **Data management:** save and load JSON data, export supported data to CSV, load fictional demo data and manage clinic services.
- **Undo:** reverse supported operations.

## Technology

| Technology | Use |
|---|---|
| C++17 | Application logic and object-oriented implementation |
| Qt 6 Widgets | Desktop interface, layouts, dialogs and event handling |
| Qt Core | Application and data/file support |
| CMake | Cross-platform configuration and build |
| JSON | Persistent application data |
| CSV | Data export |
| C++ Standard Library | Containers and supporting utilities |
| CTest | Automated test execution |
| GitHub Actions | Windows and Linux build and release workflows |

## Architecture and object-oriented design

The application is organized into four principal namespaces:

- `scapms::model` — patient, appointment, service and clinic database domain objects.
- `scapms::controller` — application operations, queue and undo management, searching, sorting and report generation.
- `scapms::gui` — Qt windows, pages, dialogs and user interaction.
- `scapms::utils` — validation, identifiers, file handling and application exceptions.

### Patient hierarchy

`PatientRecord` is an abstract base class. The concrete patient types are:

- `GeneralPatient`
- `EmergencyPatient`
- `FollowUpPatient`

The hierarchy demonstrates abstraction, inheritance and runtime polymorphism. The `Appointable` abstraction represents appointment-related behaviour.

### Main components

- `Appointment` and `ClinicDatabase` represent appointment and clinic data.
- `QueueManager` maintains the consultation queue.
- `UndoManager` coordinates supported undo operations.
- `SearchEngine` and `SortEngine` implement the searching and sorting methods.
- `ReportGenerator` prepares clinic summaries.
- Qt signals and slots connect GUI events to application operations.

## Data structures and algorithms

| Structure or algorithm | Application |
|---|---|
| `std::vector` | Variable-sized patient collection and working lists |
| `std::queue` | First-in, first-out waiting queue |
| `std::stack` | Last-in, first-out undo history |
| Fixed-size array | Frequently accessed patient cache |
| Linear Search | Search records without requiring sorted data |
| Binary Search | Search supported fields when the list is correctly sorted |
| Recursive Search | Search by recursively dividing the search range |
| Selection Sort | Sort by repeatedly selecting the next record |
| Insertion Sort | Sort by inserting each record into the sorted portion |

The searching and sorting methods are implemented in the project rather than delegated to standard-library sorting or searching algorithms. The detailed explanation, pseudocode and complexity notes are in **[docs/ALGORITHMS.md](docs/ALGORITHMS.md)**.

## Data storage

Application data is stored as JSON (schema version 1) in the current user's application-data directory, separate from the executable and source tree. Users can choose another location through **Save As**. The application also supports CSV export.

The waiting queue and undo history are runtime structures and are not persisted in the JSON data file. Use fictional or test data only.

## Download a release

**[Download SCAPMS v1.0.0](https://github.com/botishbot/SCAPMS/releases/tag/v1.0.0)**

| Platform | Release package | Run |
|---|---|---|
| Windows 64-bit | [SCAPMS-Windows-x64.zip](https://github.com/botishbot/SCAPMS/releases/download/v1.0.0/SCAPMS-Windows-x64.zip) | Extract the archive and run `SCAPMS.exe`. Keep the bundled DLLs and Qt plugin folders beside it. |
| Linux x86_64 | [SCAPMS-Linux-x86_64.AppImage](https://github.com/botishbot/SCAPMS/releases/download/v1.0.0/SCAPMS-Linux-x86_64.AppImage) | Make the AppImage executable and launch it. |

Linux:

```bash
chmod +x SCAPMS-Linux-x86_64.AppImage
./SCAPMS-Linux-x86_64.AppImage
```

## Build from source

### Requirements

- C++17-compatible compiler
- CMake 3.16 or newer
- Qt 6 with Core and Widgets
- Git

### Linux (Ubuntu/Debian)

Install dependencies:

```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev
```

Clone and build:

```bash
git clone https://github.com/botishbot/SCAPMS.git
cd SCAPMS
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/SCAPMS
```

### Windows

Install Visual Studio 2022 with the **Desktop development with C++** workload, CMake and a compatible Qt 6 kit. From a developer terminal:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/msvc2022_64"
cmake --build build --config Release --parallel
.\build\Release\SCAPMS.exe
```

Replace the example Qt path with the actual installation path. The Qt kit and compiler must use compatible toolchains.

## Run tests

After configuring and building:

```bash
ctest --test-dir build --output-on-failure
```

For a multi-configuration Windows build:

```powershell
cmake --build build --config Release --target scapms_tests
ctest --test-dir build -C Release --output-on-failure
```

The tagged v1.0.0 GitHub Actions workflow completed successfully for both Windows x64 and Linux AppImage build/package jobs. The test command above lets contributors run the test suite in their own build environment.

## Project structure

```text
SCAPMS/
├── .github/workflows/   # Automated build and release workflow
├── data/                # Example JSON data
├── docs/                # Technical documentation
├── include/             # Application headers
│   ├── controller/
│   ├── gui/
│   ├── model/
│   └── utils/
├── resources/           # Qt resources and stylesheet
├── src/                 # Application implementation
│   ├── controller/
│   ├── gui/
│   ├── model/
│   └── utils/
├── tests/               # Automated tests
├── CMakeLists.txt
└── README.md
```

## Project documentation

- [Three-page project report](docs/SCAPMS_Project_Report.pdf)
- [UML class and architecture diagrams (PDF)](docs/SCAPMS_UML_Class_Diagrams.pdf)
- [Algorithm explanations and complexity notes](docs/ALGORITHMS.md)

## Project notes

- This repository is intended to demonstrate software development and programming concepts.
- Demonstration data is fictional. Do not commit real patient or sensitive personal information.
- No open-source licence is currently specified in the repository. Unless a licence is added, all rights are reserved; public visibility does not itself grant permission to reuse the code.

## Author

**George Oluwasemilore**  
GitHub: [@botishbot](https://github.com/botishbot)

---

<p align="center"><strong>SCAPMS</strong><br>Smart Clinic Appointment & Patient Management System</p>
