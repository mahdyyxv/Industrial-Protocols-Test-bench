import QtQuick
import QtQuick.Layouts
import rtu.ui

// ─────────────────────────────────────────────────────────────
// ProLockOverlay
//
// Wrap any page with this to gate Pro features.
// Shows upgrade CTA when user is on Free tier.
// ─────────────────────────────────────────────────────────────
Item {
    id: root

    property bool   requiresPro: true
    property bool   isPro:       false
    property string featureName: "This feature"

    signal upgradeRequested()

    // ── Slot for the actual page content ─────────────────────
    default property alias pageContent: contentSlot.data

    anchors.fill: parent ? parent : undefined

    // Actual page content (shown only when unlocked)
    Item {
        id: contentSlot
        anchors.fill: parent
        visible: !root.requiresPro || root.isPro
    }

    // Upgrade wall (shown when locked)
    Rectangle {
        anchors.fill: parent
        visible: root.requiresPro && !root.isPro
        color:   ThemeManager.background

        ColumnLayout {
            anchors.centerIn: parent
            spacing: AppStyle.spaceLg
            width: Math.min(parent.width * 0.6, 420)

            // Lock icon
            Text {
                Layout.alignment: Qt.AlignHCenter
                text:  "🔒"
                font.pixelSize: 48
            }

            // Title
            Text {
                Layout.alignment:    Qt.AlignHCenter
                text:                root.featureName + " — Pro Only"
                color:               ThemeManager.text
                font {
                    pixelSize: AppStyle.fontXl
                    family:    AppStyle.fontFamily
                    weight:    600
                }
                horizontalAlignment: Text.AlignHCenter
                wrapMode:            Text.WordWrap
                Layout.fillWidth:    true
            }

            // Description
            Text {
                Layout.alignment:    Qt.AlignHCenter
                text:                "Upgrade to Pro to unlock advanced protocols, " +
                                     "automation, and analysis tools."
                color:               ThemeManager.textSecondary
                font { pixelSize: AppStyle.fontBase; family: AppStyle.fontFamily }
                horizontalAlignment: Text.AlignHCenter
                wrapMode:            Text.WordWrap
                Layout.fillWidth:    true
            }

            // Upgrade button
            Rectangle {
                Layout.alignment:       Qt.AlignHCenter
                implicitWidth:          upgradeLabel.implicitWidth + AppStyle.spaceXl * 2
                implicitHeight:         AppStyle.buttonHeight + AppStyle.spaceXs
                radius:                 AppStyle.radiusMd
                color:                  upgradeHover.containsMouse
                                            ? ThemeManager.accentHover
                                            : ThemeManager.accent

                Behavior on color { ColorAnimation { duration: AppStyle.animFast } }

                Text {
                    id: upgradeLabel
                    anchors.centerIn: parent
                    text:  "★  Upgrade to Pro"
                    color: "#FFFFFF"
                    font {
                        pixelSize: AppStyle.fontMd
                        family:    AppStyle.fontFamily
                        weight:    500
                    }
                }

                MouseArea {
                    id: upgradeHover
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape:  Qt.PointingHandCursor
                    onClicked:    root.upgradeRequested()
                }
            }
        }
    }
}
