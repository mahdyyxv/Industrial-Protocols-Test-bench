#include "DeviceSimulator.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QRandomGenerator>

namespace rtu::core::simulator {

// ──────────────────────────────────────────────────────────────
// SimulatorScenario — JSON parsing
// ──────────────────────────────────────────────────────────────

static SimulatorAction::Type parseActionType(const QString& s)
{
    if (s == QLatin1String("set_register"))     return SimulatorAction::Type::SetRegister;
    if (s == QLatin1String("set_ioa"))          return SimulatorAction::Type::SetIOA;
    if (s == QLatin1String("send_spontaneous")) return SimulatorAction::Type::SendSpontaneous;
    if (s == QLatin1String("randomize"))        return SimulatorAction::Type::Randomize;
    return SimulatorAction::Type::SetRegister;   // fallback — caller checks validity
}

SimulatorScenario SimulatorScenario::fromJson(const QByteArray& json, QString* error)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        if (error) *error = parseError.errorString();
        return {};
    }
    if (!doc.isObject()) {
        if (error) *error = QStringLiteral("Root element must be a JSON object");
        return {};
    }

    const QJsonObject root = doc.object();
    SimulatorScenario scenario;
    scenario.name = root.value(QLatin1String("name")).toString(QStringLiteral("Unnamed"));
    scenario.loop = root.value(QLatin1String("loop")).toBool(false);

    const QJsonArray stepsArray = root.value(QLatin1String("steps")).toArray();
    scenario.steps.reserve(stepsArray.size());

    for (const QJsonValue& stepVal : stepsArray) {
        const QJsonObject stepObj = stepVal.toObject();
        SimulatorStep step;
        step.delayMs = stepObj.value(QLatin1String("delay_ms")).toInt(0);

        const QJsonArray actionsArray = stepObj.value(QLatin1String("actions")).toArray();
        step.actions.reserve(actionsArray.size());

        for (const QJsonValue& actionVal : actionsArray) {
            const QJsonObject actionObj = actionVal.toObject();
            const QString typeStr = actionObj.value(QLatin1String("type")).toString();

            // Skip completely unknown types
            const bool knownType =
                typeStr == QLatin1String("set_register")     ||
                typeStr == QLatin1String("set_ioa")          ||
                typeStr == QLatin1String("send_spontaneous") ||
                typeStr == QLatin1String("randomize");
            if (!knownType) continue;

            SimulatorAction action;
            action.type = parseActionType(typeStr);

            if (action.type == SimulatorAction::Type::SetRegister ||
                action.type == SimulatorAction::Type::SetIOA)
            {
                action.address = actionObj.value(QLatin1String("address")).toInt(0);
                action.value   = actionObj.value(QLatin1String("value")).toVariant();
            }

            step.actions.append(action);
        }

        scenario.steps.append(step);
    }

    return scenario;
}

SimulatorScenario SimulatorScenario::fromFile(const QString& filePath, QString* error)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error)
            *error = QStringLiteral("Cannot open file: %1").arg(file.errorString());
        return {};
    }
    return fromJson(file.readAll(), error);
}

// ──────────────────────────────────────────────────────────────
// DeviceSimulator
// ──────────────────────────────────────────────────────────────

DeviceSimulator::DeviceSimulator(QObject* parent)
    : QObject(parent)
    , m_stepTimer(new QTimer(this))
{
    m_stepTimer->setSingleShot(true);
    connect(m_stepTimer, &QTimer::timeout, this, &DeviceSimulator::onStepTimer);
}

// ── Device control ────────────────────────────────────────────

void DeviceSimulator::start()
{
    if (m_running) return;
    m_running = true;
    emit started();
}

void DeviceSimulator::stop()
{
    if (!m_running) return;
    stopScenario();
    m_running = false;
    emit stopped();
}

void DeviceSimulator::reset()
{
    stopScenario();
    m_registers.clear();
    m_ioaValues.clear();
    m_currentStep = 0;
    emit resetDone();
}

bool DeviceSimulator::isRunning() const { return m_running; }

// ── Modbus simulation ─────────────────────────────────────────

void DeviceSimulator::setRegisterValue(int address, uint16_t value)
{
    m_registers[address] = value;
    emit registerChanged(address, value);
}

uint16_t DeviceSimulator::getRegisterValue(int address) const
{
    return m_registers.value(address, 0);
}

QMap<int, uint16_t> DeviceSimulator::allRegisters() const
{
    return m_registers;
}

// ── IEC-104 simulation ────────────────────────────────────────

void DeviceSimulator::sendSpontaneousData()
{
    for (auto it = m_ioaValues.cbegin(); it != m_ioaValues.cend(); ++it)
        emit ioaChanged(it.key(), it.value());
    emit spontaneousDataSent();
}

void DeviceSimulator::updateIOA(int ioa, const QVariant& value)
{
    m_ioaValues[ioa] = value;
    emit ioaChanged(ioa, value);
}

