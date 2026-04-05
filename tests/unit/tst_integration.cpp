// ──────────────────────────────────────────────────────────────
// tst_integration.cpp — Controller integration tests (GoogleTest)
//
// Coverage:
//   • ModbusController — property defaults, connect guards, read/write guards
//   • SerialController — property defaults, open guards, send guards
//   • AnalyzerController — capture control, feed data, packet model, stats
//   • SimulatorController — start/stop/reset, register map, IOA map, random
//   • IECController       — property defaults, connect guard, sendCommand guard
//   • SessionViewModel    — delegation to ModbusController
// ──────────────────────────────────────────────────────────────

#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QSignalSpy>

#include "ui/viewmodels/ModbusController.h"
#include "ui/viewmodels/SerialController.h"
#include "ui/viewmodels/AnalyzerController.h"
#include "core/analyzer/PacketAnalyzer.h"
#include "ui/viewmodels/SimulatorController.h"
#include "ui/viewmodels/IECController.h"
#include "ui/viewmodels/SessionViewModel.h"

using namespace rtu::ui;

// ──────────────────────────────────────────────────────────────
// ModbusController
// ──────────────────────────────────────────────────────────────

TEST(ModbusController, DefaultsNotConnected)
{
    ModbusController c;
    EXPECT_FALSE(c.connected());
}

TEST(ModbusController, DefaultModeRTU)
{
    ModbusController c;
    EXPECT_EQ(c.mode(), QStringLiteral("RTU"));
}

TEST(ModbusController, SetMode)
{
    ModbusController c;
    c.setMode(QStringLiteral("TCP"));
    EXPECT_EQ(c.mode(), QStringLiteral("TCP"));
}

TEST(ModbusController, SetModeEmitsSignal)
{
    int argc = 0; QCoreApplication app(argc, nullptr);
    ModbusController c;
    QSignalSpy spy(&c, &ModbusController::modeChanged);
    c.setMode(QStringLiteral("TCP"));
    EXPECT_EQ(spy.count(), 1);
}

TEST(ModbusController, SetSameModeSilent)
{
    int argc = 0; QCoreApplication app(argc, nullptr);
    ModbusController c;
    QSignalSpy spy(&c, &ModbusController::modeChanged);
    c.setMode(QStringLiteral("RTU"));  // already RTU
    EXPECT_EQ(spy.count(), 0);
}

TEST(ModbusController, SetHostEmitsConfigChanged)
{
    int argc = 0; QCoreApplication app(argc, nullptr);
    ModbusController c;
    QSignalSpy spy(&c, &ModbusController::configChanged);
    c.setHost(QStringLiteral("10.0.0.1"));
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(c.host(), QStringLiteral("10.0.0.1"));
}

TEST(ModbusController, ReadWhileDisconnectedEmitsError)
{
    int argc = 0; QCoreApplication app(argc, nullptr);
    ModbusController c;
    QSignalSpy spy(&c, &ModbusController::errorOccurred);
    c.read(0, 10);
    EXPECT_EQ(spy.count(), 1);
    EXPECT_FALSE(c.lastError().isEmpty());
}

TEST(ModbusController, WriteWhileDisconnectedEmitsError)
{
    int argc = 0; QCoreApplication app(argc, nullptr);
    ModbusController c;
    QSignalSpy spy(&c, &ModbusController::errorOccurred);
    c.write(0, 42);
    EXPECT_EQ(spy.count(), 1);
}

TEST(ModbusController, ClearLogEmptiesModels)
{
    int argc = 0; QCoreApplication app(argc, nullptr);
    ModbusController c;
    c.clearLog();
    EXPECT_TRUE(c.responseModel().isEmpty());
    EXPECT_TRUE(c.rawLog().isEmpty());
}

// ──────────────────────────────────────────────────────────────
// SerialController
// ──────────────────────────────────────────────────────────────

TEST(SerialController, DefaultsPortClosed)
{
    SerialController c;
    EXPECT_FALSE(c.portOpen());
}

TEST(SerialController, DefaultBaudRate)
{
    SerialController c;
    EXPECT_EQ(c.baudRate(), 9600);
}

TEST(SerialController, SetPortName)
{
    SerialController c;
    c.setPortName(QStringLiteral("COM3"));
    EXPECT_EQ(c.portName(), QStringLiteral("COM3"));
}

TEST(SerialController, SetPortNameEmitsConfigChanged)
{
    int argc = 0; QCoreApplication app(argc, nullptr);
    SerialController c;
    QSignalSpy spy(&c, &SerialController::configChanged);
    c.setPortName(QStringLiteral("COM5"));
    EXPECT_EQ(spy.count(), 1);
}

