import QtQuick
import rtu.ui

// A subtle section header in the Sidebar
Item {
    property string text: ""
    property bool   collapsed: false

    implicitWidth:  parent ? parent.width : 200
    implicitHeight: AppStyle.spaceLg + AppStyle.spaceSm

    Text {
        anchors {
            left:           parent.left
            leftMargin:     AppStyle.spaceMd + AppStyle.spaceXs
            verticalCenter: parent.verticalCenter
        }
        text:  parent.text.toUpperCase()
        color: ThemeManager.textDisabled
        font {
            pixelSize: AppStyle.fontXs
            family:    AppStyle.fontFamily
            weight:    500
            letterSpacing: 0.8
        }
        opacity: parent.collapsed ? 0 : 1
        Behavior on opacity { NumberAnimation { duration: AppStyle.animFast } }
    }

    // Thin line shown when collapsed
    Rectangle {
        anchors {
            left:           parent.left
            right:          parent.right
            leftMargin:     AppStyle.spaceMd
            rightMargin:    AppStyle.spaceMd
            verticalCenter: parent.verticalCenter
        }
        height:  1
        color:   ThemeManager.border
        opacity: parent.collapsed ? 0.6 : 0
        Behavior on opacity { NumberAnimation { duration: AppStyle.animFast } }
    }
}
