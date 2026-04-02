#pragma once

#include <QObject>
#include "SubscriptionTier.h"

namespace rtu::services {

// ──────────────────────────────────────────────────────────────
// ISubscriptionService
//
// Contract for querying the user's subscription state.
// The concrete implementation will talk to the backend (TBD).
//
// All feature gates in Core and UI go through this interface.
// ──────────────────────────────────────────────────────────────
class ISubscriptionService : public QObject {
    Q_OBJECT
public:
    explicit ISubscriptionService(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~ISubscriptionService() = default;

    virtual SubscriptionTier currentTier()              const = 0;
    virtual bool             hasFeature(Feature feature) const = 0;

    // Trigger re-validation against backend (e.g., after login)
    virtual void refresh() = 0;

signals:
    void tierChanged(SubscriptionTier newTier);
};

} // namespace rtu::services
