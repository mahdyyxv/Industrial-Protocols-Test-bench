# Phase 4 — Modbus Module

**Role:** CORE ENGINEER  
**Status:** Complete  
**Date:** 2026-04-03

---

## What Was Implemented

A clean Modbus client layer supporting both **TCP** and **RTU** transports, wrapping [libmodbus](https://libmodbus.org/) behind a pure C++ interface. No UI binding occurs in this layer.

| Component | File | Purpose |
|-----------|------|---------|
| Interface | `src/core/protocols/modbus/IModbusClient.h` | Contract + `ModbusState` enum |
| TCP client | `src/core/protocols/modbus/ModbusTcpClient.h/.cpp` | Modbus TCP/IP (slave mode) |
| RTU client | `src/core/protocols/modbus/ModbusRtuClient.h/.cpp` | Modbus RTU over serial port |
| Tests | `tests/unit/tst_modbus.cpp` | 20+ GoogleTest cases |

---

## How libmodbus Is Integrated

libmodbus is detected at CMake configure time. The search order is:

1. **CMake config** (`find_package(libmodbus CONFIG)`) — satisfies vcpkg installs on Windows
2. **pkg-config** (`pkg_check_modules(MODBUS libmodbus)`) — satisfies system installs on Linux/macOS
3. If neither found: CMake emits a warning and the module is excluded from the build

When found, the `RTU_MODBUS_ENABLED` compile definition is added to `rtu_core`. All Modbus code is guarded by `#ifdef RTU_MODBUS_ENABLED` in tests.

**Install instructions:**

```bash
# Linux (Debian/Ubuntu)
sudo apt-get install libmodbus-dev

# Windows — vcpkg
vcpkg install libmodbus
cmake -B build -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake
```

---

## Wrapper Design

### Problem: libmodbus leaks C types

libmodbus exposes `modbus_t*`, `modbus_strerror()`, and other C symbols throughout its API. These must not appear in public headers consumed by ViewModels or the UI layer.

### Solution: forward-declaration + confined includes

```
IModbusClient.h          — pure C++ interface, no modbus.h
ModbusTcpClient.h        — forward-declares struct _modbus*
ModbusTcpClient.cpp      — only file that includes <modbus.h>
ModbusRtuClient.h        — same pattern
ModbusRtuClient.cpp      — only file that includes <modbus.h>
```

`struct _modbus` is `modbus_t`'s underlying struct (typedef'd in `modbus.h`). Forward-declaring it in the header allows holding a pointer without pulling in the full C header.

### ModbusState

```cpp
enum class ModbusState { Disconnected, Connecting, Connected, Error };
```

Both concrete classes maintain this state and emit `stateChanged(ModbusState)` on every transition.

---

## API Exposed to Upper Layers

### IModbusClient (pure C++ interface)

```cpp
class IModbusClient {
    virtual bool        connect()    = 0;
    virtual void        disconnect() = 0;
    virtual bool        isConnected() const = 0;
    virtual ModbusState state()       const = 0;
    virtual QString     lastError()   const = 0;

    virtual std::vector<uint16_t> readHoldingRegisters(int addr, int count)  = 0;
    virtual bool                  writeSingleRegister(int addr, uint16_t val) = 0;
};
```

No libmodbus types appear in this interface.

### ModbusTcpClient — construction

```cpp
ModbusTcpClient client("192.168.1.10", 502);
client.setResponseTimeout(1000);   // ms
client.connect();
auto regs = client.readHoldingRegisters(0, 10);
client.writeSingleRegister(5, 0xFF00);
```

### ModbusRtuClient — construction

```cpp
ModbusRtuConfig cfg;
cfg.portName          = "/dev/ttyUSB0";
cfg.baudRate          = 9600;
cfg.parity            = 'N';      // 'N', 'E', 'O'
cfg.dataBits          = 8;
cfg.stopBits          = 1;
cfg.responseTimeoutMs = 500;

ModbusRtuClient client(cfg);
client.setSlaveId(1);             // Modbus device address on bus
client.connect();
```

### Qt signals (both classes)

```cpp
signals:
    void stateChanged(ModbusState newState);
    void errorOccurred(const QString& message);
```

ViewModels should `connect()` to these to react to state changes without polling.

---

## How RTU Uses SerialManager

libmodbus RTU manages its own serial file descriptor (opened via `modbus_new_rtu()`). It cannot share a port with `SerialManager`.

