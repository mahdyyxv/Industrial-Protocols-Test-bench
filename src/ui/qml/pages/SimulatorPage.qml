import QtQuick
import QtQuick.Layouts
import rtu.ui

// ─────────────────────────────────────────────────────────────
// SimulatorPage  (Pro)
//
// Binds to SimulatorVM (SimulatorController context property).
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

            // ── Config panel ──────────────────────────────────
            ColumnLayout {
                Layout.preferredWidth: 280
                Layout.fillHeight:     true
                spacing: AppStyle.spaceMd

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight:   cfgCol.implicitHeight + AppStyle.spaceLg * 2
                    radius: AppStyle.radiusLg; color: ThemeManager.surface
                    border { color: ThemeManager.border; width: 1 }

                    ColumnLayout {
                        id: cfgCol
                        anchors { fill: parent; margins: AppStyle.spaceMd + AppStyle.spaceXs }
                        spacing: AppStyle.spaceMd

                        Text { text: "Simulator Config"; color: ThemeManager.text; font { pixelSize: AppStyle.fontMd; family: AppStyle.fontFamily; weight: 600 } }

                        // Add register
                        ColumnLayout { Layout.fillWidth: true; spacing: AppStyle.spaceXs
                            Text { text: "Add Register (addr = value)"; color: ThemeManager.textSecondary; font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily } }
                            RowLayout {
                                spacing: AppStyle.spaceXs
                                SimField { id: regAddrF; placeholder: "0"; Layout.preferredWidth: 80 }
                                Text { text: "="; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontBase }
                                SimField { id: regValF;  placeholder: "0"; Layout.preferredWidth: 80 }
                                SimBtn { label: "+"; isPrimary: true
                                    onClicked: {
                                        const a = parseInt(regAddrF.text); const v = parseInt(regValF.text)
                                        if (!isNaN(a) && !isNaN(v)) SimulatorVM.setRegister(a, v)
                                    }
                                }
                            }
                        }

                        // Random mode
                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "Random Values"; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily; Layout.fillWidth: true }
                            Rectangle {
                                width: 44; height: 24; radius: 12
                                color: SimulatorVM.randomEnabled ? ThemeManager.accent : ThemeManager.border
                                Behavior on color { ColorAnimation { duration: AppStyle.animFast } }
                                Rectangle {
                                    width: 18; height: 18; radius: 9
                                    anchors.verticalCenter: parent.verticalCenter
                                    x: SimulatorVM.randomEnabled ? parent.width - width - 3 : 3
                                    Behavior on x { NumberAnimation { duration: AppStyle.animFast } }
                                    color: "white"
                                }
                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: SimulatorVM.enableRandom(!SimulatorVM.randomEnabled) }
                            }
                        }

                        // Global range
                        ColumnLayout { Layout.fillWidth: true; spacing: AppStyle.spaceXs
                            Text { text: "Global Range (min – max)"; color: ThemeManager.textSecondary; font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily } }
                            RowLayout {
                                spacing: AppStyle.spaceXs
                                SimField { id: minF; placeholder: "0.0";   Layout.fillWidth: true }
                                Text { text: "–"; color: ThemeManager.textSecondary }
                                SimField { id: maxF; placeholder: "100.0"; Layout.fillWidth: true }
                                SimBtn { label: "Set"; isPrimary: false
                                    onClicked: {
                                        const mn = parseFloat(minF.text); const mx = parseFloat(maxF.text)
                                        if (!isNaN(mn) && !isNaN(mx)) SimulatorVM.setGlobalRange(mn, mx)
                                    }
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }

                        SimBtn {
                            Layout.fillWidth: true
                            label:     SimulatorVM.running ? (Icons.stop + "  Stop") : (Icons.play + "  Start Simulator")
                            isPrimary: true
                            onClicked: SimulatorVM.running ? SimulatorVM.stop() : SimulatorVM.start()
                        }

                        SimBtn {
                            Layout.fillWidth: true; label: Icons.clear + "  Reset"; isPrimary: false
                            onClicked: SimulatorVM.reset()
                        }
                    }
                }
            }

            // ── Register map editor ───────────────────────────
            Rectangle {
                Layout.fillWidth:  true
                Layout.fillHeight: true
                radius: AppStyle.radiusLg; color: ThemeManager.surface
                border { color: ThemeManager.border; width: 1 }

                ColumnLayout {
                    anchors { fill: parent; margins: AppStyle.spaceMd + AppStyle.spaceXs }
                    spacing: AppStyle.spaceMd

                    RowLayout {
                        Text { text: "Register Map"; color: ThemeManager.text; font.pixelSize: AppStyle.fontMd; font.family: AppStyle.fontFamily; font.weight: 600; Layout.fillWidth: true }
                        Text { text: SimulatorVM.registerMap.length + " entries"; color: ThemeManager.textSecondary; font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily } }
                    }

                    // Header
                    Rectangle {
                        Layout.fillWidth: true; height: 32; radius: AppStyle.radiusSm; color: ThemeManager.background
                        RowLayout {
                            anchors { fill: parent; leftMargin: AppStyle.spaceMd; rightMargin: AppStyle.spaceMd }
                            Repeater {
                                model: ["Address", "Value (Dec)", "Value (Hex)", "Description"]
                                delegate: Text { text: modelData; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily; font.weight: 500; Layout.fillWidth: true }
                            }
                        }
                    }

                    // Rows
                    ListView {
                        Layout.fillWidth:  true
                        Layout.fillHeight: true
                        model: SimulatorVM.registerMap
                        clip:  true

                        delegate: Rectangle {
                            required property var modelData
                            width: ListView.view.width; height: 28
                            color: index % 2 === 0 ? "transparent" : Qt.rgba(ThemeManager.text.r, ThemeManager.text.g, ThemeManager.text.b, 0.03)

                            RowLayout {
                                anchors { fill: parent; leftMargin: AppStyle.spaceMd; rightMargin: AppStyle.spaceMd }
                                Text { text: modelData.address     ?? ""; color: ThemeManager.accent; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontMono; Layout.fillWidth: true }
                                Text { text: modelData.value       ?? ""; color: ThemeManager.text;   font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontMono; Layout.fillWidth: true }
                                Text { text: modelData.hex         ?? ""; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontMono; Layout.fillWidth: true }
                                Text { text: modelData.description ?? ""; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily; Layout.fillWidth: true; elide: Text.ElideRight }
                            }
                        }

                        Text {
                            visible: parent.count === 0
                            anchors.centerIn: parent
                            text: "No registers configured.\nEnter address = value and click '+' to add."
                            color: ThemeManager.textDisabled; horizontalAlignment: Text.AlignHCenter
                            font { pixelSize: AppStyle.fontBase; family: AppStyle.fontFamily }
                        }
                    }
                }
            }
        }
    }

    // ── Inline components ─────────────────────────────────────
    component SimField: Rectangle {
        property alias text:        _sf.text
        property string placeholder: ""
        height: AppStyle.inputHeight; radius: AppStyle.radiusMd; color: ThemeManager.background
        border { color: _sf.activeFocus ? ThemeManager.borderFocus : ThemeManager.border; width: 1 }
        TextInput {
            id: _sf
            anchors { fill: parent; leftMargin: AppStyle.spaceSm + AppStyle.spaceXs; rightMargin: AppStyle.spaceSm }
            color: ThemeManager.text; font.pixelSize: AppStyle.fontBase; font.family: AppStyle.fontFamily; clip: true
            Text { visible: !parent.text && !parent.activeFocus; text: parent.parent.placeholder; color: ThemeManager.textDisabled; font: parent.font; anchors { left: parent.left; verticalCenter: parent.verticalCenter } }
        }
    }

    component SimBtn: Rectangle {
        property string label:     ""
        property bool   isPrimary: true
        signal clicked()
        implicitWidth: _sbt.implicitWidth + AppStyle.spaceLg * 2; implicitHeight: AppStyle.buttonHeightSm
        radius: AppStyle.radiusMd
        color: isPrimary ? (_sbh.containsMouse ? ThemeManager.accentHover : ThemeManager.accent) : (_sbh.containsMouse ? Qt.rgba(ThemeManager.text.r, ThemeManager.text.g, ThemeManager.text.b, 0.08) : "transparent")
        border { color: isPrimary ? "transparent" : ThemeManager.border; width: 1 }
        Behavior on color { ColorAnimation { duration: AppStyle.animFast } }
        Text { id: _sbt; anchors.centerIn: parent; text: label; color: isPrimary ? "#FFF" : ThemeManager.textSecondary; font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily } }
        MouseArea { id: _sbh; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: parent.clicked() }
    }
}
