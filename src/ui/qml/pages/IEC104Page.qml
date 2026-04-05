import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import rtu.ui

// ─────────────────────────────────────────────────────────────
// IEC104Page  (Pro)
//
// Binds to IECVM (IECController context property).
// ─────────────────────────────────────────────────────────────
Item {
    property string pageId: "iec104"

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

                        ColumnLayout { Layout.fillWidth: true; spacing: AppStyle.spaceXs
                            Text { text: "Host"; color: ThemeManager.textSecondary; font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily } }
                            IecField { placeholder: "192.168.1.100"; text: IECVM.host; onTextChanged: IECVM.host = text }
                        }
                        ColumnLayout { Layout.fillWidth: true; spacing: AppStyle.spaceXs
                            Text { text: "Port"; color: ThemeManager.textSecondary; font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily } }
                            IecField { placeholder: "2404"; text: IECVM.port; onTextChanged: { const v = parseInt(text); if (!isNaN(v)) IECVM.port = v } }
                        }
                        ColumnLayout { Layout.fillWidth: true; spacing: AppStyle.spaceXs
                            Text { text: "Common Address"; color: ThemeManager.textSecondary; font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily } }
                            IecField { placeholder: "1"; text: IECVM.commonAddress; onTextChanged: { const v = parseInt(text); if (!isNaN(v)) IECVM.commonAddress = v } }
                        }

                        // State text
                        Text { text: IECVM.stateText; color: ThemeManager.textSecondary; font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily } }

                        IecButton { label: IECVM.connected ? "Disconnect" : "Connect"; isPrimary: true; Layout.fillWidth: true; onClicked: IECVM.connected ? IECVM.disconnectDevice() : IECVM.connectDevice() }
                    }
                }

                // Session control card
                Rectangle {
                    Layout.fillWidth:  true
                    implicitHeight:    ctrlCol.implicitHeight + AppStyle.spaceLg * 2
                    radius: AppStyle.radiusLg
                    color:  ThemeManager.surface
                    border { color: ThemeManager.border; width: 1 }

                    ColumnLayout {
                        id: ctrlCol
                        anchors { fill: parent; margins: AppStyle.spaceMd + AppStyle.spaceXs }
                        spacing: AppStyle.spaceSm

                        Text { text: "Session Control"; color: ThemeManager.text; font { pixelSize: AppStyle.fontMd; family: AppStyle.fontFamily; weight: 600 } }

                        IecButton { label: "STARTDT"; Layout.fillWidth: true; isPrimary: false; isEnabled: IECVM.connected; onClicked: IECVM.sendStartDT() }
                        IecButton { label: "STOPDT";  Layout.fillWidth: true; isPrimary: false; isEnabled: IECVM.connected; onClicked: IECVM.sendStopDT() }
                        IecButton { label: "Interrogation (C_IC_NA_1)"; Layout.fillWidth: true; isPrimary: false; isEnabled: IECVM.connected; onClicked: IECVM.sendInterrogation() }
                        IecButton { label: "Clear Data Points"; Layout.fillWidth: true; isPrimary: false; onClicked: IECVM.clearDataPoints() }
                    }
                }

                Item { Layout.fillHeight: true }
            }

            // ── RIGHT: Data point table ───────────────────────
            Rectangle {
                Layout.fillWidth:  true
                Layout.fillHeight: true
                radius: AppStyle.radiusLg
                color:  ThemeManager.surface
                border { color: ThemeManager.border; width: 1 }

                ColumnLayout {
                    anchors { fill: parent; margins: AppStyle.spaceMd + AppStyle.spaceXs }
                    spacing: AppStyle.spaceMd

                    Text { text: _titles[pageId] ?? "Protocol"; color: ThemeManager.text; font { pixelSize: AppStyle.fontXl; family: AppStyle.fontFamily; weight: 600 } }

                    // Table header
                    Rectangle {
                        Layout.fillWidth: true; height: 32
                        color: ThemeManager.background; radius: AppStyle.radiusSm
                        RowLayout {
                            anchors { fill: parent; leftMargin: AppStyle.spaceMd; rightMargin: AppStyle.spaceMd }
                            Repeater {
                                model: ["IOA", "Type", "Value", "Quality", "COT", "Timestamp"]
                                delegate: Text {
                                    text: modelData; color: ThemeManager.textSecondary
                                    font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily; weight: 500 }
                                    Layout.fillWidth: true
                                }
                            }
                        }
                    }

                    // Data point rows
                    ListView {
                        Layout.fillWidth:  true
                        Layout.fillHeight: true
                        model: IECVM.dataPoints
                        clip:  true

                        delegate: Rectangle {
                            required property var modelData
                            width:  ListView.view.width
                            height: 28
                            color: index % 2 === 0 ? "transparent" : Qt.rgba(ThemeManager.text.r, ThemeManager.text.g, ThemeManager.text.b, 0.03)

                            RowLayout {
                                anchors { fill: parent; leftMargin: AppStyle.spaceMd; rightMargin: AppStyle.spaceMd }
                                Text { text: modelData.ioa       ?? ""; color: ThemeManager.accent; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontMono; Layout.fillWidth: true }
                                Text { text: modelData.typeName  ?? ""; color: ThemeManager.text;   font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily; Layout.fillWidth: true }
                                Text { text: modelData.value     ?? ""; color: ThemeManager.text;   font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontMono; Layout.fillWidth: true }
                                Text { text: modelData.quality   ?? ""; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontMono; Layout.fillWidth: true }
                                Text { text: modelData.cot       ?? ""; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontMono; Layout.fillWidth: true }
                                Text { text: modelData.timestamp ?? ""; color: ThemeManager.textSecondary; font.pixelSize: AppStyle.fontSm; font.family: AppStyle.fontFamily; Layout.fillWidth: true }
                            }
                        }

                        Text {
                            visible: parent.count === 0
                            anchors.centerIn: parent
                            text: IECVM.connected ? "No data points received yet.\nSend an Interrogation command."
                                                  : "Connect to an outstation to begin."
                            color: ThemeManager.textDisabled
                            font { pixelSize: AppStyle.fontBase; family: AppStyle.fontFamily }
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
        Text { id: iecToastText; anchors.centerIn: parent; color: "#FFF"; font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily } }
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
        color: isPrimary ? (_ibh.containsMouse ? ThemeManager.accentHover : ThemeManager.accent) : (_ibh.containsMouse ? Qt.rgba(ThemeManager.text.r, ThemeManager.text.g, ThemeManager.text.b, 0.08) : "transparent")
        border { color: isPrimary ? "transparent" : ThemeManager.border; width: 1 }
        Behavior on color { ColorAnimation { duration: AppStyle.animFast } }
        Text { id: _ib; anchors.centerIn: parent; text: label; color: isPrimary ? "#FFF" : ThemeManager.textSecondary; font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily } }
        MouseArea { id: _ibh; anchors.fill: parent; hoverEnabled: true; cursorShape: isEnabled ? Qt.PointingHandCursor : Qt.ArrowCursor; onClicked: if (parent.isEnabled) parent.clicked() }
    }
}
