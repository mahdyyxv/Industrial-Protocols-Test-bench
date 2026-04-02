#pragma once

#include <QString>

namespace rtu::services {

enum class SubscriptionTier {
    Free,
    Pro
};

// Feature flags — checked at runtime before unlocking capabilities
enum class Feature {
    // Free tier
    ModbusRTU,
    ModbusTCP,
    BasicLogging,
    ManualTesting,

    // Pro tier only
    DNP3,
    IEC101,
    IEC104,
    SessionExport,
    ScriptAutomation,
    AdvancedAnalysis,
    UnlimitedSessions
};

inline QString tierName(SubscriptionTier tier) {
    return tier == SubscriptionTier::Pro ? QStringLiteral("Pro") : QStringLiteral("Free");
}

} // namespace rtu::services
