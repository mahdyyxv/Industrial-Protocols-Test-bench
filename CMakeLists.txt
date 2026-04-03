cmake_minimum_required(VERSION 3.21)

project(Project1RTU
    VERSION 0.1.0
    DESCRIPTION "RTU/SCADA Protocol Testing Tool"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Qt6 modules
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Quick
    QuickControls2
    Network
    Test
    QuickTest
)

# SerialPort — optional, must match the Qt version in CMAKE_PREFIX_PATH.
# To add SerialPort to your Qt 6.11 install:
#   Open Qt Maintenance Tool → Add components → Qt 6.11 → Serial Port
# Or install the matching system dev package if building against system Qt.
find_package(Qt6 OPTIONAL_COMPONENTS SerialPort)
if(Qt6SerialPort_FOUND)
    message(STATUS "Qt6SerialPort ${Qt6SerialPort_VERSION} — serial module enabled")
else()
    message(WARNING "Qt6SerialPort not found — serial module disabled.\n"
        "   To enable: open Qt Maintenance Tool and add 'Qt Serial Port' "
        "under Qt 6.11 components, then re-run cmake.")
endif()

# QML resource path policies — must be set before qt_standard_project_setup()
# QTP0001 NEW: places QML files under qrc:/qt/qml/<URI>/ (required for static plugin import)
# QTP0004 NEW: generates separate qmldir resource files (avoids stale-cache warnings)
qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

qt_standard_project_setup()

# Version info available to all targets
add_compile_definitions(
    APP_VERSION="${PROJECT_VERSION}"
    APP_NAME="${PROJECT_NAME}"
)

add_subdirectory(src)

enable_testing()
add_subdirectory(tests)
