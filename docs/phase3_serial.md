# Phase 3 — Serial Communication Module

## Overview

Adds a hardware-layer serial protocol to the RTU Test Bench. The module wraps `QSerialPort` behind a clean `IProtocol` interface so the rest of the application is agnostic to the underlying transport.

---

## Architecture

```
SerialProtocol  (implements IProtocol)
    └── SerialManager  (wraps QSerialPort)
            └── QSerialPort
```

**SerialManager** is the pure I/O layer. It knows nothing about frames or protocols — it just opens/closes ports and moves bytes.

**SerialProtocol** is the protocol adapter. It owns a `SerialManager`, assembles received bytes into `ProtocolFrame` objects, and exposes the `IProtocol` contract to the rest of the application.

---

## Files

| File | Purpose |
|------|---------|
| `src/core/protocols/serial/SerialManager.h` | I/O layer header |
| `src/core/protocols/serial/SerialManager.cpp` | I/O layer implementation |
| `src/core/protocols/serial/SerialProtocol.h` | IProtocol adapter header |
| `src/core/protocols/serial/SerialProtocol.cpp` | IProtocol adapter implementation |
| `tests/unit/tst_serial_manager.cpp` | 14 unit tests |

---

## SerialConfig

```cpp
struct SerialConfig {
    QString                  portName;
    qint32                   baudRate      { 9600 };
    QSerialPort::DataBits    dataBits      { QSerialPort::Data8 };
    QSerialPort::Parity      parity        { QSerialPort::NoParity };
    QSerialPort::StopBits    stopBits      { QSerialPort::OneStop };
    QSerialPort::FlowControl flowControl   { QSerialPort::NoFlowControl };
    int                      readTimeoutMs { 1000 };
};
```

All fields have safe defaults. Callers only need to set `portName` for typical use.

---

## SerialManager Interface

```cpp
class SerialManager : public QObject {
public:
    bool open(const SerialConfig& config);   // returns false + emits errorOccurred on failure
    void close();
    bool write(const QByteArray& data);      // returns false + emits errorOccurred if not connected
    void clearBuffer();
    QByteArray receiveBuffer() const;
    bool isConnected() const;
    QString portName() const;
    static QStringList availablePorts();

signals:
    void dataSent(const QByteArray& data);
    void dataReceived(const QByteArray& data);
    void receiveTimeout();
    void errorOccurred(const QString& message, QSerialPort::SerialPortError code);
    void connectionChanged(bool connected);
};
```

### Receive Timeout Strategy

Serial ports have no packet framing. `SerialManager` uses a single-shot `QTimer` to detect end-of-response:

1. First byte arrives → timer starts (`readTimeoutMs`, default 1000 ms)
2. Each subsequent byte → timer restarts
3. Timer fires → `receiveTimeout()` emitted
4. `SerialProtocol` listens to `receiveTimeout()` and assembles the accumulated buffer into a `ProtocolFrame`

This strategy works correctly for Modbus RTU (response fully arrives within one inter-frame silence period) and most other RTU protocols.

### Error Handling

Unrecoverable `QSerialPort` errors trigger automatic port close:
- `QSerialPort::ResourceError` — port physically disconnected
- `QSerialPort::DeviceNotFoundError` — port removed mid-session

All errors emit `errorOccurred(message, code)` before any state change. The UI should connect to `errorOccurred` to display user-visible messages.

---

## SerialProtocol Interface

Implements `IProtocol`. Key decisions:

| Property | Value | Reason |
|----------|-------|--------|
| `name()` | `"Serial"` | Display name in UI |
| `transport()` | `TransportType::Serial` | Used by ProtocolRegistry |
| `requiresPro()` | `false` | Serial is available in Free tier |

### connect(QVariantMap)

Accepted keys:

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `port` | string | — | **Required.** Port name (e.g. `COM3`, `/dev/ttyUSB0`) |
| `baudRate` | int | 9600 | Baud rate |
| `dataBits` | int | 8 | Data bits (5/6/7/8) |
| `parity` | int | 0 | 0=None, 2=Even, 3=Odd, 4=Space, 5=Mark |
| `stopBits` | int | 1 | 1, 2, or 3 (1.5) |
| `flowControl` | int | 0 | 0=None, 1=Hardware, 2=Software |
| `timeout` | int | 1000 | Read silence timeout (ms) |

Returns `false` and emits `errorOccurred` if `port` is missing or port fails to open.

### buildRequest(QVariantMap)

Accepted keys:

| Key | Type | Description |
|-----|------|-------------|
| `hex` | string | Raw bytes as hex string (e.g. `"010300000001"`) |

Returns an invalid `ProtocolFrame` with `errorReason` if `hex` is empty.

### parseResponse(QByteArray)

