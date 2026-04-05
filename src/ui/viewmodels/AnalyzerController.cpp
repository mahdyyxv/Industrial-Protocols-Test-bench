#include "AnalyzerController.h"
#include "core/analyzer/PacketAnalyzer.h"
#include "core/analyzer/AnalyzedPacket.h"

namespace rtu::ui {

AnalyzerController::AnalyzerController(QObject* parent)
    : QObject(parent)
    , m_analyzer(std::make_unique<core::analyzer::PacketAnalyzer>(this))
{
    QObject::connect(m_analyzer.get(), &core::analyzer::PacketAnalyzer::captureStarted,
                     this, [this]() { emit capturingChanged(); });

    QObject::connect(m_analyzer.get(), &core::analyzer::PacketAnalyzer::captureStopped,
                     this, [this]() { emit capturingChanged(); });

    QObject::connect(m_analyzer.get(), &core::analyzer::PacketAnalyzer::packetStored,
                     this, [this](const core::analyzer::AnalyzedPacket& p) {
                         const QVariantMap map = packetToMap(p);
                         emit packetsChanged();
                         emit packetAdded(map);
                     });

    QObject::connect(m_analyzer.get(), &core::analyzer::PacketAnalyzer::packetsCleared,
                     this, [this]() { emit packetsChanged(); });
}

AnalyzerController::~AnalyzerController() = default;

// ── Accessors ─────────────────────────────────────────────────

bool AnalyzerController::capturing() const
{
    return m_analyzer->isCapturing();
}

int AnalyzerController::packetCount() const
{
    return m_analyzer->packetCount();
}

QVariantList AnalyzerController::packets() const
{
    return getPackets();
}

QVariantMap AnalyzerController::statistics() const
{
    return m_analyzer->statistics();
}

// ── QML API ───────────────────────────────────────────────────

void AnalyzerController::startCapture()
{
    m_analyzer->startCapture();
}

void AnalyzerController::stopCapture()
{
    m_analyzer->stopCapture();
}

void AnalyzerController::clearPackets()
{
    m_analyzer->clearPackets();
}

QVariantList AnalyzerController::getPackets() const
{
    QVariantList result;
    const auto pkts = m_analyzer->getPackets();
    result.reserve(pkts.size());
    for (const auto& p : pkts)
        result.append(packetToMap(p));
    return result;
}

void AnalyzerController::feedTcpData(const QByteArray& data)
{
    m_analyzer->processTcpPacket(data);
}

void AnalyzerController::feedSerialData(const QByteArray& data)
{
    m_analyzer->processSerialData(data);
}

// ── Private helpers ───────────────────────────────────────────

QVariantMap AnalyzerController::packetToMap(const core::analyzer::AnalyzedPacket& p)
{
    QVariantMap map;
    map[QStringLiteral("id")]          = p.id;
    map[QStringLiteral("timestamp")]   = p.timestamp.toString(QStringLiteral("hh:mm:ss.zzz"));
    map[QStringLiteral("protocol")]    = core::analyzer::PacketAnalyzer::protocolName(p.protocol);
    map[QStringLiteral("direction")]   = (p.direction == rtu::core::FrameDirection::Sent)
                                         ? QStringLiteral("TX") : QStringLiteral("RX");
    map[QStringLiteral("length")]      = p.raw.size();
    map[QStringLiteral("summary")]     = p.summary;
    map[QStringLiteral("valid")]       = p.valid;
    map[QStringLiteral("errorReason")] = p.errorReason;
    map[QStringLiteral("fields")]      = p.fields;
    map[QStringLiteral("hex")]         = QString::fromLatin1(p.raw.toHex(' ')).toUpper();
    return map;
}

} // namespace rtu::ui
