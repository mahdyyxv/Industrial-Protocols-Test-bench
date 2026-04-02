#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include "core/models/ProtocolFrame.h"

namespace rtu::core::engine {

enum class SessionState {
    Idle,
    Running,
    Paused,
    Finished,
    Error
};

// ──────────────────────────────────────────────────────────────
// ITestSession
//
// Represents a single protocol test run.
// Emits frames in real-time; exposes state for UI binding.
//
// Pro-only features (script automation, export) are gated by the
// TestEngine before a session is created.
// ──────────────────────────────────────────────────────────────
class ITestSession : public QObject {
    Q_OBJECT
public:
    explicit ITestSession(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~ITestSession() = default;

    virtual QString      sessionId()   const = 0;
    virtual SessionState state()       const = 0;
    virtual QList<ProtocolFrame> frames() const = 0;

public slots:
    virtual void start()  = 0;
    virtual void pause()  = 0;
    virtual void resume() = 0;
    virtual void stop()   = 0;

signals:
    void stateChanged(SessionState newState);
    void frameReceived(const rtu::core::ProtocolFrame& frame);
    void errorOccurred(const QString& message);
};

} // namespace rtu::core::engine
