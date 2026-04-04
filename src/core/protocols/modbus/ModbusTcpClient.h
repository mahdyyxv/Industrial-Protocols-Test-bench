#pragma once

#include "IModbusClient.h"
#include <QObject>
#include <QString>

// Forward-declare libmodbus internal type to keep modbus.h out of this header.
// modbus_t is typedef struct _modbus modbus_t — forward-declaring the struct
// is sufficient for holding a pointer.
struct _modbus;

namespace rtu::core::modbus {

// ──────────────────────────────────────────────────────────────
// ModbusTcpClient
//
// Connects to a Modbus TCP server (slave) over TCP/IP.
// All libmodbus types are confined to the .cpp file.
// ──────────────────────────────────────────────────────────────
class ModbusTcpClient : public QObject, public IModbusClient {
    Q_OBJECT

public:
    explicit ModbusTcpClient(const QString& host, int port,
                              QObject* parent = nullptr);
    ~ModbusTcpClient() override;

    // IModbusClient
    bool        connect()     override;
    void        disconnect()  override;
    bool        isConnected() const override;
    ModbusState state()       const override;
    QString     lastError()   const override;

    std::vector<uint16_t> readHoldingRegisters(int addr, int count)     override;
    bool                  writeSingleRegister(int addr, uint16_t value) override;

    // Response timeout (milliseconds, applied before each call)
    void setResponseTimeout(int ms);
    int  responseTimeout() const;

signals:
    void stateChanged(ModbusState newState);
    void errorOccurred(const QString& message);

private:
    void setState(ModbusState s);
    void handleError(const QString& context);

    QString      m_host;
    int          m_port;
    int          m_responseTimeoutMs { 500 };
    ModbusState  m_state { ModbusState::Disconnected };
    QString      m_lastError;
    struct _modbus* m_ctx { nullptr };
};

} // namespace rtu::core::modbus
