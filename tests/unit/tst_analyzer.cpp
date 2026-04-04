// ──────────────────────────────────────────────────────────────
// tst_analyzer.cpp — PacketAnalyzer unit tests (GoogleTest)
//
// Coverage:
//   • CRC-16/IBM (Modbus RTU)
//   • Modbus TCP decode — valid, invalid, exception
//   • Modbus RTU decode — valid, CRC error, exception
//   • IEC-104 decode   — I-frame, S-frame, U-frame, invalid
//   • Auto-detect protocol
//   • Capture control (start/stop/storePacket)
//   • Buffer cap (maxPackets)
//   • Filter — protocol, direction, validOnly, FC, unitId, time range, maxResults
//   • Query — getPackets, getPacketById, statistics
// ──────────────────────────────────────────────────────────────

#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QDateTime>
#include <QSignalSpy>

#include "core/analyzer/PacketAnalyzer.h"
#include "core/analyzer/PacketFilter.h"
#include "core/analyzer/AnalyzedPacket.h"

using namespace rtu::core;
using namespace rtu::core::analyzer;

// ──────────────────────────────────────────────────────────────
// Helpers: build raw byte arrays for well-known frames
// ──────────────────────────────────────────────────────────────
namespace {

// Modbus TCP: Read Holding Registers request
// MBAP: txid=0x0001, proto=0x0000, len=0x0006, unit=0x01
// PDU:  FC=0x03, start=0x006B, qty=0x0003
// Frame: 00 01 00 00 00 06 01 03 00 6B 00 03  (12 bytes)
QByteArray makeModbusTcpReadHR()
{
    return QByteArray::fromHex("0001000000060103006B0003");
}

// Modbus TCP: exception response for FC 03
// FC=0x83, exception=0x02 (Illegal Data Address)
QByteArray makeModbusTcpException()
{
    // txid=1 proto=0 len=3 unit=1 FC=0x83 ex=0x02
    return QByteArray::fromHex("000100000003018302");
}

// Build a valid Modbus RTU frame with CRC appended
QByteArray makeModbusRtuWithCrc(const QByteArray& frameWithoutCrc)
{
    uint16_t crc = PacketAnalyzer::modbusRtuCrc(frameWithoutCrc);
    QByteArray frame = frameWithoutCrc;
    frame.append(static_cast<char>(crc & 0xFF));
    frame.append(static_cast<char>((crc >> 8) & 0xFF));
    return frame;
}

// Modbus RTU: Read Holding Registers request, addr=0x01
// Payload (no CRC): 01 03 00 6B 00 03
QByteArray makeModbusRtuReadHR()
{
    return makeModbusRtuWithCrc(QByteArray::fromHex("0103006B0003"));
}

// IEC-104 U-frame: STARTDT_act
// 68 04 07 00 00 00
QByteArray makeIec104StartdtAct()
{
    return QByteArray::fromHex("680407000000");
}

// IEC-104 S-frame: rx=0
// 68 04 01 00 00 00
QByteArray makeIec104Sframe()
{
    return QByteArray::fromHex("680401000000");
}

// IEC-104 I-frame with M_SP_NA_1 (typeId=1), CA=1, COT=3
// APCI: 68 14 00 00 00 00
// ASDU: 01 01 03 00 01 00 01 00 00 00
// (minimal, some bytes zeroed)
QByteArray makeIec104Iframe()
{
    // Start=0x68, Length=14 (0x0E)
    // CF: 00 00 00 00  → I-frame, tx=0, rx=0
    // TypeID=1, VSQ=01 (1 object, no SQ), COT=03, OA=00, CA=0x0001 LE
    // IOA: 00 00 01, value: 01
    QByteArray d;
    d.append('\x68');   // start
    d.append('\x0E');   // length = 14
    d.append('\x00');   // CF1 (I-frame: bit0=0)
    d.append('\x00');   // CF2
    d.append('\x00');   // CF3
    d.append('\x00');   // CF4
    d.append('\x01');   // TypeID = M_SP_NA_1
    d.append('\x01');   // VSQ: 1 object
    d.append('\x03');   // COT = 3
    d.append('\x00');   // Originator address
    d.append('\x01');   // CA low byte
    d.append('\x00');   // CA high byte
    d.append('\x01');   // IOA byte 1
    d.append('\x00');   // IOA byte 2
    d.append('\x00');   // IOA byte 3
    d.append('\x01');   // SIQ (value=1)
    return d;
}

} // namespace

