#include "IEC104Client.h"

// lib60870-C headers — confined to this .cpp file
#include "cs104_connection.h"
#include "cs101_information_objects.h"
#include "iec60870_common.h"

#include <QDataStream>
#include <QDateTime>
#include <QIODevice>

namespace rtu::core::iec104 {

using namespace protocols;

// ──────────────────────────────────────────────────────────────
// Static C callback bridges
// lib60870 calls these with a void* parameter which is 'this'.
// ──────────────────────────────────────────────────────────────

bool IEC104Client::staticAsduHandler(void* param, int addr, void* asdu)
{
    return static_cast<IEC104Client*>(param)->onAsduReceived(addr, asdu);
}

void IEC104Client::staticConnectionHandler(void* param, void* /*connection*/, int event)
{
    static_cast<IEC104Client*>(param)
        ->onConnectionEvent(static_cast<int>(event));
}

// ──────────────────────────────────────────────────────────────
// Constructor / Destructor
// ──────────────────────────────────────────────────────────────

IEC104Client::IEC104Client(QObject* parent)
    : QObject(parent)
{
    qRegisterMetaType<rtu::core::iec104::IEC104DataPoint>();
    qRegisterMetaType<rtu::core::iec104::IEC104State>();
}

IEC104Client::~IEC104Client()
{
    IEC104Client::disconnect();
}

// ──────────────────────────────────────────────────────────────
// IProtocol identity
// ──────────────────────────────────────────────────────────────

ProtocolType  IEC104Client::type()        const { return ProtocolType::IEC104; }
QString       IEC104Client::name()        const { return QStringLiteral("IEC-104"); }
TransportType IEC104Client::transport()   const { return TransportType::TCP; }
bool          IEC104Client::requiresPro() const { return true; }

// ──────────────────────────────────────────────────────────────
// IProtocol lifecycle
// ──────────────────────────────────────────────────────────────

bool IEC104Client::connect(const QVariantMap& params)
{
    const QString host = params.value(QStringLiteral("host"),
                                      QStringLiteral("127.0.0.1")).toString();
    const int port = params.value(QStringLiteral("port"), 2404).toInt();
    m_commonAddr        = params.value(QStringLiteral("common_addr"), 1).toInt();
    m_responseTimeoutMs = params.value(QStringLiteral("response_timeout_ms"), 5000).toInt();
    return connectToOutstation(host, port);
}

void IEC104Client::disconnect()
{
    if (m_connection) {
        CS104_Connection_destroy(m_connection);
        m_connection = nullptr;
    }
    m_dataTransferActive = false;
    if (m_state != IEC104State::Disconnected)
        setState(IEC104State::Disconnected);
}

bool IEC104Client::isConnected() const
{
    return m_state == IEC104State::Connected ||
           m_state == IEC104State::DataTransfer;
}

// ──────────────────────────────────────────────────────────────
// IProtocol frame operations
// ──────────────────────────────────────────────────────────────

ProtocolFrame IEC104Client::buildRequest(const QVariantMap& params) const
{
    const QString cmd = params.value(QStringLiteral("command")).toString();
    if (cmd.isEmpty()) {
        return ProtocolFrame{
            {}, QStringLiteral("buildRequest: 'command' parameter required"),
            QDateTime::currentDateTime(), FrameDirection::Sent, false,
            QStringLiteral("missing 'command'")
        };
    }

    const QByteArray encoded = encodeCommand(params);
    const QString desc = QStringLiteral("IEC-104 %1 (IOA=%2, CA=%3)")
        .arg(cmd)
        .arg(params.value(QStringLiteral("ioa"), 0).toInt())
        .arg(params.value(QStringLiteral("common_addr"), m_commonAddr).toInt());

    return ProtocolFrame{
        encoded, desc,
        QDateTime::currentDateTime(), FrameDirection::Sent,
        !encoded.isEmpty(),
        encoded.isEmpty()
            ? QStringLiteral("unknown command '%1'").arg(cmd)
            : QString{}
    };
}

bool IEC104Client::sendFrame(const ProtocolFrame& frame)
{
    if (!frame.valid) {
        emit errorOccurred(QStringLiteral("sendFrame: invalid frame — ") + frame.errorReason);
        return false;
    }
    if (!isConnected()) {
        emit errorOccurred(QStringLiteral("sendFrame: not connected"));
        return false;
    }
    return dispatchCommand(frame.raw);
}

ProtocolFrame IEC104Client::parseResponse(const QByteArray& raw) const
{
    if (raw.size() < 6) {
        return ProtocolFrame{
            raw,
            QStringLiteral("IEC-104: frame too short (%1 bytes)").arg(raw.size()),
            QDateTime::currentDateTime(), FrameDirection::Received,
            false, QStringLiteral("APDU must be at least 6 bytes")
        };
    }

    if (static_cast<uint8_t>(raw[0]) != 0x68) {
        return ProtocolFrame{
            raw,
            QStringLiteral("IEC-104: invalid start byte 0x%1")
                .arg(static_cast<uint8_t>(raw[0]), 2, 16, QChar('0')),
            QDateTime::currentDateTime(), FrameDirection::Received,
            false, QStringLiteral("start byte must be 0x68")
        };
    }

    const uint8_t cf1 = static_cast<uint8_t>(raw[2]);
    QString desc;

    if ((cf1 & 0x01) == 0) {
        // I-frame
        const int txSeq = ((static_cast<uint8_t>(raw[3]) << 7) |
                           (static_cast<uint8_t>(raw[2]) >> 1)) & 0x7FFF;
        const int rxSeq = ((static_cast<uint8_t>(raw[5]) << 7) |
                           (static_cast<uint8_t>(raw[4]) >> 1)) & 0x7FFF;
        desc = QStringLiteral("I-frame: Tx=%1 Rx=%2").arg(txSeq).arg(rxSeq);
    } else if ((cf1 & 0x03) == 0x01) {
        // S-frame
        const int rxSeq = ((static_cast<uint8_t>(raw[5]) << 7) |
                           (static_cast<uint8_t>(raw[4]) >> 1)) & 0x7FFF;
        desc = QStringLiteral("S-frame: Rx=%1").arg(rxSeq);
    } else {
        // U-frame
        if      (cf1 & 0x04) desc = QStringLiteral("U-frame: STARTDT act");
        else if (cf1 & 0x08) desc = QStringLiteral("U-frame: STARTDT con");
        else if (cf1 & 0x10) desc = QStringLiteral("U-frame: STOPDT act");
        else if (cf1 & 0x20) desc = QStringLiteral("U-frame: STOPDT con");
        else if (cf1 & 0x40) desc = QStringLiteral("U-frame: TESTFR act");
        else if (cf1 & 0x80) desc = QStringLiteral("U-frame: TESTFR con");
        else                 desc = QStringLiteral("U-frame: unknown (0x%1)")
                                    .arg(cf1, 2, 16, QChar('0'));
    }

    return ProtocolFrame{
        raw, desc,
        QDateTime::currentDateTime(), FrameDirection::Received, true, {}
    };
}

// ──────────────────────────────────────────────────────────────
// Connection
// ──────────────────────────────────────────────────────────────

bool IEC104Client::connectToOutstation(const QString& host, int port)
{
    if (isConnected()) return true;

    setState(IEC104State::Connecting);

    m_connection = CS104_Connection_create(host.toUtf8().constData(), port);
    if (!m_connection) {
        handleError(QStringLiteral("CS104_Connection_create failed for %1:%2")
                    .arg(host).arg(port));
        return false;
    }

    // Set response timeout via APCI t1 (seconds, rounded up)
    struct sCS104_APCIParameters apci = *CS104_Connection_getAPCIParameters(m_connection);
    apci.t1 = qMax(1, m_responseTimeoutMs / 1000);
    CS104_Connection_setAPCIParameters(m_connection, &apci);

    // Register callbacks (lib60870 uses C function pointers)
    CS104_Connection_setASDUReceivedHandler(
        m_connection,
        reinterpret_cast<CS101_ASDUReceivedHandler>(staticAsduHandler),
        this
    );
    CS104_Connection_setConnectionHandler(
        m_connection,
        reinterpret_cast<CS104_ConnectionHandler>(staticConnectionHandler),
        this
    );

    if (!CS104_Connection_connect(m_connection)) {
        handleError(QStringLiteral("CS104_Connection_connect failed for %1:%2")
                    .arg(host).arg(port));
        CS104_Connection_destroy(m_connection);
        m_connection = nullptr;
        return false;
    }

    setState(IEC104State::Connected);
    return true;
}

IEC104State IEC104Client::state()       const { return m_state; }
QString     IEC104Client::lastError()   const { return m_lastError; }
bool        IEC104Client::isDataTransferActive() const { return m_dataTransferActive; }

void IEC104Client::setCommonAddress(int addr)     { m_commonAddr = addr; }
int  IEC104Client::commonAddress()     const      { return m_commonAddr; }
void IEC104Client::setResponseTimeoutMs(int ms)   { m_responseTimeoutMs = ms; }
int  IEC104Client::responseTimeoutMs() const      { return m_responseTimeoutMs; }
void IEC104Client::setOriginatorAddress(int addr) { m_originatorAddr = addr; }

// ──────────────────────────────────────────────────────────────
// Session control
// ──────────────────────────────────────────────────────────────

void IEC104Client::sendStartDT()
{
    if (!m_connection) { handleError(QStringLiteral("sendStartDT: not connected")); return; }
    CS104_Connection_sendStartDT(m_connection);
}

void IEC104Client::sendStopDT()
{
    if (!m_connection) { handleError(QStringLiteral("sendStopDT: not connected")); return; }
    CS104_Connection_sendStopDT(m_connection);
}

void IEC104Client::sendTestFrameActivation()
{
    if (!m_connection) { handleError(QStringLiteral("sendTestFrameActivation: not connected")); return; }
    // lib60870 v2.3: CS104_Connection_sendTestCommand sends TESTFR act
    CS104_Connection_sendTestCommand(m_connection, m_commonAddr);
}

// ──────────────────────────────────────────────────────────────
// Interrogation
// ──────────────────────────────────────────────────────────────

void IEC104Client::sendInterrogationCommand(int qoi)
{
    if (!m_connection) { handleError(QStringLiteral("sendInterrogationCommand: not connected")); return; }
    CS104_Connection_sendInterrogationCommand(
        m_connection,
        CS101_COT_ACTIVATION,
        m_commonAddr,
        static_cast<QualifierOfInterrogation>(qoi)
    );
}

void IEC104Client::sendCounterInterrogationCommand(int qrp)
{
    if (!m_connection) { handleError(QStringLiteral("sendCounterInterrogationCommand: not connected")); return; }
    CS104_Connection_sendCounterInterrogationCommand(
        m_connection,
        CS101_COT_ACTIVATION,
        m_commonAddr,
        static_cast<uint8_t>(qrp)
    );
}

void IEC104Client::sendClockSynchronization()
{
    if (!m_connection) { handleError(QStringLiteral("sendClockSynchronization: not connected")); return; }
    struct sCP56Time2a time;
    CP56Time2a_createFromMsTimestamp(
        &time,
        static_cast<uint64_t>(QDateTime::currentDateTime().toMSecsSinceEpoch())
    );
    CS104_Connection_sendClockSyncCommand(m_connection, m_commonAddr, &time);
}

// ──────────────────────────────────────────────────────────────
// Commands
// ──────────────────────────────────────────────────────────────

bool IEC104Client::sendSingleCommand(int ioa, bool value, bool select)
{
    if (!m_connection) { handleError(QStringLiteral("sendSingleCommand: not connected")); return false; }
    SingleCommand sc = SingleCommand_create(nullptr, ioa, value, select, 0);
    const bool ok = CS104_Connection_sendProcessCommandEx(
        m_connection, CS101_COT_ACTIVATION, m_commonAddr,
        reinterpret_cast<InformationObject>(sc)
    );
    SingleCommand_destroy(sc);
    if (!ok) handleError(QStringLiteral("sendSingleCommand failed (IOA=%1)").arg(ioa));
    return ok;
}

bool IEC104Client::sendDoubleCommand(int ioa, int value, bool select)
{
    if (!m_connection) { handleError(QStringLiteral("sendDoubleCommand: not connected")); return false; }
    DoubleCommand dc = DoubleCommand_create(nullptr, ioa, value, select, 0);
    const bool ok = CS104_Connection_sendProcessCommandEx(
        m_connection, CS101_COT_ACTIVATION, m_commonAddr,
        reinterpret_cast<InformationObject>(dc)
    );
    DoubleCommand_destroy(dc);
    if (!ok) handleError(QStringLiteral("sendDoubleCommand failed (IOA=%1)").arg(ioa));
    return ok;
}

bool IEC104Client::sendRegulatingStepCommand(int ioa, int direction, bool select)
{
    if (!m_connection) { handleError(QStringLiteral("sendRegulatingStepCommand: not connected")); return false; }
    StepCommand rc = StepCommand_create(
        nullptr, ioa,
        static_cast<StepCommandValue>(direction),
        select, 0
    );
    const bool ok = CS104_Connection_sendProcessCommandEx(
        m_connection, CS101_COT_ACTIVATION, m_commonAddr,
        reinterpret_cast<InformationObject>(rc)
    );
    StepCommand_destroy(rc);
    if (!ok) handleError(QStringLiteral("sendRegulatingStepCommand failed (IOA=%1)").arg(ioa));
    return ok;
}

bool IEC104Client::sendSetpointNormalized(int ioa, float value, bool select)
{
    if (!m_connection) { handleError(QStringLiteral("sendSetpointNormalized: not connected")); return false; }
    SetpointCommandNormalized sp = SetpointCommandNormalized_create(nullptr, ioa, value, select, 0);
    const bool ok = CS104_Connection_sendProcessCommandEx(
        m_connection, CS101_COT_ACTIVATION, m_commonAddr,
        reinterpret_cast<InformationObject>(sp)
    );
    SetpointCommandNormalized_destroy(sp);
    if (!ok) handleError(QStringLiteral("sendSetpointNormalized failed (IOA=%1)").arg(ioa));
    return ok;
}

bool IEC104Client::sendSetpointScaled(int ioa, int value, bool select)
{
    if (!m_connection) { handleError(QStringLiteral("sendSetpointScaled: not connected")); return false; }
    SetpointCommandScaled sp = SetpointCommandScaled_create(nullptr, ioa, value, select, 0);
    const bool ok = CS104_Connection_sendProcessCommandEx(
        m_connection, CS101_COT_ACTIVATION, m_commonAddr,
        reinterpret_cast<InformationObject>(sp)
    );
    SetpointCommandScaled_destroy(sp);
    if (!ok) handleError(QStringLiteral("sendSetpointScaled failed (IOA=%1)").arg(ioa));
    return ok;
}

bool IEC104Client::sendSetpointFloat(int ioa, float value, bool select)
{
    if (!m_connection) { handleError(QStringLiteral("sendSetpointFloat: not connected")); return false; }
    SetpointCommandShort sp = SetpointCommandShort_create(nullptr, ioa, value, select, 0);
    const bool ok = CS104_Connection_sendProcessCommandEx(
        m_connection, CS101_COT_ACTIVATION, m_commonAddr,
        reinterpret_cast<InformationObject>(sp)
    );
    SetpointCommandShort_destroy(sp);
    if (!ok) handleError(QStringLiteral("sendSetpointFloat failed (IOA=%1)").arg(ioa));
    return ok;
}

bool IEC104Client::sendBitstringCommand(int ioa, uint32_t bits)
{
    if (!m_connection) { handleError(QStringLiteral("sendBitstringCommand: not connected")); return false; }
    Bitstring32Command bc = Bitstring32Command_create(nullptr, ioa, bits);
    const bool ok = CS104_Connection_sendProcessCommandEx(
        m_connection, CS101_COT_ACTIVATION, m_commonAddr,
        reinterpret_cast<InformationObject>(bc)
    );
    Bitstring32Command_destroy(bc);
    if (!ok) handleError(QStringLiteral("sendBitstringCommand failed (IOA=%1)").arg(ioa));
    return ok;
}

// ──────────────────────────────────────────────────────────────
// Utilities
// ──────────────────────────────────────────────────────────────

QString IEC104Client::stateToString(IEC104State s)
{
    switch (s) {
        case IEC104State::Disconnected: return QStringLiteral("Disconnected");
        case IEC104State::Connecting:   return QStringLiteral("Connecting");
        case IEC104State::Connected:    return QStringLiteral("Connected");
        case IEC104State::DataTransfer: return QStringLiteral("DataTransfer");
        case IEC104State::Error:        return QStringLiteral("Error");
    }
    return QStringLiteral("Unknown");
}

QString IEC104Client::typeIdToString(int typeId)
{
    IEC104DataPoint dp;
    dp.typeId = static_cast<IEC104TypeId>(typeId);
    return dp.typeName();
}

QString IEC104Client::describeApdu(const QByteArray& raw)
{
    IEC104Client tmp;
    return tmp.parseResponse(raw).description;
}

// ──────────────────────────────────────────────────────────────
// Private: ASDU callback handler
//
// lib60870-C's information object structs use C-style inheritance:
// timed variants (e.g. SinglePointWithCP56Time2a) have the same
// first fields as the base type, so a cast to the base pointer is
// safe for getValue() / getQuality().
// ──────────────────────────────────────────────────────────────

bool IEC104Client::onAsduReceived(int commonAddr, void* asduPtr)
{
    CS101_ASDU asdu          = static_cast<CS101_ASDU>(asduPtr);
    const int  typeIdRaw     = static_cast<int>(CS101_ASDU_getTypeID(asdu));
    const int  elementCount  = CS101_ASDU_getNumberOfElements(asdu);
    const int  cot           = static_cast<int>(CS101_ASDU_getCOT(asdu));

    // System ASDUs (no loop needed)
    if (typeIdRaw == static_cast<int>(IEC104TypeId::M_EI_NA_1)) {
        emit endOfInitialization(commonAddr);
        return true;
    }
    if (typeIdRaw == static_cast<int>(IEC104TypeId::C_IC_NA_1) &&
        cot == CS101_COT_ACTIVATION_CON) {
        emit interrogationComplete(commonAddr);
        return true;
    }

    for (int i = 0; i < elementCount; ++i) {
        InformationObject io = CS101_ASDU_getElement(asdu, i);
        if (!io) continue;

        IEC104DataPoint dp;
        dp.commonAddr   = commonAddr;
        dp.typeId       = static_cast<IEC104TypeId>(typeIdRaw);
        dp.ioa          = InformationObject_getObjectAddress(io);
        dp.cot          = cot;
        dp.timestamp    = QDateTime::currentDateTime();
        dp.hasTimestamp = false;

        switch (static_cast<IEC104TypeId>(typeIdRaw)) {

            // ── Single-point ──────────────────────────────────
            case IEC104TypeId::M_SP_NA_1: {
                auto* obj  = reinterpret_cast<SinglePointInformation>(io);
                dp.value   = QVariant(SinglePointInformation_getValue(obj));
                dp.quality = SinglePointInformation_getQuality(obj);
                break;
            }
            case IEC104TypeId::M_SP_TB_1: {
                // Cast to base — same first fields (C-style inheritance)
                auto* base = reinterpret_cast<SinglePointInformation>(io);
                dp.value   = QVariant(SinglePointInformation_getValue(base));
                dp.quality = SinglePointInformation_getQuality(base);
                auto* obj  = reinterpret_cast<SinglePointWithCP56Time2a>(io);
                CP56Time2a t = SinglePointWithCP56Time2a_getTimestamp(obj);
                dp.timestamp    = QDateTime::fromMSecsSinceEpoch(
                    static_cast<qint64>(CP56Time2a_toMsTimestamp(t)));
                dp.hasTimestamp = true;
                break;
            }

            // ── Double-point ──────────────────────────────────
            case IEC104TypeId::M_DP_NA_1: {
                auto* obj  = reinterpret_cast<DoublePointInformation>(io);
                dp.value   = QVariant(static_cast<int>(DoublePointInformation_getValue(obj)));
                dp.quality = DoublePointInformation_getQuality(obj);
                break;
            }
            case IEC104TypeId::M_DP_TB_1: {
                auto* base = reinterpret_cast<DoublePointInformation>(io);
                dp.value   = QVariant(static_cast<int>(DoublePointInformation_getValue(base)));
                dp.quality = DoublePointInformation_getQuality(base);
                auto* obj  = reinterpret_cast<DoublePointWithCP56Time2a>(io);
                CP56Time2a t = DoublePointWithCP56Time2a_getTimestamp(obj);
                dp.timestamp    = QDateTime::fromMSecsSinceEpoch(
                    static_cast<qint64>(CP56Time2a_toMsTimestamp(t)));
                dp.hasTimestamp = true;
                break;
            }

            // ── Step position ─────────────────────────────────
            case IEC104TypeId::M_ST_NA_1: {
                auto* obj  = reinterpret_cast<StepPositionInformation>(io);
                dp.value   = QVariant(StepPositionInformation_getValue(obj));
                dp.quality = StepPositionInformation_getQuality(obj);
                break;
            }

            // ── Bitstring ─────────────────────────────────────
            case IEC104TypeId::M_BO_NA_1: {
                auto* obj  = reinterpret_cast<BitString32>(io);
                dp.value   = QVariant(static_cast<uint>(BitString32_getValue(obj)));
                dp.quality = BitString32_getQuality(obj);
                break;
            }

            // ── Measured value — normalized ───────────────────
            case IEC104TypeId::M_ME_NA_1: {
                auto* obj  = reinterpret_cast<MeasuredValueNormalized>(io);
                dp.value   = QVariant(static_cast<double>(MeasuredValueNormalized_getValue(obj)));
                dp.quality = MeasuredValueNormalized_getQuality(obj);
                break;
            }
            case IEC104TypeId::M_ME_TD_1: {
                auto* base = reinterpret_cast<MeasuredValueNormalized>(io);
                dp.value   = QVariant(static_cast<double>(MeasuredValueNormalized_getValue(base)));
                dp.quality = MeasuredValueNormalized_getQuality(base);
                auto* obj  = reinterpret_cast<MeasuredValueNormalizedWithCP56Time2a>(io);
                CP56Time2a t = MeasuredValueNormalizedWithCP56Time2a_getTimestamp(obj);
                dp.timestamp    = QDateTime::fromMSecsSinceEpoch(
                    static_cast<qint64>(CP56Time2a_toMsTimestamp(t)));
                dp.hasTimestamp = true;
                break;
            }

            // ── Measured value — scaled ───────────────────────
            case IEC104TypeId::M_ME_NB_1: {
                auto* obj  = reinterpret_cast<MeasuredValueScaled>(io);
                dp.value   = QVariant(MeasuredValueScaled_getValue(obj));
                dp.quality = MeasuredValueScaled_getQuality(obj);
                break;
            }
            case IEC104TypeId::M_ME_TE_1: {
                auto* base = reinterpret_cast<MeasuredValueScaled>(io);
                dp.value   = QVariant(MeasuredValueScaled_getValue(base));
                dp.quality = MeasuredValueScaled_getQuality(base);
                auto* obj  = reinterpret_cast<MeasuredValueScaledWithCP56Time2a>(io);
                CP56Time2a t = MeasuredValueScaledWithCP56Time2a_getTimestamp(obj);
                dp.timestamp    = QDateTime::fromMSecsSinceEpoch(
                    static_cast<qint64>(CP56Time2a_toMsTimestamp(t)));
                dp.hasTimestamp = true;
                break;
            }

            // ── Measured value — short float ──────────────────
            case IEC104TypeId::M_ME_NC_1: {
                auto* obj  = reinterpret_cast<MeasuredValueShort>(io);
                dp.value   = QVariant(static_cast<double>(MeasuredValueShort_getValue(obj)));
                dp.quality = MeasuredValueShort_getQuality(obj);
                break;
            }
            case IEC104TypeId::M_ME_TF_1: {
                auto* base = reinterpret_cast<MeasuredValueShort>(io);
                dp.value   = QVariant(static_cast<double>(MeasuredValueShort_getValue(base)));
                dp.quality = MeasuredValueShort_getQuality(base);
                auto* obj  = reinterpret_cast<MeasuredValueShortWithCP56Time2a>(io);
                CP56Time2a t = MeasuredValueShortWithCP56Time2a_getTimestamp(obj);
                dp.timestamp    = QDateTime::fromMSecsSinceEpoch(
                    static_cast<qint64>(CP56Time2a_toMsTimestamp(t)));
                dp.hasTimestamp = true;
                break;
            }

            // ── Integrated totals ─────────────────────────────
            case IEC104TypeId::M_IT_NA_1: {
                auto* obj = reinterpret_cast<IntegratedTotals>(io);
                BinaryCounterReading bcr = IntegratedTotals_getBCR(obj);
                dp.value = QVariant(BinaryCounterReading_getValue(bcr));
                break;
            }

            // ── Command confirmations / negations ─────────────
            case IEC104TypeId::C_SC_NA_1:
            case IEC104TypeId::C_DC_NA_1:
            case IEC104TypeId::C_RC_NA_1:
            case IEC104TypeId::C_SE_NA_1:
            case IEC104TypeId::C_SE_NB_1:
            case IEC104TypeId::C_SE_NC_1: {
                if (cot == CS101_COT_ACTIVATION_CON)
                    emit commandConfirmed(dp.ioa, typeIdRaw);
                else if (cot == CS101_COT_ACTIVATION_TERMINATION)
                    emit commandNegated(dp.ioa, typeIdRaw, cot);
                InformationObject_destroy(io);
                continue;
            }

            default:
                // Unknown/unsupported — emit with null value for tracing
                break;
        }

        InformationObject_destroy(io);
        emit dataPointReceived(dp);
    }

    return true;
}

void IEC104Client::onConnectionEvent(int eventInt)
{
    // CS104_ConnectionEvent values (cs104_connection.h):
    //   CS104_CONNECTION_OPENED            = 0
    //   CS104_CONNECTION_CLOSED            = 1
    //   CS104_CONNECTION_STARTDT_CON_RECEIVED = 2
    //   CS104_CONNECTION_STOPDT_CON_RECEIVED  = 3
    switch (eventInt) {
        case CS104_CONNECTION_OPENED:
            setState(IEC104State::Connected);
            break;
        case CS104_CONNECTION_CLOSED:
            m_dataTransferActive = false;
            setState(IEC104State::Disconnected);
            break;
        case CS104_CONNECTION_STARTDT_CON_RECEIVED:
            m_dataTransferActive = true;
            setState(IEC104State::DataTransfer);
            emit dataTransferStarted();
            break;
        case CS104_CONNECTION_STOPDT_CON_RECEIVED:
            m_dataTransferActive = false;
            setState(IEC104State::Connected);
            emit dataTransferStopped();
            break;
        default:
            break;
    }
}

// ──────────────────────────────────────────────────────────────
// Private helpers
// ──────────────────────────────────────────────────────────────

void IEC104Client::setState(IEC104State s)
{
    if (m_state == s) return;
    m_state = s;
    emit stateChanged(m_state);
}

void IEC104Client::handleError(const QString& message)
{
    m_lastError = message;
    setState(IEC104State::Error);
    emit errorOccurred(message);
}

// Command encoding: 1-byte tag + payload via QDataStream
QByteArray IEC104Client::encodeCommand(const QVariantMap& params)
{
    const QString cmd = params.value(QStringLiteral("command")).toString();

    QByteArray buf;
    QDataStream ds(&buf, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::LittleEndian);
    ds.setFloatingPointPrecision(QDataStream::SinglePrecision);

    if      (cmd == QStringLiteral("startdt"))             { ds << uint8_t(0x01); }
    else if (cmd == QStringLiteral("stopdt"))              { ds << uint8_t(0x02); }
    else if (cmd == QStringLiteral("test_frame"))          { ds << uint8_t(0x03); }
    else if (cmd == QStringLiteral("interrogation"))       {
        ds << uint8_t(0x04)
           << int32_t(params.value(QStringLiteral("qoi"), 20).toInt());
    }
    else if (cmd == QStringLiteral("single_cmd"))          {
        ds << uint8_t(0x10)
           << int32_t(params.value(QStringLiteral("ioa"),    0).toInt())
           << uint8_t(params.value(QStringLiteral("value"),  false).toBool() ? 1 : 0)
           << uint8_t(params.value(QStringLiteral("select"), false).toBool() ? 1 : 0);
    }
    else if (cmd == QStringLiteral("double_cmd"))          {
        ds << uint8_t(0x11)
           << int32_t(params.value(QStringLiteral("ioa"),    0).toInt())
           << int32_t(params.value(QStringLiteral("value"),  0).toInt())
           << uint8_t(params.value(QStringLiteral("select"), false).toBool() ? 1 : 0);
    }
    else if (cmd == QStringLiteral("setpoint_normalized")) {
        ds << uint8_t(0x12)
           << int32_t(params.value(QStringLiteral("ioa"),    0).toInt())
           << float(  params.value(QStringLiteral("value"),  0.0f).toFloat())
           << uint8_t(params.value(QStringLiteral("select"), false).toBool() ? 1 : 0);
    }
    else if (cmd == QStringLiteral("setpoint_scaled"))     {
        ds << uint8_t(0x13)
           << int32_t(params.value(QStringLiteral("ioa"),    0).toInt())
           << int32_t(params.value(QStringLiteral("value"),  0).toInt())
           << uint8_t(params.value(QStringLiteral("select"), false).toBool() ? 1 : 0);
    }
    else if (cmd == QStringLiteral("setpoint_float"))      {
        ds << uint8_t(0x14)
           << int32_t(params.value(QStringLiteral("ioa"),    0).toInt())
           << float(  params.value(QStringLiteral("value"),  0.0f).toFloat())
           << uint8_t(params.value(QStringLiteral("select"), false).toBool() ? 1 : 0);
    }
    else if (cmd == QStringLiteral("clock_sync"))          { ds << uint8_t(0x20); }
    // Unknown command → empty buffer → caller marks frame invalid

    return buf;
}

bool IEC104Client::dispatchCommand(const QByteArray& encoded)
{
    if (encoded.isEmpty()) return false;

    QDataStream ds(encoded);
    ds.setByteOrder(QDataStream::LittleEndian);
    ds.setFloatingPointPrecision(QDataStream::SinglePrecision);

    uint8_t tag = 0;
    ds >> tag;

    switch (tag) {
        case 0x01: sendStartDT();             return true;
        case 0x02: sendStopDT();              return true;
        case 0x03: sendTestFrameActivation(); return true;
        case 0x04: {
            int32_t qoi = 20; ds >> qoi;
            sendInterrogationCommand(qoi);
            return true;
        }
        case 0x10: {
            int32_t ioa = 0; uint8_t val = 0, sel = 0;
            ds >> ioa >> val >> sel;
            return sendSingleCommand(ioa, val != 0, sel != 0);
        }
        case 0x11: {
            int32_t ioa = 0, val = 0; uint8_t sel = 0;
            ds >> ioa >> val >> sel;
            return sendDoubleCommand(ioa, val, sel != 0);
        }
        case 0x12: {
            int32_t ioa = 0; float val = 0.0f; uint8_t sel = 0;
            ds >> ioa >> val >> sel;
            return sendSetpointNormalized(ioa, val, sel != 0);
        }
        case 0x13: {
            int32_t ioa = 0, val = 0; uint8_t sel = 0;
            ds >> ioa >> val >> sel;
            return sendSetpointScaled(ioa, val, sel != 0);
        }
        case 0x14: {
            int32_t ioa = 0; float val = 0.0f; uint8_t sel = 0;
            ds >> ioa >> val >> sel;
            return sendSetpointFloat(ioa, val, sel != 0);
        }
        case 0x20: sendClockSynchronization(); return true;
        default:   return false;
    }
}

} // namespace rtu::core::iec104
