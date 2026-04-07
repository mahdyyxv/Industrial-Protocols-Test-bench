#include "SerialController.h"

#include <QDateTime>
#include <QRegularExpression>

#ifdef RTU_SERIAL_ENABLED
#include "core/protocols/serial/SerialManager.h"
#endif

namespace rtu::ui {

SerialController::SerialController(QObject* parent)
    : QObject(parent)
{
#ifdef RTU_SERIAL_ENABLED
    m_serial = std::make_unique<rtu::core::serial::SerialManager>(this);

    QObject::connect(m_serial.get(), &rtu::core::serial::SerialManager::dataReceived,
                     this, [this](const QByteArray& data) {
                         const QString hex = QString::fromLatin1(data.toHex(' ')).toUpper();
                         appendTerminal(QStringLiteral("← ") + hex);
                         emit dataReceived(hex);
                     });

    QObject::connect(m_serial.get(), &rtu::core::serial::SerialManager::errorOccurred,
                     this, [this](const QString& msg, int /*code*/) {
                         setLastError(msg);
                         emit errorOccurred(msg);
                     });

    QObject::connect(m_serial.get(), &rtu::core::serial::SerialManager::connectionChanged,
                     this, [this](bool c) {
                         if (m_portOpen != c) {
                             m_portOpen = c;
                             emit portOpenChanged();
                         }
                     });
#endif
}

SerialController::~SerialController() { close(); }

// ── Accessors ─────────────────────────────────────────────────

bool        SerialController::portOpen()       const { return m_portOpen; }
QString     SerialController::role()           const { return m_role; }
QString     SerialController::portName()       const { return m_portName; }
int         SerialController::baudRate()       const { return m_baudRate; }
int         SerialController::dataBits()       const { return m_dataBits; }
QString     SerialController::parity()         const { return m_parity; }
int         SerialController::stopBits()       const { return m_stopBits; }
QString     SerialController::lastError()      const { return m_lastError; }
QStringList SerialController::terminalLines()  const { return m_terminalLines; }

// ── Setters ───────────────────────────────────────────────────

void SerialController::setRole    (const QString& v) { if (m_role     != v) { m_role     = v; emit roleChanged();   } }
void SerialController::setPortName(const QString& v) { if (m_portName != v) { m_portName = v; emit configChanged(); } }
void SerialController::setBaudRate(int v)            { if (m_baudRate != v) { m_baudRate = v; emit configChanged(); } }
void SerialController::setDataBits(int v)            { if (m_dataBits != v) { m_dataBits = v; emit configChanged(); } }
void SerialController::setParity  (const QString& v) { if (m_parity   != v) { m_parity   = v; emit configChanged(); } }
void SerialController::setStopBits(int v)            { if (m_stopBits != v) { m_stopBits = v; emit configChanged(); } }

// ── open() ────────────────────────────────────────────────────

void SerialController::open()
{
    if (m_portOpen) { close(); return; }

#ifdef RTU_SERIAL_ENABLED
    using namespace rtu::core::serial;

    auto parityEnum = QSerialPort::NoParity;
    if      (m_parity == QLatin1String("Even")) parityEnum = QSerialPort::EvenParity;
    else if (m_parity == QLatin1String("Odd"))  parityEnum = QSerialPort::OddParity;
    else if (m_parity == QLatin1String("Mark")) parityEnum = QSerialPort::MarkParity;
    else if (m_parity == QLatin1String("Space"))parityEnum = QSerialPort::SpaceParity;

    auto dataBitsEnum = QSerialPort::Data8;
    switch (m_dataBits) {
        case 5: dataBitsEnum = QSerialPort::Data5; break;
        case 6: dataBitsEnum = QSerialPort::Data6; break;
        case 7: dataBitsEnum = QSerialPort::Data7; break;
        default:dataBitsEnum = QSerialPort::Data8; break;
    }

    auto stopBitsEnum = QSerialPort::OneStop;
    if (m_stopBits == 2) stopBitsEnum = QSerialPort::TwoStop;

    SerialConfig cfg;
    cfg.portName  = m_portName;
    cfg.baudRate  = m_baudRate;
    cfg.dataBits  = dataBitsEnum;
    cfg.parity    = parityEnum;
    cfg.stopBits  = stopBitsEnum;

    if (!m_serial->open(cfg)) {
        setLastError(QStringLiteral("Failed to open %1").arg(m_portName));
        emit errorOccurred(m_lastError);
        return;
    }

    m_portOpen = true;
    emit portOpenChanged();
    appendTerminal(QStringLiteral("Port %1 opened @ %2 baud").arg(m_portName).arg(m_baudRate));
#else
    setLastError(QStringLiteral("Serial support not compiled in (RTU_SERIAL_ENABLED not set)"));
    emit errorOccurred(m_lastError);
#endif
}

// ── close() ───────────────────────────────────────────────────

void SerialController::close()
{
    if (!m_portOpen) return;

#ifdef RTU_SERIAL_ENABLED
    m_serial->close();
#endif

    m_portOpen = false;
    emit portOpenChanged();
    appendTerminal(QStringLiteral("Port closed"));
}

// ── send() ────────────────────────────────────────────────────

void SerialController::send(const QString& data)
{
    if (!m_portOpen) {
        setLastError(QStringLiteral("Port not open"));
        emit errorOccurred(m_lastError);
        return;
    }

    QByteArray bytes;

    // Detect hex input: tokens of 1-2 hex digits separated by spaces
    static const QRegularExpression hexRe(
        QStringLiteral("^(?:[0-9A-Fa-f]{1,2}\\s*)+$"));
    if (hexRe.match(data.trimmed()).hasMatch()) {
        const QStringList tokens = data.trimmed().split(
            QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
        for (const QString& tok : tokens)
            bytes.append(static_cast<char>(tok.toUInt(nullptr, 16)));
    } else {
        bytes = data.toUtf8();
    }

    appendTerminal(QStringLiteral("→ ") + QString::fromLatin1(bytes.toHex(' ')).toUpper());

#ifdef RTU_SERIAL_ENABLED
    if (!m_serial->write(bytes)) {
        setLastError(QStringLiteral("Write failed"));
        emit errorOccurred(m_lastError);
    }
#endif
}

// ── clearTerminal() ───────────────────────────────────────────

void SerialController::clearTerminal()
{
    m_terminalLines.clear();
    emit terminalChanged();
}

// ── Private helpers ───────────────────────────────────────────

void SerialController::appendTerminal(const QString& line)
{
    const QString entry = QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss.zzz"))
                          + QStringLiteral("  ") + line;
    m_terminalLines.append(entry);
    if (m_terminalLines.size() > 500)
        m_terminalLines.removeFirst();
    emit terminalChanged();
}

void SerialController::setLastError(const QString& err) { m_lastError = err; }

} // namespace rtu::ui