TEST(SerialController, SendWhileClosedEmitsError)
{
    int argc = 0; QCoreApplication app(argc, nullptr);
    SerialController c;
    QSignalSpy spy(&c, &SerialController::errorOccurred);
    c.send(QStringLiteral("AA BB CC"));
    EXPECT_EQ(spy.count(), 1);
}

TEST(SerialController, ClearTerminal)
{
    int argc = 0; QCoreApplication app(argc, nullptr);
    SerialController c;
    c.clearTerminal();
    EXPECT_TRUE(c.terminalLines().isEmpty());
}

// ──────────────────────────────────────────────────────────────
// AnalyzerController
// ──────────────────────────────────────────────────────────────

TEST(AnalyzerController, InitiallyNotCapturing)
{
    AnalyzerController c;
    EXPECT_FALSE(c.capturing());
}

TEST(AnalyzerController, StartStopCapture)
{
    AnalyzerController c;
    c.startCapture();
    EXPECT_TRUE(c.capturing());
    c.stopCapture();
    EXPECT_FALSE(c.capturing());
}

TEST(AnalyzerController, StartCaptureEmitsSignal)
{
    int argc = 0; QCoreApplication app(argc, nullptr);
    AnalyzerController c;
    QSignalSpy spy(&c, &AnalyzerController::capturingChanged);
    c.startCapture();
    EXPECT_EQ(spy.count(), 1);
}

TEST(AnalyzerController, FeedTcpDataStoresPacket)
{
    AnalyzerController c;
    c.startCapture();
    // Modbus TCP: Read Holding Registers (valid frame)
    c.feedTcpData(QByteArray::fromHex("0001000000060103006B0003"));
    EXPECT_EQ(c.packetCount(), 1);
}

TEST(AnalyzerController, FeedSerialDataStoresPacket)
{
    AnalyzerController c;
    c.startCapture();
    // Modbus RTU with valid CRC: addr=1 FC=3 start=0x006B qty=0x0003
    QByteArray payload = QByteArray::fromHex("0103006B0003");
    uint16_t crc = rtu::core::analyzer::PacketAnalyzer::modbusRtuCrc(payload);
    payload.append(static_cast<char>(crc & 0xFF));
    payload.append(static_cast<char>((crc >> 8) & 0xFF));
    c.feedSerialData(payload);
    EXPECT_EQ(c.packetCount(), 1);
}

TEST(AnalyzerController, PacketsModelNotEmpty)
{
    AnalyzerController c;
    c.startCapture();
    c.feedTcpData(QByteArray::fromHex("0001000000060103006B0003"));
    EXPECT_EQ(c.packets().size(), 1);
    const QVariantMap p = c.packets()[0].toMap();
    EXPECT_EQ(p.value(QStringLiteral("protocol")).toString(), QStringLiteral("Modbus TCP"));
}

TEST(AnalyzerController, ClearPackets)
{
    AnalyzerController c;
    c.startCapture();
    c.feedTcpData(QByteArray::fromHex("0001000000060103006B0003"));
    c.clearPackets();
    EXPECT_EQ(c.packetCount(), 0);
}

TEST(AnalyzerController, StatisticsTotal)
{
    AnalyzerController c;
    c.startCapture();
    c.feedTcpData(QByteArray::fromHex("0001000000060103006B0003"));
    c.feedTcpData(QByteArray::fromHex("0001000000060103006B0003"));
    EXPECT_EQ(c.statistics().value(QStringLiteral("total")).toInt(), 2);
}

TEST(AnalyzerController, PacketAddedSignal)
{
    int argc = 0; QCoreApplication app(argc, nullptr);
    AnalyzerController c;
    c.startCapture();
    QSignalSpy spy(&c, &AnalyzerController::packetAdded);
    c.feedTcpData(QByteArray::fromHex("0001000000060103006B0003"));
    EXPECT_EQ(spy.count(), 1);
    const QVariantMap p = spy[0][0].toMap();
    EXPECT_EQ(p.value(QStringLiteral("protocol")).toString(), QStringLiteral("Modbus TCP"));
}

// ──────────────────────────────────────────────────────────────
// SimulatorController
// ──────────────────────────────────────────────────────────────

TEST(SimulatorController, InitiallyNotRunning)
{
    SimulatorController c;
    EXPECT_FALSE(c.running());
}

TEST(SimulatorController, StartStop)
{
    SimulatorController c;
    c.start();
    EXPECT_TRUE(c.running());
    c.stop();
    EXPECT_FALSE(c.running());
}

TEST(SimulatorController, StartEmitsSignal)
{
    int argc = 0; QCoreApplication app(argc, nullptr);
    SimulatorController c;
    QSignalSpy spy(&c, &SimulatorController::runningChanged);
    c.start();
    EXPECT_EQ(spy.count(), 1);
}

