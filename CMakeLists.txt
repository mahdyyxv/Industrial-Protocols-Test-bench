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

# Suppress Qt6.8 QML policy warnings
qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

# SerialPort is optional — install Qt6SerialPort to enable serial protocols
find_package(Qt6 OPTIONAL_COMPONENTS SerialPort)
if(Qt6SerialPort_FOUND)
    message(STATUS "Qt6SerialPort found — serial protocols enabled")
    add_compile_definitions(RTU_SERIAL_ENABLED)
else()
    message(WARNING "Qt6SerialPort not found — serial protocols disabled. "
                    "Install libqt6serialport6-dev to enable.")
endif()

qt_standard_project_setup()

# Version info available to all targets
add_compile_definitions(
    APP_VERSION="${PROJECT_VERSION}"
    APP_NAME="${PROJECT_NAME}"
)

add_subdirectory(src)

enable_testing()
add_subdirectory(tests)
