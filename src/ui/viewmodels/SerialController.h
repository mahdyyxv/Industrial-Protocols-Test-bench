#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <memory>

namespace rtu::core::serial { class SerialManager; }

namespace rtu::ui {

// ──────────────────────────────────────────────────────────────
// SerialController
//
// QML-facing bridge for raw serial communication.
// Exposed to QML as context property "SerialVM".
// Guarded by RTU_SERIAL_ENABLED at runtime.
// ──────────────────────────────────────────────────────────────
class SerialController : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool       portOpen      READ portOpen      NOTIFY portOpenChanged)
    Q_PROPERTY(QString    portName      READ portName      WRITE setPortName  NOTIFY configChanged)
    Q_PROPERTY(int        baudRate      READ baudRate      WRITE setBaudRate  NOTIFY configChanged)
    Q_PROPERTY(int        dataBits      READ dataBits      WRITE setDataBits  NOTIFY configChanged)
    Q_PROPERTY(QString    parity        READ parity        WRITE setParity    NOTIFY configChanged)
    Q_PROPERTY(int        stopBits      READ stopBits      WRITE setStopBits  NOTIFY configChanged)
    Q_PROPERTY(QString    lastError     READ lastError     NOTIFY errorOccurred)
    Q_PROPERTY(QStringList terminalLines READ terminalLines NOTIFY terminalChanged)

public:
    explicit SerialController(QObject* parent = nullptr);
    ~SerialController() override;

    bool       portOpen()      const;
    QString    portName()      const;
    int        baudRate()      const;
    int        dataBits()      const;
    QString    parity()        const;
    int        stopBits()      const;
    QString    lastError()     const;
    QStringList terminalLines()const;

    void setPortName(const QString& v);
    void setBaudRate(int v);
    void setDataBits(int v);
    void setParity  (const QString& v);
    void setStopBits(int v);

    // ── QML API ───────────────────────────────────────────────

    // Open the serial port with current config. Toggles open/close.
    Q_INVOKABLE void open();

    // Close the serial port explicitly.
    Q_INVOKABLE void close();

    // Send data.  `data` is treated as hex bytes if it looks like
    // "AA BB CC…" (all hex tokens), otherwise sent as ASCII.
    Q_INVOKABLE void send(const QString& data);

    // Clear the terminal log.
    Q_INVOKABLE void clearTerminal();

signals:
    void portOpenChanged();
    void configChanged();
    void terminalChanged();
    void dataReceived(const QString& hex);
    void errorOccurred(const QString& message);

private:
    void appendTerminal(const QString& line);
    void setLastError(const QString& err);

    bool        m_portOpen  { false };
    QString     m_portName  { QStringLiteral("COM1") };
    int         m_baudRate  { 9600 };
    int         m_dataBits  { 8 };
    QString     m_parity    { QStringLiteral("None") };
    int         m_stopBits  { 1 };
    QString     m_lastError;
    QStringList m_terminalLines;

#ifdef RTU_SERIAL_ENABLED
    std::unique_ptr<core::serial::SerialManager> m_serial;
#endif
};

} // namespace rtu::ui
