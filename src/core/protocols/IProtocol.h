#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>
#include "core/models/ProtocolFrame.h"

namespace rtu::core::protocols {

// Identifies each supported RTU/SCADA protocol
enum class ProtocolType {
    ModbusRTU,   // Free tier
    ModbusTCP,   // Free tier
    DNP3,        // Pro tier
    IEC101,      // Pro tier
    IEC104,      // Pro tier
    Unknown
};

// Connection medium
enum class TransportType {
    Serial,
    TCP,
    UDP
};

// ──────────────────────────────────────────────────────────────
// IProtocol
//
// Contract every protocol plugin must fulfill.
// Implementations live in src/core/protocols/<name>/
//
// NOTE: This is a pure C++ interface (not QObject) so it can be
//       composed freely without multiple-inheritance QObject issues.
//       Concrete classes inherit QObject separately and hold an
//       IProtocol* via the ProtocolRegistry.
// ──────────────────────────────────────────────────────────────
class IProtocol {
public:
    virtual ~IProtocol() = default;

    // Identity
    virtual ProtocolType  type()      const = 0;
    virtual QString       name()      const = 0;
    virtual TransportType transport() const = 0;

    // Feature gate — Pro-only protocols return true
    virtual bool requiresPro() const = 0;

    // Connection lifecycle
    virtual bool connect(const QVariantMap& connectionParams) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;

    // Frame operations
    virtual ProtocolFrame buildRequest(const QVariantMap& params) const = 0;
    virtual bool          sendFrame(const ProtocolFrame& frame)         = 0;

    // Validation (parse raw bytes and mark frame valid/invalid)
    virtual ProtocolFrame parseResponse(const QByteArray& raw) const = 0;
};

} // namespace rtu::core::protocols
