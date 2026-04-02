import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import rtu.ui

// ─────────────────────────────────────────────────────────────
// LoginPage
//
// Authentication form — binds to AppVM only.
// No logic inside.
// ─────────────────────────────────────────────────────────────
Item {
    id: root

    Rectangle {
        anchors.fill: parent
        color: ThemeManager.background
    }

    // ── Error message listener ────────────────────────────────
    property string errorMessage: ""

    Connections {
        target: AppVM
        function onLoginFailed(reason) {
            root.errorMessage = reason
            errorAnim.restart()
        }
    }

    // ── Centered card ─────────────────────────────────────────
    Rectangle {
        id: card
        anchors.centerIn: parent
        width:  420
        height: cardLayout.implicitHeight + AppStyle.space2xl * 2
        radius: AppStyle.radiusXl
        color:  ThemeManager.surface
        border { color: ThemeManager.border; width: 1 }

        ColumnLayout {
            id: cardLayout
            anchors {
                top:     parent.top
                left:    parent.left
                right:   parent.right
                margins: AppStyle.space2xl
            }
            spacing: AppStyle.spaceMd

            // Logo + title
            ColumnLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: AppStyle.spaceSm

                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: 52; height: 52
                    radius: AppStyle.radiusMd
                    color: ThemeManager.accent

                    Text {
                        anchors.centerIn: parent
                        text:  "R"
                        color: "#FFFFFF"
                        font { pixelSize: AppStyle.font2xl; weight: Font.Bold }
                    }
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text:  "RTU Tester"
                    color: ThemeManager.text
                    font {
                        pixelSize: AppStyle.font2xl
                        family:    AppStyle.fontFamily
                        weight:    Font.Bold
                    }
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text:  "Sign in to continue"
                    color: ThemeManager.textSecondary
                    font { pixelSize: AppStyle.fontBase; family: AppStyle.fontFamily }
                }
            }

            // ── Divider ───────────────────────────────────
            Item { Layout.preferredHeight: AppStyle.spaceSm }

            // ── Email field ───────────────────────────────
            ColumnLayout {
                Layout.fillWidth: true
                spacing: AppStyle.spaceXs

                Text {
                    text:  "Email"
                    color: ThemeManager.textSecondary
                    font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: AppStyle.inputHeight
                    radius: AppStyle.radiusMd
                    color:  ThemeManager.background
                    border {
                        color: emailField.activeFocus
                                   ? ThemeManager.borderFocus
                                   : ThemeManager.border
                        width: emailField.activeFocus ? 2 : 1
                    }
                    Behavior on border.color { ColorAnimation { duration: AppStyle.animFast } }

                    TextInput {
                        id: emailField
                        anchors {
                            fill:           parent
                            leftMargin:     AppStyle.spaceMd
                            rightMargin:    AppStyle.spaceMd
                            verticalCenter: parent.verticalCenter
                        }
                        color:        ThemeManager.text
                        font { pixelSize: AppStyle.fontBase; family: AppStyle.fontFamily }
                        inputMethodHints: Qt.ImhEmailCharactersOnly
                        clip: true

                        Text {
                            visible: !parent.text && !parent.activeFocus
                            text:    "you@example.com"
                            color:   ThemeManager.textDisabled
                            font:    parent.font
                            anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                        }

                        Keys.onReturnPressed: passwordField.forceActiveFocus()
                    }
                }
            }

            // ── Password field ────────────────────────────
            ColumnLayout {
                Layout.fillWidth: true
                spacing: AppStyle.spaceXs

                Text {
                    text:  "Password"
                    color: ThemeManager.textSecondary
                    font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: AppStyle.inputHeight
                    radius: AppStyle.radiusMd
                    color:  ThemeManager.background
                    border {
                        color: passwordField.activeFocus
                                   ? ThemeManager.borderFocus
                                   : ThemeManager.border
                        width: passwordField.activeFocus ? 2 : 1
                    }
                    Behavior on border.color { ColorAnimation { duration: AppStyle.animFast } }

                    TextInput {
                        id: passwordField
                        anchors {
                            fill:        parent
                            leftMargin:  AppStyle.spaceMd
                            rightMargin: AppStyle.spaceMd
                        }
                        color:     ThemeManager.text
                        font { pixelSize: AppStyle.fontBase; family: AppStyle.fontFamily }
                        echoMode:  TextInput.Password
                        clip:      true

                        Text {
                            visible: !parent.text && !parent.activeFocus
                            text:    "••••••••"
                            color:   ThemeManager.textDisabled
                            font:    parent.font
                            anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                        }

                        Keys.onReturnPressed: loginBtn.triggerLogin()
                    }
                }
            }

            // ── Error message ─────────────────────────────
            Rectangle {
                Layout.fillWidth: true
                height: errorLabel.implicitHeight + AppStyle.spaceMd
                radius: AppStyle.radiusMd
                color:  Qt.rgba(ThemeManager.error.r,
                                ThemeManager.error.g,
                                ThemeManager.error.b, 0.12)
                border { color: ThemeManager.error; width: 1 }
                visible: root.errorMessage !== ""

                SequentialAnimation {
                    id: errorAnim
                    NumberAnimation { target: card; property: "x"; from: -8; to: 8;  duration: 60 }
                    NumberAnimation { target: card; property: "x"; from:  8; to: -8; duration: 60 }
                    NumberAnimation { target: card; property: "x"; from: -4; to:  4; duration: 50 }
                    NumberAnimation { target: card; property: "x"; from:  4; to:  0; duration: 50 }
                }

                Text {
                    id: errorLabel
                    anchors { fill: parent; margins: AppStyle.spaceSm }
                    text:     root.errorMessage
                    color:    ThemeManager.error
                    font { pixelSize: AppStyle.fontSm; family: AppStyle.fontFamily }
                    wrapMode: Text.WordWrap
                }
            }

            // ── Login button ──────────────────────────────
            Rectangle {
                id: loginBtn
                Layout.fillWidth: true
                height: AppStyle.buttonHeight
                radius: AppStyle.radiusMd
                color: {
                    if (!_canLogin) return ThemeManager.textDisabled
                    if (loginHover.containsMouse) return ThemeManager.accentHover
                    return ThemeManager.accent
                }
                Behavior on color { ColorAnimation { duration: AppStyle.animFast } }

                readonly property bool _canLogin:
                    emailField.text.length > 0 &&
                    passwordField.text.length > 0 &&
                    !_isLoading
                readonly property bool _isLoading: false // bind to AppVM.authState when implemented

                function triggerLogin() {
                    if (!_canLogin) return
                    root.errorMessage = ""
                    AppVM.login(emailField.text, passwordField.text)
                }

                Row {
                    anchors.centerIn: parent
                    spacing: AppStyle.spaceSm

                    // Loading spinner placeholder
                    Rectangle {
                        visible: loginBtn._isLoading
                        width: 14; height: 14
                        radius: AppStyle.radiusFull
                        color: "transparent"
                        border { color: "#FFFFFF"; width: 2 }
                        anchors.verticalCenter: parent.verticalCenter

                        RotationAnimation on rotation {
                            running: loginBtn._isLoading
                            loops:   Animation.Infinite
                            from: 0; to: 360
                            duration: 800
                        }
                    }

                    Text {
                        text:  loginBtn._isLoading ? "Signing in…" : "Sign in"
                        color: "#FFFFFF"
                        font {
                            pixelSize: AppStyle.fontMd
                            family:    AppStyle.fontFamily
                            weight:    Font.Medium
                        }
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                MouseArea {
                    id: loginHover
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape:  loginBtn._canLogin
                                      ? Qt.PointingHandCursor
                                      : Qt.ArrowCursor
                    onClicked:    loginBtn.triggerLogin()
                }
            }

            // Spacer at bottom
            Item { Layout.preferredHeight: AppStyle.spaceSm }
        }
    }
}
