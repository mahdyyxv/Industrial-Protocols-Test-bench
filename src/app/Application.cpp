#include "Application.h"
#include <QQmlContext>
#include <QUrl>
#include <QtQml/QQmlExtensionPlugin>
#include "utils/Logger.h"

#include "ui/viewmodels/AppViewModel.h"
#include "ui/viewmodels/ModbusController.h"
#include "ui/viewmodels/IECController.h"
#include "ui/viewmodels/SerialController.h"
#include "ui/viewmodels/AnalyzerController.h"
#include "ui/viewmodels/SimulatorController.h"
#include "services/ServiceLocator.h"
#include "services/dev/StubAuthService.h"
#include "services/dev/StubSubscriptionService.h"

// Statically import the rtu.ui QML plugin so the engine can find it
// when the module is linked as a static library (no separate .so).
Q_IMPORT_QML_PLUGIN(rtu_uiPlugin)

namespace rtu {

Application::Application(int& argc, char** argv)
    : m_app(argc, argv)
{
    QCoreApplication::setOrganizationName("RTUTester");
    QCoreApplication::setApplicationName("Project1RTU");
    QCoreApplication::setApplicationVersion(APP_VERSION);
}

Application::~Application() = default;

int Application::run()
{
    initServices();
    registerQmlTypes();

    // Required for static builds: tell the engine where embedded QML modules live.
    // QTP0001 NEW sets the resource prefix to :/qt/qml/
    m_engine.addImportPath(QStringLiteral("qrc:/qt/qml"));

    // Load root QML — URI rtu.ui maps to qrc:/qt/qml/rtu/ui/
    const QUrl rootUrl(QStringLiteral("qrc:/qt/qml/rtu/ui/qml/main.qml"));
    m_engine.load(rootUrl);

    if (m_engine.rootObjects().isEmpty()) {
        qCCritical(rtuApp) << "Failed to load root QML";
        return -1;
    }

    return m_app.exec();
}

void Application::initServices()
{
    // TODO (Integration phase): replace stubs with real backend services.
    // Stubs accept any non-empty email/password and return Pro tier.
    auto auth = std::make_shared<services::dev::StubAuthService>();
    auto sub  = std::make_shared<services::dev::StubSubscriptionService>();

    services::ServiceLocator::instance().registerAuth(auth);
    services::ServiceLocator::instance().registerSubscription(sub);

    qCInfo(rtuApp) << "Dev stub services registered (auth + subscription)";
}

void Application::registerQmlTypes()
{
    auto* ctx = m_engine.rootContext();

    // Auth / subscription
    auto* appVM = new ui::AppViewModel(&m_engine);
    ctx->setContextProperty(QStringLiteral("AppVM"), appVM);

    // Protocol controllers — one instance each, alive for app lifetime
    ctx->setContextProperty(QStringLiteral("ModbusVM"),    new ui::ModbusController(&m_engine));
    ctx->setContextProperty(QStringLiteral("IECVM"),       new ui::IECController(&m_engine));
    ctx->setContextProperty(QStringLiteral("SerialVM"),    new ui::SerialController(&m_engine));
    ctx->setContextProperty(QStringLiteral("AnalyzerVM"),  new ui::AnalyzerController(&m_engine));
    ctx->setContextProperty(QStringLiteral("SimulatorVM"), new ui::SimulatorController(&m_engine));

    qCInfo(rtuApp) << "All controllers registered with QML engine";
}

} // namespace rtu
