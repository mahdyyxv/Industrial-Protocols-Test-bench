// ──────────────────────────────────────────────────────────────
// Phase 5 — IEC-104 module tests
//
// Tests cover:
//   1. IEC104State / IEC104DataPoint data model
//   2. IEC104Client state machine (no real connection needed)
//   3. buildRequest / encodeCommand logic
//   4. parseResponse APDU parser
//   5. Command send guards (disconnected state)
//   6. Mock interface contract tests
// ──────────────────────────────────────────────────────────────

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#ifdef RTU_IEC104_ENABLED
#include "core/protocols/iec104/IEC104Client.h"
#include "core/protocols/iec104/IEC104DataPoint.h"
#include <QCoreApplication>
#endif

using namespace testing;

#ifdef RTU_IEC104_ENABLED
using rtu::core::FrameDirection;
using rtu::core::ProtocolFrame;
#endif

// ──────────────────────────────────────────────────────────────
// IEC104Client interface mock (for contract validation)
// ──────────────────────────────────────────────────────────────
#ifdef RTU_IEC104_ENABLED

using namespace rtu::core::iec104;
using namespace rtu::core::protocols;

// Minimal mock that satisfies IProtocol without QObject inheritance
class MockIEC104Protocol {
public:
    MOCK_METHOD(bool,         connect,        (const QVariantMap&), ());
    MOCK_METHOD(void,         disconnect,     (), ());
    MOCK_METHOD(bool,         isConnected,    (), (const));
    MOCK_METHOD(IEC104State,  state,          (), (const));
    MOCK_METHOD(QString,      lastError,      (), (const));
    MOCK_METHOD(bool,         sendSingleCommand, (int, bool, bool), ());
    MOCK_METHOD(bool,         sendDoubleCommand, (int, int, bool),  ());
    MOCK_METHOD(bool,         sendSetpointNormalized, (int, float, bool), ());
    MOCK_METHOD(bool,         sendSetpointScaled,     (int, int, bool),   ());
    MOCK_METHOD(bool,         sendSetpointFloat,      (int, float, bool), ());
    MOCK_METHOD(bool,         sendBitstringCommand,   (int, uint32_t),    ());
    MOCK_METHOD(void,         sendStartDT,    (), ());
    MOCK_METHOD(void,         sendStopDT,     (), ());
    MOCK_METHOD(void,         sendInterrogationCommand, (int), ());
    MOCK_METHOD(void,         sendCounterInterrogationCommand, (int), ());
    MOCK_METHOD(void,         sendTestFrameActivation, (), ());
    MOCK_METHOD(void,         sendClockSynchronization, (), ());
};

// ──────────────────────────────────────────────────────────────
// 1. IEC104DataPoint — data model
// ──────────────────────────────────────────────────────────────

TEST(IEC104DataPoint, DefaultConstruction)
{
    IEC104DataPoint dp;
    EXPECT_EQ(dp.ioa,         0);
    EXPECT_EQ(dp.commonAddr,  0);
    EXPECT_EQ(dp.cot,         0);
    EXPECT_FALSE(dp.value.isValid());
    EXPECT_FALSE(dp.hasTimestamp);
    EXPECT_EQ(dp.typeId, IEC104TypeId::Unknown);
}

TEST(IEC104DataPoint, TypeNameSinglePoint)
{
    IEC104DataPoint dp;
    dp.typeId = IEC104TypeId::M_SP_NA_1;
    EXPECT_EQ(dp.typeName(), QStringLiteral("M_SP_NA_1"));
}

TEST(IEC104DataPoint, TypeNameDoublePoint)
{
    IEC104DataPoint dp;
    dp.typeId = IEC104TypeId::M_DP_NA_1;
    EXPECT_EQ(dp.typeName(), QStringLiteral("M_DP_NA_1"));
}

TEST(IEC104DataPoint, TypeNameNormalizedMeasured)
{
    IEC104DataPoint dp;
    dp.typeId = IEC104TypeId::M_ME_NA_1;
    EXPECT_EQ(dp.typeName(), QStringLiteral("M_ME_NA_1"));
}

