// ──────────────────────────────────────────────────────────────
// tst_simulator.cpp — DeviceSimulator unit tests (GoogleTest)
//
// Coverage:
//   • SimulatorScenario JSON parsing (valid, invalid, unknown types)
//   • Device control (start/stop/reset signals + guards)
//   • Value update tests — Modbus registers + IEC-104 IOAs
//   • Spontaneous data emission
//   • Scenario execution tests — step actions, ordering, loop, finish
//   • Randomization — range, global range, random values in range
//   • Scenario + randomize action
// ──────────────────────────────────────────────────────────────

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QTest>       // QTest::qWait

#include "core/simulator/DeviceSimulator.h"
#include "core/simulator/SimulatorScenario.h"

using namespace rtu::core::simulator;

// ── Shared QCoreApplication (required by QTimer / QSignalSpy) ─
static int    s_argc = 0;
static QCoreApplication* s_app = nullptr;

// ── Helpers ───────────────────────────────────────────────────
namespace {

QByteArray validScenarioJson(bool loop = false, int delayMs = 10)
{
    return QString(R"({
        "name": "TestScenario",
        "loop": %1,
        "steps": [
            {
                "delay_ms": %2,
                "actions": [
                    { "type": "set_register", "address": 0, "value": 42 },
                    { "type": "set_ioa",      "address": 1001, "value": 3.14 }
                ]
            },
            {
                "delay_ms": %2,
                "actions": [
                    { "type": "set_register", "address": 1, "value": 99 },
                    { "type": "send_spontaneous" }
                ]
            }
        ]
    })")
    .arg(loop ? "true" : "false")
    .arg(delayMs)
    .toUtf8();
}

// Write JSON bytes to a temporary file; caller owns file lifetime
QString writeTempFile(const QByteArray& content)
{
    auto* f = new QTemporaryFile();
    f->setAutoRemove(false);
    if (!f->open()) return {};
    f->write(content);
    f->close();
    return f->fileName();
}

} // namespace

// ──────────────────────────────────────────────────────────────
// SimulatorScenario JSON parsing
// ──────────────────────────────────────────────────────────────

TEST(ScenarioParsing, ValidJson)
{
    QString err;
    SimulatorScenario s = SimulatorScenario::fromJson(validScenarioJson(), &err);

    EXPECT_TRUE(err.isEmpty());
    EXPECT_EQ(s.name, QStringLiteral("TestScenario"));
    EXPECT_FALSE(s.loop);
    EXPECT_EQ(s.steps.size(), 2);
    EXPECT_EQ(s.steps[0].actions.size(), 2);
    EXPECT_EQ(s.steps[1].actions.size(), 2);
}

TEST(ScenarioParsing, StepDelayParsed)
{
    SimulatorScenario s = SimulatorScenario::fromJson(validScenarioJson(false, 250));
    EXPECT_EQ(s.steps[0].delayMs, 250);
}

TEST(ScenarioParsing, LoopFlag)
{
    SimulatorScenario s = SimulatorScenario::fromJson(validScenarioJson(true));
    EXPECT_TRUE(s.loop);
}

TEST(ScenarioParsing, SetRegisterAction)
{
    SimulatorScenario s = SimulatorScenario::fromJson(validScenarioJson());
    const SimulatorAction& a = s.steps[0].actions[0];
    EXPECT_EQ(a.type,    SimulatorAction::Type::SetRegister);
    EXPECT_EQ(a.address, 0);
    EXPECT_EQ(a.value.toInt(), 42);
}

TEST(ScenarioParsing, SetIoaAction)
{
    SimulatorScenario s = SimulatorScenario::fromJson(validScenarioJson());
    const SimulatorAction& a = s.steps[0].actions[1];
    EXPECT_EQ(a.type,            SimulatorAction::Type::SetIOA);
    EXPECT_EQ(a.address,         1001);
    EXPECT_NEAR(a.value.toDouble(), 3.14, 1e-6);
}

