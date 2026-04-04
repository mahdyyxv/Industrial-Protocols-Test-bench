#pragma once

#include "AnalyzedPacket.h"
#include "PacketFilter.h"
#include "core/models/ProtocolFrame.h"
#include <QObject>
#include <QVector>
#include <optional>

namespace rtu::core::analyzer {

// ──────────────────────────────────────────────────────────────
// PacketAnalyzer
//
// Protocol-aware packet capture, decode, filter, and query.
//
// Design:
//   - Reuses IEC104Client::parseResponse / describeApdu for IEC-104
//   - Contains standalone static decoders for Modbus TCP + RTU
//     (libmodbus is a communication library, not a frame parser;
//     decoding raw bytes is lightweight and has no external deps)
//   - No UI logic, no connections — pure data processing
//   - Thread-safe reads via const getters; writes must be on owner
//     thread (same pattern as the rest of rtu_core)
//
// Capture semantics:
//   - processTcpPacket / processSerialData always decode and emit
//   - storePacket only stores when capture is active
//   - Decode methods are public and usable standalone
// ──────────────────────────────────────────────────────────────
class PacketAnalyzer : public QObject {
    Q_OBJECT

public:
    explicit PacketAnalyzer(QObject* parent = nullptr);

    // ── Capture control ───────────────────────────────────────
    void startCapture();
    void stopCapture();
    bool isCapturing() const;

    // ── Input ─────────────────────────────────────────────────
    // Auto-detects protocol from header bytes, decodes, and
    // (if capturing) stores the resulting packet.
    void processTcpPacket(const QByteArray& data,
                          FrameDirection dir = FrameDirection::Received);
    void processSerialData(const QByteArray& data,
                           FrameDirection dir = FrameDirection::Received);

    // ── Decode (standalone — no connection required) ──────────

    // Decode a Modbus TCP frame (MBAP header + PDU).
    // Returns invalid packet if protocol_id != 0 or frame is too short.
    AnalyzedPacket decodeModbusTcpPacket(const QByteArray& data,
                                         FrameDirection dir = FrameDirection::Received) const;

    // Decode a Modbus RTU frame (address + FC + data + CRC).
    // Returns invalid packet if CRC fails or frame is too short.
    AnalyzedPacket decodeModbusRtuPacket(const QByteArray& data,
                                         FrameDirection dir = FrameDirection::Received) const;

    // Decode an IEC-104 APDU (reuses IEC104Client::parseResponse).
    // Returns invalid packet if start byte != 0x68 or frame < 6 bytes.
    AnalyzedPacket decodeIEC104Packet(const QByteArray& data,
                                      FrameDirection dir = FrameDirection::Received) const;

    // Auto-detect protocol and decode.
    AnalyzedPacket autoDecodePacket(const QByteArray& data,
                                    FrameDirection dir = FrameDirection::Received) const;

    // ── Filtering ─────────────────────────────────────────────
    void         setFilter(const PacketFilter& filter);
    void         clearFilter();
    PacketFilter currentFilter() const;

    // ── Storage ───────────────────────────────────────────────
    // Append a pre-decoded packet directly (bypasses auto-decode).
    void storePacket(const AnalyzedPacket& packet);
    void clearPackets();
    int  packetCount() const;

    // Buffer cap — oldest packets are dropped when full. 0 = unlimited.
    void setMaxPackets(int max);
    int  maxPackets()  const;

    // ── Query ─────────────────────────────────────────────────
    // Returns all stored packets matching the active filter.
    QVector<AnalyzedPacket> getPackets() const;

    // Returns stored packets matching an explicit filter (ignores active filter).
    QVector<AnalyzedPacket> getPackets(const PacketFilter& filter) const;

    // Lookup by id — returns nullopt if not found.
    std::optional<AnalyzedPacket> getPacketById(int id) const;

    // ── Statistics ────────────────────────────────────────────
    // Returns a map with:
    //   "total"         int
    //   "modbus_tcp"    int
    //   "modbus_rtu"    int
    //   "iec104"        int
    //   "unknown"       int
    //   "valid"         int
    //   "invalid"       int
    //   "error_rate"    double  (0.0–1.0)
    QVariantMap statistics() const;

    // Same but scoped to a filter.
    QVariantMap statistics(const PacketFilter& filter) const;

    // ── Utilities (static) ────────────────────────────────────
    static QString protocolName(ProtocolHint hint);
    static QString modbusFunctionName(int fc);
    static QString modbusExceptionName(int code);

    // CRC-16/IBM (Modbus standard) — exposed for testing
    static uint16_t modbusRtuCrc(const QByteArray& data);

signals:
    void packetStored(const rtu::core::analyzer::AnalyzedPacket& packet);
    void captureStarted();
    void captureStopped();
    void packetsCleared();

private:
    // Auto-detection heuristics
    static ProtocolHint detectProtocol(const QByteArray& data);
    static bool         looksLikeIEC104(const QByteArray& data);
    static bool         looksLikeModbusTcp(const QByteArray& data);
    static bool         looksLikeModbusRtu(const QByteArray& data);

    // Internal decode workers (accept pre-assigned id)
    AnalyzedPacket decodeModbusTcpInternal(const QByteArray& data,
                                            int id, FrameDirection dir) const;
    AnalyzedPacket decodeModbusRtuInternal(const QByteArray& data,
                                            int id, FrameDirection dir) const;
    AnalyzedPacket decodeIEC104Internal(const QByteArray& data,
                                         int id, FrameDirection dir) const;

    // Apply a filter to a single packet
    static bool applyFilter(const AnalyzedPacket& p, const PacketFilter& f);

    bool                    m_capturing  { false };
    int                     m_nextId     { 1 };
    int                     m_maxPackets { 10000 };
    QVector<AnalyzedPacket> m_packets;
    PacketFilter            m_filter;
};

} // namespace rtu::core::analyzer
