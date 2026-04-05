import QtQuick
import QtQuick.Layouts
import rtu.ui

// ─────────────────────────────────────────────────────────────
// SerialPage  (Free)
//
// Binds to SerialVM (SerialController context property).
// ─────────────────────────────────────────────────────────────
Item {
    Rectangle { anchors.fill: parent; color: ThemeManager.background }

    RowLayout {
        anchors { fill: parent; margins: AppStyle.contentPadding }
        spacing: AppStyle.spaceMd

        // ── Config panel ──────────────────────────────────────
        Rectangle {
            Layout.preferredWidth: 260
            Layout.fillHeight:     true
            radius: AppStyle.radiusLg
            color:  ThemeManager.surface
            border { color: ThemeManager.border; width: 1 }

            ColumnLayout {
                anchors { fill: parent; margins: AppStyle.spaceMd + AppStyle.spaceXs }
                spacing: AppStyle.spaceMd

                Text {
                    text:  "Serial Port"
                    color: ThemeManager.text
                    font { pixelSize: AppStyle.fontMd; family: AppStyle.fontFamily; weight: 600 }
                }

                // Port
                ColumnLayout { Layout.fillWidth: true; spacing: AppStyle.spaceXs
                    Text { text: "Port"; color: ThemeManager.textSecondary; font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily } }
                    ConfigField { id: portF; placeholder: "COM1"; text: SerialVM.portName; onTextChanged: SerialVM.portName = text }
                }

                // Baud Rate
                ColumnLayout { Layout.fillWidth: true; spacing: AppStyle.spaceXs
                    Text { text: "Baud Rate"; color: ThemeManager.textSecondary; font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily } }
                    ConfigField { placeholder: "9600"; text: SerialVM.baudRate; onTextChanged: { const v = parseInt(text); if (!isNaN(v)) SerialVM.baudRate = v } }
                }

                // Data Bits
                ColumnLayout { Layout.fillWidth: true; spacing: AppStyle.spaceXs
                    Text { text: "Data Bits"; color: ThemeManager.textSecondary; font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily } }
                    ConfigField { placeholder: "8"; text: SerialVM.dataBits; onTextChanged: { const v = parseInt(text); if (!isNaN(v)) SerialVM.dataBits = v } }
                }

                // Parity
                ColumnLayout { Layout.fillWidth: true; spacing: AppStyle.spaceXs
                    Text { text: "Parity"; color: ThemeManager.textSecondary; font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily } }
                    ConfigField { placeholder: "None"; text: SerialVM.parity; onTextChanged: SerialVM.parity = text }
                }

                // Stop Bits
                ColumnLayout { Layout.fillWidth: true; spacing: AppStyle.spaceXs
                    Text { text: "Stop Bits"; color: ThemeManager.textSecondary; font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily } }
                    ConfigField { placeholder: "1"; text: SerialVM.stopBits; onTextChanged: { const v = parseInt(text); if (!isNaN(v)) SerialVM.stopBits = v } }
                }

                Item { Layout.fillHeight: true }

                Rectangle {
                    Layout.fillWidth: true
                    height: AppStyle.buttonHeight
                    radius: AppStyle.radiusMd
                    color:  SerialVM.portOpen ? "#CC2222" : ThemeManager.accent
                    Behavior on color { ColorAnimation { duration: AppStyle.animFast } }

                    Text {
                        anchors.centerIn: parent
                        text:  SerialVM.portOpen ? "Close Port" : "Open Port"
                        color: "#FFFFFF"
                        font { pixelSize: AppStyle.fontMd; family: AppStyle.fontFamily; weight: 500 }
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape:  Qt.PointingHandCursor
                        onClicked:    SerialVM.open()
                    }
                }
            }
        }

        // ── Terminal ──────────────────────────────────────────
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
                        text: "Terminal"; color: ThemeManager.text
                        font { pixelSize: AppStyle.fontMd; family: AppStyle.fontFamily; weight: 600 }
                        Layout.fillWidth: true
                    }
                    StatusBadge { status: SerialVM.portOpen ? "connected" : "disconnected" }
                    Rectangle {
                        width: 60; height: AppStyle.buttonHeightSm
                        radius: AppStyle.radiusMd; color: "transparent"
                        border { color: ThemeManager.border; width: 1 }
                        Text { anchors.centerIn: parent; text: Icons.clear; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm }
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: SerialVM.clearTerminal() }
                    }
                }

                // Terminal output
                Rectangle {
                    Layout.fillWidth:  true
                    Layout.fillHeight: true
                    radius: AppStyle.radiusSm
                    color:  ThemeManager.background

                    ListView {
                        anchors { fill: parent; margins: AppStyle.spaceSm }
                        model: SerialVM.terminalLines
                        clip:  true
                        verticalLayoutDirection: ListView.BottomToTop

                        delegate: Text {
                            required property string modelData
                            width: ListView.view.width
                            text:  modelData
                            color: modelData.includes("→") ? ThemeManager.accent
                                 : modelData.includes("←") ? ThemeManager.text
                                 : ThemeManager.textSecondary
                            font { pixelSize: AppStyle.fontSm; family: AppStyle.fontMono }
                            wrapMode: Text.WrapAnywhere
                        }

                        Text {
                            visible: parent.count === 0
                            anchors.centerIn: parent
                            text:  "Waiting for connection…"
                            color: ThemeManager.textSecondary
                            font { pixelSize: AppStyle.fontSm; family: AppStyle.fontMono }
                        }
                    }
                }

                // Send row
                RowLayout {
                    Layout.fillWidth: true
                    spacing: AppStyle.spaceSm

                    Rectangle {
                        Layout.fillWidth: true
                        height: AppStyle.inputHeight
                        radius: AppStyle.radiusMd
                        color:  ThemeManager.background
                        border { color: sendInput.activeFocus ? ThemeManager.borderFocus : ThemeManager.border; width: 1 }

                        TextInput {
                            id: sendInput
                            anchors { fill: parent; leftMargin: AppStyle.spaceSm + AppStyle.spaceXs; rightMargin: AppStyle.spaceSm }
                            color: ThemeManager.text
                            font { pixelSize: AppStyle.fontBase; family: AppStyle.fontMono }
                            clip: true
                            Keys.onReturnPressed: sendBtn.doSend()
                            Text {
                                visible: !parent.text && !parent.activeFocus
                                text:    "Type hex (AA BB CC) or ASCII to send…"
                                color:   ThemeManager.textDisabled
                                font:    parent.font
                                anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                            }
                        }
                    }

                    Rectangle {
                        id: sendBtn
                        width: 80; height: AppStyle.inputHeight
                        radius: AppStyle.radiusMd
                        color: SerialVM.portOpen ? ThemeManager.accent : ThemeManager.border
                        Behavior on color { ColorAnimation { duration: AppStyle.animFast } }

                        function doSend() {
                            if (SerialVM.portOpen && sendInput.text.length > 0) {
                                SerialVM.send(sendInput.text)
                                sendInput.text = ""
                            }
                        }

                        Text {
                            anchors.centerIn: parent
                            text: Icons.send + " Send"; color: "#FFF"
                            font { pixelSize: AppStyle.fontBase; family: AppStyle.fontFamily }
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: sendBtn.doSend()
                        }
                    }
                }
            }
        }
    }

    // ── Config field component ────────────────────────────────
    component ConfigField: Rectangle {
        property alias text:        _ci.text
        property string placeholder: ""

        Layout.fillWidth: true
        height: AppStyle.inputHeight
        radius: AppStyle.radiusMd
        color:  ThemeManager.background
        border { color: _ci.activeFocus ? ThemeManager.borderFocus : ThemeManager.border; width: 1 }

        TextInput {
            id: _ci
            anchors { fill: parent; leftMargin: AppStyle.spaceSm + AppStyle.spaceXs; rightMargin: AppStyle.spaceSm }
            color: ThemeManager.text
            font { pixelSize: AppStyle.fontBase; family: AppStyle.fontFamily }
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
}
