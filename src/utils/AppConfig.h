#pragma once

#include <QSettings>
#include <QString>

namespace rtu::utils {

// ──────────────────────────────────────────────────────────────
// AppConfig
//
// Thin wrapper around QSettings for typed config access.
// Persists user preferences locally (Windows registry / ini).
// Does NOT store auth tokens — those are handled by IAuthService.
// ──────────────────────────────────────────────────────────────
class AppConfig {
public:
    static AppConfig& instance();

    QString lastUsedProtocol()   const;
    void    setLastUsedProtocol(const QString& name);

    bool    darkTheme()          const;
    void    setDarkTheme(bool enabled);

    QString language()           const;
    void    setLanguage(const QString& locale);

private:
    AppConfig();
    QSettings m_settings;
};

} // namespace rtu::utils
