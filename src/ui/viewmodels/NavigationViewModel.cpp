#include "NavigationViewModel.h"

namespace rtu::ui {

// ── Stub — Integration Engineer wires this if needed ──

NavigationViewModel::NavigationViewModel(QObject* parent) : QObject(parent) {}

QString NavigationViewModel::currentPageId() const { return QStringLiteral("dashboard"); }

void NavigationViewModel::navigateTo(const QString&) {}

} // namespace rtu::ui
