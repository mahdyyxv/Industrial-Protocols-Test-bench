#pragma once

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <memory>

namespace rtu {

// ──────────────────────────────────────────────────────────────
// Application
//
// Bootstrap class. Owns the QML engine and initialises all
// services before the first QML component is loaded.
//
// Startup order:
//   1. Logger
//   2. AppConfig
//   3. ServiceLocator (auth + subscription concrete impls)
//   4. Register C++ types with QML engine
//   5. Load root QML
// ──────────────────────────────────────────────────────────────
class Application {
public:
    explicit Application(int& argc, char** argv);
    ~Application();

    int run();

private:
    void initServices();
    void registerQmlTypes();

    QGuiApplication        m_app;
    QQmlApplicationEngine  m_engine;
};

} // namespace rtu
