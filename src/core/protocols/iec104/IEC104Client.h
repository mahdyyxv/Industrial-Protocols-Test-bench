#pragma once

#include "IEC104DataPoint.h"
#include "core/protocols/IProtocol.h"
#include <QObject>
#include <QString>

// Forward-declare lib60870 handle to keep library headers out of this header.
// CS104_Connection is typedef struct sCS104_Connection* CS104_Connection.
struct sCS104_Connection;

namespace rtu::core::iec104 {

// ──────────────────────────────────────────────────────────────
// IEC104State
// ──────────────────────────────────────────────────────────────
enum class IEC104State {
    Disconnected,
    Connecting,
    Connected,      // TCP connected, data transfer not yet active
    DataTransfer,   // STARTDT confirmed — receiving data
    Error
};

// ──────────────────────────────────────────────────────────────
// IEC104Client
//
// Implements IProtocol so it integrates into the ProtocolRegistry
// and TestEngine like every other protocol.
//
// Also exposes a richer IEC-104–specific API (sendStartDT,
// sendInterrogationCommand, sendSingleCommand, etc.).
//
// All lib60870-C types are confined to the .cpp file.
// This header is completely free of lib60870 dependencies.
// ──────────────────────────────────────────────────────────────
class IEC104Client : public QObject, public protocols::IProtocol {
    Q_OBJECT

public:
    explicit IEC104Client(QObject* parent = nullptr);
    ~IEC104Client() override;

    // ── IProtocol identity ────────────────────────────────────
    protocols::ProtocolType  type()        const override;  // IEC104
    QString                  name()        const override;  // "IEC-104"
    protocols::TransportType transport()   const override;  // TCP
    bool                     requiresPro() const override;  // true

    // ── IProtocol lifecycle ───────────────────────────────────
    // params: "host" (string), "port" (int, default 2404),
    //         "common_addr" (int, default 1),
    //         "response_timeout_ms" (int, default 5000)
    bool connect(const QVariantMap& params) override;
    void disconnect()                       override;
    bool isConnected()               const  override;

    // ── IProtocol frame ops ───────────────────────────────────
    // buildRequest params:
    //   "command": "interrogation" | "single_cmd" | "double_cmd" |
    //              "setpoint_normalized" | "setpoint_scaled" | "setpoint_float" |
    //              "startdt" | "stopdt" | "test_frame" | "clock_sync"
    //   "ioa":          int      — Information Object Address
    //   "value":        QVariant — command value
    //   "common_addr":  int      — ASDU common address (default: configured value)
    //   "select":       bool     — select-before-operate flag (default false)
    ProtocolFrame buildRequest(const QVariantMap& params) const override;
    bool          sendFrame(const ProtocolFrame& frame)         override;
    ProtocolFrame parseResponse(const QByteArray& raw)   const  override;

    // ── Connection ────────────────────────────────────────────
    bool connectToOutstation(const QString& host, int port = 2404);

    IEC104State state()       const;
    QString     lastError()   const;
    bool        isDataTransferActive() const;

    // Per-connection configuration (must be set before connect())
    void setCommonAddress(int addr);
    int  commonAddress() const;
    void setResponseTimeoutMs(int ms);
    int  responseTimeoutMs() const;
    void setOriginatorAddress(int addr);

    // ── Session control ───────────────────────────────────────
    void sendStartDT();           // Activate data transfer (STARTDT act)
    void sendStopDT();            // Deactivate data transfer (STOPDT act)
    void sendTestFrameActivation(); // Send TESTFR act (keepalive)

    // ── Interrogation ─────────────────────────────────────────
    // qoi = 20 → general station interrogation (default)
    // qoi = 21–36 → group interrogation (IEC 60870-5-101 §7.6.4)
    void sendInterrogationCommand(int qoi = 20);
    void sendCounterInterrogationCommand(int qrp = 5);  // request-in-run counters
    void sendClockSynchronization();                     // C_CS_NA_1

    // ── Commands (Control direction) ──────────────────────────
    // select: true = Select-Before-Operate (SBO), false = Direct Execute
    bool sendSingleCommand(int ioa, bool value, bool select = false);
    bool sendDoubleCommand(int ioa, int value, bool select = false);
    bool sendRegulatingStepCommand(int ioa, int direction, bool select = false);
    bool sendSetpointNormalized(int ioa, float value, bool select = false);
    bool sendSetpointScaled(int ioa, int value, bool select = false);
    bool sendSetpointFloat(int ioa, float value, bool select = false);
    // Bitstring (C_BO_NA_1)
    bool sendBitstringCommand(int ioa, uint32_t bits);

    // ── Utilities ─────────────────────────────────────────────
    static QString stateToString(IEC104State s);
    static QString typeIdToString(int typeId);
    // Parse a raw APDU byte array and return a human-readable description
    static QString describeApdu(const QByteArray& raw);

signals:
    // Connection / session
    void stateChanged(IEC104State newState);
    void errorOccurred(const QString& message);
    void dataTransferStarted();
    void dataTransferStopped();

    // Inbound data
    void dataPointReceived(const rtu::core::iec104::IEC104DataPoint& dp);
    void interrogationComplete(int commonAddr);  // C_IC_NA_1 activation confirmation
    void endOfInitialization(int commonAddr);    // M_EI_NA_1 received

    // Command feedback
    void commandConfirmed(int ioa, int typeId);
    void commandNegated(int ioa, int typeId, int cot);

private:
    // Called from static C callbacks (lib60870 types resolved in .cpp)
    bool onAsduReceived(int commonAddr, void* asdu);       // CS101_ASDU in .cpp
    void onConnectionEvent(int event);                     // CS104_ConnectionEvent in .cpp

    // Static C callbacks registered with lib60870
    static bool  staticAsduHandler(void* param, int addr, void* asdu);
    static void  staticConnectionHandler(void* param, void* connection, int event);

    void setState(IEC104State s);
    void handleError(const QString& message);

    // Helpers for buildRequest / sendFrame encoding
    static QByteArray encodeCommand(const QVariantMap& params);
    bool dispatchCommand(const QByteArray& encoded);

    struct sCS104_Connection* m_connection    { nullptr };
    int          m_commonAddr    { 1 };
    int          m_originatorAddr{ 0 };
    int          m_responseTimeoutMs { 5000 };
    IEC104State  m_state         { IEC104State::Disconnected };
    bool         m_dataTransferActive { false };
    QString      m_lastError;
};

} // namespace rtu::core::iec104
