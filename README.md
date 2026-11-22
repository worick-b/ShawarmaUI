# 🥙 ShawarmaUI
~Sweet Dreams (Are Made of This)

> *A UI Sexy Model — Ultra-lightweight, single-header C++17 Immediate-Mode GUI library for Windows and Android.*

**ShawarmaUI** is a `single-header` retained/immediate hybrid GUI library built from scratch with zero external dependencies. Designed for high-performance utilities, launchers, media players or anything your stomach want, that demand native speed, crisp typography, and near-zero RAM/CPU overhead.

---

## ✨ Key Features

- **Single-Header & Zero Dependencies:** `~I've got to break free, I've fallen in love"`
- **For Windows:** Drop `ShawarmaUI.h` into your project, choose `GDI` or `Direct2D` and compile.
  - **GDI (Default):** Works on every Windows machine out of the box (including Safe Mode, Windows PE, and basic video drivers).
  - **Direct2D + DirectWrite (`#define SHAWARMAGUI_USE_D2D`):** Hardware-accelerated rendering with cached `IDWriteTextLayout` for zero per-frame allocations.
- **For Android:** Drop `ShawarmaUI_Android.h && NanoVG` into your project, disable Material Design and compile.
- **Event-Driven & Ultra-Light:** Consumes **~5–8 MB of RAM** and **0.0% CPU** when idle, with smooth 30 FPS animations only while transitions are active.
- **Fluent API & RAII Scopes:** Chain layout modifiers (`ui.SameLine().AlignText().Text(...)`) and use automatic RAII scopes (`if (auto p = ui.Panel("Title")) { ... }`).
- **Built-in Themes:** Switch entire color schemes in one line (`DarkBlue`, `NeonRed`, `CyberPurple`, `LightClean`).
- **External Render Target Integration:** Embed ShawarmaUI HUD overlays directly into existing `IDXGISwapChain1` (DirectX 11) or `ID2D1HwndRenderTarget` pipelines.
- **It's:** `A personal project, don't expect a dev master blaster (12" Version)`.

---

## 🧩 Included Widgets

- **Containers & Layout:** `Panel` (RAII), `Child` (scrollable area), `Modal` (RAII popup), `Columns`, `SameLine`, `SameLineRight`, `Separator`, `SeparatorText`
- **Inputs & Controls:** `Button`, `ToggleButton`, `IconButton`, `Checkbox`, `ToggleSwitch`, `SliderFloat`, `SliderInt`, `InputText` (with clipboard & password support), `Combo`
- **Navigation & Lists:** `TabBar`, `SegmentedControl`, `SelectableRow`
- **Feedback & Display:** `Text`, `TextColored`, `Badge`, `KeyValue`, `ProgressBar`, `Spinner`, `Toast` (auto-fading corner notifications), `Tooltip`

---

## 🚀 Quick Start (Windows)

```cpp
#define SHAWARMAGUI_USE_D2D // Optional: omit for pure GDI mode
#include "ShawarmaUI.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    ShawarmaUI::Context ui;
    ui.ApplyTheme(ShawarmaUI::Theme::NeonRed);

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
                ctx.KeyValue("Library", "ShawarmaUI v5");
                ctx.Badge("Header-Only • C++17");
            }
        }
    });

    return 0;
}
```

---

## 🛠️ Compilation (MinGW / GCC)

### Pure GDI Mode (Maximum Compatibility)
```bash
g++ -std=c++17 -O3 -s main.cpp -o app.exe -lgdi32 -lmsimg32 -ldwmapi -lshell32 -mwindows -static
```

### Direct2D + DirectWrite Mode (Hardware Accelerated)
```bash
g++ -std=c++17 -O3 -s main.cpp -o app.exe -ld2d1 -ldwrite -ldwmapi -lshell32 -mwindows -static
```

---

## 🗺️ Roadmap

- [x] Windows GDI & Direct2D backends
- [x] Fluent API, RAII scopes, and 16+ core widgets
- [x] DXGI SwapChain & D2D HUD overlay mode
- [x] **Android Backend (`Zero Material Design Baby! S2`)** powered by OpenGL ES 3.0 + NanoVG (Updated `stb_truetype` & `stb_image`)

---