TEST(IEC104DataPoint, TypeNameScaledMeasured)
{
    IEC104DataPoint dp;
    dp.typeId = IEC104TypeId::M_ME_NB_1;
    EXPECT_EQ(dp.typeName(), QStringLiteral("M_ME_NB_1"));
}

TEST(IEC104DataPoint, TypeNameFloatMeasured)
{
    IEC104DataPoint dp;
    dp.typeId = IEC104TypeId::M_ME_NC_1;
    EXPECT_EQ(dp.typeName(), QStringLiteral("M_ME_NC_1"));
}

TEST(IEC104DataPoint, TypeNameInterrogationCommand)
{
    IEC104DataPoint dp;
    dp.typeId = IEC104TypeId::C_IC_NA_1;
    EXPECT_EQ(dp.typeName(), QStringLiteral("C_IC_NA_1"));
}

TEST(IEC104DataPoint, TypeNameUnknown)
{
    IEC104DataPoint dp;
    dp.typeId = static_cast<IEC104TypeId>(255);
    EXPECT_TRUE(dp.typeName().startsWith(QStringLiteral("Unknown")));
}

// ──────────────────────────────────────────────────────────────
// 2. IEC104Client — state machine (no real connection)
// ──────────────────────────────────────────────────────────────

TEST(IEC104ClientState, InitialStateIsDisconnected)
{
    IEC104Client client;
    EXPECT_EQ(client.state(), IEC104State::Disconnected);
    EXPECT_FALSE(client.isConnected());
    EXPECT_FALSE(client.isDataTransferActive());
}

TEST(IEC104ClientState, ConnectToUnreachableHostFails)
{
    IEC104Client client;
    // Port 1 is not normally open
    EXPECT_FALSE(client.connectToOutstation(QStringLiteral("127.0.0.1"), 1));
    EXPECT_EQ(client.state(), IEC104State::Error);
    EXPECT_FALSE(client.lastError().isEmpty());
}

TEST(IEC104ClientState, ConnectViaQVariantMap)
{
    IEC104Client client;
    QVariantMap params;
    params[QStringLiteral("host")] = QStringLiteral("127.0.0.1");
    params[QStringLiteral("port")] = 1;  // unreachable
    EXPECT_FALSE(client.connect(params));
    EXPECT_EQ(client.state(), IEC104State::Error);
}

TEST(IEC104ClientState, DisconnectWhenNotConnectedIsSafe)
{
    IEC104Client client;
    client.disconnect();  // must not crash
    EXPECT_EQ(client.state(), IEC104State::Disconnected);
}

TEST(IEC104ClientState, IsConnectedReturnsFalseInitially)
{
    IEC104Client client;
    EXPECT_FALSE(client.isConnected());
}

// ──────────────────────────────────────────────────────────────
// 3. IEC104Client — command send guards (disconnected state)
// ──────────────────────────────────────────────────────────────

TEST(IEC104ClientGuards, SendSingleCommandWhenDisconnectedReturnsFalse)
{
    IEC104Client client;
    EXPECT_FALSE(client.sendSingleCommand(100, true));
    EXPECT_FALSE(client.lastError().isEmpty());
}

TEST(IEC104ClientGuards, SendDoubleCommandWhenDisconnectedReturnsFalse)
{
    IEC104Client client;
    EXPECT_FALSE(client.sendDoubleCommand(100, 1));
    EXPECT_FALSE(client.lastError().isEmpty());
}

TEST(IEC104ClientGuards, SendSetpointNormalizedWhenDisconnectedReturnsFalse)
{
    IEC104Client client;
    EXPECT_FALSE(client.sendSetpointNormalized(100, 0.5f));
    EXPECT_FALSE(client.lastError().isEmpty());
}

TEST(IEC104ClientGuards, SendSetpointScaledWhenDisconnectedReturnsFalse)
{
    IEC104Client client;
    EXPECT_FALSE(client.sendSetpointScaled(100, 1000));
    EXPECT_FALSE(client.lastError().isEmpty());
}

