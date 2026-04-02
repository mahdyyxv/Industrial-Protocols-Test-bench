import QtQuick
import QtQuick.Layouts
import rtu.ui

// ─────────────────────────────────────────────────────────────
// SerialPage  (Free)
//
// Raw serial port monitor — send and receive raw bytes.
// ─────────────────────────────────────────────────────────────
Item {
    Rectangle { anchors.fill: parent; color: ThemeManager.background }

    RowLayout {
        anchors { fill: parent; margins: AppStyle.contentPadding }
        spacing: AppStyle.spaceMd

        // Config panel
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
                    font { pixelSize: AppStyle.fontMd; family: AppStyle.fontFamily; weight: Font.SemiBold }
                }

                Repeater {
                    model: [
                        { label: "Port",       placeholder: "COM1" },
                        { label: "Baud Rate",  placeholder: "9600" },
                        { label: "Data Bits",  placeholder: "8"    },
                        { label: "Parity",     placeholder: "None" },
                        { label: "Stop Bits",  placeholder: "1"    },
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
                        text:  "Open Port"
                        color: "#FFFFFF"
                        font { pixelSize: AppStyle.fontMd; family: AppStyle.fontFamily; weight: Font.Medium }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape:  Qt.PointingHandCursor
                    }
                }
            }
        }

        // Terminal
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
                        text:  "Terminal"
                        color: ThemeManager.text
                        font { pixelSize: AppStyle.fontMd; family: AppStyle.fontFamily; weight: Font.SemiBold }
                        Layout.fillWidth: true
                    }
                    StatusBadge { status: "disconnected" }
                }

                Rectangle {
                    Layout.fillWidth:  true
                    Layout.fillHeight: true
                    radius: AppStyle.radiusSm
                    color:  ThemeManager.background

                    // Terminal output — bind to SessionVM.terminalLines
                    Text {
                        anchors { fill: parent; margins: AppStyle.spaceSm }
                        text:  "Waiting for connection…"
                        color: ThemeManager.textSecondary
                        font { pixelSize: AppStyle.fontSm; family: AppStyle.fontMono }
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
                        border { color: ThemeManager.border; width: 1 }
                        TextInput {
                            anchors { fill: parent; leftMargin: AppStyle.spaceSm + AppStyle.spaceXs; rightMargin: AppStyle.spaceSm }
                            color: ThemeManager.text
                            font { pixelSize: AppStyle.fontBase; family: AppStyle.fontMono }
                            clip:  true
                            Text {
                                visible: !parent.text && !parent.activeFocus
                                text:    "Type hex or ASCII to send…"
                                color:   ThemeManager.textDisabled
                                font:    parent.font
                                anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                            }
                        }
                    }

                    Rectangle {
                        width:  80; height: AppStyle.inputHeight
                        radius: AppStyle.radiusMd
                        color:  ThemeManager.accent
                        Text {
                            anchors.centerIn: parent
                            text:  Icons.send + " Send"
                            color: "#FFFFFF"
                            font { pixelSize: AppStyle.fontBase; family: AppStyle.fontFamily }
                        }
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor }
                    }
                }
            }
        }
    }
}
