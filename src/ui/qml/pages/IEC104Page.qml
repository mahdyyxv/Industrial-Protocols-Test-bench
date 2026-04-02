import QtQuick
import QtQuick.Layouts
import rtu.ui

// ─────────────────────────────────────────────────────────────
// IEC104Page  (Pro feature)
//
// Placeholder for IEC 60870-5-104 / DNP3 / IEC-101 testing UI.
// Feature-gated behind ProLockOverlay.
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

        // ── Page body (shown only when Pro) ──────────────
        Rectangle {
            anchors.fill:    parent
            color:           ThemeManager.background

            ColumnLayout {
                anchors { fill: parent; margins: AppStyle.contentPadding }
                spacing: AppStyle.spaceLg

                // Header
                Text {
                    text:  _titles[pageId] ?? "Protocol"
                    color: ThemeManager.text
                    font {
                        pixelSize: AppStyle.fontXl
                        family:    AppStyle.fontFamily
                        weight:    Font.SemiBold
                    }
                }

                // TODO (Core Engineer): wire IEC104 / DNP3 session UI here
                Rectangle {
                    Layout.fillWidth:  true
                    Layout.fillHeight: true
                    radius: AppStyle.radiusLg
                    color:  ThemeManager.surface
                    border { color: ThemeManager.border; width: 1 }

                    Text {
                        anchors.centerIn: parent
                        text:  "IEC / DNP3 test interface\n— implemented in Core Engineer phase"
                        color: ThemeManager.textSecondary
                        font { pixelSize: AppStyle.fontBase; family: AppStyle.fontFamily }
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }
        }
    }
}
