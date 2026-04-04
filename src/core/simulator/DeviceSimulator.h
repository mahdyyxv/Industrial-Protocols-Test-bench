#pragma once

#include "SimulatorScenario.h"
#include <QObject>
#include <QMap>
#include <QVariant>
#include <QTimer>

namespace rtu::core::simulator {

// ──────────────────────────────────────────────────────────────
// ValueRange
//
// Inclusive [min, max] used by the randomizer.
// ──────────────────────────────────────────────────────────────
struct ValueRange {
    double min { 0.0 };
    double max { 100.0 };
};

// ──────────────────────────────────────────────────────────────
// DeviceSimulator
//
// Simulates an industrial device that speaks Modbus and/or IEC-104.
//
// Design:
//   - No libmodbus / lib60870 dependency — pure Qt data layer.
//     Callers wire signals to real protocol clients if needed.
//   - Modbus state: a QMap<int, uint16_t> of holding register values.
//   - IEC-104 state: a QMap<int, QVariant> of IOA values.
//   - Scenario: JSON file loaded into SimulatorScenario; steps are
//     executed sequentially using a QTimer (single-shot per step).
//   - Randomizer: per-address ValueRange; global fallback range.
//     When random mode is ON, SetRegister/SetIOA scenario actions
//     use randomised values instead of the JSON-specified literal.
//
// Capture semantics:
//   - start() / stop() gate scenario execution and signal emission.
//   - reset() clears registers and IOAs but keeps the loaded scenario
//     and range configuration intact.
// ──────────────────────────────────────────────────────────────
class DeviceSimulator : public QObject {
    Q_OBJECT

public:
    explicit DeviceSimulator(QObject* parent = nullptr);

    // ── Device control ────────────────────────────────────────
    void start();
    void stop();
    void reset();
    bool isRunning() const;

    // ── Modbus simulation ─────────────────────────────────────
    void                setRegisterValue(int address, uint16_t value);
    uint16_t            getRegisterValue(int address) const;
    QMap<int, uint16_t> allRegisters()               const;

    // ── IEC-104 simulation ────────────────────────────────────

    // Emit ioaChanged for every currently-stored IOA value.
    // Callers can connect this signal directly to IEC104Client.
    void                sendSpontaneousData();

    void                updateIOA(int ioa, const QVariant& value);
    QVariant            getIOA(int ioa)                const;
    QMap<int, QVariant> allIOAs()                      const;

    // ── Scenario ──────────────────────────────────────────────

    // Load a JSON scenario file.  Returns false and sets lastScenarioError()
    // on failure.  Does NOT start execution — call runScenario() separately.
    bool loadScenario(const QString& filePath);

    // Start executing the loaded scenario from step 0.
    // Requires isRunning() == true and a non-empty scenario.
    void runScenario();

    void              stopScenario();
    bool              isScenarioRunning() const;
    SimulatorScenario currentScenario()   const;
    QString           lastScenarioError() const;

    // ── Randomization ─────────────────────────────────────────

    // When enabled, SetRegister/SetIOA scenario actions AND
    // applyRandomToAll() use randomised values from the configured range.
    void       enableRandomValues(bool enabled);
    bool       randomValuesEnabled() const;

    // Per-address range.  Covers both register addresses and IOAs.
    void       setRange(int address, double min, double max);

    // Fallback range used when no per-address range is set.
    void       setGlobalRange(double min, double max);

    ValueRange rangeFor(int address) const;

signals:
    void started();
    void stopped();
    void resetDone();

    // Emitted whenever a register value changes.
    void registerChanged(int address, uint16_t value);

    // Emitted whenever an IOA value changes.
    void ioaChanged(int ioa, const QVariant& value);

    // Emitted after sendSpontaneousData() finishes all ioaChanged emissions.
    void spontaneousDataSent();

    void scenarioLoaded(const QString& name);
    void scenarioStarted();
    void scenarioStopped();
    void scenarioStepExecuted(int stepIndex);
    void scenarioFinished();

private slots:
    void onStepTimer();

private:
    void     executeStep(int index);
    void     applyRandomToAll();
    uint16_t randomRegisterValue(int address) const;
    QVariant randomIoaValue(int address)      const;
    double   randomInRange(const ValueRange&  r) const;
    void     scheduleStep(int delayMs);

    bool                  m_running         { false };
    bool                  m_randomEnabled   { false };
    ValueRange            m_globalRange     { 0.0, 100.0 };

    QMap<int, uint16_t>   m_registers;
    QMap<int, QVariant>   m_ioaValues;
    QMap<int, ValueRange> m_ranges;

    SimulatorScenario     m_scenario;
    QString               m_lastScenarioError;
    bool                  m_scenarioRunning { false };
    int                   m_currentStep     { 0 };
    QTimer*               m_stepTimer;
};

} // namespace rtu::core::simulator