TEST(ScenarioParsing, SendSpontaneousAction)
{
    SimulatorScenario s = SimulatorScenario::fromJson(validScenarioJson());
    const SimulatorAction& a = s.steps[1].actions[1];
    EXPECT_EQ(a.type, SimulatorAction::Type::SendSpontaneous);
}

TEST(ScenarioParsing, RandomizeAction)
{
    QByteArray json = R"({"name":"r","loop":false,"steps":[{"delay_ms":0,"actions":[{"type":"randomize"}]}]})";
    SimulatorScenario s = SimulatorScenario::fromJson(json);
    ASSERT_EQ(s.steps.size(), 1);
    EXPECT_EQ(s.steps[0].actions[0].type, SimulatorAction::Type::Randomize);
}

TEST(ScenarioParsing, UnknownActionTypeSkipped)
{
    QByteArray json = R"({
        "name":"x","loop":false,
        "steps":[{"delay_ms":0,"actions":[
            {"type":"set_register","address":0,"value":1},
            {"type":"unknown_future_type"},
            {"type":"set_register","address":1,"value":2}
        ]}]
    })";
    SimulatorScenario s = SimulatorScenario::fromJson(json);
    // unknown type silently skipped — 2 valid actions remain
    EXPECT_EQ(s.steps[0].actions.size(), 2);
}

TEST(ScenarioParsing, InvalidJsonReturnsEmptyWithError)
{
    QString err;
    SimulatorScenario s = SimulatorScenario::fromJson("not json at all", &err);
    EXPECT_TRUE(s.isEmpty());
    EXPECT_FALSE(err.isEmpty());
}

TEST(ScenarioParsing, NotAnObjectReturnsEmpty)
{
    QString err;
    SimulatorScenario s = SimulatorScenario::fromJson("[1,2,3]", &err);
    EXPECT_TRUE(s.isEmpty());
    EXPECT_FALSE(err.isEmpty());
}

TEST(ScenarioParsing, FromFileMissingPath)
{
    QString err;
    SimulatorScenario s = SimulatorScenario::fromFile("/no/such/file.json", &err);
    EXPECT_TRUE(s.isEmpty());
    EXPECT_FALSE(err.isEmpty());
}

TEST(ScenarioParsing, FromFileValid)
{
    QString path = writeTempFile(validScenarioJson());
    ASSERT_FALSE(path.isEmpty());
    QString err;
    SimulatorScenario s = SimulatorScenario::fromFile(path, &err);
    QFile::remove(path);
    EXPECT_TRUE(err.isEmpty());
    EXPECT_EQ(s.name, QStringLiteral("TestScenario"));
    EXPECT_EQ(s.steps.size(), 2);
}

// ──────────────────────────────────────────────────────────────
// Device control
// ──────────────────────────────────────────────────────────────

TEST(DeviceControl, InitiallyNotRunning)
{
    DeviceSimulator sim;
    EXPECT_FALSE(sim.isRunning());
}

TEST(DeviceControl, StartSetsRunning)
{
    DeviceSimulator sim;
    sim.start();
    EXPECT_TRUE(sim.isRunning());
}

TEST(DeviceControl, StopClearsRunning)
{
    DeviceSimulator sim;
    sim.start();
    sim.stop();
    EXPECT_FALSE(sim.isRunning());
}

TEST(DeviceControl, StartEmitsSignal)
{
    DeviceSimulator sim;
    QSignalSpy spy(&sim, &DeviceSimulator::started);
    sim.start();
    EXPECT_EQ(spy.count(), 1);
}

TEST(DeviceControl, StopEmitsSignal)
{
    DeviceSimulator sim;
    sim.start();
    QSignalSpy spy(&sim, &DeviceSimulator::stopped);
    sim.stop();
    EXPECT_EQ(spy.count(), 1);
}

TEST(DeviceControl, DoubleStartNoop)
{
    DeviceSimulator sim;
    sim.start();
    QSignalSpy spy(&sim, &DeviceSimulator::started);
    sim.start();   // second call — should be no-op
    EXPECT_EQ(spy.count(), 0);
}

