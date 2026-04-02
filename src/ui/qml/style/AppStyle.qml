pragma Singleton
import QtQuick

// ─────────────────────────────────────────────────────────────
// AppStyle  (Singleton)
//
// Layout constants, typography scale, animation durations.
// No colors here — use ThemeManager for all colors.
// ─────────────────────────────────────────────────────────────
QtObject {

    // ── Typography ────────────────────────────────────────────
    readonly property int fontXs:   11
    readonly property int fontSm:   12
    readonly property int fontBase: 13
    readonly property int fontMd:   14
    readonly property int fontLg:   16
    readonly property int fontXl:   20
    readonly property int font2xl:  24
    readonly property int font3xl:  32

    readonly property string fontFamily: "Segoe UI"          // Windows default
    readonly property string fontMono:   "Cascadia Code, Consolas, monospace"

    // ── Spacing ───────────────────────────────────────────────
    readonly property int spaceXxs:  2
    readonly property int spaceXs:   4
    readonly property int spaceSm:   8
    readonly property int spaceMd:  16
    readonly property int spaceLg:  24
    readonly property int spaceXl:  32
    readonly property int space2xl: 48

    // ── Border radius ─────────────────────────────────────────
    readonly property int radiusSm:  4
    readonly property int radiusMd:  8
    readonly property int radiusLg: 12
    readonly property int radiusXl: 16
    readonly property int radiusFull: 999

    // ── Layout ────────────────────────────────────────────────
    readonly property int topBarHeight:       56
    readonly property int sidebarWidthFull:  240
    readonly property int sidebarWidthCollapsed: 60
    readonly property int contentPadding:     24

    // ── Animation durations (ms) ──────────────────────────────
    readonly property int animFast:   120
    readonly property int animNormal: 220
    readonly property int animSlow:   380

    readonly property int animEasing: Easing.OutCubic

    // ── Elevation (shadow helpers) ────────────────────────────
    // Usage: layer.enabled + layer.effect (DropShadow)
    readonly property real shadowSm: 4
    readonly property real shadowMd: 8
    readonly property real shadowLg: 16

    // ── Input / Control sizes ─────────────────────────────────
    readonly property int inputHeight:   40
    readonly property int buttonHeight:  40
    readonly property int buttonHeightSm: 32
    readonly property int iconSize:      18
    readonly property int iconSizeLg:    24
}
