import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import rtu.ui

// ─────────────────────────────────────────────────────────────
// ModbusPage
//
// UI for Modbus RTU / TCP testing.
// Binds to SessionVM (stub) — no protocol logic here.
//
// Layout:
//   Left panel:  Connection config + Request builder
//   Right panel: Response table + Raw log
// ─────────────────────────────────────────────────────────────
Item {
    id: root

    // pageId injected by StackView on push
    property string pageId: "modbus_rtu"
    readonly property bool isTCP: pageId === "modbus_tcp"

    Rectangle { anchors.fill: parent; color: ThemeManager.background }

    RowLayout {
        anchors {
            fill:    parent
            margins: AppStyle.contentPadding
        }
        spacing: AppStyle.spaceMd

        // ── LEFT: Config + Request ────────────────────────
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

                    // Card title row
                    RowLayout {
                        Text {
                            text:  "Connection"
                            color: ThemeManager.text
                            font { pixelSize: AppStyle.fontMd; family: AppStyle.fontFamily; weight: 600 }
                            Layout.fillWidth: true
                        }
                        StatusBadge {
                            status: "disconnected"  // bind: SessionVM.connectionStatus
                        }
                    }

                    // ── Serial port (RTU only) ────────────
                    ColumnLayout {
                        visible: !root.isTCP
                        Layout.fillWidth: true
                        spacing: AppStyle.spaceXs

                        Text {
                            text:  "Port"
                            color: ThemeManager.textSecondary
                            font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily }
                        }
                        InputField { placeholder: "COM1"; Layout.fillWidth: true }

                        Text {
                            text:  "Baud Rate"
                            color: ThemeManager.textSecondary
                            font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily }
                        }
                        InputField { placeholder: "9600"; Layout.fillWidth: true }
                    }

                    // ── TCP host / port ───────────────────
                    ColumnLayout {
                        visible: root.isTCP
                        Layout.fillWidth: true
                        spacing: AppStyle.spaceXs

                        Text {
                            text:  "Host"
                            color: ThemeManager.textSecondary
                            font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily }
                        }
                        InputField { placeholder: "192.168.1.1"; Layout.fillWidth: true }

                        Text {
                            text:  "Port"
                            color: ThemeManager.textSecondary
                            font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily }
                        }
                        InputField { placeholder: "502"; Layout.fillWidth: true }
                    }

                    // ── Slave / Unit ID ───────────────────
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: AppStyle.spaceXs
                        Text {
                            text:  "Unit ID"
                            color: ThemeManager.textSecondary
                            font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily }
                        }
                        InputField { placeholder: "1"; Layout.fillWidth: true }
                    }

                    // Connect / Disconnect button
                    ActionButton {
                        Layout.fillWidth: true
                        label: "Connect"   // bind: SessionVM.isConnected ? "Disconnect" : "Connect"
                        isPrimary: true
                    }
                }
            }

            // Request builder card
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
                        text:  "Request"
                        color: ThemeManager.text
                        font { pixelSize: AppStyle.fontMd; family: AppStyle.fontFamily; weight: 600 }
                    }

                    // Function code selector
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: AppStyle.spaceXs
                        Text {
                            text:  "Function Code"
                            color: ThemeManager.textSecondary
                            font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily }
                        }
                        InputField {
                            placeholder: "03 - Read Holding Registers"
                            Layout.fillWidth: true
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: AppStyle.spaceSm

                        ColumnLayout {
                            spacing: AppStyle.spaceXs
                            Layout.fillWidth: true
                            Text {
                                text:  "Start Address"
                                color: ThemeManager.textSecondary
                                font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily }
                            }
                            InputField { placeholder: "0"; Layout.fillWidth: true }
                        }

                        ColumnLayout {
                            spacing: AppStyle.spaceXs
                            Layout.fillWidth: true
                            Text {
                                text:  "Count"
                                color: ThemeManager.textSecondary
                                font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily }
                            }
                            InputField { placeholder: "10"; Layout.fillWidth: true }
                        }
                    }

                    Item { Layout.fillHeight: true }

                    ActionButton {
                        Layout.fillWidth: true
                        label:     Icons.send + "  Send Request"
                        isPrimary: true
                        // onClicked: SessionVM.sendRequest(...)
                    }
                }
            }
        }

        // ── RIGHT: Response + Log ─────────────────────────
        ColumnLayout {
            Layout.fillWidth:  true
            Layout.fillHeight: true
            spacing: AppStyle.spaceMd

            // Response table
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
                            text:  "Response"
                            color: ThemeManager.text
                            font { pixelSize: AppStyle.fontMd; family: AppStyle.fontFamily; weight: 600 }
                            Layout.fillWidth: true
                        }
                        ActionButton { label: Icons.clear + "  Clear"; isPrimary: false }
                    }

                    // Table header
                    Rectangle {
                        Layout.fillWidth: true
                        height: 32
                        color:  ThemeManager.background
                        radius: AppStyle.radiusSm

                        RowLayout {
                            anchors { fill: parent; leftMargin: AppStyle.spaceMd; rightMargin: AppStyle.spaceMd }
                            spacing: 0

                            Repeater {
                                model: ["Address", "Value (Dec)", "Value (Hex)", "Value (Bin)"]
                                delegate: Text {
                                    text:  modelData
                                    color: ThemeManager.textSecondary
                                    font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily; weight: 500 }
                                    Layout.fillWidth: true
                                }
                            }
                        }
                    }

                    // Table rows (bind to SessionVM.responseModel)
                    Rectangle {
                        Layout.fillWidth:  true
                        Layout.fillHeight: true
                        color: "transparent"

                        Text {
                            anchors.centerIn: parent
                            text:  "Response will appear here after sending a request."
                            color: ThemeManager.textDisabled
                            font { pixelSize: AppStyle.fontBase; family: AppStyle.fontFamily }
                        }
                    }
                }
            }

            // Raw hex log
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
                        Text {
                            text:  "Raw Log"
                            color: ThemeManager.text
                            font { pixelSize: AppStyle.fontMd; family: AppStyle.fontFamily; weight: 600 }
                            Layout.fillWidth: true
                        }
                        ActionButton { label: Icons.copy; isPrimary: false }
                        ActionButton { label: Icons.clear; isPrimary: false }
                    }

                    Rectangle {
                        Layout.fillWidth:  true
                        Layout.fillHeight: true
                        radius: AppStyle.radiusSm
                        color:  ThemeManager.background

                        Text {
                            anchors { fill: parent; margins: AppStyle.spaceSm }
                            text:  "→ Ready\n← Waiting for response…"
                            color: ThemeManager.textSecondary
                            font { pixelSize: AppStyle.fontSm; family: AppStyle.fontMono }
                            wrapMode: Text.WrapAnywhere
                        }
                    }
                }
            }
        }
    }

    // ── Inline helper components ──────────────────────────────
    component InputField: Rectangle {
        property string placeholder: ""
        height: AppStyle.inputHeight
        radius: AppStyle.radiusMd
        color:  ThemeManager.background
        border { color: _input.activeFocus ? ThemeManager.borderFocus : ThemeManager.border; width: 1 }

        TextInput {
            id: _input
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

    component ActionButton: Rectangle {
        property string label:     ""
        property bool   isPrimary: true
        signal clicked()

        implicitWidth:  _btnText.implicitWidth + AppStyle.spaceLg * 2
        implicitHeight: AppStyle.buttonHeightSm
        radius: AppStyle.radiusMd
        color: {
            if (isPrimary) return _btnHover.containsMouse ? ThemeManager.accentHover : ThemeManager.accent
            return _btnHover.containsMouse
                       ? Qt.rgba(ThemeManager.text.r, ThemeManager.text.g, ThemeManager.text.b, 0.08)
                       : "transparent"
        }
        border { color: isPrimary ? "transparent" : ThemeManager.border; width: 1 }
        Behavior on color { ColorAnimation { duration: AppStyle.animFast } }

        Text {
            id: _btnText
            anchors.centerIn: parent
            text:  label
            color: isPrimary ? "#FFFFFF" : ThemeManager.textSecondary
            font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily }
        }

        MouseArea {
            id: _btnHover
            anchors.fill: parent
            hoverEnabled: true
            cursorShape:  Qt.PointingHandCursor
            onClicked:    parent.clicked()
        }
    }
}
