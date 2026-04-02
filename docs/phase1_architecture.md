# Phase 1 — Architecture

**Role:** ARCHITECT  
**Status:** Complete  
**Date:** 2026-04-02

---

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                          QML / UI Layer                         │
│   main.qml  ←→  AppViewModel  ←→  (future: SessionViewModel)   │
└────────────────────────┬────────────────────────────────────────┘
                         │ Q_PROPERTY bindings / Q_INVOKABLE
┌────────────────────────▼────────────────────────────────────────┐
│                       Core Layer                                │
│          TestEngine  →  ITestSession  →  IProtocol             │
│                              ↓                                  │
│                      ProtocolRegistry                           │
│              [ModbusRTU] [ModbusTCP] [DNP3] [IEC101] [IEC104]  │
└────────────────────────┬────────────────────────────────────────┘
                         │ feature gate checks
┌────────────────────────▼────────────────────────────────────────┐
│                     Services Layer                              │
│         IAuthService    ←→    ISubscriptionService             │
│                    ServiceLocator (singleton)                   │
└────────────────────────┬────────────────────────────────────────┘
                         │ (backend TBD)
                    [ REST / gRPC API ]
```

---

## Module Responsibilities

| Module | Responsibility | Notes |
|--------|---------------|-------|
| `app/` | Bootstrap, QML engine init, service wiring | No business logic |
| `ui/` | QML files + ViewModels (QObject) | Only Qt signals/properties, no protocol logic |
| `core/protocols/` | IProtocol contract + ProtocolRegistry | One subfolder per protocol |
| `core/engine/` | TestEngine, ITestSession | Enforces feature gates |
| `core/models/` | Pure data structs (ProtocolFrame, etc.) | No Qt signals |
| `services/auth/` | IAuthService — login / token refresh | Concrete impl uses backend |
| `services/subscription/` | ISubscriptionService + SubscriptionTier | Feature flag queries |
| `services/` | ServiceLocator | Single access point for all services |
| `utils/` | Logger (QLoggingCategory) + AppConfig (QSettings) | No module deps |

---

## Interfaces

### IProtocol
```cpp
// src/core/protocols/IProtocol.h
virtual ProtocolType  type()        const = 0;
virtual QString       name()        const = 0;
virtual TransportType transport()   const = 0;
virtual bool          requiresPro() const = 0;
virtual bool   connect(const QVariantMap& params) = 0;
virtual void   disconnect() = 0;
virtual bool   isConnected() const = 0;
virtual ProtocolFrame buildRequest(const QVariantMap& params) const = 0;
virtual bool          sendFrame(const ProtocolFrame& frame) = 0;
virtual ProtocolFrame parseResponse(const QByteArray& raw) const = 0;
```

### IAuthService
```cpp
// src/services/auth/IAuthService.h
virtual AuthState state()         const = 0;
virtual QString   currentUserId() const = 0;
virtual QString   accessToken()   const = 0;
// slots: login(), logout(), refreshToken()
// signals: stateChanged, loginSucceeded, loginFailed, loggedOut
```

### ISubscriptionService
```cpp
// src/services/subscription/ISubscriptionService.h
virtual SubscriptionTier currentTier()               const = 0;
virtual bool             hasFeature(Feature feature)  const = 0;
virtual void             refresh() = 0;
// signals: tierChanged
```

### ITestSession
```cpp
// src/core/engine/ITestSession.h
virtual QString               sessionId() const = 0;
virtual SessionState          state()     const = 0;
virtual QList<ProtocolFrame>  frames()    const = 0;
// slots:  start(), pause(), resume(), stop()
// signals: stateChanged, frameReceived, errorOccurred
```

---

## Key Decisions

### QML vs Widgets → QML Chosen
| Criterion | QML | Widgets |
|-----------|-----|---------|
| UI/Logic separation | Excellent (declarative bindings) | Moderate |
| Custom styling / branding | Easy (SaaS UI) | Hard |
| Data visualization (frame inspector, oscilloscope) | Qt Quick Scene Graph | Requires manual painting |
| Free/Pro visual differentiation | Property bindings | Manual show/hide |
| Role-based dev workflow | UI Engineer writes QML, Core writes C++ | Mixed |
| Windows 10/11 modern look | Qt Quick Controls 2 | Dated |

**Verdict:** QML with Qt Quick Controls 2. Core logic stays in C++; QML only binds to ViewModels.

### ServiceLocator vs Full DI
Full DI (e.g. inject interfaces via constructors everywhere) adds complexity not justified at this scale. ServiceLocator gives testability (swap impls in tests) with minimal overhead. Revisit if module count grows significantly.

### Feature Gating Strategy
- `IProtocol::requiresPro()` — protocol-level gate  
- `ISubscriptionService::hasFeature(Feature)` — fine-grained gate used by TestEngine and ViewModels  
- UI binds to `AppViewModel::isPro` to show/hide Pro-only controls  

---

## Folder Structure

```
Project1-RTU/
├── CMakeLists.txt
├── src/
│   ├── CMakeLists.txt
│   ├── app/
│   │   ├── CMakeLists.txt
│   │   ├── main.cpp
│   │   └── Application.h
│   ├── core/
│   │   ├── CMakeLists.txt
│   │   ├── models/
│   │   │   └── ProtocolFrame.h
│   │   ├── protocols/
│   │   │   ├── IProtocol.h
│   │   │   └── ProtocolRegistry.h
│   │   └── engine/
│   │       ├── ITestSession.h
│   │       └── TestEngine.h
│   ├── services/
│   │   ├── CMakeLists.txt
│   │   ├── ServiceLocator.h
│   │   ├── auth/
│   │   │   └── IAuthService.h
│   │   └── subscription/
│   │       ├── ISubscriptionService.h
│   │       └── SubscriptionTier.h
│   ├── ui/
│   │   ├── CMakeLists.txt
│   │   ├── viewmodels/
│   │   │   └── AppViewModel.h
│   │   └── qml/
│   │       └── main.qml
│   └── utils/
│       ├── CMakeLists.txt
│       ├── Logger.h
│       └── AppConfig.h
├── tests/
│   ├── CMakeLists.txt
│   ├── unit/
│   │   ├── CMakeLists.txt
│   │   └── tst_protocol_interface.cpp
│   └── integration/
│       └── CMakeLists.txt
├── docs/
│   └── phase1_architecture.md
└── resources/
    └── qml/
