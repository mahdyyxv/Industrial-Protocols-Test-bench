pragma Singleton
import QtQuick
import QtCore

// ─────────────────────────────────────────────────────────────
// ThemeManager  (Singleton)
//
// Each color token uses a ternary chain on currentTheme.
// Ternary expressions guarantee QML's binding engine registers
// currentTheme as a dependency and re-evaluates every token
// when setTheme() is called.
// ─────────────────────────────────────────────────────────────
QtObject {
    id: root

    // ── Persistence ──────────────────────────────────────────
    property Settings _store: Settings {
        category: "ui"
        property string themeName: "Dark"
    }

    // Writable — setTheme() assigns directly (breaks init binding,
    // which is intentional after first load)
    property string currentTheme: _store.themeName

    function setTheme(name) {
        currentTheme   = name
        _store.themeName = name
    }

    readonly property var availableThemes: [
        "Dark", "Light", "Monokai", "SolarizedDark", "SolarizedLight"
    ]

    readonly property bool isDark:
        currentTheme === "Dark"        ||
        currentTheme === "Monokai"     ||
        currentTheme === "SolarizedDark"

    // ── Color tokens ─────────────────────────────────────────
    // Ternary chains — QML tracks currentTheme as a dependency
    // in every expression, so all tokens update atomically.

    readonly property color background:
        currentTheme === "Light"          ? "#F0F2F5" :
        currentTheme === "Monokai"        ? "#272822" :
        currentTheme === "SolarizedDark"  ? "#002B36" :
        currentTheme === "SolarizedLight" ? "#FDF6E3" :
                                            "#1A1B26"   // Dark

    readonly property color surface:
        currentTheme === "Light"          ? "#FFFFFF"  :
        currentTheme === "Monokai"        ? "#3E3D32"  :
        currentTheme === "SolarizedDark"  ? "#073642"  :
        currentTheme === "SolarizedLight" ? "#EEE8D5"  :
                                            "#24283B"

    readonly property color surfaceAlt:
        currentTheme === "Light"          ? "#F8FAFC" :
        currentTheme === "Monokai"        ? "#49483E" :
        currentTheme === "SolarizedDark"  ? "#0D3D4C" :
        currentTheme === "SolarizedLight" ? "#E8E2CF" :
                                            "#2D3149"

    readonly property color sidebarBg:
        currentTheme === "Light"          ? "#E2E8F0" :
        currentTheme === "Monokai"        ? "#1E1F1A" :
        currentTheme === "SolarizedDark"  ? "#001F26" :
        currentTheme === "SolarizedLight" ? "#DDD8C8" :
                                            "#16161E"

    readonly property color topBarBg:
        currentTheme === "Light"          ? "#FFFFFF"  :
        currentTheme === "Monokai"        ? "#2D2E27"  :
        currentTheme === "SolarizedDark"  ? "#073642"  :
        currentTheme === "SolarizedLight" ? "#EEE8D5"  :
                                            "#1F2335"

    readonly property color accent:
        currentTheme === "Light"          ? "#4361EE" :
        currentTheme === "Monokai"        ? "#A6E22E" :
        currentTheme === "SolarizedDark"  ? "#268BD2" :
        currentTheme === "SolarizedLight" ? "#268BD2" :
                                            "#7AA2F7"

    readonly property color accentHover:  Qt.lighter(accent, 1.18)
    readonly property color accentSubtle: Qt.rgba(accent.r, accent.g, accent.b, 0.15)

    readonly property color text:
        currentTheme === "Light"          ? "#1E2030" :
        currentTheme === "Monokai"        ? "#F8F8F2" :
        currentTheme === "SolarizedDark"  ? "#839496" :
        currentTheme === "SolarizedLight" ? "#657B83" :
                                            "#C0CAF5"

    readonly property color textSecondary:
        currentTheme === "Light"          ? "#6B7280" :
        currentTheme === "Monokai"        ? "#75715E" :
        currentTheme === "SolarizedDark"  ? "#586E75" :
        currentTheme === "SolarizedLight" ? "#93A1A1" :
                                            "#565F89"

    readonly property color textDisabled:
        currentTheme === "Light"          ? "#C4CAD4" :
        currentTheme === "Monokai"        ? "#4E4D44" :
        currentTheme === "SolarizedDark"  ? "#2A3F47" :
        currentTheme === "SolarizedLight" ? "#C9C4B4" :
                                            "#3B4261"

    readonly property color border:
        currentTheme === "Light"          ? "#D1D5DB" :
        currentTheme === "Monokai"        ? "#49483E" :
        currentTheme === "SolarizedDark"  ? "#0D4050" :
        currentTheme === "SolarizedLight" ? "#D3CBB4" :
                                            "#2A2B3D"

    readonly property color borderFocus: accent

    readonly property color success:
        currentTheme === "Light"          ? "#22C55E" :
        currentTheme === "Monokai"        ? "#A6E22E" :
        currentTheme === "SolarizedDark"  ? "#859900" :
        currentTheme === "SolarizedLight" ? "#859900" :
                                            "#9ECE6A"

    readonly property color error:
        currentTheme === "Light"          ? "#EF4444" :
        currentTheme === "Monokai"        ? "#F92672" :
        currentTheme === "SolarizedDark"  ? "#DC322F" :
        currentTheme === "SolarizedLight" ? "#DC322F" :
                                            "#F7768E"

    readonly property color warning:
        currentTheme === "Light"          ? "#F59E0B" :
        currentTheme === "Monokai"        ? "#FD971F" :
        currentTheme === "SolarizedDark"  ? "#CB4B16" :
        currentTheme === "SolarizedLight" ? "#CB4B16" :
                                            "#E0AF68"

    readonly property color proGold:       "#F6C94E"
    readonly property color proGoldSubtle: "#2A240E"
}
