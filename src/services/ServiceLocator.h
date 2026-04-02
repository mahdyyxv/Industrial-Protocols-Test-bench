#pragma once

#include <memory>
#include "auth/IAuthService.h"
#include "subscription/ISubscriptionService.h"

namespace rtu::services {

// ──────────────────────────────────────────────────────────────
// ServiceLocator
//
// Central access point for application-wide services.
// Services are registered once at startup (in Application::init).
// All modules access services through this locator — no raw
// service pointers are passed through constructors.
//
// This keeps interfaces decoupled without full DI framework.
// ──────────────────────────────────────────────────────────────
class ServiceLocator {
public:
    static ServiceLocator& instance();

    void registerAuth(std::shared_ptr<IAuthService> auth);
    void registerSubscription(std::shared_ptr<ISubscriptionService> subscription);

    IAuthService*         auth()         const;
    ISubscriptionService* subscription() const;

private:
    ServiceLocator() = default;

    std::shared_ptr<IAuthService>         m_auth;
    std::shared_ptr<ISubscriptionService> m_subscription;
};

} // namespace rtu::services
