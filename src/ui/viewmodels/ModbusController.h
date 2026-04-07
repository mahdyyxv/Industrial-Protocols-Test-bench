#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <memory>

#ifdef RTU_MODBUS_ENABLED
namespace rtu::core::modbus {
    class ModbusTcpClient;
    class ModbusRtuClient;
    class ModbusTcpServer;
}
#endif

namespace rtu::ui {

// ──────────────────────────────────────────────────────────────
// ModbusController
//
// QML bridge for Modbus TCP and RTU — supports Client and Server roles.
//
// role     : "Client" | "Server"
// mode     : "TCP"    | "RTU"      (TCP ignored for Server RTU)
//
// Client role: connectDevice() creates a TCP/RTU client and connects.
// Server role: connectDevice() starts a TCP listener on listenPort.
//              The server register bank is exposed via serverRegisters.
// ──────────────────────────────────────────────────────────────
class ModbusController : public QObject {
    Q_OBJECT

    // ── Common ────────────────────────────────────────────────
    Q_PROPERTY(bool    connected  READ connected  NOTIFY connectedChanged)
    Q_PROPERTY(QString role       READ role       WRITE setRole   NOTIFY roleChanged)
    Q_PROPERTY(QString mode       READ mode       WRITE setMode   NOTIFY modeChanged)
    Q_PROPERTY(QString lastError  READ lastError  NOTIFY errorOccurred)
    Q_PROPERTY(QStringList rawLog READ rawLog     NOTIFY rawLogChanged)

    // ── Client config ─────────────────────────────────────────
    Q_PROPERTY(QString host     READ host     WRITE setHost     NOTIFY configChanged)
    Q_PROPERTY(int     port     READ port     WRITE setPort     NOTIFY configChanged)
    Q_PROPERTY(QString portName READ portName WRITE setPortName NOTIFY configChanged)
    Q_PROPERTY(int     baudRate READ baudRate WRITE setBaudRate NOTIFY configChanged)
    Q_PROPERTY(int     slaveId  READ slaveId  WRITE setSlaveId  NOTIFY configChanged)

    // ── Client read results ───────────────────────────────────
    Q_PROPERTY(QVariantList responseModel READ responseModel NOTIFY responseModelChanged)

    // ── Server config / state ─────────────────────────────────
    Q_PROPERTY(int          listenPort      READ listenPort      WRITE setListenPort   NOTIFY configChanged)
    Q_PROPERTY(QVariantList serverRegisters READ serverRegisters NOTIFY serverRegistersChanged)
    Q_PROPERTY(int          clientCount     READ clientCount     NOTIFY clientCountChanged)

public:
    explicit ModbusController(QObject* parent = nullptr);
    ~ModbusController() override;

    // Common
    bool         connected()      const;
    QString      role()           const;
    QString      mode()           const;
    QString      lastError()      const;
    QStringList  rawLog()         const;

    // Client
    QString      host()           const;
    int          port()           const;
    QString      portName()       const;
    int          baudRate()       const;
    int          slaveId()        const;
    QVariantList responseModel()  const;

    // Server
    int          listenPort()     const;
    QVariantList serverRegisters() const;
    int          clientCount()    const;

    void setRole    (const QString& v);
    void setMode    (const QString& v);
    void setHost    (const QString& v);
    void setPort    (int v);
    void setPortName(const QString& v);
    void setBaudRate(int v);
    void setSlaveId (int v);
    void setListenPort(int v);

    // ── QML API ───────────────────────────────────────────────

    // Client: connect / disconnect.  Server: start / stop listener.
    Q_INVOKABLE void connectDevice();
    Q_INVOKABLE void disconnectDevice();

    // Client read/write
    Q_INVOKABLE void read (int address, int count);
    Q_INVOKABLE void write(int address, int value);

    // Server: update a register in the bank (client reads it)
    Q_INVOKABLE void setServerRegister(int address, int value);
    Q_INVOKABLE int  getServerRegister(int address) const;

    Q_INVOKABLE void clearLog();

signals:
    void connectedChanged();
    void roleChanged();
    void modeChanged();
    void configChanged();
    void responseModelChanged();
    void rawLogChanged();
    void serverRegistersChanged();
    void clientCountChanged();

    void readCompleted  (const QVariantList& values);
    void writeCompleted (bool success);
    void errorOccurred  (const QString& message);

private:
    void setConnected(bool v);
    void appendLog(const QString& line);
    void setLastError(const QString& err);
    void destroyClients();
    void rebuildServerRegisters();

    // ── Common ────────────────────────────────────────────────
    bool         m_connected   { false };
    QString      m_role        { QStringLiteral("Client") };
    QString      m_mode        { QStringLiteral("RTU") };
    QString      m_lastError;
    QStringList  m_rawLog;

    // ── Client ────────────────────────────────────────────────
    QString      m_host        { QStringLiteral("192.168.1.1") };
    int          m_port        { 502 };
    QString      m_portName    { QStringLiteral("COM1") };
    int          m_baudRate    { 9600 };
    int          m_slaveId     { 1 };
    QVariantList m_responseModel;

#ifdef RTU_MODBUS_ENABLED
    std::unique_ptr<core::modbus::ModbusTcpClient> m_tcpClient;
    std::unique_ptr<core::modbus::ModbusRtuClient> m_rtuClient;
#endif

    // ── Server ────────────────────────────────────────────────
    int          m_listenPort  { 502 };
    int          m_clientCount { 0 };
    QVariantList m_serverRegisters;

#ifdef RTU_MODBUS_ENABLED
    std::unique_ptr<core::modbus::ModbusTcpServer> m_server;
#endif
};

} // namespace rtu::ui
