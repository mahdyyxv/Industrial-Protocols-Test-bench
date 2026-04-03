add_library(rtu_core STATIC
    models/ProtocolFrame.h
    protocols/IProtocol.h
    protocols/ProtocolRegistry.h
    engine/ITestSession.h
    engine/TestEngine.h
    engine/TestEngine.cpp
)

target_include_directories(rtu_core PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(rtu_core PUBLIC
    Qt6::Core
    Qt6::Network
    rtu_utils
)

# ── Serial module — requires Qt6SerialPort ─────────────────────
if(Qt6SerialPort_FOUND)
    message(STATUS "Qt6SerialPort found — building serial module")
    target_sources(rtu_core PRIVATE
        protocols/serial/SerialManager.h
        protocols/serial/SerialManager.cpp
        protocols/serial/SerialProtocol.h
        protocols/serial/SerialProtocol.cpp
    )
    target_link_libraries(rtu_core PUBLIC Qt6::SerialPort)
    target_compile_definitions(rtu_core PUBLIC RTU_SERIAL_ENABLED)
else()
    message(WARNING "Qt6SerialPort not found — serial module disabled. "
                    "Run: sudo apt-get install qt6-serialport-dev")
endif()
