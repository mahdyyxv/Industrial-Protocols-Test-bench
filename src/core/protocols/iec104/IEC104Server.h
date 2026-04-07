#pragma once

#include <QObject>
#include <QMutex>
#include <QString>
#include <QVariant>
#include <QMap>
#include <atomic>

// Forward-declare C library types to keep them out of this header
struct sCS104_Slave;
struct sCS101_ASDU;
struct sIMasterConnection;

namespace rtu::core::iec104 {

// ──────────────────────────────────────────────────────────────
// IEC104Server  (outstation / slave)
//
// Wraps lib60870-C CS104_Slave.  Listens on a TCP port for
// control-centre connections.  Supports:
//   • Interrogation responses (sends all IOAs on GI)
//   • Spontaneous updates    (enqueue ASDU when IOA changes)
//   • Single/Double point commands from client
//   • Clock sync
//
// Requires RTU_IEC104_ENABLED compile flag.
// The CS104_Slave runs in its own internal lib60870 threads.
// All Qt signals are emitted via QMetaObject::invokeMethod so they
// are safely delivered on the correct thread.
// ──────────────────────────────────────────────────────────────
class IEC104Server : public QObject {
    Q_OBJECT

public:
    explicit IEC104Server(QObject* parent = nullptr);
    ~IEC104Server() override;

    bool start(int port = 2404, int commonAddress = 1);
    void stop();

    bool    isRunning()     const;
    int     listenPort()    const;
    int     commonAddress() const;
    QString lastError()     const;

    // Update a measured value (float) IOA and send it to all connected masters
    void updateMeasuredValue(int ioa, float value);

    // Update a single-point information IOA
    void updateSinglePoint(int ioa, bool value);

    // Update a double-point information IOA (0=Indet, 1=Off, 2=On, 3=Indet)
    void updateDoublePoint(int ioa, int value);

    // Returns a snapshot of all current IOA values
    QMap<int, QVariant> allIOAs() const;

signals:
    void serverStarted(int port);
    void serverStopped();
    void clientConnected(const QString& clientIp);
    void clientDisconnected(const QString& clientIp);
    void commandReceived(int ioa, const QVariant& value, const QString& type);
    void interrogationRequested();
    void errorOccurred(const QString& message);

private:
    // ── Static callbacks for lib60870-C ──────────────────────
    static bool  onInterrogation(void* param, sIMasterConnection* conn,
                                 sCS101_ASDU* asdu, uint8_t qoi);
    static bool  onASDU         (void* param, sIMasterConnection* conn,
                                 sCS101_ASDU* asdu);
    static void  onConnectionEvent(void* param, sIMasterConnection* conn, int eventCode);
    static bool  onConnectionRequest(void* param, const char* ipAddress);

    void sendInterrogationResponse(sIMasterConnection* conn, sCS101_ASDU* asdu);
    void enqueueUpdate(int ioa, const QVariant& value, int typeId);

    void setLastError(const QString& err);

    mutable QMutex          m_mutex;
    QMap<int, QVariant>     m_ioaValues;   // current IOA state (thread-safe via m_mutex)
    sCS104_Slave*           m_slave    { nullptr };
    int                     m_port     { 2404 };
    int                     m_ca       { 1 };    // common address
    std::atomic<bool>       m_running  { false };
    QString                 m_lastError;
};

} // namespace rtu::core::iec104
