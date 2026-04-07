#include "IECController.h"

#ifdef RTU_IEC104_ENABLED
#include "core/protocols/iec104/IEC104Client.h"
#include "core/protocols/iec104/IEC104DataPoint.h"
#include "core/protocols/iec104/IEC104Server.h"
#endif

namespace rtu::ui {

IECController::IECController(QObject* parent)
    : QObject(parent)
{}

IECController::~IECController() { disconnectDevice(); }

// ── Accessors ─────────────────────────────────────────────────

bool         IECController::connected()     const { return m_connected; }
QString      IECController::role()          const { return m_role; }
QString      IECController::stateText()     const { return m_stateText; }
QString      IECController::host()          const { return m_host; }
int          IECController::port()          const { return m_port; }
int          IECController::commonAddress() const { return m_commonAddress; }
QString      IECController::lastError()     const { return m_lastError; }
QVariantList IECController::dataPoints()    const { return m_dataPoints; }
int          IECController::listenPort()    const { return m_listenPort; }
QVariantList IECController::serverIOAs()    const { return m_serverIOAs; }
int          IECController::clientCount()   const { return m_clientCount; }

// ── Setters ───────────────────────────────────────────────────

void IECController::setRole         (const QString& v) { if (m_role          != v) { m_role          = v; emit roleChanged();    } }
void IECController::setHost         (const QString& v) { if (m_host          != v) { m_host          = v; emit configChanged();  } }
void IECController::setPort         (int v)            { if (m_port          != v) { m_port          = v; emit configChanged();  } }
void IECController::setCommonAddress(int v)            { if (m_commonAddress != v) { m_commonAddress = v; emit configChanged();  } }
void IECController::setListenPort   (int v)            { if (m_listenPort    != v) { m_listenPort    = v; emit configChanged();  } }

// ── connectDevice() ───────────────────────────────────────────

void IECController::connectDevice()
{
    if (m_connected) { disconnectDevice(); return; }

#ifdef RTU_IEC104_ENABLED
    if (m_role == QLatin1String("Server")) {
        // ── Server mode ───────────────────────────────────────
        auto srv = std::make_unique<core::iec104::IEC104Server>(this);

        QObject::connect(srv.get(), &core::iec104::IEC104Server::serverStarted,
                         this, [this](int p) {
            m_connected = true;
            m_stateText = QStringLiteral("Listening on port %1").arg(p);
            emit connectedChanged();
            emit stateChanged();
        });
        QObject::connect(srv.get(), &core::iec104::IEC104Server::serverStopped,
                         this, [this]() {
            m_connected = false;
            m_stateText = QStringLiteral("Stopped");
            emit connectedChanged();
            emit stateChanged();
        });
        QObject::connect(srv.get(), &core::iec104::IEC104Server::clientConnected,
                         this, [this](const QString& ip) {
            m_clientCount++;
            emit clientCountChanged();
            m_stateText = QStringLiteral("Client connected: %1").arg(ip);
            emit stateChanged();
        });
        QObject::connect(srv.get(), &core::iec104::IEC104Server::clientDisconnected,
                         this, [this](const QString&) {
            if (m_clientCount > 0) { m_clientCount--; emit clientCountChanged(); }
        });
        QObject::connect(srv.get(), &core::iec104::IEC104Server::commandReceived,
                         this, [this](int ioa, const QVariant& val, const QString& type) {
            QVariantMap map;
            map[QStringLiteral("ioa")]   = ioa;
            map[QStringLiteral("value")] = val;
            map[QStringLiteral("type")]  = type;
            m_dataPoints.prepend(map);
            if (m_dataPoints.size() > 200) m_dataPoints.removeLast();
            emit dataPointsChanged();
            rebuildServerIOAs();
        });
        QObject::connect(srv.get(), &core::iec104::IEC104Server::errorOccurred,
                         this, [this](const QString& err) {
            setLastError(err);
            emit errorOccurred(err);
        });

        m_server = std::move(srv);
        if (!m_server->start(m_listenPort, m_commonAddress)) {
            setLastError(m_server->lastError());
            emit errorOccurred(m_lastError);
            m_server.reset();
        }

    } else {
        // ── Client mode ───────────────────────────────────────
        m_client = std::make_unique<core::iec104::IEC104Client>(this);
        m_client->setCommonAddress(m_commonAddress);

        QObject::connect(m_client.get(), &core::iec104::IEC104Client::stateChanged,
                         this, [this](core::iec104::IEC104State s) { onStateChanged(static_cast<int>(s)); });
        QObject::connect(m_client.get(), &core::iec104::IEC104Client::errorOccurred,
                         this, [this](const QString& msg) { onError(msg); });
        QObject::connect(m_client.get(), &core::iec104::IEC104Client::dataPointReceived,
                         this, [this](const core::iec104::IEC104DataPoint& dp) { onDataPoint(dp); });
        QObject::connect(m_client.get(), &core::iec104::IEC104Client::dataTransferStarted,
                         this, [this]() {
            m_stateText = QStringLiteral("Data Transfer");
            emit stateChanged();
        });

        if (!m_client->connectToOutstation(m_host, m_port)) {
            setLastError(m_client->lastError());
            emit errorOccurred(m_lastError);
            m_client.reset();
            return;
        }

        m_connected = true;
        m_stateText = QStringLiteral("Connected");
        emit connectedChanged();
        emit stateChanged();
        m_client->sendStartDT();
    }
#else
    setLastError(QStringLiteral("IEC-104 not compiled in (RTU_IEC104_ENABLED not set)"));
    emit errorOccurred(m_lastError);
#endif
}

// ── disconnectDevice() ────────────────────────────────────────

void IECController::disconnectDevice()
{
    if (!m_connected) return;

#ifdef RTU_IEC104_ENABLED
    if (m_server) {
        m_server->stop();
        m_server.reset();
        m_clientCount = 0;
        emit clientCountChanged();
        m_serverIOAs.clear();
        emit serverIOAsChanged();
    } else if (m_client) {
        m_client->sendStopDT();
        m_client->disconnect();
        m_client.reset();
    }
#endif

    m_connected = false;
    m_stateText = QStringLiteral("Disconnected");
    emit connectedChanged();
    emit stateChanged();
}

// ── Client session control ────────────────────────────────────

void IECController::sendStartDT()
{
#ifdef RTU_IEC104_ENABLED
    if (m_client && m_connected) m_client->sendStartDT();
#endif
}

void IECController::sendStopDT()
{
#ifdef RTU_IEC104_ENABLED
    if (m_client && m_connected) m_client->sendStopDT();
#endif
}

void IECController::sendInterrogation()
{
#ifdef RTU_IEC104_ENABLED
    if (m_client && m_connected) m_client->sendInterrogationCommand();
#endif
}

void IECController::sendCommand(const QVariantMap& params)
{
#ifdef RTU_IEC104_ENABLED
    if (!m_client || !m_connected) {
        setLastError(QStringLiteral("Not connected as client"));
        emit errorOccurred(m_lastError);
        return;
    }

    const QString cmd = params.value(QStringLiteral("command")).toString();

    if (cmd == QLatin1String("startdt"))       { m_client->sendStartDT();               return; }
    if (cmd == QLatin1String("stopdt"))        { m_client->sendStopDT();                return; }
    if (cmd == QLatin1String("test_frame"))    { m_client->sendTestFrameActivation();   return; }
    if (cmd == QLatin1String("interrogation")) { m_client->sendInterrogationCommand();  return; }
    if (cmd == QLatin1String("clock_sync"))    { m_client->sendClockSynchronization();  return; }

    if (cmd == QLatin1String("single_cmd")) {
        m_client->sendSingleCommand(
            params.value(QStringLiteral("ioa")).toInt(),
            params.value(QStringLiteral("value")).toBool(),
            params.value(QStringLiteral("select"), false).toBool());
        return;
    }
    if (cmd == QLatin1String("double_cmd")) {
        m_client->sendDoubleCommand(
            params.value(QStringLiteral("ioa")).toInt(),
            params.value(QStringLiteral("value")).toInt(),
            params.value(QStringLiteral("select"), false).toBool());
        return;
    }
    if (cmd == QLatin1String("setpoint_float")) {
        m_client->sendSetpointFloat(
            params.value(QStringLiteral("ioa")).toInt(),
            params.value(QStringLiteral("value")).toFloat(),
            params.value(QStringLiteral("select"), false).toBool());
        return;
    }

    setLastError(QStringLiteral("Unknown command: %1").arg(cmd));
    emit errorOccurred(m_lastError);
#else
    Q_UNUSED(params)
    setLastError(QStringLiteral("IEC-104 not compiled in"));
    emit errorOccurred(m_lastError);
#endif
}

void IECController::clearDataPoints()
{
    m_dataPoints.clear();
    emit dataPointsChanged();
}

// ── Server IOA helpers ────────────────────────────────────────

void IECController::setServerIOA(int ioa, float value)
{
#ifdef RTU_IEC104_ENABLED
    if (m_server) {
        m_server->updateMeasuredValue(ioa, value);
        rebuildServerIOAs();
    }
#else
    Q_UNUSED(ioa) Q_UNUSED(value)
#endif
}

float IECController::getServerIOA(int ioa) const
{
#ifdef RTU_IEC104_ENABLED
    if (m_server)
        return m_server->allIOAs().value(ioa).toFloat();
#endif
    Q_UNUSED(ioa)
    return 0.0f;
}

// ── Private helpers ───────────────────────────────────────────

#ifdef RTU_IEC104_ENABLED
void IECController::onDataPoint(const core::iec104::IEC104DataPoint& dp)
{
    QVariantMap map;
    map[QStringLiteral("ioa")]       = dp.ioa;
    map[QStringLiteral("typeId")]    = static_cast<int>(dp.typeId);
    map[QStringLiteral("typeName")]  = dp.typeName();
    map[QStringLiteral("value")]     = dp.value;
    map[QStringLiteral("quality")]   = dp.quality;
    map[QStringLiteral("cot")]       = dp.cot;
    map[QStringLiteral("timestamp")] = dp.timestamp.toString(Qt::ISODate);

    for (int i = 0; i < m_dataPoints.size(); ++i) {
        if (m_dataPoints[i].toMap().value(QStringLiteral("ioa")).toInt() == dp.ioa) {
            m_dataPoints[i] = map;
            emit dataPointsChanged();
            emit dataPointReceived(map);
            return;
        }
    }
    m_dataPoints.append(map);
    emit dataPointsChanged();
    emit dataPointReceived(map);
}

void IECController::onStateChanged(int newState)
{
    switch (newState) {
        case 0: m_stateText = QStringLiteral("Disconnected"); m_connected = false; emit connectedChanged(); break;
        case 1: m_stateText = QStringLiteral("Connecting");   break;
        case 2: m_stateText = QStringLiteral("Connected");    m_connected = true;  emit connectedChanged(); break;
        case 3: m_stateText = QStringLiteral("Data Transfer");break;
        case 4: m_stateText = QStringLiteral("Error");        break;
        default: m_stateText = QStringLiteral("Unknown");     break;
    }
    emit stateChanged();
}

void IECController::onError(const QString& msg)
{
    setLastError(msg);
    emit errorOccurred(msg);
}

void IECController::rebuildServerIOAs()
{
    m_serverIOAs.clear();
    if (!m_server) return;
    const auto snap = m_server->allIOAs();
    for (auto it = snap.cbegin(); it != snap.cend(); ++it) {
        QVariantMap row;
        row[QStringLiteral("ioa")]   = it.key();
        row[QStringLiteral("value")] = it.value();
        m_serverIOAs.append(row);
    }
    emit serverIOAsChanged();
}
#endif

void IECController::setLastError(const QString& err) { m_lastError = err; }

} // namespace rtu::ui
