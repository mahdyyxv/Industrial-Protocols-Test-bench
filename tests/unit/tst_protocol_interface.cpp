#include <QtTest>
#include "core/protocols/IProtocol.h"
#include "services/subscription/SubscriptionTier.h"

using namespace rtu::core;
using namespace rtu::core::protocols;
using namespace rtu::services;

// ──────────────────────────────────────────────────────────────
// Minimal stub implementing IProtocol — used only in tests.
// Real implementations are added by the Core Engineer.
// ──────────────────────────────────────────────────────────────
class StubProtocol final : public IProtocol {
public:
    ProtocolType  type()      const override { return ProtocolType::ModbusRTU; }
    QString       name()      const override { return QStringLiteral("ModbusRTU-Stub"); }
    TransportType transport() const override { return TransportType::Serial; }
    bool          requiresPro() const override { return false; }

    bool connect(const QVariantMap&) override { m_connected = true;  return true; }
    void disconnect()                override { m_connected = false; }
    bool isConnected()         const override { return m_connected; }

    ProtocolFrame buildRequest(const QVariantMap&) const override {
        return ProtocolFrame{
            QByteArray("\x01\x03\x00\x00\x00\x01", 6),
            QStringLiteral("Read Holding Registers"),
            QDateTime::currentDateTime(),
            FrameDirection::Sent,
            true,
            {}
        };
    }

    bool sendFrame(const ProtocolFrame&) override { return m_connected; }

    ProtocolFrame parseResponse(const QByteArray& raw) const override {
        return ProtocolFrame{raw, {}, QDateTime::currentDateTime(),
                             FrameDirection::Received, !raw.isEmpty(), {}};
    }

private:
    bool m_connected { false };
};

// ──────────────────────────────────────────────────────────────
class TestProtocolInterface : public QObject {
    Q_OBJECT

private slots:
    void test_stubConnectsAndDisconnects() {
        StubProtocol p;
        QVERIFY(!p.isConnected());
        QVERIFY(p.connect({}));
        QVERIFY(p.isConnected());
        p.disconnect();
        QVERIFY(!p.isConnected());
    }

    void test_stubIsFreeTier() {
        StubProtocol p;
        QCOMPARE(p.requiresPro(), false);
    }

    void test_buildRequestReturnsValidFrame() {
        StubProtocol p;
        const auto frame = p.buildRequest({});
        QVERIFY(!frame.raw.isEmpty());
        QCOMPARE(frame.valid, true);
        QCOMPARE(frame.direction, FrameDirection::Sent);
    }

    void test_parseEmptyResponseIsInvalid() {
        StubProtocol p;
        const auto frame = p.parseResponse({});
        QCOMPARE(frame.valid, false);
    }

    void test_featureFlagFreeProtocol() {
        // Architectural contract: Free-tier features must always
        // be available regardless of subscription state
        StubProtocol p;
        QCOMPARE(p.requiresPro(), false);
    }
};

QTEST_MAIN(TestProtocolInterface)
#include "tst_protocol_interface.moc"
