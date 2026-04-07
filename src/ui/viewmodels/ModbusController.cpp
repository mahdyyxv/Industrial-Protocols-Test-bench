#include "ModbusController.h"

#include <QDateTime>

#ifdef RTU_MODBUS_ENABLED
#include "core/protocols/modbus/ModbusTcpClient.h"
#include "core/protocols/modbus/ModbusRtuClient.h"
#include "core/protocols/modbus/ModbusTcpServer.h"
#endif

namespace rtu::ui {

ModbusController::ModbusController(QObject* parent)
    : QObject(parent)
{}

ModbusController::~ModbusController() { destroyClients(); }

// ── Accessors ─────────────────────────────────────────────────

bool         ModbusController::connected()       const { return m_connected; }
QString      ModbusController::role()            const { return m_role; }
QString      ModbusController::mode()            const { return m_mode; }
QString      ModbusController::host()            const { return m_host; }
int          ModbusController::port()            const { return m_port; }
QString      ModbusController::portName()        const { return m_portName; }
int          ModbusController::baudRate()        const { return m_baudRate; }
int          ModbusController::slaveId()         const { return m_slaveId; }
QString      ModbusController::lastError()       const { return m_lastError; }
QVariantList ModbusController::responseModel()   const { return m_responseModel; }
QStringList  ModbusController::rawLog()          const { return m_rawLog; }
int          ModbusController::listenPort()      const { return m_listenPort; }
QVariantList ModbusController::serverRegisters() const { return m_serverRegisters; }
int          ModbusController::clientCount()     const { return m_clientCount; }

// ── Setters ───────────────────────────────────────────────────

void ModbusController::setRole    (const QString& v) { if (m_role     != v) { m_role     = v; emit roleChanged();    } }
void ModbusController::setMode    (const QString& v) { if (m_mode     != v) { m_mode     = v; emit modeChanged();    } }
void ModbusController::setHost    (const QString& v) { if (m_host     != v) { m_host     = v; emit configChanged();  } }
void ModbusController::setPort    (int v)            { if (m_port     != v) { m_port     = v; emit configChanged();  } }
void ModbusController::setPortName(const QString& v) { if (m_portName != v) { m_portName = v; emit configChanged();  } }
void ModbusController::setBaudRate(int v)            { if (m_baudRate != v) { m_baudRate = v; emit configChanged();  } }
void ModbusController::setSlaveId (int v)            { if (m_slaveId  != v) { m_slaveId  = v; emit configChanged();  } }
void ModbusController::setListenPort(int v)          { if (m_listenPort != v) { m_listenPort = v; emit configChanged(); } }

// ── connectDevice() ───────────────────────────────────────────

void ModbusController::connectDevice()
{
    if (m_connected) {
        disconnectDevice();
        return;
    }

#ifdef RTU_MODBUS_ENABLED
    destroyClients();

    if (m_role == QLatin1String("Server")) {
        // ── Server mode: start TCP listener ──────────────────
        auto srv = std::make_unique<core::modbus::ModbusTcpServer>(this);

        QObject::connect(srv.get(), &core::modbus::ModbusTcpServer::serverStarted,
                         this, [this](int p) {
            setConnected(true);
            appendLog(QStringLiteral("→ Server listening on port %1").arg(p));
        });
        QObject::connect(srv.get(), &core::modbus::ModbusTcpServer::serverStopped,
                         this, [this]() {
            setConnected(false);
            appendLog(QStringLiteral("← Server stopped"));
        });
        QObject::connect(srv.get(), &core::modbus::ModbusTcpServer::clientConnected,
                         this, [this](const QString& ip) {
            m_clientCount++;
            emit clientCountChanged();
            appendLog(QStringLiteral("→ Client connected: %1").arg(ip));
        });
        QObject::connect(srv.get(), &core::modbus::ModbusTcpServer::clientDisconnected,
                         this, [this]() {
            if (m_clientCount > 0) { m_clientCount--; emit clientCountChanged(); }
            appendLog(QStringLiteral("← Client disconnected"));
        });
        QObject::connect(srv.get(), &core::modbus::ModbusTcpServer::registerWritten,
                         this, [this](int addr, uint16_t val) {
            appendLog(QStringLiteral("← Client wrote reg[%1] = %2").arg(addr).arg(val));
            rebuildServerRegisters();
        });
        QObject::connect(srv.get(), &core::modbus::ModbusTcpServer::errorOccurred,
                         this, [this](const QString& err) {
            setLastError(err);
            emit errorOccurred(err);
            appendLog(QStringLiteral("✗ Server error: %1").arg(err));
        });

        m_server = std::move(srv);
        m_server->start(m_listenPort);
        // connected flag set asynchronously via serverStarted signal

    } else {
        // ── Client mode: TCP or RTU ───────────────────────────
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
    }

#else
    setLastError(QStringLiteral("Modbus support not compiled in (RTU_MODBUS_ENABLED not set)"));
    emit errorOccurred(m_lastError);
#endif
}

// ── disconnectDevice() ────────────────────────────────────────

void ModbusController::disconnectDevice()
{
#ifdef RTU_MODBUS_ENABLED
    if (!m_connected && !m_server) return;
#else
    if (!m_connected) return;
#endif

#ifdef RTU_MODBUS_ENABLED
    if (m_server) {
        m_server->stop();
        m_server.reset();
        m_clientCount = 0;
        emit clientCountChanged();
        m_serverRegisters.clear();
        emit serverRegistersChanged();
    } else {
        if (m_tcpClient) m_tcpClient->disconnect();
        if (m_rtuClient) m_rtuClient->disconnect();
        destroyClients();
    }
#endif

    setConnected(false);
    appendLog(QStringLiteral("← Disconnected"));
}

// ── read() ────────────────────────────────────────────────────

void ModbusController::read(int address, int count)
{
    if (!m_connected || m_role == QLatin1String("Server")) {
        setLastError(QStringLiteral("Not connected as client"));
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
    if (!m_connected || m_role == QLatin1String("Server")) {
        setLastError(QStringLiteral("Not connected as client"));
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

// ── setServerRegister() / getServerRegister() ─────────────────

void ModbusController::setServerRegister(int address, int value)
{
#ifdef RTU_MODBUS_ENABLED
    if (m_server) {
        m_server->setRegisterValue(address, static_cast<uint16_t>(value));
        rebuildServerRegisters();
        appendLog(QStringLiteral("→ Set server reg[%1] = %2").arg(address).arg(value));
    }
#else
    Q_UNUSED(address) Q_UNUSED(value)
#endif
}

int ModbusController::getServerRegister(int address) const
{
#ifdef RTU_MODBUS_ENABLED
    if (m_server)
        return m_server->registerValue(address);
#endif
    Q_UNUSED(address)
    return 0;
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

void ModbusController::rebuildServerRegisters()
{
    m_serverRegisters.clear();
#ifdef RTU_MODBUS_ENABLED
    if (!m_server) return;
    auto regs = m_server->allRegisters();
    for (int i = 0; i < static_cast<int>(regs.size()); ++i) {
        if (regs[static_cast<size_t>(i)] == 0) continue;
        QVariantMap row;
        row[QStringLiteral("address")] = i;
        row[QStringLiteral("value")]   = regs[static_cast<size_t>(i)];
        row[QStringLiteral("hex")]     = QStringLiteral("0x%1").arg(regs[static_cast<size_t>(i)], 4, 16, QChar('0')).toUpper();
        m_serverRegisters.append(row);
    }
#endif
    emit serverRegistersChanged();
}

} // namespace rtu::ui
