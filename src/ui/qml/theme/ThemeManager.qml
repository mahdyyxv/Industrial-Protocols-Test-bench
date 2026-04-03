pragma Singleton
import QtQuick
import QtCore

// ─────────────────────────────────────────────────────────────
// ThemeManager  (Singleton)
//
// Single source of truth for all color tokens.
// Each token is a direct binding on currentTheme so the QML
// binding engine re-evaluates every property independently
// when the theme changes — no intermediate QtObject indirection.
//
// Usage:  import rtu.ui
//         color: ThemeManager.background
// ─────────────────────────────────────────────────────────────
QtObject {
    id: root

    // ── Persistence ──────────────────────────────────────────
    property Settings _store: Settings {
        category: "ui"
        property string themeName: "Dark"
    }

    property string currentTheme: _store.themeName

    function setTheme(name) {
        currentTheme = name
        _store.themeName = name
    }

    readonly property var availableThemes: [
        "Dark", "Light", "Monokai", "SolarizedDark", "SolarizedLight"
    ]

    // ── isDark helper ─────────────────────────────────────────
    readonly property bool isDark: currentTheme === "Dark"     ||
                                   currentTheme === "Monokai"  ||
                                   currentTheme === "SolarizedDark"

    // ════════════════════════════════════════════════════════
    //  COLOR TOKENS  (each binding re-evaluates on currentTheme)
    // ════════════════════════════════════════════════════════

    readonly property color background: {
        switch (currentTheme) {
        case "Light":          return "#F0F2F5"
        case "Monokai":        return "#272822"
        case "SolarizedDark":  return "#002B36"
        case "SolarizedLight": return "#FDF6E3"
        default:               return "#1A1B26"   // Dark
        }
    }

    readonly property color surface: {
        switch (currentTheme) {
        case "Light":          return "#FFFFFF"
        case "Monokai":        return "#3E3D32"
        case "SolarizedDark":  return "#073642"
        case "SolarizedLight": return "#EEE8D5"
        default:               return "#24283B"
        }
    }

    readonly property color surfaceAlt: {
        switch (currentTheme) {
        case "Light":          return "#F8FAFC"
        case "Monokai":        return "#49483E"
        case "SolarizedDark":  return "#0D3D4C"
        case "SolarizedLight": return "#E8E2CF"
        default:               return "#2D3149"
        }
    }

    readonly property color sidebarBg: {
        switch (currentTheme) {
        case "Light":          return "#E2E8F0"
        case "Monokai":        return "#1E1F1A"
        case "SolarizedDark":  return "#001F26"
        case "SolarizedLight": return "#DDD8C8"
        default:               return "#16161E"
        }
    }

    readonly property color topBarBg: {
        switch (currentTheme) {
        case "Light":          return "#FFFFFF"
        case "Monokai":        return "#2D2E27"
        case "SolarizedDark":  return "#073642"
        case "SolarizedLight": return "#EEE8D5"
        default:               return "#1F2335"
        }
    }

    readonly property color accent: {
        switch (currentTheme) {
        case "Light":          return "#4361EE"
        case "Monokai":        return "#A6E22E"
        case "SolarizedDark":  return "#268BD2"
        case "SolarizedLight": return "#268BD2"
        default:               return "#7AA2F7"
        }
    }

    readonly property color accentHover:   Qt.lighter(accent, 1.18)
    readonly property color accentSubtle:  Qt.rgba(accent.r, accent.g, accent.b, 0.15)

    readonly property color text: {
        switch (currentTheme) {
        case "Light":          return "#1E2030"
        case "Monokai":        return "#F8F8F2"
        case "SolarizedDark":  return "#839496"
        case "SolarizedLight": return "#657B83"
        default:               return "#C0CAF5"
        }
    }

    readonly property color textSecondary: {
        switch (currentTheme) {
        case "Light":          return "#6B7280"
        case "Monokai":        return "#75715E"
        case "SolarizedDark":  return "#586E75"
        case "SolarizedLight": return "#93A1A1"
        default:               return "#565F89"
        }
    }

    readonly property color textDisabled: {
        switch (currentTheme) {
        case "Light":          return "#C4CAD4"
        case "Monokai":        return "#4E4D44"
        case "SolarizedDark":  return "#2A3F47"
        case "SolarizedLight": return "#C9C4B4"
        default:               return "#3B4261"
        }
    }

    readonly property color border: {
        switch (currentTheme) {
        case "Light":          return "#D1D5DB"
        case "Monokai":        return "#49483E"
        case "SolarizedDark":  return "#0D4050"
        case "SolarizedLight": return "#D3CBB4"
        default:               return "#2A2B3D"
        }
    }

    readonly property color borderFocus: accent

    readonly property color success: {
        switch (currentTheme) {
        case "Light":          return "#22C55E"
        case "Monokai":        return "#A6E22E"
        case "SolarizedDark":  return "#859900"
        case "SolarizedLight": return "#859900"
        default:               return "#9ECE6A"
        }
    }

    readonly property color error: {
        switch (currentTheme) {
        case "Light":          return "#EF4444"
        case "Monokai":        return "#F92672"
        case "SolarizedDark":  return "#DC322F"
        case "SolarizedLight": return "#DC322F"
        default:               return "#F7768E"
        }
    }

    readonly property color warning: {
        switch (currentTheme) {
        case "Light":          return "#F59E0B"
        case "Monokai":        return "#FD971F"
        case "SolarizedDark":  return "#CB4B16"
        case "SolarizedLight": return "#CB4B16"
        default:               return "#E0AF68"
        }
    }

    readonly property color proGold:       "#F6C94E"
    readonly property color proGoldSubtle: "#2A240E"
}