Wraps raw bytes in a `ProtocolFrame`. Returns invalid frame for empty input.

---

## Build Requirements

Qt6SerialPort must be installed for the Qt version you are building against.

**Qt Maintenance Tool (recommended for bundled Qt 6.11):**
1. Open `~/Qt/MaintenanceTool`
2. Add/Remove Components → Qt 6.11 → Add **Qt Serial Port**
3. Re-run cmake

**System package (matches system Qt only):**
```bash
sudo apt-get install qt6-serialport-dev
```

When `Qt6SerialPort` is not found, CMake emits a warning and the serial module is excluded from the build. The `RTU_SERIAL_ENABLED` compile definition is only set when SerialPort is available. All serial tests use `QSKIP` when the definition is absent, so the test suite always passes.

---

## Tests

File: `tests/unit/tst_serial_manager.cpp`

| Test | What it verifies |
|------|-----------------|
| `test_initialStateIsDisconnected` | Default state: `isConnected=false`, empty portName and buffer |
| `test_openWithEmptyPortNameFails` | `open()` returns false for empty portName |
| `test_openNonExistentPortEmitsError` | `open()` emits `errorOccurred`, `isConnected` stays false |
| `test_closeWhileDisconnectedIsSafe` | `close()` on closed port doesn't crash |
| `test_writeWhileDisconnectedEmitsError` | `write()` returns false and emits error with "not open" message |
| `test_writeEmptyDataSucceedsWithoutError` | Empty write on disconnected port → error path (guard fires) |
| `test_clearBufferEmptiesAccumulator` | `clearBuffer()` is idempotent, doesn't crash |
| `test_availablePortsReturnsList` | Returns a list without crashing (may be empty on CI) |
| `test_protocolIdentity` | name, transport, requiresPro, isConnected match spec |
| `test_protocolConnectMissingPortEmitsError` | `connect({})` emits errorOccurred containing "port" |
| `test_protocolBuildRequestFromHex` | Hex string → correct raw bytes, `valid=true`, `direction=Sent` |
| `test_protocolBuildRequestEmptyHexIsInvalid` | Empty params → `valid=false`, non-empty errorReason |
| `test_protocolParseResponseEmpty` | Empty bytes → `valid=false`, `direction=Received` |
| `test_protocolParseResponseNonEmpty` | Non-empty bytes → `valid=true`, raw preserved |
| `test_protocolSendInvalidFrameEmitsError` | Sending invalid frame → returns false, emits errorOccurred |
| `test_protocolDisconnectWhenNotConnectedIsSafe` | `disconnect()` on fresh protocol doesn't crash |
| `test_protocolAvailablePorts` | Static method returns without crashing |

Run with:
```bash
cd build
ctest -R tst_serial --output-on-failure
```

---

## Key Design Decisions

**1. Two-class split (Manager + Protocol)**
`SerialManager` is reusable outside the protocol system (e.g., direct use from a future terminal view). `SerialProtocol` contains only the `IProtocol` glue, keeping both classes focused.

**2. Timeout-based framing instead of byte count**
RTU protocols vary in response length. Timeout-based framing requires no protocol-specific knowledge in the transport layer. The default 1000 ms covers slow baud rates; the caller can reduce it for faster devices.

**3. requiresPro() = false**
Serial is the baseline hardware interface for RTU/SCADA work. Restricting it to Pro would block the free trial's core use case. Pro-only serial features (if any) are added at the `ITestSession` level, not the transport level.

**4. Conditional compilation via RTU_SERIAL_ENABLED**
Keeps the build clean on systems without SerialPort. The test guard pattern (`#ifndef RTU_SERIAL_ENABLED / QSKIP`) ensures CI never fails due to a missing optional dependency.

---

## Next Phase — Phase 4: INTEGRATION

The integration phase must:

1. **Register SerialProtocol with ProtocolRegistry**
   ```cpp
   // In ProtocolRegistry::registerDefaults():
   register(std::make_shared<SerialProtocol>());
   ```

2. **Wire SessionViewModel to protocol selection**
   `SessionViewModel::setActiveProtocol(const QString& name)` should look up the protocol by name and hold a shared pointer.

3. **Add Serial configuration panel in QML**
   A `SerialConfigPanel.qml` component exposing port selection (populated from `SerialProtocol::availablePorts()`), baud rate, and the connect/disconnect button.

4. **Port hot-plug support (optional)**
   Poll `availablePorts()` on a 2-second timer and update the UI list. `QSerialPortInfo` doesn't emit change notifications, so polling is required.

5. **Modbus RTU framing (future — Phase 5)**
   `SerialProtocol` currently treats all bytes as raw. A `ModbusRtuProtocol` that owns a `SerialManager` (or delegates to `SerialProtocol`) should add CRC validation and function-code parsing.
