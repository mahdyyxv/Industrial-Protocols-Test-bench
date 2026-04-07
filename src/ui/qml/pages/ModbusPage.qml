import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import rtu.ui

// ─────────────────────────────────────────────────────────────
// ModbusPage
//
// Binds to ModbusVM (ModbusController context property).
// Supports Client (TCP/RTU) and Server roles via ComboBox.
// ─────────────────────────────────────────────────────────────
Item {
    id: root

    property string pageId: "modbus_rtu"
    readonly property bool isTCP: pageId === "modbus_tcp"
    readonly property bool isServer: ModbusVM.role === "Server"

    Component.onCompleted: ModbusVM.mode = isTCP ? "TCP" : "RTU"

    Rectangle { anchors.fill: parent; color: ThemeManager.background }

    RowLayout {
        anchors { fill: parent; margins: AppStyle.contentPadding }
        spacing: AppStyle.spaceMd

        // ── LEFT: Config ──────────────────────────────────────
        ColumnLayout {
            Layout.preferredWidth: 320
            Layout.fillHeight:     true
            spacing: AppStyle.spaceMd

            // Connection card
            Rectangle {
                Layout.fillWidth: true
                implicitHeight:   connLayout.implicitHeight + AppStyle.spaceLg * 2
                radius: AppStyle.radiusLg
                color:  ThemeManager.surface
                border { color: ThemeManager.border; width: 1 }

                ColumnLayout {
                    id: connLayout
                    anchors { fill: parent; margins: AppStyle.spaceMd + AppStyle.spaceXs }
                    spacing: AppStyle.spaceMd

                    RowLayout {
                        Text {
                            text:  "Connection"
                            color: ThemeManager.text
                            font.pixelSize: AppStyle.fontMd
                            font.family:    AppStyle.fontFamily
                            font.weight:    600
                            Layout.fillWidth: true
                        }
                        StatusBadge {
                            status: ModbusVM.connected ? "connected" : "disconnected"
                        }
                    }

                    // ── Role selector ─────────────────────────
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: AppStyle.spaceXs
                        Text { text: "Role"; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily }
                        ComboBox {
                            id: roleCombo
                            Layout.fillWidth: true
                            model: ["Client", "Server"]
                            currentIndex: ModbusVM.role === "Server" ? 1 : 0
                            enabled: !ModbusVM.connected
                            onActivated: ModbusVM.role = currentText
                            background: Rectangle {
                                color:  ThemeManager.background
                                radius: AppStyle.radiusMd
                                border { color: ThemeManager.border; width: 1 }
                            }
                            contentItem: Text {
                                leftPadding: AppStyle.spaceSm + AppStyle.spaceXs
                                text:  roleCombo.displayText
                                color: ThemeManager.text
                                font.pixelSize: AppStyle.fontBase
                                font.family:    AppStyle.fontFamily
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }

                    // ── Client: Serial port (RTU only) ────────
                    ColumnLayout {
                        visible: !root.isTCP && !root.isServer
                        Layout.fillWidth: true
                        spacing: AppStyle.spaceXs
                        Text { text: "Port"; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily }
                        InputField { placeholder: "COM1"; text: ModbusVM.portName; onTextChanged: ModbusVM.portName = text; Layout.fillWidth: true }
                        Text { text: "Baud Rate"; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily }
                        InputField {
                            placeholder: "9600"; text: ModbusVM.baudRate
                            onTextChanged: { const v = parseInt(text); if (!isNaN(v)) ModbusVM.baudRate = v }
                            Layout.fillWidth: true
                        }
                    }

                    // ── Client: TCP host / port ────────────────
                    ColumnLayout {
                        visible: root.isTCP && !root.isServer
                        Layout.fillWidth: true
                        spacing: AppStyle.spaceXs
                        Text { text: "Host"; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily }
                        InputField { placeholder: "192.168.1.1"; text: ModbusVM.host; onTextChanged: ModbusVM.host = text; Layout.fillWidth: true }
                        Text { text: "Port"; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily }
                        InputField {
                            placeholder: "502"; text: ModbusVM.port
                            onTextChanged: { const v = parseInt(text); if (!isNaN(v)) ModbusVM.port = v }
                            Layout.fillWidth: true
                        }
                    }

                    // ── Client: Unit ID ───────────────────────
                    ColumnLayout {
                        visible: !root.isServer
                        Layout.fillWidth: true
                        spacing: AppStyle.spaceXs
                        Text { text: "Unit ID"; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily }
                        InputField {
                            placeholder: "1"; text: ModbusVM.slaveId
                            onTextChanged: { const v = parseInt(text); if (!isNaN(v)) ModbusVM.slaveId = v }
                            Layout.fillWidth: true
                        }
                    }

                    // ── Server: Listen port ───────────────────
                    ColumnLayout {
                        visible: root.isServer
                        Layout.fillWidth: true
                        spacing: AppStyle.spaceXs
                        Text { text: "Listen Port"; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily }
                        InputField {
                            placeholder: "502"; text: ModbusVM.listenPort
                            onTextChanged: { const v = parseInt(text); if (!isNaN(v)) ModbusVM.listenPort = v }
                            Layout.fillWidth: true
                        }
                        Text { text: "Clients: " + ModbusVM.clientCount; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily }
                    }

                    ActionButton {
                        Layout.fillWidth: true
                        label: ModbusVM.connected
                               ? (root.isServer ? "Stop Server" : "Disconnect")
                               : (root.isServer ? "Start Server" : "Connect")
                        isPrimary: true
                        onClicked: ModbusVM.connected ? ModbusVM.disconnectDevice() : ModbusVM.connectDevice()
                    }
                }
            }

            // ── Client: Request builder card ──────────────────
            Rectangle {
                Layout.fillWidth:  true
                Layout.fillHeight: true
                visible: !root.isServer
                radius: AppStyle.radiusLg
                color:  ThemeManager.surface
                border { color: ThemeManager.border; width: 1 }

                ColumnLayout {
                    anchors { fill: parent; margins: AppStyle.spaceMd + AppStyle.spaceXs }
                    spacing: AppStyle.spaceMd

                    Text {
                        text:  "Request"
                        color: ThemeManager.text
                        font.pixelSize: AppStyle.fontMd
                        font.family:    AppStyle.fontFamily
                        font.weight:    600
                    }

                    ColumnLayout {
                        Layout.fillWidth: true; spacing: AppStyle.spaceXs
                        Text { text: "Function Code"; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily }
                        InputField { id: fcField; placeholder: "3 - Read Holding Registers"; text: "3"; Layout.fillWidth: true }
                    }

                    RowLayout {
                        Layout.fillWidth: true; spacing: AppStyle.spaceSm
                        ColumnLayout {
                            spacing: AppStyle.spaceXs; Layout.fillWidth: true
                            Text { text: "Start Address"; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily }
                            InputField { id: addrField; placeholder: "0"; text: "0"; Layout.fillWidth: true }
                        }
                        ColumnLayout {
                            spacing: AppStyle.spaceXs; Layout.fillWidth: true
                            Text { text: "Count / Value"; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily }
                            InputField { id: countField; placeholder: "10"; text: "10"; Layout.fillWidth: true }
                        }
                    }

                    Item { Layout.fillHeight: true }

                    ActionButton {
                        Layout.fillWidth: true
                        label:     Icons.send + "  Send Request"
                        isPrimary: true
                        isEnabled: ModbusVM.connected
                        onClicked: {
                            const fc   = parseInt(fcField.text) || 3
                            const addr = parseInt(addrField.text) || 0
                            const cnt  = parseInt(countField.text) || 10
                            if (fc === 6) ModbusVM.write(addr, cnt)
                            else          ModbusVM.read(addr, cnt)
                        }
                    }
                }
            }

            // ── Server: Set register card ─────────────────────
            Rectangle {
                Layout.fillWidth:  true
                implicitHeight:    setRegLayout.implicitHeight + AppStyle.spaceLg * 2
                visible: root.isServer
                radius: AppStyle.radiusLg
                color:  ThemeManager.surface
                border { color: ThemeManager.border; width: 1 }

                ColumnLayout {
                    id: setRegLayout
                    anchors { fill: parent; margins: AppStyle.spaceMd + AppStyle.spaceXs }
                    spacing: AppStyle.spaceMd

                    Text {
                        text:  "Set Register"
                        color: ThemeManager.text
                        font.pixelSize: AppStyle.fontMd
                        font.family:    AppStyle.fontFamily
                        font.weight:    600
                    }
                    RowLayout {
                        Layout.fillWidth: true; spacing: AppStyle.spaceSm
                        ColumnLayout {
                            spacing: AppStyle.spaceXs; Layout.fillWidth: true
                            Text { text: "Address"; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily }
                            InputField { id: srvAddrField; placeholder: "0"; text: "0"; Layout.fillWidth: true }
                        }
                        ColumnLayout {
                            spacing: AppStyle.spaceXs; Layout.fillWidth: true
                            Text { text: "Value"; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily }
                            InputField { id: srvValField; placeholder: "0"; text: "0"; Layout.fillWidth: true }
                        }
                    }
                    ActionButton {
                        Layout.fillWidth: true
                        label: "Set Register"
                        isPrimary: true
                        isEnabled: ModbusVM.connected
                        onClicked: {
                            const addr = parseInt(srvAddrField.text) || 0
                            const val  = parseInt(srvValField.text)  || 0
                            ModbusVM.setServerRegister(addr, val)
                        }
                    }
                }
            }

            Item { Layout.fillHeight: true; visible: root.isServer }
        }

        // ── RIGHT: Response / Server registers + Log ──────────
        ColumnLayout {
            Layout.fillWidth:  true
            Layout.fillHeight: true
            spacing: AppStyle.spaceMd

            // Client: Response table  |  Server: Register bank
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
                            text:  root.isServer ? "Server Registers" : "Response"
                            color: ThemeManager.text
                            font.pixelSize: AppStyle.fontMd
                            font.family:    AppStyle.fontFamily
                            font.weight:    600
                            Layout.fillWidth: true
                        }
                        ActionButton { label: Icons.clear + "  Clear"; isPrimary: false; onClicked: ModbusVM.clearLog() }
                    }

                    // Table header
                    Rectangle {
                        Layout.fillWidth: true; height: 32
                        color: ThemeManager.background; radius: AppStyle.radiusSm
                        RowLayout {
                            anchors { fill: parent; leftMargin: AppStyle.spaceMd; rightMargin: AppStyle.spaceMd }
                            spacing: 0
                            Repeater {
                                model: root.isServer ? ["Address", "Value (Dec)", "Value (Hex)"]
                                                     : ["Address", "Value (Dec)", "Value (Hex)", "Value (Bin)"]
                                delegate: Text {
                                    text: modelData; color: ThemeManager.textSecondary
                                    font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily; font.weight: 500
                                    Layout.fillWidth: true
                                }
                            }
                        }
                    }

                    // Table rows
                    ListView {
                        Layout.fillWidth:  true
                        Layout.fillHeight: true
                        model: root.isServer ? ModbusVM.serverRegisters : ModbusVM.responseModel
                        clip:  true

                        delegate: Rectangle {
                            required property var modelData
                            width:  ListView.view.width
                            height: 28
                            color:  index % 2 === 0 ? "transparent"
                                  : Qt.rgba(ThemeManager.text.r, ThemeManager.text.g, ThemeManager.text.b, 0.03)

                            RowLayout {
                                anchors { fill: parent; leftMargin: AppStyle.spaceMd; rightMargin: AppStyle.spaceMd }
                                spacing: 0
                                Text { text: modelData.address ?? ""; color: ThemeManager.text; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontMono; Layout.fillWidth: true }
                                Text { text: (root.isServer ? modelData.value : modelData.dec) ?? ""; color: ThemeManager.text; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontMono; Layout.fillWidth: true }
                                Text { text: modelData.hex ?? ""; color: ThemeManager.accent; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontMono; Layout.fillWidth: true }
                                Text { visible: !root.isServer; text: modelData.bin ?? ""; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontMono; Layout.fillWidth: true }
                            }
                        }

                        Text {
                            visible: parent.count === 0
                            anchors.centerIn: parent
                            text: root.isServer
                                  ? (ModbusVM.connected ? "No non-zero registers.\nUse 'Set Register' to populate." : "Start server to see register bank.")
                                  : "Response will appear here after sending a request."
                            color: ThemeManager.textDisabled
                            font.pixelSize: AppStyle.fontBase; font.family: AppStyle.fontFamily
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                }
            }

            // Raw log
            Rectangle {
                Layout.fillWidth:   true
                Layout.preferredHeight: 160
                radius: AppStyle.radiusLg
                color:  ThemeManager.surface
                border { color: ThemeManager.border; width: 1 }

                ColumnLayout {
                    anchors { fill: parent; margins: AppStyle.spaceMd + AppStyle.spaceXs }
                    spacing: AppStyle.spaceSm

                    RowLayout {
                        Text { text: "Raw Log"; color: ThemeManager.text; font.pixelSize: AppStyle.fontMd; font.family: AppStyle.fontFamily; font.weight: 600; Layout.fillWidth: true }
                        ActionButton { label: Icons.clear; isPrimary: false; onClicked: ModbusVM.clearLog() }
                    }

                    Rectangle {
                        Layout.fillWidth:  true
                        Layout.fillHeight: true
                        radius: AppStyle.radiusSm
                        color:  ThemeManager.background

                        ListView {
                            anchors { fill: parent; margins: AppStyle.spaceSm }
                            model: ModbusVM.rawLog
                            clip:  true
                            verticalLayoutDirection: ListView.BottomToTop

                            delegate: Text {
                                required property string modelData
                                width: ListView.view.width
                                text:  modelData
                                color: modelData.startsWith("✗") ? "#FF5555"
                                     : modelData.startsWith("→") ? ThemeManager.accent
                                     : ThemeManager.textSecondary
                                font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontMono
                                wrapMode: Text.WrapAnywhere
                            }

                            Text {
                                visible: parent.count === 0
                                anchors.centerIn: parent
                                text:  "→ Ready"
                                color: ThemeManager.textSecondary
                                font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontMono
                            }
                        }
                    }
                }
            }
        }
    }

    // Error toast
    Rectangle {
        id: errorToast; visible: false
        anchors { bottom: parent.bottom; horizontalCenter: parent.horizontalCenter; bottomMargin: AppStyle.spaceLg }
        width: errorText.implicitWidth + AppStyle.spaceLg * 2; height: AppStyle.buttonHeightSm
        radius: AppStyle.radiusMd; color: "#CC2222"
        Text { id: errorText; anchors.centerIn: parent; color: "#FFF"; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily }
        Timer { id: toastTimer; interval: 3000; onTriggered: errorToast.visible = false }
    }
    Connections {
        target: ModbusVM
        function onErrorOccurred(msg) { errorText.text = msg; errorToast.visible = true; toastTimer.restart() }
    }

    // ── Inline components ─────────────────────────────────────
    component InputField: Rectangle {
        property alias text:        _input.text
        property string placeholder: ""

        height: AppStyle.inputHeight
        radius: AppStyle.radiusMd
        color:  ThemeManager.background
        border { color: _input.activeFocus ? ThemeManager.borderFocus : ThemeManager.border; width: 1 }

        TextInput {
            id: _input
            anchors { fill: parent; leftMargin: AppStyle.spaceSm + AppStyle.spaceXs; rightMargin: AppStyle.spaceSm }
            color: ThemeManager.text
            font.pixelSize: AppStyle.fontBase; font.family: AppStyle.fontFamily
            clip: true
            Text {
                visible: !parent.text && !parent.activeFocus
                text:    parent.parent.placeholder
                color:   ThemeManager.textDisabled
                font:    parent.font
                anchors { left: parent.left; verticalCenter: parent.verticalCenter }
            }
        }
    }

    component ActionButton: Rectangle {
        property string label:     ""
        property bool   isPrimary: true
        property bool   isEnabled: true
        signal clicked()

        implicitWidth:  _btnText.implicitWidth + AppStyle.spaceLg * 2
        implicitHeight: AppStyle.buttonHeightSm
        radius: AppStyle.radiusMd
        opacity: isEnabled ? 1.0 : 0.4
        color: {
            if (isPrimary) return _btnHover.containsMouse ? ThemeManager.accentHover : ThemeManager.accent
            return _btnHover.containsMouse
                       ? Qt.rgba(ThemeManager.text.r, ThemeManager.text.g, ThemeManager.text.b, 0.08)
                       : "transparent"
        }
        border { color: isPrimary ? "transparent" : ThemeManager.border; width: 1 }
        Behavior on color { ColorAnimation { duration: AppStyle.animFast } }

        Text { id: _btnText; anchors.centerIn: parent; text: label; color: isPrimary ? "#FFFFFF" : ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily }
        MouseArea { id: _btnHover; anchors.fill: parent; hoverEnabled: true; cursorShape: isEnabled ? Qt.PointingHandCursor : Qt.ArrowCursor; onClicked: if (parent.isEnabled) parent.clicked() }
    }
}
