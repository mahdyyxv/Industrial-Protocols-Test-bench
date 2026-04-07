// tst_server_modes.cpp
//
// Tests for Phase 9 server-side core classes:
//   - ModbusTcpServer: start/stop, register read/write, lifecycle
//   - IEC104Server:    start/stop, IOA read/write, lifecycle

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QSignalSpy>
#include <QEventLoop>
#include <QTimer>

// Helper: spin the event loop until spy has >= 1 entry or ms timeout elapses.
static bool waitForSignal(QSignalSpy& spy, int ms = 1000)
{
    if (spy.count() > 0) return true;
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    timeout.setInterval(ms);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    // Poll every 20 ms; QSignalSpy is not directly connectable in Qt6
    QTimer poll;
    poll.setInterval(20);
    QObject::connect(&poll, &QTimer::timeout, [&]() {
        if (spy.count() > 0) loop.quit();
    });
    timeout.start();
    poll.start();
    loop.exec();
    return spy.count() > 0;
}

// ── Modbus Server ─────────────────────────────────────────────
#ifdef RTU_MODBUS_ENABLED
#include "core/protocols/modbus/ModbusTcpServer.h"
using namespace rtu::core::modbus;

TEST(ModbusTcpServer, InitialState)
{
    ModbusTcpServer srv;
    EXPECT_FALSE(srv.isRunning());
    EXPECT_EQ(srv.registerValue(0), 0);
    EXPECT_EQ(srv.registerValue(255), 0);
}

TEST(ModbusTcpServer, RegisterReadWrite)
{
    ModbusTcpServer srv;
    srv.setRegisterValue(0,   0x1234);
    srv.setRegisterValue(100, 0xABCD);
    srv.setRegisterValue(255, 0x0001);

    EXPECT_EQ(srv.registerValue(0),   0x1234);
    EXPECT_EQ(srv.registerValue(100), 0xABCD);
    EXPECT_EQ(srv.registerValue(255), 0x0001);
}

TEST(ModbusTcpServer, RegisterBoundsCheck)
{
    ModbusTcpServer srv;
    srv.setRegisterValue(-1,  0xFFFF);
    srv.setRegisterValue(256, 0xFFFF);
    EXPECT_EQ(srv.registerValue(0),   0);
    EXPECT_EQ(srv.registerValue(-1),  0);
    EXPECT_EQ(srv.registerValue(256), 0);
}

TEST(ModbusTcpServer, AllRegisters)
{
    ModbusTcpServer srv;
    srv.setRegisterValue(3, 42);
    srv.setRegisterValue(10, 999);
    auto regs = srv.allRegisters();
    EXPECT_EQ(regs[3],  42);
    EXPECT_EQ(regs[10], 999);
    EXPECT_EQ(regs[0],  0);
}

TEST(ModbusTcpServer, StartAndStop)
{
    ModbusTcpServer srv;
    QSignalSpy startedSpy(&srv, &ModbusTcpServer::serverStarted);
    QSignalSpy stoppedSpy(&srv, &ModbusTcpServer::serverStopped);

    EXPECT_TRUE(srv.start(19502));
    EXPECT_TRUE(waitForSignal(startedSpy, 1000)) << "serverStarted not emitted";
    EXPECT_TRUE(srv.isRunning());

    srv.stop();
    EXPECT_TRUE(waitForSignal(stoppedSpy, 1000)) << "serverStopped not emitted";
    EXPECT_FALSE(srv.isRunning());
}

TEST(ModbusTcpServer, StopWithoutStart)
{
    ModbusTcpServer srv;
    EXPECT_NO_THROW(srv.stop());
    EXPECT_FALSE(srv.isRunning());
}

TEST(ModbusTcpServer, DoubleStart)
{
    ModbusTcpServer srv;
    QSignalSpy startedSpy(&srv, &ModbusTcpServer::serverStarted);

    srv.start(19503);
    waitForSignal(startedSpy, 800);

    // Second start should restart cleanly
    startedSpy.clear();
    EXPECT_TRUE(srv.start(19503));
    EXPECT_TRUE(waitForSignal(startedSpy, 1000)) << "serverStarted not emitted on restart";
    EXPECT_TRUE(srv.isRunning());

    srv.stop();
}

#endif // RTU_MODBUS_ENABLED

// ── IEC104Server unit tests ───────────────────────────────────
#ifdef RTU_IEC104_ENABLED
#include "core/protocols/iec104/IEC104Server.h"
using namespace rtu::core::iec104;

TEST(IEC104Server, InitialState)
{
    IEC104Server srv;
    EXPECT_FALSE(srv.isRunning());
    EXPECT_TRUE(srv.allIOAs().isEmpty());
}

TEST(IEC104Server, UpdateIOABeforeStart)
{
    IEC104Server srv;
    EXPECT_NO_THROW(srv.updateMeasuredValue(100, 1.5f));
    EXPECT_NO_THROW(srv.updateSinglePoint(101, true));
    EXPECT_NO_THROW(srv.updateDoublePoint(102, 2));
}

TEST(IEC104Server, IOAValuesStored)
{
    IEC104Server srv;
    srv.updateMeasuredValue(100, 3.14f);
    srv.updateSinglePoint(200, true);
    srv.updateDoublePoint(300, 2);

    const auto snap = srv.allIOAs();
    EXPECT_EQ(snap.size(), 3);
    EXPECT_FLOAT_EQ(snap.value(100).toFloat(), 3.14f);
    EXPECT_EQ(snap.value(200).toBool(), true);
    EXPECT_EQ(snap.value(300).toInt(), 2);
}

TEST(IEC104Server, StartAndStop)
{
    IEC104Server srv;
    QSignalSpy startedSpy(&srv, &IEC104Server::serverStarted);
    QSignalSpy stoppedSpy(&srv, &IEC104Server::serverStopped);

    bool ok = srv.start(19404);
    if (!ok) {
        GTEST_SKIP() << "Port 19404 unavailable: " << srv.lastError().toStdString();
    }

    // lib60870 starts synchronously — wait briefly for event delivery
    EXPECT_TRUE(waitForSignal(startedSpy, 800)) << "serverStarted not emitted";
    EXPECT_TRUE(srv.isRunning());

    srv.stop();
    EXPECT_TRUE(waitForSignal(stoppedSpy, 800)) << "serverStopped not emitted";
    EXPECT_FALSE(srv.isRunning());
}

TEST(IEC104Server, StopWithoutStart)
{
    IEC104Server srv;
    EXPECT_NO_THROW(srv.stop());
    EXPECT_FALSE(srv.isRunning());
}

#endif // RTU_IEC104_ENABLED

// ── Always-run sanity test ────────────────────────────────────
TEST(ServerModes, Placeholder)
{
    EXPECT_TRUE(true);
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