TEST(DeviceControl, ResetEmitsSignal)
{
    DeviceSimulator sim;
    QSignalSpy spy(&sim, &DeviceSimulator::resetDone);
    sim.reset();
    EXPECT_EQ(spy.count(), 1);
}

// ──────────────────────────────────────────────────────────────
// Value update tests — Modbus registers
// ──────────────────────────────────────────────────────────────

TEST(RegisterValues, SetAndGet)
{
    DeviceSimulator sim;
    sim.setRegisterValue(10, 1234);
    EXPECT_EQ(sim.getRegisterValue(10), 1234u);
}

TEST(RegisterValues, DefaultIsZero)
{
    DeviceSimulator sim;
    EXPECT_EQ(sim.getRegisterValue(999), 0u);
}

TEST(RegisterValues, EmitsRegisterChanged)
{
    DeviceSimulator sim;
    QSignalSpy spy(&sim, &DeviceSimulator::registerChanged);
    sim.setRegisterValue(5, 100);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy[0][0].toInt(), 5);
    EXPECT_EQ(spy[0][1].toUInt(), 100u);
}

TEST(RegisterValues, OverwriteRegister)
{
    DeviceSimulator sim;
    sim.setRegisterValue(0, 10);
    sim.setRegisterValue(0, 20);
    EXPECT_EQ(sim.getRegisterValue(0), 20u);
}

TEST(RegisterValues, AllRegisters)
{
    DeviceSimulator sim;
    sim.setRegisterValue(1, 11);
    sim.setRegisterValue(2, 22);
    auto regs = sim.allRegisters();
    EXPECT_EQ(regs.size(), 2);
    EXPECT_EQ(regs.value(1), 11u);
    EXPECT_EQ(regs.value(2), 22u);
}

TEST(RegisterValues, ResetClearsRegisters)
{
    DeviceSimulator sim;
    sim.setRegisterValue(0, 99);
    sim.reset();
    EXPECT_EQ(sim.getRegisterValue(0), 0u);
    EXPECT_TRUE(sim.allRegisters().isEmpty());
}

// ──────────────────────────────────────────────────────────────
// Value update tests — IEC-104 IOAs
// ──────────────────────────────────────────────────────────────

TEST(IOAValues, UpdateAndGet)
{
    DeviceSimulator sim;
    sim.updateIOA(1001, QVariant(1.5));
    EXPECT_NEAR(sim.getIOA(1001).toDouble(), 1.5, 1e-9);
}

TEST(IOAValues, DefaultIsInvalid)
{
    DeviceSimulator sim;
    EXPECT_FALSE(sim.getIOA(9999).isValid());
}

TEST(IOAValues, EmitsIoaChanged)
{
    DeviceSimulator sim;
    QSignalSpy spy(&sim, &DeviceSimulator::ioaChanged);
    sim.updateIOA(2000, QVariant(true));
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy[0][0].toInt(), 2000);
    EXPECT_EQ(spy[0][1].toBool(), true);
}

TEST(IOAValues, AllIOAs)
{
    DeviceSimulator sim;
    sim.updateIOA(100, QVariant(1));
    sim.updateIOA(200, QVariant(2));
    auto ioaMap = sim.allIOAs();
    EXPECT_EQ(ioaMap.size(), 2);
}

TEST(IOAValues, ResetClearsIOAs)
{
    DeviceSimulator sim;
    sim.updateIOA(1, QVariant(42));
    sim.reset();
    EXPECT_FALSE(sim.getIOA(1).isValid());
}

// ──────────────────────────────────────────────────────────────
// Spontaneous data
// ──────────────────────────────────────────────────────────────