TEST(IEC104ClientGuards, SendSetpointFloatWhenDisconnectedReturnsFalse)
{
    IEC104Client client;
    EXPECT_FALSE(client.sendSetpointFloat(100, 3.14f));
    EXPECT_FALSE(client.lastError().isEmpty());
}

TEST(IEC104ClientGuards, SendBitstringWhenDisconnectedReturnsFalse)
{
    IEC104Client client;
    EXPECT_FALSE(client.sendBitstringCommand(100, 0xDEADBEEF));
    EXPECT_FALSE(client.lastError().isEmpty());
}

TEST(IEC104ClientGuards, SendRegulatingStepWhenDisconnectedReturnsFalse)
{
    IEC104Client client;
    EXPECT_FALSE(client.sendRegulatingStepCommand(100, 1));
    EXPECT_FALSE(client.lastError().isEmpty());
}

// sendStartDT/sendStopDT/sendInterrogation must not crash when disconnected
TEST(IEC104ClientGuards, SendStartDTWhenDisconnectedSetsError)
{
    IEC104Client client;
    client.sendStartDT();  // must not crash; sets error
    EXPECT_FALSE(client.lastError().isEmpty());
}

TEST(IEC104ClientGuards, SendInterrogationWhenDisconnectedSetsError)
{
    IEC104Client client;
    client.sendInterrogationCommand();
    EXPECT_FALSE(client.lastError().isEmpty());
}

// ──────────────────────────────────────────────────────────────
// 4. IEC104Client — buildRequest
// ──────────────────────────────────────────────────────────────

TEST(IEC104ClientBuildRequest, MissingCommandReturnsInvalidFrame)
{
    IEC104Client client;
    const auto frame = client.buildRequest({});
    EXPECT_FALSE(frame.valid);
    EXPECT_FALSE(frame.errorReason.isEmpty());
}

TEST(IEC104ClientBuildRequest, InterrogationCommandBuildsValidFrame)
{
    IEC104Client client;
    QVariantMap p;
    p[QStringLiteral("command")] = QStringLiteral("interrogation");
    const auto frame = client.buildRequest(p);
    EXPECT_TRUE(frame.valid);
    EXPECT_FALSE(frame.raw.isEmpty());
    EXPECT_TRUE(frame.description.contains(QStringLiteral("interrogation")));
    EXPECT_EQ(frame.direction, FrameDirection::Sent);
}

TEST(IEC104ClientBuildRequest, SingleCommandBuildsValidFrame)
{
    IEC104Client client;
    QVariantMap p;
    p[QStringLiteral("command")] = QStringLiteral("single_cmd");
    p[QStringLiteral("ioa")]     = 200;
    p[QStringLiteral("value")]   = true;
    const auto frame = client.buildRequest(p);
    EXPECT_TRUE(frame.valid);
    EXPECT_FALSE(frame.raw.isEmpty());
}

TEST(IEC104ClientBuildRequest, DoubleCommandBuildsValidFrame)
{
    IEC104Client client;
    QVariantMap p;
    p[QStringLiteral("command")] = QStringLiteral("double_cmd");
    p[QStringLiteral("ioa")]     = 300;
    p[QStringLiteral("value")]   = 2;
    const auto frame = client.buildRequest(p);
    EXPECT_TRUE(frame.valid);
}

TEST(IEC104ClientBuildRequest, SetpointNormalizedBuildsValidFrame)
{
    IEC104Client client;
    QVariantMap p;
    p[QStringLiteral("command")] = QStringLiteral("setpoint_normalized");
    p[QStringLiteral("ioa")]     = 400;
    p[QStringLiteral("value")]   = 0.75f;
    const auto frame = client.buildRequest(p);
    EXPECT_TRUE(frame.valid);
}

TEST(IEC104ClientBuildRequest, StartDTBuildsValidFrame)
{
    IEC104Client client;
    QVariantMap p;
    p[QStringLiteral("command")] = QStringLiteral("startdt");
    const auto frame = client.buildRequest(p);
    EXPECT_TRUE(frame.valid);
    EXPECT_EQ(frame.raw.size(), 1);  // just the tag byte
}

