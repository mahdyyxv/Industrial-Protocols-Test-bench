#include "TestEngine.h"

namespace rtu::core::engine {

TestEngine::TestEngine(QObject* parent)
    : QObject(parent)
{}

void TestEngine::setProtocol(std::unique_ptr<protocols::IProtocol> protocol)
{
    m_protocol = std::move(protocol);
    emit protocolChanged(m_protocol ? m_protocol->name() : QString{});
}

const protocols::IProtocol* TestEngine::activeProtocol() const
{
    return m_protocol.get();
}

std::unique_ptr<ITestSession> TestEngine::createSession(const QVariantMap& /*params*/)
{
    if (!m_protocol) {
        emit sessionCreationFailed(QStringLiteral("No protocol set"));
        return nullptr;
    }
    return nullptr; // concrete sessions implemented in integration phase
}

} // namespace rtu::core::engine