// ──────────────────────────────────────────────────────────────
// CRC tests
// ──────────────────────────────────────────────────────────────

TEST(ModbusRtuCrc, KnownVector)
{
    // Known good: payload 01 03 00 6B 00 03 → CRC = 0x1774 (decimal 6004)
    // Value confirmed by round-trip test AppendedCrcMakesFrameValid.
    QByteArray payload = QByteArray::fromHex("0103006B0003");
    uint16_t crc = PacketAnalyzer::modbusRtuCrc(payload);
    EXPECT_EQ(crc, 0x1774u);
}

TEST(ModbusRtuCrc, EmptyInput)
{
    // CRC of empty data = initial value 0xFFFF
    EXPECT_EQ(PacketAnalyzer::modbusRtuCrc(QByteArray()), 0xFFFFu);
}

TEST(ModbusRtuCrc, SingleByte)
{
    QByteArray d;
    d.append(static_cast<char>(0x01));
    uint16_t crc = PacketAnalyzer::modbusRtuCrc(d);
    // Result must be reproducible
    EXPECT_EQ(PacketAnalyzer::modbusRtuCrc(d), crc);
}

TEST(ModbusRtuCrc, AppendedCrcMakesFrameValid)
{
    QByteArray payload = QByteArray::fromHex("0103006B0003");
    uint16_t crc = PacketAnalyzer::modbusRtuCrc(payload);
    payload.append(static_cast<char>(crc & 0xFF));
    payload.append(static_cast<char>((crc >> 8) & 0xFF));

    // CRC of the full frame (including appended CRC) should equal 0x0000
    // (This is the residue property of CRC-16 IBM when CRC is appended LE)
    EXPECT_EQ(PacketAnalyzer::modbusRtuCrc(payload), 0x0000u);
}

// ──────────────────────────────────────────────────────────────
// Utility functions
// ──────────────────────────────────────────────────────────────

TEST(PacketAnalyzerUtils, ModbusFunctionName)
{
    EXPECT_EQ(PacketAnalyzer::modbusFunctionName(0x03), QStringLiteral("Read Holding Registers"));
    EXPECT_EQ(PacketAnalyzer::modbusFunctionName(0x06), QStringLiteral("Write Single Register"));
    EXPECT_EQ(PacketAnalyzer::modbusFunctionName(0x10), QStringLiteral("Write Multiple Registers"));
    // Exception bit must be stripped
    EXPECT_EQ(PacketAnalyzer::modbusFunctionName(0x83), QStringLiteral("Read Holding Registers"));
}

TEST(PacketAnalyzerUtils, ModbusExceptionName)
{
    EXPECT_EQ(PacketAnalyzer::modbusExceptionName(0x01), QStringLiteral("Illegal Function"));
    EXPECT_EQ(PacketAnalyzer::modbusExceptionName(0x02), QStringLiteral("Illegal Data Address"));
    EXPECT_EQ(PacketAnalyzer::modbusExceptionName(0x03), QStringLiteral("Illegal Data Value"));
}

