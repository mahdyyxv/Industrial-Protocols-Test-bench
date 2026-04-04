#pragma once

#include "AnalyzedPacket.h"
#include <optional>
#include <QDateTime>

namespace rtu::core::analyzer {

// ──────────────────────────────────────────────────────────────
// PacketFilter
//
// Value type passed to PacketAnalyzer::setFilter() or getPackets().
// Any field left at its sentinel value means "no constraint".
// Multiple fields are ANDed together.
// ──────────────────────────────────────────────────────────────
struct PacketFilter {
    // Protocol constraint
    std::optional<ProtocolHint>   protocol;

    // Direction constraint
    std::optional<FrameDirection> direction;

    // Validity constraint — true = keep only valid frames
    bool validOnly { false };

    // Modbus-specific: -1 means "any"
    int modbusFunction  { -1 };   // filter by FC (e.g. 3 = Read Holding Registers)
    int modbusAddress   { -1 };   // filter by start_address field
    int modbusUnitId    { -1 };   // filter by unit_id / device_address

    // IEC-104-specific: -1 means "any"
    int iec104TypeId     { -1 };  // filter by ASDU type id
    int iec104CommonAddr { -1 };  // filter by common address

    // Time range — null QDateTime means open-ended
    QDateTime since;
    QDateTime until;

    // Result cap — 0 = unlimited
    int maxResults { 0 };

    // ── Factory helpers ───────────────────────────────────────
    static PacketFilter forProtocol(ProtocolHint p)
    {
        PacketFilter f;
        f.protocol = p;
        return f;
    }

    static PacketFilter forModbusFunction(int fc)
    {
        PacketFilter f;
        f.protocol        = ProtocolHint::ModbusTcp;   // caller can widen
        f.modbusFunction  = fc;
        return f;
    }

    static PacketFilter validFramesOnly()
    {
        PacketFilter f;
        f.validOnly = true;
        return f;
    }

    static PacketFilter afterTime(const QDateTime& t)
    {
        PacketFilter f;
        f.since = t;
        return f;
    }

    bool isEmpty() const
    {
        return !protocol.has_value()
            && !direction.has_value()
            && !validOnly
            && modbusFunction  == -1
            && modbusAddress   == -1
            && modbusUnitId    == -1
            && iec104TypeId    == -1
            && iec104CommonAddr == -1
            && !since.isValid()
            && !until.isValid()
            && maxResults == 0;
    }
};

} // namespace rtu::core::analyzer
