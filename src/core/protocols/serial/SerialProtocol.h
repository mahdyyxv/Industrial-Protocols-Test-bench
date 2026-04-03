#pragma once

#include "core/protocols/IProtocol.h"
#include "SerialManager.h"
#include <QObject>
#include <memory>

namespace rtu::core::serial {

// ──────────────────────────────────────────────────────────────
// SerialProtocol
//
// IProtocol adapter for raw serial communication.
// Implements the protocol contract; delegates all I/O to
// an owned SerialManager instance.
//
// connectionParams keys expected by connect():
//   "port"        QString  e.g. "COM3" / "/dev/ttyUSB0"
//   "baudRate"    int      default 9600
//   "dataBits"    int      5/6/7/8  default 8
//   "parity"      int      QSerialPort::Parity enum  default 0 (None)
//   "stopBits"    int      1/2/3  default 1 (One)
//   "flowControl" int      QSerialPort::FlowControl  default 0 (None)
//   "timeout"     int      read timeout ms  default 1000
// ──────────────────────────────────────────────────────────────
class SerialProtocol final : public QObject, public protocols::IProtocol {
    Q_OBJECT
public:
    explicit SerialProtocol(QObject* parent = nullptr);

    // ── IProtocol ─────────────────────────────────────────────
    protocols::ProtocolType  type()       const override;
    QString                  name()       const override;
    protocols::TransportType transport()  const override;
    bool                     requiresPro() const override;

    bool connect(const QVariantMap& params) override;
    void disconnect()                       override;
    bool isConnected()              const   override;

    ProtocolFrame buildRequest(const QVariantMap& params) const override;
    bool          sendFrame(const ProtocolFrame& frame)         override;
    ProtocolFrame parseResponse(const QByteArray& raw)    const override;

    // ── Extended API (serial-specific) ───────────────────────
    SerialManager*       manager();
    const SerialManager* manager() const;

    static QStringList availablePorts();

signals:
    // Re-emitted from SerialManager for protocol-layer consumers
    void frameReceived(const rtu::core::ProtocolFrame& frame);
    void errorOccurred(const QString& message);
    void connectionChanged(bool connected);

private slots:
    void onDataReceived(const QByteArray& data);
    void onReceiveTimeout();
    void onSerialError(const QString& message, QSerialPort::SerialPortError code);

private:
    SerialManager m_manager;
    QByteArray    m_pendingBuffer;  // accumulates until receiveTimeout
};

} // namespace rtu::core::serial
