import QtQuick
import QtQuick.Controls
import rtu.ui

// ─────────────────────────────────────────────────────────────
// NavItem
//
// Single navigation entry in the Sidebar.
// Handles hover, active state, Pro-lock indicator.
// ─────────────────────────────────────────────────────────────
Item {
    id: root

    property string label:      ""
    property string icon:       ""
    property bool   active:     false
    property bool   requiresPro: false
    property bool   collapsed:  false    // sidebar collapse state
    property bool   isPro:      false    // from AppVM

    signal clicked()

    implicitWidth:  parent ? parent.width : 200
    implicitHeight: AppStyle.buttonHeight + AppStyle.spaceXs

    // ── Lock state ────────────────────────────────────────────
    readonly property bool locked: requiresPro && !isPro

    // ── Hover state ───────────────────────────────────────────
    property bool hovered: false

    // ── Background ───────────────────────────────────────────
    Rectangle {
        id: bg
        anchors {
            fill:        parent
            leftMargin:  AppStyle.spaceXs
            rightMargin: AppStyle.spaceXs
            topMargin:   AppStyle.spaceXxs
            bottomMargin:AppStyle.spaceXxs
        }
        radius: AppStyle.radiusMd
        color: {
            if (root.active)   return ThemeManager.accentSubtle
            if (root.hovered)  return Qt.rgba(
                ThemeManager.text.r,
                ThemeManager.text.g,
                ThemeManager.text.b, 0.07)
            return "transparent"
        }

        Behavior on color { ColorAnimation { duration: AppStyle.animFast } }

        // Active left indicator bar
        Rectangle {
            anchors {
                left:   parent.left
                top:    parent.top
                bottom: parent.bottom
            }
            width:  3
            radius: AppStyle.radiusFull
            color:  ThemeManager.accent
            opacity: root.active ? 1 : 0

            Behavior on opacity {
                NumberAnimation { duration: AppStyle.animFast }
            }
        }

        // ── Icon ──────────────────────────────────────────────
        Text {
            id: iconText
            anchors {
                left:           parent.left
                leftMargin:     AppStyle.spaceMd
                verticalCenter: parent.verticalCenter
            }
            text:  root.icon
            color: root.active  ? ThemeManager.accent :
                   root.locked  ? ThemeManager.textDisabled :
                                  ThemeManager.textSecondary
            font {
                pixelSize: AppStyle.iconSizeLg
                family:    AppStyle.fontFamily
            }

            Behavior on color { ColorAnimation { duration: AppStyle.animFast } }
        }

        // ── Label (hidden when collapsed) ────────────────────
        Text {
            id: labelText
            anchors {
                left:           iconText.right
                leftMargin:     AppStyle.spaceMd
                right:          lockIcon.left
                rightMargin:    AppStyle.spaceSm
                verticalCenter: parent.verticalCenter
            }
            text:  root.label
            color: root.active  ? ThemeManager.text :
                   root.locked  ? ThemeManager.textDisabled :
                                  ThemeManager.textSecondary
            font {
                pixelSize: AppStyle.fontBase
                family:    AppStyle.fontFamily
                weight:    root.active ? Font.Medium : Font.Normal
            }
            elide: Text.ElideRight
            opacity: root.collapsed ? 0 : 1

            Behavior on opacity {
                NumberAnimation { duration: AppStyle.animFast }
            }
        }

        // ── Pro lock icon ─────────────────────────────────────
        Text {
            id: lockIcon
            anchors {
                right:          parent.right
                rightMargin:    AppStyle.spaceMd
                verticalCenter: parent.verticalCenter
            }
            text:      Icons.lock
            font.pixelSize: AppStyle.fontSm
            color:     ThemeManager.textDisabled
            visible:   root.locked && !root.collapsed
        }
    }

    // ── Mouse interaction ─────────────────────────────────────
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape:  root.locked ? Qt.ForbiddenCursor : Qt.PointingHandCursor

        onEntered:  root.hovered = true
        onExited:   root.hovered = false

        onClicked: root.clicked()
    }

    // ── Tooltip when collapsed ────────────────────────────────
    ToolTip {
        visible:  root.collapsed && root.hovered
        text:     root.locked ? root.label + " (Pro)" : root.label
        delay:    400
        timeout:  3000

        background: Rectangle {
            color:  ThemeManager.surface
            border.color: ThemeManager.border
            radius: AppStyle.radiusSm
        }
        contentItem: Text {
            text:  root.locked ? root.label + " (Pro)" : root.label
            color: ThemeManager.text
            font.pixelSize: AppStyle.fontSm
        }
    }
}
