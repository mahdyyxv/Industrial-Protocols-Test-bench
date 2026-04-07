cmake_minimum_required(VERSION 3.21)

project(Project1RTU
    VERSION 0.1.0
    DESCRIPTION "RTU/SCADA Protocol Testing Tool"
    LANGUAGES CXX C
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

# ── Windows: inject MSYS2 MinGW64 into the search paths ──────
# libmodbus is installed there via: pacman -S mingw-w64-x86_64-libmodbus
if(WIN32)
    list(APPEND CMAKE_PREFIX_PATH "C:/msys64/mingw64")
    set(ENV{PKG_CONFIG_PATH} "C:/msys64/mingw64/lib/pkgconfig;$ENV{PKG_CONFIG_PATH}")
    set(ENV{PATH} "C:/msys64/mingw64/bin;$ENV{PATH}")
endif()

# ── libmodbus — optional, enables Modbus RTU + TCP module ─────
# Windows (MSYS2/MinGW): pacman -S mingw-w64-x86_64-libmodbus  (already handled above)
# Windows (vcpkg):       vcpkg install libmodbus  (pass -DCMAKE_TOOLCHAIN_FILE=.../vcpkg.cmake)
# Linux:                 sudo apt-get install libmodbus-dev
find_package(libmodbus QUIET CONFIG)   # vcpkg / CMake config style (creates libmodbus::modbus)

if(NOT libmodbus_FOUND)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        # IMPORTED_TARGET creates PkgConfig::MODBUS with correct link flags automatically
        # Works on Linux and MSYS2/MinGW64
        pkg_check_modules(MODBUS QUIET IMPORTED_TARGET libmodbus)
        if(MODBUS_FOUND)
            set(libmodbus_FOUND TRUE)
        endif()
    endif()
endif()

if(libmodbus_FOUND)
    message(STATUS "libmodbus found — Modbus module enabled")
else()
    message(WARNING "libmodbus not found — Modbus module disabled.\n"
        "   Windows (MSYS2/MinGW): pacman -S mingw-w64-x86_64-libmodbus\n"
        "   Windows (vcpkg):       vcpkg install libmodbus\n"
        "   Linux:                 sudo apt-get install libmodbus-dev")
endif()

# ── lib60870-C — IEC 60870-5-104 client library ───────────────
# Fetched automatically; no system install required.
# Source: https://github.com/mz-automation/lib60870
include(FetchContent)
FetchContent_Declare(
    lib60870
    GIT_REPOSITORY https://github.com/mz-automation/lib60870.git
    GIT_TAG        v2.3.2
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  lib60870-C
)
# Suppress the library's own examples and tests
set(WITH_EXAMPLES     OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS       OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(lib60870)
set(lib60870_FOUND TRUE)
message(STATUS "lib60870-C fetched — IEC-104 module enabled")

# Belt-and-suspenders: exclude lib60870's example executables from the default
# 'all' target even if WITH_EXAMPLES=OFF didn't take effect in a cached build.
if(IS_DIRECTORY "${lib60870_SOURCE_DIR}/lib60870-C/examples")
    set_property(
        DIRECTORY "${lib60870_SOURCE_DIR}/lib60870-C/examples"
        PROPERTY  EXCLUDE_FROM_ALL TRUE
    )
endif()

add_subdirectory(src)

enable_testing()
add_subdirectory(tests)
