#pragma once

#include <QObject>
#include <memory>
#include "ITestSession.h"
#include "core/protocols/IProtocol.h"

namespace rtu::core::engine {

// ──────────────────────────────────────────────────────────────
// TestEngine
//
// Central coordinator. Owns the active IProtocol instance and
// creates/manages ITestSession objects.
//
// Responsibilities:
//   - Enforce Pro feature gates before session creation
//   - Delegate protocol lifecycle to IProtocol
//   - Expose session factory to ViewModels
//
// Does NOT own UI state — ViewModels observe sessions via signals.
// ──────────────────────────────────────────────────────────────
class TestEngine : public QObject {
    Q_OBJECT
public:
    explicit TestEngine(QObject* parent = nullptr);

    // Sets the active protocol; replaces any existing one
    void setProtocol(std::unique_ptr<protocols::IProtocol> protocol);

    const protocols::IProtocol* activeProtocol() const;

    // Creates a new session for the active protocol.
    // Returns nullptr if:
    //   - No protocol is set
    //   - Protocol requires Pro and user is on Free tier
    std::unique_ptr<ITestSession> createSession(const QVariantMap& params);

signals:
    void protocolChanged(const QString& protocolName);
    void sessionCreationFailed(const QString& reason);

private:
    std::unique_ptr<protocols::IProtocol> m_protocol;
};

} // namespace rtu::core::engine