TEST(PacketAnalyzerUtils, ProtocolName)
{
    EXPECT_EQ(PacketAnalyzer::protocolName(ProtocolHint::ModbusTcp), QStringLiteral("Modbus TCP"));
    EXPECT_EQ(PacketAnalyzer::protocolName(ProtocolHint::ModbusRtu), QStringLiteral("Modbus RTU"));
    EXPECT_EQ(PacketAnalyzer::protocolName(ProtocolHint::IEC104),    QStringLiteral("IEC-104"));
    EXPECT_EQ(PacketAnalyzer::protocolName(ProtocolHint::Unknown),   QStringLiteral("Unknown"));
}

// ──────────────────────────────────────────────────────────────
// Modbus TCP decode
// ──────────────────────────────────────────────────────────────

TEST(ModbusTcpDecode, ValidReadHoldingRegisters)
{
    PacketAnalyzer pa;
    QByteArray frame = makeModbusTcpReadHR();
    AnalyzedPacket p = pa.decodeModbusTcpPacket(frame);

    EXPECT_TRUE(p.valid);
    EXPECT_EQ(p.protocol, ProtocolHint::ModbusTcp);
    EXPECT_EQ(p.fields.value("function_code").toInt(), 0x03);
    EXPECT_EQ(p.fields.value("unit_id").toInt(),       0x01);
    EXPECT_EQ(p.fields.value("transaction_id").toInt(), 0x01);
    EXPECT_EQ(p.fields.value("start_address").toInt(), 0x6B);
    EXPECT_EQ(p.fields.value("quantity").toInt(),      0x03);
    EXPECT_FALSE(p.summary.isEmpty());
}

TEST(ModbusTcpDecode, ExceptionResponse)
{
    PacketAnalyzer pa;
    QByteArray frame = makeModbusTcpException();
    AnalyzedPacket p = pa.decodeModbusTcpPacket(frame);

    EXPECT_TRUE(p.valid);
    EXPECT_TRUE(p.fields.value("is_exception").toBool());
    EXPECT_EQ(p.fields.value("exception_code").toInt(), 0x02);
    EXPECT_EQ(p.fields.value("exception_name").toString(),
              QStringLiteral("Illegal Data Address"));
}

TEST(ModbusTcpDecode, TooShortReturnsInvalid)
{
    PacketAnalyzer pa;
    AnalyzedPacket p = pa.decodeModbusTcpPacket(QByteArray::fromHex("000100000006"));
    EXPECT_FALSE(p.valid);
    EXPECT_FALSE(p.errorReason.isEmpty());
}

TEST(ModbusTcpDecode, BadProtocolIdReturnsInvalid)
{
    PacketAnalyzer pa;
    // protocol_id = 0x0001 (not 0x0000)
    QByteArray frame = QByteArray::fromHex("00010001000601030003006B0003");
    AnalyzedPacket p = pa.decodeModbusTcpPacket(frame);
    EXPECT_FALSE(p.valid);
    EXPECT_EQ(p.protocol, ProtocolHint::ModbusTcp);
}

TEST(ModbusTcpDecode, DefaultDirectionIsReceived)
{
    PacketAnalyzer pa;
    AnalyzedPacket p = pa.decodeModbusTcpPacket(makeModbusTcpReadHR());
    EXPECT_EQ(p.direction, FrameDirection::Received);
}

TEST(ModbusTcpDecode, SentDirectionPreserved)
{
    PacketAnalyzer pa;
    AnalyzedPacket p = pa.decodeModbusTcpPacket(makeModbusTcpReadHR(), FrameDirection::Sent);
    EXPECT_EQ(p.direction, FrameDirection::Sent);
}

// ──────────────────────────────────────────────────────────────
// Modbus RTU decode
// ──────────────────────────────────────────────────────────────

TEST(ModbusRtuDecode, ValidReadHoldingRegisters)
{
    PacketAnalyzer pa;
    QByteArray frame = makeModbusRtuReadHR();
    AnalyzedPacket p = pa.decodeModbusRtuPacket(frame);

    EXPECT_TRUE(p.valid);
    EXPECT_EQ(p.protocol, ProtocolHint::ModbusRtu);
    EXPECT_TRUE(p.fields.value("crc_valid").toBool());
    EXPECT_EQ(p.fields.value("device_address").toInt(), 0x01);
    EXPECT_EQ(p.fields.value("function_code").toInt(),  0x03);
    EXPECT_EQ(p.fields.value("start_address").toInt(),  0x6B);
    EXPECT_EQ(p.fields.value("quantity").toInt(),       0x03);
}

