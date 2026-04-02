import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import rtu.ui

// ─────────────────────────────────────────────────────────────
// DashboardPage
//
// Overview: stat cards, quick actions, recent activity.
// All data binds to SessionVM (stub — filled by Core Engineer).
// ─────────────────────────────────────────────────────────────
ScrollView {
    id: root
    contentWidth: availableWidth
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

    Rectangle {
        anchors.fill: parent
        color: ThemeManager.background
    }

    ColumnLayout {
        x:       AppStyle.contentPadding
        width:   root.availableWidth - AppStyle.contentPadding * 2
        spacing: AppStyle.spaceLg

        // ── Stat cards row ────────────────────────────────
        Row {
            Layout.fillWidth: true
            spacing: AppStyle.spaceMd

            Repeater {
                model: [
                    { label: "Active Sessions", value: "0",     icon: Icons.play,       color: ThemeManager.accent   },
                    { label: "Protocols Ready", value: "2",     icon: Icons.protocols,  color: ThemeManager.success  },
                    { label: "Last Session",    value: "—",     icon: Icons.log,        color: ThemeManager.warning  },
                    { label: "Frames Captured", value: "0",     icon: Icons.analyzer,   color: ThemeManager.textSecondary },
                ]

                delegate: Rectangle {
                    width:  (root.availableWidth - AppStyle.contentPadding * 2
                                                 - AppStyle.spaceMd * 3) / 4
                    height: 100
                    radius: AppStyle.radiusLg
                    color:  ThemeManager.surface
                    border { color: ThemeManager.border; width: 1 }

                    ColumnLayout {
                        anchors { fill: parent; margins: AppStyle.spaceMd }
                        spacing: AppStyle.spaceXs

                        Row {
                            spacing: AppStyle.spaceSm
                            Text {
                                text:  modelData.icon
                                color: modelData.color
                                font.pixelSize: AppStyle.iconSizeLg
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Text {
                                text:  modelData.label
                                color: ThemeManager.textSecondary
                                font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily }
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }

                        Text {
                            text:  modelData.value
                            color: ThemeManager.text
                            font {
                                pixelSize: AppStyle.font3xl
                                family:    AppStyle.fontFamily
                                weight:    Font.Bold
                            }
                        }
                    }
                }
            }
        }

        // ── Quick actions ─────────────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            spacing: AppStyle.spaceMd

            Text {
                text:  "Quick Start"
                color: ThemeManager.text
                font {
                    pixelSize: AppStyle.fontLg
                    family:    AppStyle.fontFamily
                    weight:    Font.SemiBold
                }
            }

            Row {
                spacing: AppStyle.spaceMd

                Repeater {
                    model: [
                        { label: "Modbus RTU Test",  icon: Icons.modbus,    page: "modbus_rtu", pro: false },
                        { label: "Modbus TCP Test",  icon: Icons.modbus,    page: "modbus_tcp", pro: false },
                        { label: "Serial Monitor",   icon: Icons.serial,    page: "serial",     pro: false },
                        { label: "IEC-104 Test",     icon: Icons.iec104,    page: "iec104",     pro: true  },
                        { label: "Protocol Analyzer",icon: Icons.analyzer,  page: "analyzer",   pro: true  },
                    ]

                    delegate: Item {
                        width:  160
                        height: 88
                        property bool hovered: false
                        readonly property bool locked: modelData.pro && !AppVM.isPro

                        Rectangle {
                            anchors.fill: parent
                            radius: AppStyle.radiusLg
                            color: {
                                if (parent.hovered) return ThemeManager.surfaceAlt
                                return ThemeManager.surface
                            }
                            border { color: ThemeManager.border; width: 1 }
                            Behavior on color { ColorAnimation { duration: AppStyle.animFast } }

                            ColumnLayout {
                                anchors.centerIn: parent
                                spacing: AppStyle.spaceSm

                                Text {
                                    Layout.alignment: Qt.AlignHCenter
                                    text:  modelData.icon
                                    color: parent.parent.parent.locked
                                               ? ThemeManager.textDisabled
                                               : ThemeManager.accent
                                    font.pixelSize: AppStyle.iconSizeLg + 4
                                }

                                Text {
                                    Layout.alignment: Qt.AlignHCenter
                                    text:  modelData.label + (parent.parent.parent.locked ? "\n🔒" : "")
                                    color: parent.parent.parent.locked
                                               ? ThemeManager.textDisabled
                                               : ThemeManager.text
                                    font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily }
                                    horizontalAlignment: Text.AlignHCenter
                                }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: parent.locked ? Qt.ForbiddenCursor : Qt.PointingHandCursor
                            onEntered: parent.hovered = true
                            onExited:  parent.hovered = false
                            onClicked: {
                                if (!parent.locked)
                                    pageLoader.item.navigateTo(modelData.page)
                            }
                        }
                    }
                }
            }
        }

        // ── Recent sessions (placeholder) ─────────────────
        ColumnLayout {
            Layout.fillWidth: true
            spacing: AppStyle.spaceMd

            Text {
                text:  "Recent Sessions"
                color: ThemeManager.text
                font {
                    pixelSize: AppStyle.fontLg
                    family:    AppStyle.fontFamily
                    weight:    Font.SemiBold
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 200
                radius: AppStyle.radiusLg
                color:  ThemeManager.surface
                border { color: ThemeManager.border; width: 1 }

                Text {
                    anchors.centerIn: parent
                    text:  "No sessions yet.\nStart a test to see results here."
                    color: ThemeManager.textSecondary
                    font { pixelSize: AppStyle.fontBase; family: AppStyle.fontFamily }
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        // Bottom padding
        Item { Layout.preferredHeight: AppStyle.spaceLg }
    }
}
