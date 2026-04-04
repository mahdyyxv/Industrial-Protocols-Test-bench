#include "ModbusTcpClient.h"

#include <modbus.h>
#include <cerrno>
#include <cstring>

namespace rtu::core::modbus {

ModbusTcpClient::ModbusTcpClient(const QString& host, int port, QObject* parent)
    : QObject(parent)
    , m_host(host)
    , m_port(port)
{}

ModbusTcpClient::~ModbusTcpClient()
{
    disconnect();
}

// ── IModbusClient lifecycle ───────────────────────────────────

bool ModbusTcpClient::connect()
{
    if (m_state == ModbusState::Connected) return true;

    setState(ModbusState::Connecting);

    // Create TCP context (no I/O yet)
    m_ctx = modbus_new_tcp(m_host.toUtf8().constData(), m_port);
    if (!m_ctx) {
        handleError(QStringLiteral("modbus_new_tcp failed"));
        return false;
    }

    // Apply response timeout
    const uint32_t sec  = static_cast<uint32_t>(m_responseTimeoutMs / 1000);
    const uint32_t usec = static_cast<uint32_t>((m_responseTimeoutMs % 1000) * 1000);
    modbus_set_response_timeout(m_ctx, sec, usec);

    // Establish TCP connection
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

void ModbusTcpClient::disconnect()
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

bool        ModbusTcpClient::isConnected() const { return m_state == ModbusState::Connected; }
ModbusState ModbusTcpClient::state()       const { return m_state; }
QString     ModbusTcpClient::lastError()   const { return m_lastError; }

// ── IModbusClient register ops ────────────────────────────────

std::vector<uint16_t> ModbusTcpClient::readHoldingRegisters(int addr, int count)
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

bool ModbusTcpClient::writeSingleRegister(int addr, uint16_t value)
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

// ── Timeout ───────────────────────────────────────────────────

void ModbusTcpClient::setResponseTimeout(int ms)
{
    m_responseTimeoutMs = ms;
    if (m_ctx) {
        const uint32_t sec  = static_cast<uint32_t>(ms / 1000);
        const uint32_t usec = static_cast<uint32_t>((ms % 1000) * 1000);
        modbus_set_response_timeout(m_ctx, sec, usec);
    }
}

int ModbusTcpClient::responseTimeout() const { return m_responseTimeoutMs; }

// ── Private helpers ───────────────────────────────────────────

void ModbusTcpClient::setState(ModbusState s)
{
    if (m_state == s) return;
    m_state = s;
    emit stateChanged(m_state);
}

void ModbusTcpClient::handleError(const QString& message)
{
    m_lastError = message;
    setState(ModbusState::Error);
    emit errorOccurred(message);
}

} // namespace rtu::core::modbus
