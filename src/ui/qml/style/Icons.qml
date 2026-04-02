pragma Singleton
import QtQuick

// ─────────────────────────────────────────────────────────────
// Icons  (Singleton)
//
// Unicode icon map — placeholder until a proper icon font
// (e.g. Phosphor Icons .ttf) is bundled in resources.
//
// To upgrade: replace unicode strings with the codepoint
// strings from your icon font and set fontFamily in Text.
// ─────────────────────────────────────────────────────────────
QtObject {
    // Navigation
    readonly property string dashboard:  "⊞"
    readonly property string protocols:  "⇄"
    readonly property string tools:      "⚙"
    readonly property string settings:   "⚙"
    readonly property string user:       "◉"

    // Protocol icons
    readonly property string modbus:     "M"
    readonly property string dnp3:       "D"
    readonly property string iec101:     "101"
    readonly property string iec104:     "104"
    readonly property string serial:     "∿"

    // Tool icons
    readonly property string analyzer:   "≋"
    readonly property string simulator:  "⚡"
    readonly property string log:        "≡"

    // Status
    readonly property string connected:  "●"
    readonly property string disconnected: "○"
    readonly property string warning:    "⚠"
    readonly property string error:      "✖"
    readonly property string success:    "✔"
    readonly property string lock:       "🔒"
    readonly property string crown:      "★"

    // Actions
    readonly property string play:       "▶"
    readonly property string stop:       "■"
    readonly property string pause:      "⏸"
    readonly property string send:       "➤"
    readonly property string clear:      "⌫"
    readonly property string export_:    "↑"
    readonly property string copy:       "⎘"
    readonly property string collapse:   "◀"
    readonly property string expand:     "▶"
    readonly property string close:      "✕"
    readonly property string plus:       "+"
    readonly property string refresh:    "↺"
}