TEST(IEC104ClientBuildRequest, UnknownCommandBuildsInvalidFrame)
{
    IEC104Client client;
    QVariantMap p;
    p[QStringLiteral("command")] = QStringLiteral("this_does_not_exist");
    const auto frame = client.buildRequest(p);
    EXPECT_FALSE(frame.valid);
}

TEST(IEC104ClientBuildRequest, SendFrameWhenDisconnectedFails)
{
    IEC104Client client;
    QVariantMap p;
    p[QStringLiteral("command")] = QStringLiteral("startdt");
    const auto frame = client.buildRequest(p);
    ASSERT_TRUE(frame.valid);
    EXPECT_FALSE(client.sendFrame(frame));  // not connected
}

TEST(IEC104ClientBuildRequest, SendInvalidFrameAlwaysFails)
{
    IEC104Client client;
    ProtocolFrame bad;
    bad.valid = false;
    bad.errorReason = QStringLiteral("test error");
    EXPECT_FALSE(client.sendFrame(bad));
}

// ──────────────────────────────────────────────────────────────
// 5. parseResponse — APDU parser
// ──────────────────────────────────────────────────────────────

TEST(IEC104ParseResponse, EmptyBytesIsInvalid)
{
    IEC104Client client;
    const auto frame = client.parseResponse({});
    EXPECT_FALSE(frame.valid);
}

TEST(IEC104ParseResponse, TooShortIsInvalid)
{
    IEC104Client client;
    const auto frame = client.parseResponse(QByteArray(3, 0x00));
    EXPECT_FALSE(frame.valid);
}

TEST(IEC104ParseResponse, WrongStartByteIsInvalid)
{
    IEC104Client client;
    QByteArray raw(6, 0x00);
    raw[0] = 0x00;  // should be 0x68
    const auto frame = client.parseResponse(raw);
    EXPECT_FALSE(frame.valid);
    EXPECT_TRUE(frame.description.contains(QStringLiteral("invalid start byte")));
}

TEST(IEC104ParseResponse, StartDTActIsUFrame)
{
    // STARTDT act: 68 04 07 00 00 00
    QByteArray raw;
    raw.append('\x68'); raw.append('\x04');
    raw.append('\x07'); raw.append('\x00');
    raw.append('\x00'); raw.append('\x00');

    IEC104Client client;
    const auto frame = client.parseResponse(raw);
    EXPECT_TRUE(frame.valid);
    EXPECT_TRUE(frame.description.contains(QStringLiteral("STARTDT act")));
    EXPECT_EQ(frame.direction, FrameDirection::Received);
}

TEST(IEC104ParseResponse, StartDTConIsUFrame)
{
    // STARTDT con: 68 04 0B 00 00 00
    QByteArray raw;
    raw.append('\x68'); raw.append('\x04');
    raw.append('\x0B'); raw.append('\x00');
    raw.append('\x00'); raw.append('\x00');

    IEC104Client client;
    const auto frame = client.parseResponse(raw);
    EXPECT_TRUE(frame.valid);
    EXPECT_TRUE(frame.description.contains(QStringLiteral("STARTDT con")));
}

TEST(IEC104ParseResponse, StopDTActIsUFrame)
{
    // STOPDT act: 68 04 13 00 00 00
    QByteArray raw;
    raw.append('\x68'); raw.append('\x04');
    raw.append('\x13'); raw.append('\x00');
    raw.append('\x00'); raw.append('\x00');

    IEC104Client client;
    const auto frame = client.parseResponse(raw);
    EXPECT_TRUE(frame.valid);
    EXPECT_TRUE(frame.description.contains(QStringLiteral("STOPDT act")));
}

TEST(IEC104ParseResponse, TestFRActIsUFrame)
{
    // TESTFR act: 68 04 43 00 00 00
    QByteArray raw;
    raw.append('\x68'); raw.append('\x04');
    raw.append('\x43'); raw.append('\x00');
    raw.append('\x00'); raw.append('\x00');

    IEC104Client client;
    const auto frame = client.parseResponse(raw);
    EXPECT_TRUE(frame.valid);
    EXPECT_TRUE(frame.description.contains(QStringLiteral("TESTFR act")));
}

