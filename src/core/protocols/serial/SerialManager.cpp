#include "SerialManager.h"
#include "utils/Logger.h"

namespace rtu::core::serial {

SerialManager::SerialManager(QObject* parent)
    : QObject(parent)
{
    // Single-shot timer: fires after silence since last byte received
    m_readTimer.setSingleShot(true);

    connect(&m_port,       &QSerialPort::readyRead,
            this,          &SerialManager::onReadyRead);
    connect(&m_port,       &QSerialPort::errorOccurred,
            this,          &SerialManager::onSerialError);
    connect(&m_readTimer,  &QTimer::timeout,
            this,          &SerialManager::onReadTimeout);
}

SerialManager::~SerialManager()
{
    close();
}

// ── State ─────────────────────────────────────────────────────

bool    SerialManager::isConnected() const { return m_connected; }
QString SerialManager::portName()    const { return m_port.portName(); }
const SerialConfig& SerialManager::config() const { return m_config; }

// ── Port discovery ────────────────────────────────────────────

QStringList SerialManager::availablePorts()
{
    QStringList names;
    for (const auto& info : QSerialPortInfo::availablePorts())
        names << info.portName();
    return names;
}

// ── Lifecycle ─────────────────────────────────────────────────

bool SerialManager::open(const SerialConfig& config)
{
    if (m_connected) {
        qCWarning(rtuCore) << "SerialManager::open called while already connected";
        close();
    }

    m_config = config;
    m_port.setPortName(config.portName);
    m_port.setBaudRate(config.baudRate);
    m_port.setDataBits(config.dataBits);
    m_port.setParity(config.parity);
    m_port.setStopBits(config.stopBits);
    m_port.setFlowControl(config.flowControl);

    if (!m_port.open(QIODevice::ReadWrite)) {
        const QString msg = QStringLiteral("Failed to open %1: %2")
                            .arg(config.portName, m_port.errorString());
        qCWarning(rtuProtocol) << msg;
        emit errorOccurred(msg, m_port.error());
        return false;
    }

    m_port.clear();
    m_rxBuffer.clear();
    m_connected = true;

    qCInfo(rtuProtocol) << "Serial port opened:" << config.portName
                        << "@ " << config.baudRate << "baud";

    emit connectionChanged(true);
    return true;
}

void SerialManager::close()
{
    if (!m_connected) return;

    m_readTimer.stop();

    if (m_port.isOpen())
        m_port.close();

    m_rxBuffer.clear();
    m_connected = false;

    qCInfo(rtuProtocol) << "Serial port closed:" << m_port.portName();
    emit connectionChanged(false);
}

// ── I/O ───────────────────────────────────────────────────────

bool SerialManager::write(const QByteArray& data)
{
    if (!m_connected) {
        emit errorOccurred(QStringLiteral("Write failed: port not open"),
                           QSerialPort::NotOpenError);
        return false;
    }

    if (data.isEmpty()) return true;

    const qint64 written = m_port.write(data);
    if (written != data.size()) {
        const QString msg = QStringLiteral("Write error on %1: %2")
                            .arg(m_port.portName(), m_port.errorString());
        qCWarning(rtuProtocol) << msg;
        emit errorOccurred(msg, m_port.error());
        return false;
    }

    emit dataSent(data);
    return true;
}

void SerialManager::clearBuffer()
{
    m_rxBuffer.clear();
}

QByteArray SerialManager::receiveBuffer() const
{
    return m_rxBuffer;
}

// ── Private slots ─────────────────────────────────────────────

void SerialManager::onReadyRead()
{
    m_rxBuffer.append(m_port.readAll());

    // Restart the silence timer on every new byte arrival
    m_readTimer.start(m_config.readTimeoutMs);

    emit dataReceived(m_rxBuffer);
}

void SerialManager::onReadTimeout()
{
    if (!m_rxBuffer.isEmpty()) {
        qCDebug(rtuProtocol) << "Read timeout —" << m_rxBuffer.size()
                             << "bytes in buffer";
        emit receiveTimeout();
    }
}

void SerialManager::onSerialError(QSerialPort::SerialPortError error)
{
    // NoError fires on open; ignore it
    if (error == QSerialPort::NoError) return;

    const QString msg = QStringLiteral("Serial error on %1: %2")
                        .arg(m_port.portName(), m_port.errorString());
    qCWarning(rtuProtocol) << msg;
    emit errorOccurred(msg, error);

    // Unrecoverable errors → close the port
    if (error == QSerialPort::ResourceError ||
        error == QSerialPort::DeviceNotFoundError) {
        close();
    }
}

} // namespace rtu::core::serial
