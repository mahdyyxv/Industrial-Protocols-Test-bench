#include "AppViewModel.h"
#include "services/ServiceLocator.h"

namespace rtu::ui {

AppViewModel::AppViewModel(QObject* parent)
    : QObject(parent)
{
    connectServices();
}

void AppViewModel::connectServices()
{
    m_auth         = services::ServiceLocator::instance().auth();
    m_subscription = services::ServiceLocator::instance().subscription();

    if (m_auth) {
        connect(m_auth, &services::IAuthService::stateChanged,
                this,   &AppViewModel::authStateChanged);
        connect(m_auth, &services::IAuthService::loginFailed,
                this,   &AppViewModel::loginFailed);
    }

    if (m_subscription) {
        connect(m_subscription, &services::ISubscriptionService::tierChanged,
                this,           &AppViewModel::subscriptionChanged);
    }
}

bool AppViewModel::isLoggedIn() const
{
    if (!m_auth) return false;
    return m_auth->state() == services::AuthState::LoggedIn;
}

bool AppViewModel::isPro() const
{
    if (!m_subscription) return false;
    return m_subscription->currentTier() == services::SubscriptionTier::Pro;
}

QString AppViewModel::userId() const
{
    if (!m_auth) return {};
    return m_auth->currentUserId();
}

QString AppViewModel::tierName() const
{
    return isPro() ? QStringLiteral("Pro") : QStringLiteral("Free");
}

void AppViewModel::login(const QString& email, const QString& password)
{
    if (m_auth) m_auth->login(email, password);
}

void AppViewModel::logout()
{
    if (m_auth) m_auth->logout();
}

} // namespace rtu::ui