```

---

## Data Flow

```
User Action (QML)
    → AppViewModel::login()
        → IAuthService::login(email, password)
            → [backend call — TBD]
            → IAuthService::loginSucceeded signal
                → ISubscriptionService::refresh()
                    → AppViewModel::subscriptionChanged signal
                        → QML isPro property updates
                        → Pro-locked controls become visible

User selects protocol + clicks "Start Test"
    → SessionViewModel::startSession()
        → TestEngine::createSession(params)
            → ISubscriptionService::hasFeature(Feature::DNP3) checked
            → IProtocol::connect(connectionParams)
            → ITestSession::start()
                → frames emitted via frameReceived signal
                    → SessionViewModel updates frame list
                        → QML ListView refreshes
```

---

## SaaS Integration Points

| Point | Interface | When |
|-------|-----------|------|
| Login / Registration | `IAuthService::login()` | App startup / login screen |
| Token refresh | `IAuthService::refreshToken()` | On 401 from backend |
| Subscription check | `ISubscriptionService::hasFeature()` | Before any Pro operation |
| Subscription refresh | `ISubscriptionService::refresh()` | After login, on resume from sleep |
| Pro upgrade prompt | `AppViewModel::isPro` (false) | When user tries locked feature |

---

## Testing Setup

**Framework:** Qt Test + CTest  
**Why:** Built into Qt, no extra deps, integrates with CMake's `enable_testing()`.

**Current coverage:**
- `tst_protocol_interface` — validates `IProtocol` contract via `StubProtocol`

**Run tests:**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
cd build && ctest --output-on-failure
```

---

## Next Phase — UI Engineer Instructions

**Role:** UI ENGINEER  
**Goal:** Implement the application shell in QML.

**Tasks:**
1. Design the main window shell:
   - Sidebar (protocol selector, session list)
   - Central content area (frame inspector)
   - Top bar (user info, tier badge, settings)

2. Implement the Login screen:
   - Bind to `AppViewModel::login()` and `AppViewModel::loginFailed`
   - Show loading state during `Authenticating`

3. Implement Free vs Pro visual gating:
   - Lock icons on Pro features
   - Upgrade prompt dialog when Free user clicks a Pro feature

4. Bind to `AppViewModel` properties:
   - `isLoggedIn`, `isPro`, `userId`, `tierName`

5. Create a `SessionViewModel` stub (header only) so QML can bind to session data

**Constraints:**
- NO business logic in QML
- All C++ interaction goes through Q_PROPERTY and Q_INVOKABLE
- Use Qt Quick Controls 2 (Material or Fluent style)
- Dark mode must be supported from day 1 (bind to `AppConfig::darkTheme`)