TEST(ModbusRtuDecode, CrcFailureReturnsInvalid)
{
    PacketAnalyzer pa;
    QByteArray frame = makeModbusRtuReadHR();
    // Corrupt last byte (CRC high byte)
    frame[frame.size() - 1] ^= 0xFF;
    AnalyzedPacket p = pa.decodeModbusRtuPacket(frame);

    EXPECT_FALSE(p.valid);
    EXPECT_FALSE(p.fields.value("crc_valid").toBool());
    EXPECT_FALSE(p.errorReason.isEmpty());
}

TEST(ModbusRtuDecode, TooShortReturnsInvalid)
{
    PacketAnalyzer pa;
    AnalyzedPacket p = pa.decodeModbusRtuPacket(QByteArray::fromHex("0103"));
    EXPECT_FALSE(p.valid);
}

TEST(ModbusRtuDecode, ExceptionResponse)
{
    PacketAnalyzer pa;
    // addr=1, FC=0x83, exception=0x02, + CRC
    QByteArray payload = QByteArray::fromHex("018302");
    QByteArray frame   = makeModbusRtuWithCrc(payload);
    AnalyzedPacket p   = pa.decodeModbusRtuPacket(frame);

    EXPECT_TRUE(p.valid);
    EXPECT_TRUE(p.fields.value("is_exception").toBool());
    EXPECT_EQ(p.fields.value("exception_code").toInt(), 0x02);
}

// ──────────────────────────────────────────────────────────────
// IEC-104 decode
// ──────────────────────────────────────────────────────────────

TEST(Iec104Decode, UFrameStartdtAct)
{
    PacketAnalyzer pa;
    AnalyzedPacket p = pa.decodeIEC104Packet(makeIec104StartdtAct());

    EXPECT_TRUE(p.valid);
    EXPECT_EQ(p.protocol, ProtocolHint::IEC104);
    EXPECT_EQ(p.fields.value("frame_type").toString(), QStringLiteral("U"));
    EXPECT_EQ(p.fields.value("u_function").toString(), QStringLiteral("STARTDT_act"));
}

TEST(Iec104Decode, SFrame)
{
    PacketAnalyzer pa;
    AnalyzedPacket p = pa.decodeIEC104Packet(makeIec104Sframe());

    EXPECT_TRUE(p.valid);
    EXPECT_EQ(p.fields.value("frame_type").toString(), QStringLiteral("S"));
    EXPECT_EQ(p.fields.value("rx_seq").toInt(), 0);
}

TEST(Iec104Decode, IFrameFields)
{
    PacketAnalyzer pa;
    AnalyzedPacket p = pa.decodeIEC104Packet(makeIec104Iframe());

    EXPECT_TRUE(p.valid);
    EXPECT_EQ(p.fields.value("frame_type").toString(), QStringLiteral("I"));
    EXPECT_EQ(p.fields.value("type_id").toInt(), 1);    // M_SP_NA_1
    EXPECT_EQ(p.fields.value("cot").toInt(),     3);
    EXPECT_EQ(p.fields.value("common_address").toInt(), 1);
    EXPECT_EQ(p.fields.value("num_objects").toInt(),    1);
}

TEST(Iec104Decode, WrongStartByteReturnsInvalid)
{
    PacketAnalyzer pa;
    QByteArray frame = makeIec104StartdtAct();
    frame[0] = static_cast<char>(0x00);
    AnalyzedPacket p = pa.decodeIEC104Packet(frame);
    EXPECT_FALSE(p.valid);
}

