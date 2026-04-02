import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import rtu.ui

// ─────────────────────────────────────────────────────────────
// AppShell
//
// Main application layout after login.
// Owns sidebar + topbar + content StackView.
// All navigation state lives here.
// ─────────────────────────────────────────────────────────────
Item {
    id: root

    // ── Page route table ──────────────────────────────────────
    readonly property var _routes: ({
        "dashboard":  "pages/DashboardPage.qml",
        "modbus_rtu": "pages/ModbusPage.qml",
        "modbus_tcp": "pages/ModbusPage.qml",
        "dnp3":       "pages/IEC104Page.qml",   // placeholder, replace in Core phase
        "iec101":     "pages/IEC104Page.qml",
        "iec104":     "pages/IEC104Page.qml",
        "serial":     "pages/SerialPage.qml",
        "analyzer":   "pages/AnalyzerPage.qml",
        "simulator":  "pages/SimulatorPage.qml"
    })

    readonly property var _pageTitles: ({
        "dashboard":  "Dashboard",
        "modbus_rtu": "Modbus RTU",
        "modbus_tcp": "Modbus TCP",
        "dnp3":       "DNP3",
        "iec101":     "IEC-101",
        "iec104":     "IEC-104",
        "serial":     "Serial Monitor",
        "analyzer":   "Analyzer",
        "simulator":  "Simulator"
    })

    property string currentPageId: "dashboard"

    function navigateTo(pageId) {
        if (pageId === currentPageId) return
        currentPageId = pageId
        const route = _routes[pageId] || "pages/DashboardPage.qml"
        // Pass pageId only to pages that share a component (e.g. ModbusPage for RTU/TCP)
        const props = (pageId === "modbus_tcp") ? { pageId: pageId } : {}
        contentStack.push(route, props, StackView.PushTransition)
    }

    // ── Layout ────────────────────────────────────────────────
    RowLayout {
        anchors.fill: parent
        spacing:      0

        // Sidebar
        Sidebar {
            id: sidebar
            Layout.preferredWidth: implicitWidth
            Layout.fillHeight:     true
            currentPageId: root.currentPageId
            isPro:         AppVM.isPro

            onNavigateTo: (pageId) => root.navigateTo(pageId)
        }

        // Main column (topbar + content)
        ColumnLayout {
            Layout.fillWidth:  true
            Layout.fillHeight: true
            spacing: 0

            // TopBar
            TopBar {
                Layout.fillWidth:       true
                Layout.preferredHeight: AppStyle.topBarHeight
                pageTitle: root._pageTitles[root.currentPageId] ?? "Dashboard"
                userId:    AppVM.userId
                tierName:  AppVM.tierName
                isPro:     AppVM.isPro

                onThemeChangeRequested: (name) => ThemeManager.setTheme(name)
                onLogoutRequested:      ()     => AppVM.logout()
            }

            // Content StackView
            StackView {
                id: contentStack
                Layout.fillWidth:  true
                Layout.fillHeight: true

                initialItem: "pages/DashboardPage.qml"

                // ── Page push transition ──────────────────
                pushEnter: Transition {
                    ParallelAnimation {
                        NumberAnimation {
                            property: "opacity"
                            from: 0; to: 1
                            duration: AppStyle.animNormal
                            easing.type: Easing.OutCubic
                        }
                        NumberAnimation {
                            property: "x"
                            from: 24; to: 0
                            duration: AppStyle.animNormal
                            easing.type: Easing.OutCubic
                        }
                    }
                }

                pushExit: Transition {
                    NumberAnimation {
                        property: "opacity"
                        from: 1; to: 0
                        duration: AppStyle.animFast
                    }
                }

                popEnter: Transition {
                    ParallelAnimation {
                        NumberAnimation {
                            property: "opacity"
                            from: 0; to: 1
                            duration: AppStyle.animNormal
                            easing.type: Easing.OutCubic
                        }
                        NumberAnimation {
                            property: "x"
                            from: -24; to: 0
                            duration: AppStyle.animNormal
                            easing.type: Easing.OutCubic
                        }
                    }
                }

                popExit: Transition {
                    NumberAnimation {
                        property: "opacity"
                        from: 1; to: 0
                        duration: AppStyle.animFast
                    }
                }
            }
        }
    }
}
