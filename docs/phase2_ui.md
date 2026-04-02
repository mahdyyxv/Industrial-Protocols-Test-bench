# Phase 2 — UI Engineer

**Role:** UI ENGINEER  
**Status:** Complete  
**Date:** 2026-04-02

---

## UI Structure

```
src/ui/qml/
├── theme/
│   └── ThemeManager.qml        ← Singleton. All color tokens.
├── style/
│   ├── AppStyle.qml            ← Singleton. Typography, spacing, animation durations.
│   └── Icons.qml               ← Singleton. Unicode icon map (swap for icon font later).
├── components/
│   ├── Sidebar.qml             ← Collapsible nav (240 ↔ 60 px). Emits navigateTo().
│   ├── TopBar.qml              ← Theme picker, tier badge, user menu.
│   ├── NavItem.qml             ← Single nav entry with hover/active/lock states.
│   ├── SidebarGroupLabel.qml   ← Section header in sidebar.
│   ├── ProLockOverlay.qml      ← Wraps any page; shows upgrade CTA for free users.
│   └── StatusBadge.qml         ← Inline connected/disconnected/error pill.
├── pages/
│   ├── DashboardPage.qml       ← Stats cards, quick actions, recent sessions.
│   ├── ModbusPage.qml          ← Modbus RTU/TCP config + request + response + log.
│   ├── IEC104Page.qml          ← Pro-gated shell (IEC-104 / IEC-101 / DNP3).
│   ├── SerialPage.qml          ← Raw serial terminal (free).
│   ├── AnalyzerPage.qml        ← Pro-gated frame capture table.
│   └── SimulatorPage.qml       ← Pro-gated RTU simulator + register map.
├── AppShell.qml                ← Sidebar + TopBar + StackView. Owns routing.
├── LoginPage.qml               ← Auth form. Binds to AppVM.login() / loginFailed.
└── main.qml                    ← ApplicationWindow. Routes login ↔ shell. Kb shortcuts.
```

---

## Theme System

### Architecture
- `ThemeManager` — QML Singleton. Persists choice via `Qt.labs.settings`.
- `AppStyle` — QML Singleton. Layout constants, never colors.
- All QML files import `rtu.ui` and reference `ThemeManager.*` directly.

### Themes

| Theme | Style | Background | Accent |
|-------|-------|-----------|--------|
| Dark | Tokyo Night | `#1A1B26` | `#7AA2F7` |
| Light | Clean | `#F0F2F5` | `#4361EE` |
| Monokai | Monokai | `#272822` | `#A6E22E` |
| SolarizedDark | Solarized | `#002B36` | `#268BD2` |
| SolarizedLight | Solarized | `#FDF6E3` | `#268BD2` |

### Color Tokens

| Token | Usage |
|-------|-------|
| `background` | Page/window background |
| `surface` | Cards, panels |
| `surfaceAlt` | Hover state surface |
| `sidebarBg` | Sidebar background |
| `topBarBg` | Top bar background |
| `accent` | Primary interactive color |
| `accentHover` | Lightened accent for hover |
| `accentSubtle` | 15% opacity accent (badge bg, selected row) |
| `text` | Primary text |
| `textSecondary` | Labels, descriptions |
| `textDisabled` | Locked/inactive elements |
| `border` | Panel, input borders |
| `borderFocus` | Input focus ring |
| `success` / `error` / `warning` | Status indicators |
| `proGold` | Pro tier badge |

### Switching Theme at Runtime
```qml
ThemeManager.setTheme("Monokai")   // persisted automatically
```

---

## Animations

| Animation | Target | Duration |
|-----------|--------|---------|
| Sidebar collapse | `implicitWidth` | 220ms OutCubic |
| Page push | `opacity` + `x` slide | 220ms OutCubic |
| Page pop | `opacity` + `-x` slide | 220ms OutCubic |
| Login → Shell | `opacity` fade | 220ms |
| Login error | Card shake (x) | 260ms sequential |
| NavItem hover bg | `ColorAnimation` | 120ms |
| Button hover bg | `ColorAnimation` | 120ms |
| Input focus border | `ColorAnimation` | 120ms |