TEST(SpontaneousData, EmitsIoaChangedForAllIOAs)
{
    DeviceSimulator sim;
    sim.updateIOA(10, QVariant(1.0));
    sim.updateIOA(20, QVariant(2.0));
    // Clear spy set-up after pre-population
    QSignalSpy ioaSpy(&sim, &DeviceSimulator::ioaChanged);
    QSignalSpy sentSpy(&sim, &DeviceSimulator::spontaneousDataSent);

    sim.sendSpontaneousData();

    EXPECT_EQ(ioaSpy.count(),  2);   // one per IOA
    EXPECT_EQ(sentSpy.count(), 1);   // final signal
}

TEST(SpontaneousData, EmptyIOAsStillEmitsSent)
{
    DeviceSimulator sim;
    QSignalSpy spy(&sim, &DeviceSimulator::spontaneousDataSent);
    sim.sendSpontaneousData();
    EXPECT_EQ(spy.count(), 1);
}

// ──────────────────────────────────────────────────────────────
// loadScenario
// ──────────────────────────────────────────────────────────────

TEST(LoadScenario, ReturnsFalseForMissingFile)
{
    DeviceSimulator sim;
    EXPECT_FALSE(sim.loadScenario("/nonexistent/path.json"));
    EXPECT_FALSE(sim.lastScenarioError().isEmpty());
}

TEST(LoadScenario, ReturnsFalseForInvalidJson)
{
    QString path = writeTempFile("not json");
    DeviceSimulator sim;
    EXPECT_FALSE(sim.loadScenario(path));
    QFile::remove(path);
}

TEST(LoadScenario, ReturnsTrueAndEmitsSignal)
{
    QString path = writeTempFile(validScenarioJson());
    DeviceSimulator sim;
    QSignalSpy spy(&sim, &DeviceSimulator::scenarioLoaded);
    bool ok = sim.loadScenario(path);
    QFile::remove(path);

    EXPECT_TRUE(ok);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy[0][0].toString(), QStringLiteral("TestScenario"));
}

TEST(LoadScenario, ScenarioAccessibleAfterLoad)
{
    QString path = writeTempFile(validScenarioJson());
    DeviceSimulator sim;
    sim.loadScenario(path);
    QFile::remove(path);
    EXPECT_EQ(sim.currentScenario().steps.size(), 2);
}

// ──────────────────────────────────────────────────────────────
// Scenario execution tests
// ──────────────────────────────────────────────────────────────

TEST(ScenarioExecution, RunScenarioRequiresStart)
{
    QString path = writeTempFile(validScenarioJson());
    DeviceSimulator sim;
    sim.loadScenario(path);
    QFile::remove(path);

    // Not started — runScenario should be a no-op
    sim.runScenario();
    EXPECT_FALSE(sim.isScenarioRunning());
}

TEST(ScenarioExecution, RunScenarioRequiresLoadedScenario)
{
    DeviceSimulator sim;
    sim.start();
    sim.runScenario();
    EXPECT_FALSE(sim.isScenarioRunning());
}

TEST(ScenarioExecution, Step0ExecutedImmediately)
{
    QString path = writeTempFile(validScenarioJson(false, 500));
    DeviceSimulator sim;
    sim.loadScenario(path);
    QFile::remove(path);
    sim.start();
    sim.runScenario();

    // Step 0 actions execute synchronously before runScenario() returns
    EXPECT_EQ(sim.getRegisterValue(0), 42u);
    EXPECT_NEAR(sim.getIOA(1001).toDouble(), 3.14, 1e-6);
    sim.stopScenario();
}

TEST(ScenarioExecution, ScenarioStartedSignal)
{
    QString path = writeTempFile(validScenarioJson(false, 500));
    DeviceSimulator sim;
    sim.loadScenario(path);
    QFile::remove(path);
    sim.start();
    QSignalSpy spy(&sim, &DeviceSimulator::scenarioStarted);
    sim.runScenario();
    EXPECT_EQ(spy.count(), 1);
    sim.stopScenario();
}

