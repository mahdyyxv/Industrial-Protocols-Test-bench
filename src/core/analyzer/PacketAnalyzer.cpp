#include "PacketAnalyzer.h"

// Reuse IEC-104 decode logic (conditional on build config)
#ifdef RTU_IEC104_ENABLED
#include "core/protocols/iec104/IEC104Client.h"
#include "core/protocols/iec104/IEC104DataPoint.h"
#endif

#include <QDateTime>

namespace rtu::core::analyzer {

// ──────────────────────────────────────────────────────────────
// Modbus function code & exception helpers
// ──────────────────────────────────────────────────────────────

QString PacketAnalyzer::modbusFunctionName(int fc)
{
    // Strip exception bit for lookup
    const int base = fc & 0x7F;
    switch (base) {
        case 0x01: return QStringLiteral("Read Coils");
        case 0x02: return QStringLiteral("Read Discrete Inputs");
        case 0x03: return QStringLiteral("Read Holding Registers");
        case 0x04: return QStringLiteral("Read Input Registers");
        case 0x05: return QStringLiteral("Write Single Coil");
        case 0x06: return QStringLiteral("Write Single Register");
        case 0x07: return QStringLiteral("Read Exception Status");
        case 0x08: return QStringLiteral("Diagnostics");
        case 0x0B: return QStringLiteral("Get Comm Event Counter");
        case 0x0C: return QStringLiteral("Get Comm Event Log");
        case 0x0F: return QStringLiteral("Write Multiple Coils");
        case 0x10: return QStringLiteral("Write Multiple Registers");
        case 0x11: return QStringLiteral("Report Server ID");
        case 0x14: return QStringLiteral("Read File Record");
        case 0x15: return QStringLiteral("Write File Record");
        case 0x16: return QStringLiteral("Mask Write Register");
        case 0x17: return QStringLiteral("Read/Write Multiple Registers");
        case 0x18: return QStringLiteral("Read FIFO Queue");
        case 0x2B: return QStringLiteral("Encapsulated Interface Transport");
        default:   return QStringLiteral("Unknown (0x%1)").arg(base, 2, 16, QChar('0'));
    }
}

QString PacketAnalyzer::modbusExceptionName(int code)
{
    switch (code) {
        case 0x01: return QStringLiteral("Illegal Function");
        case 0x02: return QStringLiteral("Illegal Data Address");
        case 0x03: return QStringLiteral("Illegal Data Value");
        case 0x04: return QStringLiteral("Server Device Failure");
        case 0x05: return QStringLiteral("Acknowledge");
        case 0x06: return QStringLiteral("Server Device Busy");
        case 0x08: return QStringLiteral("Memory Parity Error");
        case 0x0A: return QStringLiteral("Gateway Path Unavailable");
        case 0x0B: return QStringLiteral("Gateway Target Device Failed to Respond");
        default:   return QStringLiteral("Unknown (0x%1)").arg(code, 2, 16, QChar('0'));
    }
}

QString PacketAnalyzer::protocolName(ProtocolHint hint)
{
    switch (hint) {
        case ProtocolHint::ModbusTcp: return QStringLiteral("Modbus TCP");
        case ProtocolHint::ModbusRtu: return QStringLiteral("Modbus RTU");
        case ProtocolHint::IEC104:    return QStringLiteral("IEC-104");
        case ProtocolHint::Serial:    return QStringLiteral("Serial");
        case ProtocolHint::Unknown:   return QStringLiteral("Unknown");
    }
    return QStringLiteral("Unknown");
}

// ──────────────────────────────────────────────────────────────
// CRC-16/IBM (Modbus standard)
//
// Polynomial: 0x8005 (reflected: 0xA001)
// Initial value: 0xFFFF
// Final XOR: none
// ──────────────────────────────────────────────────────────────
uint16_t PacketAnalyzer::modbusRtuCrc(const QByteArray& data)
{
    uint16_t crc = 0xFFFF;
    for (int i = 0; i < data.size(); ++i) {
        crc ^= static_cast<uint8_t>(data[i]);
        for (int b = 0; b < 8; ++b) {
            if (crc & 0x0001)
                crc = static_cast<uint16_t>((crc >> 1) ^ 0xA001u);
            else
                crc >>= 1;
        }
    }
    return crc;
}

// ──────────────────────────────────────────────────────────────
// Protocol detection heuristics
// ──────────────────────────────────────────────────────────────

bool PacketAnalyzer::looksLikeIEC104(const QByteArray& data)
{
    if (data.size() < 6) return false;
    if (static_cast<uint8_t>(data[0]) != 0x68) return false;
    // Length byte = data.size() - 2 (excludes start and length bytes)
    const int declaredLen = static_cast<uint8_t>(data[1]);
    return (data.size() == declaredLen + 2);
}

bool PacketAnalyzer::looksLikeModbusTcp(const QByteArray& data)
{
    if (data.size() < 8) return false;
    // Protocol ID must be 0x0000
    const uint16_t proto = (static_cast<uint8_t>(data[2]) << 8) |
                            static_cast<uint8_t>(data[3]);
    if (proto != 0x0000) return false;
    // Length field must be consistent with actual data size
    const uint16_t len = (static_cast<uint8_t>(data[4]) << 8) |
                          static_cast<uint8_t>(data[5]);
    return (static_cast<int>(len) + 6 == data.size());
}

bool PacketAnalyzer::looksLikeModbusRtu(const QByteArray& data)
{
    if (data.size() < 4) return false;
    // CRC validation: last 2 bytes = CRC of preceding bytes
    const QByteArray payload = data.left(data.size() - 2);
    const uint16_t computed  = modbusRtuCrc(payload);
    const uint16_t stored    =
        static_cast<uint16_t>(
            static_cast<uint8_t>(data[data.size() - 2]) |
            (static_cast<uint8_t>(data[data.size() - 1]) << 8));
    return computed == stored;
}

ProtocolHint PacketAnalyzer::detectProtocol(const QByteArray& data)
{
    // Order matters: IEC-104 has the most distinctive header
    if (looksLikeIEC104(data))    return ProtocolHint::IEC104;
    if (looksLikeModbusTcp(data)) return ProtocolHint::ModbusTcp;
    if (looksLikeModbusRtu(data)) return ProtocolHint::ModbusRtu;
    return ProtocolHint::Unknown;
}

// ──────────────────────────────────────────────────────────────
// Internal Modbus TCP decoder
// ──────────────────────────────────────────────────────────────

AnalyzedPacket PacketAnalyzer::decodeModbusTcpInternal(const QByteArray& data,
                                                        int id,
                                                        FrameDirection dir) const
{
    AnalyzedPacket p;
    p.id        = id;
    p.timestamp = QDateTime::currentDateTime();
    p.protocol  = ProtocolHint::ModbusTcp;
    p.direction = dir;
    p.raw       = data;

    if (data.size() < 8) {
        p.valid       = false;
        p.errorReason = QStringLiteral("Modbus TCP: frame too short (%1 bytes, min 8)").arg(data.size());
        p.summary     = p.errorReason;
        return p;
    }

    const uint16_t transactionId = (static_cast<uint8_t>(data[0]) << 8) | static_cast<uint8_t>(data[1]);
    const uint16_t protocolId    = (static_cast<uint8_t>(data[2]) << 8) | static_cast<uint8_t>(data[3]);
    const uint16_t length        = (static_cast<uint8_t>(data[4]) << 8) | static_cast<uint8_t>(data[5]);
    const uint8_t  unitId        =  static_cast<uint8_t>(data[6]);
    const uint8_t  fc            =  static_cast<uint8_t>(data[7]);

    if (protocolId != 0x0000) {
        p.valid       = false;
        p.errorReason = QStringLiteral("Modbus TCP: invalid protocol id 0x%1 (expected 0x0000)")
                        .arg(protocolId, 4, 16, QChar('0'));
        p.summary     = p.errorReason;
        return p;
    }

    p.fields[QStringLiteral("transaction_id")] = transactionId;
    p.fields[QStringLiteral("protocol_id")]    = protocolId;
    p.fields[QStringLiteral("length")]         = length;
    p.fields[QStringLiteral("unit_id")]        = unitId;
    p.fields[QStringLiteral("function_code")]  = fc;

    // Exception response (FC | 0x80)
    if (fc & 0x80) {
        const int baseFc = fc & 0x7F;
        p.fields[QStringLiteral("function_name")]  = modbusFunctionName(baseFc);
        p.fields[QStringLiteral("is_exception")]   = true;
        if (data.size() >= 9) {
            const int exCode = static_cast<uint8_t>(data[8]);
            p.fields[QStringLiteral("exception_code")] = exCode;
            p.fields[QStringLiteral("exception_name")] = modbusExceptionName(exCode);
            p.summary = QStringLiteral("Modbus TCP Exception — FC %1 %2: %3 (unit=%4)")
                        .arg(baseFc).arg(modbusFunctionName(baseFc))
                        .arg(modbusExceptionName(exCode)).arg(unitId);
        } else {
            p.summary = QStringLiteral("Modbus TCP Exception — FC 0x%1 (unit=%2)")
                        .arg(fc, 2, 16, QChar('0')).arg(unitId);
        }
        p.valid = true;
        return p;
    }

    p.fields[QStringLiteral("function_name")] = modbusFunctionName(fc);
    p.fields[QStringLiteral("is_exception")]  = false;

    // PDU data: bytes after unit_id and FC
    const QByteArray pdu = data.mid(8);

    // Decode request fields for FC 01–04 and 0F/10 (start_address + quantity)
    if ((fc >= 0x01 && fc <= 0x04) || fc == 0x0F || fc == 0x10) {
        if (pdu.size() >= 4) {
            const int startAddr = (static_cast<uint8_t>(pdu[0]) << 8) | static_cast<uint8_t>(pdu[1]);
            const int quantity  = (static_cast<uint8_t>(pdu[2]) << 8) | static_cast<uint8_t>(pdu[3]);
            p.fields[QStringLiteral("start_address")] = startAddr;
            p.fields[QStringLiteral("quantity")]      = quantity;
        }
    }

    // FC 05, 06: output address + value
    if (fc == 0x05 || fc == 0x06) {
        if (pdu.size() >= 4) {
            const int outAddr = (static_cast<uint8_t>(pdu[0]) << 8) | static_cast<uint8_t>(pdu[1]);
            const int value   = (static_cast<uint8_t>(pdu[2]) << 8) | static_cast<uint8_t>(pdu[3]);
            p.fields[QStringLiteral("start_address")] = outAddr;
            p.fields[QStringLiteral("value")]         = value;
        }
    }

    // FC 16 (0x10) also has write values — already decoded start+quantity above
    if (fc == 0x10 && pdu.size() >= 5) {
        p.fields[QStringLiteral("byte_count")] = static_cast<uint8_t>(pdu[4]);
    }

    p.valid   = true;
    p.summary = QStringLiteral("Modbus TCP FC%1 %2 (unit=%3, txid=%4)")
                .arg(fc, 2, 16, QChar('0'))
                .arg(modbusFunctionName(fc))
                .arg(unitId)
                .arg(transactionId);
    return p;
}

// ──────────────────────────────────────────────────────────────
// Internal Modbus RTU decoder
// ──────────────────────────────────────────────────────────────

AnalyzedPacket PacketAnalyzer::decodeModbusRtuInternal(const QByteArray& data,
                                                        int id,
                                                        FrameDirection dir) const
{
    AnalyzedPacket p;
    p.id        = id;
    p.timestamp = QDateTime::currentDateTime();
    p.protocol  = ProtocolHint::ModbusRtu;
    p.direction = dir;
    p.raw       = data;

    if (data.size() < 4) {
        p.valid       = false;
        p.errorReason = QStringLiteral("Modbus RTU: frame too short (%1 bytes, min 4)").arg(data.size());
        p.summary     = p.errorReason;
        return p;
    }

    // CRC check: last 2 bytes, stored little-endian
    const QByteArray payload   = data.left(data.size() - 2);
    const uint16_t   computed  = modbusRtuCrc(payload);
    const uint16_t   stored    =
        static_cast<uint16_t>(
            static_cast<uint8_t>(data[data.size() - 2]) |
            (static_cast<uint8_t>(data[data.size() - 1]) << 8));
    const bool crcValid = (computed == stored);

    const uint8_t deviceAddr = static_cast<uint8_t>(data[0]);
    const uint8_t fc         = static_cast<uint8_t>(data[1]);

    p.fields[QStringLiteral("device_address")] = deviceAddr;
    p.fields[QStringLiteral("function_code")]  = fc;
    p.fields[QStringLiteral("crc_valid")]      = crcValid;

    if (!crcValid) {
        p.valid       = false;
        p.errorReason = QStringLiteral("Modbus RTU: CRC mismatch (computed 0x%1, stored 0x%2)")
                        .arg(computed, 4, 16, QChar('0'))
                        .arg(stored,   4, 16, QChar('0'));
        p.fields[QStringLiteral("function_name")] = modbusFunctionName(fc);
        p.summary = QStringLiteral("Modbus RTU CRC ERROR — addr=%1 FC=0x%2")
                    .arg(deviceAddr).arg(fc, 2, 16, QChar('0'));
        return p;
    }

    // Exception response
    if (fc & 0x80) {
        const int baseFc = fc & 0x7F;
        p.fields[QStringLiteral("function_name")] = modbusFunctionName(baseFc);
        p.fields[QStringLiteral("is_exception")]  = true;
        if (data.size() >= 3) {
            const int exCode = static_cast<uint8_t>(data[2]);
            p.fields[QStringLiteral("exception_code")] = exCode;
            p.fields[QStringLiteral("exception_name")] = modbusExceptionName(exCode);
            p.summary = QStringLiteral("Modbus RTU Exception — FC %1 %2: %3 (addr=%4)")
                        .arg(baseFc).arg(modbusFunctionName(baseFc))
                        .arg(modbusExceptionName(exCode)).arg(deviceAddr);
        } else {
            p.summary = QStringLiteral("Modbus RTU Exception — FC 0x%1 (addr=%2)")
                        .arg(fc, 2, 16, QChar('0')).arg(deviceAddr);
        }
        p.valid = true;
        return p;
    }

    p.fields[QStringLiteral("function_name")] = modbusFunctionName(fc);
    p.fields[QStringLiteral("is_exception")]  = false;

    // PDU data: bytes between FC and CRC
    const QByteArray pdu = data.mid(2, data.size() - 4);

    if ((fc >= 0x01 && fc <= 0x04) || fc == 0x0F || fc == 0x10) {
        if (pdu.size() >= 4) {
            const int startAddr = (static_cast<uint8_t>(pdu[0]) << 8) | static_cast<uint8_t>(pdu[1]);
            const int quantity  = (static_cast<uint8_t>(pdu[2]) << 8) | static_cast<uint8_t>(pdu[3]);
            p.fields[QStringLiteral("start_address")] = startAddr;
            p.fields[QStringLiteral("quantity")]      = quantity;
        }
    }

    if (fc == 0x05 || fc == 0x06) {
        if (pdu.size() >= 4) {
            const int outAddr = (static_cast<uint8_t>(pdu[0]) << 8) | static_cast<uint8_t>(pdu[1]);
            const int value   = (static_cast<uint8_t>(pdu[2]) << 8) | static_cast<uint8_t>(pdu[3]);
            p.fields[QStringLiteral("start_address")] = outAddr;
            p.fields[QStringLiteral("value")]         = value;
        }
    }

    p.valid   = true;
    p.summary = QStringLiteral("Modbus RTU FC%1 %2 (addr=%3)")
                .arg(fc, 2, 16, QChar('0'))
                .arg(modbusFunctionName(fc))
                .arg(deviceAddr);
    return p;
}

// ──────────────────────────────────────────────────────────────
// Internal IEC-104 decoder
//
// Reuses IEC104Client::describeApdu() for the summary line.
// Byte-level field extraction avoids needing a live connection.
//
// APCI layout (all frames):
//   Byte 0: Start (0x68)
//   Byte 1: Length (number of following bytes, ≥ 4)
//   Bytes 2-5: Control fields (CF1–CF4)
//
// I-frame (CF1 bit0 == 0):
//   Tx seq = (CF2 << 7) | (CF1 >> 1)
//   Rx seq = (CF4 << 7) | (CF3 >> 1)
//   Bytes 6+: ASDU
//     Byte 6:  TypeID
//     Byte 7:  VSQ (SQ bit + number of objects)
//     Byte 8:  COT (low 6 bits = cause, bits 6-7 = flags)
//     Byte 9:  Originator address
//     Bytes 10-11: Common Address (LE)
// ──────────────────────────────────────────────────────────────

AnalyzedPacket PacketAnalyzer::decodeIEC104Internal(const QByteArray& data,
                                                     int id,
                                                     FrameDirection dir) const
{
    AnalyzedPacket p;
    p.id        = id;
    p.timestamp = QDateTime::currentDateTime();
    p.protocol  = ProtocolHint::IEC104;
    p.direction = dir;
    p.raw       = data;

    if (data.size() < 6) {
        p.valid       = false;
        p.errorReason = QStringLiteral("IEC-104: frame too short (%1 bytes, min 6)").arg(data.size());
        p.summary     = p.errorReason;
        return p;
    }

    if (static_cast<uint8_t>(data[0]) != 0x68) {
        p.valid       = false;
        p.errorReason = QStringLiteral("IEC-104: invalid start byte 0x%1 (expected 0x68)")
                        .arg(static_cast<uint8_t>(data[0]), 2, 16, QChar('0'));
        p.summary     = p.errorReason;
        return p;
    }

    const uint8_t cf1 = static_cast<uint8_t>(data[2]);
    const uint8_t cf2 = static_cast<uint8_t>(data[3]);
    const uint8_t cf3 = static_cast<uint8_t>(data[4]);
    const uint8_t cf4 = static_cast<uint8_t>(data[5]);

    if ((cf1 & 0x01) == 0) {
        // I-frame
        const int txSeq = ((cf2 << 7) | (cf1 >> 1)) & 0x7FFF;
        const int rxSeq = ((cf4 << 7) | (cf3 >> 1)) & 0x7FFF;

        p.fields[QStringLiteral("frame_type")] = QStringLiteral("I");
        p.fields[QStringLiteral("tx_seq")]     = txSeq;
        p.fields[QStringLiteral("rx_seq")]     = rxSeq;

        if (data.size() >= 12) {
            const int typeId     =  static_cast<uint8_t>(data[6]);
            const int vsq        =  static_cast<uint8_t>(data[7]);
            const int numObjects = vsq & 0x7F;
            const int cotRaw     =  static_cast<uint8_t>(data[8]);
            const int cot        = cotRaw & 0x3F;
            const int commonAddr =  static_cast<uint8_t>(data[10]) |
                                   (static_cast<uint8_t>(data[11]) << 8);

            p.fields[QStringLiteral("type_id")]       = typeId;
            p.fields[QStringLiteral("num_objects")]   = numObjects;
            p.fields[QStringLiteral("cot")]           = cot;
            p.fields[QStringLiteral("common_address")]= commonAddr;

            // Reuse IEC104DataPoint::typeName() for the ASDU type string
#ifdef RTU_IEC104_ENABLED
            p.fields[QStringLiteral("type_name")] =
                iec104::IEC104Client::typeIdToString(typeId);
#else
            p.fields[QStringLiteral("type_name")] =
                QStringLiteral("TypeID_%1").arg(typeId);
#endif
            p.summary = QStringLiteral("IEC-104 I[%1/%2] TypeID=%3 %4 CA=%5 COT=%6")
                        .arg(txSeq).arg(rxSeq)
                        .arg(typeId)
                        .arg(p.fields[QStringLiteral("type_name")].toString())
                        .arg(commonAddr).arg(cot);
        } else {
            p.summary = QStringLiteral("IEC-104 I-frame [Tx=%1 Rx=%2]").arg(txSeq).arg(rxSeq);
        }

    } else if ((cf1 & 0x03) == 0x01) {
        // S-frame
        const int rxSeq = ((cf4 << 7) | (cf3 >> 1)) & 0x7FFF;
        p.fields[QStringLiteral("frame_type")] = QStringLiteral("S");
        p.fields[QStringLiteral("rx_seq")]     = rxSeq;
        p.summary = QStringLiteral("IEC-104 S-frame [Rx=%1]").arg(rxSeq);

    } else {
        // U-frame
        QString uFunc;
        if      (cf1 & 0x04) uFunc = QStringLiteral("STARTDT_act");
        else if (cf1 & 0x08) uFunc = QStringLiteral("STARTDT_con");
        else if (cf1 & 0x10) uFunc = QStringLiteral("STOPDT_act");
        else if (cf1 & 0x20) uFunc = QStringLiteral("STOPDT_con");
        else if (cf1 & 0x40) uFunc = QStringLiteral("TESTFR_act");
        else if (cf1 & 0x80) uFunc = QStringLiteral("TESTFR_con");
        else                 uFunc = QStringLiteral("U_0x%1").arg(cf1, 2, 16, QChar('0'));

        p.fields[QStringLiteral("frame_type")] = QStringLiteral("U");
        p.fields[QStringLiteral("u_function")] = uFunc;
        p.summary = QStringLiteral("IEC-104 U-frame [%1]").arg(uFunc);
    }

    p.valid = true;
    return p;
}

// ──────────────────────────────────────────────────────────────
// Filter application
// ──────────────────────────────────────────────────────────────

bool PacketAnalyzer::applyFilter(const AnalyzedPacket& p, const PacketFilter& f)
{
    if (f.isEmpty()) return true;

    if (f.protocol.has_value()  && p.protocol  != *f.protocol)  return false;
    if (f.direction.has_value() && p.direction != *f.direction) return false;
    if (f.validOnly && !p.valid)                                 return false;

    if (f.modbusFunction != -1) {
        const QVariant fcVar = p.fields.value(QStringLiteral("function_code"));
        if (!fcVar.isValid() || fcVar.toInt() != f.modbusFunction) return false;
    }

    if (f.modbusAddress != -1) {
        const QVariant addrVar = p.fields.value(QStringLiteral("start_address"));
        if (!addrVar.isValid() || addrVar.toInt() != f.modbusAddress) return false;
    }

    if (f.modbusUnitId != -1) {
        const QVariant uid = p.fields.value(QStringLiteral("unit_id"),
                             p.fields.value(QStringLiteral("device_address")));
        if (!uid.isValid() || uid.toInt() != f.modbusUnitId) return false;
    }

    if (f.iec104TypeId != -1) {
        const QVariant tidVar = p.fields.value(QStringLiteral("type_id"));
        if (!tidVar.isValid() || tidVar.toInt() != f.iec104TypeId) return false;
    }

    if (f.iec104CommonAddr != -1) {
        const QVariant caVar = p.fields.value(QStringLiteral("common_address"));
        if (!caVar.isValid() || caVar.toInt() != f.iec104CommonAddr) return false;
    }

    if (f.since.isValid()  && p.timestamp < f.since) return false;
    if (f.until.isValid()  && p.timestamp > f.until) return false;

    return true;
}

// ──────────────────────────────────────────────────────────────
// Constructor
// ──────────────────────────────────────────────────────────────

PacketAnalyzer::PacketAnalyzer(QObject* parent)
    : QObject(parent)
{
    qRegisterMetaType<rtu::core::analyzer::AnalyzedPacket>();
    qRegisterMetaType<rtu::core::analyzer::ProtocolHint>();
}

// ──────────────────────────────────────────────────────────────
// Capture control
// ──────────────────────────────────────────────────────────────

void PacketAnalyzer::startCapture()
{
    if (m_capturing) return;
    m_capturing = true;
    emit captureStarted();
}

void PacketAnalyzer::stopCapture()
{
    if (!m_capturing) return;
    m_capturing = false;
    emit captureStopped();
}

bool PacketAnalyzer::isCapturing() const { return m_capturing; }

// ──────────────────────────────────────────────────────────────
// Input
// ──────────────────────────────────────────────────────────────

void PacketAnalyzer::processTcpPacket(const QByteArray& data, FrameDirection dir)
{
    // Auto-detect between IEC-104 and Modbus TCP on the same transport
    const ProtocolHint hint = detectProtocol(data);
    AnalyzedPacket pkt;

    if (hint == ProtocolHint::IEC104)
        pkt = decodeIEC104Internal(data, m_nextId, dir);
    else
        pkt = decodeModbusTcpInternal(data, m_nextId, dir);

    storePacket(pkt);
}

void PacketAnalyzer::processSerialData(const QByteArray& data, FrameDirection dir)
{
    // Serial data is always treated as Modbus RTU
    AnalyzedPacket pkt = decodeModbusRtuInternal(data, m_nextId, dir);
    storePacket(pkt);
}

// ──────────────────────────────────────────────────────────────
// Public decode — standalone (assigns a temporary id of 0)
// ──────────────────────────────────────────────────────────────

AnalyzedPacket PacketAnalyzer::decodeModbusTcpPacket(const QByteArray& data, FrameDirection dir) const
{
    return decodeModbusTcpInternal(data, 0, dir);
}

AnalyzedPacket PacketAnalyzer::decodeModbusRtuPacket(const QByteArray& data, FrameDirection dir) const
{
    return decodeModbusRtuInternal(data, 0, dir);
}

AnalyzedPacket PacketAnalyzer::decodeIEC104Packet(const QByteArray& data, FrameDirection dir) const
{
    return decodeIEC104Internal(data, 0, dir);
}

AnalyzedPacket PacketAnalyzer::autoDecodePacket(const QByteArray& data, FrameDirection dir) const
{
    const ProtocolHint hint = detectProtocol(data);
    switch (hint) {
        case ProtocolHint::IEC104:    return decodeIEC104Internal(data, 0, dir);
        case ProtocolHint::ModbusTcp: return decodeModbusTcpInternal(data, 0, dir);
        case ProtocolHint::ModbusRtu: return decodeModbusRtuInternal(data, 0, dir);
        default:
            AnalyzedPacket p;
            p.id        = 0;
            p.timestamp = QDateTime::currentDateTime();
            p.protocol  = ProtocolHint::Unknown;
            p.direction = dir;
            p.raw       = data;
            p.valid     = false;
            p.errorReason = QStringLiteral("Could not detect protocol");
            p.summary     = p.errorReason;
            return p;
    }
}

// ──────────────────────────────────────────────────────────────
// Filter
// ──────────────────────────────────────────────────────────────

void         PacketAnalyzer::setFilter(const PacketFilter& f) { m_filter = f; }
void         PacketAnalyzer::clearFilter()                    { m_filter = {}; }
PacketFilter PacketAnalyzer::currentFilter() const            { return m_filter; }

// ──────────────────────────────────────────────────────────────
// Storage
// ──────────────────────────────────────────────────────────────

void PacketAnalyzer::storePacket(const AnalyzedPacket& packet)
{
    if (!m_capturing) return;

    AnalyzedPacket p  = packet;
    p.id              = m_nextId++;

    // Enforce buffer cap — drop oldest first
    if (m_maxPackets > 0 && m_packets.size() >= m_maxPackets)
        m_packets.removeFirst();

    m_packets.append(p);
    emit packetStored(p);
}

void PacketAnalyzer::clearPackets()
{
    m_packets.clear();
    m_nextId = 1;
    emit packetsCleared();
}

int  PacketAnalyzer::packetCount() const { return m_packets.size(); }
void PacketAnalyzer::setMaxPackets(int max) { m_maxPackets = max; }
int  PacketAnalyzer::maxPackets()  const { return m_maxPackets; }

// ──────────────────────────────────────────────────────────────
// Query
// ──────────────────────────────────────────────────────────────

QVector<AnalyzedPacket> PacketAnalyzer::getPackets() const
{
    return getPackets(m_filter);
}

QVector<AnalyzedPacket> PacketAnalyzer::getPackets(const PacketFilter& filter) const
{
    if (filter.isEmpty()) {
        if (filter.maxResults > 0 && m_packets.size() > filter.maxResults)
            return m_packets.mid(m_packets.size() - filter.maxResults);
        return m_packets;
    }

    QVector<AnalyzedPacket> result;
    result.reserve(m_packets.size());

    for (const auto& p : m_packets) {
        if (applyFilter(p, filter)) {
            result.append(p);
            if (filter.maxResults > 0 && result.size() >= filter.maxResults)
                break;
        }
    }
    return result;
}

std::optional<AnalyzedPacket> PacketAnalyzer::getPacketById(int id) const
{
    for (const auto& p : m_packets) {
        if (p.id == id) return p;
    }
    return std::nullopt;
}

// ──────────────────────────────────────────────────────────────
// Statistics
// ──────────────────────────────────────────────────────────────

QVariantMap PacketAnalyzer::statistics() const
{
    return statistics(PacketFilter{});
}

QVariantMap PacketAnalyzer::statistics(const PacketFilter& filter) const
{
    const QVector<AnalyzedPacket> packets = getPackets(filter);

    int modbusTcp = 0, modbusRtu = 0, iec104 = 0, unknown = 0, valid = 0, invalid = 0;

    for (const auto& p : packets) {
        switch (p.protocol) {
            case ProtocolHint::ModbusTcp: ++modbusTcp; break;
            case ProtocolHint::ModbusRtu: ++modbusRtu; break;
            case ProtocolHint::IEC104:    ++iec104;    break;
            default:                      ++unknown;   break;
        }
        p.valid ? ++valid : ++invalid;
    }

    const int total = packets.size();
    QVariantMap stats;
    stats[QStringLiteral("total")]      = total;
    stats[QStringLiteral("modbus_tcp")] = modbusTcp;
    stats[QStringLiteral("modbus_rtu")] = modbusRtu;
    stats[QStringLiteral("iec104")]     = iec104;
    stats[QStringLiteral("unknown")]    = unknown;
    stats[QStringLiteral("valid")]      = valid;
    stats[QStringLiteral("invalid")]    = invalid;
    stats[QStringLiteral("error_rate")] = total > 0
                                        ? static_cast<double>(invalid) / total
                                        : 0.0;
    return stats;
}

} // namespace rtu::core::analyzer
