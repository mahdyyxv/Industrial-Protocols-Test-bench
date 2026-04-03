import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import rtu.ui

// ─────────────────────────────────────────────────────────────
// TopBar
//
// Application header: page title, theme switcher, user info.
// Binds to AppVM — no logic.
// ─────────────────────────────────────────────────────────────
Rectangle {
    id: root

    // ── Public API ────────────────────────────────────────────
    property string pageTitle:  "Dashboard"
    property string userId:     ""
    property string tierName:   "Free"
    property bool   isPro:      false

    signal themeChangeRequested(string themeName)
    signal logoutRequested()

    implicitWidth:  parent ? parent.width : 800
    implicitHeight: AppStyle.topBarHeight
    color:          ThemeManager.topBarBg

    // Bottom border
    Rectangle {
        anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
        height: 1
        color:  ThemeManager.border
    }

    RowLayout {
        anchors {
            fill:           parent
            leftMargin:     AppStyle.spaceLg
            rightMargin:    AppStyle.spaceMd
        }
        spacing: AppStyle.spaceMd

        // ── Page title ────────────────────────────────────
        Text {
            text:  root.pageTitle
            color: ThemeManager.text
            font {
                pixelSize: AppStyle.fontLg
                family:    AppStyle.fontFamily
                weight:    600
            }
        }

        Item { Layout.fillWidth: true }   // spacer

        // ── Theme selector ────────────────────────────────
        Item {
            id: themeSelector
            implicitWidth:  themeButton.implicitWidth
            implicitHeight: AppStyle.buttonHeightSm
            property bool menuOpen: false

            // Theme toggle button
            Rectangle {
                id: themeButton
                implicitWidth:  themeBtnLabel.implicitWidth + AppStyle.spaceLg * 2
                implicitHeight: AppStyle.buttonHeightSm
                radius: AppStyle.radiusMd
                color:  themeSelector.menuOpen || themeBtnHover.containsMouse
                            ? ThemeManager.accentSubtle
                            : "transparent"
                border { color: ThemeManager.border; width: 1 }

                Behavior on color { ColorAnimation { duration: AppStyle.animFast } }

                Text {
                    id: themeBtnLabel
                    anchors.centerIn: parent
                    text:  "◑  " + ThemeManager.currentTheme
                    color: ThemeManager.textSecondary
                    font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily }
                }

                MouseArea {
                    id: themeBtnHover
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape:  Qt.PointingHandCursor
                    onClicked:    themeSelector.menuOpen = !themeSelector.menuOpen
                }
            }

            // Dropdown menu
            Rectangle {
                id: themeMenu
                anchors { top: themeButton.bottom; topMargin: AppStyle.spaceXs; right: themeButton.right }
                width:   160
                radius:  AppStyle.radiusMd
                color:   ThemeManager.surface
                border { color: ThemeManager.border; width: 1 }
                visible: themeSelector.menuOpen
                z:       999

                // Dismiss on outside click
                Popup {
                    id: dismissPopup
                    visible: themeSelector.menuOpen
                    modal: true
                    background: Item {}
                    onClosed: themeSelector.menuOpen = false
                }

                Column {
                    anchors { fill: parent; margins: AppStyle.spaceXs }
                    spacing: 0

                    Repeater {
                        model: ThemeManager.availableThemes

                        delegate: Item {
                            width:  parent.width
                            height: AppStyle.buttonHeightSm + AppStyle.spaceXxs * 2
                            property bool hovered: false

                            Rectangle {
                                anchors { fill: parent; margins: AppStyle.spaceXxs }
                                radius: AppStyle.radiusSm
                                color: {
                                    if (ThemeManager.currentTheme === modelData)
                                        return ThemeManager.accentSubtle
                                    if (parent.hovered)
                                        return Qt.rgba(ThemeManager.text.r,
                                                       ThemeManager.text.g,
                                                       ThemeManager.text.b, 0.06)
                                    return "transparent"
                                }
                                Behavior on color { ColorAnimation { duration: AppStyle.animFast } }
                            }

                            Text {
                                anchors {
                                    left:           parent.left
                                    leftMargin:     AppStyle.spaceMd
                                    verticalCenter: parent.verticalCenter
                                }
                                text:  modelData
                                color: ThemeManager.currentTheme === modelData
                                           ? ThemeManager.accent
                                           : ThemeManager.text
                                font { pixelSize: AppStyle.fontBase; family: AppStyle.fontFamily }
                            }

                            MouseArea {
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape:  Qt.PointingHandCursor
                                onEntered:    parent.hovered = true
                                onExited:     parent.hovered = false
                                onClicked: {
                                    root.themeChangeRequested(modelData)
                                    themeSelector.menuOpen = false
                                }
                            }
                        }
                    }
                }
            }
        }

        // ── Tier badge ────────────────────────────────────
        Rectangle {
            implicitWidth:  tierLabel.implicitWidth + AppStyle.spaceLg
            implicitHeight: AppStyle.buttonHeightSm
            radius: AppStyle.radiusFull
            color:  root.isPro
                        ? ThemeManager.proGoldSubtle
                        : ThemeManager.accentSubtle
            border {
                color: root.isPro ? ThemeManager.proGold : ThemeManager.accent
                width: 1
            }

            Text {
                id: tierLabel
                anchors.centerIn: parent
                text:  root.isPro ? "★  Pro" : "Free"
                color: root.isPro ? ThemeManager.proGold : ThemeManager.accent
                font {
                    pixelSize: AppStyle.fontSm
                    family:    AppStyle.fontFamily
                    weight:    500
                }
            }
        }

        // ── User menu ─────────────────────────────────────
        Item {
            id: userMenu
            implicitWidth:  userBtn.implicitWidth
            implicitHeight: AppStyle.buttonHeightSm
            property bool menuOpen: false

            Rectangle {
                id: userBtn
                implicitWidth:  userLabel.implicitWidth + AppStyle.spaceLg * 2
                implicitHeight: AppStyle.buttonHeightSm
                radius: AppStyle.radiusMd
                color:  userMenu.menuOpen || userHover.containsMouse
                            ? ThemeManager.accentSubtle
                            : "transparent"
                border { color: ThemeManager.border; width: 1 }
                Behavior on color { ColorAnimation { duration: AppStyle.animFast } }

                Text {
                    id: userLabel
                    anchors.centerIn: parent
                    text:  Icons.user + "  " + (root.userId || "Account")
                    color: ThemeManager.textSecondary
                    font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily }
                }

                MouseArea {
                    id: userHover
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape:  Qt.PointingHandCursor
                    onClicked:    userMenu.menuOpen = !userMenu.menuOpen
                }
            }

            // User dropdown
            Rectangle {
                anchors { top: userBtn.bottom; topMargin: AppStyle.spaceXs; right: userBtn.right }
                width:   160
                radius:  AppStyle.radiusMd
                color:   ThemeManager.surface
                border { color: ThemeManager.border; width: 1 }
                visible: userMenu.menuOpen
                z:       999

                Popup {
                    visible: userMenu.menuOpen
                    modal: true
                    background: Item {}
                    onClosed: userMenu.menuOpen = false
                }

                Column {
                    anchors { fill: parent; margins: AppStyle.spaceXs }
                    spacing: 0

                    // Logout
                    Item {
                        width:  parent.width
                        height: AppStyle.buttonHeightSm + AppStyle.spaceXs
                        property bool hovered: false

                        Rectangle {
                            anchors { fill: parent; margins: AppStyle.spaceXxs }
                            radius: AppStyle.radiusSm
                            color:  parent.hovered
                                        ? Qt.rgba(ThemeManager.error.r,
                                                  ThemeManager.error.g,
                                                  ThemeManager.error.b, 0.1)
                                        : "transparent"
                            Behavior on color { ColorAnimation { duration: AppStyle.animFast } }
                        }

                        Text {
                            anchors {
                                left:           parent.left
                                leftMargin:     AppStyle.spaceMd
                                verticalCenter: parent.verticalCenter
                            }
                            text:  "Sign out"
                            color: ThemeManager.error
                            font { pixelSize: AppStyle.fontBase; family: AppStyle.fontFamily }
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape:  Qt.PointingHandCursor
                            onEntered:    parent.hovered = true
                            onExited:     parent.hovered = false
                            onClicked: {
                                userMenu.menuOpen = false
                                root.logoutRequested()
                            }
                        }
                    }
                }
            }
        }
    }
}
