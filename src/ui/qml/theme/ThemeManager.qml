pragma Singleton
import QtQuick
import QtCore

// ─────────────────────────────────────────────────────────────
// ThemeManager  (Singleton)
//
// The single source of truth for all color tokens.
// Usage:  import rtu.ui
//         color: ThemeManager.background
//
// Theme names: "Dark" | "Light" | "Monokai" |
//              "SolarizedDark" | "SolarizedLight"
// ─────────────────────────────────────────────────────────────
QtObject {
    id: root

    // ── Persistence ──────────────────────────────────────────
    readonly property var _store: Settings {
        category: "ui"
        property string themeName: "Dark"
    }

    property string currentTheme: _store.themeName

    function setTheme(name) {
        _store.themeName = name
        currentTheme = name
    }

    readonly property var availableThemes: [
        "Dark", "Light", "Monokai", "SolarizedDark", "SolarizedLight"
    ]

    // ── Active palette (resolved) ─────────────────────────────
    readonly property var _p: {
        switch (currentTheme) {
        case "Light":          return _light
        case "Monokai":        return _monokai
        case "SolarizedDark":  return _solarizedDark
        case "SolarizedLight": return _solarizedLight
        default:               return _dark
        }
    }

    // ── Semantic tokens ───────────────────────────────────────
    readonly property color background:     _p.background
    readonly property color surface:        _p.surface
    readonly property color surfaceAlt:     _p.surfaceAlt
    readonly property color sidebarBg:      _p.sidebar
    readonly property color topBarBg:       _p.topBar
    readonly property color accent:         _p.accent
    readonly property color accentHover:    Qt.lighter(_p.accent, 1.18)
    readonly property color accentSubtle:   Qt.rgba(_p.accent.r, _p.accent.g, _p.accent.b, 0.15)
    readonly property color text:           _p.text
    readonly property color textSecondary:  _p.textSec
    readonly property color textDisabled:   _p.textDis
    readonly property color border:         _p.border
    readonly property color borderFocus:    _p.accent
    readonly property color success:        _p.success
    readonly property color error:          _p.error
    readonly property color warning:        _p.warning
    readonly property color proGold:        "#F6C94E"
    readonly property color proGoldSubtle:  "#2A240E"

    // ── Helper: isDark ────────────────────────────────────────
    readonly property bool isDark: (currentTheme === "Dark" ||
                                    currentTheme === "Monokai" ||
                                    currentTheme === "SolarizedDark")

    // ════════════════════════════════════════════════════════
    //  PALETTE DEFINITIONS
    // ════════════════════════════════════════════════════════

    // ── Dark (Tokyo Night inspired) ───────────────────────────
    readonly property var _dark: QtObject {
        readonly property color background: "#1A1B26"
        readonly property color surface:    "#24283B"
        readonly property color surfaceAlt: "#2D3149"
        readonly property color sidebar:    "#16161E"
        readonly property color topBar:     "#1F2335"
        readonly property color accent:     "#7AA2F7"
        readonly property color text:       "#C0CAF5"
        readonly property color textSec:    "#565F89"
        readonly property color textDis:    "#3B4261"
        readonly property color border:     "#2A2B3D"
        readonly property color success:    "#9ECE6A"
        readonly property color error:      "#F7768E"
        readonly property color warning:    "#E0AF68"
    }

    // ── Light ────────────────────────────────────────────────
    readonly property var _light: QtObject {
        readonly property color background: "#F0F2F5"
        readonly property color surface:    "#FFFFFF"
        readonly property color surfaceAlt: "#F8FAFC"
        readonly property color sidebar:    "#E2E8F0"
        readonly property color topBar:     "#FFFFFF"
        readonly property color accent:     "#4361EE"
        readonly property color text:       "#1E2030"
        readonly property color textSec:    "#6B7280"
        readonly property color textDis:    "#C4CAD4"
        readonly property color border:     "#D1D5DB"
        readonly property color success:    "#22C55E"
        readonly property color error:      "#EF4444"
        readonly property color warning:    "#F59E0B"
    }

    // ── Monokai ───────────────────────────────────────────────
    readonly property var _monokai: QtObject {
        readonly property color background: "#272822"
        readonly property color surface:    "#3E3D32"
        readonly property color surfaceAlt: "#49483E"
        readonly property color sidebar:    "#1E1F1A"
        readonly property color topBar:     "#2D2E27"
        readonly property color accent:     "#A6E22E"
        readonly property color text:       "#F8F8F2"
        readonly property color textSec:    "#75715E"
        readonly property color textDis:    "#4E4D44"
        readonly property color border:     "#49483E"
        readonly property color success:    "#A6E22E"
        readonly property color error:      "#F92672"
        readonly property color warning:    "#FD971F"
    }

    // ── Solarized Dark ────────────────────────────────────────
    readonly property var _solarizedDark: QtObject {
        readonly property color background: "#002B36"
        readonly property color surface:    "#073642"
        readonly property color surfaceAlt: "#0D3D4C"
        readonly property color sidebar:    "#001F26"
        readonly property color topBar:     "#073642"
        readonly property color accent:     "#268BD2"
        readonly property color text:       "#839496"
        readonly property color textSec:    "#586E75"
        readonly property color textDis:    "#2A3F47"
        readonly property color border:     "#0D4050"
        readonly property color success:    "#859900"
        readonly property color error:      "#DC322F"
        readonly property color warning:    "#CB4B16"
    }

    // ── Solarized Light ───────────────────────────────────────
    readonly property var _solarizedLight: QtObject {
        readonly property color background: "#FDF6E3"
        readonly property color surface:    "#EEE8D5"
        readonly property color surfaceAlt: "#E8E2CF"
        readonly property color sidebar:    "#DDD8C8"
        readonly property color topBar:     "#EEE8D5"
        readonly property color accent:     "#268BD2"
        readonly property color text:       "#657B83"
        readonly property color textSec:    "#93A1A1"
        readonly property color textDis:    "#C9C4B4"
        readonly property color border:     "#D3CBB4"
        readonly property color success:    "#859900"
        readonly property color error:      "#DC322F"
        readonly property color warning:    "#CB4B16"
    }
}
