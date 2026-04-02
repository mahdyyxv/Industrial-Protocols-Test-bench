#pragma once

#include <QObject>
#include <QString>
#include "services/auth/IAuthService.h"
#include "services/subscription/ISubscriptionService.h"

namespace rtu::ui {

// ──────────────────────────────────────────────────────────────
// AppViewModel
//
// Root ViewModel exposed to QML via setContextProperty("AppVM").
// Bridges auth/subscription state into QML-bindable properties.
//
// Registered in Application::initServices() as:
//   engine.rootContext()->setContextProperty("AppVM", appViewModel);
//
// UI Engineer: bind your root window to these properties.
// Core Engineer: do NOT add protocol logic here.
// ──────────────────────────────────────────────────────────────
class AppViewModel : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool isLoggedIn   READ isLoggedIn   NOTIFY authStateChanged)
    Q_PROPERTY(bool isPro        READ isPro        NOTIFY subscriptionChanged)
    Q_PROPERTY(QString userId    READ userId       NOTIFY authStateChanged)
    Q_PROPERTY(QString tierName  READ tierName     NOTIFY subscriptionChanged)

public:
    explicit AppViewModel(QObject* parent = nullptr);

    bool    isLoggedIn() const;
    bool    isPro()      const;
    QString userId()     const;
    QString tierName()   const;

    // Called from QML login form
    Q_INVOKABLE void login(const QString& email, const QString& password);
    Q_INVOKABLE void logout();

signals:
    void authStateChanged();
    void subscriptionChanged();
    void loginFailed(const QString& reason);

private:
    void connectServices();

    services::IAuthService*         m_auth         { nullptr };
    services::ISubscriptionService* m_subscription { nullptr };
};

} // namespace rtu::ui