---

## Navigation Model

Defined in `AppShell.qml._routes`:

| PageId | Component | Tier |
|--------|-----------|------|
| `dashboard` | DashboardPage | Free |
| `modbus_rtu` | ModbusPage (RTU) | Free |
| `modbus_tcp` | ModbusPage (TCP) | Free |
| `dnp3` | IEC104Page | Pro |
| `iec101` | IEC104Page | Pro |
| `iec104` | IEC104Page | Pro |
| `serial` | SerialPage | Free |
| `analyzer` | AnalyzerPage | Pro |
| `simulator` | SimulatorPage | Pro |

Keyboard shortcuts: `Ctrl+1` → Dashboard, `Ctrl+2` → Modbus RTU.

---

## Feature Gating in UI

### Two-layer approach:

**1. NavItem lock icon**  
```qml
// NavItem.qml
readonly property bool locked: requiresPro && !isPro
// → shows 🔒 icon, Qt.ForbiddenCursor
```

**2. ProLockOverlay page wrapper**  
```qml
// IEC104Page.qml
ProLockOverlay {
    requiresPro: true
    isPro: AppVM.isPro
    featureName: "IEC-104"
    onUpgradeRequested: Qt.openUrlExternally(upgradeUrl)
    
    // Actual page content here — hidden until unlocked
    Rectangle { ... }
}
```

---

## ViewModel Bindings

### AppVM (context property — set by Application class)

| QML Usage | Property |
|-----------|----------|
| `AppVM.isLoggedIn` | Login ↔ Shell routing |
| `AppVM.isPro` | Feature gate throughout |
| `AppVM.userId` | TopBar user label |
| `AppVM.tierName` | TopBar tier badge |
| `AppVM.login(email, pass)` | LoginPage submit |
| `AppVM.logout()` | TopBar user menu |

### SessionVM (to be wired in Integration phase)

| QML Binding | Property |
|-------------|----------|
| `SessionVM.connectionStatus` | StatusBadge in each page |
| `SessionVM.responseModel` | Response table (ModbusPage) |
| `SessionVM.rawLog` | Hex log panel |
| `SessionVM.connect(params)` | Connect button |
| `SessionVM.sendRequest(params)` | Send button |

---

## Testing

**QML tests:** `tests/unit/tst_theme.qml`  
**Runner:** `tests/unit/tst_ui_runner.cpp`

Coverage:
- Default theme is Dark
- All 5 themes switch correctly
- `isDark` flag correct per theme
- All color tokens defined for all themes
- Theme count = 5

**Run:**
```bash
cmake --build build
cd build && ctest -R tst_ui --output-on-failure
```

---

## Integration Instructions for Core Engineer

**Role:** CORE ENGINEER  
**Goal:** Implement `IProtocol` and `ITestSession` for Modbus RTU and Modbus TCP.

**What the UI expects from `SessionViewModel`:**

```cpp
// These Q_PROPERTYs must emit their NOTIFY signals for UI to refresh:
connectionStatus()  → "connected" | "disconnected" | "error"
isConnected()       → bool
isRunning()         → bool
responseModel()     → QVariantList of { address, dec, hex, bin }
rawLog()            → QStringList of "→ XX XX XX" / "← XX XX XX" lines

// These Q_INVOKABLEs must be implemented:
connect(params)     → { port, baud, unitId }  (RTU)
                       { host, port, unitId }  (TCP)
disconnect()
sendRequest(params) → { functionCode, startAddress, count }
clearLog()
```

**What the UI expects from `AppViewModel`:**
```cpp
// Already defined in AppViewModel.h (Phase 1).
// Implement the body — wire IAuthService and ISubscriptionService.
```

**Pro feature flag usage in Core:**
```cpp
// Before creating any Pro session in TestEngine:
if (protocol->requiresPro()) {
    auto* sub = ServiceLocator::instance().subscription();
    if (!sub->hasFeature(Feature::DNP3)) {  // or IEC104, etc.
        emit sessionCreationFailed("Pro subscription required");
        return nullptr;
    }
}
```
