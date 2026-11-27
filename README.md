# 🥙 ShawarmaUI
~Sweet Dreams (Are Made of This)

> *A UI Sexy Model — Ultra-lightweight, single-header Immediate-Mode library for Windows and Android.*

**ShawarmaUI** is a `single-header` retained/immediate hybrid GUI library built from scratch with zero external dependencies and **zero C-style arrays** (modern C++17 containers). Designed for high-performance utilities, launchers, media players or anything your stomach wants — native speed, crisp typography, and **~2–15 MB RAM** footprint even on legacy CPUs.

---

## ✨ Key Features

* **Single-Header & Zero Dependencies:** `"~I've got to break free, I've fallen in love"`
* **Unified Frosted Glass & Decoupled Living Backdrop:** Smoked-glass panels with specular rim-light borders over an organically drifting ambient orb backdrop — cached at **15 FPS** while keeping UI interactions crisp at **30+ FPS** (Windows & Android).
* **3D Vertical Gradients:** Subtle top-to-bottom lighting gradients and specular edge highlights on active buttons, tabs, and segmented controls.
* **Fluent API, Responsive Wrapping & RAII Scopes:** Chain layout modifiers (`ui.SameLineWrap(w)`, `ui.SameLine().AlignText().Text(...)`), nest panels, and isolate dynamic list items with `ctx.ID(...)` or `"Label##id"` suffixes across both Windows and Android.
* **Zero-Copy HUD Overlays:** Embed directly into existing `IDXGISwapChain1` (DirectX 11 / ReShade compatible), `ID2D1HwndRenderTarget` (Direct2D), or Android OpenGL ES video pipelines (`BeginHUDFrame`).
* **Built-in Themes & Auto-Persistence:** Switch entire color schemes in one line (`NeonRed` *[Default]*, `CyberPurple`, `RoseGold`, `LightClean`) with optional automatic Windows Registry persistence (`EnableThemePersistence`) and DWM title-bar sync.
* **It's:** `A personal project, don't expect a dev master blaster (12" Version)`.

---

## 🖥️ Supported Backends

| Platform | Header | Rendering | Details |
| :--- | :--- | :--- | :--- |
| **Windows (Default)** | `ShawarmaUI.h` | Pure Win32 GDI | Out-of-the-box compatibility (Safe Mode, Windows PE, basic drivers) with 15 FPS cached `HBITMAP` backdrop, `GradientFill` & `AlphaBlend` |
| **Windows (GPU)** | `ShawarmaUI.h` | Direct2D + DirectWrite | `#define SHAWARMAUI_USE_D2D` • Cached `IDWriteTextLayout` & offscreen `ID2D1BitmapRenderTarget` backdrop for zero per-frame allocations |
| **Android** | `ShawarmaUI_Android.h` | OpenGL ES 3.0 + NanoVG | Zero Material Design, zero XML bloat, ~30 FPS throttled redraws & `LoadIconFontFromMemory` (`ShawarmaIcons.h`) |

---

## 🧩 Included Widgets

| Category | Available Widgets |
| :--- | :--- |
| **Layout & Containers** | `Panel` *(RAII, nestable)*, `Child` *(scrollable)*, `Modal` *(RAII)*, `ID` / `PushID` *(RAII)*, `Columns`, `SameLine`, `SameLineRight`, `SameLineWrap` *(auto-wrap)*, `Separator`, `SeparatorText` |
| **Inputs & Controls** | `Button`, `ToggleButton`, `IconButton`, `ThemeSwitcherButton`, `Checkbox`, `ToggleSwitch`, `SliderFloat`, `SliderInt`, `InputText`, `InputTextMultiline` *(Android)*, `Combo` |
| **Navigation & Lists** | `TabBar`, `SegmentedControl`, `SelectableRow` |
| **Feedback & Display** | `Text`, `TextColored`, `Badge`, `KeyValue`, `ProgressBar`, `Spinner`, `Toast`, `Tooltip` |

---

## 🚀 Quick Start & Compilation

<details>
<summary><b> Windows — Header Example & MinGW Compilation</b> (Click to expand)</summary>

```cpp
#define SHAWARMAUI_USE_D2D // Optional: omit for pure GDI mode
#include "ShawarmaUI.h"
```

**Pure GDI Mode (Maximum Compatibility):**
```bash
g++ -Os -std=c++17 -Iinclude examples/windows/Demo-GDI.cpp -o ShawarmaUI_GDI.exe -lgdi32 -lmsimg32 -ldwmapi -lshell32 -ladvapi32 -mwindows -static -s
```

**Direct2D + DirectWrite Mode (Hardware Accelerated):**
```bash
g++ -Os -std=c++17 -Iinclude examples/windows/Demo-D2D.cpp -o ShawarmaUI_D2D.exe -ld2d1 -ldwrite -luuid -ldwmapi -lshell32 -ladvapi32 -mwindows -static -s
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

- [x] Windows GDI & Direct2D backends with frosted-glass panels & decoupled 15 FPS cached backdrop
- [x] Fluent API, responsive `SameLineWrap`, RAII scopes, and 18+ core widgets (100% C-array free)
- [x] Zero-copy DXGI SwapChain & D2D HUD overlay mode
- [x] Android Backend powered by OpenGL ES 3.0 + NanoVG (`Updated stb_truetype & stb_image`)
- [x] Unified themes (`NeonRed`, `CyberPurple`, `RoseGold`, `LightClean`) with Windows Registry persistence & DWM title-bar sync
