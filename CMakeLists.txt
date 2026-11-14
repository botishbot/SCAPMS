cmake_minimum_required(VERSION 3.16)
project(SCAPMS VERSION 0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# Qt-free core: model, controller, utils (testable without any GUI)
file(GLOB CORE_SOURCES CONFIGURE_DEPENDS src/model/*.cpp src/controller/*.cpp src/utils/*.cpp)
list(FILTER CORE_SOURCES EXCLUDE REGEX "FileHandler")
add_library(scapms_core STATIC ${CORE_SOURCES})
target_include_directories(scapms_core PUBLIC include)

# File handling (JSON save/load, CSV export) needs Qt Core only, so it is a separate small library
add_library(scapms_io STATIC src/utils/FileHandler.cpp)
target_link_libraries(scapms_io PUBLIC scapms_core Qt6::Core)

# GUI application. On macOS, create an application bundle so macdeployqt can package it.
file(GLOB GUI_SOURCES CONFIGURE_DEPENDS src/gui/*.cpp)
file(GLOB GUI_HEADERS CONFIGURE_DEPENDS include/gui/*.h)
if(APPLE)
    add_executable(SCAPMS MACOSX_BUNDLE src/main.cpp ${GUI_SOURCES} ${GUI_HEADERS} resources/resources.qrc)
else()
    add_executable(SCAPMS src/main.cpp ${GUI_SOURCES} ${GUI_HEADERS} resources/resources.qrc)
endif()
target_link_libraries(SCAPMS PRIVATE scapms_io scapms_core Qt6::Widgets)

# Unit tests (run with: ctest --test-dir build --output-on-failure)
enable_testing()
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS tests/*.cpp)
add_executable(scapms_tests ${TEST_SOURCES})
target_link_libraries(scapms_tests PRIVATE scapms_io scapms_core)
add_test(NAME scapms_tests COMMAND scapms_tests)
