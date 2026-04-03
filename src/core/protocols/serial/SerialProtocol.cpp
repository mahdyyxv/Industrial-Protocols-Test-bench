#include "SerialProtocol.h"
#include "utils/Logger.h"
#include <QDateTime>

namespace rtu::core::serial {

using namespace protocols;

SerialProtocol::SerialProtocol(QObject* parent)
    : QObject(parent)
{
    QObject::connect(&m_manager, &SerialManager::dataReceived,
                     this,       &SerialProtocol::onDataReceived);
    QObject::connect(&m_manager, &SerialManager::receiveTimeout,
                     this,       &SerialProtocol::onReceiveTimeout);
    QObject::connect(&m_manager, &SerialManager::errorOccurred,
                     this,       &SerialProtocol::onSerialError);
    QObject::connect(&m_manager, &SerialManager::connectionChanged,
                     this,       &SerialProtocol::connectionChanged);
}

// ── IProtocol identity ────────────────────────────────────────

ProtocolType  SerialProtocol::type()        const { return ProtocolType::Unknown; }
QString       SerialProtocol::name()        const { return QStringLiteral("Serial"); }
TransportType SerialProtocol::transport()   const { return TransportType::Serial; }
bool          SerialProtocol::requiresPro() const { return false; }

// ── IProtocol lifecycle ───────────────────────────────────────

bool SerialProtocol::connect(const QVariantMap& params)
{
    SerialConfig cfg;
    cfg.portName    = params.value(QStringLiteral("port")).toString();
    cfg.baudRate    = params.value(QStringLiteral("baudRate"),    9600).toInt();
    cfg.dataBits    = static_cast<QSerialPort::DataBits>(
                        params.value(QStringLiteral("dataBits"), 8).toInt());
    cfg.parity      = static_cast<QSerialPort::Parity>(
                        params.value(QStringLiteral("parity"),   0).toInt());
    cfg.stopBits    = static_cast<QSerialPort::StopBits>(
                        params.value(QStringLiteral("stopBits"), 1).toInt());
    cfg.flowControl = static_cast<QSerialPort::FlowControl>(
                        params.value(QStringLiteral("flowControl"), 0).toInt());
    cfg.readTimeoutMs = params.value(QStringLiteral("timeout"), 1000).toInt();

    if (cfg.portName.isEmpty()) {
        emit errorOccurred(QStringLiteral("connect: 'port' parameter is required"));
        return false;
    }

    m_pendingBuffer.clear();
    return m_manager.open(cfg);
}

void SerialProtocol::disconnect()
{
    m_manager.close();
    m_pendingBuffer.clear();
}

bool SerialProtocol::isConnected() const
{
    return m_manager.isConnected();
}

// ── IProtocol frame ops ───────────────────────────────────────

ProtocolFrame SerialProtocol::buildRequest(const QVariantMap& params) const
{
    // Raw serial: the caller provides the bytes directly
    const QByteArray raw = QByteArray::fromHex(
        params.value(QStringLiteral("hex")).toString().toUtf8());

    return ProtocolFrame{
        raw,
        QStringLiteral("Raw serial frame (%1 bytes)").arg(raw.size()),
        QDateTime::currentDateTime(),
        FrameDirection::Sent,
        !raw.isEmpty(),
        raw.isEmpty() ? QStringLiteral("Empty frame") : QString{}
    };
}

bool SerialProtocol::sendFrame(const ProtocolFrame& frame)
{
    if (!frame.valid) {
        emit errorOccurred(QStringLiteral("sendFrame: invalid frame — ")
                           + frame.errorReason);
        return false;
    }
    return m_manager.write(frame.raw);
}

ProtocolFrame SerialProtocol::parseResponse(const QByteArray& raw) const
{
    return ProtocolFrame{
        raw,
        QStringLiteral("Serial response (%1 bytes)").arg(raw.size()),
        QDateTime::currentDateTime(),
        FrameDirection::Received,
        !raw.isEmpty(),
        raw.isEmpty() ? QStringLiteral("Empty response") : QString{}
    };
}

// ── Extended ─────────────────────────────────────────────────

SerialManager*       SerialProtocol::manager()       { return &m_manager; }
const SerialManager* SerialProtocol::manager() const { return &m_manager; }

QStringList SerialProtocol::availablePorts()
{
    return SerialManager::availablePorts();
}

// ── Private slots ─────────────────────────────────────────────

void SerialProtocol::onDataReceived(const QByteArray& data)
{
    m_pendingBuffer = data;   // SerialManager already accumulates
}

void SerialProtocol::onReceiveTimeout()
{
    if (m_pendingBuffer.isEmpty()) return;

    const ProtocolFrame frame = parseResponse(m_pendingBuffer);
    m_pendingBuffer.clear();
    m_manager.clearBuffer();

    emit frameReceived(frame);
}

void SerialProtocol::onSerialError(const QString& message,
                                   QSerialPort::SerialPortError /*code*/)
{
    emit errorOccurred(message);
}

} // namespace rtu::core::serial
