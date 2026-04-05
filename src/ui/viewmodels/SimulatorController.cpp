#include "SimulatorController.h"
#include "core/simulator/DeviceSimulator.h"

namespace rtu::ui {

SimulatorController::SimulatorController(QObject* parent)
    : QObject(parent)
    , m_sim(std::make_unique<core::simulator::DeviceSimulator>(this))
{
    QObject::connect(m_sim.get(), &core::simulator::DeviceSimulator::started,
                     this, [this]() { emit runningChanged(); });
    QObject::connect(m_sim.get(), &core::simulator::DeviceSimulator::stopped,
                     this, [this]() { emit runningChanged(); });
    QObject::connect(m_sim.get(), &core::simulator::DeviceSimulator::resetDone,
                     this, [this]() { rebuildRegisterMap(); rebuildIoaMap(); });

    QObject::connect(m_sim.get(), &core::simulator::DeviceSimulator::registerChanged,
                     this, [this](int, uint16_t) { rebuildRegisterMap(); });
    QObject::connect(m_sim.get(), &core::simulator::DeviceSimulator::ioaChanged,
                     this, [this](int, const QVariant&) { rebuildIoaMap(); });

    QObject::connect(m_sim.get(), &core::simulator::DeviceSimulator::scenarioStarted,
                     this, [this]() { emit scenarioRunningChanged(); });
    QObject::connect(m_sim.get(), &core::simulator::DeviceSimulator::scenarioStopped,
                     this, [this]() { emit scenarioRunningChanged(); });
    QObject::connect(m_sim.get(), &core::simulator::DeviceSimulator::scenarioFinished,
                     this, [this]() { emit scenarioRunningChanged(); emit scenarioFinished(); });
}

SimulatorController::~SimulatorController() = default;

// ── Accessors ─────────────────────────────────────────────────

bool         SimulatorController::running()         const { return m_sim->isRunning(); }
bool         SimulatorController::scenarioRunning() const { return m_sim->isScenarioRunning(); }
bool         SimulatorController::randomEnabled()   const { return m_sim->randomValuesEnabled(); }
QString      SimulatorController::lastError()       const { return m_lastError; }
QVariantList SimulatorController::registerMap()     const { return m_registerMap; }
QVariantList SimulatorController::ioaMap()          const { return m_ioaMap; }

// ── Device control ────────────────────────────────────────────

void SimulatorController::start()  { m_sim->start(); }
void SimulatorController::stop()   { m_sim->stop(); }
void SimulatorController::reset()  { m_sim->reset(); }

// ── Modbus registers ──────────────────────────────────────────

void SimulatorController::setRegister(int address, int value)
{
    m_sim->setRegisterValue(address, static_cast<uint16_t>(value));
}

int SimulatorController::getRegister(int address) const
{
    return static_cast<int>(m_sim->getRegisterValue(address));
}

void SimulatorController::removeRegister(int address)
{
    // DeviceSimulator doesn't have a removeRegister, but we can zero it
    // and rebuild the map excluding that address. We track removal via
    // the QMap in the simulator; set to 0 and rebuild the display map.
    Q_UNUSED(address)
    // No removal API on DeviceSimulator — calling setRegister(addr, 0) is
    // the closest semantic. The display map will show addr=0 as "zeroed".
    m_sim->setRegisterValue(address, 0);
}

// ── IEC-104 IOAs ──────────────────────────────────────────────

void SimulatorController::updateIOA(int ioa, const QVariant& value)
{
    m_sim->updateIOA(ioa, value);
}

QVariant SimulatorController::getIOA(int ioa) const
{
    return m_sim->getIOA(ioa);
}

// ── Scenario ──────────────────────────────────────────────────

bool SimulatorController::loadScenario(const QString& filePath)
{
    const bool ok = m_sim->loadScenario(filePath);
    if (!ok) {
        m_lastError = m_sim->lastScenarioError();
        emit errorOccurred(m_lastError);
    }
    return ok;
}

void SimulatorController::runScenario()   { m_sim->runScenario(); }
void SimulatorController::stopScenario()  { m_sim->stopScenario(); }

// ── Randomization ─────────────────────────────────────────────

void SimulatorController::enableRandom(bool enabled)
{
    m_sim->enableRandomValues(enabled);
    emit randomEnabledChanged();
}

void SimulatorController::setRange(int address, double min, double max)
{
    m_sim->setRange(address, min, max);
}

void SimulatorController::setGlobalRange(double min, double max)
{
    m_sim->setGlobalRange(min, max);
}

// ── Private helpers ───────────────────────────────────────────

void SimulatorController::rebuildRegisterMap()
{
    m_registerMap.clear();
    const auto regs = m_sim->allRegisters();
    for (auto it = regs.cbegin(); it != regs.cend(); ++it) {
        QVariantMap row;
        row[QStringLiteral("address")]     = it.key();
        row[QStringLiteral("value")]       = it.value();
        row[QStringLiteral("hex")]         = QStringLiteral("0x%1").arg(it.value(), 4, 16, QChar('0')).toUpper();
        row[QStringLiteral("description")] = QStringLiteral("Holding Register %1").arg(it.key());
        m_registerMap.append(row);
    }
    emit registerMapChanged();
}

void SimulatorController::rebuildIoaMap()
{
    m_ioaMap.clear();
    const auto ioaValues = m_sim->allIOAs();
    for (auto it = ioaValues.cbegin(); it != ioaValues.cend(); ++it) {
        QVariantMap row;
        row[QStringLiteral("ioa")]   = it.key();
        row[QStringLiteral("value")] = it.value();
        m_ioaMap.append(row);
    }
    emit ioaMapChanged();
}

} // namespace rtu::ui
