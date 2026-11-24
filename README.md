# 🥙 ShawarmaUI
~Sweet Dreams (Are Made of This)

> *A UI Sexy Model — Ultra-lightweight, single-header C++17 Immediate-Mode GUI library for Windows and Android.*

**ShawarmaUI** is a `single-header` retained/immediate hybrid GUI library built from scratch with zero external dependencies and **zero C-style arrays** (100% modern C++17 containers). Designed for high-performance utilities, launchers, media players or anything your stomach wants. Native speed, crisp typography, and near-zero RAM/CPU overhead.

---

## ✨ Key Features

- **Single-Header & Zero Dependencies:** `~I've got to break free, I've fallen in love"`
- **Unified Frosted Glass & Living Backdrop (Windows & Android):** Translucent smoked-glass panels with specular rim-light borders over an organically drifting ambient orb backdrop.
- **3D Vertical Gradients:** Subtle top-to-bottom lighting gradients and specular edge highlights on active buttons, tabs, and segmented controls.
- **For Windows (`ShawarmaUI.h`):** Drop it into your project, choose `GDI` or `Direct2D`, and compile.
  - **GDI (Default):** Works on every Windows machine out of the box (including Safe Mode, Windows PE, and basic video drivers) with `GradientFill` and `AlphaBlend` effects.
  - **Direct2D + DirectWrite (`#define SHAWARMAUI_USE_D2D`):** Hardware-accelerated rendering with cached `IDWriteTextLayout` for zero per-frame allocations.
- **For Android (`ShawarmaUI_Android.h`):** Powered by OpenGL ES 3.0 + NanoVG — zero Material Design, zero XML bloat.
  - **Battery-Friendly & Responsive:** Throttled redraws (~30 FPS during drag/scroll, immediate response on tap) and embedded icon font fallback support (`LoadIconFontFromMemory`, compatible with `ShawarmaIcons.h`).
- **Event-Driven & Ultra-Light:** Consumes **~2–15 MB of RAM** and near-zero CPU when idle (runs smoothly even on legacy CPUs).
- **Fluent API & RAII Scopes:** Chain layout modifiers (`ui.SameLine().AlignText().Text(...)`) and use automatic RAII scopes (`if (auto p = ui.Panel("Title")) { ... }`).
- **Built-in Themes:** Switch entire color schemes in one line (`NeonRed` [Default], `CyberPurple`, `RoseGold`, `LightClean`).
- **Zero-Copy External Render Target Integration:** Embed ShawarmaUI HUD overlays directly into existing `IDXGISwapChain1` (DirectX 11 / ReShade compatible), `ID2D1HwndRenderTarget` (Direct2D), or Android OpenGL ES video pipelines (`BeginHUDFrame`).
- **It's:** `A personal project, don't expect a dev master blaster (12" Version)`.

---

## 🧩 Included Widgets

- **Containers & Layout:** `Panel` (RAII), `Child` (scrollable area), `Modal` (RAII popup), `Columns`, `SameLine`, `SameLineRight`, `Separator`, `SeparatorText`
- **Inputs & Controls:** `Button`, `ToggleButton`, `IconButton`, `Checkbox`, `ToggleSwitch`, `SliderFloat`, `SliderInt`, `InputText` (single-line & password support), `InputTextMultiline` (scrollable text/code editor on Android), `Combo`
- **Navigation & Lists:** `TabBar`, `SegmentedControl`, `SelectableRow`
- **Feedback & Display:** `Text`, `TextColored`, `Badge`, `KeyValue`, `ProgressBar`, `Spinner`, `Toast` (auto-fading notifications), `Tooltip` (hover on Windows / long-press on Android)

---

## 🚀 Quick Start (Windows)

```cpp
#define SHAWARMAUI_USE_D2D // Optional: omit for pure GDI mode
#include "ShawarmaUI.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    ShawarmaUI::Context ui;
    ui.ApplyTheme(ShawarmaUI::Theme::NeonRed); // NeonRed, CyberPurple, RoseGold, LightClean, ShawarmaGold

    if (!ui.Create("ShawarmaUI Demo", 480, 320)) return 1;

    int tab = 0;
    bool turbo = true;
    int targetFps = 60;

    ui.Run([&](ShawarmaUI::Context& ctx) {
        ctx.TabBar(tab, { "General", "About" });
        ctx.Spacing();

        if (tab == 0) {
            if (auto p = ctx.Panel("Performance Settings")) {
                ctx.ToggleSwitch("Enable Turbo Mode", turbo);
                ctx.SliderInt(targetFps, 30, 144, -1, 18, " FPS");

                if (ctx.Button("Apply Settings", -1, 32)) {
                    ctx.Toast("Settings saved!", ShawarmaUI::Color(115, 224, 140));
                }
            }
        } else {
            if (auto p = ctx.Panel("System Info")) {
                ctx.KeyValue("Library", "ShawarmaUI");
                ctx.Badge("Header-Only • Modern C++17");
            }
        }
    });

    return 0;
}
```

---

## 📱 Quick Start (Android / OpenGL ES 3.0)

```cpp
#include "ShawarmaUI_Android.h"
#include "ShawarmaIcons.h" // Optional vector icon font

ShawarmaUI::Context ui;

// Inside GLSurfaceView.Renderer::onSurfaceCreated:
void OnSurfaceCreated(NVGcontext* vg, float density) {
    ui.InitGLES3(vg, density);
    ui.LoadIconFontFromMemory(ShawarmaUI::kIconFontTTF.data(), (int)ShawarmaUI::kIconFontTTF.size());
    ui.ApplyTheme(ShawarmaUI::Theme::NeonRed); // NeonRed, CyberPurple, RoseGold, LightClean
}

// Inside GLSurfaceView.Renderer::onDrawFrame:
void OnDrawFrame(int width, int height) {
    ui.BeginFrame(width, height);
    if (auto p = ui.Panel("Frosted Glass Panel")) {
        ui.Text("Translucent glass over ambient lights.");
        if (ui.Button("Action", -1, 38)) {
            ui.Toast("Clicked!");
        }
    }
    ui.EndFrame();
}
```

---

## 🛠️ Compilation (Windows — MinGW / GCC)

### Pure GDI Mode (Maximum Compatibility)
```bash
g++ -std=c++17 -O3 -s main.cpp -o app.exe -lgdi32 -lmsimg32 -ldwmapi -lshell32 -mwindows -static
```

### Direct2D + DirectWrite Mode (Hardware Accelerated)
```bash
g++ -std=c++17 -O3 -s main.cpp -o app.exe -ld2d1 -ldxgi -ldwrite -ldwmapi -lshell32 -mwindows -static
```

---

## 🗺️ Roadmap

- [x] Windows GDI & Direct2D backends with frosted-glass panels & living backdrop
- [x] Fluent API, RAII scopes, and 18+ core widgets (100% C-array free)
- [x] Zero-copy DXGI SwapChain & D2D HUD overlay mode
- [x] Android Backend **(`Zero Material Design Baby! S2`)** powered by OpenGL ES 3.0 + NanoVG (`stb_truetype` & `stb_image`)
- [x] Translucent frosted-glass panels, 3D vertical gradients & 10 FPS battery-friendly ambient backdrop
- [x] Unified themes (`NeonRed`, `CyberPurple`, `RoseGold`, `LightClean`)
