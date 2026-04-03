import QtQuick
import QtQuick.Layouts
import rtu.ui

// ─────────────────────────────────────────────────────────────
// AnalyzerPage  (Pro)
//
// Protocol frame capture and analysis view.
// ─────────────────────────────────────────────────────────────
Item {
    Rectangle { anchors.fill: parent; color: ThemeManager.background }

    ProLockOverlay {
        requiresPro: true
        isPro:       AppVM.isPro
        featureName: "Protocol Analyzer"
        onUpgradeRequested: Qt.openUrlExternally("https://rtu-tester.app/upgrade")

        ColumnLayout {
            anchors { fill: parent; margins: AppStyle.contentPadding }
            spacing: AppStyle.spaceMd

            // Toolbar
            RowLayout {
                Layout.fillWidth: true
                spacing: AppStyle.spaceSm

                Text {
                    text:  "Protocol Analyzer"
                    color: ThemeManager.text
                    font { pixelSize: AppStyle.fontXl; family: AppStyle.fontFamily; weight: 600 }
                    Layout.fillWidth: true
                }

                Rectangle {
                    width: 100; height: AppStyle.buttonHeightSm
                    radius: AppStyle.radiusMd; color: ThemeManager.accent
                    Text { anchors.centerIn: parent; text: Icons.play + " Capture"; color: "#FFF"; font.pixelSize: AppStyle.fontSm }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor }
                }
                Rectangle {
                    width: 80; height: AppStyle.buttonHeightSm
                    radius: AppStyle.radiusMd; color: "transparent"
                    border { color: ThemeManager.border; width: 1 }
                    Text { anchors.centerIn: parent; text: Icons.export_ + " Export"; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor }
                }
            }

            // Frame list (bind to AnalyzerVM.frames)
            Rectangle {
                Layout.fillWidth:  true
                Layout.fillHeight: true
                radius: AppStyle.radiusLg
                color:  ThemeManager.surface
                border { color: ThemeManager.border; width: 1 }

                ColumnLayout {
                    anchors { fill: parent; margins: AppStyle.spaceMd }
                    spacing: 0

                    // Header row
                    Rectangle {
                        Layout.fillWidth: true
                        height: 32
                        color:  ThemeManager.background
                        radius: AppStyle.radiusSm

                        RowLayout {
                            anchors { fill: parent; leftMargin: AppStyle.spaceMd; rightMargin: AppStyle.spaceMd }
                            Repeater {
                                model: ["#", "Time", "Dir", "Protocol", "Length", "Description", "Valid"]
                                delegate: Text {
                                    text:  modelData
                                    color: ThemeManager.textSecondary
                                    font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily; weight: 500 }
                                    Layout.preferredWidth: index === 5 ? -1 : 80
                                    Layout.fillWidth: index === 5
                                }
                            }
                        }
                    }

                    Text {
                        Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                        Layout.fillHeight: true
                        text:  "No frames captured yet.\nStart capture to see protocol frames."
                        color: ThemeManager.textDisabled
                        horizontalAlignment: Text.AlignHCenter
                        font { pixelSize: AppStyle.fontBase; family: AppStyle.fontFamily }
                    }
                }
            }

            // Detail panel
            Rectangle {
                Layout.fillWidth:   true
                Layout.preferredHeight: 140
                radius: AppStyle.radiusLg
                color:  ThemeManager.surface
                border { color: ThemeManager.border; width: 1 }

                ColumnLayout {
                    anchors { fill: parent; margins: AppStyle.spaceMd }
                    spacing: AppStyle.spaceSm
                    Text {
                        text:  "Frame Detail"
                        color: ThemeManager.text
                        font { pixelSize: AppStyle.fontMd; family: AppStyle.fontFamily; weight: 600 }
                    }
                    Text {
                        text:  "Select a frame above to inspect its fields."
                        color: ThemeManager.textDisabled
                        font { pixelSize: AppStyle.fontBase; family: AppStyle.fontFamily }
                    }
                }
            }
        }
    }
}
