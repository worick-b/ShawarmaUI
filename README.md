# 🥙 ShawarmaUI
~Sweet Dreams (Are Made of This)

> *A UI Sexy Model — Ultra-lightweight, single-header Immediate-Mode library for Windows and Android.*

**ShawarmaUI** is a `single-header` retained/immediate hybrid GUI library built from scratch with zero external dependencies and **zero C-style arrays** (modern C++17 containers). Designed for high-performance utilities, launchers, media players or anything your stomach wants — native speed, crisp typography, and **~2–15 MB RAM** footprint even on legacy CPUs.

---

## ✨ Key Features

* **Single-Header & Zero Dependencies:** `"~I've got to break free, I've fallen in love"`
* **Hybrid Runtime Backend (Windows):** Dynamically loads **Direct2D + DirectWrite** at runtime with zero static DLL imports — automatically falling back to pure **Win32 GDI** in Windows PE, Safe Mode, or legacy drivers (plus live switching via `SwitchBackend()`).
* **Unified Frosted Glass & Decoupled Living Backdrop:** Smoked-glass panels with specular rim-light borders over an organically drifting ambient orb backdrop — cached at **15 FPS** while keeping UI interactions crisp at **30+ FPS** (Windows & Android).
* **Cached Word-Wrap & Responsive Layouts:** Automatic text wrapping (`WrapTextCached`) across `TextWrapped`, `InputTextMultiline`, `KeyValue`, and `Panel` headers, paired with `ButtonRow` and `SameLineWrap` for adaptive portrait/landscape layouts.
* **3D Vertical Gradients:** Subtle top-to-bottom lighting gradients and specular edge highlights on active buttons, tabs, and segmented controls.
* **Fluent API & RAII Scopes:** Chain layout modifiers (`ui.SameLineWrap(w)`, `ui.SameLine().AlignText().Text(...)`), nest panels, and isolate dynamic list items with `ctx.ID(...)` or `"Label##id"` suffixes across both Windows and Android.
* **Zero-Copy HUD Overlays:** Embed directly into existing `IDXGISwapChain1` (DirectX 11 / ReShade compatible), `ID2D1HwndRenderTarget` (Direct2D), or Android OpenGL ES video pipelines (`BeginHUDFrame`).
* **Built-in Themes & Auto-Persistence:** Switch entire color schemes in one line (`TealGold` *[Default]*, `NeonRed`, `CyberPurple`, `RoseGold`, `LightClean`) with optional automatic Windows Registry persistence (`EnableThemePersistence`) and DWM title-bar sync.
* **It's:** `A personal project, don't expect a dev master blaster (12" Version)`.

---

## 🖥️ Supported Backends

| Platform | Header | Rendering | Details |
| :--- | :--- | :--- | :--- |
| **Windows (Hybrid)** | `ShawarmaUI.h` | Direct2D + DirectWrite *(Auto GDI Fallback)* | Runtime `LoadLibraryW` loading with cached `IDWriteTextLayout` & offscreen `ID2D1BitmapRenderTarget` backdrop — falls back seamlessly to pure Win32 GDI on WinPE / Safe Mode with zero extra linker flags |
| **Android** | `ShawarmaUI_Android.h` | OpenGL ES 3.0 + NanoVG | Zero Material Design, zero XML bloat, ~30 FPS throttled redraws & `LoadIconFontFromMemory` (`ShawarmaIcons.h`) |

---

## 🧩 Included Widgets

| Category | Available Widgets |
| :--- | :--- |
| **Layout & Containers** | `Panel` *(RAII, nestable, auto-wrapping title)*, `Child` *(scrollable)*, `Modal` *(RAII)*, `ID` / `PushID` *(RAII)*, `Columns`, `SameLine`, `SameLineRight`, `SameLineWrap` *(auto-wrap)*, `Separator`, `SeparatorText` |
| **Inputs & Controls** | `Button`, `ButtonRow` *(responsive)*, `ToggleButton`, `IconButton`, `ThemeSwitcherButton`, `Checkbox`, `ToggleSwitch`, `SliderFloat`, `SliderInt`, `InputText`, `InputTextMultiline` *(Windows & Android, auto-wrap + scroll)*, `InputTextReadOnly`, `Combo` |
| **Navigation & Lists** | `TabBar`, `SegmentedControl`, `SelectableRow` |
| **Feedback & Display** | `Text`, `TextColored`, `TextWrapped` *(cached)*, `Badge`, `KeyValue` *(auto-wrap)*, `ProgressBar`, `Spinner`, `Toast`, `Tooltip` |

---

## 🚀 Quick Start & Compilation

<details>
<summary><b> Windows — Single-Command MinGW Compilation (D2D + GDI Fallback)</b> (Click to expand)</summary>

```cpp
#include "ShawarmaUI.h"

int main() {
    ShawarmaUI::Context ui;
    ui.ApplyTheme(ShawarmaUI::Theme::TealGold);
    if (!ui.Create("ShawarmaUI App", 640, 560)) return 1;

    ui.Run([](ShawarmaUI::Context& ctx) {
        if (auto p = ctx.Panel("Hello ShawarmaUI")) {
            ctx.TextWrapped("Runs on Direct2D + DirectWrite automatically, with seamless GDI fallback!");
            if (ctx.Button("Switch Theme")) ctx.CycleTheme();
        }
    });
    return 0;
}
```

**Single Build Command (No `-ld2d1` or `-ldwrite` needed):**
```bash
g++ -Os -std=c++17 -Iinclude ShawarmaUI_Demo.cpp -o ShawarmaUI_Demo.exe -lgdi32 -lmsimg32 -ldwmapi -lshell32 -ladvapi32 -mwindows -static -s
```
</details>

<details>
<summary><b> Android — OpenGL ES 3.0 + NanoVG Header Example</b> (Click to expand)</summary>

```cpp
#include "ShawarmaUI_Android.h"
#include "ShawarmaIcons.h" // Optional vector icon font
```
</details>

---

## 🗺️ Roadmap

- [x] Windows Hybrid Direct2D + DirectWrite backend with automatic Win32 GDI runtime fallback & 15 FPS cached backdrop
- [x] Fluent API, cached word-wrap (`WrapTextCached`), responsive `ButtonRow` / `SameLineWrap`, RAII scopes, and 20+ core widgets (100% C-array free)
- [x] Zero-copy DXGI SwapChain & D2D HUD overlay mode
- [x] Android Backend powered by OpenGL ES 3.0 + NanoVG (`Updated stb_truetype & stb_image`)
- [x] Unified themes (`TealGold`, `NeonRed`, `CyberPurple`, `RoseGold`, `LightClean`) with Windows Registry persistence & DWM title-bar sync
