# Phase 7 — Device Simulator Module

**Role:** CORE ENGINEER  
**Status:** Complete  
**Date:** 2026-04-04

---

## What Was Implemented

A protocol-agnostic industrial device simulator that maintains Modbus register state and IEC-104 IOA state, executes JSON-driven scenarios, and supports per-address value randomization.

| Component | File | Purpose |
|-----------|------|---------|
| Data models | `src/core/simulator/SimulatorScenario.h` | Action/Step/Scenario types + JSON parser |
| Simulator | `src/core/simulator/DeviceSimulator.h` | Full API |
| Implementation | `src/core/simulator/DeviceSimulator.cpp` | Device state, scenario engine, randomizer |
| Tests | `tests/unit/tst_simulator.cpp` | GoogleTest: ~40 cases |

---

## Design Decisions

### No protocol library dependency

`DeviceSimulator` operates on plain data (register maps, IOA maps). It has no dependency on libmodbus or lib60870-C. Callers wire its signals to protocol clients:

```cpp
// Wire to IEC104Client:
connect(&sim, &DeviceSimulator::ioaChanged,
        [&](int ioa, const QVariant& val) {
            client.sendSetpointFloat(ioa, val.toDouble(), false);
        });

// Wire to ModbusTcpClient (or respond to incoming read requests):
connect(&sim, &DeviceSimulator::registerChanged,
        [&](int addr, uint16_t val) {
            // update your response table
        });
```

### Scenario timing model

Each `SimulatorStep` carries a `delayMs` value — the pause **after** its actions execute before the next step starts. `QTimer::singleShot` is used even for 0 ms delays to defer execution to the next event-loop iteration, preventing call-stack overflow on zero-delay scenario chains.

```
runScenario()
  └── executeStep(0)       ← synchronous
  └── start timer(step[0].delayMs)
        └── onStepTimer()
              └── executeStep(1)
              └── start timer(step[1].delayMs)
                    └── onStepTimer()
                          └── (loop → step 0) or (finish → scenarioFinished)
```

### Randomizer interaction with scenarios

When `enableRandomValues(true)`:
- `SetRegister` actions use `randomRegisterValue(addr)` instead of the JSON literal.
- `SetIOA` actions use `randomIoaValue(addr)` instead of the JSON literal.
- `Randomize` actions always apply random values regardless of this flag (it is designed to randomize explicitly).
- Direct calls to `setRegisterValue()` and `updateIOA()` are **never** intercepted — they always set the exact value passed.

---

## API Summary

### Device control

```cpp
void start();
void stop();
void reset();
bool isRunning() const;
```

`reset()` clears registers and IOAs but keeps the loaded scenario and range configuration.

### Modbus simulation

```cpp
void                setRegisterValue(int address, uint16_t value);
uint16_t            getRegisterValue(int address) const;
QMap<int, uint16_t> allRegisters()               const;
```

### IEC-104 simulation

```cpp
void                sendSpontaneousData();                   // emit all IOAs
void                updateIOA(int ioa, const QVariant& value);
QVariant            getIOA(int ioa)                const;
QMap<int, QVariant> allIOAs()                      const;
```

`sendSpontaneousData()` emits `ioaChanged` for every stored IOA, then emits `spontaneousDataSent`. Wire this to IEC104Client to push unsolicited data.

### Scenario

```cpp
bool              loadScenario(const QString& filePath);
void              runScenario();
void              stopScenario();
bool              isScenarioRunning() const;
SimulatorScenario currentScenario()   const;
QString           lastScenarioError() const;
```

`runScenario()` is a no-op if `isRunning() == false` or no scenario is loaded.

### Randomization

```cpp
void       enableRandomValues(bool enabled);
bool       randomValuesEnabled() const;
void       setRange(int address, double min, double max);
void       setGlobalRange(double min, double max);
ValueRange rangeFor(int address) const;
```

`rangeFor(addr)` returns the per-address range if set, otherwise the global range (default `[0, 100]`).

---

## Scenario JSON Format

