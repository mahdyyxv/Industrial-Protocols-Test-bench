#pragma once

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QString>
#include <QByteArray>
#include <QTimer>

namespace rtu::core::serial {

// ──────────────────────────────────────────────────────────────
// SerialConfig
//
// Plain value type — passed to SerialManager::open().
// No Qt dependencies so it can be constructed from QVariantMap
// in the ViewModel without pulling in QtSerialPort headers.
// ──────────────────────────────────────────────────────────────
struct SerialConfig {
    QString             portName;
    qint32              baudRate    { 9600 };
    QSerialPort::DataBits   dataBits  { QSerialPort::Data8 };
    QSerialPort::Parity     parity    { QSerialPort::NoParity };
    QSerialPort::StopBits   stopBits  { QSerialPort::OneStop };
    QSerialPort::FlowControl flowControl { QSerialPort::NoFlowControl };
    int                 readTimeoutMs { 1000 };   // ms to wait for a full frame
};

// ──────────────────────────────────────────────────────────────
// SerialManager
//
// Thin, stateful wrapper around QSerialPort.
// Responsibilities:
//   - Open / close port
//   - Send raw bytes
//   - Accumulate incoming bytes and emit dataReceived()
//   - Manage read timeout (partial-frame guard)
//   - Surface all errors through errorOccurred()
//
// NOT a protocol — it knows nothing about Modbus, IEC, etc.
// Protocol implementations use SerialManager as their transport.
// ──────────────────────────────────────────────────────────────
class SerialManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectionChanged)
    Q_PROPERTY(QString portName READ portName NOTIFY connectionChanged)

public:
    explicit SerialManager(QObject* parent = nullptr);
    ~SerialManager() override;

    // ── State ─────────────────────────────────────────────────
    bool    isConnected() const;
    QString portName()    const;
    const SerialConfig& config() const;

    // ── Port discovery ────────────────────────────────────────
    static QStringList availablePorts();

    // ── Lifecycle ─────────────────────────────────────────────
    // Returns true if port opened successfully.
    bool open(const SerialConfig& config);
    void close();

    // ── I/O ───────────────────────────────────────────────────
    // Writes bytes to port. Returns false if not connected or
    // the underlying write fails.
    bool write(const QByteArray& data);

    // Clears the internal receive buffer.
    void clearBuffer();

    // Current content of receive buffer (not yet emitted).
    QByteArray receiveBuffer() const;

signals:
    // Emitted after each successful write (echoes the sent bytes).
    void dataSent(const QByteArray& data);

    // Emitted when new bytes arrive. The full accumulated buffer
    // is passed; callers decide when a complete frame has arrived.
    void dataReceived(const QByteArray& data);

    // Emitted when readTimeoutMs passes after the last byte
    // received — signals "end of frame" to protocol layers.
    void receiveTimeout();

    // Emitted on any QSerialPort error.
    void errorOccurred(const QString& message, QSerialPort::SerialPortError code);

    // Emitted on open/close transitions.
    void connectionChanged(bool connected);

private slots:
    void onReadyRead();
    void onSerialError(QSerialPort::SerialPortError error);
    void onReadTimeout();

private:
    QSerialPort  m_port;
    SerialConfig m_config;
    QByteArray   m_rxBuffer;
    QTimer       m_readTimer;   // fires receiveTimeout() after silence
    bool         m_connected { false };
};

} // namespace rtu::core::serial
