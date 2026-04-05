# Phase 8 — UI ↔ Core Integration

## Overview

Phase 8 wires the QML UI pages built in Phase 2 to the core protocol engines built in Phases 3–7.
Each protocol gains a dedicated **Controller** (`QObject` subclass) that:

- exposes Qt properties consumed directly by QML (`READ`/`WRITE`/`NOTIFY`)
- provides `Q_INVOKABLE` methods for button callbacks
- delegates real work to the corresponding core engine
- emits `errorOccurred(message)` for graceful failure reporting in QML

---

## New Files

| File | Role |
|------|------|
| `src/ui/viewmodels/ModbusController.h/.cpp`    | Bridge for Modbus TCP/RTU |
| `src/ui/viewmodels/SerialController.h/.cpp`    | Bridge for raw serial port |
| `src/ui/viewmodels/AnalyzerController.h/.cpp`  | Bridge for packet analyzer |
| `src/ui/viewmodels/SimulatorController.h/.cpp` | Bridge for device simulator |
| `src/ui/viewmodels/IECController.h/.cpp`       | Bridge for IEC 60870-5-104 |
| `tests/unit/tst_integration.cpp`               | GoogleTest suite for all controllers |

---

## Modified Files

| File | Change |
|------|--------|
| `src/ui/viewmodels/SessionViewModel.cpp` | Delegates to `ModbusController` |
| `src/app/Application.cpp`               | Registers all controllers as QML context properties |
| `src/ui/CMakeLists.txt`                 | Adds new controller sources; links `rtu_core` |
| `tests/unit/CMakeLists.txt`             | Adds `tst_integration` target |
| `src/ui/qml/pages/ModbusPage.qml`       | Wired to `ModbusVM` |
| `src/ui/qml/pages/SerialPage.qml`       | Wired to `SerialVM` |
| `src/ui/qml/pages/AnalyzerPage.qml`     | Wired to `AnalyzerVM` |
| `src/ui/qml/pages/SimulatorPage.qml`    | Wired to `SimulatorVM` |
| `src/ui/qml/pages/IEC104Page.qml`       | Wired to `IECVM` |

---

## QML Context Properties

Registered in `Application::registerQmlTypes()`:

| QML name       | C++ type              | Feature |
|----------------|-----------------------|---------|
| `AppVM`        | `AppViewModel`        | Auth, subscription tier |
| `ModbusVM`     | `ModbusController`    | Modbus TCP / RTU |
| `IECVM`        | `IECController`       | IEC 60870-5-104 |
| `SerialVM`     | `SerialController`    | Raw serial terminal |
| `AnalyzerVM`   | `AnalyzerController`  | Packet capture / decode |
| `SimulatorVM`  | `SimulatorController` | Device register simulator |

All controllers are parented to `m_engine` and live for the application lifetime.

---

## Controller API Summary

### ModbusController (`ModbusVM`)

| Property | Type | Notes |
|----------|------|-------|
| `connected` | `bool` | Read-only |
| `mode` | `string` | `"RTU"` or `"TCP"` |
| `host` | `string` | TCP target |
| `port` | `int` | default 502 |
| `portName` | `string` | RTU port |
| `baudRate` | `int` | RTU baud |
| `slaveId` | `int` | Modbus unit ID |
| `lastError` | `string` | Last error text |
| `responseModel` | `var` (list) | Decoded register table |
| `rawLog` | `list<string>` | Hex log |

Invokables: `connect()`, `disconnect()`, `read(address, count)`, `write(address, value)`, `clearLog()`

Signals: `readCompleted(values)`, `writeCompleted(success)`, `errorOccurred(msg)`

---

### SerialController (`SerialVM`)

| Property | Type | Notes |
|----------|------|-------|
| `portOpen` | `bool` | Read-only |
| `portName` | `string` | e.g. `COM3`, `/dev/ttyUSB0` |
| `baudRate` | `int` | default 9600 |
| `dataBits` | `int` | default 8 |
| `parity` | `string` | `"None"`, `"Even"`, `"Odd"` |
| `stopBits` | `int` | default 1 |
| `terminalLines` | `list<string>` | Terminal history |

Invokables: `open()`, `close()`, `send(data)`, `clearTerminal()`

Signals: `dataReceived(hex)`, `errorOccurred(msg)`

---

### AnalyzerController (`AnalyzerVM`)

| Property | Type | Notes |
|----------|------|-------|
| `capturing` | `bool` | Capture active |
| `packetCount` | `int` | Total captured |
| `packets` | `var` (list) | `QVariantList<QVariantMap>` |
| `statistics` | `var` (map) | `total`, `valid`, `invalid`, per-protocol counts |

Invokables: `startCapture()`, `stopCapture()`, `clearPackets()`, `feedTcpData(bytes)`, `feedSerialData(bytes)`

Signal: `packetAdded(packet)`

Each packet map keys: `id`, `timestamp`, `direction`, `protocol`, `length`, `summary`, `hex`, `valid`

---

### SimulatorController (`SimulatorVM`)

| Property | Type | Notes |
|----------|------|-------|
| `running` | `bool` | Simulator active |
| `scenarioRunning` | `bool` | Scenario playback active |
| `randomEnabled` | `bool` | Random value generation |
| `registerMap` | `var` (list) | Modbus register entries |
| `ioaMap` | `var` (list) | IEC-104 IOA entries |

Invokables: `start()`, `stop()`, `reset()`, `setRegister(addr, val)`, `getRegister(addr)`, `updateIOA(ioa, val)`, `getIOA(ioa)`, `loadScenario(path)`, `enableRandom(bool)`, `setGlobalRange(min, max)`

Signals: `scenarioFinished()`, `errorOccurred(msg)`

---

### IECController (`IECVM`)

| Property | Type | Notes |
|----------|------|-------|
| `connected` | `bool` | Read-only |
| `stateText` | `string` | Human-readable state |
| `host` | `string` | Outstation IP |
| `port` | `int` | default 2404 |
| `commonAddress` | `int` | ASDU common address |
| `dataPoints` | `var` (list) | Received data points |

Invokables: `connect()`, `disconnect()`, `sendStartDT()`, `sendStopDT()`, `sendInterrogation()`, `sendCommand(params)`, `clearDataPoints()`

Signals: `dataPointReceived(point)`, `errorOccurred(msg)`

---

## Optional Feature Guards

| Guard macro | Controller | Behaviour when absent |
|-------------|------------|-----------------------|
| `RTU_SERIAL_ENABLED` | `SerialController` | Port always reports closed; `open()` emits error |
| `RTU_IEC104_ENABLED` | `IECController`   | Always disconnected; invokables emit error |

---

## Test Coverage (`tst_integration`)

47 GoogleTest cases across all six controllers:

- **ModbusController** (9): defaults, mode set/signal, host config, read/write error guards, clear log
- **SerialController** (6): defaults, port name, config signal, send guard, clear terminal
- **AnalyzerController** (8): capture toggle, signal, TCP feed, serial feed, model, clear, statistics, `packetAdded` signal
- **SimulatorController** (8): running toggle, signal, register map, IOA map, reset, random flag, scenario load error
- **IECController** (5): defaults, host config, send command guard, clear data points
- **SessionViewModel** (7): initial state, running, model empty, clear/disconnect no-crash, send while disconnected

---

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
ctest --test-dir build -R tst_integration --output-on-failure
```
