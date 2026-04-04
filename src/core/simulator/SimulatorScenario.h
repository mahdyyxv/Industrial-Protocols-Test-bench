#pragma once

#include <QString>
#include <QVariant>
#include <QVector>
#include <QByteArray>

namespace rtu::core::simulator {

// ──────────────────────────────────────────────────────────────
// SimulatorAction
//
// One atomic operation within a scenario step.
//
// JSON "type" values:
//   "set_register"    — write a Modbus holding register
//   "set_ioa"         — write an IEC-104 IOA value
//   "send_spontaneous"— emit all current IOA values as spontaneous data
//   "randomize"       — randomize all configured registers and IOAs
// ──────────────────────────────────────────────────────────────
struct SimulatorAction {
    enum class Type {
        SetRegister,       // address = register addr, value = uint16 target
        SetIOA,            // address = IOA, value = QVariant target
        SendSpontaneous,   // no address / value needed
        Randomize          // apply random values to all configured addresses
    };

    Type     type    { Type::SetRegister };
    int      address { 0 };    // register address or IOA
    QVariant value;            // target value (ignored for SendSpontaneous / Randomize)
};

// ──────────────────────────────────────────────────────────────
// SimulatorStep
//
// A sequence of actions executed together, followed by a delay
// before the next step begins.
// ──────────────────────────────────────────────────────────────
struct SimulatorStep {
    int                      delayMs { 0 };   // pause after executing this step's actions
    QVector<SimulatorAction> actions;
};

// ──────────────────────────────────────────────────────────────
// SimulatorScenario
//
// Loaded from a JSON file.  Schema:
//
//   {
//     "name":  "Power Meter RTU",
//     "loop":  true,
//     "steps": [
//       {
//         "delay_ms": 1000,
//         "actions": [
//           { "type": "set_register", "address": 0,    "value": 230  },
//           { "type": "set_ioa",      "address": 1001, "value": 1.5  },
//           { "type": "send_spontaneous" },
//           { "type": "randomize" }
//         ]
//       }
//     ]
//   }
//
// Unknown action types are silently skipped.
// ──────────────────────────────────────────────────────────────
struct SimulatorScenario {
    QString                name;
    bool                   loop  { false };
    QVector<SimulatorStep> steps;

    bool isEmpty() const { return steps.isEmpty(); }

    // Parse from raw JSON bytes — returns default-constructed scenario on error.
    static SimulatorScenario fromJson(const QByteArray& json,
                                     QString*           error = nullptr);

    // Load and parse a JSON file — returns default-constructed on error.
    static SimulatorScenario fromFile(const QString& filePath,
                                     QString*        error = nullptr);
};

} // namespace rtu::core::simulator
