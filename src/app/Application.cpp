#include "Application.h"
#include <QQmlContext>
#include <QUrl>
#include <QtQml/QQmlExtensionPlugin>
#include "utils/Logger.h"

// Stub ViewModels — concrete implementations added in Integration phase
#include "ui/viewmodels/AppViewModel.h"
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
    // ViewModels exposed as context properties so QML can use "AppVM.*"
    auto* appVM = new ui::AppViewModel(&m_engine);
    m_engine.rootContext()->setContextProperty(QStringLiteral("AppVM"), appVM);
}

} // namespace rtu
