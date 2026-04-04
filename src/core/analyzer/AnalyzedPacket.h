#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <QVariantMap>
#include "core/models/ProtocolFrame.h"

namespace rtu::core::analyzer {

// Which protocol was detected / used during decoding
enum class ProtocolHint {
    ModbusTcp,
    ModbusRtu,
    IEC104,
    Serial,    // raw serial — no protocol framing detected
    Unknown
};

// ──────────────────────────────────────────────────────────────
// AnalyzedPacket
//
// Result of decoding one raw byte buffer.
// `fields` carries structured key/value pairs extracted by the
// decoder — protocol-specific, documented per decoder below.
//
// Modbus TCP fields:
//   "transaction_id"  int    — MBAP transaction identifier
//   "protocol_id"     int    — MBAP protocol id (0 = Modbus)
//   "unit_id"         int    — unit / slave identifier
//   "function_code"   int    — FC byte
//   "function_name"   string — e.g. "Read Holding Registers"
//   "start_address"   int    — if decodeable from PDU
//   "quantity"        int    — if decodeable from PDU
//   "exception_code"  int    — only for exception responses (FC|0x80)
//   "exception_name"  string — human-readable exception
//
// Modbus RTU fields:
//   "device_address"  int
//   "function_code"   int
//   "function_name"   string
//   "start_address"   int    — if decodeable
//   "quantity"        int    — if decodeable
//   "crc_valid"       bool
//   "exception_code"  int    — only for exception responses
//   "exception_name"  string
//
// IEC-104 fields:
//   "frame_type"      string — "I" | "S" | "U"
//   "tx_seq"          int    — I-frame only
//   "rx_seq"          int    — I-frame and S-frame
//   "type_id"         int    — I-frame only (ASDU type)
//   "type_name"       string — e.g. "M_SP_NA_1"
//   "cot"             int    — cause of transmission
//   "common_address"  int    — ASDU common address
//   "num_objects"     int    — number of information objects
//   "u_function"      string — U-frame only (e.g. "STARTDT_act")
// ──────────────────────────────────────────────────────────────
struct AnalyzedPacket {
    int            id          { 0 };
    QDateTime      timestamp;
    ProtocolHint   protocol    { ProtocolHint::Unknown };
    FrameDirection direction   { FrameDirection::Received };
    QByteArray     raw;
    QString        summary;        // one-line human-readable description
    bool           valid       { false };
    QString        errorReason;
    QVariantMap    fields;         // structured decoded fields (see above)
};

} // namespace rtu::core::analyzer

Q_DECLARE_METATYPE(rtu::core::analyzer::AnalyzedPacket)
Q_DECLARE_METATYPE(rtu::core::analyzer::ProtocolHint)
