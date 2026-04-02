#pragma once

#include <QObject>
#include <QVariantList>
#include <QString>

namespace rtu::ui {

// ──────────────────────────────────────────────────────────────
// SessionViewModel  (stub — Core Engineer fills implementation)
//
// Exposes a single test session's state to QML pages.
// One SessionViewModel per active protocol test page.
// ──────────────────────────────────────────────────────────────
// Exposed via: engine.rootContext()->setContextProperty("SessionVM", ...)
class SessionViewModel : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString  connectionStatus READ connectionStatus NOTIFY connectionChanged)
    Q_PROPERTY(bool     isConnected      READ isConnected      NOTIFY connectionChanged)
    Q_PROPERTY(bool     isRunning        READ isRunning        NOTIFY sessionStateChanged)
    Q_PROPERTY(QVariantList responseModel READ responseModel   NOTIFY dataChanged)
    Q_PROPERTY(QStringList  rawLog        READ rawLog          NOTIFY dataChanged)

public:
    explicit SessionViewModel(QObject* parent = nullptr);

    QString      connectionStatus() const;
    bool         isConnected()      const;
    bool         isRunning()        const;
    QVariantList responseModel()    const;
    QStringList  rawLog()           const;

    // Called from QML — implementations added by Core Engineer
    Q_INVOKABLE void connect(const QVariantMap& params);
    Q_INVOKABLE void disconnect();
    Q_INVOKABLE void sendRequest(const QVariantMap& params);
    Q_INVOKABLE void clearLog();

signals:
    void connectionChanged();
    void sessionStateChanged();
    void dataChanged();
    void errorOccurred(const QString& message);
};

} // namespace rtu::ui
