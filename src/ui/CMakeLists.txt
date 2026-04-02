qt_add_library(rtu_ui STATIC
    viewmodels/AppViewModel.h
    viewmodels/AppViewModel.cpp
    viewmodels/SessionViewModel.h
    viewmodels/SessionViewModel.cpp
    viewmodels/NavigationViewModel.h
    viewmodels/NavigationViewModel.cpp
)

# ── QML Singletons ────────────────────────────────────────────
set_source_files_properties(
    qml/theme/ThemeManager.qml
    qml/style/AppStyle.qml
    qml/style/Icons.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

# ── QML Module ────────────────────────────────────────────────
qt_add_qml_module(rtu_ui
    URI rtu.ui
    VERSION 1.0

    QML_FILES
        # Singletons
        qml/theme/ThemeManager.qml
        qml/style/AppStyle.qml
        qml/style/Icons.qml

        # Shell
        qml/main.qml
        qml/AppShell.qml
        qml/LoginPage.qml

        # Components
        qml/components/Sidebar.qml
        qml/components/TopBar.qml
        qml/components/NavItem.qml
        qml/components/SidebarGroupLabel.qml
        qml/components/ProLockOverlay.qml
        qml/components/StatusBadge.qml

        # Pages
        qml/pages/DashboardPage.qml
        qml/pages/ModbusPage.qml
        qml/pages/IEC104Page.qml
        qml/pages/SerialPage.qml
        qml/pages/AnalyzerPage.qml
        qml/pages/SimulatorPage.qml
)

target_include_directories(rtu_ui PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(rtu_ui PUBLIC
    Qt6::Core
    Qt6::Quick
    Qt6::QuickControls2
    rtu_services
)
