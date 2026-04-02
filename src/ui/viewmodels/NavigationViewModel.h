#pragma once

#include <QObject>
#include <QString>

namespace rtu::ui {

// ──────────────────────────────────────────────────────────────
// NavigationViewModel  (stub)
//
// Optional C++ bridge for navigation state if QML-only routing
// proves insufficient. Currently navigation is handled in
// AppShell.qml; promote logic here if deep linking or
// programmatic routing from C++ is needed.
// ──────────────────────────────────────────────────────────────
// Exposed via: engine.rootContext()->setContextProperty("NavVM", ...)
class NavigationViewModel : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString currentPageId READ currentPageId NOTIFY pageChanged)

public:
    explicit NavigationViewModel(QObject* parent = nullptr);

    QString currentPageId() const;

    Q_INVOKABLE void navigateTo(const QString& pageId);

signals:
    void pageChanged();
};

} // namespace rtu::ui
