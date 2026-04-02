#include "SessionViewModel.h"

namespace rtu::ui {

// ── Stub implementations — Core Engineer fills these in Phase 3 ──

SessionViewModel::SessionViewModel(QObject* parent) : QObject(parent) {}

QString      SessionViewModel::connectionStatus() const { return QStringLiteral("disconnected"); }
bool         SessionViewModel::isConnected()      const { return false; }
bool         SessionViewModel::isRunning()        const { return false; }
QVariantList SessionViewModel::responseModel()    const { return {}; }
QStringList  SessionViewModel::rawLog()           const { return {}; }

void SessionViewModel::connect(const QVariantMap&)    {}
void SessionViewModel::disconnect()                   {}
void SessionViewModel::sendRequest(const QVariantMap&){}
void SessionViewModel::clearLog()                     {}

} // namespace rtu::ui
