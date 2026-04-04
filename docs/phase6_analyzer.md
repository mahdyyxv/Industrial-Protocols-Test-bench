# Phase 6 — Packet Analyzer Module

**Role:** CORE ENGINEER  
**Status:** Complete  
**Date:** 2026-04-03

---

## What Was Implemented

A protocol-aware packet capture, decode, filter, and query engine. No UI logic, no connections — pure data processing.

| Component | File | Purpose |
|-----------|------|---------|
| Packet type | `src/core/analyzer/AnalyzedPacket.h` | Result of decoding one raw byte buffer |
| Filter type | `src/core/analyzer/PacketFilter.h` | ANDed constraint set for queries |
| Analyzer | `src/core/analyzer/PacketAnalyzer.h` | Full API |
| Implementation | `src/core/analyzer/PacketAnalyzer.cpp` | Decoders, storage, query |
| Tests | `tests/unit/tst_analyzer.cpp` | GoogleTest: ~40 cases |

---

## Design Decisions

### No external dependencies

Modbus CRC and frame decoding are implemented directly — libmodbus is a communication library, not a frame parser. The CRC-16/IBM (polynomial 0xA001, initial value 0xFFFF) is 14 lines and has no external deps.

IEC-104 APCI parsing uses the public bit-pattern specification. `IEC104Client::typeIdToString()` is called for the ASDU type name string when `RTU_IEC104_ENABLED` is defined, but the decoder itself has no lib60870 dependency.

### Capture semantics

- `processTcpPacket` / `processSerialData` **always decode and emit** `packetStored` if capturing.
- `storePacket` **only stores** when `isCapturing()` is true. Direct decode methods (`decodeModbusTcpPacket` etc.) are standalone and never store.
- Buffer cap: when `m_packets.size() >= m_maxPackets` the oldest packet is dropped before inserting. 0 = unlimited (default 10 000).

### Protocol auto-detection order

Detection is ordered by specificity to minimise false positives:

1. **IEC-104**: unique 0x68 start byte + `data.size() == length_byte + 2`
2. **Modbus TCP**: `protocolId == 0x0000` + `length_field + 6 == data.size()`
3. **Modbus RTU**: CRC-16/IBM validation passes
4. **Unknown**: none of the above matched

Modbus RTU CRC validation can produce false positives on random data, but because it is checked last (after the two more distinctive protocols), the collision probability in practice is negligible.

---

## API Summary

### Capture control

```cpp
void startCapture();
void stopCapture();
bool isCapturing() const;
```

### Input

```cpp
void processTcpPacket(const QByteArray& data, FrameDirection dir = Received);
void processSerialData(const QByteArray& data, FrameDirection dir = Received);
```

`processTcpPacket` auto-detects IEC-104 vs Modbus TCP.  
`processSerialData` always decodes as Modbus RTU.

### Standalone decode

```cpp
AnalyzedPacket decodeModbusTcpPacket(const QByteArray& data, FrameDirection dir) const;
AnalyzedPacket decodeModbusRtuPacket(const QByteArray& data, FrameDirection dir) const;
AnalyzedPacket decodeIEC104Packet   (const QByteArray& data, FrameDirection dir) const;
AnalyzedPacket autoDecodePacket     (const QByteArray& data, FrameDirection dir) const;
```

These return decoded packets without storing them.

### Filtering

```cpp
void         setFilter(const PacketFilter& filter);
void         clearFilter();
PacketFilter currentFilter() const;
```

### Storage

```cpp
void storePacket(const AnalyzedPacket& packet);
void clearPackets();
int  packetCount() const;
void setMaxPackets(int max);   // 0 = unlimited
int  maxPackets()  const;
```

### Query

```cpp
QVector<AnalyzedPacket> getPackets() const;                        // uses active filter
QVector<AnalyzedPacket> getPackets(const PacketFilter& f) const;   // explicit filter
std::optional<AnalyzedPacket> getPacketById(int id) const;
```

### Statistics

```cpp
QVariantMap statistics() const;
QVariantMap statistics(const PacketFilter& filter) const;
```

Returns:

| Key | Type | Description |
|-----|------|-------------|
| `total` | int | Total packets matched |
| `modbus_tcp` | int | Modbus TCP packets |
| `modbus_rtu` | int | Modbus RTU packets |
| `iec104` | int | IEC-104 packets |
| `unknown` | int | Unknown/Serial packets |
| `valid` | int | Valid frames |
| `invalid` | int | Invalid / errored frames |
| `error_rate` | double | `invalid / total` (0.0–1.0) |

---

## AnalyzedPacket Fields

