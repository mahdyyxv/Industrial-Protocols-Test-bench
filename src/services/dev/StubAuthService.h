#pragma once

#include "services/auth/IAuthService.h"

namespace rtu::services::dev {

// ──────────────────────────────────────────────────────────────
// StubAuthService
//
// Development-only auth stub.
// Accepts any non-empty email/password combination.
// Replaced by the real backend service in Integration phase.
//
// Registered only when RTU_DEV_STUB_SERVICES is defined.
// ──────────────────────────────────────────────────────────────
class StubAuthService final : public IAuthService {
    Q_OBJECT
public:
    explicit StubAuthService(QObject* parent = nullptr);

    AuthState state()         const override;
    QString   currentUserId() const override;
    QString   accessToken()   const override;

public slots:
    void login(const QString& email, const QString& password) override;
    void logout()       override;
    void refreshToken() override;

private:
    AuthState m_state  { AuthState::LoggedOut };
    QString   m_userId;
    QString   m_token  { QStringLiteral("dev-stub-token") };
};

} // namespace rtu::services::dev
