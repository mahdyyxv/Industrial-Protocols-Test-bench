#include "StubAuthService.h"
#include <QTimer>

namespace rtu::services::dev {

StubAuthService::StubAuthService(QObject* parent) : IAuthService(parent) {}

AuthState StubAuthService::state()         const { return m_state; }
QString   StubAuthService::currentUserId() const { return m_userId; }
QString   StubAuthService::accessToken()   const { return m_token; }

void StubAuthService::login(const QString& email, const QString& password)
{
    if (email.isEmpty() || password.isEmpty()) {
        emit loginFailed(QStringLiteral("Email and password are required."));
        return;
    }

    // Simulate a short async delay so the UI loading state is visible
    m_state = AuthState::Authenticating;
    emit stateChanged(m_state);

    QTimer::singleShot(400, this, [this, email]() {
        m_userId = email;
        m_state  = AuthState::LoggedIn;
        emit stateChanged(m_state);
        emit loginSucceeded(m_userId);
    });
}

void StubAuthService::logout()
{
    m_userId.clear();
    m_state = AuthState::LoggedOut;
    emit stateChanged(m_state);
    emit loggedOut();
}

void StubAuthService::refreshToken() { /* no-op in stub */ }

} // namespace rtu::services::dev