TEST(ScenarioExecution, StopScenarioEmitsSignal)
{
    QString path = writeTempFile(validScenarioJson(false, 500));
    DeviceSimulator sim;
    sim.loadScenario(path);
    QFile::remove(path);
    sim.start();
    sim.runScenario();
    QSignalSpy spy(&sim, &DeviceSimulator::scenarioStopped);
    sim.stopScenario();
    EXPECT_EQ(spy.count(), 1);
    EXPECT_FALSE(sim.isScenarioRunning());
}

TEST(ScenarioExecution, AllStepsExecuted)
{
    // Use 10 ms delays and wait up to 2 s for both steps to complete
    QString path = writeTempFile(validScenarioJson(false, 10));
    DeviceSimulator sim;
    sim.loadScenario(path);
    QFile::remove(path);
    sim.start();

    QSignalSpy stepSpy(&sim, &DeviceSimulator::scenarioStepExecuted);
    QSignalSpy finSpy(&sim,  &DeviceSimulator::scenarioFinished);

    sim.runScenario();

    // Wait for scenario to finish (2 steps × 10 ms + margin)
    QTest::qWait(500);

    EXPECT_EQ(stepSpy.count(), 2);   // step 0 and step 1
    EXPECT_EQ(finSpy.count(),  1);
}

TEST(ScenarioExecution, Step1SetsValues)
{
    QString path = writeTempFile(validScenarioJson(false, 10));
    DeviceSimulator sim;
    sim.loadScenario(path);
    QFile::remove(path);
    sim.start();
    sim.runScenario();

    QTest::qWait(300);

    // Step 1 sets register 1 to 99
    EXPECT_EQ(sim.getRegisterValue(1), 99u);
}

TEST(ScenarioExecution, ScenarioFinishedSignalFired)
{
    QString path = writeTempFile(validScenarioJson(false, 10));
    DeviceSimulator sim;
    sim.loadScenario(path);
    QFile::remove(path);
    sim.start();

    QSignalSpy spy(&sim, &DeviceSimulator::scenarioFinished);
    sim.runScenario();
    QTest::qWait(300);

    EXPECT_EQ(spy.count(), 1);
    EXPECT_FALSE(sim.isScenarioRunning());
}

TEST(ScenarioExecution, LoopDoesNotEmitFinished)
{
    QString path = writeTempFile(validScenarioJson(true, 10));
    DeviceSimulator sim;
    sim.loadScenario(path);
    QFile::remove(path);
    sim.start();

    QSignalSpy finSpy(&sim, &DeviceSimulator::scenarioFinished);
    QSignalSpy stepSpy(&sim, &DeviceSimulator::scenarioStepExecuted);

    sim.runScenario();
    QTest::qWait(300);   // long enough for multiple loops

    EXPECT_EQ(finSpy.count(), 0);        // never finishes
    EXPECT_GE(stepSpy.count(), 4);       // at least 2 full loops = 4 step-executions
    sim.stopScenario();
}

TEST(ScenarioExecution, SendSpontaneousActionWorks)
{
    // Step 1 contains a send_spontaneous action
    QString path = writeTempFile(validScenarioJson(false, 10));
    DeviceSimulator sim;
    sim.loadScenario(path);
    QFile::remove(path);
    sim.start();
    // Pre-populate an IOA so spontaneous emits something meaningful
    sim.updateIOA(9999, QVariant(7));

    QSignalSpy spy(&sim, &DeviceSimulator::spontaneousDataSent);
    sim.runScenario();
    QTest::qWait(300);

    EXPECT_GE(spy.count(), 1);
}

// ──────────────────────────────────────────────────────────────
// Randomization
// ──────────────────────────────────────────────────────────────

TEST(Randomization, DefaultDisabled)
{
    DeviceSimulator sim;
    EXPECT_FALSE(sim.randomValuesEnabled());
}

TEST(Randomization, EnableDisable)
{
    DeviceSimulator sim;
    sim.enableRandomValues(true);
    EXPECT_TRUE(sim.randomValuesEnabled());
    sim.enableRandomValues(false);
    EXPECT_FALSE(sim.randomValuesEnabled());
}

