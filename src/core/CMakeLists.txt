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

# ── Modbus module — requires libmodbus ─────────────────────────
if(libmodbus_FOUND)
    target_sources(rtu_core PRIVATE
        protocols/modbus/IModbusClient.h
        protocols/modbus/ModbusTcpClient.h
        protocols/modbus/ModbusTcpClient.cpp
        protocols/modbus/ModbusRtuClient.h
        protocols/modbus/ModbusRtuClient.cpp
    )
    # vcpkg creates libmodbus::modbus; pkg-config IMPORTED_TARGET creates PkgConfig::MODBUS
    if(TARGET libmodbus::modbus)
        target_link_libraries(rtu_core PUBLIC libmodbus::modbus)
    elseif(TARGET PkgConfig::MODBUS)
        target_link_libraries(rtu_core PUBLIC PkgConfig::MODBUS)
    endif()
    target_compile_definitions(rtu_core PUBLIC RTU_MODBUS_ENABLED)
endif()

# ── IEC-104 module — lib60870-C (always fetched) ──────────────
# lib60870-C uses directory-scope include_directories(), so we must
# add its API and HAL paths explicitly via the FetchContent source dir.
if(lib60870_FOUND)
    target_sources(rtu_core PRIVATE
        protocols/iec104/IEC104DataPoint.h
        protocols/iec104/IEC104Client.h
        protocols/iec104/IEC104Client.cpp
    )
    target_link_libraries(rtu_core PUBLIC lib60870)
    target_include_directories(rtu_core PRIVATE
        ${lib60870_SOURCE_DIR}/lib60870-C/src/inc/api
        ${lib60870_SOURCE_DIR}/lib60870-C/src/hal/inc
        ${lib60870_SOURCE_DIR}/lib60870-C/config
    )
    target_compile_definitions(rtu_core PUBLIC RTU_IEC104_ENABLED)
endif()

# ── Packet Analyzer module — always built ─────────────────────
target_sources(rtu_core PRIVATE
    analyzer/AnalyzedPacket.h
    analyzer/PacketFilter.h
    analyzer/PacketAnalyzer.h
    analyzer/PacketAnalyzer.cpp
)

# ── Device Simulator module — always built ────────────────────
target_sources(rtu_core PRIVATE
    simulator/SimulatorScenario.h
    simulator/DeviceSimulator.h
    simulator/DeviceSimulator.cpp
)
