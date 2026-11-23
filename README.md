# 🥙 ShawarmaUI
~Sweet Dreams (Are Made of This)

> *A UI Sexy Model — Ultra-lightweight, single-header C++17 Immediate-Mode GUI library for Windows and Android.*

**ShawarmaUI** is a `single-header` retained/immediate hybrid GUI library built from scratch with zero external dependencies. Designed for high-performance utilities, launchers, media players or anything your stomach want, that demand native speed, crisp typography, and near-zero RAM/CPU overhead.

---

## ✨ Key Features

- **Single-Header & Zero Dependencies:** `~I've got to break free, I've fallen in love"`
- **For Windows:** Drop `ShawarmaUI.h` into your project, choose `GDI` or `Direct2D` and compile.
  - **GDI (Default):** Works on every Windows machine out of the box (including Safe Mode, Windows PE, and basic video drivers).
  - **Direct2D + DirectWrite (`#define SHAWARMAUI_USE_D2D`):** Hardware-accelerated rendering with cached `IDWriteTextLayout` for zero per-frame allocations.
- **For Android (`ShawarmaUI_Android.h`):** Powered by OpenGL ES 3.0 + NanoVG — zero Material Design, zero XML bloat.
  - **Frosted Glass & Living Backdrop:** Translucent smoked-glass panels with specular rim-light borders over an organically drifting ambient orb backdrop (locked at ~10 FPS for ultra-low battery usage while keeping touch input snappy up to 30+ FPS).
  - **3D Vertical Pill Gradients:** Subtle top-to-bottom lighting gradients and specular edge highlights on active buttons, tabs, and segmented controls.
  - **Embedded Icon Font Ready:** Built-in `LoadIconFontFromMemory` fallback support (compatible with `ShawarmaIcons.h`).
- **Event-Driven & Ultra-Light:** Consumes **~2–8 MB of RAM** and near-zero CPU when idle.
- **Fluent API & RAII Scopes:** Chain layout modifiers (`ui.SameLine().AlignText().Text(...)`) and use automatic RAII scopes (`if (auto p = ui.Panel("Title")) { ... }`).
- **Built-in Themes:** Switch entire color schemes in one line (`RoseGold`, `NeonRed`, `CyberPurple`, `LightClean`).
- **External Render Target Integration:** Embed ShawarmaUI HUD overlays directly into existing `IDXGISwapChain1` (DirectX 11), `ID2D1HwndRenderTarget` (Direct2D), or Android OpenGL ES video pipelines (`BeginHUDFrame`).
- **It's:** `A personal project, don't expect a dev master blaster (12" Version)`.

---

## 🧩 Included Widgets

- **Containers & Layout:** `Panel` (RAII), `Child` (touch-scrollable area), `Modal` (RAII popup), `Columns`, `SameLine`, `SameLineRight`, `Separator`, `SeparatorText`
- **Inputs & Controls:** `Button`, `ToggleButton`, `IconButton`, `Checkbox`, `ToggleSwitch`, `SliderFloat`, `SliderInt`, `InputText` (single-line & password support), `InputTextMultiline` (scrollable text/code editor), `Combo`
- **Navigation & Lists:** `TabBar`, `SegmentedControl`, `SelectableRow`
- **Feedback & Display:** `Text`, `TextColored`, `Badge`, `KeyValue`, `ProgressBar`, `Spinner`, `Toast` (auto-fading corner notifications), `Tooltip` (long-press support on Android)

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

## 📱 Quick Start (Android / OpenGL ES 3.0)

```cpp
#include "ShawarmaUI_Android.h"
#include "ShawarmaIcons.h" // Optional vector icon font

ShawarmaUI::Context ui;

// Inside GLSurfaceView.Renderer::onSurfaceCreated:
void OnSurfaceCreated(NVGcontext* vg, float density) {
    ui.InitGLES3(vg, density);
    ui.LoadIconFontFromMemory(ShawarmaUI::kIconFontTTF.data(), (int)ShawarmaUI::kIconFontTTF.size());
    ui.ApplyTheme(ShawarmaUI::Theme::RoseGold); // RoseGold, NeonRed, CyberPurple, LightClean
}

// Inside GLSurfaceView.Renderer::onDrawFrame (use RENDERMODE_WHEN_DIRTY + 100ms ticker for 10 FPS idle backdrop):
void OnDrawFrame(int width, int height) {
    ui.BeginFrame(width, height);
    if (auto p = ui.Panel("Frosted Glass Panel")) {
        ui.Text("Translucent glass over 10 FPS ambient lights.");
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
g++ -std=c++17 -O3 -s main.cpp -o app.exe -ld2d1 -ldwrite -ldwmapi -lshell32 -mwindows -static
```

---

## 🗺️ Roadmap

- [x] Windows GDI & Direct2D backends
- [x] Fluent API, RAII scopes, and 18+ core widgets
- [x] DXGI SwapChain & D2D HUD overlay mode
- [x] Android Backend **(`Zero Material Design Baby! S2`)** powered by OpenGL ES 3.0 + NanoVG (`stb_truetype` & `stb_image`)
- [x] Translucent frosted-glass panels, 3D vertical gradients & 10 FPS battery-friendly ambient backdrop
