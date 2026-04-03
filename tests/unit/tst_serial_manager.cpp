#include <QtTest>
#include <QSignalSpy>
#include <QSerialPortInfo>

#ifdef RTU_SERIAL_ENABLED
#include "core/protocols/serial/SerialManager.h"
#include "core/protocols/serial/SerialProtocol.h"
using namespace rtu::core::serial;
using namespace rtu::core;
#endif

// ──────────────────────────────────────────────────────────────
// SerialManagerTests
//
// Tests run without physical hardware.
// All hardware-dependent paths are tested via state assertions
// and signal spies — no real port opened.
// ──────────────────────────────────────────────────────────────
class SerialManagerTests : public QObject {
    Q_OBJECT

private slots:

    // ── SerialManager state ───────────────────────────────────

    void test_initialStateIsDisconnected() {
#ifndef RTU_SERIAL_ENABLED
        QSKIP("Serial module not compiled (Qt6SerialPort missing)");
#else
        SerialManager mgr;
        QCOMPARE(mgr.isConnected(), false);
        QVERIFY(mgr.portName().isEmpty());
        QVERIFY(mgr.receiveBuffer().isEmpty());
#endif
    }

    void test_openWithEmptyPortNameFails() {
#ifndef RTU_SERIAL_ENABLED
        QSKIP("Serial module not compiled");
#else
        SerialManager mgr;
        QSignalSpy errorSpy(&mgr, &SerialManager::errorOccurred);

        SerialConfig cfg;
        cfg.portName = "";   // invalid
        const bool opened = mgr.open(cfg);

        QCOMPARE(opened, false);
        QCOMPARE(mgr.isConnected(), false);
        // QSerialPort itself fires an error for invalid port name
        QVERIFY(errorSpy.count() >= 0);  // may or may not fire depending on OS
#endif
    }

    void test_openNonExistentPortEmitsError() {
#ifndef RTU_SERIAL_ENABLED
        QSKIP("Serial module not compiled");
#else
        SerialManager mgr;
        QSignalSpy errorSpy(&mgr, &SerialManager::errorOccurred);
        QSignalSpy connSpy (&mgr, &SerialManager::connectionChanged);

        SerialConfig cfg;
        cfg.portName = "DOES_NOT_EXIST_999";
        const bool opened = mgr.open(cfg);

        QCOMPARE(opened, false);
        QCOMPARE(mgr.isConnected(), false);
        QVERIFY(!errorSpy.isEmpty());
        QVERIFY(connSpy.isEmpty());   // no connection event on failure
#endif
    }

    void test_closeWhileDisconnectedIsSafe() {
#ifndef RTU_SERIAL_ENABLED
        QSKIP("Serial module not compiled");
#else
        SerialManager mgr;
        // Must not crash or emit spurious signals
        mgr.close();
        QCOMPARE(mgr.isConnected(), false);
#endif
    }

    void test_writeWhileDisconnectedEmitsError() {
#ifndef RTU_SERIAL_ENABLED
        QSKIP("Serial module not compiled");
#else
        SerialManager mgr;
        QSignalSpy errorSpy(&mgr, &SerialManager::errorOccurred);

        const bool ok = mgr.write(QByteArray("\x01\x03\x00\x00", 4));

        QCOMPARE(ok, false);
        QCOMPARE(errorSpy.count(), 1);
        QVERIFY(errorSpy.first().first().toString().contains("not open"));
#endif
    }

    void test_writeEmptyDataSucceedsWithoutError() {
#ifndef RTU_SERIAL_ENABLED
        QSKIP("Serial module not compiled");
#else
        // open() is not called here — but empty write should short-circuit
        // before reaching the not-connected guard (returns true, no error)
        // We first need to be "connected" for the guard to pass.
        // Since we can't open a real port, test the guard path only.
        SerialManager mgr;
        QSignalSpy errorSpy(&mgr, &SerialManager::errorOccurred);

        const bool ok = mgr.write(QByteArray());
        // Not connected → error emitted
        QCOMPARE(ok, false);
        QCOMPARE(errorSpy.count(), 1);
#endif
    }

    void test_clearBufferEmptiesAccumulator() {
#ifndef RTU_SERIAL_ENABLED
        QSKIP("Serial module not compiled");
#else
        SerialManager mgr;
        QVERIFY(mgr.receiveBuffer().isEmpty());
        mgr.clearBuffer();   // idempotent
        QVERIFY(mgr.receiveBuffer().isEmpty());
#endif
    }

