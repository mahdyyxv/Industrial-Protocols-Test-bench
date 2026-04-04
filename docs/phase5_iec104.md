# Phase 5 — IEC 60870-5-104 Module

**Role:** CORE ENGINEER  
**Status:** Complete  
**Date:** 2026-04-03

---

## What Was Implemented

A complete IEC-104 client layer built on top of [lib60870-C v2.3.2](https://github.com/mz-automation/lib60870), implementing `IProtocol` for registry integration and exposing the full IEC-104 function set.

| Component | File | Purpose |
|-----------|------|---------|
| Data model | `src/core/protocols/iec104/IEC104DataPoint.h` | Type-safe ASDU representation |
| Client | `src/core/protocols/iec104/IEC104Client.h` | Full API header |
| Implementation | `src/core/protocols/iec104/IEC104Client.cpp` | lib60870 wrapper |
| Tests | `tests/unit/tst_iec104.cpp` | 58 GoogleTest cases |

---

## How lib60870-C Is Integrated

lib60870-C is fetched automatically at configure time via CMake `FetchContent` — no system install required.

```cmake
FetchContent_Declare(
    lib60870
    GIT_REPOSITORY https://github.com/mz-automation/lib60870.git
    GIT_TAG        v2.3.2
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  lib60870-C
)
FetchContent_MakeAvailable(lib60870)
```

lib60870-C uses directory-level `include_directories()` instead of target INTERFACE includes, so the API headers are added explicitly to `rtu_core`:

```cmake
target_include_directories(rtu_core PRIVATE
    ${lib60870_SOURCE_DIR}/lib60870-C/src/inc/api
    ${lib60870_SOURCE_DIR}/lib60870-C/src/hal/inc
    ${lib60870_SOURCE_DIR}/lib60870-C/config
)
```

When found, `RTU_IEC104_ENABLED` is defined. All IEC-104 code is guarded by this symbol; tests use `GTEST_SKIP()` when absent.

---

## Wrapper Design

### Isolation strategy

lib60870-C exports C symbols (`CS104_Connection`, `CS101_ASDU`, `InformationObject`, etc.). These must not appear in public headers.

**Header** ([IEC104Client.h](../src/core/protocols/iec104/IEC104Client.h)) forward-declares:
```cpp
struct sCS104_Connection;   // underlying type of CS104_Connection
```
and holds only `struct sCS104_Connection* m_connection` — no lib60870 typedefs visible.

**Implementation** ([IEC104Client.cpp](../src/core/protocols/iec104/IEC104Client.cpp)) is the only file that `#include`s lib60870 headers.

### C callback bridge

lib60870 calls back into C function pointers. Two static methods act as bridges:

```cpp
// Registered with lib60870; calls instance method
static bool staticAsduHandler(void* param, int addr, void* asdu);
static void staticConnectionHandler(void* param, void* conn, int event);
```

The `void* param` is always `this`, cast back inside the static function.

### C-style struct inheritance

lib60870-C implements object orientation in C: timed variants (e.g. `SinglePointWithCP56Time2a`) share the same first fields as their base type (`SinglePointInformation`). Casting between them is safe:

```cpp
// M_SP_TB_1: cast to base for value/quality; keep typed ptr for timestamp
auto* base = reinterpret_cast<SinglePointInformation>(io);
dp.value   = SinglePointInformation_getValue(base);
auto* obj  = reinterpret_cast<SinglePointWithCP56Time2a>(io);
CP56Time2a t = SinglePointWithCP56Time2a_getTimestamp(obj);
```

---

## IEC104State

```cpp
enum class IEC104State {
    Disconnected,
    Connecting,
    Connected,      // TCP open, data transfer not yet active
    DataTransfer,   // STARTDT_CON received — data flowing
    Error
};
```

State transitions driven by `CS104_ConnectionEvent` callbacks and command errors. Every transition emits `stateChanged(IEC104State)`.

---

## API Exposed to Upper Layers

### Connection

```cpp
// Via IProtocol (QVariantMap keys: "host", "port", "common_addr", "response_timeout_ms")
bool connect(const QVariantMap& params);
void disconnect();
bool isConnected() const;

// Direct API
bool connectToOutstation(const QString& host, int port = 2404);
IEC104State state() const;
QString     lastError() const;
bool        isDataTransferActive() const;

void setCommonAddress(int addr);
void setResponseTimeoutMs(int ms);
void setOriginatorAddress(int addr);
```

### Session control

```cpp
void sendStartDT();             // STARTDT act — begin data transfer
void sendStopDT();              // STOPDT act — halt data transfer
void sendTestFrameActivation(); // TESTFR act — keepalive
```

### Interrogation

```cpp
void sendInterrogationCommand(int qoi = 20);          // C_IC_NA_1 (qoi=20 → all stations)
void sendCounterInterrogationCommand(int qrp = 5);    // C_CI_NA_1
void sendClockSynchronization();                       // C_CS_NA_1
```

### Commands (control direction)

| Method | FC | select-before-operate |
|--------|----|-----------------------|
| `sendSingleCommand(ioa, value, select)` | C_SC_NA_1 (45) | ✓ |
| `sendDoubleCommand(ioa, value, select)` | C_DC_NA_1 (46) | ✓ |
| `sendRegulatingStepCommand(ioa, dir, select)` | C_RC_NA_1 (47) | ✓ |
| `sendSetpointNormalized(ioa, value, select)` | C_SE_NA_1 (48) | ✓ |
| `sendSetpointScaled(ioa, value, select)` | C_SE_NB_1 (49) | ✓ |
| `sendSetpointFloat(ioa, value, select)` | C_SE_NC_1 (50) | ✓ |
| `sendBitstringCommand(ioa, bits)` | C_BO_NA_1 | ✗ |

### IProtocol frame operations

`buildRequest(QVariantMap)` encodes a command into a `ProtocolFrame.raw` byte buffer. Supported `"command"` values:

| Command string | Sends |
|----------------|-------|
| `"startdt"` | STARTDT act |
| `"stopdt"` | STOPDT act |
| `"test_frame"` | TESTFR act |
| `"interrogation"` | C_IC_NA_1 (param: `qoi`) |
| `"single_cmd"` | C_SC_NA_1 (params: `ioa`, `value`, `select`) |
| `"double_cmd"` | C_DC_NA_1 (params: `ioa`, `value`, `select`) |
| `"setpoint_normalized"` | C_SE_NA_1 (params: `ioa`, `value`, `select`) |
| `"setpoint_scaled"` | C_SE_NB_1 (params: `ioa`, `value`, `select`) |
| `"setpoint_float"` | C_SE_NC_1 (params: `ioa`, `value`, `select`) |
| `"clock_sync"` | C_CS_NA_1 |

`parseResponse(QByteArray)` classifies a raw APDU as I-frame / S-frame / U-frame and returns a human-readable description without needing an active connection.

### Signals

```cpp
void stateChanged(IEC104State newState);
void errorOccurred(const QString& message);
void dataTransferStarted();
void dataTransferStopped();

void dataPointReceived(const IEC104DataPoint& dp);
void interrogationComplete(int commonAddr);
void endOfInitialization(int commonAddr);

void commandConfirmed(int ioa, int typeId);
void commandNegated(int ioa, int typeId, int cot);
```

---

## Supported ASDU Types

### Monitor direction (inbound data)

| TypeID | Value | Description | `dp.value` type |
|--------|-------|-------------|-----------------|
| M_SP_NA_1 | 1 | Single-point | `bool` |
| M_DP_NA_1 | 3 | Double-point | `int` (0–3) |
| M_ST_NA_1 | 5 | Step position | `int` |
| M_BO_NA_1 | 7 | Bitstring 32 | `uint` |
| M_ME_NA_1 | 9 | Normalized value | `double` |
| M_ME_NB_1 | 11 | Scaled value | `int` |
| M_ME_NC_1 | 13 | Short float | `double` |
| M_IT_NA_1 | 15 | Integrated totals | `int` |
| M_SP_TB_1 | 30 | Single-point + CP56Time2a | `bool` |
| M_DP_TB_1 | 31 | Double-point + CP56Time2a | `int` |
| M_ME_TD_1 | 34 | Normalized + CP56Time2a | `double` |
| M_ME_TE_1 | 35 | Scaled + CP56Time2a | `int` |
| M_ME_TF_1 | 36 | Short float + CP56Time2a | `double` |
| M_EI_NA_1 | 70 | End of initialization | — (emits `endOfInitialization`) |

### Control direction (command feedback)

| TypeID | Value | Description |
|--------|-------|-------------|
| C_IC_NA_1 | 100 | Interrogation ACK → emits `interrogationComplete` |
| C_SC_NA_1 | 45 | Single command ACK/NAK |
| C_DC_NA_1 | 46 | Double command ACK/NAK |
| C_RC_NA_1 | 47 | Regulating step ACK/NAK |
| C_SE_NA_1 | 48 | Set-point normalized ACK/NAK |
| C_SE_NB_1 | 49 | Set-point scaled ACK/NAK |
| C_SE_NC_1 | 50 | Set-point float ACK/NAK |

---

## IEC104DataPoint

```cpp
struct IEC104DataPoint {
    int          commonAddr;    // ASDU common address
    int          ioa;           // Information Object Address
    IEC104TypeId typeId;        // Enum (M_SP_NA_1, M_ME_NC_1, …)
    int          cot;           // Cause of Transmission
    QVariant     value;         // Typed: bool / int / double / uint
    int          quality;       // Quality descriptor bits
    QDateTime    timestamp;     // Local receive time or CP56Time2a
    bool         hasTimestamp;  // true when CP56Time2a was present
    QString      typeName() const;  // human-readable type string
};
```

---

## lib60870-C ↔ IEC104Client API Mapping

| lib60870-C function | IEC104Client method |
|---------------------|---------------------|
| `CS104_Connection_create()` | `connectToOutstation()` |
| `CS104_Connection_destroy()` | `disconnect()` |
| `CS104_Connection_sendStartDT()` | `sendStartDT()` |
| `CS104_Connection_sendStopDT()` | `sendStopDT()` |
| `CS104_Connection_sendTestCommand()` | `sendTestFrameActivation()` |
| `CS104_Connection_sendInterrogationCommand()` | `sendInterrogationCommand(qoi)` |
| `CS104_Connection_sendCounterInterrogationCommand()` | `sendCounterInterrogationCommand(qrp)` |
| `CS104_Connection_sendClockSyncCommand()` | `sendClockSynchronization()` |
| `CS104_Connection_sendProcessCommandEx()` | `sendSingleCommand()` / `sendDoubleCommand()` / etc. |
| `CS104_Connection_setASDUReceivedHandler()` | → `staticAsduHandler` → `onAsduReceived()` |
| `CS104_Connection_setConnectionHandler()` | → `staticConnectionHandler` → `onConnectionEvent()` |
| `CS101_ASDU_getElement()` + type switch | → `dataPointReceived` signal |

---

## Known Limitations

1. **Slave / server mode not implemented.** `IEC104Client` is a master (controller) only. Server-side (outstation) requires `CS104_Slave` — out of scope for this phase.

2. **Synchronous connection only.** `connectToOutstation()` blocks until the TCP handshake completes (or fails). For UI use, call from `QThread` or `QtConcurrent::run()`.

3. **t1 timeout resolution is 1 second.** lib60870's `sCS104_APCIParameters.t1` is in whole seconds. Sub-second response timeouts are not supported.

4. **No TLS.** `CS104_Connection_createSecure()` is not exposed. Add when TLS configuration is available.

5. **No auto-reconnect.** If the connection drops (`CS104_CONNECTION_CLOSED`), callers must call `connectToOutstation()` again. Retry policy belongs in `ITestSession`.

6. **Bitstring32 command has no SBO.** `Bitstring32Command_create()` does not accept a select flag in lib60870 v2.3.

7. **ASDU types with no quality field** (e.g. `M_IT_NA_1`) leave `dp.quality = 0`.

---

## Integration Phase — Phase 6

### Register with ProtocolRegistry

```cpp
// In Application.cpp bootstrap:
#ifdef RTU_IEC104_ENABLED
#include "core/protocols/iec104/IEC104Client.h"
using namespace rtu::core;

ProtocolRegistry::instance().registerProtocol(
    protocols::ProtocolType::IEC104,
    []() { return std::make_unique<iec104::IEC104Client>(); }
);
#endif
```

### Feature gate check

`IEC104Client::requiresPro()` returns `true`. `TestEngine` checks this before creating a session:

```cpp
if (proto->requiresPro() &&
    !ServiceLocator::instance().subscription()->hasFeature(Feature::IEC104)) {
    emit sessionCreationFailed("IEC-104 requires Pro subscription");
    return nullptr;
}
```

### Typical usage sequence

```cpp
IEC104Client client;
client.setCommonAddress(1);

QObject::connect(&client, &IEC104Client::dataPointReceived,
    [](const auto& dp) { qDebug() << dp.ioa << dp.value; });

client.connectToOutstation("192.168.1.100", 2404);
client.sendStartDT();
client.sendInterrogationCommand();  // qoi=20 → all data
```

### Wire SessionViewModel

`SessionViewModel` should expose `Q_INVOKABLE` methods that delegate to `IEC104Client`:
- `connectToOutstation(host, port)` → `IEC104Client::connect(params)`
- `sendInterrogation()` → `IEC104Client::sendInterrogationCommand()`
- bind `stateChanged` → `Q_PROPERTY connectionState`
- bind `dataPointReceived` → append to `QAbstractListModel` for frame table

### Next module: DNP3

DNP3 is the remaining Pro-tier protocol. Recommended library: **libdnp3** (openpal + opendnp3). Architecture mirrors IEC-104:

```
src/core/protocols/dnp3/
├── DNP3DataPoint.h
├── DNP3MasterSession.h/.cpp   — wraps opendnp3 MasterStack
└── DNP3SlaveSession.h/.cpp    — wraps opendnp3 OutstationStack
```

`ProtocolType::DNP3` and `Feature::DNP3` are already declared in `IProtocol.h` and `SubscriptionTier.h`.
