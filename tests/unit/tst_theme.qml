import QtQuick
import QtTest
import rtu.ui

// ─────────────────────────────────────────────────────────────
// tst_theme.qml
//
// QML unit tests for ThemeManager theme switching.
// Run via tst_ui_runner
// ─────────────────────────────────────────────────────────────
TestCase {
    id: root
    name: "ThemeManagerTests"

    // ── Theme switching ───────────────────────────────────────
    function test_defaultThemeIsDark() {
        // After first run (no persisted setting) default is Dark
        compare(ThemeManager.isDark, true)
    }

    function test_switchToLight() {
        ThemeManager.setTheme("Light")
        compare(ThemeManager.currentTheme, "Light")
        compare(ThemeManager.isDark, false)
        // Verify a light-theme token
        verify(ThemeManager.background !== "#1A1B26",
               "Light background must differ from Dark")
    }

    function test_switchToMonokai() {
        ThemeManager.setTheme("Monokai")
        compare(ThemeManager.currentTheme, "Monokai")
        compare(ThemeManager.isDark, true)
    }

    function test_switchToSolarizedDark() {
        ThemeManager.setTheme("SolarizedDark")
        compare(ThemeManager.currentTheme, "SolarizedDark")
        compare(ThemeManager.isDark, true)
    }

    function test_switchToSolarizedLight() {
        ThemeManager.setTheme("SolarizedLight")
        compare(ThemeManager.currentTheme, "SolarizedLight")
        compare(ThemeManager.isDark, false)
    }

    function test_switchBackToDark() {
        ThemeManager.setTheme("Dark")
        compare(ThemeManager.currentTheme, "Dark")
        compare(ThemeManager.isDark, true)
    }

    // ── Token availability ────────────────────────────────────
    function test_tokensAreDefinedForAllThemes() {
        const themes = ThemeManager.availableThemes
        for (let i = 0; i < themes.length; i++) {
            ThemeManager.setTheme(themes[i])
            verify(ThemeManager.background   !== undefined, themes[i] + ".background missing")
            verify(ThemeManager.surface      !== undefined, themes[i] + ".surface missing")
            verify(ThemeManager.accent       !== undefined, themes[i] + ".accent missing")
            verify(ThemeManager.text         !== undefined, themes[i] + ".text missing")
            verify(ThemeManager.error        !== undefined, themes[i] + ".error missing")
        }
        ThemeManager.setTheme("Dark")
    }

    function test_availableThemesCount() {
        compare(ThemeManager.availableThemes.length, 5)
    }

    // ── Token reactivity (regression for indirect-binding bug) ──
    function test_backgroundChangesOnThemeSwitch() {
        ThemeManager.setTheme("Dark")
        const darkBg = ThemeManager.background.toString()
        ThemeManager.setTheme("Light")
        const lightBg = ThemeManager.background.toString()
        verify(darkBg !== lightBg,
               "background token must change when theme switches")
        ThemeManager.setTheme("Dark")
    }

    function test_accentChangesOnThemeSwitch() {
        ThemeManager.setTheme("Dark")
        const darkAccent = ThemeManager.accent.toString()
        ThemeManager.setTheme("Monokai")
        const monokaiAccent = ThemeManager.accent.toString()
        verify(darkAccent !== monokaiAccent,
               "accent token must change between Dark and Monokai")
        ThemeManager.setTheme("Dark")
    }
}
