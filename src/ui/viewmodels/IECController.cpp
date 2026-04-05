#include "IECController.h"

#ifdef RTU_IEC104_ENABLED
#include "core/protocols/iec104/IEC104Client.h"
#include "core/protocols/iec104/IEC104DataPoint.h"
#endif

namespace rtu::ui {

IECController::IECController(QObject* parent)
    : QObject(parent)
{}

IECController::~IECController() { disconnect(); }

// ── Accessors ─────────────────────────────────────────────────

bool         IECController::connected()     const { return m_connected; }
QString      IECController::stateText()     const { return m_stateText; }
QString      IECController::host()          const { return m_host; }
int          IECController::port()          const { return m_port; }
int          IECController::commonAddress() const { return m_commonAddress; }
QString      IECController::lastError()     const { return m_lastError; }
QVariantList IECController::dataPoints()    const { return m_dataPoints; }

// ── Setters ───────────────────────────────────────────────────

void IECController::setHost         (const QString& v) { if (m_host          != v) { m_host          = v; emit configChanged(); } }
void IECController::setPort         (int v)            { if (m_port          != v) { m_port          = v; emit configChanged(); } }
void IECController::setCommonAddress(int v)            { if (m_commonAddress != v) { m_commonAddress = v; emit configChanged(); } }

// ── connect() ─────────────────────────────────────────────────

void IECController::connect()
{
    if (m_connected) { disconnect(); return; }

#ifdef RTU_IEC104_ENABLED
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

    // Immediately activate data transfer
    m_client->sendStartDT();
#else
    setLastError(QStringLiteral("IEC-104 not compiled in (RTU_IEC104_ENABLED not set)"));
    emit errorOccurred(m_lastError);
#endif
}

// ── disconnect() ──────────────────────────────────────────────

void IECController::disconnect()
{
    if (!m_connected) return;

#ifdef RTU_IEC104_ENABLED
    if (m_client) {
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

// ── Session control ───────────────────────────────────────────

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

// ── sendCommand() ─────────────────────────────────────────────

void IECController::sendCommand(const QVariantMap& params)
{
#ifdef RTU_IEC104_ENABLED
    if (!m_client || !m_connected) {
        setLastError(QStringLiteral("Not connected"));
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

// ── clearDataPoints() ─────────────────────────────────────────

void IECController::clearDataPoints()
{
    m_dataPoints.clear();
    emit dataPointsChanged();
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

    // Update existing entry or append
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
    // Mirror IEC104State enum values
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
#endif

void IECController::setLastError(const QString& err) { m_lastError = err; }

} // namespace rtu::ui