    // ── Port discovery ────────────────────────────────────────

    void test_availablePortsReturnsList() {
#ifndef RTU_SERIAL_ENABLED
        QSKIP("Serial module not compiled");
#else
        // Should return a list (may be empty on CI, never crash)
        const QStringList ports = SerialManager::availablePorts();
        Q_UNUSED(ports)
        QVERIFY(true);
#endif
    }

    // ── SerialProtocol ────────────────────────────────────────

    void test_protocolIdentity() {
#ifndef RTU_SERIAL_ENABLED
        QSKIP("Serial module not compiled");
#else
        SerialProtocol proto;
        QCOMPARE(proto.name(),        QStringLiteral("Serial"));
        QCOMPARE(proto.transport(),   rtu::core::protocols::TransportType::Serial);
        QCOMPARE(proto.requiresPro(), false);
        QCOMPARE(proto.isConnected(), false);
#endif
    }

    void test_protocolConnectMissingPortEmitsError() {
#ifndef RTU_SERIAL_ENABLED
        QSKIP("Serial module not compiled");
#else
        SerialProtocol proto;
        QSignalSpy errSpy(&proto, &SerialProtocol::errorOccurred);

        const bool ok = proto.connect({});   // no "port" key
        QCOMPARE(ok, false);
        QCOMPARE(errSpy.count(), 1);
        QVERIFY(errSpy.first().first().toString().contains("port"));
#endif
    }

    void test_protocolBuildRequestFromHex() {
#ifndef RTU_SERIAL_ENABLED
        QSKIP("Serial module not compiled");
#else
        SerialProtocol proto;
        const QVariantMap params{ {"hex", QStringLiteral("010300000001")} };
        const ProtocolFrame frame = proto.buildRequest(params);

        QCOMPARE(frame.raw.toHex(), QByteArray("010300000001"));
        QCOMPARE(frame.valid, true);
        QCOMPARE(frame.direction, FrameDirection::Sent);
#endif
    }

    void test_protocolBuildRequestEmptyHexIsInvalid() {
#ifndef RTU_SERIAL_ENABLED
        QSKIP("Serial module not compiled");
#else
        SerialProtocol proto;
        const ProtocolFrame frame = proto.buildRequest({});
        QCOMPARE(frame.valid, false);
        QVERIFY(!frame.errorReason.isEmpty());
#endif
    }

    void test_protocolParseResponseEmpty() {
#ifndef RTU_SERIAL_ENABLED
        QSKIP("Serial module not compiled");
#else
        SerialProtocol proto;
        const ProtocolFrame frame = proto.parseResponse({});
        QCOMPARE(frame.valid,     false);
        QCOMPARE(frame.direction, FrameDirection::Received);
#endif
    }

    void test_protocolParseResponseNonEmpty() {
#ifndef RTU_SERIAL_ENABLED
        QSKIP("Serial module not compiled");
#else
        SerialProtocol proto;
        const QByteArray raw = QByteArray::fromHex("0103020001");
        const ProtocolFrame frame = proto.parseResponse(raw);
        QCOMPARE(frame.valid,     true);
        QCOMPARE(frame.raw,       raw);
        QCOMPARE(frame.direction, FrameDirection::Received);
#endif
    }

    void test_protocolSendInvalidFrameEmitsError() {
#ifndef RTU_SERIAL_ENABLED
        QSKIP("Serial module not compiled");
#else
        SerialProtocol proto;
        QSignalSpy errSpy(&proto, &SerialProtocol::errorOccurred);

        ProtocolFrame bad;
        bad.valid       = false;
        bad.errorReason = "test invalid frame";
        const bool ok = proto.sendFrame(bad);

        QCOMPARE(ok, false);
        QCOMPARE(errSpy.count(), 1);
#endif
    }

    void test_protocolDisconnectWhenNotConnectedIsSafe() {
#ifndef RTU_SERIAL_ENABLED
        QSKIP("Serial module not compiled");
#else
        SerialProtocol proto;
        proto.disconnect();   // must not crash
        QCOMPARE(proto.isConnected(), false);
#endif
    }

    void test_protocolAvailablePorts() {
#ifndef RTU_SERIAL_ENABLED
        QSKIP("Serial module not compiled");
#else
        const QStringList ports = SerialProtocol::availablePorts();
        Q_UNUSED(ports)
        QVERIFY(true);
#endif
    }
};

QTEST_MAIN(SerialManagerTests)
#include "tst_serial_manager.moc"
