#include "SessionViewModel.h"
#include "ModbusController.h"

namespace rtu::ui {

// ──────────────────────────────────────────────────────────────
// SessionViewModel
//
// Thin coordinator that delegates to ModbusController.
// For IEC-104 / Serial, pages bind directly to IECVM / SerialVM.
// ──────────────────────────────────────────────────────────────

SessionViewModel::SessionViewModel(QObject* parent)
    : QObject(parent)
    , m_modbus(new ModbusController(this))
{
    // Propagate ModbusController changes to SessionViewModel properties
    QObject::connect(m_modbus, &ModbusController::connectedChanged,
                     this,     &SessionViewModel::connectionChanged);

    QObject::connect(m_modbus, &ModbusController::responseModelChanged,
                     this,     &SessionViewModel::dataChanged);

    QObject::connect(m_modbus, &ModbusController::rawLogChanged,
                     this,     &SessionViewModel::dataChanged);

    QObject::connect(m_modbus, &ModbusController::errorOccurred,
                     this,     &SessionViewModel::errorOccurred);
}

// ── Property implementations ──────────────────────────────────

QString SessionViewModel::connectionStatus() const
{
    return m_modbus->connected() ? QStringLiteral("connected")
                                 : QStringLiteral("disconnected");
}

bool SessionViewModel::isConnected() const
{
    return m_modbus->connected();
}

bool SessionViewModel::isRunning() const
{
    return m_modbus->connected();
}

QVariantList SessionViewModel::responseModel() const
{
    return m_modbus->responseModel();
}

QStringList SessionViewModel::rawLog() const
{
    return m_modbus->rawLog();
}

// ── Invokable implementations ─────────────────────────────────

void SessionViewModel::connect(const QVariantMap& params)
{
    if (!params.isEmpty()) {
        const QString mode = params.value(QStringLiteral("mode"), QStringLiteral("RTU")).toString();
        m_modbus->setMode(mode);

        if (mode == QLatin1String("TCP")) {
            m_modbus->setHost(params.value(QStringLiteral("host"), QStringLiteral("192.168.1.1")).toString());
            m_modbus->setPort(params.value(QStringLiteral("port"), 502).toInt());
        } else {
            m_modbus->setPortName(params.value(QStringLiteral("portName"), QStringLiteral("COM1")).toString());
            m_modbus->setBaudRate(params.value(QStringLiteral("baudRate"), 9600).toInt());
        }
        m_modbus->setSlaveId(params.value(QStringLiteral("slaveId"), 1).toInt());
    }
    m_modbus->connectDevice();
}

void SessionViewModel::disconnect()
{
    m_modbus->disconnectDevice();
}

void SessionViewModel::sendRequest(const QVariantMap& params)
{
    const int fc      = params.value(QStringLiteral("fc"),          3).toInt();
    const int address = params.value(QStringLiteral("address"),     0).toInt();
    const int count   = params.value(QStringLiteral("count"),      10).toInt();
    const int value   = params.value(QStringLiteral("value"),       0).toInt();

    if (fc == 3 || fc == 4 || fc == 1 || fc == 2)
        m_modbus->read(address, count);
    else if (fc == 6)
        m_modbus->write(address, value);
}

void SessionViewModel::clearLog()
{
    m_modbus->clearLog();
}

} // namespace rtu::ui
