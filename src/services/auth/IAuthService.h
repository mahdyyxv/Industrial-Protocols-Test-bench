#pragma once

#include <QObject>
#include <QString>

namespace rtu::services {

enum class AuthState {
    LoggedOut,
    Authenticating,
    LoggedIn,
    TokenExpired,
    Error
};

// ──────────────────────────────────────────────────────────────
// IAuthService
//
// Contract for authentication flow.
// The concrete implementation will use the backend API (TBD).
//
// On successful login → ISubscriptionService::refresh() is called
// automatically by the ServiceLocator bootstrap.
// ──────────────────────────────────────────────────────────────
class IAuthService : public QObject {
    Q_OBJECT
public:
    explicit IAuthService(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~IAuthService() = default;

    virtual AuthState state()        const = 0;
    virtual QString   currentUserId() const = 0;
    virtual QString   accessToken()   const = 0;

public slots:
    virtual void login(const QString& email, const QString& password) = 0;
    virtual void logout()        = 0;
    virtual void refreshToken()  = 0;

signals:
    void stateChanged(AuthState newState);
    void loginSucceeded(const QString& userId);
    void loginFailed(const QString& reason);
    void loggedOut();
};

} // namespace rtu::services
