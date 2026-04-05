import QtQuick
import QtQuick.Layouts
import rtu.ui

// ─────────────────────────────────────────────────────────────
// AnalyzerPage  (Pro)
//
// Binds to AnalyzerVM (AnalyzerController context property).
// ─────────────────────────────────────────────────────────────
Item {
    id: root
    property int selectedIndex: -1

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
                    text: "Protocol Analyzer"; color: ThemeManager.text
                    font { pixelSize: AppStyle.fontXl; family: AppStyle.fontFamily; weight: 600 }
                    Layout.fillWidth: true
                }

                // Stats badges
                Text {
                    text: AnalyzerVM.statistics.total + " packets"
                    color: ThemeManager.textSecondary
                    font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily }
                }

                // Capture button
                Rectangle {
                    width: 110; height: AppStyle.buttonHeightSm
                    radius: AppStyle.radiusMd
                    color: AnalyzerVM.capturing ? "#CC2222" : ThemeManager.accent
                    Behavior on color { ColorAnimation { duration: AppStyle.animFast } }
                    Text {
                        anchors.centerIn: parent
                        text: AnalyzerVM.capturing ? Icons.stop + " Stop" : Icons.play + " Capture"
                        color: "#FFF"; font.pixelSize: AppStyle.fontSm
                    }
                    MouseArea {
                        anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                        onClicked: AnalyzerVM.capturing ? AnalyzerVM.stopCapture() : AnalyzerVM.startCapture()
                    }
                }

                Rectangle {
                    width: 80; height: AppStyle.buttonHeightSm
                    radius: AppStyle.radiusMd; color: "transparent"
                    border { color: ThemeManager.border; width: 1 }
                    Text { anchors.centerIn: parent; text: Icons.clear + " Clear"; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { AnalyzerVM.clearPackets(); root.selectedIndex = -1 } }
                }
            }

            // Frame list
            Rectangle {
                Layout.fillWidth:  true
                Layout.fillHeight: true
                radius: AppStyle.radiusLg
                color:  ThemeManager.surface
                border { color: ThemeManager.border; width: 1 }

                ColumnLayout {
                    anchors { fill: parent; margins: AppStyle.spaceMd }
                    spacing: 0

                    // Header
                    Rectangle {
                        Layout.fillWidth: true; height: 32
                        color: ThemeManager.background; radius: AppStyle.radiusSm
                        RowLayout {
                            anchors { fill: parent; leftMargin: AppStyle.spaceMd; rightMargin: AppStyle.spaceMd }
                            Repeater {
                                model: ["#", "Time", "Dir", "Protocol", "Length", "Description", "Valid"]
                                delegate: Text {
                                    text: modelData; color: ThemeManager.textSecondary
                                    font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily; weight: 500 }
                                    Layout.preferredWidth: index === 5 ? -1 : 80
                                    Layout.fillWidth: index === 5
                                }
                            }
                        }
                    }

                    // Packet rows
                    ListView {
                        id: packetList
                        Layout.fillWidth:  true
                        Layout.fillHeight: true
                        model: AnalyzerVM.packets
                        clip:  true

                        delegate: Rectangle {
                            required property var    modelData
                            required property int    index
                            width:  ListView.view.width
                            height: 28
                            color: root.selectedIndex === index
                                   ? Qt.rgba(ThemeManager.accent.r, ThemeManager.accent.g, ThemeManager.accent.b, 0.2)
                                   : index % 2 === 0 ? "transparent"
                                   : Qt.rgba(ThemeManager.text.r, ThemeManager.text.g, ThemeManager.text.b, 0.03)

                            RowLayout {
                                anchors { fill: parent; leftMargin: AppStyle.spaceMd; rightMargin: AppStyle.spaceMd }
                                Text { text: modelData.id        ?? ""; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontMono; Layout.preferredWidth: 80 }
                                Text { text: modelData.timestamp ?? ""; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontMono; Layout.preferredWidth: 80 }
                                Text { text: modelData.direction ?? ""; color: modelData.direction === "TX" ? ThemeManager.accent : "#55AA55"; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontMono; Layout.preferredWidth: 80 }
                                Text { text: modelData.protocol  ?? ""; color: ThemeManager.text; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily; Layout.preferredWidth: 80 }
                                Text { text: modelData.length    ?? ""; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontMono; Layout.preferredWidth: 80 }
                                Text { text: modelData.summary   ?? ""; color: ThemeManager.text; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily; Layout.fillWidth: true; elide: Text.ElideRight }
                                Text { text: modelData.valid ? "✓" : "✗"; color: modelData.valid ? "#55AA55" : "#FF5555"; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily; Layout.preferredWidth: 80 }
                            }

                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: root.selectedIndex = index }
                        }

                        Text {
                            visible: parent.count === 0
                            anchors.centerIn: parent
                            text: AnalyzerVM.capturing ? "Capturing…\nFrames will appear as they arrive." : "No frames captured yet.\nStart capture to see protocol frames."
                            color: ThemeManager.textDisabled
                            horizontalAlignment: Text.AlignHCenter
                            font { pixelSize: AppStyle.fontBase; family: AppStyle.fontFamily }
                        }
                    }
                }
            }

            // Detail panel
            Rectangle {
                Layout.fillWidth:   true
                Layout.preferredHeight: 160
                radius: AppStyle.radiusLg
                color:  ThemeManager.surface
                border { color: ThemeManager.border; width: 1 }

                ColumnLayout {
                    anchors { fill: parent; margins: AppStyle.spaceMd }
                    spacing: AppStyle.spaceSm

                    Text { text: "Frame Detail"; color: ThemeManager.text; font { pixelSize: AppStyle.fontMd; family: AppStyle.fontFamily; weight: 600 } }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: AppStyle.spaceLg

                        // Selected packet info
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: AppStyle.spaceXs
                            visible: root.selectedIndex >= 0 && root.selectedIndex < AnalyzerVM.packets.length

                            property var pkt: root.selectedIndex >= 0 && root.selectedIndex < AnalyzerVM.packets.length
                                              ? AnalyzerVM.packets[root.selectedIndex] : null

                            Text { text: parent.pkt ? parent.pkt.summary : ""; color: ThemeManager.text; font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily } }
                            Text { text: parent.pkt ? "Raw: " + parent.pkt.hex : ""; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontMono; wrapMode: Text.WrapAnywhere }
                        }

                        Text {
                            visible: root.selectedIndex < 0
                            text:    "Select a frame above to inspect its fields."
                            color:   ThemeManager.textDisabled
                            font { pixelSize: AppStyle.fontBase; family: AppStyle.fontFamily }
                        }
                    }
                }
            }
        }
    }
}