TEST(SimulatorController, SetRegisterUpdatesMap)
{
    int argc = 0; QCoreApplication app(argc, nullptr);
    SimulatorController c;
    QSignalSpy spy(&c, &SimulatorController::registerMapChanged);
    c.setRegister(10, 500);
    EXPECT_EQ(c.getRegister(10), 500);
    EXPECT_GE(spy.count(), 1);
    EXPECT_EQ(c.registerMap().size(), 1);
}

TEST(SimulatorController, UpdateIOAUpdatesMap)
{
    int argc = 0; QCoreApplication app(argc, nullptr);
    SimulatorController c;
    QSignalSpy spy(&c, &SimulatorController::ioaMapChanged);
    c.updateIOA(1001, QVariant(3.14));
    EXPECT_NEAR(c.getIOA(1001).toDouble(), 3.14, 1e-6);
    EXPECT_GE(spy.count(), 1);
}

TEST(SimulatorController, ResetClearsRegisterMap)
{
    int argc = 0; QCoreApplication app(argc, nullptr);
    SimulatorController c;
    c.setRegister(5, 99);
    c.reset();
    EXPECT_TRUE(c.registerMap().isEmpty());
}

TEST(SimulatorController, RandomEnabled)
{
    SimulatorController c;
    EXPECT_FALSE(c.randomEnabled());
    c.enableRandom(true);
    EXPECT_TRUE(c.randomEnabled());
}

TEST(SimulatorController, LoadScenarioMissingFile)
{
    int argc = 0; QCoreApplication app(argc, nullptr);
    SimulatorController c;
    QSignalSpy spy(&c, &SimulatorController::errorOccurred);
    const bool ok = c.loadScenario(QStringLiteral("/no/such/file.json"));
    EXPECT_FALSE(ok);
    EXPECT_GE(spy.count(), 1);
    EXPECT_FALSE(c.lastError().isEmpty());
}

// ──────────────────────────────────────────────────────────────
// IECController
// ──────────────────────────────────────────────────────────────

TEST(IECController, DefaultsNotConnected)
{
    IECController c;
    EXPECT_FALSE(c.connected());
}

TEST(IECController, DefaultStateText)
{
    IECController c;
    EXPECT_EQ(c.stateText(), QStringLiteral("Disconnected"));
}

TEST(IECController, SetHost)
{
    int argc = 0; QCoreApplication app(argc, nullptr);
    IECController c;
    QSignalSpy spy(&c, &IECController::configChanged);
    c.setHost(QStringLiteral("10.0.0.5"));
    EXPECT_EQ(c.host(), QStringLiteral("10.0.0.5"));
    EXPECT_EQ(spy.count(), 1);
}

TEST(IECController, SendCommandWhileDisconnectedEmitsError)
{
    int argc = 0; QCoreApplication app(argc, nullptr);
    IECController c;
    QSignalSpy spy(&c, &IECController::errorOccurred);
    c.sendCommand({{ QStringLiteral("command"), QStringLiteral("interrogation") }});
    EXPECT_GE(spy.count(), 1);
}

TEST(IECController, ClearDataPoints)
{
    int argc = 0; QCoreApplication app(argc, nullptr);
    IECController c;
    c.clearDataPoints();
    EXPECT_TRUE(c.dataPoints().isEmpty());
}

// ──────────────────────────────────────────────────────────────
// SessionViewModel
// ──────────────────────────────────────────────────────────────

TEST(SessionViewModel, InitiallyDisconnected)
{
    SessionViewModel vm;
    EXPECT_FALSE(vm.isConnected());
    EXPECT_EQ(vm.connectionStatus(), QStringLiteral("disconnected"));
}

TEST(SessionViewModel, InitiallyNotRunning)
{
    SessionViewModel vm;
    EXPECT_FALSE(vm.isRunning());
}

TEST(SessionViewModel, ResponseModelEmpty)
{
    SessionViewModel vm;
    EXPECT_TRUE(vm.responseModel().isEmpty());
}

TEST(SessionViewModel, RawLogEmpty)
{
    SessionViewModel vm;
    EXPECT_TRUE(vm.rawLog().isEmpty());
}

TEST(SessionViewModel, ClearLogNoCrash)
{
    SessionViewModel vm;
    vm.clearLog();   // should not crash even when disconnected
}

TEST(SessionViewModel, SendRequestWhileDisconnectedNoCrash)
{
    int argc = 0; QCoreApplication app(argc, nullptr);
    SessionViewModel vm;
    vm.sendRequest({{ QStringLiteral("fc"), 3 }, { QStringLiteral("address"), 0 }, { QStringLiteral("count"), 10 }});
    // should propagate error (not crash)
}

TEST(SessionViewModel, DisconnectWhileDisconnectedNoCrash)
{
    SessionViewModel vm;
    vm.disconnect();
}

// ──────────────────────────────────────────────────────────────
// main
// ──────────────────────────────────────────────────────────────

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
