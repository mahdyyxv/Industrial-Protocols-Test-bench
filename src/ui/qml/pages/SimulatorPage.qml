import QtQuick
import QtQuick.Layouts
import rtu.ui

// ─────────────────────────────────────────────────────────────
// SimulatorPage  (Pro)
//
// RTU response simulator — configure simulated register maps
// and respond to master queries automatically.
// ─────────────────────────────────────────────────────────────
Item {
    Rectangle { anchors.fill: parent; color: ThemeManager.background }

    ProLockOverlay {
        requiresPro: true
        isPro:       AppVM.isPro
        featureName: "RTU Simulator"
        onUpgradeRequested: Qt.openUrlExternally("https://rtu-tester.app/upgrade")

        RowLayout {
            anchors { fill: parent; margins: AppStyle.contentPadding }
            spacing: AppStyle.spaceMd

            // Config panel
            Rectangle {
                Layout.preferredWidth: 280
                Layout.fillHeight:     true
                radius: AppStyle.radiusLg
                color:  ThemeManager.surface
                border { color: ThemeManager.border; width: 1 }

                ColumnLayout {
                    anchors { fill: parent; margins: AppStyle.spaceMd + AppStyle.spaceXs }
                    spacing: AppStyle.spaceMd

                    Text {
                        text:  "Simulator Config"
                        color: ThemeManager.text
                        font { pixelSize: AppStyle.fontMd; family: AppStyle.fontFamily; weight: 600 }
                    }

                    Repeater {
                        model: [
                            { label: "Protocol",   placeholder: "Modbus RTU" },
                            { label: "Device ID",  placeholder: "1"          },
                            { label: "Transport",  placeholder: "Serial"     },
                        ]
                        delegate: ColumnLayout {
                            Layout.fillWidth: true
                            spacing: AppStyle.spaceXs
                            Text {
                                text:  modelData.label
                                color: ThemeManager.textSecondary
                                font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily }
                            }
                            Rectangle {
                                Layout.fillWidth: true
                                height: AppStyle.inputHeight
                                radius: AppStyle.radiusMd
                                color:  ThemeManager.background
                                border { color: ThemeManager.border; width: 1 }
                                Text {
                                    anchors { left: parent.left; leftMargin: AppStyle.spaceSm + AppStyle.spaceXs; verticalCenter: parent.verticalCenter }
                                    text:  modelData.placeholder
                                    color: ThemeManager.textDisabled
                                    font { pixelSize: AppStyle.fontBase; family: AppStyle.fontFamily }
                                }
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }

                    Rectangle {
                        Layout.fillWidth: true
                        height: AppStyle.buttonHeight
                        radius: AppStyle.radiusMd
                        color:  ThemeManager.accent
                        Text {
                            anchors.centerIn: parent
                            text:  Icons.play + "  Start Simulator"
                            color: "#FFFFFF"
                            font { pixelSize: AppStyle.fontMd; family: AppStyle.fontFamily; weight: 500 }
                        }
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor }
                    }
                }
            }

            // Register map editor
            Rectangle {
                Layout.fillWidth:  true
                Layout.fillHeight: true
                radius: AppStyle.radiusLg
                color:  ThemeManager.surface
                border { color: ThemeManager.border; width: 1 }

                ColumnLayout {
                    anchors { fill: parent; margins: AppStyle.spaceMd + AppStyle.spaceXs }
                    spacing: AppStyle.spaceMd

                    RowLayout {
                        Text {
                            text:  "Register Map"
                            color: ThemeManager.text
                            font { pixelSize: AppStyle.fontMd; family: AppStyle.fontFamily; weight: 600 }
                            Layout.fillWidth: true
                        }
                        Rectangle {
                            width: 60; height: AppStyle.buttonHeightSm
                            radius: AppStyle.radiusMd; color: ThemeManager.accent
                            Text { anchors.centerIn: parent; text: Icons.plus + " Add"; color: "#FFF"; font.pixelSize: AppStyle.fontSm }
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor }
                        }
                    }

                    // Register table header
                    Rectangle {
                        Layout.fillWidth: true
                        height: 32; radius: AppStyle.radiusSm; color: ThemeManager.background
                        RowLayout {
                            anchors { fill: parent; leftMargin: AppStyle.spaceMd; rightMargin: AppStyle.spaceMd }
                            Repeater {
                                model: ["Address", "Type", "Value", "Description"]
                                delegate: Text {
                                    text:  modelData
                                    color: ThemeManager.textSecondary
                                    font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily; weight: 500 }
                                    Layout.fillWidth: true
                                }
                            }
                        }
                    }

                    Text {
                        Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                        Layout.fillHeight: true
                        text:  "No registers configured.\nClick '+ Add' to define simulated register values."
                        color: ThemeManager.textDisabled
                        horizontalAlignment: Text.AlignHCenter
                        font { pixelSize: AppStyle.fontBase; family: AppStyle.fontFamily }
                    }
                }
            }
        }
    }
}