### Modbus TCP

| Key | Type | Notes |
|-----|------|-------|
| `transaction_id` | int | MBAP transaction identifier |
| `protocol_id` | int | Always 0 for valid frames |
| `unit_id` | int | Unit / slave identifier |
| `function_code` | int | Raw FC byte |
| `function_name` | string | Human-readable FC name |
| `start_address` | int | For FC 01–04, 0F, 10, 05, 06 |
| `quantity` | int | For FC 01–04, 0F, 10 |
| `is_exception` | bool | True when FC bit7 set |
| `exception_code` | int | Only for exception responses |
| `exception_name` | string | Human-readable exception |

### Modbus RTU

| Key | Type | Notes |
|-----|------|-------|
| `device_address` | int | Slave address byte |
| `function_code` | int | Raw FC byte |
| `function_name` | string | Human-readable FC name |
| `crc_valid` | bool | Always present |
| `start_address` | int | FC-dependent |
| `quantity` | int | FC-dependent |
| `is_exception` | bool | True when FC bit7 set |
| `exception_code` | int | Only for exception responses |
| `exception_name` | string | Human-readable exception |

### IEC-104

| Key | Type | Notes |
|-----|------|-------|
| `frame_type` | string | `"I"` / `"S"` / `"U"` |
| `tx_seq` | int | I-frame only |
| `rx_seq` | int | I-frame and S-frame |
| `type_id` | int | I-frame only |
| `type_name` | string | e.g. `"M_SP_NA_1"` |
| `cot` | int | Cause of transmission |
| `common_address` | int | ASDU common address |
| `num_objects` | int | Number of information objects |
| `u_function` | string | U-frame only (e.g. `"STARTDT_act"`) |

---

## PacketFilter

All fields default to "no constraint" (`-1`, `false`, null `QDateTime`). Multiple fields are ANDed.

```cpp
struct PacketFilter {
    std::optional<ProtocolHint>   protocol;
    std::optional<FrameDirection> direction;
    bool validOnly           { false };
    int  modbusFunction      { -1 };
    int  modbusAddress       { -1 };
    int  modbusUnitId        { -1 };
    int  iec104TypeId        { -1 };
    int  iec104CommonAddr    { -1 };
    QDateTime since;
    QDateTime until;
    int  maxResults          { 0 };
};
```

Factory helpers: `PacketFilter::forProtocol(p)`, `forModbusFunction(fc)`, `validFramesOnly()`, `since(t)`.

---

## Static Utilities

```cpp
static QString  protocolName(ProtocolHint hint);
static QString  modbusFunctionName(int fc);    // strips exception bit
static QString  modbusExceptionName(int code);
static uint16_t modbusRtuCrc(const QByteArray& data);   // CRC-16/IBM
```

---

## Signals

```cpp
void packetStored(const AnalyzedPacket& packet);
void captureStarted();
void captureStopped();
void packetsCleared();
```

---

## Test Coverage

`tests/unit/tst_analyzer.cpp` — ~40 GoogleTest cases:

| Category | Tests |
|----------|-------|
| CRC | known vector, empty input, single byte, residue property |
| Utility functions | FC names, exception names, protocol names |
| Modbus TCP decode | valid, exception, too short, bad protocol ID, direction |
| Modbus RTU decode | valid, CRC failure, too short, exception |
| IEC-104 decode | U-frame, S-frame, I-frame fields, wrong start byte, too short |
| Auto-detect | each protocol, unknown data |
| Capture control | initial state, start/stop, signals, double start no-op, store guard |
| Buffer cap | enforces max, drops oldest first, zero = unlimited |
| Storage | storePacket directly, clearPackets, signals |
| Query | getPacketById, getPacketById not found, getPackets all |
| Filter | protocol, direction, validOnly, FC, maxResults, time range, active filter |
| Statistics | counts by protocol, error rate, empty, with filter |
| processSerialData | stores RTU packet |

---

## Integration Notes

`PacketAnalyzer` is always built (no feature gate). It integrates with the rest of the stack as a passive observer:

```cpp
// Wire to IEC104Client:
QObject::connect(&client, &IEC104Client::rawApduReceived,
    &analyzer, [&](const QByteArray& data) {
        analyzer.processTcpPacket(data, FrameDirection::Received);
    });

// Wire to SerialManager:
QObject::connect(&serial, &SerialManager::dataReceived,
    &analyzer, [&](const QByteArray& data) {
        analyzer.processSerialData(data, FrameDirection::Received);
    });
```

The analyzer does not hold references to any protocol client. It operates on raw byte arrays only.
