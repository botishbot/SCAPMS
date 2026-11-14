# SCAPMS — Smart Clinic Appointment & Patient Management System

> A desktop clinic-management application built with **C++17**, **Qt 6 Widgets** and **CMake**. SCAPMS brings patient records, appointments, waiting-list operations, search, sorting and clinic reports into one graphical application.

![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)
![Qt](https://img.shields.io/badge/Qt-6-41CD52?style=for-the-badge&logo=qt&logoColor=white)
![CMake](https://img.shields.io/badge/CMake-Build-064F8C?style=for-the-badge&logo=cmake&logoColor=white)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-blue?style=for-the-badge)
![License](https://img.shields.io/badge/License-Not%20specified-lightgrey?style=for-the-badge)

---

## Contents

- [Overview](#-overview)
- [Features](#-features)
- [Screenshots](#-screenshots)
- [Downloads](#-downloads)
- [Technology stack](#-technology-stack)
- [Getting started](#-getting-started)
- [Building from source](#-building-from-source)
- [Using the application](#-using-the-application)
- [Architecture and object-oriented design](#-architecture-and-object-oriented-design)
- [Data structures and algorithms](#-data-structures-and-algorithms)
- [Data storage and validation](#-data-storage-and-validation)
- [Project structure](#-project-structure)
- [Testing](#-testing)
- [Contributing](#-contributing)
- [Author](#-author)

## 📖 Overview

The **Smart Clinic Appointment & Patient Management System (SCAPMS)** is a C++ desktop application developed to demonstrate how programming concepts can be applied to routine clinic operations. It provides a single graphical workspace for managing patient information, appointments and the waiting queue, while also demonstrating object-oriented design, manually implemented searching and sorting algorithms, recursion, data persistence and report generation.

The application is organized into model, controller, GUI and utility components. This separation keeps patient and appointment data management distinct from the interface and makes the codebase easier to navigate and maintain.

SCAPMS is an educational project. It is not intended to replace a clinically validated electronic medical-record system or to store real patient information in a production environment.

## ✨ Features

### Patient management
- Register and manage patient records.
- Represent General, Emergency and Follow-Up patient categories.
- Validate common patient details before creating records.
- View patient information and appointment history.

### Appointment management
- Schedule and manage patient appointments.
- Track appointment states and pending work.
- Keep appointment operations associated with patient records.

### Waiting queue
- Manage the consultation waiting list using first-in, first-out (FIFO) order.
- Keep queue operations separate from the patient data model.

### Search and sorting
- Search patient records by ID, name or category.
- Choose from linear, binary and recursive search where the current field and data order permit.
- Sort by name, ID or age using manually implemented Selection Sort and Insertion Sort.
- Display algorithm comparison and movement statistics in the application.

### Dashboard and reports
- View clinic summary information on the dashboard.
- Generate reports covering frequently attended patients, pending appointments, patient-category distribution and daily appointment summaries.

### Data and administration
- Save and load application data in JSON format.
- Export supported tabular information to CSV.
- Manage clinic services and load demonstration data.
- Use undo support for supported operations.

## 🖼️ Screenshots

Screenshots should show the application itself. The repository's screenshot assets will be linked here as they are added.

| Application area | What it demonstrates |
|---|---|
| Dashboard | Clinic overview and navigation |
| Patients | Patient registration and record management |
| Appointments | Appointment creation and management |
| Waiting queue | FIFO queue workflow |
| Search and sorting | Search methods and sorting options |
| Reports | Generated clinic summaries |

> **Screenshot assets:** Add the verified application captures under `screenshots/` and link them here. No screenshot paths are assumed until the corresponding files are present in the repository.

## 📦 Downloads

Pre-built application packages are published on the **[GitHub Releases page](https://github.com/botishbot/SCAPMS/releases)**. Open the latest release and download the package for your operating system.

| Platform | Package | How to run |
|---|---|---|
| Windows 10/11, 64-bit | `SCAPMS-Windows-x64.zip` | Extract the ZIP and run `SCAPMS.exe`. Keep the included DLLs and Qt plugin folders beside the executable. |
| Linux, x86_64 | `SCAPMS-Linux-x86_64.AppImage` | Make the file executable, then launch it. |
| macOS | Release asset, when available | Follow the instructions attached to the corresponding release. |

### Linux AppImage

After downloading the AppImage:

```bash
chmod +x SCAPMS-Linux-x86_64.AppImage
./SCAPMS-Linux-x86_64.AppImage
```

### Windows

1. Download `SCAPMS-Windows-x64.zip` from the latest release.
2. Extract the entire archive to a folder.
3. Run `SCAPMS.exe`.
4. Do not move the executable away from its bundled DLLs or Qt plugin directories.

**Important:** A release asset will only be available after the corresponding GitHub Actions build has completed successfully. If a platform package is missing from a release, use the source-build instructions below or check the release's build status.

## 🛠️ Technology stack

- **C++17** — application logic and object-oriented implementation.
- **Qt 6 Widgets** — desktop graphical user interface and event handling.
- **CMake** — cross-platform project configuration and build system.
- **Qt Core** — JSON/file-related support.
- **JSON** — persistent application data.
- **CSV** — data export.
- **C++ Standard Library** — containers including `std::vector`, `std::queue` and `std::stack`.
- **GitHub Actions** — automated build and release workflows.

## 🚀 Getting started

### Requirements

To build the application from source, install:

- A C++17-compatible compiler.
- CMake 3.16 or newer.
- Qt 6 with the Core and Widgets components.
- Git (to clone the repository).

On Ubuntu/Debian-based Linux distributions, the basic dependencies can be installed with:

```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev
```

### Get the source code

```bash
git clone https://github.com/botishbot/SCAPMS.git
cd SCAPMS
```

## 🔧 Building from source

### Linux

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/SCAPMS
```

### Windows

Install Visual Studio 2022 with the Desktop development with C++ workload, CMake and Qt 6. Open a developer terminal in the repository directory:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/msvc2022_64"
cmake --build build --config Release --parallel
.\build\Release\SCAPMS.exe
```

Replace the Qt path with the actual location and version installed on your computer. The exact generator and Qt kit must use compatible compiler toolchains.

### macOS

Install Xcode Command Line Tools, CMake and Qt 6. Configure CMake with the path to the installed Qt package:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build build --config Release --parallel
./build/SCAPMS
```

The macOS build instructions are provided as a source-build route; availability of a signed/notarized distributable depends on the release configuration.

## 🧭 Using the application

On first launch, open **Administration** and select **Load demo data** to populate the application with fictional patient records, services and appointments. The repository also contains `data/sample_data.json`, which can be loaded through the application's file-loading option.

The main areas of the application include:

1. **Dashboard** — review clinic summary information.
2. **Patients** — register and manage patient records.
3. **Appointments** — schedule and manage appointments.
4. **Waiting Queue** — manage the order in which patients are attended.
5. **Search & Sort** — find records and arrange the working list.
6. **Reports** — generate clinic activity summaries.
7. **Administration** — manage services and data operations.

Use fictional or test data only. The project has not been presented as a production-ready clinical records platform.

## 🧱 Architecture and object-oriented design

The code is organized into four principal namespaces:

- `scapms::model` — patient, appointment, service and database domain objects.
- `scapms::controller` — application operations, queue/undo management, searching, sorting and reports.
- `scapms::gui` — Qt windows, pages and user interaction.
- `scapms::utils` — validation, identifiers, file handling and application exceptions.

### Patient class hierarchy

`PatientRecord` is an abstract base class that contains common patient information and implements the `Appointable` abstraction. The concrete patient types are:

- `GeneralPatient`
- `EmergencyPatient`
- `FollowUpPatient`

Each concrete class provides its category-specific behaviour through the common abstraction. This demonstrates abstraction, inheritance and runtime polymorphism.

### Main domain and application components

- **Appointment** represents an appointment and its details.
- **ClinicDatabase** manages patient and service data.
- **QueueManager** manages the waiting queue.
- **UndoManager** coordinates supported undo commands.
- **SearchEngine** and **SortEngine** implement the required algorithms.
- **ReportGenerator** prepares clinic summaries.
- **MainWindow** and the GUI pages provide the Qt Widgets interface.

Qt signals and slots connect user-interface events to application operations. The GUI uses controllers rather than taking direct ownership of the application's database logic.

## 🧠 Data structures and algorithms

The project includes hand-written searching and sorting algorithms. The detailed explanation, pseudocode, selection rationale and complexity discussion are available in **[docs/ALGORITHMS.md](docs/ALGORITHMS.md)**.

| Structure / algorithm | Role in SCAPMS |
|---|---|
| `std::vector` | Stores the variable-sized patient collection and working lists. |
| `std::queue` | Maintains the waiting list in FIFO order. |
| `std::stack` | Stores undo commands in last-in, first-out order. |
| Fixed array cache | Keeps a limited set of frequently accessed patient records. |
| Linear Search | Searches records without requiring a particular list order. |
| Binary Search | Searches an appropriately ordered list; the application restricts its use to valid sorted states and supported fields. |
| Recursive Search | Divides the search range into smaller parts using recursive calls. |
| Selection Sort | Repeatedly selects the next record in the required order. |
| Insertion Sort | Inserts each record into its position in the already sorted portion. |

The sorting options support name, ID and age. Search behaviour depends on the selected field and current sort state; the application does not treat binary search as valid on an unsorted list.

## 💾 Data storage and validation

Application data is stored in JSON (schema version 1). The saved data includes patients and their appointments, clinic services, and appointment history for deleted patients. The waiting queue and undo history are runtime structures and are not saved as part of the JSON data file.

When data is loaded, the file is validated before replacing the current application data. Invalid JSON and invalid or inconsistent records are handled as errors; the documented loader is designed to leave the current data unchanged when validation fails.

CSV export is provided for supported tabular data. Input validation and application exceptions are used to handle unsuitable values and invalid operations.

## 📁 Project structure

```text
SCAPMS/
├── CMakeLists.txt
├── README.md
├── include/
│   ├── model/
│   ├── controller/
│   ├── gui/
│   └── utils/
├── src/
│   ├── model/
│   ├── controller/
│   ├── gui/
│   ├── utils/
│   └── main.cpp
├── resources/
│   ├── resources.qrc
│   └── style.qss
├── data/
│   └── sample_data.json
├── tests/
└── docs/
    └── ALGORITHMS.md
```

| Directory | Responsibility |
|---|---|
| `include/` | Public application headers grouped by component. |
| `src/model/` | Domain models and clinic data. |
| `src/controller/` | Application operations and algorithms. |
| `src/gui/` | Qt windows, pages and dialogs. |
| `src/utils/` | Validation, exceptions, identifiers and file utilities. |
| `resources/` | Qt resources and application styling. |
| `data/` | Example JSON data. |
| `tests/` | Automated test source files. |
| `docs/` | Supporting technical documentation. |

## 🧪 Testing

The project includes a CMake/CTest test target. After configuring and building the project, run:

```bash
ctest --test-dir build --output-on-failure
```

For a multi-configuration Windows build, build the test target in the desired configuration before running CTest:

```powershell
cmake --build build --config Release --target scapms_tests
ctest --test-dir build -C Release --output-on-failure
```

Test results should be checked for the specific build and platform being used; this README does not claim that every platform configuration has already passed.

## 🤝 Contributing

Suggestions, bug reports and improvements are welcome.

1. Fork the repository.
2. Create a focused feature or fix branch.
3. Make and test your changes.
4. Open a pull request describing the change and the checks performed.

Please avoid including real patient or other sensitive personal information in issues, screenshots, sample files or pull requests.

## 📄 License

No license file was present when this README was updated. Until a license is added by the project owner, the repository should be treated as **all rights reserved**; do not assume that public visibility grants permission to redistribute or modify the code.

## 👤 Author

**BotishBot**

GitHub: [@botishbot](https://github.com/botishbot)

If you find the project useful, consider starring the repository.

---

<p align="center">
  <strong>SCAPMS</strong><br>
  Smart Clinic Appointment & Patient Management System
</p>
