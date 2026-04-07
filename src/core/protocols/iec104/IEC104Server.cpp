#include "IEC104Server.h"

#ifdef RTU_IEC104_ENABLED
#include "cs104_slave.h"
#include "cs101_information_objects.h"
#include "hal_time.h"
#endif

#include <QMetaObject>

namespace rtu::core::iec104 {

// ── IEC104Server ──────────────────────────────────────────────

IEC104Server::IEC104Server(QObject* parent)
    : QObject(parent)
{}

IEC104Server::~IEC104Server()
{
    stop();
}

bool IEC104Server::start(int port, int commonAddress)
{
    if (m_running) stop();

    m_port = port;
    m_ca   = commonAddress;

#ifdef RTU_IEC104_ENABLED
    m_slave = CS104_Slave_create(20, 10);
    if (!m_slave) {
        setLastError(QStringLiteral("CS104_Slave_create failed"));
        emit errorOccurred(m_lastError);
        return false;
    }

    CS104_Slave_setLocalPort(m_slave, port);
    CS104_Slave_setServerMode(m_slave, CS104_MODE_CONNECTION_IS_REDUNDANCY_GROUP);
    CS104_Slave_setMaxOpenConnections(m_slave, 4);

    // Register callbacks
    CS104_Slave_setConnectionRequestHandler(m_slave, onConnectionRequest, this);
    // Adapter: lib60870 requires CS104_PeerConnectionEvent parameter type
    CS104_Slave_setConnectionEventHandler(m_slave,
        [](void* p, sIMasterConnection* c, CS104_PeerConnectionEvent ev) {
            IEC104Server::onConnectionEvent(p, c, static_cast<int>(ev));
        }, this);
    CS104_Slave_setInterrogationHandler    (m_slave, onInterrogation,     this);
    CS104_Slave_setASDUHandler             (m_slave, onASDU,              this);

    CS104_Slave_start(m_slave);

    if (!CS104_Slave_isRunning(m_slave)) {
        const QString err = QStringLiteral("CS104_Slave failed to start on port %1").arg(port);
        CS104_Slave_destroy(m_slave);
        m_slave = nullptr;
        setLastError(err);
        emit errorOccurred(err);
        return false;
    }

    m_running = true;
    emit serverStarted(port);
    return true;
#else
    setLastError(QStringLiteral("IEC-104 not compiled in"));
    emit errorOccurred(m_lastError);
    return false;
#endif
}

void IEC104Server::stop()
{
#ifdef RTU_IEC104_ENABLED
    if (m_slave) {
        CS104_Slave_stop(m_slave);
        CS104_Slave_destroy(m_slave);
        m_slave = nullptr;
    }
#endif
    if (m_running) {
        m_running = false;
        emit serverStopped();
    }
}

bool    IEC104Server::isRunning()     const { return m_running; }
int     IEC104Server::listenPort()    const { return m_port; }
int     IEC104Server::commonAddress() const { return m_ca; }
QString IEC104Server::lastError()     const { QMutexLocker lk(&m_mutex); return m_lastError; }

void IEC104Server::setLastError(const QString& err)
{
    QMutexLocker lk(&m_mutex);
    m_lastError = err;
}

QMap<int, QVariant> IEC104Server::allIOAs() const
{
    QMutexLocker lk(&m_mutex);
    return m_ioaValues;
}

// ── IOA update helpers ────────────────────────────────────────

void IEC104Server::updateMeasuredValue(int ioa, float value)
{
    {
        QMutexLocker lk(&m_mutex);
        m_ioaValues[ioa] = value;
    }
    enqueueUpdate(ioa, value, 13 /*M_ME_NC_1 = short float*/);
}

void IEC104Server::updateSinglePoint(int ioa, bool value)
{
    {
        QMutexLocker lk(&m_mutex);
        m_ioaValues[ioa] = value;
    }
    enqueueUpdate(ioa, value, 1 /*M_SP_NA_1*/);
}

void IEC104Server::updateDoublePoint(int ioa, int value)
{
    {
        QMutexLocker lk(&m_mutex);
        m_ioaValues[ioa] = value;
    }
    enqueueUpdate(ioa, value, 3 /*M_DP_NA_1*/);
}

void IEC104Server::enqueueUpdate(int ioa, const QVariant& value, int typeId)
{
#ifdef RTU_IEC104_ENABLED
    if (!m_slave || !m_running) return;

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(m_slave);
    CS101_ASDU asdu = CS101_ASDU_create(alParams, false,
                                         CS101_COT_SPONTANEOUS, 0, m_ca, false, false);

    InformationObject io = nullptr;
    switch (typeId) {
        case 1: // M_SP_NA_1 — single point
            io = (InformationObject) SinglePointInformation_create(
                nullptr, ioa, value.toBool(), IEC60870_QUALITY_GOOD);
            break;
        case 3: // M_DP_NA_1 — double point
            io = (InformationObject) DoublePointInformation_create(
                nullptr, ioa,
                static_cast<DoublePointValue>(value.toInt()),
                IEC60870_QUALITY_GOOD);
            break;
        case 13: // M_ME_NC_1 — short float
        default:
            io = (InformationObject) MeasuredValueShortWithCP56Time2a_create(
                nullptr, ioa, value.toFloat(), IEC60870_QUALITY_GOOD, nullptr);
            // Fall back to scaled if short-float fails (shouldn't)
            if (!io) {
                io = (InformationObject) MeasuredValueScaled_create(
                    nullptr, ioa,
                    static_cast<int16_t>(value.toFloat()),
                    IEC60870_QUALITY_GOOD);
            }
            break;
    }

    if (io) {
        CS101_ASDU_addInformationObject(asdu, io);
        InformationObject_destroy(io);
        CS104_Slave_enqueueASDU(m_slave, asdu);
    }
    CS101_ASDU_destroy(asdu);
#else
    Q_UNUSED(ioa) Q_UNUSED(value) Q_UNUSED(typeId)
#endif
}

// ── Static callbacks ──────────────────────────────────────────

bool IEC104Server::onConnectionRequest(void* param, const char* /*ipAddress*/)
{
    Q_UNUSED(param)
    return true; // accept all
}

void IEC104Server::onConnectionEvent(void* param, sIMasterConnection* /*conn*/, int event)
{
    auto* self = static_cast<IEC104Server*>(param);
    // event 0 = opened, 1 = closed, 2 = startDT, 3 = stopDT
    if (event == 0) {
        QMetaObject::invokeMethod(self, [self]() {
            emit self->clientConnected(QStringLiteral("client"));
        }, Qt::QueuedConnection);
    } else if (event == 1) {
        QMetaObject::invokeMethod(self, [self]() {
            emit self->clientDisconnected(QStringLiteral("client"));
        }, Qt::QueuedConnection);
    }
}

bool IEC104Server::onInterrogation(void* param, sIMasterConnection* conn,
                                    sCS101_ASDU* asdu, uint8_t qoi)
{
    auto* self = static_cast<IEC104Server*>(param);

    if (qoi == 20) { // station interrogation
        QMetaObject::invokeMethod(self, [self]() {
            emit self->interrogationRequested();
        }, Qt::QueuedConnection);

        self->sendInterrogationResponse(conn, asdu);
    }
    return true;
}

bool IEC104Server::onASDU(void* param, sIMasterConnection* conn, sCS101_ASDU* asdu)
{
#ifdef RTU_IEC104_ENABLED
    auto* self = static_cast<IEC104Server*>(param);
    Q_UNUSED(conn)

    int typeId = CS101_ASDU_getTypeID(asdu);
    int numObjects = CS101_ASDU_getNumberOfElements(asdu);

    for (int i = 0; i < numObjects; ++i) {
        InformationObject io = CS101_ASDU_getElement(asdu, i);
        if (!io) continue;

        int ioa = InformationObject_getObjectAddress(io);
        QVariant value;
        QString typeName;

        switch (typeId) {
            case 45: { // C_SC_NA_1 single command
                auto* sc = (SingleCommand) io;
                value    = (bool) SingleCommand_getState(sc);
                typeName = QStringLiteral("C_SC_NA_1");
                break;
            }
            case 46: { // C_DC_NA_1 double command
                auto* dc = (DoubleCommand) io;
                value    = static_cast<int>(DoubleCommand_getState(dc));
                typeName = QStringLiteral("C_DC_NA_1");
                break;
            }
            case 50: { // C_SE_NC_1 setpoint float
                auto* sp = (SetpointCommandShort) io;
                value    = (float) SetpointCommandShort_getValue(sp);
                typeName = QStringLiteral("C_SE_NC_1");
                break;
            }
            default:
                InformationObject_destroy(io);
                continue;
        }

        {
            QMutexLocker lk(&self->m_mutex);
            self->m_ioaValues[ioa] = value;
        }

        QMetaObject::invokeMethod(self, [self, ioa, value, typeName]() {
            emit self->commandReceived(ioa, value, typeName);
        }, Qt::QueuedConnection);

        InformationObject_destroy(io);
    }
#else
    Q_UNUSED(param) Q_UNUSED(conn) Q_UNUSED(asdu)
#endif
    return true;
}

void IEC104Server::sendInterrogationResponse(sIMasterConnection* conn, sCS101_ASDU* refAsdu)
{
#ifdef RTU_IEC104_ENABLED
    CS101_AppLayerParameters alParams = IMasterConnection_getApplicationLayerParameters(conn);

    IMasterConnection_sendACT_CON(conn, refAsdu, false);

    // Send all current IOAs in one or more ASDUs
    QMap<int, QVariant> snapshot;
    {
        QMutexLocker lk(&m_mutex);
        snapshot = m_ioaValues;
    }

    if (snapshot.isEmpty()) {
        // Send at least one dummy measurement so the client knows interrogation completed
        CS101_ASDU asdu = CS101_ASDU_create(alParams, false,
                                             CS101_COT_INTERROGATED_BY_STATION,
                                             0, m_ca, false, false);
        auto* io = (InformationObject) MeasuredValueScaled_create(
            nullptr, 100, 0, IEC60870_QUALITY_GOOD);
        CS101_ASDU_addInformationObject(asdu, io);
        InformationObject_destroy(io);
        IMasterConnection_sendASDU(conn, asdu);
        CS101_ASDU_destroy(asdu);
    } else {
        for (auto it = snapshot.cbegin(); it != snapshot.cend(); ++it) {
            CS101_ASDU asdu = CS101_ASDU_create(alParams, false,
                                                 CS101_COT_INTERROGATED_BY_STATION,
                                                 0, m_ca, false, false);
            // Send as scaled measured value
            int16_t raw = static_cast<int16_t>(it.value().toFloat());
            auto* io = (InformationObject) MeasuredValueScaled_create(
                nullptr, it.key(), raw, IEC60870_QUALITY_GOOD);
            CS101_ASDU_addInformationObject(asdu, io);
            InformationObject_destroy(io);
            IMasterConnection_sendASDU(conn, asdu);
            CS101_ASDU_destroy(asdu);
        }
    }

    IMasterConnection_sendACT_TERM(conn, refAsdu);
#else
    Q_UNUSED(conn) Q_UNUSED(refAsdu)
#endif
}

} // namespace rtu::core::iec104