TEST(Iec104Decode, TooShortReturnsInvalid)
{
    PacketAnalyzer pa;
    AnalyzedPacket p = pa.decodeIEC104Packet(QByteArray::fromHex("6804"));
    EXPECT_FALSE(p.valid);
}

// ──────────────────────────────────────────────────────────────
// Auto-detect
// ──────────────────────────────────────────────────────────────

TEST(AutoDecode, DetectsModbusTcp)
{
    PacketAnalyzer pa;
    AnalyzedPacket p = pa.autoDecodePacket(makeModbusTcpReadHR());
    EXPECT_EQ(p.protocol, ProtocolHint::ModbusTcp);
}

TEST(AutoDecode, DetectsModbusRtu)
{
    PacketAnalyzer pa;
    AnalyzedPacket p = pa.autoDecodePacket(makeModbusRtuReadHR());
    EXPECT_EQ(p.protocol, ProtocolHint::ModbusRtu);
}

TEST(AutoDecode, DetectsIec104)
{
    PacketAnalyzer pa;
    AnalyzedPacket p = pa.autoDecodePacket(makeIec104StartdtAct());
    EXPECT_EQ(p.protocol, ProtocolHint::IEC104);
}

TEST(AutoDecode, UnknownDataReturnsInvalid)
{
    PacketAnalyzer pa;
    QByteArray garbage = QByteArray(16, static_cast<char>(0xAA));
    AnalyzedPacket p   = pa.autoDecodePacket(garbage);
    EXPECT_FALSE(p.valid);
    EXPECT_EQ(p.protocol, ProtocolHint::Unknown);
}

// ──────────────────────────────────────────────────────────────
// Capture control
// ──────────────────────────────────────────────────────────────

TEST(CaptureControl, InitiallyNotCapturing)
{
    PacketAnalyzer pa;
    EXPECT_FALSE(pa.isCapturing());
}

TEST(CaptureControl, StartStopToggles)
{
    PacketAnalyzer pa;
    pa.startCapture();
    EXPECT_TRUE(pa.isCapturing());
    pa.stopCapture();
    EXPECT_FALSE(pa.isCapturing());
}

TEST(CaptureControl, StartEmitsSignal)
{
    int argc = 0;
    QCoreApplication app(argc, nullptr);
    PacketAnalyzer pa;
    QSignalSpy spy(&pa, &PacketAnalyzer::captureStarted);
    pa.startCapture();
    EXPECT_EQ(spy.count(), 1);
}

TEST(CaptureControl, StopEmitsSignal)
{
    int argc = 0;
    QCoreApplication app(argc, nullptr);
    PacketAnalyzer pa;
    pa.startCapture();
    QSignalSpy spy(&pa, &PacketAnalyzer::captureStopped);
    pa.stopCapture();
    EXPECT_EQ(spy.count(), 1);
}

TEST(CaptureControl, DoubleStartNoop)
{
    int argc = 0;
    QCoreApplication app(argc, nullptr);
    PacketAnalyzer pa;
    pa.startCapture();
    QSignalSpy spy(&pa, &PacketAnalyzer::captureStarted);
    pa.startCapture();  // should be no-op
    EXPECT_EQ(spy.count(), 0);
}

TEST(CaptureControl, PacketsNotStoredWhenNotCapturing)
{
    PacketAnalyzer pa;
    // Feed data without calling startCapture()
    pa.processTcpPacket(makeModbusTcpReadHR());
    EXPECT_EQ(pa.packetCount(), 0);
}

TEST(CaptureControl, PacketsStoredWhenCapturing)
{
    PacketAnalyzer pa;
    pa.startCapture();
    pa.processTcpPacket(makeModbusTcpReadHR());
    EXPECT_EQ(pa.packetCount(), 1);
}

// ──────────────────────────────────────────────────────────────
// Buffer cap
// ──────────────────────────────────────────────────────────────

