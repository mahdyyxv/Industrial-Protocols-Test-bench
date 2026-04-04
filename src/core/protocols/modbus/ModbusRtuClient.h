#pragma once

#include "IModbusClient.h"
#include <QObject>
#include <QString>

// Forward-declare libmodbus internal type to keep modbus.h out of this header.
struct _modbus;

namespace rtu::core::modbus {

// ──────────────────────────────────────────────────────────────
// ModbusRtuConfig
//
// Serial port configuration for Modbus RTU.
// Uses the same field names as serial::SerialConfig for easy
// interoperability; parity uses libmodbus convention ('N'/'E'/'O').
//
// Conversion helper from serial::SerialConfig is provided in
// ModbusRtuClient.cpp when RTU_SERIAL_ENABLED is defined.
// ──────────────────────────────────────────────────────────────
struct ModbusRtuConfig {
    QString portName;
    int     baudRate          { 9600 };
    char    parity            { 'N' };  // 'N'=None, 'E'=Even, 'O'=Odd
    int     dataBits          { 8 };
    int     stopBits          { 1 };
    int     responseTimeoutMs { 500 };
};

// ──────────────────────────────────────────────────────────────
// ModbusRtuClient
//
// Connects to a Modbus RTU slave over a serial port.
// libmodbus manages the serial fd; SerialManager MUST NOT open
// the same port simultaneously.
// ──────────────────────────────────────────────────────────────
class ModbusRtuClient : public QObject, public IModbusClient {
    Q_OBJECT

public:
    explicit ModbusRtuClient(const ModbusRtuConfig& config,
                              QObject* parent = nullptr);
    ~ModbusRtuClient() override;

    // IModbusClient
    bool        connect()     override;
    void        disconnect()  override;
    bool        isConnected() const override;
    ModbusState state()       const override;
    QString     lastError()   const override;

    std::vector<uint16_t> readHoldingRegisters(int addr, int count)     override;
    bool                  writeSingleRegister(int addr, uint16_t value) override;

    // Slave ID to address on the bus (default 1)
    void setSlaveId(int id);
    int  slaveId() const;

signals:
    void stateChanged(ModbusState newState);
    void errorOccurred(const QString& message);

private:
    void setState(ModbusState s);
    void handleError(const QString& context);

    ModbusRtuConfig m_config;
    int             m_slaveId { 1 };
    ModbusState     m_state   { ModbusState::Disconnected };
    QString         m_lastError;
    struct _modbus* m_ctx { nullptr };
};

} // namespace rtu::core::modbus


// ──────────────────────────────────────────────────────────────
// SerialConfig → ModbusRtuConfig conversion helper
// Only available when Qt6SerialPort is present (Phase 3 serial module).
// ──────────────────────────────────────────────────────────────
#ifdef RTU_SERIAL_ENABLED
#include "core/protocols/serial/SerialManager.h"
namespace rtu::core::modbus {

inline ModbusRtuConfig fromSerialConfig(const serial::SerialConfig& sc)
{
    // Map QSerialPort::Parity enum → libmodbus char
    char parity = 'N';
    switch (sc.parity) {
        case QSerialPort::EvenParity:  parity = 'E'; break;
        case QSerialPort::OddParity:   parity = 'O'; break;
        default:                       parity = 'N'; break;
    }

    return ModbusRtuConfig{
        sc.portName,
        static_cast<int>(sc.baudRate),
        parity,
        static_cast<int>(sc.dataBits),
        static_cast<int>(sc.stopBits),
        sc.readTimeoutMs
    };
}

} // namespace rtu::core::modbus
#endif // RTU_SERIAL_ENABLED
