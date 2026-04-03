import QtQuick
import rtu.ui

// Small inline status badge: Connected / Disconnected / Error
Item {
    property string status: "disconnected"   // "connected" | "disconnected" | "error"
    property string label:  ""               // optional override

    implicitWidth:  badgeText.implicitWidth + AppStyle.spaceLg
    implicitHeight: 22

    readonly property color _color: {
        switch (status) {
        case "connected":    return ThemeManager.success
        case "error":        return ThemeManager.error
        default:             return ThemeManager.textDisabled
        }
    }
    readonly property string _label: label !== "" ? label : status.charAt(0).toUpperCase() + status.slice(1)

    Rectangle {
        anchors.fill: parent
        radius: AppStyle.radiusFull
        color:  Qt.rgba(_color.r, _color.g, _color.b, 0.15)
        border { color: Qt.rgba(_color.r, _color.g, _color.b, 0.4); width: 1 }
    }

    Row {
        anchors.centerIn: parent
        spacing: AppStyle.spaceXs

        Rectangle {
            width: 6; height: 6
            radius: AppStyle.radiusFull
            color:  _color
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            id: badgeText
            text:  _label
            color: _color
            font { pixelSize: AppStyle.fontXs; family: AppStyle.fontFamily; weight: 500 }
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