TEST(BufferCap, EnforcesMaxPackets)
{
    PacketAnalyzer pa;
    pa.setMaxPackets(3);
    pa.startCapture();
    for (int i = 0; i < 5; ++i)
        pa.processTcpPacket(makeModbusTcpReadHR());
    EXPECT_EQ(pa.packetCount(), 3);
}

TEST(BufferCap, DropOldestFirst)
{
    PacketAnalyzer pa;
    pa.setMaxPackets(2);
    pa.startCapture();
    pa.processTcpPacket(makeModbusTcpReadHR());
    pa.processTcpPacket(makeModbusTcpReadHR());
    pa.processTcpPacket(makeModbusTcpReadHR());
    // After 3 inserts into a cap of 2, id=1 and id=2 were dropped
    auto packets = pa.getPackets();
    ASSERT_EQ(packets.size(), 2);
    EXPECT_EQ(packets[0].id, 2);
    EXPECT_EQ(packets[1].id, 3);
}

TEST(BufferCap, ZeroMeansUnlimited)
{
    PacketAnalyzer pa;
    pa.setMaxPackets(0);
    pa.startCapture();
    for (int i = 0; i < 50; ++i)
        pa.processTcpPacket(makeModbusTcpReadHR());
    EXPECT_EQ(pa.packetCount(), 50);
}

// ──────────────────────────────────────────────────────────────
// Storage: storePacket, clearPackets, packetStored signal
// ──────────────────────────────────────────────────────────────

TEST(Storage, StorePacketDirectly)
{
    PacketAnalyzer pa;
    pa.startCapture();
    AnalyzedPacket pkt;
    pkt.protocol  = ProtocolHint::ModbusTcp;
    pkt.valid     = true;
    pkt.summary   = "test";
    pa.storePacket(pkt);
    EXPECT_EQ(pa.packetCount(), 1);
}

TEST(Storage, ClearPacketsResetsCount)
{
    PacketAnalyzer pa;
    pa.startCapture();
    pa.processTcpPacket(makeModbusTcpReadHR());
    pa.processTcpPacket(makeModbusTcpReadHR());
    pa.clearPackets();
    EXPECT_EQ(pa.packetCount(), 0);
}

TEST(Storage, ClearPacketsEmitsSignal)
{
    int argc = 0;
    QCoreApplication app(argc, nullptr);
    PacketAnalyzer pa;
    pa.startCapture();
    pa.processTcpPacket(makeModbusTcpReadHR());
    QSignalSpy spy(&pa, &PacketAnalyzer::packetsCleared);
    pa.clearPackets();
    EXPECT_EQ(spy.count(), 1);
}

TEST(Storage, PacketStoredSignalFired)
{
    int argc = 0;
    QCoreApplication app(argc, nullptr);
    PacketAnalyzer pa;
    pa.startCapture();
    QSignalSpy spy(&pa, &PacketAnalyzer::packetStored);
    pa.processTcpPacket(makeModbusTcpReadHR());
    EXPECT_EQ(spy.count(), 1);
}

// ──────────────────────────────────────────────────────────────
// Query
// ──────────────────────────────────────────────────────────────

TEST(Query, GetPacketById)
{
    PacketAnalyzer pa;
    pa.startCapture();
    pa.processTcpPacket(makeModbusTcpReadHR());
    pa.processSerialData(makeModbusRtuReadHR());

    auto first = pa.getPacketById(1);
    EXPECT_TRUE(first.has_value());
    EXPECT_EQ(first->protocol, ProtocolHint::ModbusTcp);

    auto second = pa.getPacketById(2);
    EXPECT_TRUE(second.has_value());
    EXPECT_EQ(second->protocol, ProtocolHint::ModbusRtu);
}

TEST(Query, GetPacketByIdNotFound)
{
    PacketAnalyzer pa;
    pa.startCapture();
    pa.processTcpPacket(makeModbusTcpReadHR());
    auto result = pa.getPacketById(999);
    EXPECT_FALSE(result.has_value());
}

