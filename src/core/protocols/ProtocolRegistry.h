#pragma once

#include <QMap>
#include <memory>
#include "IProtocol.h"

namespace rtu::core::protocols {

// ──────────────────────────────────────────────────────────────
// ProtocolRegistry
//
// Singleton registry that maps ProtocolType → IProtocol factory.
// Protocols register themselves at startup; the engine retrieves
// them by type. Unregistered types are not available in the UI.
// ──────────────────────────────────────────────────────────────
class ProtocolRegistry {
public:
    using Factory = std::function<std::unique_ptr<IProtocol>()>;

    static ProtocolRegistry& instance();

    void registerProtocol(ProtocolType type, Factory factory);

    // Returns nullptr if type is unknown or not registered
    std::unique_ptr<IProtocol> create(ProtocolType type) const;

    // All registered protocol types (for populating UI lists)
    QList<ProtocolType> available() const;

private:
    ProtocolRegistry() = default;
    QMap<ProtocolType, Factory> m_factories;
};

} // namespace rtu::core::protocols
