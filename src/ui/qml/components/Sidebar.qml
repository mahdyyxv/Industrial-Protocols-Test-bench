import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import rtu.ui

// ─────────────────────────────────────────────────────────────
// Sidebar
//
// Collapsible vertical navigation panel.
// Emits navigateTo(pageId) — no logic inside.
// ─────────────────────────────────────────────────────────────
Rectangle {
    id: root

    // ── Public API ────────────────────────────────────────────
    property string currentPageId: "dashboard"
    property bool   isPro:         false

    signal navigateTo(string pageId)

    // ── Collapse state ────────────────────────────────────────
    property bool collapsed: false

    implicitWidth:  collapsed
                        ? AppStyle.sidebarWidthCollapsed
                        : AppStyle.sidebarWidthFull
    implicitHeight: parent ? parent.height : 600

    Behavior on implicitWidth {
        NumberAnimation {
            duration: AppStyle.animNormal
            easing.type: Easing.OutCubic
        }
    }

    color: ThemeManager.sidebarBg

    // Right border
    Rectangle {
        anchors { right: parent.right; top: parent.top; bottom: parent.bottom }
        width: 1
        color: ThemeManager.border
    }

    // ── Navigation model ──────────────────────────────────────
    readonly property var _navModel: [
        // ── Dashboard (no group) ─────────────────────────
        { id: "dashboard",  label: "Dashboard",     icon: Icons.dashboard, pro: false, group: "" },

        // ── Protocols group ──────────────────────────────
        { id: "modbus_rtu", label: "Modbus RTU",    icon: Icons.modbus,   pro: false, group: "Protocols" },
        { id: "modbus_tcp", label: "Modbus TCP",    icon: Icons.modbus,   pro: false, group: "" },
        { id: "dnp3",       label: "DNP3",          icon: Icons.dnp3,     pro: true,  group: "" },
        { id: "iec101",     label: "IEC-101",       icon: Icons.iec101,   pro: true,  group: "" },
        { id: "iec104",     label: "IEC-104",       icon: Icons.iec104,   pro: true,  group: "" },

        // ── Tools group ──────────────────────────────────
        { id: "serial",     label: "Serial Monitor",icon: Icons.serial,   pro: false, group: "Tools" },
        { id: "analyzer",   label: "Analyzer",      icon: Icons.analyzer, pro: true,  group: "" },
        { id: "simulator",  label: "Simulator",     icon: Icons.simulator,pro: true,  group: "" },
    ]

    // ── Content ───────────────────────────────────────────────
    ColumnLayout {
        anchors.fill:    parent
        spacing:         0

        // ── App logo / title ──────────────────────────────
        Item {
            Layout.fillWidth:   true
            Layout.preferredHeight: AppStyle.topBarHeight

            Row {
                anchors {
                    left:           parent.left
                    leftMargin:     AppStyle.spaceMd
                    verticalCenter: parent.verticalCenter
                }
                spacing: AppStyle.spaceSm

                // Logo placeholder rectangle
                Rectangle {
                    width:  28; height: 28
                    radius: AppStyle.radiusSm
                    color:  ThemeManager.accent

                    Text {
                        anchors.centerIn: parent
                        text:  "R"
                        color: "#FFFFFF"
                        font { pixelSize: AppStyle.fontLg; weight: Font.Bold }
                    }
                }

                Text {
                    text:    "RTU Tester"
                    color:   ThemeManager.text
                    visible: !root.collapsed
                    font {
                        pixelSize: AppStyle.fontLg
                        family:    AppStyle.fontFamily
                        weight:    Font.SemiBold
                    }
                    opacity: root.collapsed ? 0 : 1
                    Behavior on opacity { NumberAnimation { duration: AppStyle.animFast } }
                }
            }
        }

        // ── Nav items ─────────────────────────────────────
        ScrollView {
            Layout.fillWidth:  true
            Layout.fillHeight: true
            clip: true
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            Column {
                id: navColumn
                width:   root.width
                padding: 0

                Repeater {
                    model: root._navModel

                    delegate: Column {
                        width: parent.width

                        // Group label (only when group name is non-empty)
                        SidebarGroupLabel {
                            width:     parent.width
                            text:      modelData.group
                            collapsed: root.collapsed
                            visible:   modelData.group !== ""
                        }

                        NavItem {
                            width:       parent.width
                            label:       modelData.label
                            icon:        modelData.icon
                            requiresPro: modelData.pro
                            isPro:       root.isPro
                            collapsed:   root.collapsed
                            active:      root.currentPageId === modelData.id

                            onClicked: root.navigateTo(modelData.id)
                        }
                    }
                }
            }
        }

        // ── Bottom: collapse toggle ───────────────────────
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: AppStyle.buttonHeight + AppStyle.spaceMd

            Rectangle {
                anchors {
                    left: parent.left; right: parent.right
                    top: parent.top
                }
                height: 1
                color:  ThemeManager.border
            }

            Item {
                id: collapseBtn
                anchors {
                    right:          parent.right
                    rightMargin:    AppStyle.spaceSm
                    verticalCenter: parent.verticalCenter
                }
                width:  36; height: 36

                property bool hovered: false

                Rectangle {
                    anchors.fill: parent
                    radius:  AppStyle.radiusMd
                    color:   parent.hovered
                                ? Qt.rgba(ThemeManager.text.r,
                                          ThemeManager.text.g,
                                          ThemeManager.text.b, 0.08)
                                : "transparent"
                    Behavior on color { ColorAnimation { duration: AppStyle.animFast } }
                }

                Text {
                    anchors.centerIn: parent
                    text:  root.collapsed ? Icons.expand : Icons.collapse
                    color: ThemeManager.textSecondary
                    font.pixelSize: AppStyle.fontBase
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape:  Qt.PointingHandCursor
                    onEntered:    collapseBtn.hovered = true
                    onExited:     collapseBtn.hovered = false
                    onClicked:    root.collapsed = !root.collapsed
                }
            }
        }
    }
}