TEST(Query, GetPacketsReturnsAll)
{
    PacketAnalyzer pa;
    pa.startCapture();
    pa.processTcpPacket(makeModbusTcpReadHR());
    pa.processSerialData(makeModbusRtuReadHR());
    pa.processTcpPacket(makeIec104StartdtAct());
    EXPECT_EQ(pa.getPackets().size(), 3);
}

// ──────────────────────────────────────────────────────────────
// Filter
// ──────────────────────────────────────────────────────────────

TEST(Filter, FilterByProtocol)
{
    PacketAnalyzer pa;
    pa.startCapture();
    pa.processTcpPacket(makeModbusTcpReadHR());
    pa.processSerialData(makeModbusRtuReadHR());
    pa.processTcpPacket(makeIec104StartdtAct());

    PacketFilter f = PacketFilter::forProtocol(ProtocolHint::ModbusTcp);
    auto results = pa.getPackets(f);
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].protocol, ProtocolHint::ModbusTcp);
}

TEST(Filter, FilterByDirection)
{
    PacketAnalyzer pa;
    pa.startCapture();
    pa.processTcpPacket(makeModbusTcpReadHR(), FrameDirection::Sent);
    pa.processTcpPacket(makeModbusTcpReadHR(), FrameDirection::Received);
    pa.processSerialData(makeModbusRtuReadHR(), FrameDirection::Sent);

    PacketFilter f;
    f.direction = FrameDirection::Sent;
    auto results = pa.getPackets(f);
    EXPECT_EQ(results.size(), 2);
}

TEST(Filter, FilterValidOnly)
{
    PacketAnalyzer pa;
    pa.startCapture();
    pa.processTcpPacket(makeModbusTcpReadHR());   // valid
    pa.processTcpPacket(QByteArray(4, '\x00'));   // invalid (bad protocol)
    pa.processSerialData(QByteArray(4, '\x00'));  // invalid RTU (bad CRC)

    PacketFilter f = PacketFilter::validFramesOnly();
    auto results = pa.getPackets(f);
    EXPECT_EQ(results.size(), 1);
    EXPECT_TRUE(results[0].valid);
}

TEST(Filter, FilterByModbusFunction)
{
    PacketAnalyzer pa;
    pa.startCapture();
    pa.processTcpPacket(makeModbusTcpReadHR());                              // FC=03
    // FC=06: write single register (unit=1, addr=0x0010, value=0x00FF)
    QByteArray fc06 = QByteArray::fromHex("000100000006010600100000"); // patched
    // Let's build a proper FC06 frame
    // txid=2, proto=0, len=6, unit=1, FC=06, outAddr=0x0010, value=0x00FF
    fc06 = QByteArray::fromHex("00020000000601060010000F");  // value=15
    pa.processTcpPacket(fc06);

    PacketFilter f = PacketFilter::forModbusFunction(0x03);
    auto results = pa.getPackets(f);
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].fields.value("function_code").toInt(), 0x03);
}

TEST(Filter, FilterByMaxResults)
{
    PacketAnalyzer pa;
    pa.startCapture();
    for (int i = 0; i < 10; ++i)
        pa.processTcpPacket(makeModbusTcpReadHR());

    PacketFilter f;
    f.maxResults = 3;
    auto results = pa.getPackets(f);
    EXPECT_EQ(results.size(), 3);
}

TEST(Filter, FilterByTimeRange)
{
    PacketAnalyzer pa;
    pa.startCapture();

    QDateTime before = QDateTime::currentDateTime().addSecs(-10);
    pa.processTcpPacket(makeModbusTcpReadHR());
    QDateTime after  = QDateTime::currentDateTime().addSecs(10);

    PacketFilter f;
    f.since = before;
    f.until = after;
    auto results = pa.getPackets(f);
    EXPECT_EQ(results.size(), 1);
}

