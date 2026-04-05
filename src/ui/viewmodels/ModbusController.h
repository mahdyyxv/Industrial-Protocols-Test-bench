#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <memory>

// Forward declarations keep libmodbus out of this header
namespace rtu::core::modbus {
    class ModbusTcpClient;
    class ModbusRtuClient;
}

namespace rtu::ui {

// ──────────────────────────────────────────────────────────────
// ModbusController
//
// QML-facing bridge for Modbus TCP and RTU.
// Exposed to QML as context property "ModbusVM".
//
// Mode is determined by the "mode" property ("TCP" | "RTU").
// connect() instantiates the matching concrete client.
// ──────────────────────────────────────────────────────────────
class ModbusController : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool    connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString mode      READ mode      WRITE setMode  NOTIFY modeChanged)
    Q_PROPERTY(QString host      READ host      WRITE setHost  NOTIFY configChanged)
    Q_PROPERTY(int     port      READ port      WRITE setPort  NOTIFY configChanged)
    Q_PROPERTY(QString portName  READ portName  WRITE setPortName NOTIFY configChanged)
    Q_PROPERTY(int     baudRate  READ baudRate  WRITE setBaudRate NOTIFY configChanged)
    Q_PROPERTY(int     slaveId   READ slaveId   WRITE setSlaveId  NOTIFY configChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY errorOccurred)
    Q_PROPERTY(QVariantList responseModel READ responseModel NOTIFY responseModelChanged)
    Q_PROPERTY(QStringList  rawLog        READ rawLog        NOTIFY rawLogChanged)

public:
    explicit ModbusController(QObject* parent = nullptr);
    ~ModbusController() override;

    bool         connected()     const;
    QString      mode()          const;
    QString      host()          const;
    int          port()          const;
    QString      portName()      const;
    int          baudRate()      const;
    int          slaveId()       const;
    QString      lastError()     const;
    QVariantList responseModel() const;
    QStringList  rawLog()        const;

    void setMode    (const QString& v);
    void setHost    (const QString& v);
    void setPort    (int v);
    void setPortName(const QString& v);
    void setBaudRate(int v);
    void setSlaveId (int v);

    // ── QML API ───────────────────────────────────────────────

    // Connect to device using current config properties.
    // Switches TCP/RTU based on mode property.
    Q_INVOKABLE void connectDevice();

    // Disconnect and destroy the active client.
    Q_INVOKABLE void disconnectDevice();

    // Read `count` holding registers starting at `address`.
    // Emits readCompleted(values) on success.
    Q_INVOKABLE void read(int address, int count);

    // Write a single holding register.
    // Emits writeCompleted(success) when done.
    Q_INVOKABLE void write(int address, int value);

    // Clear the response table and raw log.
    Q_INVOKABLE void clearLog();

signals:
    void connectedChanged();
    void modeChanged();
    void configChanged();
    void responseModelChanged();
    void rawLogChanged();

    void readCompleted(const QVariantList& values);
    void writeCompleted(bool success);
    void errorOccurred(const QString& message);

private:
    void setConnected(bool v);
    void appendLog(const QString& line);
    void setLastError(const QString& err);
    void destroyClients();

    bool         m_connected  { false };
    QString      m_mode       { QStringLiteral("RTU") };
    QString      m_host       { QStringLiteral("192.168.1.1") };
    int          m_port       { 502 };
    QString      m_portName   { QStringLiteral("COM1") };
    int          m_baudRate   { 9600 };
    int          m_slaveId    { 1 };
    QString      m_lastError;
    QVariantList m_responseModel;
    QStringList  m_rawLog;

    std::unique_ptr<core::modbus::ModbusTcpClient> m_tcpClient;
    std::unique_ptr<core::modbus::ModbusRtuClient> m_rtuClient;
};

} // namespace rtu::ui
