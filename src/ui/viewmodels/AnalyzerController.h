#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <memory>

namespace rtu::core::analyzer {
    class PacketAnalyzer;
    struct AnalyzedPacket;
}

namespace rtu::ui {

// ──────────────────────────────────────────────────────────────
// AnalyzerController
//
// QML-facing bridge for PacketAnalyzer.
// Exposed to QML as context property "AnalyzerVM".
// ──────────────────────────────────────────────────────────────
class AnalyzerController : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool         capturing   READ capturing   NOTIFY capturingChanged)
    Q_PROPERTY(int          packetCount READ packetCount NOTIFY packetsChanged)
    Q_PROPERTY(QVariantList packets     READ packets     NOTIFY packetsChanged)
    Q_PROPERTY(QVariantMap  statistics  READ statistics  NOTIFY packetsChanged)

public:
    explicit AnalyzerController(QObject* parent = nullptr);
    ~AnalyzerController() override;

    bool         capturing()   const;
    int          packetCount() const;
    QVariantList packets()     const;
    QVariantMap  statistics()  const;

    // ── QML API ───────────────────────────────────────────────

    Q_INVOKABLE void startCapture();
    Q_INVOKABLE void stopCapture();
    Q_INVOKABLE void clearPackets();

    // Returns all packets as a QVariantList of QVariantMaps.
    // Each map mirrors AnalyzedPacket fields.
    Q_INVOKABLE QVariantList getPackets() const;

    // Feed raw TCP data into the analyzer (for wiring from controllers).
    Q_INVOKABLE void feedTcpData(const QByteArray& data);

    // Feed raw serial data into the analyzer.
    Q_INVOKABLE void feedSerialData(const QByteArray& data);

signals:
    void capturingChanged();
    void packetsChanged();
    void packetAdded(const QVariantMap& packet);

private:
    static QVariantMap packetToMap(const core::analyzer::AnalyzedPacket& p);

    std::unique_ptr<core::analyzer::PacketAnalyzer> m_analyzer;
};

} // namespace rtu::ui