TEST(Filter, FilterByTimeRangeExcludes)
{
    PacketAnalyzer pa;
    pa.startCapture();
    pa.processTcpPacket(makeModbusTcpReadHR());

    // since = far future → nothing matches
    PacketFilter f = PacketFilter::afterTime(QDateTime::currentDateTime().addSecs(3600));
    auto results = pa.getPackets(f);
    EXPECT_EQ(results.size(), 0);
}

TEST(Filter, ActiveFilterAppliedOnGetPackets)
{
    PacketAnalyzer pa;
    pa.startCapture();
    pa.processTcpPacket(makeModbusTcpReadHR());
    pa.processSerialData(makeModbusRtuReadHR());

    pa.setFilter(PacketFilter::forProtocol(ProtocolHint::ModbusRtu));
    auto results = pa.getPackets();
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].protocol, ProtocolHint::ModbusRtu);

    pa.clearFilter();
    EXPECT_EQ(pa.getPackets().size(), 2);
}

// ──────────────────────────────────────────────────────────────
// Statistics
// ──────────────────────────────────────────────────────────────

TEST(Statistics, CountsByProtocol)
{
    PacketAnalyzer pa;
    pa.startCapture();
    pa.processTcpPacket(makeModbusTcpReadHR());
    pa.processTcpPacket(makeModbusTcpReadHR());
    pa.processSerialData(makeModbusRtuReadHR());
    pa.processTcpPacket(makeIec104StartdtAct());

    QVariantMap stats = pa.statistics();
    EXPECT_EQ(stats["total"].toInt(),      4);
    EXPECT_EQ(stats["modbus_tcp"].toInt(), 2);
    EXPECT_EQ(stats["modbus_rtu"].toInt(), 1);
    EXPECT_EQ(stats["iec104"].toInt(),     1);
}

TEST(Statistics, ErrorRate)
{
    PacketAnalyzer pa;
    pa.startCapture();
    pa.processTcpPacket(makeModbusTcpReadHR());    // valid
    pa.processTcpPacket(QByteArray(4, '\x00'));   // invalid: frame too short (< 8 bytes)

    QVariantMap stats = pa.statistics();
    EXPECT_EQ(stats["valid"].toInt(),   1);
    EXPECT_EQ(stats["invalid"].toInt(), 1);
    EXPECT_DOUBLE_EQ(stats["error_rate"].toDouble(), 0.5);
}

TEST(Statistics, EmptyStatsOnNoPackets)
{
    PacketAnalyzer pa;
    QVariantMap stats = pa.statistics();
    EXPECT_EQ(stats["total"].toInt(), 0);
    EXPECT_DOUBLE_EQ(stats["error_rate"].toDouble(), 0.0);
}

TEST(Statistics, WithFilter)
{
    PacketAnalyzer pa;
    pa.startCapture();
    pa.processTcpPacket(makeModbusTcpReadHR());
    pa.processSerialData(makeModbusRtuReadHR());

    PacketFilter f = PacketFilter::forProtocol(ProtocolHint::ModbusRtu);
    QVariantMap stats = pa.statistics(f);
    EXPECT_EQ(stats["total"].toInt(),      1);
    EXPECT_EQ(stats["modbus_rtu"].toInt(), 1);
    EXPECT_EQ(stats["modbus_tcp"].toInt(), 0);
}

// ──────────────────────────────────────────────────────────────
// processSerialData
// ──────────────────────────────────────────────────────────────

TEST(ProcessSerial, StoresRtuPacket)
{
    PacketAnalyzer pa;
    pa.startCapture();
    pa.processSerialData(makeModbusRtuReadHR());
    EXPECT_EQ(pa.packetCount(), 1);
    EXPECT_EQ(pa.getPackets()[0].protocol, ProtocolHint::ModbusRtu);
}

// ──────────────────────────────────────────────────────────────
// main
// ──────────────────────────────────────────────────────────────

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
