#pragma once

#include <QByteArray>
#include <QString>
#include <QDateTime>
#include <QVariantMap>

namespace rtu::core {

// Direction of a protocol frame on the wire
enum class FrameDirection {
    Sent,
    Received
};

// A single protocol data unit captured during a test session
struct ProtocolFrame {
    QByteArray      raw;          // Raw bytes on the wire
    QString         description;  // Human-readable summary
    QDateTime       timestamp;
    FrameDirection  direction;
    bool            valid;        // Passed protocol-level validation
    QString         errorReason;  // Set when valid == false
};

} // namespace rtu::core
