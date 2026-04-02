import QtQuick
import QtQuick.Controls
import rtu.ui

// ─────────────────────────────────────────────────────────────
// main.qml
//
// Application root. Routes between LoginPage and AppShell
// based on AppVM.isLoggedIn. Applies theme background.
// ─────────────────────────────────────────────────────────────
ApplicationWindow {
    id: root
    visible:  true
    width:    1280
    height:   800
    minimumWidth:  900
    minimumHeight: 600
    title:    "RTU Tester"
    color:    ThemeManager.background

    Behavior on color { ColorAnimation { duration: AppStyle.animNormal } }

    // ── Login ↔ Shell router ──────────────────────────────────
    Loader {
        id: pageLoader
        anchors.fill: parent
        source: AppVM.isLoggedIn ? "AppShell.qml" : "LoginPage.qml"

        // Fade transition between login and shell
        onSourceChanged: fadeIn.restart()
    }

    NumberAnimation {
        id: fadeIn
        target:   pageLoader.item
        property: "opacity"
        from: 0; to: 1
        duration: AppStyle.animNormal
        easing.type: AppStyle.animEasing
    }

    // ── Keyboard shortcuts ────────────────────────────────────
    Shortcut {
        sequence:  "Ctrl+1"
        onActivated: if (AppVM.isLoggedIn && pageLoader.item)
                         pageLoader.item.navigateTo("dashboard")
    }
    Shortcut {
        sequence:  "Ctrl+2"
        onActivated: if (AppVM.isLoggedIn && pageLoader.item)
                         pageLoader.item.navigateTo("modbus_rtu")
    }
}