TEST(IEC104ParseResponse, SFrameIdentified)
{
    // S-frame: CF1=0x01
    QByteArray raw;
    raw.append('\x68'); raw.append('\x04');
    raw.append('\x01'); raw.append('\x00');
    raw.append('\x04'); raw.append('\x00');  // Rx=2

    IEC104Client client;
    const auto frame = client.parseResponse(raw);
    EXPECT_TRUE(frame.valid);
    EXPECT_TRUE(frame.description.contains(QStringLiteral("S-frame")));
}

TEST(IEC104ParseResponse, IFrameIdentified)
{
    // I-frame: CF1 bit0=0 → Tx/Rx sequence numbers
    QByteArray raw;
    raw.append('\x68'); raw.append('\x04');
    raw.append('\x02'); raw.append('\x00');  // Tx=1
    raw.append('\x02'); raw.append('\x00');  // Rx=1

    IEC104Client client;
    const auto frame = client.parseResponse(raw);
    EXPECT_TRUE(frame.valid);
    EXPECT_TRUE(frame.description.contains(QStringLiteral("I-frame")));
}

// ──────────────────────────────────────────────────────────────
// 6. Utilities
// ──────────────────────────────────────────────────────────────

TEST(IEC104Utilities, StateToStringCoversAllValues)
{
    EXPECT_EQ(IEC104Client::stateToString(IEC104State::Disconnected), QStringLiteral("Disconnected"));
    EXPECT_EQ(IEC104Client::stateToString(IEC104State::Connecting),   QStringLiteral("Connecting"));
    EXPECT_EQ(IEC104Client::stateToString(IEC104State::Connected),    QStringLiteral("Connected"));
    EXPECT_EQ(IEC104Client::stateToString(IEC104State::DataTransfer), QStringLiteral("DataTransfer"));
    EXPECT_EQ(IEC104Client::stateToString(IEC104State::Error),        QStringLiteral("Error"));
}

TEST(IEC104Utilities, TypeIdToStringKnownTypes)
{
    EXPECT_EQ(IEC104Client::typeIdToString(1),   QStringLiteral("M_SP_NA_1"));
    EXPECT_EQ(IEC104Client::typeIdToString(3),   QStringLiteral("M_DP_NA_1"));
    EXPECT_EQ(IEC104Client::typeIdToString(9),   QStringLiteral("M_ME_NA_1"));
    EXPECT_EQ(IEC104Client::typeIdToString(11),  QStringLiteral("M_ME_NB_1"));
    EXPECT_EQ(IEC104Client::typeIdToString(13),  QStringLiteral("M_ME_NC_1"));
    EXPECT_EQ(IEC104Client::typeIdToString(100), QStringLiteral("C_IC_NA_1"));
}

TEST(IEC104Utilities, DescribeApduStartDT)
{
    QByteArray raw;
    raw.append('\x68'); raw.append('\x04');
    raw.append('\x07'); raw.append('\x00');
    raw.append('\x00'); raw.append('\x00');
    EXPECT_TRUE(IEC104Client::describeApdu(raw).contains(QStringLiteral("STARTDT act")));
}

TEST(IEC104Utilities, CommonAddressRoundtrip)
{
    IEC104Client client;
    client.setCommonAddress(5);
    EXPECT_EQ(client.commonAddress(), 5);
}

TEST(IEC104Utilities, ResponseTimeoutRoundtrip)
{
    IEC104Client client;
    client.setResponseTimeoutMs(3000);
    EXPECT_EQ(client.responseTimeoutMs(), 3000);
}

// ──────────────────────────────────────────────────────────────
// 7. Mock contract tests
// ──────────────────────────────────────────────────────────────

TEST(IEC104MockContract, ConnectAndDisconnect)
{
    MockIEC104Protocol mock;
    EXPECT_CALL(mock, connect(_)).WillOnce(Return(true));
    EXPECT_CALL(mock, disconnect()).Times(1);
    EXPECT_CALL(mock, isConnected()).WillRepeatedly(Return(true));

    mock.connect({});
    EXPECT_TRUE(mock.isConnected());
    mock.disconnect();
}