**Integration point:** `ModbusRtuConfig` uses the same field names and semantics as `serial::SerialConfig` (Phase 3). A conversion helper bridges them when `RTU_SERIAL_ENABLED` is defined:

```cpp
// Only available when Qt6SerialPort is present
#ifdef RTU_SERIAL_ENABLED
ModbusRtuConfig cfg = rtu::core::modbus::fromSerialConfig(serialConfig);
#endif
```

The helper maps:
- `portName` → `portName` (direct)
- `baudRate` → `baudRate` (direct)
- `QSerialPort::EvenParity` → `'E'`, `OddParity` → `'O'`, `NoParity` → `'N'`
- `dataBits` (enum int) → `dataBits` (int)
- `stopBits` (enum int) → `stopBits` (int)
- `readTimeoutMs` → `responseTimeoutMs`

This means the UI can expose a single serial-port configuration form and feed it to either `SerialProtocol` (raw serial) or `ModbusRtuClient` depending on the selected protocol.

**Constraint:** Only one component may open a given serial port at a time. If `SerialManager` has the port open, `ModbusRtuClient` must not call `connect()` until `SerialManager::close()` is called.

---

## Error Handling

All libmodbus errors are mapped through the following pattern:

```cpp
if (modbus_connect(m_ctx) == -1) {
    handleError("modbus_connect: " + modbus_strerror(errno));
    // → sets state to Error, emits errorOccurred(message)
}
```

`handleError()`:
1. Stores the message in `m_lastError` (accessible via `lastError()`)
2. Transitions state to `ModbusState::Error`
3. Emits `errorOccurred(message)` for reactive error display

Operations called in a disconnected/error state return safe empty values:
- `readHoldingRegisters()` → empty `vector`
- `writeSingleRegister()` → `false`

---

## Known Limitations

1. **Synchronous API only.** `readHoldingRegisters()` and `writeSingleRegister()` block the calling thread until the response arrives or times out. For UI use, wrap calls in `QThread` or `QtConcurrent::run()`.

2. **Single-register write only.** `writeSingleRegister()` maps to Modbus function code 06. Multi-register write (FC 16) is not yet implemented.

3. **Holding registers only.** Coils (FC 01, 05, 15), discrete inputs (FC 02), and input registers (FC 04) are not yet exposed.

4. **No slave/server mode.** Both clients operate as Modbus masters. A slave/server implementation requires `modbus_mapping_t` and a listener loop, which is out of scope for this phase.

5. **No connection retry logic.** If `connect()` fails, callers must retry explicitly. An auto-reconnect policy belongs in the `ITestSession` layer.

---

## Next Phase — Phase 5: IEC-104

### Architecture guidance

IEC-104 (IEC 60870-5-104) is a TCP-based protocol with persistent connections and asynchronous event delivery. Its structure differs from Modbus:

- **Connection:** TCP to port 2404 (standard)
- **Framing:** APCI header (6 bytes) + ASDU payload (variable)
- **Model:** asynchronous — data arrives via spontaneous transmission (SPONT), not only in response to polls
- **Frame types:** I-frames (data), S-frames (supervisory), U-frames (STARTDT/STOPDT/TESTFR)

### Recommended file structure

```
src/core/protocols/iec104/
├── IEC104Client.h        — QObject, connects to outstation (server)
├── IEC104Client.cpp      — Uses QTcpSocket for async I/O
├── IEC104Frame.h         — APCI/ASDU parsing (no external lib needed)
└── IEC104FrameParser.cpp
```

### Recommended approach

Unlike Modbus, IEC-104 should use **Qt's async socket API** (`QTcpSocket`) rather than a blocking C library:
- `QTcpSocket::readyRead` triggers frame assembly
- `QTimer` implements T1 (response timeout), T2 (supervisory frame), T3 (test frame) per the standard
- This avoids blocking the event loop

### Interface suggestion

```cpp
class IEC104Client : public QObject {
    Q_OBJECT
public:
    bool connectToOutstation(const QString& host, int port = 2404);
    void disconnect();
    void sendInterrogation();           // C_IC_NA_1 — all data request

signals:
    void connectionChanged(bool connected);
    void informationObjectReceived(int ioa, QVariant value, QDateTime timestamp);
    void errorOccurred(const QString& message);
};
```

### Feature gating

IEC-104 is a **Pro-tier** feature (`requiresPro() = true`). The `Feature::IEC104` enum value is already defined in `src/services/subscription/SubscriptionTier.h`.
