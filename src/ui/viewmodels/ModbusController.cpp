#include "ModbusController.h"

#include <QDateTime>

#ifdef RTU_MODBUS_ENABLED
#include "core/protocols/modbus/ModbusTcpClient.h"
#include "core/protocols/modbus/ModbusRtuClient.h"
#endif

namespace rtu::ui {

ModbusController::ModbusController(QObject* parent)
    : QObject(parent)
{}

ModbusController::~ModbusController() { destroyClients(); }

// ── Accessors ─────────────────────────────────────────────────

bool         ModbusController::connected()     const { return m_connected; }
QString      ModbusController::mode()          const { return m_mode; }
QString      ModbusController::host()          const { return m_host; }
int          ModbusController::port()          const { return m_port; }
QString      ModbusController::portName()      const { return m_portName; }
int          ModbusController::baudRate()       const { return m_baudRate; }
int          ModbusController::slaveId()       const { return m_slaveId; }
QString      ModbusController::lastError()     const { return m_lastError; }
QVariantList ModbusController::responseModel() const { return m_responseModel; }
QStringList  ModbusController::rawLog()        const { return m_rawLog; }

// ── Setters ───────────────────────────────────────────────────

void ModbusController::setMode    (const QString& v) { if (m_mode     != v) { m_mode     = v; emit modeChanged();   } }
void ModbusController::setHost    (const QString& v) { if (m_host     != v) { m_host     = v; emit configChanged(); } }
void ModbusController::setPort    (int v)            { if (m_port     != v) { m_port     = v; emit configChanged(); } }
void ModbusController::setPortName(const QString& v) { if (m_portName != v) { m_portName = v; emit configChanged(); } }
void ModbusController::setBaudRate(int v)            { if (m_baudRate != v) { m_baudRate = v; emit configChanged(); } }
void ModbusController::setSlaveId (int v)            { if (m_slaveId  != v) { m_slaveId  = v; emit configChanged(); } }

// ── connect() ─────────────────────────────────────────────────

void ModbusController::connectDevice()
{
    if (m_connected) {
        disconnectDevice();
        return;
    }

#ifdef RTU_MODBUS_ENABLED
    destroyClients();

    if (m_mode == QLatin1String("TCP")) {
        auto client = std::make_unique<core::modbus::ModbusTcpClient>(m_host, m_port, this);

        QObject::connect(client.get(), &core::modbus::ModbusTcpClient::errorOccurred,
                         this, [this](const QString& err) { setLastError(err); emit errorOccurred(err); });

        if (!client->connect()) {
            setLastError(client->lastError());
            emit errorOccurred(m_lastError);
            appendLog(QStringLiteral("✗ TCP connect failed: %1").arg(m_lastError));
            return;
        }
        m_tcpClient = std::move(client);

    } else {
        // RTU
        using Cfg = core::modbus::ModbusRtuConfig;
        Cfg cfg;
        cfg.portName          = m_portName;
        cfg.baudRate          = m_baudRate;
        cfg.parity            = 'N';
        cfg.dataBits          = 8;
        cfg.stopBits          = 1;
        cfg.responseTimeoutMs = 1000;

        auto client = std::make_unique<core::modbus::ModbusRtuClient>(cfg, this);

        QObject::connect(client.get(), &core::modbus::ModbusRtuClient::errorOccurred,
                         this, [this](const QString& err) { setLastError(err); emit errorOccurred(err); });

        if (!client->connect()) {
            setLastError(client->lastError());
            emit errorOccurred(m_lastError);
            appendLog(QStringLiteral("✗ RTU connect failed: %1").arg(m_lastError));
            return;
        }
        m_rtuClient = std::move(client);
    }

    setConnected(true);
    appendLog(QStringLiteral("→ Connected (%1)").arg(m_mode));

#else
    setLastError(QStringLiteral("Modbus support not compiled in (RTU_MODBUS_ENABLED not set)"));
    emit errorOccurred(m_lastError);
#endif
}

// ── disconnect() ──────────────────────────────────────────────

void ModbusController::disconnectDevice()
{
    if (!m_connected) return;

#ifdef RTU_MODBUS_ENABLED
    if (m_tcpClient) m_tcpClient->disconnect();
    if (m_rtuClient) m_rtuClient->disconnect();
    destroyClients();
#endif

    setConnected(false);
    appendLog(QStringLiteral("← Disconnected"));
}

// ── read() ────────────────────────────────────────────────────

void ModbusController::read(int address, int count)
{
    if (!m_connected) {
        setLastError(QStringLiteral("Not connected"));
        emit errorOccurred(m_lastError);
        return;
    }

    appendLog(QStringLiteral("→ Read HR addr=%1 count=%2").arg(address).arg(count));

#ifdef RTU_MODBUS_ENABLED
    std::vector<uint16_t> values;

    if (m_mode == QLatin1String("TCP") && m_tcpClient)
        values = m_tcpClient->readHoldingRegisters(address, count);
    else if (m_rtuClient)
        values = m_rtuClient->readHoldingRegisters(address, count);

    if (values.empty()) {
        const QString err = m_mode == QLatin1String("TCP")
            ? (m_tcpClient ? m_tcpClient->lastError() : QStringLiteral("No client"))
            : (m_rtuClient ? m_rtuClient->lastError() : QStringLiteral("No client"));
        setLastError(err);
        emit errorOccurred(err);
        appendLog(QStringLiteral("✗ Read failed: %1").arg(err));
        return;
    }

    // Build response model
    m_responseModel.clear();
    QVariantList readResult;
    for (int i = 0; i < static_cast<int>(values.size()); ++i) {
        QVariantMap row;
        row[QStringLiteral("address")] = address + i;
        row[QStringLiteral("dec")]     = values[i];
        row[QStringLiteral("hex")]     = QStringLiteral("0x%1").arg(values[i], 4, 16, QChar('0')).toUpper();
        row[QStringLiteral("bin")]     = QStringLiteral("%1").arg(values[i], 16, 2, QChar('0'));
        m_responseModel.append(row);
        readResult.append(row);
    }
    emit responseModelChanged();
    emit readCompleted(readResult);
    appendLog(QStringLiteral("← Read OK: %1 registers").arg(values.size()));

#else
    emit errorOccurred(QStringLiteral("Modbus not compiled in"));
#endif
}

// ── write() ───────────────────────────────────────────────────

void ModbusController::write(int address, int value)
{
    if (!m_connected) {
        setLastError(QStringLiteral("Not connected"));
        emit errorOccurred(m_lastError);
        return;
    }

    appendLog(QStringLiteral("→ Write HR addr=%1 value=%2").arg(address).arg(value));

#ifdef RTU_MODBUS_ENABLED
    bool ok = false;
    if (m_mode == QLatin1String("TCP") && m_tcpClient)
        ok = m_tcpClient->writeSingleRegister(address, static_cast<uint16_t>(value));
    else if (m_rtuClient)
        ok = m_rtuClient->writeSingleRegister(address, static_cast<uint16_t>(value));

    if (!ok) {
        const QString err = m_mode == QLatin1String("TCP")
            ? (m_tcpClient ? m_tcpClient->lastError() : QStringLiteral("No client"))
            : (m_rtuClient ? m_rtuClient->lastError() : QStringLiteral("No client"));
        setLastError(err);
        emit errorOccurred(err);
        appendLog(QStringLiteral("✗ Write failed: %1").arg(err));
    } else {
        appendLog(QStringLiteral("← Write OK"));
    }
    emit writeCompleted(ok);
#else
    emit errorOccurred(QStringLiteral("Modbus not compiled in"));
    emit writeCompleted(false);
#endif
}

// ── clearLog() ────────────────────────────────────────────────

void ModbusController::clearLog()
{
    m_responseModel.clear();
    m_rawLog.clear();
    emit responseModelChanged();
    emit rawLogChanged();
}

// ── Private helpers ───────────────────────────────────────────

void ModbusController::setConnected(bool v)
{
    if (m_connected == v) return;
    m_connected = v;
    emit connectedChanged();
}

void ModbusController::appendLog(const QString& line)
{
    const QString entry = QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss.zzz"))
                          + QStringLiteral("  ") + line;
    m_rawLog.append(entry);
    // Keep log bounded to 500 lines
    if (m_rawLog.size() > 500)
        m_rawLog.removeFirst();
    emit rawLogChanged();
}

void ModbusController::setLastError(const QString& err)
{
    m_lastError = err;
}

void ModbusController::destroyClients()
{
#ifdef RTU_MODBUS_ENABLED
    m_tcpClient.reset();
    m_rtuClient.reset();
#endif
}

} // namespace rtu::ui
