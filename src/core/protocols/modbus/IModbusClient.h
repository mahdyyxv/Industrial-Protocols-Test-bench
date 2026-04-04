#pragma once

#include <cstdint>
#include <vector>
#include <QString>

namespace rtu::core::modbus {

// Connection states for both TCP and RTU clients
enum class ModbusState {
    Disconnected,
    Connecting,
    Connected,
    Error
};

// ──────────────────────────────────────────────────────────────
// IModbusClient
//
// Pure C++ interface hiding all libmodbus types from callers.
// Concrete classes (ModbusTcpClient, ModbusRtuClient) inherit
// QObject separately and expose Qt signals on top of this.
// ──────────────────────────────────────────────────────────────
class IModbusClient {
public:
    virtual ~IModbusClient() = default;

    // Lifecycle
    virtual bool connect()    = 0;
    virtual void disconnect() = 0;

    // State
    virtual bool        isConnected() const = 0;
    virtual ModbusState state()       const = 0;
    virtual QString     lastError()   const = 0;

    // Register operations — return empty vector on failure
    virtual std::vector<uint16_t> readHoldingRegisters(int addr, int count)  = 0;
    virtual bool                  writeSingleRegister(int addr, uint16_t value) = 0;
};

} // namespace rtu::core::modbus