TEST(Randomization, GlobalRange)
{
    DeviceSimulator sim;
    sim.setGlobalRange(10.0, 20.0);
    ValueRange r = sim.rangeFor(9999);   // no per-address range → global
    EXPECT_DOUBLE_EQ(r.min, 10.0);
    EXPECT_DOUBLE_EQ(r.max, 20.0);
}

TEST(Randomization, PerAddressRangeOverridesGlobal)
{
    DeviceSimulator sim;
    sim.setGlobalRange(0.0, 100.0);
    sim.setRange(42, 200.0, 300.0);
    ValueRange r = sim.rangeFor(42);
    EXPECT_DOUBLE_EQ(r.min, 200.0);
    EXPECT_DOUBLE_EQ(r.max, 300.0);
    // Other address still uses global
    EXPECT_DOUBLE_EQ(sim.rangeFor(43).max, 100.0);
}

TEST(Randomization, RandomRegisterValuesInRange)
{
    DeviceSimulator sim;
    sim.setRange(0, 100.0, 200.0);
    sim.enableRandomValues(true);

    // Build a 1-step scenario that sets register 0
    QByteArray json = R"({
        "name":"r","loop":false,
        "steps":[{"delay_ms":0,"actions":[
            {"type":"set_register","address":0,"value":9999}
        ]}]
    })";
    QString path = writeTempFile(json);
    sim.loadScenario(path);
    QFile::remove(path);
    sim.start();
    sim.runScenario();
    QTest::qWait(100);

    // Value should be in [100, 200] — NOT the literal 9999
    uint16_t v = sim.getRegisterValue(0);
    EXPECT_GE(v, 100u);
    EXPECT_LE(v, 200u);
}

TEST(Randomization, RandomIOAValuesInRange)
{
    DeviceSimulator sim;
    sim.setRange(1001, 50.0, 60.0);
    sim.enableRandomValues(true);

    QByteArray json = R"({
        "name":"r","loop":false,
        "steps":[{"delay_ms":0,"actions":[
            {"type":"set_ioa","address":1001,"value":9999}
        ]}]
    })";
    QString path = writeTempFile(json);
    sim.loadScenario(path);
    QFile::remove(path);
    sim.start();
    sim.runScenario();
    QTest::qWait(100);

    double v = sim.getIOA(1001).toDouble();
    EXPECT_GE(v, 50.0);
    EXPECT_LE(v, 60.0);
}

TEST(Randomization, RandomizeActionUpdatesAllValues)
{
    DeviceSimulator sim;
    // Pre-populate
    sim.setRegisterValue(0, 0);
    sim.setRegisterValue(1, 0);
    sim.updateIOA(100, QVariant(0.0));

    sim.setRange(0,   10.0,  20.0);
    sim.setRange(1,   30.0,  40.0);
    sim.setRange(100, 50.0,  60.0);
    sim.enableRandomValues(true);

    QByteArray json = R"({
        "name":"r","loop":false,
        "steps":[{"delay_ms":0,"actions":[{"type":"randomize"}]}]
    })";
    QString path = writeTempFile(json);
    sim.loadScenario(path);
    QFile::remove(path);
    sim.start();
    sim.runScenario();
    QTest::qWait(100);

    EXPECT_GE(sim.getRegisterValue(0), 10u);
    EXPECT_LE(sim.getRegisterValue(0), 20u);
    EXPECT_GE(sim.getRegisterValue(1), 30u);
    EXPECT_LE(sim.getRegisterValue(1), 40u);

    double ioa = sim.getIOA(100).toDouble();
    EXPECT_GE(ioa, 50.0);
    EXPECT_LE(ioa, 60.0);
}

// ──────────────────────────────────────────────────────────────
// main
// ──────────────────────────────────────────────────────────────

int main(int argc, char** argv)
{
    // QCoreApplication is required for QTimer and QSignalSpy
    QCoreApplication app(argc, argv);
    s_app = &app;

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
