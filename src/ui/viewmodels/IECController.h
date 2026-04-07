#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <memory>

namespace rtu::core::iec104 {
    class IEC104Client;
    class IEC104Server;
}
#ifdef RTU_IEC104_ENABLED
#include "core/protocols/iec104/IEC104DataPoint.h"
#endif

namespace rtu::ui {

// ──────────────────────────────────────────────────────────────
// IECController
//
// QML-facing bridge for IEC 60870-5-104.
// Exposed to QML as context property "IECVM".
//
// role : "Client" | "Server"
//
// Client role: connectDevice() connects to outstation at host:port.
// Server role: connectDevice() starts a CS104_Slave listener on listenPort.
//              IOA values are exposed via serverIOAs.
// ──────────────────────────────────────────────────────────────
class IECController : public QObject {
    Q_OBJECT

    // ── Common ────────────────────────────────────────────────
    Q_PROPERTY(bool    connected   READ connected   NOTIFY connectedChanged)
    Q_PROPERTY(QString role        READ role        WRITE setRole   NOTIFY roleChanged)
    Q_PROPERTY(QString stateText   READ stateText   NOTIFY stateChanged)
    Q_PROPERTY(QString lastError   READ lastError   NOTIFY errorOccurred)

    // ── Client config ─────────────────────────────────────────
    Q_PROPERTY(QString host          READ host          WRITE setHost          NOTIFY configChanged)
    Q_PROPERTY(int     port          READ port          WRITE setPort          NOTIFY configChanged)
    Q_PROPERTY(int     commonAddress READ commonAddress WRITE setCommonAddress NOTIFY configChanged)
    Q_PROPERTY(QVariantList dataPoints READ dataPoints  NOTIFY dataPointsChanged)

    // ── Server config / state ─────────────────────────────────
    Q_PROPERTY(int          listenPort   READ listenPort   WRITE setListenPort NOTIFY configChanged)
    Q_PROPERTY(QVariantList serverIOAs   READ serverIOAs   NOTIFY serverIOAsChanged)
    Q_PROPERTY(int          clientCount  READ clientCount  NOTIFY clientCountChanged)

public:
    explicit IECController(QObject* parent = nullptr);
    ~IECController() override;

    // Common
    bool         connected()     const;
    QString      role()          const;
    QString      stateText()     const;
    QString      lastError()     const;

    // Client
    QString      host()          const;
    int          port()          const;
    int          commonAddress() const;
    QVariantList dataPoints()    const;

    // Server
    int          listenPort()    const;
    QVariantList serverIOAs()    const;
    int          clientCount()   const;

    void setRole         (const QString& v);
    void setHost         (const QString& v);
    void setPort         (int v);
    void setCommonAddress(int v);
    void setListenPort   (int v);

    // ── QML API ───────────────────────────────────────────────

    Q_INVOKABLE void connectDevice();
    Q_INVOKABLE void disconnectDevice();

    // Client operations
    Q_INVOKABLE void sendStartDT();
    Q_INVOKABLE void sendStopDT();
    Q_INVOKABLE void sendInterrogation();
    Q_INVOKABLE void sendCommand(const QVariantMap& params);
    Q_INVOKABLE void clearDataPoints();

    // Server: set/get IOA value
    Q_INVOKABLE void setServerIOA(int ioa, float value);
    Q_INVOKABLE float getServerIOA(int ioa) const;

signals:
    void connectedChanged();
    void roleChanged();
    void stateChanged();
    void configChanged();
    void dataPointsChanged();
    void dataPointReceived(const QVariantMap& point);
    void serverIOAsChanged();
    void clientCountChanged();

    void errorOccurred(const QString& message);

private:
#ifdef RTU_IEC104_ENABLED
    void onDataPoint(const core::iec104::IEC104DataPoint& dp);
    void onStateChanged(int newState);
    void onError(const QString& msg);
    void rebuildServerIOAs();
#endif
    void setLastError(const QString& err);

    // ── Common ────────────────────────────────────────────────
    bool         m_connected     { false };
    QString      m_role          { QStringLiteral("Client") };
    QString      m_stateText     { QStringLiteral("Disconnected") };
    QString      m_lastError;

    // ── Client ────────────────────────────────────────────────
    QString      m_host          { QStringLiteral("192.168.1.100") };
    int          m_port          { 2404 };
    int          m_commonAddress { 1 };
    QVariantList m_dataPoints;

    // ── Server ────────────────────────────────────────────────
    int          m_listenPort    { 2404 };
    int          m_clientCount   { 0 };
    QVariantList m_serverIOAs;

#ifdef RTU_IEC104_ENABLED
    std::unique_ptr<core::iec104::IEC104Client> m_client;
    std::unique_ptr<core::iec104::IEC104Server> m_server;
#endif
};

} // namespace rtu::ui
