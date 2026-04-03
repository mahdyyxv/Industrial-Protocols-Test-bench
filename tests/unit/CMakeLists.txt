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
