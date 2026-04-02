#pragma once

#include "services/subscription/ISubscriptionService.h"

namespace rtu::services::dev {

// ──────────────────────────────────────────────────────────────
// StubSubscriptionService
//
// Development-only subscription stub.
// Returns Pro tier so all features are unlocked during development.
// Toggle RTU_DEV_TIER_FREE to test Free-tier gating.
// ──────────────────────────────────────────────────────────────
class StubSubscriptionService final : public ISubscriptionService {
    Q_OBJECT
public:
    explicit StubSubscriptionService(QObject* parent = nullptr);

    SubscriptionTier currentTier()               const override;
    bool             hasFeature(Feature feature)  const override;
    void             refresh()                          override;

    // Toggle at runtime for UI testing
    void setTier(SubscriptionTier tier);

private:
    SubscriptionTier m_tier { SubscriptionTier::Pro };
};

} // namespace rtu::services::dev
