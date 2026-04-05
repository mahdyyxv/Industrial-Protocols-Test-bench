# ── GoogleTest — fetched once, shared by all gtest targets ────
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
    GIT_SHALLOW    TRUE
)
# Do not install gtest alongside the main project
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# ── C++ protocol interface test ───────────────────────────────
qt_add_executable(tst_protocol_interface
    tst_protocol_interface.cpp
)
target_include_directories(tst_protocol_interface PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tst_protocol_interface PRIVATE Qt6::Test Qt6::Core rtu_core rtu_services)
add_test(NAME tst_protocol_interface COMMAND tst_protocol_interface)

# ── QML theme / UI state tests ────────────────────────────────
qt_add_executable(tst_ui_runner
    tst_ui_runner.cpp
)

qt_add_qml_module(tst_ui_runner
    URI tst.ui
    VERSION 1.0
    QML_FILES tst_theme.qml
)

target_include_directories(tst_ui_runner PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tst_ui_runner PRIVATE
    Qt6::QuickTest
    Qt6::Quick
    Qt6::Test
    rtu_ui
    rtu_uiplugin
)

add_test(
    NAME    tst_ui
    COMMAND tst_ui_runner -input ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(tst_ui tst_protocol_interface PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

# ── Serial manager tests ──────────────────────────────────────
qt_add_executable(tst_serial_manager
    tst_serial_manager.cpp
)
target_include_directories(tst_serial_manager PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tst_serial_manager PRIVATE Qt6::Test Qt6::Core rtu_core)
if(Qt6SerialPort_FOUND)
    target_link_libraries(tst_serial_manager PRIVATE Qt6::SerialPort)
endif()
add_test(NAME tst_serial COMMAND tst_serial_manager)
set_tests_properties(tst_serial PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

# ── IEC-104 module tests (GoogleTest) ────────────────────────
if(lib60870_FOUND)
    add_executable(tst_iec104 tst_iec104.cpp)
    target_include_directories(tst_iec104 PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(tst_iec104 PRIVATE
        GTest::gtest
        GTest::gmock
        Qt6::Core
        rtu_core
    )
    add_test(NAME tst_iec104 COMMAND tst_iec104)
    set_tests_properties(tst_iec104 PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endif()

# ── Device Simulator tests (GoogleTest) ──────────────────────
add_executable(tst_simulator tst_simulator.cpp)
target_include_directories(tst_simulator PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tst_simulator PRIVATE
    GTest::gtest
    GTest::gmock
    Qt6::Core
    Qt6::Test
    rtu_core
)
add_test(NAME tst_simulator COMMAND tst_simulator)
set_tests_properties(tst_simulator PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

# ── Packet Analyzer tests (GoogleTest) ───────────────────────
add_executable(tst_analyzer tst_analyzer.cpp)
target_include_directories(tst_analyzer PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tst_analyzer PRIVATE
    GTest::gtest
    GTest::gmock
    Qt6::Core
    Qt6::Test
    rtu_core
)
add_test(NAME tst_analyzer COMMAND tst_analyzer)
set_tests_properties(tst_analyzer PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

# ── Modbus module tests (GoogleTest) ──────────────────────────
if(libmodbus_FOUND)
    add_executable(tst_modbus tst_modbus.cpp)
    target_include_directories(tst_modbus PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(tst_modbus PRIVATE
        GTest::gtest
        GTest::gmock
        Qt6::Core
        rtu_core
    )
    if(Qt6SerialPort_FOUND)
        target_link_libraries(tst_modbus PRIVATE Qt6::SerialPort)
    endif()
    add_test(NAME tst_modbus COMMAND tst_modbus)
    set_tests_properties(tst_modbus PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endif()

# ── Controller integration tests (GoogleTest) ────────────────
add_executable(tst_integration tst_integration.cpp)
target_include_directories(tst_integration PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tst_integration PRIVATE
    GTest::gtest
    GTest::gmock
    Qt6::Core
    Qt6::Test
    rtu_core
    rtu_ui
)
add_test(NAME tst_integration COMMAND tst_integration)
set_tests_properties(tst_integration PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