TEST(IEC104MockContract, SingleCommandSuccess)
{
    MockIEC104Protocol mock;
    EXPECT_CALL(mock, sendSingleCommand(100, true, false)).WillOnce(Return(true));
    EXPECT_TRUE(mock.sendSingleCommand(100, true, false));
}

TEST(IEC104MockContract, DoubleCommandFailure)
{
    MockIEC104Protocol mock;
    EXPECT_CALL(mock, sendDoubleCommand(_, _, _)).WillOnce(Return(false));
    EXPECT_FALSE(mock.sendDoubleCommand(200, 1, false));
}

TEST(IEC104MockContract, SetpointNormalizedCalled)
{
    MockIEC104Protocol mock;
    EXPECT_CALL(mock, sendSetpointNormalized(50, FloatEq(0.75f), false)).WillOnce(Return(true));
    EXPECT_TRUE(mock.sendSetpointNormalized(50, 0.75f, false));
}

TEST(IEC104MockContract, SetpointScaledCalled)
{
    MockIEC104Protocol mock;
    EXPECT_CALL(mock, sendSetpointScaled(60, 1234, true)).WillOnce(Return(true));
    EXPECT_TRUE(mock.sendSetpointScaled(60, 1234, true));
}

TEST(IEC104MockContract, SetpointFloatCalled)
{
    MockIEC104Protocol mock;
    EXPECT_CALL(mock, sendSetpointFloat(70, FloatEq(3.14f), false)).WillOnce(Return(true));
    EXPECT_TRUE(mock.sendSetpointFloat(70, 3.14f, false));
}

TEST(IEC104MockContract, BitstringCommandCalled)
{
    MockIEC104Protocol mock;
    EXPECT_CALL(mock, sendBitstringCommand(80, 0xABCDu)).WillOnce(Return(true));
    EXPECT_TRUE(mock.sendBitstringCommand(80, 0xABCDu));
}

TEST(IEC104MockContract, InterrogationCalled)
{
    MockIEC104Protocol mock;
    EXPECT_CALL(mock, sendInterrogationCommand(20)).Times(1);
    mock.sendInterrogationCommand(20);
}

TEST(IEC104MockContract, CounterInterrogationCalled)
{
    MockIEC104Protocol mock;
    EXPECT_CALL(mock, sendCounterInterrogationCommand(5)).Times(1);
    mock.sendCounterInterrogationCommand(5);
}

TEST(IEC104MockContract, StartDTAndStopDTCalled)
{
    MockIEC104Protocol mock;
    EXPECT_CALL(mock, sendStartDT()).Times(1);
    EXPECT_CALL(mock, sendStopDT()).Times(1);
    mock.sendStartDT();
    mock.sendStopDT();
}

TEST(IEC104MockContract, TestFrameCalled)
{
    MockIEC104Protocol mock;
    EXPECT_CALL(mock, sendTestFrameActivation()).Times(1);
    mock.sendTestFrameActivation();
}

TEST(IEC104MockContract, ClockSyncCalled)
{
    MockIEC104Protocol mock;
    EXPECT_CALL(mock, sendClockSynchronization()).Times(1);
    mock.sendClockSynchronization();
}

TEST(IEC104MockContract, LastErrorOnFailure)
{
    MockIEC104Protocol mock;
    EXPECT_CALL(mock, connect(_)).WillOnce(Return(false));
    EXPECT_CALL(mock, state()).WillOnce(Return(IEC104State::Error));
    EXPECT_CALL(mock, lastError()).WillOnce(Return(QString("connection refused")));

    mock.connect({});
    EXPECT_EQ(mock.state(), IEC104State::Error);
    EXPECT_FALSE(mock.lastError().isEmpty());
}

#else  // RTU_IEC104_ENABLED not defined

TEST(IEC104Skipped, LibraryNotAvailable)
{
    GTEST_SKIP() << "lib60870 not available — IEC-104 module not built";
}

#endif // RTU_IEC104_ENABLED

// ──────────────────────────────────────────────────────────────
// Entry point
// ──────────────────────────────────────────────────────────────
int main(int argc, char** argv)
{
    // QCoreApplication needed for QVariant and QDateTime
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
