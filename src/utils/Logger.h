#pragma once

#include <QLoggingCategory>

// Application-wide logging categories
// Usage: qCDebug(rtuCore) << "message";

Q_DECLARE_LOGGING_CATEGORY(rtuApp)
Q_DECLARE_LOGGING_CATEGORY(rtuCore)
Q_DECLARE_LOGGING_CATEGORY(rtuProtocol)
Q_DECLARE_LOGGING_CATEGORY(rtuServices)
Q_DECLARE_LOGGING_CATEGORY(rtuUI)
