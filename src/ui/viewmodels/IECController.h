#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <memory>

namespace rtu::core::iec104 { class IEC104Client; }
#ifdef RTU_IEC104_ENABLED
#include "core/protocols/iec104/IEC104DataPoint.h"
#endif

namespace rtu::ui {

// ──────────────────────────────────────────────────────────────
// IECController
//
// QML-facing bridge for IEC 60870-5-104.
// Exposed to QML as context property "IECVM".
// Guarded by RTU_IEC104_ENABLED at runtime (graceful fallback).
// ──────────────────────────────────────────────────────────────
class IECController : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool    connected      READ connected      NOTIFY connectedChanged)
    Q_PROPERTY(QString stateText      READ stateText      NOTIFY stateChanged)
    Q_PROPERTY(QString host           READ host           WRITE setHost          NOTIFY configChanged)
    Q_PROPERTY(int     port           READ port           WRITE setPort          NOTIFY configChanged)
    Q_PROPERTY(int     commonAddress  READ commonAddress  WRITE setCommonAddress NOTIFY configChanged)
    Q_PROPERTY(QString lastError      READ lastError      NOTIFY errorOccurred)
    Q_PROPERTY(QVariantList dataPoints READ dataPoints    NOTIFY dataPointsChanged)

public:
    explicit IECController(QObject* parent = nullptr);
    ~IECController() override;

    bool         connected()     const;
    QString      stateText()     const;
    QString      host()          const;
    int          port()          const;
    int          commonAddress() const;
    QString      lastError()     const;
    QVariantList dataPoints()    const;

    void setHost         (const QString& v);
    void setPort         (int v);
    void setCommonAddress(int v);

    // ── QML API ───────────────────────────────────────────────

    // Connect to outstation at current host:port. Starts data transfer.
    Q_INVOKABLE void connectDevice();

    // Disconnect from outstation.
    Q_INVOKABLE void disconnectDevice();

    // Send STARTDT activation.
    Q_INVOKABLE void sendStartDT();

    // Send STOPDT activation.
    Q_INVOKABLE void sendStopDT();

    // Send C_IC_NA_1 interrogation (qoi=20 → all stations).
    Q_INVOKABLE void sendInterrogation();

    // Send a command described by a QVariantMap.
    // Supported "command" keys (mirror IEC104Client::buildRequest):
    //   "startdt", "stopdt", "test_frame", "interrogation",
    //   "single_cmd", "double_cmd", "setpoint_float", "clock_sync"
    Q_INVOKABLE void sendCommand(const QVariantMap& params);

    // Clear the data point list.
    Q_INVOKABLE void clearDataPoints();

signals:
    void connectedChanged();
    void stateChanged();
    void configChanged();
    void dataPointsChanged();
    void dataPointReceived(const QVariantMap& point);

    void errorOccurred(const QString& message);

private:
#ifdef RTU_IEC104_ENABLED
    void onDataPoint(const core::iec104::IEC104DataPoint& dp);
    void onStateChanged(int newState);
    void onError(const QString& msg);
#endif
    void setLastError(const QString& err);

    bool         m_connected     { false };
    QString      m_stateText     { QStringLiteral("Disconnected") };
    QString      m_host          { QStringLiteral("192.168.1.100") };
    int          m_port          { 2404 };
    int          m_commonAddress { 1 };
    QString      m_lastError;
    QVariantList m_dataPoints;

#ifdef RTU_IEC104_ENABLED
    std::unique_ptr<core::iec104::IEC104Client> m_client;
#endif
};

} // namespace rtu::ui
