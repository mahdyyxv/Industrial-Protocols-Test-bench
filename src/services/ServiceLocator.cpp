#include "ServiceLocator.h"
#include <stdexcept>

namespace rtu::services {

ServiceLocator& ServiceLocator::instance()
{
    static ServiceLocator s;
    return s;
}

void ServiceLocator::registerAuth(std::shared_ptr<IAuthService> auth)
{
    m_auth = std::move(auth);
}

void ServiceLocator::registerSubscription(std::shared_ptr<ISubscriptionService> sub)
{
    m_subscription = std::move(sub);
}

IAuthService* ServiceLocator::auth() const
{
    return m_auth.get();
}

ISubscriptionService* ServiceLocator::subscription() const
{
    return m_subscription.get();
}

} // namespace rtu::services
