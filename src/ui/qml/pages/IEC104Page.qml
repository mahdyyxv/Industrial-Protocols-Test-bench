import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import rtu.ui

// ─────────────────────────────────────────────────────────────
// IEC104Page  (Pro)
//
// Binds to IECVM (IECController context property).
// Supports Client (connect to outstation) and Server (listen) roles.
// ─────────────────────────────────────────────────────────────
Item {
    property string pageId: "iec104"
    readonly property bool isServer: IECVM.role === "Server"

    readonly property var _titles: ({
        "iec104": "IEC 60870-5-104",
        "iec101": "IEC 60870-5-101",
        "dnp3":   "DNP3"
    })

    Rectangle { anchors.fill: parent; color: ThemeManager.background }

    ProLockOverlay {
        requiresPro: true
        isPro:       AppVM.isPro
        featureName: _titles[pageId] ?? "This protocol"
        onUpgradeRequested: Qt.openUrlExternally("https://rtu-tester.app/upgrade")

        RowLayout {
            anchors { fill: parent; margins: AppStyle.contentPadding }
            spacing: AppStyle.spaceMd

            // ── LEFT: Connection + Controls ───────────────────
            ColumnLayout {
                Layout.preferredWidth: 300
                Layout.fillHeight:     true
                spacing: AppStyle.spaceMd

                // Connection card
                Rectangle {
                    Layout.fillWidth:  true
                    implicitHeight:    connCol.implicitHeight + AppStyle.spaceLg * 2
                    radius: AppStyle.radiusLg
                    color:  ThemeManager.surface
                    border { color: ThemeManager.border; width: 1 }

                    ColumnLayout {
                        id: connCol
                        anchors { fill: parent; margins: AppStyle.spaceMd + AppStyle.spaceXs }
                        spacing: AppStyle.spaceMd

                        RowLayout {
                            Text { text: "Connection"; color: ThemeManager.text; font.pixelSize: AppStyle.fontMd; font.family: AppStyle.fontFamily; font.weight: 600; Layout.fillWidth: true }
                            StatusBadge { status: IECVM.connected ? "connected" : "disconnected" }
                        }

                        // ── Role selector ─────────────────────
                        ColumnLayout {
                            Layout.fillWidth: true; spacing: AppStyle.spaceXs
                            Text { text: "Role"; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily }
                            ComboBox {
                                id: iecRoleCombo
                                Layout.fillWidth: true
                                model: ["Client", "Server"]
                                currentIndex: IECVM.role === "Server" ? 1 : 0
                                enabled: !IECVM.connected
                                onActivated: IECVM.role = currentText
                                background: Rectangle {
                                    color: ThemeManager.background; radius: AppStyle.radiusMd
                                    border { color: ThemeManager.border; width: 1 }
                                }
                                contentItem: Text {
                                    leftPadding: AppStyle.spaceSm + AppStyle.spaceXs
                                    text: iecRoleCombo.displayText; color: ThemeManager.text
                                    font.pixelSize: AppStyle.fontBase; font.family: AppStyle.fontFamily
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }
                        }

                        // ── Client: host / port ───────────────
                        ColumnLayout {
                            visible: !isServer
                            Layout.fillWidth: true; spacing: AppStyle.spaceXs
                            Text { text: "Host"; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily }
                            IecField { placeholder: "192.168.1.100"; text: IECVM.host; onTextChanged: IECVM.host = text }
                            Text { text: "Port"; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily }
                            IecField { placeholder: "2404"; text: IECVM.port; onTextChanged: { const v = parseInt(text); if (!isNaN(v)) IECVM.port = v } }
                        }

                        // ── Server: listen port ───────────────
                        ColumnLayout {
                            visible: isServer
                            Layout.fillWidth: true; spacing: AppStyle.spaceXs
                            Text { text: "Listen Port"; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily }
                            IecField { placeholder: "2404"; text: IECVM.listenPort; onTextChanged: { const v = parseInt(text); if (!isNaN(v)) IECVM.listenPort = v } }
                            Text { text: "Clients: " + IECVM.clientCount; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily }
                        }

                        // ── Common Address ────────────────────
                        ColumnLayout {
                            Layout.fillWidth: true; spacing: AppStyle.spaceXs
                            Text { text: "Common Address"; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily }
                            IecField { placeholder: "1"; text: IECVM.commonAddress; onTextChanged: { const v = parseInt(text); if (!isNaN(v)) IECVM.commonAddress = v } }
                        }

                        Text { text: IECVM.stateText; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily }

                        IecButton {
                            label: IECVM.connected
                                   ? (isServer ? "Stop Server" : "Disconnect")
                                   : (isServer ? "Start Server" : "Connect")
                            isPrimary: true
                            Layout.fillWidth: true
                            onClicked: IECVM.connected ? IECVM.disconnectDevice() : IECVM.connectDevice()
                        }
                    }
                }

                // ── Client: session control card ──────────────
                Rectangle {
                    Layout.fillWidth:  true
                    implicitHeight:    ctrlCol.implicitHeight + AppStyle.spaceLg * 2
                    visible: !isServer
                    radius: AppStyle.radiusLg
                    color:  ThemeManager.surface
                    border { color: ThemeManager.border; width: 1 }

                    ColumnLayout {
                        id: ctrlCol
                        anchors { fill: parent; margins: AppStyle.spaceMd + AppStyle.spaceXs }
                        spacing: AppStyle.spaceSm

                        Text { text: "Session Control"; color: ThemeManager.text; font.pixelSize: AppStyle.fontMd; font.family: AppStyle.fontFamily; font.weight: 600 }

                        IecButton { label: "STARTDT"; Layout.fillWidth: true; isPrimary: false; isEnabled: IECVM.connected; onClicked: IECVM.sendStartDT() }
                        IecButton { label: "STOPDT";  Layout.fillWidth: true; isPrimary: false; isEnabled: IECVM.connected; onClicked: IECVM.sendStopDT() }
                        IecButton { label: "Interrogation (C_IC_NA_1)"; Layout.fillWidth: true; isPrimary: false; isEnabled: IECVM.connected; onClicked: IECVM.sendInterrogation() }
                        IecButton { label: "Clear Data Points"; Layout.fillWidth: true; isPrimary: false; onClicked: IECVM.clearDataPoints() }
                    }
                }

                // ── Server: set IOA card ──────────────────────
                Rectangle {
                    Layout.fillWidth:  true
                    implicitHeight:    setIoaCol.implicitHeight + AppStyle.spaceLg * 2
                    visible: isServer
                    radius: AppStyle.radiusLg
                    color:  ThemeManager.surface
                    border { color: ThemeManager.border; width: 1 }

                    ColumnLayout {
                        id: setIoaCol
                        anchors { fill: parent; margins: AppStyle.spaceMd + AppStyle.spaceXs }
                        spacing: AppStyle.spaceMd

                        Text { text: "Update IOA"; color: ThemeManager.text; font.pixelSize: AppStyle.fontMd; font.family: AppStyle.fontFamily; font.weight: 600 }
                        RowLayout {
                            Layout.fillWidth: true; spacing: AppStyle.spaceSm
                            ColumnLayout {
                                spacing: AppStyle.spaceXs; Layout.fillWidth: true
                                Text { text: "IOA"; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily }
                                IecField { id: ioaAddrField; placeholder: "100"; text: "100" }
                            }
                            ColumnLayout {
                                spacing: AppStyle.spaceXs; Layout.fillWidth: true
                                Text { text: "Float Value"; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily }
                                IecField { id: ioaValField; placeholder: "0.0"; text: "0.0" }
                            }
                        }
                        IecButton {
                            label: "Send Spontaneous"
                            isPrimary: true
                            Layout.fillWidth: true
                            isEnabled: IECVM.connected
                            onClicked: {
                                const ioa = parseInt(ioaAddrField.text) || 100
                                const val = parseFloat(ioaValField.text) || 0.0
                                IECVM.setServerIOA(ioa, val)
                            }
                        }
                    }
                }

                Item { Layout.fillHeight: true }
            }

            // ── RIGHT: Data points / Server IOAs ──────────────
            Rectangle {
                Layout.fillWidth:  true
                Layout.fillHeight: true
                radius: AppStyle.radiusLg
                color:  ThemeManager.surface
                border { color: ThemeManager.border; width: 1 }

                ColumnLayout {
                    anchors { fill: parent; margins: AppStyle.spaceMd + AppStyle.spaceXs }
                    spacing: AppStyle.spaceMd

                    Text {
                        text: isServer ? "IOA State" : (_titles[pageId] ?? "Protocol")
                        color: ThemeManager.text
                        font.pixelSize: AppStyle.fontXl; font.family: AppStyle.fontFamily; font.weight: 600
                    }

                    // Table header
                    Rectangle {
                        Layout.fillWidth: true; height: 32
                        color: ThemeManager.background; radius: AppStyle.radiusSm
                        RowLayout {
                            anchors { fill: parent; leftMargin: AppStyle.spaceMd; rightMargin: AppStyle.spaceMd }
                            Repeater {
                                model: isServer ? ["IOA", "Value"]
                                                : ["IOA", "Type", "Value", "Quality", "COT", "Timestamp"]
                                delegate: Text {
                                    text: modelData; color: ThemeManager.textSecondary
                                    font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily; font.weight: 500
                                    Layout.fillWidth: true
                                }
                            }
                        }
                    }

                    // Rows
                    ListView {
                        Layout.fillWidth:  true
                        Layout.fillHeight: true
                        model: isServer ? IECVM.serverIOAs : IECVM.dataPoints
                        clip:  true

                        delegate: Rectangle {
                            required property var modelData
                            width:  ListView.view.width
                            height: 28
                            color: index % 2 === 0 ? "transparent"
                                 : Qt.rgba(ThemeManager.text.r, ThemeManager.text.g, ThemeManager.text.b, 0.03)

                            RowLayout {
                                anchors { fill: parent; leftMargin: AppStyle.spaceMd; rightMargin: AppStyle.spaceMd }
                                Text { text: modelData.ioa       ?? ""; color: ThemeManager.accent;        font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontMono;   Layout.fillWidth: true }
                                Text { visible: !isServer; text: modelData.typeName  ?? ""; color: ThemeManager.text;          font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily; Layout.fillWidth: true }
                                Text { text: modelData.value     ?? ""; color: ThemeManager.text;          font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontMono;   Layout.fillWidth: true }
                                Text { visible: !isServer; text: modelData.quality   ?? ""; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontMono;   Layout.fillWidth: true }
                                Text { visible: !isServer; text: modelData.cot       ?? ""; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontMono;   Layout.fillWidth: true }
                                Text { visible: !isServer; text: modelData.timestamp ?? ""; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily; Layout.fillWidth: true }
                            }
                        }

                        Text {
                            visible: parent.count === 0
                            anchors.centerIn: parent
                            text: isServer
                                  ? (IECVM.connected ? "No IOA values set.\nUse 'Update IOA' to push spontaneous data." : "Start server to see IOA state.")
                                  : (IECVM.connected ? "No data points received yet.\nSend an Interrogation command."
                                                     : "Connect to an outstation to begin.")
                            color: ThemeManager.textDisabled
                            font.pixelSize: AppStyle.fontBase; font.family: AppStyle.fontFamily
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                }
            }
        }
    }

    // ── Error toast ───────────────────────────────────────────
    Rectangle {
        id: iecToast; visible: false
        anchors { bottom: parent.bottom; horizontalCenter: parent.horizontalCenter; bottomMargin: AppStyle.spaceLg }
        width: iecToastText.implicitWidth + AppStyle.spaceLg * 2; height: AppStyle.buttonHeightSm
        radius: AppStyle.radiusMd; color: "#CC2222"
        Text { id: iecToastText; anchors.centerIn: parent; color: "#FFF"; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily }
        Timer { id: iecToastTimer; interval: 3000; onTriggered: iecToast.visible = false }
    }
    Connections {
        target: IECVM
        function onErrorOccurred(msg) { iecToastText.text = msg; iecToast.visible = true; iecToastTimer.restart() }
    }

    // ── Inline components ─────────────────────────────────────
    component IecField: Rectangle {
        property alias text:        _if.text
        property string placeholder: ""
        Layout.fillWidth: true; height: AppStyle.inputHeight
        radius: AppStyle.radiusMd; color: ThemeManager.background
        border { color: _if.activeFocus ? ThemeManager.borderFocus : ThemeManager.border; width: 1 }
        TextInput {
            id: _if
            anchors { fill: parent; leftMargin: AppStyle.spaceSm + AppStyle.spaceXs; rightMargin: AppStyle.spaceSm }
            color: ThemeManager.text; font.pixelSize: AppStyle.fontBase; font.family: AppStyle.fontFamily; clip: true
            Text { visible: !parent.text && !parent.activeFocus; text: parent.parent.placeholder; color: ThemeManager.textDisabled; font: parent.font; anchors { left: parent.left; verticalCenter: parent.verticalCenter } }
        }
    }

    component IecButton: Rectangle {
        property string label:     ""
        property bool   isPrimary: true
        property bool   isEnabled: true
        signal clicked()
        implicitWidth: _ib.implicitWidth + AppStyle.spaceLg * 2; implicitHeight: AppStyle.buttonHeightSm
        radius: AppStyle.radiusMd; opacity: isEnabled ? 1.0 : 0.4
        color: isPrimary ? (_ibh.containsMouse ? ThemeManager.accentHover : ThemeManager.accent)
                         : (_ibh.containsMouse ? Qt.rgba(ThemeManager.text.r, ThemeManager.text.g, ThemeManager.text.b, 0.08) : "transparent")
        border { color: isPrimary ? "transparent" : ThemeManager.border; width: 1 }
        Behavior on color { ColorAnimation { duration: AppStyle.animFast } }
        Text { id: _ib; anchors.centerIn: parent; text: label; color: isPrimary ? "#FFF" : ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily }
        MouseArea { id: _ibh; anchors.fill: parent; hoverEnabled: true; cursorShape: isEnabled ? Qt.PointingHandCursor : Qt.ArrowCursor; onClicked: if (parent.isEnabled) parent.clicked() }
    }
}
