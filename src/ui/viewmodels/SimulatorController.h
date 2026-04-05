#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <memory>

namespace rtu::core::simulator { class DeviceSimulator; }

namespace rtu::ui {

// ──────────────────────────────────────────────────────────────
// SimulatorController
//
// QML-facing bridge for DeviceSimulator.
// Exposed to QML as context property "SimulatorVM".
// ──────────────────────────────────────────────────────────────
class SimulatorController : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool         running         READ running         NOTIFY runningChanged)
    Q_PROPERTY(bool         scenarioRunning READ scenarioRunning NOTIFY scenarioRunningChanged)
    Q_PROPERTY(bool         randomEnabled   READ randomEnabled   NOTIFY randomEnabledChanged)
    Q_PROPERTY(QString      lastError       READ lastError       NOTIFY errorOccurred)
    Q_PROPERTY(QVariantList registerMap     READ registerMap     NOTIFY registerMapChanged)
    Q_PROPERTY(QVariantList ioaMap          READ ioaMap          NOTIFY ioaMapChanged)

public:
    explicit SimulatorController(QObject* parent = nullptr);
    ~SimulatorController() override;

    bool         running()         const;
    bool         scenarioRunning() const;
    bool         randomEnabled()   const;
    QString      lastError()       const;
    QVariantList registerMap()     const;
    QVariantList ioaMap()          const;

    // ── QML API ───────────────────────────────────────────────

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void reset();

    // Modbus registers
    Q_INVOKABLE void setRegister(int address, int value);
    Q_INVOKABLE int  getRegister(int address) const;
    Q_INVOKABLE void removeRegister(int address);

    // IEC-104 IOAs
    Q_INVOKABLE void    updateIOA(int ioa, const QVariant& value);
    Q_INVOKABLE QVariant getIOA(int ioa) const;

    // Scenario
    Q_INVOKABLE bool loadScenario(const QString& filePath);
    Q_INVOKABLE void runScenario();
    Q_INVOKABLE void stopScenario();

    // Randomization
    Q_INVOKABLE void enableRandom(bool enabled);
    Q_INVOKABLE void setRange(int address, double min, double max);
    Q_INVOKABLE void setGlobalRange(double min, double max);

signals:
    void runningChanged();
    void scenarioRunningChanged();
    void randomEnabledChanged();
    void registerMapChanged();
    void ioaMapChanged();
    void scenarioFinished();
    void errorOccurred(const QString& message);

private:
    void rebuildRegisterMap();
    void rebuildIoaMap();

    QString      m_lastError;
    QVariantList m_registerMap;
    QVariantList m_ioaMap;

    std::unique_ptr<core::simulator::DeviceSimulator> m_sim;
};

} // namespace rtu::ui