```json
{
  "name": "Power Meter RTU",
  "loop": true,
  "steps": [
    {
      "delay_ms": 1000,
      "actions": [
        { "type": "set_register", "address": 0,    "value": 230  },
        { "type": "set_register", "address": 1,    "value": 50   },
        { "type": "set_ioa",      "address": 1001, "value": 1.5  },
        { "type": "send_spontaneous" }
      ]
    },
    {
      "delay_ms": 500,
      "actions": [
        { "type": "randomize" }
      ]
    }
  ]
}
```

| Field | Required | Description |
|-------|----------|-------------|
| `name` | No | Human-readable scenario name |
| `loop` | No | If `true`, restarts after the last step |
| `steps[].delay_ms` | No | Pause (ms) after executing this step's actions |
| `steps[].actions[].type` | Yes | `set_register`, `set_ioa`, `send_spontaneous`, `randomize` |
| `steps[].actions[].address` | For set_* | Register address or IOA |
| `steps[].actions[].value` | For set_* | Target value (number / bool / string) |

Unknown `type` values are silently skipped.

---

## Action Types

| Type | Effect |
|------|--------|
| `set_register` | Sets a Modbus holding register; emits `registerChanged` |
| `set_ioa` | Sets an IEC-104 IOA value; emits `ioaChanged` |
| `send_spontaneous` | Emits `ioaChanged` for all stored IOAs, then `spontaneousDataSent` |
| `randomize` | Applies random values (within configured ranges) to all registers and IOAs |

---

## SimulatorScenario Data Model

```cpp
struct SimulatorAction {
    enum class Type { SetRegister, SetIOA, SendSpontaneous, Randomize };
    Type     type;
    int      address;
    QVariant value;
};

struct SimulatorStep {
    int                      delayMs;
    QVector<SimulatorAction> actions;
};

struct SimulatorScenario {
    QString                name;
    bool                   loop;
    QVector<SimulatorStep> steps;

    bool isEmpty() const;
    static SimulatorScenario fromJson(const QByteArray& json, QString* error = nullptr);
    static SimulatorScenario fromFile(const QString& path,   QString* error = nullptr);
};
```

---

## Signals

```cpp
void started();
void stopped();
void resetDone();

void registerChanged(int address, uint16_t value);
void ioaChanged(int ioa, const QVariant& value);
void spontaneousDataSent();

void scenarioLoaded(const QString& name);
void scenarioStarted();
void scenarioStopped();
void scenarioStepExecuted(int stepIndex);
void scenarioFinished();
```

---

## Test Coverage

`tests/unit/tst_simulator.cpp` — ~40 GoogleTest cases:

| Category | Tests |
|----------|-------|
| JSON parsing | valid JSON, loop flag, delays, action types (SetRegister, SetIOA, SendSpontaneous, Randomize), unknown type skipped, invalid JSON error, root-not-object error, file-not-found error, file load valid |
| Device control | initial state, start/stop, signals, double-start no-op, reset signal |
| Register values | set/get, default zero, signal emitted, overwrite, allRegisters, reset clears |
| IOA values | update/get, default invalid, signal emitted, allIOAs, reset clears |
| Spontaneous data | emits ioaChanged per IOA, final spontaneousDataSent, empty IOA map |
| loadScenario | missing file, invalid JSON, valid file + signal, scenario accessible |
| Scenario execution | requires start, requires loaded scenario, step 0 synchronous, started signal, stop signal, all steps executed, step 1 values, scenarioFinished signal, loop no-finish + multiple loops, send_spontaneous action |
| Randomization | default disabled, enable/disable, global range, per-address override, register values in range, IOA values in range, randomize action updates all |

---

## Integration Example

```cpp
DeviceSimulator sim;

// Configure ranges for randomization
sim.setRange(0,   220.0, 240.0);   // voltage register
sim.setRange(1,   45.0,  55.0);    // frequency register
sim.setRange(1001, 0.0,   1.0);    // single-point IOA

// Load a scenario
sim.loadScenario("scenarios/power_meter.json");

// Wire to IEC104Client
QObject::connect(&sim, &DeviceSimulator::ioaChanged,
    [&](int ioa, const QVariant& val) {
        iec104Client.sendSetpointFloat(ioa, val.toFloat(), false);
    });

// Start
sim.start();
sim.enableRandomValues(true);
sim.runScenario();
```
