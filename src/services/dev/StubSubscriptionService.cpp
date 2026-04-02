#include "StubSubscriptionService.h"
#include <QSet>

namespace rtu::services::dev {

// Feature sets per tier
static const QSet<Feature> s_freeFeatures = {
    Feature::ModbusRTU,
    Feature::ModbusTCP,
    Feature::BasicLogging,
    Feature::ManualTesting,
};

StubSubscriptionService::StubSubscriptionService(QObject* parent)
    : ISubscriptionService(parent) {}

SubscriptionTier StubSubscriptionService::currentTier() const { return m_tier; }

bool StubSubscriptionService::hasFeature(Feature feature) const
{
    if (m_tier == SubscriptionTier::Pro) return true;
    return s_freeFeatures.contains(feature);
}

void StubSubscriptionService::refresh() { /* no-op */ }

void StubSubscriptionService::setTier(SubscriptionTier tier)
{
    if (m_tier == tier) return;
    m_tier = tier;
    emit tierChanged(m_tier);
}

} // namespace rtu::services::dev
