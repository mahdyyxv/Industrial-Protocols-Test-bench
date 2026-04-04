#include "ModbusRtuClient.h"

#include <modbus.h>
#include <cerrno>
#include <cstring>

namespace rtu::core::modbus {

ModbusRtuClient::ModbusRtuClient(const ModbusRtuConfig& config, QObject* parent)
    : QObject(parent)
    , m_config(config)
{}

ModbusRtuClient::~ModbusRtuClient()
{
    disconnect();
}

// ── IModbusClient lifecycle ───────────────────────────────────

bool ModbusRtuClient::connect()
{
    if (m_state == ModbusState::Connected) return true;

    if (m_config.portName.isEmpty()) {
        handleError(QStringLiteral("connect: portName is empty"));
        return false;
    }

    setState(ModbusState::Connecting);

    m_ctx = modbus_new_rtu(
        m_config.portName.toUtf8().constData(),
        m_config.baudRate,
        m_config.parity,
        m_config.dataBits,
        m_config.stopBits
    );

    if (!m_ctx) {
        handleError(QStringLiteral("modbus_new_rtu failed: ") +
                    QString::fromUtf8(modbus_strerror(errno)));
        return false;
    }

    // Set slave address on the bus
    if (modbus_set_slave(m_ctx, m_slaveId) == -1) {
        handleError(QStringLiteral("modbus_set_slave: ") +
                    QString::fromUtf8(modbus_strerror(errno)));
        modbus_free(m_ctx);
        m_ctx = nullptr;
        return false;
    }

    // Apply response timeout
    const uint32_t sec  = static_cast<uint32_t>(m_config.responseTimeoutMs / 1000);
    const uint32_t usec = static_cast<uint32_t>((m_config.responseTimeoutMs % 1000) * 1000);
    modbus_set_response_timeout(m_ctx, sec, usec);

    if (modbus_connect(m_ctx) == -1) {
        handleError(QStringLiteral("modbus_connect: ") +
                    QString::fromUtf8(modbus_strerror(errno)));
        modbus_free(m_ctx);
        m_ctx = nullptr;
        return false;
    }

    setState(ModbusState::Connected);
    return true;
}

void ModbusRtuClient::disconnect()
{
    if (m_ctx) {
        modbus_close(m_ctx);
        modbus_free(m_ctx);
        m_ctx = nullptr;
    }
    if (m_state != ModbusState::Disconnected)
        setState(ModbusState::Disconnected);
}

// ── IModbusClient state ───────────────────────────────────────

bool        ModbusRtuClient::isConnected() const { return m_state == ModbusState::Connected; }
ModbusState ModbusRtuClient::state()       const { return m_state; }
QString     ModbusRtuClient::lastError()   const { return m_lastError; }

// ── IModbusClient register ops ────────────────────────────────

std::vector<uint16_t> ModbusRtuClient::readHoldingRegisters(int addr, int count)
{
    if (!isConnected()) {
        handleError(QStringLiteral("readHoldingRegisters: not connected"));
        return {};
    }
    if (count <= 0 || count > MODBUS_MAX_READ_REGISTERS) {
        handleError(QStringLiteral("readHoldingRegisters: count %1 out of range [1, %2]")
                    .arg(count).arg(MODBUS_MAX_READ_REGISTERS));
        return {};
    }

    std::vector<uint16_t> regs(static_cast<size_t>(count));
    const int rc = modbus_read_registers(m_ctx, addr, count, regs.data());
    if (rc == -1) {
        handleError(QStringLiteral("readHoldingRegisters: ") +
                    QString::fromUtf8(modbus_strerror(errno)));
        return {};
    }

    return regs;
}

bool ModbusRtuClient::writeSingleRegister(int addr, uint16_t value)
{
    if (!isConnected()) {
        handleError(QStringLiteral("writeSingleRegister: not connected"));
        return false;
    }

    if (modbus_write_register(m_ctx, addr, value) == -1) {
        handleError(QStringLiteral("writeSingleRegister: ") +
                    QString::fromUtf8(modbus_strerror(errno)));
        return false;
    }

    return true;
}

// ── Slave ID ──────────────────────────────────────────────────

void ModbusRtuClient::setSlaveId(int id)
{
    m_slaveId = id;
    if (m_ctx)
        modbus_set_slave(m_ctx, id);
}

int ModbusRtuClient::slaveId() const { return m_slaveId; }

// ── Private helpers ───────────────────────────────────────────

void ModbusRtuClient::setState(ModbusState s)
{
    if (m_state == s) return;
    m_state = s;
    emit stateChanged(m_state);
}

void ModbusRtuClient::handleError(const QString& message)
{
    m_lastError = message;
    setState(ModbusState::Error);
    emit errorOccurred(message);
}

} // namespace rtu::core::modbus