QVariant DeviceSimulator::getIOA(int ioa) const
{
    return m_ioaValues.value(ioa);
}

QMap<int, QVariant> DeviceSimulator::allIOAs() const
{
    return m_ioaValues;
}

// ── Scenario ──────────────────────────────────────────────────

bool DeviceSimulator::loadScenario(const QString& filePath)
{
    QString err;
    SimulatorScenario s = SimulatorScenario::fromFile(filePath, &err);

    if (s.isEmpty() && !err.isEmpty()) {
        m_lastScenarioError = err;
        return false;
    }
    if (s.isEmpty()) {
        m_lastScenarioError = QStringLiteral("Scenario contains no steps");
        return false;
    }

    m_scenario           = std::move(s);
    m_lastScenarioError.clear();
    emit scenarioLoaded(m_scenario.name);
    return true;
}

void DeviceSimulator::runScenario()
{
    if (!m_running || m_scenario.isEmpty()) return;
    if (m_scenarioRunning) stopScenario();

    m_currentStep    = 0;
    m_scenarioRunning = true;
    emit scenarioStarted();

    executeStep(0);
    scheduleStep(m_scenario.steps[0].delayMs);
}

void DeviceSimulator::stopScenario()
{
    if (!m_scenarioRunning) return;
    m_stepTimer->stop();
    m_scenarioRunning = false;
    emit scenarioStopped();
}

bool DeviceSimulator::isScenarioRunning() const { return m_scenarioRunning; }

SimulatorScenario DeviceSimulator::currentScenario() const { return m_scenario; }

QString DeviceSimulator::lastScenarioError() const { return m_lastScenarioError; }

// ── Scenario step execution ───────────────────────────────────

void DeviceSimulator::executeStep(int index)
{
    if (index < 0 || index >= m_scenario.steps.size()) return;

    const SimulatorStep& step = m_scenario.steps[index];

    for (const SimulatorAction& action : step.actions) {
        switch (action.type) {

            case SimulatorAction::Type::SetRegister: {
                const uint16_t val = m_randomEnabled
                    ? randomRegisterValue(action.address)
                    : static_cast<uint16_t>(action.value.toUInt());
                setRegisterValue(action.address, val);
                break;
            }

            case SimulatorAction::Type::SetIOA: {
                const QVariant val = m_randomEnabled
                    ? randomIoaValue(action.address)
                    : action.value;
                updateIOA(action.address, val);
                break;
            }

            case SimulatorAction::Type::SendSpontaneous:
                sendSpontaneousData();
                break;

            case SimulatorAction::Type::Randomize:
                applyRandomToAll();
                break;
        }
    }

    emit scenarioStepExecuted(index);
}

void DeviceSimulator::scheduleStep(int delayMs)
{
    // Always use an async timer — even 0 ms defers to the next event-loop
    // iteration, avoiding deep call stacks when every step has 0 delay.
    m_stepTimer->start(qMax(0, delayMs));
}

void DeviceSimulator::onStepTimer()
{
    if (!m_scenarioRunning) return;

    int nextStep = m_currentStep + 1;

    if (nextStep >= m_scenario.steps.size()) {
        if (m_scenario.loop) {
            nextStep = 0;
        } else {
            m_scenarioRunning = false;
            emit scenarioFinished();
            return;
        }
    }

    m_currentStep = nextStep;
    executeStep(m_currentStep);
    scheduleStep(m_scenario.steps[m_currentStep].delayMs);
}

// ── Randomization ─────────────────────────────────────────────

void DeviceSimulator::enableRandomValues(bool enabled) { m_randomEnabled = enabled; }
bool DeviceSimulator::randomValuesEnabled()       const { return m_randomEnabled; }

void DeviceSimulator::setRange(int address, double min, double max)
{
    m_ranges[address] = { min, max };
}

void DeviceSimulator::setGlobalRange(double min, double max)
{
    m_globalRange = { min, max };
}

ValueRange DeviceSimulator::rangeFor(int address) const
{
    return m_ranges.value(address, m_globalRange);
}

double DeviceSimulator::randomInRange(const ValueRange& r) const
{
    return r.min + QRandomGenerator::global()->generateDouble() * (r.max - r.min);
}

uint16_t DeviceSimulator::randomRegisterValue(int address) const
{
    const double v = randomInRange(rangeFor(address));
    return static_cast<uint16_t>(qBound(0.0, v, 65535.0));
}

QVariant DeviceSimulator::randomIoaValue(int address) const
{
    return QVariant(randomInRange(rangeFor(address)));
}

void DeviceSimulator::applyRandomToAll()
{
    for (auto it = m_registers.begin(); it != m_registers.end(); ++it)
        setRegisterValue(it.key(), randomRegisterValue(it.key()));

    for (auto it = m_ioaValues.begin(); it != m_ioaValues.end(); ++it)
        updateIOA(it.key(), randomIoaValue(it.key()));
}

} // namespace rtu::core::simulator
