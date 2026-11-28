#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <winreg.h>
#ifdef DrawText
#undef DrawText
#endif
#include <d2d1_1.h>
#include <dwrite.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ShawarmaUI {

struct Color {
    uint8_t r, g, b, a;
    constexpr Color(uint8_t r = 255, uint8_t g = 255, uint8_t b = 255, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}
    COLORREF ToWin32() const { return RGB(r, g, b); }
    uint32_t Pack() const { return (uint32_t(a) << 24) | (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(b); }

    D2D1_COLOR_F ToD2D() const {
        return D2D1::ColorF(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
    }

    static Color Lerp(Color a, Color b, float t) {
        t = std::clamp(t, 0.0f, 1.0f);
        return Color(
            static_cast<uint8_t>(std::round(a.r + (b.r - a.r) * t)),
            static_cast<uint8_t>(std::round(a.g + (b.g - a.g) * t)),
            static_cast<uint8_t>(std::round(a.b + (b.b - a.b) * t)),
            static_cast<uint8_t>(std::round(a.a + (b.a - a.a) * t))
        );
    }
};

enum class Theme {
    TealGold = 0,
    NeonRed,
    CyberPurple,
    RoseGold,
    LightClean
};

struct Style {
    Color windowBg          = Color(11, 14, 22, 255);
    Color panelBg           = Color(16, 22, 32, 175);
    Color childBg           = Color(13, 18, 26, 165);
    Color popupBg           = Color(16, 22, 32, 245);
    Color tooltipBg         = Color(14, 19, 28, 245);
    Color modalDimBg        = Color(6, 8, 14, 195);
    Color border            = Color(248, 212, 108, 165);
    Color borderFocus       = Color(24, 218, 156, 255);
    Color text              = Color(246, 243, 244, 255);
    Color textDim           = Color(168, 156, 160, 255);
    Color frameBg           = Color(20, 28, 40, 175);
    Color frameHover        = Color(26, 38, 52, 210);
    Color checkMark         = Color(24, 218, 156, 255);
    Color sliderGrab        = Color(18, 152, 118, 255);
    Color button            = Color(18, 198, 148, 235);
    Color buttonGradEnd     = Color(10, 122, 142, 245);
    Color buttonHover       = Color(22, 162, 128, 255);
    Color buttonActive      = Color(12, 145, 128, 255);
    Color toggleActive      = Color(16, 184, 144, 240);
    Color toggleHover       = Color(22, 162, 128, 255);
    Color separator         = Color(255, 255, 255, 28);
    Color disabledBg        = Color(30, 24, 26, 150);
    Color disabledText      = Color(115, 104, 108, 255);
    Color scrollbarBg       = Color(14, 20, 30, 170);
    Color scrollbarGrab     = Color(248, 212, 108, 140);
    Color secondaryBtn      = Color(22, 32, 46, 240);
    Color secondaryBtnHover = Color(30, 44, 62, 250);
    Color success           = Color(129, 199, 132, 255);
    Color warning           = Color(255, 213, 79, 255);
    Color danger            = Color(229, 115, 115, 255);
    Color accent            = Color(248, 212, 108, 255);

    int windowPadding   = 16;
    int framePaddingX   = 10;
    int framePaddingY   = 6;
    int itemSpacingX    = 10;
    int itemSpacingY    = 8;
    int rounding        = 8;
    int animFps         = 30;
    int backdropFps     = 15;
};

class Context {
public:
    Style style;
    Theme currentTheme = Theme::TealGold;
    bool preferD2D = true;
    float backdropSeed = 0.0f;
    HWND hwnd = nullptr;
    float dpiScale = 1.0f;
    int minW = 280;
    int minH = 180;
    std::function<void(const std::string&)> onFileDrop;
    std::function<void()> onTimer;

    bool IsUsingD2D() const { return activeD2D; }

    struct PanelScope {
        Context& ctx;
        bool active;
        PanelScope(Context& c, const std::string& title, Color col, int customW) : ctx(c), active(true) {
            ctx.BeginPanel(title, col, customW);
        }
        ~PanelScope() { if (active) ctx.EndPanel(); }
        explicit operator bool() const { return active; }
    };

    struct ChildScope {
        Context& ctx;
        bool active;
        ChildScope(Context& c, const std::string& id, int w, int h, bool border) : ctx(c), active(true) {
            ctx.BeginChild(id, w, h, border);
        }
        ~ChildScope() { if (active) ctx.EndChild(); }
        explicit operator bool() const { return active; }
    };

    struct DisabledScope {
        Context& ctx;
        bool active;
        DisabledScope(Context& c, bool disabled) : ctx(c), active(true) {
            ctx.BeginDisabled(disabled);
        }
        ~DisabledScope() { if (active) ctx.EndDisabled(); }
        explicit operator bool() const { return active; }
    };

    struct ModalScope {
        Context& ctx;
        bool open;
        ModalScope(Context& c, const std::string& id, int w, int h) : ctx(c) {
            open = ctx.BeginPopupModal(id, w, h);
        }
        ~ModalScope() { if (open) ctx.EndPopupModal(); }
        explicit operator bool() const { return open; }
    };

    struct IdScope {
        Context& ctx;
        IdScope(Context& c, int id) : ctx(c) { ctx.PushID(id); }
        IdScope(Context& c, const std::string& id) : ctx(c) { ctx.PushID(id); }
        ~IdScope() { ctx.PopID(); }
        explicit operator bool() const { return true; }
    };

    PanelScope Panel(const std::string& titleUtf8 = "", Color titleColor = Color(0, 0, 0, 0), int customWidth = -1) {
        return PanelScope(*this, titleUtf8, titleColor, customWidth);
    }
    ChildScope Child(const std::string& idStr, int customW = -1, int customH = 200, bool border = true) {
        return ChildScope(*this, idStr, customW, customH, border);
    }
    DisabledScope Disabled(bool disabled = true) {
        return DisabledScope(*this, disabled);
    }
    ModalScope Modal(const std::string& id, int customW = 420, int customH = 115) {
        return ModalScope(*this, id, customW, customH);
    }
    IdScope ID(int id) { return IdScope(*this, id); }
    IdScope ID(const std::string& id) { return IdScope(*this, id); }

    Context() {
        interactiveRects.reserve(64);
        panelStack.reserve(4);
        modalDraws.reserve(16);
        comboOverlayDraws.reserve(8);
        clipStack.reserve(8);
        childStack.reserve(4);
        idStack.reserve(8);
    }

    ~Context() {
        CleanupResources();
        if (hModD2D1) FreeLibrary(hModD2D1);
        if (hModDWrite) FreeLibrary(hModDWrite);
    }

    void ApplyTheme(Theme theme) {
        currentTheme = theme;
        style = Style{};
        if (theme == Theme::NeonRed) {
            style.windowBg          = Color(12, 12, 16, 255);
            style.panelBg           = Color(24, 16, 22, 145);
            style.childBg           = Color(20, 14, 18, 135);
            style.popupBg           = Color(24, 20, 28, 240);
            style.tooltipBg         = Color(20, 18, 24, 245);
            style.modalDimBg        = Color(8, 6, 10, 185);
            style.border            = Color(230, 25, 65, 110);
            style.borderFocus       = Color(255, 50, 90, 255);
            style.frameBg           = Color(36, 18, 24, 155);
            style.frameHover        = Color(62, 20, 34, 195);
            style.checkMark         = Color(235, 35, 75, 255);
            style.sliderGrab        = Color(255, 40, 85, 255);
            style.button            = Color(225, 32, 68, 235);
            style.buttonGradEnd     = Color(165, 16, 42, 245);
            style.buttonHover       = Color(245, 45, 82, 250);
            style.buttonActive      = Color(185, 20, 50, 255);
            style.toggleActive      = Color(225, 32, 68, 240);
            style.toggleHover       = Color(250, 48, 88, 255);
            style.scrollbarBg       = Color(28, 22, 25, 160);
            style.scrollbarGrab     = Color(98, 76, 84, 225);
            style.secondaryBtn      = Color(43, 50, 64, 240);
            style.secondaryBtnHover = Color(58, 68, 86, 250);
            style.accent            = Color(255, 105, 130, 255);
        } else if (theme == Theme::CyberPurple) {
            style.windowBg      = Color(13, 15, 20, 255);
            style.panelBg       = Color(24, 27, 36, 145);
            style.childBg       = Color(20, 23, 30, 135);
            style.popupBg       = Color(28, 32, 42, 240);
            style.border        = Color(140, 110, 190, 85);
            style.borderFocus   = Color(186, 104, 200, 255);
            style.frameBg       = Color(32, 36, 46, 155);
            style.frameHover    = Color(46, 52, 66, 195);
            style.checkMark     = Color(206, 147, 216, 255);
            style.sliderGrab    = Color(186, 104, 200, 255);
            style.button        = Color(142, 44, 205, 235);
            style.buttonGradEnd = Color(92, 22, 148, 245);
            style.buttonHover   = Color(142, 36, 170, 245);
            style.buttonActive  = Color(74, 20, 140, 255);
            style.toggleActive  = Color(123, 31, 162, 240);
            style.toggleHover   = Color(156, 39, 176, 255);
            style.accent        = Color(206, 147, 216, 255);
        } else if (theme == Theme::RoseGold) {
            style.windowBg      = Color(12, 12, 15, 255);
            style.panelBg       = Color(26, 25, 30, 145);
            style.childBg       = Color(20, 19, 24, 135);
            style.popupBg       = Color(24, 23, 28, 240);
            style.border        = Color(255, 255, 255, 38);
            style.borderFocus   = Color(224, 128, 108, 230);
            style.frameBg       = Color(38, 36, 43, 150);
            style.frameHover    = Color(54, 50, 60, 190);
            style.checkMark     = Color(224, 128, 108, 255);
            style.sliderGrab    = Color(230, 136, 115, 255);
            style.button        = Color(214, 98, 96, 235);
            style.buttonGradEnd = Color(168, 66, 68, 245);
            style.buttonHover   = Color(230, 114, 112, 250);
            style.buttonActive  = Color(178, 68, 76, 255);
            style.toggleActive  = Color(214, 98, 96, 245);
            style.toggleHover   = Color(235, 118, 116, 255);
            style.accent        = Color(242, 168, 148, 255);
        } else if (theme == Theme::LightClean) {
            style.windowBg          = Color(210, 220, 234, 255);
            style.panelBg           = Color(234, 240, 248, 205);
            style.childBg           = Color(226, 234, 244, 190);
            style.popupBg           = Color(238, 243, 250, 245);
            style.tooltipBg         = Color(28, 34, 46, 240);
            style.border            = Color(155, 172, 196, 165);
            style.borderFocus       = Color(38, 106, 210, 255);
            style.text              = Color(26, 32, 44, 255);
            style.textDim           = Color(88, 98, 116, 255);
            style.frameBg           = Color(204, 215, 230, 200);
            style.frameHover        = Color(190, 204, 222, 225);
            style.checkMark         = Color(38, 106, 210, 255);
            style.sliderGrab        = Color(38, 106, 210, 255);
            style.button            = Color(188, 202, 222, 235);
            style.buttonGradEnd     = Color(24, 86, 192, 245);
            style.buttonHover       = Color(172, 188, 212, 250);
            style.buttonActive      = Color(156, 174, 202, 255);
            style.toggleActive      = Color(48, 122, 235, 245);
            style.toggleHover       = Color(62, 134, 242, 255);
            style.disabledBg        = Color(206, 216, 228, 180);
            style.disabledText      = Color(135, 146, 164, 255);
            style.scrollbarBg       = Color(204, 215, 230, 175);
            style.scrollbarGrab     = Color(148, 162, 184, 225);
            style.secondaryBtn      = Color(186, 198, 216, 240);
            style.secondaryBtnHover = Color(168, 184, 206, 250);
            style.success           = Color(32, 132, 52, 255);
            style.warning           = Color(180, 108, 0, 255);
            style.danger            = Color(195, 42, 42, 255);
            style.accent            = Color(28, 92, 196, 255);
        }
        if (!themeRegSubKey.empty()) {
            HKEY hKey = nullptr;
            if (RegCreateKeyExW(HKEY_CURRENT_USER, themeRegSubKey.c_str(), 0, nullptr,
                                REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
                DWORD val = static_cast<DWORD>(theme);
                RegSetValueExW(hKey, L"Theme", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&val), sizeof(val));
                RegCloseKey(hKey);
            }
        }
        if (hwnd) {
            BOOL useDark = (theme != Theme::LightClean) ? TRUE : FALSE;
            DwmSetWindowAttribute(hwnd, 20, &useDark, sizeof(useDark));
            DwmSetWindowAttribute(hwnd, 19, &useDark, sizeof(useDark));
        }
    }

    void EnableThemePersistence(const std::wstring& appName) {
        themeRegSubKey = L"Software\\ShawarmaUI\\" + appName;
        HKEY hKey = nullptr;
        DWORD val = 0;
        DWORD size = sizeof(val);
        if (RegOpenKeyExW(HKEY_CURRENT_USER, themeRegSubKey.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            if (RegQueryValueExW(hKey, L"Theme", nullptr, nullptr, reinterpret_cast<BYTE*>(&val), &size) == ERROR_SUCCESS && val <= 4) {
                ApplyTheme(static_cast<Theme>(val));
            }
            RegCloseKey(hKey);
        }
    }

    void CycleTheme() {
        int next = (static_cast<int>(currentTheme) + 1) % 5;
        ApplyTheme(static_cast<Theme>(next));
    }

    const char* GetCurrentThemeName() const {
        switch (currentTheme) {
            case Theme::NeonRed:     return "Neon Red";
            case Theme::CyberPurple: return "Cyber Purple";
            case Theme::RoseGold:    return "Rose Gold";
            case Theme::LightClean:  return "Light Clean";
            default:                 return "Teal Gold";
        }
    }

    bool ThemeSwitcherButton(int customW = 135, int customH = 24) {
        std::string label = std::string("Tema: ") + GetCurrentThemeName();
        if (Button(label, customW, customH)) {
            CycleTheme();
            RequestRedraw(true);
            return true;
        }
        return false;
    }

    bool Create(const std::string& titleUtf8, int baseW, int baseH, HICON customIcon = nullptr) {
        EnableDPIAwareness();
        dpiScale = GetSystemDpiScale();

        WNDCLASSEXW wc = { sizeof(wc), CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS, StaticWndProc, 0, 0,
                           GetModuleHandleW(nullptr), customIcon, LoadCursor(nullptr, IDC_ARROW),
                           nullptr, nullptr, L"ShawarmaUI_WindowClass", customIcon };
        RegisterClassExW(&wc);

        RECT workArea = {};
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
        int screenW = workArea.right - workArea.left;
        int screenH = workArea.bottom - workArea.top;

        int winW = std::min(static_cast<int>(baseW * dpiScale), screenW - 40);
        int winH = std::min(static_cast<int>(baseH * dpiScale), screenH - 40);
        int posX = workArea.left + (screenW - winW) / 2;
        int posY = workArea.top + (screenH - winH) / 2;

        std::wstring titleW = Utf8ToWide(titleUtf8);
        hwnd = CreateWindowW(wc.lpszClassName, titleW.c_str(),
                             WS_OVERLAPPEDWINDOW,
                             posX, posY, winW, winH, nullptr, nullptr, wc.hInstance, this);
        if (!hwnd) return false;

        DragAcceptFiles(hwnd, TRUE);
        BOOL useDark = (currentTheme != Theme::LightClean) ? TRUE : FALSE;
        DwmSetWindowAttribute(hwnd, 20, &useDark, sizeof(useDark));
        DwmSetWindowAttribute(hwnd, 19, &useDark, sizeof(useDark));

        InitGraphicsBackend();

        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
        return true;
    }

    void SwitchBackend(bool wantD2D) {
        pendingWantD2D = wantD2D;
        pendingBackendSwitch = true;
        RequestRedraw(true);
    }

    void Run(std::function<void(Context&)> onRender) {
        renderCallback = std::move(onRender);
        RequestRedraw(true);
        MSG msg;
        while (GetMessageW(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    void RequestRedraw(bool force = true) {
        if (!hwnd) return;
        DWORD now = GetTickCount();
        DWORD intervalMs = (style.animFps > 0) ? (1000u / static_cast<UINT>(style.animFps)) : 33u;
        if (force || (now - lastRedrawRequestMs >= intervalMs)) {
            lastRedrawRequestMs = now;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
    }

    bool HasActiveAnimations() const { return hasActiveAnimations || hasBackdropAnimation; }

    UINT GetNextFrameDelayMs() const {
        if (hasActiveAnimations) return (style.animFps > 0) ? std::max(10u, 1000u / static_cast<UINT>(style.animFps)) : 33u;
        if (hasBackdropAnimation) return (style.backdropFps > 0) ? std::max(16u, 1000u / static_cast<UINT>(style.backdropFps)) : 66u;
        return 0u;
    }

    // --- HOOKS PARA SWAPCHAIN / D2D EXTERNO ---

    bool AttachDXGISwapChain(HWND targetHwnd, IDXGISwapChain1* swapChain) {
        hwnd = targetHwnd;
        dpiScale = GetSystemDpiScale();
        externalSwapChain = swapChain;
        if (InitD2DFromSwapChain(swapChain)) {
            activeD2D = true;
            return true;
        }
        return false;
    }

    void ReleaseDXGISwapChain() {
        d2dBgBitmap.Reset();
        d2dBgTarget.Reset();
        d2dBgW = 0;
        d2dBgH = 0;
        if (!d2dContext) return;
        d2dContext->SetTarget(nullptr);
        d2dTargetBitmap.Reset();
    }

    void OnResizeDXGISwapChain() {
        if (!d2dContext || !externalSwapChain) return;
        CreateD2DTargetFromSwapChain(externalSwapChain);
    }

    void AttachD2DHwndTarget(HWND targetHwnd, ID2D1HwndRenderTarget* externalRT) {
        hwnd = targetHwnd;
        dpiScale = GetSystemDpiScale();
        d2dHwndTarget = externalRT;
        if (!d2dFactory || !dwriteFactory) {
            TryLoadD2DRuntime();
            RebuildDWriteFormats();
        }
        if (d2dHwndTarget && !d2dBrush) {
            d2dHwndTarget->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1), &d2dBrush);
        }
        activeD2D = (d2dHwndTarget != nullptr);
    }

    void ReleaseD2DHwndTarget() {
        d2dBgBitmap.Reset();
        d2dBgTarget.Reset();
        d2dBgW = 0;
        d2dBgH = 0;
        textLayoutCache.clear();
        d2dBrush.Reset();
        d2dHwndTarget.Reset();
        activeD2D = false;
    }

    void BeginExternalFrame(int width, int height) {
        clientW = width;
        clientH = height;
        if (!d2dContext) return;
        d2dContext->BeginDraw();
        d2dContext->SetTransform(D2D1::Matrix3x2F::Identity());
        BeginFrameState();
        DrawGlassBackdrop(width, height);
    }

    void EndExternalFrame() {
        if (!d2dContext) return;
        EndFrameState();
        d2dContext->EndDraw();
    }

    void BeginHUDFrame(int width, int height) {
        clientW = width;
        clientH = height;
        BeginFrameState();
    }

    void EndHUDFrame() {
        EndFrameState();
    }

    ID2D1RenderTarget* GetActiveD2DRenderTarget() const { return ActiveD2DTarget(); }

    bool HandleExternalMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
            case WM_MOUSEMOVE:
                mousePos = { static_cast<short>(LOWORD(lParam)), static_cast<short>(HIWORD(lParam)) };
                return IsAnyItemHovered() || activeDragId != 0 || openComboId != 0;
            case WM_MOUSELEAVE:
                mousePos = { -1, -1 };
                return false;
            case WM_MOUSEWHEEL:
                if (hoveredChildId != 0 || openComboId != 0) {
                    pendingWheelDelta += static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / static_cast<float>(WHEEL_DELTA);
                    return true;
                }
                return false;
            case WM_LBUTTONDOWN:
                mouseDown = true;
                mouseClicked = true;
                return IsAnyItemHovered() || activeDragId != 0 || openComboId != 0;
            case WM_LBUTTONUP: {
                bool wasDragging = (activeDragId != 0);
                mouseDown = false;
                activeDragId = 0;
                return IsAnyItemHovered() || wasDragging || openComboId != 0;
            }
            case WM_LBUTTONDBLCLK:
                mouseDoubleClicked = true;
                mouseClicked = true;
                return IsAnyItemHovered() || activeDragId != 0 || openComboId != 0;
            case WM_RBUTTONDOWN:
                rightMouseClicked = true;
                return IsAnyItemHovered();
            case WM_CHAR:
                if (focusedInputId != 0) {
                    pendingChars.push_back(static_cast<wchar_t>(wParam));
                    return true;
                }
                return false;
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
                if (wParam < keyDown.size()) {
                    if (!keyDown.at(wParam)) keyPressed.at(wParam) = true;
                    keyDown.at(wParam) = true;
                }
                if (focusedInputId != 0) {
                    pendingKeys.push_back(static_cast<UINT>(wParam));
                    return true;
                }
                if (openComboId != 0 && wParam == VK_ESCAPE) {
                    openComboId = 0;
                    return true;
                }
                return false;
            case WM_KEYUP:
            case WM_SYSKEYUP:
                if (wParam < keyDown.size()) {
                    keyDown.at(wParam) = false;
                }
                return false;
            case WM_KILLFOCUS:
                mouseDown = false;
                activeDragId = 0;
                focusedInputId = 0;
                openComboId = 0;
                mousePos = { -1, -1 };
                keyDown.fill(false);
                keyPressed.fill(false);
                return false;
        }
        return false;
    }

    bool IsAnyItemHovered() const {
        return GetHoveredRectIndex(mousePos) != -1 || hoveredChildId != 0;
    }

    void PushID(int intId) {
        uint32_t seed = idStack.empty() ? 2166136261u : idStack.back();
        seed ^= static_cast<uint32_t>(intId);
        seed *= 16777619u;
        idStack.push_back(seed);
    }

    void PushID(const std::string& strId) {
        uint32_t seed = idStack.empty() ? 2166136261u : idStack.back();
        for (unsigned char c : strId) {
            seed ^= c;
            seed *= 16777619u;
        }
        idStack.push_back(seed);
    }

    void PopID() {
        if (!idStack.empty()) idStack.pop_back();
    }

    bool IsItemHovered() const { return IsLastItemHovered(); }
    bool IsItemActive() const { return lastItemActive; }
    bool IsItemClicked(int button = 0) const {
        if (button == 0) return IsLastItemHovered() && mouseClicked;
        if (button == 1) return IsLastItemHovered() && rightMouseClicked;
        return false;
    }
    bool IsItemDoubleClicked() const {
        return IsLastItemHovered() && mouseDoubleClicked;
    }
    bool IsKeyPressed(int vkCode) const {
        return (vkCode >= 0 && vkCode < static_cast<int>(keyPressed.size())) ? keyPressed.at(static_cast<size_t>(vkCode)) : false;
    }
    bool IsKeyDown(int vkCode) const {
        return (vkCode >= 0 && vkCode < static_cast<int>(keyDown.size())) ? keyDown.at(static_cast<size_t>(vkCode)) : false;
    }

    // --- LAYOUT E CONTROLE FLUENTE ---

    int GetAvailWidth() const { return std::max(0, regionRight - cursorX); }

    int GetTextWidth(const std::string& textUtf8, bool bold = false) {
        int px = CalcTextWidth(StripIdSuffix(textUtf8), bold);
        return (dpiScale > 0.0f) ? static_cast<int>(std::ceil(px / dpiScale)) : px;
    }

    Context& SetCursorPos(int x, int y) {
        cursorX = x;
        cursorY = y;
        return *this;
    }

    int GetCursorX() const { return cursorX; }
    int GetCursorY() const { return cursorY; }
    int GetClientWidth() const { return clientW; }
    int GetClientHeight() const { return clientH; }
    int GetLogicalWidth() const { return (dpiScale > 0.0f) ? static_cast<int>(clientW / dpiScale) : clientW; }
    int GetLogicalHeight() const { return (dpiScale > 0.0f) ? static_cast<int>(clientH / dpiScale) : clientH; }

    Context& SameLineWrap(int nextItemWidth) {
        int nextX = lastItemRight + S(style.itemSpacingX);
        if (nextX + S(nextItemWidth) <= regionRight) {
            cursorX = nextX;
            cursorY = lastItemTop;
        } else {
            cursorX = regionLeft;
        }
        return *this;
    }

    Context& AlignTextToFramePadding() {
        alignTextToFrame = true;
        return *this;
    }

    Context& AlignText() {
        return AlignTextToFramePadding();
    }

    Context& SameLine(int offsetX = 0) {
        if (offsetX > 0) cursorX = regionLeft + S(offsetX);
        else cursorX = lastItemRight + S(style.itemSpacingX);
        cursorY = lastItemTop;
        return *this;
    }

    Context& SameLineRight(int offsetFromRight) {
        cursorX = std::max(regionLeft, regionRight - S(offsetFromRight));
        cursorY = lastItemTop;
        return *this;
    }

    Context& Spacing(int pixels = -1) {
        cursorY += (pixels < 0) ? S(style.itemSpacingY) : S(pixels);
        return *this;
    }

    Context& Separator() {
        int y = cursorY + S(2);
        int l = regionLeft, r = regionRight;
        Color c = style.border;
        QueueOrDraw([=, this]() { DrawLine(l, y, r, y, c); });
        cursorY += S(8);
        return *this;
    }

    Context& SeparatorText(const std::string& labelUtf8, Color labelCol = Color(0, 0, 0, 0)) {
        std::string disp = StripIdSuffix(labelUtf8);
        int h = S(20);
        int y = cursorY;
        int l = regionLeft, r = regionRight;
        int midY = y + h / 2;
        int textW = CalcTextWidth(disp, true);
        int pad = S(8);
        Color col = (labelCol.a > 0) ? labelCol : style.accent;
        Color lineCol = style.border;

        QueueOrDraw([=, this]() {
            int line1End = l + S(12);
            if (line1End > l) DrawLine(l, midY, line1End, midY, lineCol);
            int textX = line1End + pad;
            int drawW = std::min(textW + S(4), std::max(0, r - textX));
            DrawTextInternal(textX, y, drawW, h, disp, col, true, DT_LEFT);
            int line2Start = textX + drawW + pad;
            if (line2Start < r) DrawLine(line2Start, midY, r, midY, lineCol);
        });

        AdvanceCursor(r - l, h);
        return *this;
    }

    Context& Columns(int count) {
        if (count <= 1) {
            colCount = 1;
            colIndex = 0;
            regionLeft = savedRegionLeft;
            regionRight = savedRegionRight;
            cursorX = regionLeft;
            cursorY = colMaxY;
        } else {
            colCount = count;
            colIndex = 0;
            savedRegionLeft = regionLeft;
            savedRegionRight = regionRight;
            colStartY = cursorY;
            colMaxY = cursorY;
            UpdateColumnBounds();
        }
        return *this;
    }

    Context& NextColumn() {
        if (colCount <= 1) return *this;
        colMaxY = std::max(colMaxY, cursorY);
        colIndex = (colIndex + 1) % colCount;
        cursorY = colStartY;
        UpdateColumnBounds();
        return *this;
    }

    void BeginDisabled(bool disabled = true) { disabledStack.push_back(disabled); }
    void EndDisabled() { if (!disabledStack.empty()) disabledStack.pop_back(); }

    // --- CONTAINERS ---

    void BeginPanel(const std::string& titleUtf8 = "", Color titleColor = Color(0, 0, 0, 0), int customWidth = -1) {
        PanelState ps{};
        ps.startY = cursorY;
        ps.left = cursorX;
        ps.right = (customWidth > 0) ? (cursorX + S(customWidth)) : regionRight;
        ps.savedRegionLeft = regionLeft;
        ps.savedRegionRight = regionRight;
        ps.title = StripIdSuffix(titleUtf8);
        ps.titleColor = (titleColor.a > 0) ? titleColor : style.accent;
        panelStack.push_back(std::move(ps));

        int pad = S(10);
        regionLeft = panelStack.back().left + pad;
        regionRight = panelStack.back().right - pad;
        cursorX = regionLeft;
        cursorY += pad;

        if (!panelStack.back().title.empty()) {
            int availTitleW = std::max(S(24), regionRight - regionLeft);
            int logicalTitleW = (dpiScale > 0.0f) ? static_cast<int>(availTitleW / dpiScale) : availTitleW;
            panelStack.back().titleLines = WrapTextCached(panelStack.back().title, logicalTitleW, true);
            int rows = std::max(1, static_cast<int>(panelStack.back().titleLines.size()));
            cursorY += rows * S(20) + (rows - 1) * S(2) + S(4);
        }
	}

    void EndPanel() {
        if (panelStack.empty()) return;
        PanelState ps = std::move(panelStack.back());
        panelStack.pop_back();

        int pad = S(10);
        int panelBottom = cursorY + pad - S(style.itemSpacingY);
        int w = ps.right - ps.left;
        int h = panelBottom - ps.startY;
        int radius = S(style.rounding);
        Color bg = style.panelBg;
        Color bdr = style.border;

        auto drawSelf = [=, this]() {
            DrawGlassPanelRect(ps.left, ps.startY, w, h, radius, bg, bdr);
            if (!ps.titleLines.empty()) {
                int lineH = S(20);
                int innerGap = S(2);
                int titleY = ps.startY + S(8);
                for (size_t i = 0; i < ps.titleLines.size(); ++i) {
                    int ly = titleY + static_cast<int>(i) * (lineH + innerGap);
                    DrawTextInternal(ps.left + pad, ly, std::max(0, w - pad * 2), lineH,
                                     ps.titleLines.at(i), ps.titleColor, true, DT_LEFT);
                }
                int rows = static_cast<int>(ps.titleLines.size());
                int sepY = titleY + rows * lineH + (rows - 1) * innerGap + S(2);
                DrawLine(ps.left + pad, sepY, ps.right - pad, sepY, bdr);
            }
            for (auto& fn : ps.deferredDraws) fn();
        };

        if (inModalRender) {
            modalDraws.emplace_back(std::move(drawSelf));
        } else if (!panelStack.empty()) {
            panelStack.back().deferredDraws.emplace_back(std::move(drawSelf));
        } else {
            drawSelf();
        }

        regionLeft = ps.savedRegionLeft;
        regionRight = ps.savedRegionRight;
        cursorX = regionLeft;
        cursorY = panelBottom + S(style.itemSpacingY);
    }

    void BeginChild(const std::string& idStr, int customW = -1, int customH = 200, bool border = true) {
        int id = HashString("child_" + idStr);
        int w = (customW < 0) ? GetAvailWidth() : S(customW);
        int h = (customH <= 0) ? std::max(S(60), clientH - cursorY - S(style.windowPadding)) : S(customH);
        int x = cursorX, y = cursorY;

        float& scrollRef = childScrollMap.try_emplace(id, 0.0f).first->second;
        float maxPrev = childMaxScrollMap.try_emplace(id, 0.0f).first->second;

        bool hovered = !modalBlocking && (openComboId == 0) && IsMouseInRect(x, y, w, h);
        if (hovered) {
            hoveredChildId = id;
            if (std::abs(pendingWheelDelta) > 0.001f) {
                scrollRef -= pendingWheelDelta * S(36);
            }
        }

        float scrollY = std::clamp(scrollRef, 0.0f, maxPrev);
        scrollRef = scrollY;

        ChildState cs{};
        cs.id = id;
        cs.x = x;
        cs.y = y;
        cs.w = w;
        cs.h = h;
        cs.border = border;
        cs.savedRegionLeft = regionLeft;
        cs.savedRegionRight = regionRight;
        cs.savedCursorX = cursorX;
        cs.savedCursorY = cursorY;
        cs.startContentY = y + S(6) - static_cast<int>(std::round(scrollY));
        childStack.push_back(cs);

        Color bgCol = style.childBg;
        Color bdrCol = border ? style.border : style.childBg;
        int rad = S(style.rounding);
        QueueOrDraw([=, this]() {
            DrawRoundRect(x, y, w, h, rad, bgCol, bdrCol);
        });

        int pad = S(8);
        int scrollBarSpace = S(10);
        regionLeft = x + pad;
        regionRight = x + w - pad - scrollBarSpace;
        cursorX = regionLeft;
        cursorY = cs.startContentY;

        int cx = x + 1, cy = y + 1;
        int cw = std::max(0, w - 2), ch = std::max(0, h - 2);
        PushClipLogical(cx, cy, cw, ch);
        QueueOrDraw([=, this]() {
            ApplyClipRect({ cx, cy, cx + cw, cy + ch });
        });
    }

    void EndChild() {
        if (childStack.empty()) return;
        ChildState cs = childStack.back();
        childStack.pop_back();

        PopClipLogical();
        QueueOrDraw([=, this]() {
            RemoveClipRect();
        });

        int contentH = (cursorY - cs.startContentY) + S(6);
        int maxScroll = std::max(0, contentH - cs.h);
        childMaxScrollMap.insert_or_assign(cs.id, static_cast<float>(maxScroll));
        float& scrollY = childScrollMap.try_emplace(cs.id, 0.0f).first->second;
        scrollY = std::clamp(scrollY, 0.0f, static_cast<float>(maxScroll));

        if (maxScroll > 0 && cs.h > S(20)) {
            int barW = S(6);
            int barX = cs.x + cs.w - barW - S(3);
            int barY = cs.y + S(4);
            int barH = cs.h - S(8);

            float visibleRatio = std::clamp(static_cast<float>(cs.h) / static_cast<float>(contentH), 0.12f, 1.0f);
            int grabH = std::max(S(18), static_cast<int>(std::round(barH * visibleRatio)));
            float scrollRatio = scrollY / static_cast<float>(maxScroll);
            int grabY = barY + static_cast<int>(std::round((barH - grabH) * scrollRatio));

            int sbId = HashString("sb_" + std::to_string(cs.id));
            bool sbHovered = RegisterInteractive(barX - S(3), barY, barW + S(6), barH, false);
            if (sbHovered && mouseClicked) {
                activeDragId = sbId;
                childDragOffsetY = static_cast<float>(mousePos.y - grabY);
            }
            if (!mouseDown && activeDragId == sbId) {
                activeDragId = 0;
            }
            if (activeDragId == sbId && barH > grabH) {
                int newGrabY = std::clamp(static_cast<int>(mousePos.y) - static_cast<int>(std::round(childDragOffsetY)), barY, barY + barH - grabH);
                float newRatio = static_cast<float>(newGrabY - barY) / static_cast<float>(barH - grabH);
                scrollY = newRatio * static_cast<float>(maxScroll);
                grabY = newGrabY;
            }

            Color grabCol = (activeDragId == sbId || sbHovered) ? style.borderFocus : style.scrollbarGrab;
            Color sbBg = style.scrollbarBg;
            QueueOrDraw([=, this]() {
                DrawRoundRect(barX, barY, barW, barH, barW / 2, sbBg, sbBg);
                DrawRoundRect(barX, grabY, barW, grabH, barW / 2, grabCol, grabCol);
            });
        }

        regionLeft = cs.savedRegionLeft;
        regionRight = cs.savedRegionRight;
        cursorX = cs.savedCursorX;
        cursorY = cs.savedCursorY;
        AdvanceCursor(cs.w, cs.h);
    }

    // --- WIDGETS ---

    Context& Text(const std::string& textUtf8, bool bold = false) {
        return TextColored(style.text, textUtf8, bold);
    }

    Context& TextColored(Color color, const std::string& textUtf8, bool bold = false) {
        std::string disp = StripIdSuffix(textUtf8);
        int offsetY = alignTextToFrame ? S(4) : 0;
        alignTextToFrame = false;
        if (IsDisabled()) color = style.disabledText;
        else color = EnsureContrast(color);
        int h = S(20);
        int w = CalcTextWidth(disp, bold);
        int drawW = std::min(w + S(4), GetAvailWidth());
        int x = cursorX, y = cursorY + offsetY;

        QueueOrDraw([=, this]() {
            DrawTextInternal(x, y, drawW, h, disp, color, bold, DT_LEFT);
        });
        lastItemActive = false;
        AdvanceCursor(drawW, h);
        return *this;
    }

    Context& TextWrapped(const std::string& textUtf8, Color color = Color(0, 0, 0, 0), bool bold = false) {
    std::string disp = StripIdSuffix(textUtf8);
    Color col = (color.a > 0) ? color : style.text;
    if (IsDisabled()) col = style.disabledText;
    else col = EnsureContrast(col);

    int availW = GetAvailWidth();
    int availLogicalW = (dpiScale > 0.0f) ? static_cast<int>(availW / dpiScale) : availW;
    const auto& lines = WrapTextCached(disp, availLogicalW, bold);

    int lineH = S(20);
    int innerGap = S(2);
    int startX = cursorX;
    int startY = cursorY;

    for (size_t i = 0; i < lines.size(); ++i) {
        int ly = startY + static_cast<int>(i) * (lineH + innerGap);
        std::string lineStr = lines.at(i);
        QueueOrDraw([=, this]() {
            if (!lineStr.empty()) {
                DrawTextInternal(startX, ly, availW, lineH, lineStr, col, bold, DT_LEFT);
            }
        });
    }

    int totalH = lines.empty()
        ? lineH
        : static_cast<int>(lines.size()) * lineH + static_cast<int>(lines.size() - 1) * innerGap;
    lastItemActive = false;
    AdvanceCursor(availW, totalH);
    return *this;
    }

    Context& Badge(const std::string& textUtf8, Color accentColor = Color(0, 0, 0, 0)) {
        if (accentColor.a == 0) accentColor = style.accent;
        accentColor = EnsureContrast(accentColor);
        std::string disp = StripIdSuffix(textUtf8);
        int padX = S(8);
        int h = S(22);
        int textW = CalcTextWidth(disp, true);
        int w = std::min(textW + padX * 2, GetAvailWidth());
        int x = cursorX, y = cursorY;

        Color bgCol = Color::Lerp(style.panelBg, accentColor, 0.20f);
        Color borderCol = Color::Lerp(style.panelBg, accentColor, 0.68f);

        QueueOrDraw([=, this]() {
            DrawRoundRect(x, y, w, h, h / 2, bgCol, borderCol);
            DrawTextInternal(x + padX, y, std::max(0, w - padX * 2), h, disp, accentColor, true, DT_CENTER);
        });
        lastItemActive = false;
        AdvanceCursor(w, h);
        return *this;
    }

    Context& KeyValue(const std::string& keyUtf8, const std::string& valUtf8, Color valColor = Color(0, 0, 0, 0)) {
        std::string dispKey = StripIdSuffix(keyUtf8) + ": ";
        std::string dispVal = StripIdSuffix(valUtf8);
        int lineH = S(20);
        int innerGap = S(2);
        int x = cursorX, y = cursorY;
        int availW = GetAvailWidth();
        int keyW = std::min(CalcTextWidth(dispKey, false), availW);
        int valW = std::max(0, availW - keyW);

        Color kCol = IsDisabled() ? style.disabledText : style.textDim;
        Color vCol = IsDisabled() ? style.disabledText : ((valColor.a > 0) ? valColor : style.text);
        if (!IsDisabled()) vCol = EnsureContrast(vCol);

        int valLogicalW = (dpiScale > 0.0f) ? static_cast<int>(valW / dpiScale) : valW;
        if (valLogicalW < 40) valLogicalW = (dpiScale > 0.0f) ? static_cast<int>(availW / dpiScale) : availW;

        const auto& valLines = WrapTextCached(dispVal, valLogicalW, true);
        bool putOnNextLine = (valW < S(40)) && !dispVal.empty();

        QueueOrDraw([=, this]() {
            DrawTextInternal(x, y, keyW, lineH, dispKey, kCol, false, DT_LEFT);
            for (size_t i = 0; i < valLines.size(); ++i) {
                int vx = (i == 0 && !putOnNextLine) ? (x + keyW) : x;
                int vw = (i == 0 && !putOnNextLine) ? valW : availW;
                int vy = y + static_cast<int>(i + (putOnNextLine ? 1 : 0)) * (lineH + innerGap);
                if (!valLines.at(i).empty() && vw > 0) {
                    DrawTextInternal(vx, vy, vw, lineH, valLines.at(i), vCol, true, DT_LEFT);
                }
            }
        });

        int totalRows = std::max(1, static_cast<int>(valLines.size()) + (putOnNextLine ? 1 : 0));
        int totalH = totalRows * lineH + (totalRows - 1) * innerGap;
        lastItemActive = false;
        AdvanceCursor(availW, totalH);
        return *this;
    }

    Context& InputTextReadOnly(const std::string& textUtf8, int customWidth = -1) {
        int availW = GetAvailWidth();
        int w = (customWidth < 0) ? availW : std::min(S(customWidth), availW);
        int h = S(28);
        int x = cursorX, y = cursorY;

        QueueOrDraw([=, this]() {
            DrawRoundRect(x, y, w, h, S(style.rounding), style.frameBg, style.border);
            DrawTextInternal(x + S(8), y, w - S(16), h, textUtf8,
                             textUtf8.empty() ? style.textDim : style.text, false, DT_LEFT | DT_PATH_ELLIPSIS);
        });
        lastItemActive = false;
        AdvanceCursor(w, h);
        return *this;
    }

    bool InputText(const std::string& idStr, std::string& bufferUtf8, const std::string& placeholderUtf8 = "",
                   int customWidth = -1, bool isPassword = false) {
        int id = HashString("inp_" + idStr);
        int availW = GetAvailWidth();
        int w = (customWidth < 0) ? availW : std::min(S(customWidth), availW);
        int h = S(28);
        int x = cursorX, y = cursorY;
        bool disabled = IsDisabled();

        bool hovered = RegisterInteractive(x, y, w, h, disabled);

        if (mouseClicked && !disabled && !modalBlocking && openComboId == 0) {
            if (hovered) {
                focusedInputId = id;
                std::wstring wStr = Utf8ToWide(bufferUtf8);
                textCursorPos = wStr.size();
                cursorBlinkStart = GetTickCount();
            } else if (focusedInputId == id) {
                focusedInputId = 0;
            }
        }

        bool focused = (focusedInputId == id) && !disabled;
        bool changed = false;

        if (focused) {
            inputRenderedThisFrame = true;
            hasActiveAnimations = true;
            std::wstring wStr = Utf8ToWide(bufferUtf8);
            textCursorPos = std::min(textCursorPos, wStr.size());

            bool ctrlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;

            for (UINT vk : pendingKeys) {
                cursorBlinkStart = GetTickCount();
                if (vk == VK_LEFT && textCursorPos > 0) {
                    textCursorPos--;
                } else if (vk == VK_RIGHT && textCursorPos < wStr.size()) {
                    textCursorPos++;
                } else if (vk == VK_HOME) {
                    textCursorPos = 0;
                } else if (vk == VK_END) {
                    textCursorPos = wStr.size();
                } else if (vk == VK_DELETE && textCursorPos < wStr.size()) {
                    wStr.erase(textCursorPos, 1);
                    changed = true;
                } else if (vk == VK_ESCAPE || vk == VK_RETURN) {
                    focusedInputId = 0;
                    focused = false;
                } else if (ctrlDown && vk == 'V') {
                    std::wstring clip = GetClipboardTextW();
                    if (!clip.empty()) {
                        clip.erase(std::remove(clip.begin(), clip.end(), L'\r'), clip.end());
                        clip.erase(std::remove(clip.begin(), clip.end(), L'\n'), clip.end());
                        wStr.insert(textCursorPos, clip);
                        textCursorPos += clip.size();
                        changed = true;
                    }
                } else if (ctrlDown && vk == 'C' && !isPassword) {
                    SetClipboardTextW(wStr);
                }
            }
            pendingKeys.clear();

            for (wchar_t ch : pendingChars) {
                cursorBlinkStart = GetTickCount();
                if (ch == L'\b') {
                    if (textCursorPos > 0 && !wStr.empty()) {
                        wStr.erase(textCursorPos - 1, 1);
                        textCursorPos--;
                        changed = true;
                    }
                } else if (ch >= 32) {
                    wStr.insert(textCursorPos, 1, ch);
                    textCursorPos++;
                    changed = true;
                }
            }
            pendingChars.clear();

            if (changed) {
                bufferUtf8 = WideToUtf8(wStr);
            }
        }

        std::string displayStr = bufferUtf8;
        if (isPassword && !bufferUtf8.empty()) {
            std::wstring wStr = Utf8ToWide(bufferUtf8);
            displayStr = std::string(wStr.size(), '*');
        }

        int cursorPixelOffset = 0;
        if (focused) {
            std::wstring wDisp = Utf8ToWide(displayStr);
            size_t safePos = std::min(textCursorPos, wDisp.size());
            std::string beforeCursor = WideToUtf8(wDisp.substr(0, safePos));
            cursorPixelOffset = CalcTextWidth(beforeCursor, false);
        }

        float hoverT = Animate(id * 2, (hovered || focused) ? 1.0f : 0.0f, 16.0f);
        Color bgCol = disabled ? style.disabledBg : Color::Lerp(style.frameBg, style.frameHover, hoverT);
        Color borderCol = focused ? style.borderFocus : style.border;

        QueueOrDraw([=, this]() {
            DrawRoundRect(x, y, w, h, S(style.rounding), bgCol, borderCol);
            int padX = S(8);
            int innerW = std::max(0, w - padX * 2);

            PushClip(x + padX, y + 2, innerW, h - 4);
            int scrollTextX = 0;
            if (focused && cursorPixelOffset > innerW - S(4)) {
                scrollTextX = cursorPixelOffset - (innerW - S(4));
            }

            if (displayStr.empty() && !placeholderUtf8.empty()) {
                DrawTextInternal(x + padX, y, innerW, h, placeholderUtf8, style.textDim, false, DT_LEFT);
            } else if (!displayStr.empty()) {
                DrawTextInternal(x + padX - scrollTextX, y, innerW + scrollTextX + S(20), h, displayStr,
                                 disabled ? style.disabledText : style.text, false, DT_LEFT);
            }

            if (focused) {
                DWORD elapsed = GetTickCount() - cursorBlinkStart;
                if ((elapsed / 500) % 2 == 0) {
                    int cx = x + padX + cursorPixelOffset - scrollTextX;
                    DrawLine(cx, y + S(5), cx, y + h - S(5), style.text);
                }
            }
            PopClip();
        });

        lastItemActive = focused;
        AdvanceCursor(w, h);
        return changed;
    }
	
	bool InputTextMultiline(const std::string& idStr, std::string& bufferUtf8, int customWidth = -1,
                        int customHeight = 140, bool readOnly = false) {
    int id = HashString("ml_" + idStr);
    int availW = GetAvailWidth();
    int w = (customWidth < 0) ? availW : std::min(S(customWidth), availW);
    int h = (customHeight <= 0) ? std::max(S(80), clientH - cursorY - S(style.windowPadding)) : S(customHeight);
    int x = cursorX, y = cursorY;
    bool disabled = IsDisabled();

    float& scrollY = childScrollMap.try_emplace(id, 0.0f).first->second;
    bool hovered = RegisterInteractive(x, y, w, h, disabled);
    if (hovered) {
        hoveredChildId = id;
        if (std::abs(pendingWheelDelta) > 0.001f) {
            scrollY -= pendingWheelDelta * S(32);
        }
    }

    if (mouseClicked && !disabled && !readOnly && !modalBlocking && openComboId == 0) {
        if (hovered) {
            focusedInputId = id;
            cursorBlinkStart = GetTickCount();
        } else if (focusedInputId == id) {
            focusedInputId = 0;
        }
    }

    bool focused = (focusedInputId == id) && !disabled && !readOnly;
    bool changed = false;

    if (focused) {
        inputRenderedThisFrame = true;
        hasActiveAnimations = true;
        std::wstring wStr = Utf8ToWide(bufferUtf8);
        bool ctrlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;

        for (UINT vk : pendingKeys) {
            cursorBlinkStart = GetTickCount();
            if (vk == VK_ESCAPE) {
                focusedInputId = 0;
                focused = false;
            } else if (vk == VK_RETURN) {
                wStr.push_back(L'\n');
                changed = true;
            } else if (ctrlDown && vk == 'V') {
                std::wstring clip = GetClipboardTextW();
                if (!clip.empty()) {
                    clip.erase(std::remove(clip.begin(), clip.end(), L'\r'), clip.end());
                    wStr += clip;
                    changed = true;
                }
            } else if (ctrlDown && vk == 'C') {
                SetClipboardTextW(wStr);
            }
        }
        pendingKeys.clear();

        for (wchar_t ch : pendingChars) {
            cursorBlinkStart = GetTickCount();
            if (ch == L'\b') {
                if (!wStr.empty()) {
                    wStr.pop_back();
                    changed = true;
                }
            } else if (ch >= 32) {
                wStr.push_back(ch);
                changed = true;
            }
        }
        pendingChars.clear();

        if (changed) {
            bufferUtf8 = WideToUtf8(wStr);
        }
    }

    int pad = S(8);
    int scrollBarSpace = S(10);
    int innerW = std::max(S(24), w - pad * 2 - scrollBarSpace);
    int innerLogicalW = (dpiScale > 0.0f) ? static_cast<int>(innerW / dpiScale) : innerW;
    const auto& lines = WrapTextCached(bufferUtf8, innerLogicalW, false);

    int lineH = S(20);
    int totalContentH = static_cast<int>(lines.size()) * lineH + pad * 2;
    int maxScroll = std::max(0, totalContentH - h);
    if (changed) scrollY = static_cast<float>(maxScroll);
    scrollY = std::clamp(scrollY, 0.0f, static_cast<float>(maxScroll));

    float hoverT = Animate(id * 2, (hovered || focused) ? 1.0f : 0.0f, 16.0f);
    Color bgCol = disabled ? style.disabledBg : Color::Lerp(style.frameBg, style.frameHover, hoverT);
    Color borderCol = focused ? style.borderFocus : style.border;

    QueueOrDraw([=, this]() {
        DrawRoundRect(x, y, w, h, S(style.rounding), bgCol, borderCol);
        PushClip(x + 2, y + 2, std::max(0, w - 4), std::max(0, h - 4));

        int drawY = y + pad - static_cast<int>(std::round(scrollY));
        Color txtCol = disabled ? style.disabledText : style.text;

        for (size_t i = 0; i < lines.size(); ++i) {
            int ly = drawY + static_cast<int>(i) * lineH;
            if (ly + lineH >= y && ly <= y + h && !lines.at(i).empty()) {
                DrawTextInternal(x + pad, ly, innerW, lineH, lines.at(i), txtCol, false, DT_LEFT);
            }
        }

        if (focused) {
            DWORD elapsed = GetTickCount() - cursorBlinkStart;
            if ((elapsed / 500) % 2 == 0) {
                const std::string& lastLine = lines.empty() ? std::string() : lines.back();
                int lastLineY = drawY + std::max(0, static_cast<int>(lines.size()) - 1) * lineH;
                int cx = x + pad + CalcTextWidth(lastLine, false);
                DrawLine(cx, lastLineY + S(2), cx, lastLineY + lineH - S(2), style.text);
            }
        }

        if (maxScroll > 0 && h > S(20)) {
            int barW = S(5);
            int barX = x + w - barW - S(3);
            int barY = y + S(4);
            int barH = h - S(8);
            float visibleRatio = std::clamp(static_cast<float>(h) / static_cast<float>(totalContentH), 0.12f, 1.0f);
            int grabH = std::max(S(16), static_cast<int>(std::round(barH * visibleRatio)));
            float scrollRatio = scrollY / static_cast<float>(maxScroll);
            int grabY = barY + static_cast<int>(std::round((barH - grabH) * scrollRatio));
            DrawRoundRect(barX, barY, barW, barH, barW / 2, style.scrollbarBg, style.scrollbarBg);
            DrawRoundRect(barX, grabY, barW, grabH, barW / 2, style.scrollbarGrab, style.scrollbarGrab);
        }

        PopClip();
    });

    lastItemActive = focused;
    AdvanceCursor(w, h);
    return changed;
    }

    bool Checkbox(const std::string& labelUtf8, bool& value) {
        int id = MakeWidgetId("chk_" + labelUtf8);
        std::string disp = StripIdSuffix(labelUtf8);
        int boxSz = S(18);
        int h = S(22);
        int textW = CalcTextWidth(disp, false);
        int totalW = std::min(boxSz + S(8) + textW + S(4), GetAvailWidth());
        int x = cursorX, y = cursorY;
        bool disabled = IsDisabled();

        bool hovered = RegisterInteractive(x, y, totalW, h, disabled);
        bool clicked = hovered && mouseClicked;
        if (clicked) value = !value;

        float hoverT = Animate(id * 2, hovered ? 1.0f : 0.0f, 15.0f);
        float checkT = Animate(id * 2 + 1, value ? 1.0f : 0.0f, 18.0f);

        Color bgCol = disabled ? style.disabledBg : Color::Lerp(style.frameBg, style.frameHover, hoverT);
        Color txtCol = disabled ? style.disabledText : style.text;
        Color baseChk = disabled ? style.disabledText : style.checkMark;
        Color chkCol = Color::Lerp(bgCol, baseChk, checkT);

        QueueOrDraw([=, this]() {
            int boxY = y + (h - boxSz) / 2;
            DrawRoundRect(x, boxY, boxSz, boxSz, S(4), bgCol, style.border);
            if (checkT > 0.02f) {
                DrawCheckmarkCached(x, boxY, boxSz, bgCol, chkCol);
            }
            DrawTextInternal(x + boxSz + S(8), y, totalW - boxSz - S(8), h, disp, txtCol, false, DT_LEFT);
        });

        lastItemActive = hovered && mouseDown;
        AdvanceCursor(totalW, h);
        return clicked;
    }

    bool ToggleSwitch(const std::string& labelUtf8, bool& value) {
        int id = MakeWidgetId("tgl_" + labelUtf8);
        std::string disp = StripIdSuffix(labelUtf8);
        int swW = S(36);
        int swH = S(20);
        int h = S(22);
        int textW = disp.empty() ? 0 : (CalcTextWidth(disp, false) + S(8));
        int totalW = std::min(swW + textW + S(4), GetAvailWidth());
        int x = cursorX, y = cursorY;
        bool disabled = IsDisabled();

        bool hovered = RegisterInteractive(x, y, totalW, h, disabled);
        bool clicked = hovered && mouseClicked;
        if (clicked) value = !value;

        float hoverT = Animate(id * 2, hovered ? 1.0f : 0.0f, 15.0f);
        float switchT = Animate(id * 2 + 1, value ? 1.0f : 0.0f, 18.0f);

        Color offBg = Color::Lerp(style.frameBg, style.frameHover, hoverT);
        Color onBg  = Color::Lerp(style.toggleActive, style.toggleHover, hoverT);
        Color trackCol = disabled ? style.disabledBg : Color::Lerp(offBg, onBg, switchT);
        Color knobCol  = disabled ? style.disabledText : Color::Lerp(style.textDim, style.text, switchT);
        Color txtCol   = disabled ? style.disabledText : style.text;

        QueueOrDraw([=, this]() {
            int swY = y + (h - swH) / 2;
            DrawRoundRect(x, swY, swW, swH, swH / 2, trackCol, disabled ? style.border : trackCol);

            int pad = S(3);
            int knobSz = swH - pad * 2;
            int maxTravel = swW - knobSz - pad * 2;
            int knobX = x + pad + static_cast<int>(std::round(maxTravel * switchT));
            int knobY = swY + pad;
            DrawRoundRect(knobX, knobY, knobSz, knobSz, knobSz / 2, knobCol, knobCol);

            if (!disp.empty() && totalW > swW + S(8)) {
                DrawTextInternal(x + swW + S(8), y, totalW - swW - S(8), h, disp, txtCol, false, DT_LEFT);
            }
        });

        lastItemActive = hovered && mouseDown;
        AdvanceCursor(totalW, h);
        return clicked;
    }

    bool Button(const std::string& labelUtf8, int customW = 0, int customH = 0,
            Color customBg = Color(0, 0, 0, 0), Color customHover = Color(0, 0, 0, 0)) {
        int id = MakeWidgetId("btn_" + labelUtf8);
        std::string disp = StripIdSuffix(labelUtf8);
        int availW = GetAvailWidth();
        int w = (customW > 0) ? std::min(S(customW), availW)
                              : std::min(CalcTextWidth(disp, true) + S(style.framePaddingX * 2), availW);
        if (customW == -1) w = availW;
        int h = (customH > 0) ? S(customH) : S(28);
        int x = cursorX, y = cursorY;
        bool disabled = IsDisabled();

        bool hovered = RegisterInteractive(x, y, w, h, disabled);
        bool pressed = hovered && mouseDown;
        bool clicked = hovered && mouseClicked;

        float hoverT = Animate(id * 2, (hovered || pressed) ? 1.0f : 0.0f, 15.0f);
        float pressT = Animate(id * 2 + 1, pressed ? 1.0f : 0.0f, 22.0f);

        Color cNormal = (customBg.a > 0) ? customBg : style.button;
        Color cHover  = (customHover.a > 0) ? customHover : style.buttonHover;
        Color activeCol = Color::Lerp(Color::Lerp(cNormal, cHover, hoverT), style.buttonActive, pressT);
        Color bgCol   = disabled ? style.disabledBg : activeCol;

        float luma = 0.299f * bgCol.r + 0.587f * bgCol.g + 0.114f * bgCol.b;
        Color autoTxt = (luma < 145.0f) ? Color(246, 243, 244, 255) : Color(26, 32, 44, 255);
        Color txtCol  = disabled ? style.disabledText : autoTxt;

        QueueOrDraw([=, this]() {
            if (!disabled && customBg.a == 0) {
                Color endCol = Color::Lerp(style.buttonGradEnd, style.buttonHover, hoverT);
                DrawMeshRect(x, y, w, h, S(style.rounding), bgCol, endCol, bgCol);
            } else {
                DrawRoundRect(x, y, w, h, S(style.rounding), bgCol, disabled ? style.border : bgCol);
            }
            int pad = S(4);
            PushClip(x + pad, y, std::max(0, w - pad * 2), h);
            DrawTextInternal(x + pad, y, std::max(0, w - pad * 2), h, disp, txtCol, true, DT_CENTER);
            PopClip();
        });

        lastItemActive = pressed;
        AdvanceCursor(w, h);
        return clicked;
    }

    bool ToggleButton(const std::string& labelUtf8, bool active, int customW = 0, int customH = 0) {
        if (active) {
            return Button(labelUtf8, customW, customH, style.toggleActive, style.toggleHover);
        }
        return Button(labelUtf8, customW, customH);
    }

    bool IconButton(int customW, int customH, std::function<void(int x, int y, int w, int h)> onDrawIcon,
                    Color customBg = Color(0, 0, 0, 0), Color customHover = Color(0, 0, 0, 0)) {
        int id = MakeWidgetId("iconbtn");
        int w = S(customW);
        int h = S(customH);
        int x = cursorX, y = cursorY;
        bool disabled = IsDisabled();

        bool hovered = RegisterInteractive(x, y, w, h, disabled);
        bool pressed = hovered && mouseDown;
        bool clicked = hovered && mouseClicked;

        float hoverT = Animate(id * 2, (hovered || pressed) ? 1.0f : 0.0f, 15.0f);
        float pressT = Animate(id * 2 + 1, pressed ? 1.0f : 0.0f, 22.0f);

        Color cNormal = (customBg.a > 0) ? customBg : style.button;
        Color cHover  = (customHover.a > 0) ? customHover : style.buttonHover;
        Color activeCol = Color::Lerp(Color::Lerp(cNormal, cHover, hoverT), style.buttonActive, pressT);
        Color bgCol   = disabled ? style.disabledBg : activeCol;

        QueueOrDraw([=, this]() {
            if (!disabled) {
                Color endCol = (customBg.a == 0)
                    ? Color::Lerp(style.buttonGradEnd, style.buttonHover, hoverT)
                    : Color::Lerp(bgCol, Color(0, 0, 0, bgCol.a), 0.25f);
                DrawMeshRect(x, y, w, h, S(style.rounding), bgCol, endCol, bgCol);
            } else {
                DrawRoundRect(x, y, w, h, S(style.rounding), bgCol, style.border);
            }
            if (onDrawIcon) onDrawIcon(x, y, w, h);
        });

        lastItemActive = pressed;
        AdvanceCursor(w, h);
        return clicked;
    }
	
	int ButtonRow(const std::vector<std::string>& labels, int customHeight = 28) {
    if (labels.empty()) return -1;
    int count = static_cast<int>(labels.size());
    int totalW = GetAvailWidth();
    int spacing = S(style.itemSpacingX);
    int btnW = std::max(S(28), (totalW - spacing * (count - 1)) / count);
    int startX = cursorX;
    int startY = cursorY;
    int clickedIndex = -1;

    for (int i = 0; i < count; ++i) {
        cursorX = startX + i * (btnW + spacing);
        cursorY = startY;
        int curW = (i == count - 1) ? std::max(btnW, (startX + totalW) - cursorX) : btnW;
        int logicalW = (dpiScale > 0.0f) ? static_cast<int>(std::round(curW / dpiScale)) : curW;
        if (Button(labels.at(static_cast<size_t>(i)), logicalW, customHeight)) {
            clickedIndex = i;
        }
    }
    return clickedIndex;
    }

    bool TabBar(int& currentTab, const std::vector<std::string>& labels, int customHeight = 30) {
        if (labels.empty()) return false;
        int count = static_cast<int>(labels.size());
        int totalW = GetAvailWidth();
        int h = S(customHeight);
        int x = cursorX, y = cursorY;
        int spacing = S(4);
        int tabW = std::max(S(30), (totalW - spacing * (count - 1)) / count);
        bool disabled = IsDisabled();
        bool changed = false;

        for (int i = 0; i < count; ++i) {
            int id = MakeWidgetId("tab_" + std::to_string(i));
            std::string disp = StripIdSuffix(labels.at(static_cast<size_t>(i)));
            int tx = x + i * (tabW + spacing);
            int curW = (i == count - 1) ? std::max(tabW, (x + totalW) - tx) : tabW;
            bool isSelected = (currentTab == i);

            bool hovered = RegisterInteractive(tx, y, curW, h, disabled);
            if (hovered && mouseClicked && currentTab != i) {
                currentTab = i;
                changed = true;
            }

            float hoverT = Animate(id * 2, hovered ? 1.0f : 0.0f, 16.0f);
            float activeT = Animate(id * 2 + 1, isSelected ? 1.0f : 0.0f, 18.0f);

            Color baseBg = Color::Lerp(style.frameBg, style.frameHover, hoverT);
            Color activeBg = Color::Lerp(style.toggleActive, style.toggleHover, hoverT);
            Color bgCol = disabled ? style.disabledBg : Color::Lerp(baseBg, activeBg, activeT);
            Color selectedTxt = (currentTheme == Theme::LightClean) ? Color(255, 255, 255, 255) : style.text;
            Color txtCol = disabled ? style.disabledText : (isSelected ? selectedTxt : Color::Lerp(style.textDim, style.text, hoverT));

            QueueOrDraw([=, this]() {
                if (!disabled && isSelected) {
                    DrawMeshRect(tx, y, curW, h, S(style.rounding), bgCol, style.buttonGradEnd, style.borderFocus);
                } else {
                    DrawRoundRect(tx, y, curW, h, S(style.rounding), bgCol, style.border);
                }
                int pad = S(4);
                    PushClip(tx + pad, y, std::max(0, curW - pad * 2), h);
                    DrawTextInternal(tx + pad, y, std::max(0, curW - pad * 2), h, disp, txtCol, isSelected, DT_CENTER);
                    PopClip();
            });
        }

        lastItemActive = false;
        AdvanceCursor(totalW, h);
        return changed;
    }

    bool SegmentedControl(int& currentIndex, const std::vector<std::string>& options, int customWidth = -1, int customHeight = 28) {
        if (options.empty()) return false;
        int count = static_cast<int>(options.size());
        int totalW = (customWidth < 0) ? GetAvailWidth() : S(customWidth);
        int h = S(customHeight);
        int x = cursorX, y = cursorY;
        int segW = std::max(S(24), totalW / count);
        bool disabled = IsDisabled();
        bool changed = false;

        QueueOrDraw([=, this]() {
            DrawRoundRect(x, y, totalW, h, S(style.rounding), style.frameBg, style.border);
        });

        for (int i = 0; i < count; ++i) {
            int id = MakeWidgetId("seg_" + std::to_string(i));
            std::string disp = StripIdSuffix(options.at(static_cast<size_t>(i)));
            int sx = x + i * segW;
            int curW = (i == count - 1) ? std::max(segW, (x + totalW) - sx) : segW;
            bool isSelected = (currentIndex == i);

            bool hovered = RegisterInteractive(sx, y, curW, h, disabled);
            if (hovered && mouseClicked && currentIndex != i) {
                currentIndex = i;
                changed = true;
            }

            float hoverT = Animate(id * 2, hovered ? 1.0f : 0.0f, 16.0f);
            float activeT = Animate(id * 2 + 1, isSelected ? 1.0f : 0.0f, 18.0f);

            Color activeBg = Color::Lerp(style.toggleActive, style.toggleHover, hoverT);
            Color hoverBg  = Color(style.frameHover.r, style.frameHover.g, style.frameHover.b, static_cast<uint8_t>(hoverT * 180));
            Color segCol   = disabled ? style.disabledBg : Color::Lerp(hoverBg, activeBg, activeT);
            Color selectedTxt = (currentTheme == Theme::LightClean) ? Color(255, 255, 255, 255) : style.text;
            Color txtCol = disabled ? style.disabledText : (isSelected ? selectedTxt : Color::Lerp(style.textDim, style.text, hoverT));

            QueueOrDraw([=, this]() {
                if (activeT > 0.02f || hoverT > 0.02f) {
                    int pad = S(2);
                    int rx = sx + pad, ry = y + pad;
                    int rw = std::max(0, curW - pad * 2), rh = std::max(0, h - pad * 2);
                    int rr = std::max(2, S(style.rounding) - 2);
                    if (!disabled && isSelected) {
                        DrawMeshRect(rx, ry, rw, rh, rr, segCol, style.buttonGradEnd, Color(0, 0, 0, 0));
                    } else {
                        DrawRoundRect(rx, ry, rw, rh, rr, segCol, segCol);
                    }
                }
                DrawTextInternal(sx, y, curW, h, disp, txtCol, isSelected, DT_CENTER);
            });
        }

        lastItemActive = false;
        AdvanceCursor(totalW, h);
        return changed;
    }

    bool SelectableRow(const std::string& titleUtf8, const std::string& subtitleUtf8 = "", bool selected = false, int customHeight = 36) {
        int id = MakeWidgetId("row_" + titleUtf8);
        std::string dispTitle = StripIdSuffix(titleUtf8);
        std::string dispSub = StripIdSuffix(subtitleUtf8);
        int w = GetAvailWidth();
        int h = S(customHeight);
        int x = cursorX, y = cursorY;
        bool disabled = IsDisabled();

        bool hovered = RegisterInteractive(x, y, w, h, disabled);
        bool clicked = hovered && mouseClicked;

        float hoverT = Animate(id * 2, hovered ? 1.0f : 0.0f, 16.0f);
        float selT   = Animate(id * 2 + 1, selected ? 1.0f : 0.0f, 18.0f);

        Color baseCol = Color::Lerp(style.panelBg, style.frameHover, hoverT);
        Color selCol  = Color::Lerp(style.toggleActive, style.toggleHover, hoverT);
        Color bgCol   = disabled ? style.disabledBg : Color::Lerp(baseCol, selCol, selT * 0.45f);
        Color bdrCol  = selected ? style.borderFocus : style.border;

        QueueOrDraw([=, this]() {
            DrawRoundRect(x, y, w, h, S(style.rounding), bgCol, bdrCol);
            int padX = S(10);
            int subW = dispSub.empty() ? 0 : (CalcTextWidth(dispSub, false) + S(12));
            int titleW = std::max(0, w - padX * 2 - subW);
            DrawTextInternal(x + padX, y, titleW, h, dispTitle, disabled ? style.disabledText : style.text, true, DT_LEFT);
            if (subW > 0) {
                DrawTextInternal(x + w - padX - subW, y, subW, h, dispSub, style.textDim, false, DT_RIGHT);
            }
        });

        lastItemActive = hovered && mouseDown;
        AdvanceCursor(w, h);
        return clicked;
    }

    bool Combo(const std::string& idStr, int& currentIndex, const std::vector<std::string>& items, int customWidth = -1) {
        int id = HashString("cmb_" + idStr);
        int availW = GetAvailWidth();
        int w = (customWidth < 0) ? availW : std::min(S(customWidth), availW);
        int h = S(28);
        int x = cursorX, y = cursorY;
        bool disabled = IsDisabled();

        bool isOpen = (openComboId == id);
        bool canInteract = !disabled && !modalBlocking && (openComboId == 0 || isOpen);

        if (canInteract && IsInsideActiveClip(x, y, w, h)) {
            interactiveRects.push_back({ x, y, x + w, y + h });
        }

        bool hovered = canInteract && IsMouseInRect(x, y, w, h) && IsInsideActiveClip(x, y, w, h);
        if (hovered && mouseClicked) {
            openComboId = isOpen ? 0 : id;
            isOpen = (openComboId == id);
            mouseClicked = false;
        }

        float hoverT = Animate(id * 2, (hovered || isOpen) ? 1.0f : 0.0f, 16.0f);
        Color bgCol = disabled ? style.disabledBg : Color::Lerp(style.frameBg, style.frameHover, hoverT);
        Color borderCol = isOpen ? style.borderFocus : style.border;

        std::string preview = (!items.empty() && currentIndex >= 0 && currentIndex < static_cast<int>(items.size()))
                              ? StripIdSuffix(items.at(static_cast<size_t>(currentIndex))) : "Selecionar...";

        QueueOrDraw([=, this]() {
            DrawRoundRect(x, y, w, h, S(style.rounding), bgCol, borderCol);
            int arrowW = S(22);
            int textW = std::max(0, w - arrowW - S(8));
            PushClip(x + S(8), y, textW, h);
            DrawTextInternal(x + S(8), y, textW, h, preview,
                             disabled ? style.disabledText : style.text, false, DT_LEFT);
            PopClip();
            DrawTextInternal(x + w - arrowW, y, arrowW, h, isOpen ? "▲" : "▼",
                             disabled ? style.disabledText : style.textDim, false, DT_CENTER);
        });

        bool selectionChanged = false;
        if (isOpen && !items.empty()) {
            int itemH = S(26);
            int dropH = static_cast<int>(items.size()) * itemH + S(6);
            int dropY = y + h + S(2);
            if (dropY + dropH > clientH - S(4) && y - dropH - S(2) >= S(4)) {
                dropY = y - dropH - S(2);
            }

            interactiveRects.push_back({ x, dropY, x + w, dropY + dropH });

            int hoveredOption = -1;
            for (int i = 0; i < static_cast<int>(items.size()); ++i) {
                int optY = dropY + S(3) + i * itemH;
                if (IsMouseInRect(x + S(3), optY, w - S(6), itemH)) {
                    hoveredOption = i;
                    if (mouseClicked) {
                        if (currentIndex != i) {
                            currentIndex = i;
                            selectionChanged = true;
                        }
                        openComboId = 0;
                        mouseClicked = false;
                    }
                }
            }

            if (mouseClicked && !IsMouseInRect(x, dropY, w, dropH) && !hovered) {
                openComboId = 0;
            }

            comboOverlayDraws.push_back([=, this]() {
                DrawRoundRect(x + S(2), dropY + S(3), w, dropH, S(style.rounding), Color(10, 10, 14, 210), Color(10, 10, 14, 210));
                DrawRoundRect(x, dropY, w, dropH, S(style.rounding), style.popupBg, style.borderFocus);
                for (int i = 0; i < static_cast<int>(items.size()); ++i) {
                    int optY = dropY + S(3) + i * itemH;
                    if (i == hoveredOption) {
                        DrawRoundRect(x + S(3), optY, w - S(6), itemH, S(4), style.buttonHover, style.buttonHover);
                    } else if (i == currentIndex) {
                        DrawRoundRect(x + S(3), optY, w - S(6), itemH, S(4), style.frameHover, style.frameHover);
                    }
                    DrawTextInternal(x + S(10), optY, std::max(0, w - S(20)), itemH, StripIdSuffix(items.at(static_cast<size_t>(i))), style.text, i == currentIndex, DT_LEFT);
                }
            });
        }

        lastItemActive = isOpen;
        AdvanceCursor(w, h);
        return selectionChanged;
    }

    bool SliderFloat(float& value, float minVal, float maxVal, int customWidth = -1, int customHeight = 16,
                     const std::string& overlayUtf8 = "", bool* outIsDragging = nullptr) {
        int id = MakeWidgetId("sldf");
        int w = (customWidth < 0) ? GetAvailWidth() : S(customWidth);
        int h = S(customHeight);
        int x = cursorX, y = cursorY;
        bool disabled = IsDisabled();

        bool hovered = RegisterInteractive(x, y, w, h, disabled);
        if (hovered && mouseClicked) {
            activeDragId = id;
        }
        if (!mouseDown && activeDragId == id) {
            activeDragId = 0;
        }

        bool dragging = (activeDragId == id);
        if (outIsDragging) *outIsDragging = dragging;

        bool changed = false;
        if (dragging && w > 0 && maxVal > minVal) {
            float t = std::clamp(static_cast<float>(mousePos.x - x) / static_cast<float>(w), 0.0f, 1.0f);
            float newVal = minVal + t * (maxVal - minVal);
            if (std::abs(newVal - value) > 0.0001f) {
                value = newVal;
                changed = true;
            }
        }

        float norm = (maxVal > minVal) ? std::clamp((value - minVal) / (maxVal - minVal), 0.0f, 1.0f) : 0.0f;
        float hoverT = Animate(id * 2, (hovered || dragging) ? 1.0f : 0.0f, 16.0f);

        Color trackCol = disabled ? style.disabledBg : Color::Lerp(style.frameBg, style.frameHover, hoverT);
        Color fillCol  = disabled ? style.disabledText : style.sliderGrab;

        QueueOrDraw([=, this]() {
            int trackH = (h > S(18)) ? h : std::max(S(6), h / 2);
            int trackY = y + (h - trackH) / 2;
            DrawRoundRect(x, trackY, w, trackH, trackH / 2, trackCol, style.border);

            int fillW = static_cast<int>(std::round(w * norm));
            if (fillW > 0) {
                DrawRoundRect(x, trackY, fillW, trackH, trackH / 2, fillCol, fillCol);
            }

            if (!overlayUtf8.empty()) {
                DrawTextInternal(x, y, w, h, overlayUtf8, style.text, true, DT_CENTER);
            }
        });

        lastItemActive = dragging;
        AdvanceCursor(w, h);
        return changed;
    }

    bool SliderInt(int& value, int minVal, int maxVal, int customWidth = -1, int customHeight = 18,
                   const std::string& suffixUtf8 = "") {
        float fVal = static_cast<float>(value);
        std::string overlay = std::to_string(value) + suffixUtf8;
        bool changed = SliderFloat(fVal, static_cast<float>(minVal), static_cast<float>(maxVal),
                                   customWidth, customHeight, overlay, nullptr);
        int rounded = static_cast<int>(std::round(fVal));
        if (rounded != value) {
            value = std::clamp(rounded, minVal, maxVal);
            return true;
        }
        return changed;
    }

    Context& ProgressBar(float fraction, int customWidth = -1, int customHeight = 16,
                         Color fillCol = Color(0, 0, 0, 0), const std::string& overlayUtf8 = "") {
        fraction = std::clamp(fraction, 0.0f, 1.0f);
        int w = (customWidth < 0) ? GetAvailWidth() : S(customWidth);
        int h = S(customHeight);
        int x = cursorX, y = cursorY;
        Color barCol = (fillCol.a > 0) ? fillCol : style.button;

        QueueOrDraw([=, this]() {
            DrawRoundRect(x, y, w, h, S(3), style.frameBg, style.border);
            int fillW = static_cast<int>(std::round((w - 2) * fraction));
            if (fillW > 0) {
                DrawRoundRect(x + 1, y + 1, fillW, h - 2, S(3), barCol, barCol);
            }
            if (!overlayUtf8.empty()) {
                DrawTextInternal(x, y, w, h, overlayUtf8, style.text, true, DT_CENTER);
            }
        });

        lastItemActive = false;
        AdvanceCursor(w, h);
        return *this;
    }

    Context& Spinner(int diameter = 20, Color color = Color(0, 0, 0, 0)) {
        hasActiveAnimations = true;
        int sz = S(diameter);
        int x = cursorX, y = cursorY;
        Color col = (color.a > 0) ? color : style.accent;
        float angle = (GetTickCount() % 1000) * 0.0062831853f;

        QueueOrDraw([=, this]() {
            float cx = x + sz * 0.5f;
            float cy = y + sz * 0.5f;
            float r = sz * 0.38f;
            const int dots = 8;
            for (int i = 0; i < dots; ++i) {
                float a = angle - i * 0.65f;
                float alphaRatio = 1.0f - (static_cast<float>(i) / static_cast<float>(dots));
                Color dotCol(col.r, col.g, col.b, static_cast<uint8_t>(col.a * alphaRatio));
                int dotR = std::max(2, sz / 9);
                int dx = static_cast<int>(std::round(cx + std::cos(a) * r)) - dotR;
                int dy = static_cast<int>(std::round(cy + std::sin(a) * r)) - dotR;
                DrawRoundRect(dx, dy, dotR * 2, dotR * 2, dotR, dotCol, dotCol);
            }
        });

        lastItemActive = false;
        AdvanceCursor(sz, sz);
        return *this;
    }

    void Toast(const std::string& textUtf8, Color accentColor = Color(0, 0, 0, 0), DWORD durationMs = 2500) {
        if (textUtf8.empty()) return;
        toastText = textUtf8;
        toastColor = (accentColor.a > 0) ? accentColor : style.accent;
        toastStartTime = GetTickCount();
        toastDurationMs = durationMs;
        hasActiveAnimations = true;
        RequestRedraw(true);
    }

    bool IsLastItemHovered() const {
        return !modalBlocking && openComboId == 0 && IsMouseInRect(lastItemLeft, lastItemTop, lastItemRight - lastItemLeft, lastItemBottom - lastItemTop);
    }

    float GetLastItemHoverRatioX() const {
        int w = lastItemRight - lastItemLeft;
        if (w <= 0) return 0.0f;
        return std::clamp(static_cast<float>(mousePos.x - lastItemLeft) / static_cast<float>(w), 0.0f, 1.0f);
    }

    Context& SetTooltip(const std::string& textUtf8) {
        if (textUtf8.empty() || !IsLastItemHovered()) return *this;
        pendingTooltipText = textUtf8;
        pendingTooltipPos = { mousePos.x + S(12), mousePos.y - S(26) };
        return *this;
    }

    Context& Tooltip(const std::string& textUtf8) {
        return SetTooltip(textUtf8);
    }

    void OpenPopup(const std::string& id) {
        activePopupId = id;
        lastHoveredIndex = -1;
        openComboId = 0;
        focusedInputId = 0;
        RequestRedraw(true);
    }

    void CloseCurrentPopup() {
        activePopupId.clear();
        lastHoveredIndex = -1;
        focusedInputId = 0;
        RequestRedraw(true);
    }

    bool BeginPopupModal(const std::string& id, int customW = 420, int customH = 115) {
        if (activePopupId != id) return false;

        savedModalState = { regionLeft, regionRight, cursorX, cursorY, modalBlocking };

        int modalW = std::min(clientW - S(30), S(customW));
        int modalH = std::min(clientH - S(30), S(customH));
        int mx = (clientW - modalW) / 2;
        int my = (clientH - modalH) / 2;

        interactiveRects.clear();
        modalBlocking = false;
        inModalRender = true;

        modalDraws.clear();
        modalDraws.push_back([=, this]() {
            DrawDimOverlay(style.modalDimBg);
            DrawRoundRect(mx + S(3), my + S(4), modalW, modalH, S(style.rounding), Color(10, 10, 14, 220), Color(10, 10, 14, 220));
            DrawRoundRect(mx, my, modalW, modalH, S(style.rounding), style.popupBg, style.checkMark);
        });

        regionLeft = mx + S(16);
        regionRight = mx + modalW - S(16);
        cursorX = regionLeft;
        cursorY = my + S(16);
        return true;
    }

    void EndPopupModal() {
        if (!inModalRender) return;
        inModalRender = false;
        regionLeft = savedModalState.regionLeft;
        regionRight = savedModalState.regionRight;
        cursorX = savedModalState.cursorX;
        cursorY = savedModalState.cursorY;
        modalBlocking = savedModalState.modalBlocking;
    }

    void AutoResizeWindowHeight() {
        if (!hwnd || !activePopupId.empty() || IsZoomed(hwnd)) return;
        int neededClientH = cursorY + S(style.windowPadding);
        RECT rcClient = {}, rcWindow = {};
        GetClientRect(hwnd, &rcClient);
        GetWindowRect(hwnd, &rcWindow);

        int currentClientH = rcClient.bottom - rcClient.top;
        int borderH = (rcWindow.bottom - rcWindow.top) - currentClientH;

        if (neededClientH > S(50) && std::abs(currentClientH - neededClientH) > 2) {
            int winW = rcWindow.right - rcWindow.left;
            int winH = neededClientH + borderH;
            SetWindowPos(hwnd, nullptr, 0, 0, winW, winH, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            RequestRedraw(true);
        }
    }

    static std::wstring Utf8ToWide(const std::string& utf8) {
        if (utf8.empty()) return L"";
        int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
        if (len <= 0) return L"";
        std::wstring wide(len - 1, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, wide.data(), len);
        return wide;
    }

    static std::string WideToUtf8(const std::wstring& wide) {
        if (wide.empty()) return "";
        int len = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (len <= 0) return "";
        std::string utf8(len - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, utf8.data(), len, nullptr, nullptr);
        return utf8;
    }

private:
    std::wstring themeRegSubKey;

    // --- ESTADO DOS BACKENDS (D2D DINÂMICO + FALLBACK GDI) ---
    HMODULE hModD2D1 = nullptr;
    HMODULE hModDWrite = nullptr;
    bool activeD2D = false;
    bool pendingBackendSwitch = false;
    bool pendingWantD2D = true;

    // Recursos Direct2D / DirectWrite
    Microsoft::WRL::ComPtr<ID2D1Factory1> d2dFactory;
    Microsoft::WRL::ComPtr<ID2D1Device> d2dDevice;
    Microsoft::WRL::ComPtr<ID2D1DeviceContext> d2dContext;
    Microsoft::WRL::ComPtr<ID2D1Bitmap1> d2dTargetBitmap;
    Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> d2dHwndTarget;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> d2dBrush;
    Microsoft::WRL::ComPtr<ID2D1BitmapRenderTarget> d2dBgTarget;
    Microsoft::WRL::ComPtr<ID2D1Bitmap> d2dBgBitmap;
    int d2dBgW = 0, d2dBgH = 0;
    DWORD lastD2dBgTime = 0;
    Theme lastD2dBgTheme = Theme::TealGold;
    Microsoft::WRL::ComPtr<IDWriteFactory> dwriteFactory;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> dwriteRegular;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> dwriteBold;
    IDXGISwapChain1* externalSwapChain = nullptr;

    struct TextLayoutKey {
        std::string text;
        int w;
        int h;
        bool bold;
        UINT align;
        bool operator==(const TextLayoutKey& o) const {
            return w == o.w && h == o.h && bold == o.bold && align == o.align && text == o.text;
        }
    };
    struct TextLayoutKeyHash {
        size_t operator()(const TextLayoutKey& k) const {
            size_t h = std::hash<std::string>{}(k.text);
            h ^= (size_t(k.w) << 16) ^ size_t(k.h) ^ (size_t(k.bold) << 1) ^ (size_t(k.align) << 2);
            return h;
        }
    };
    std::unordered_map<TextLayoutKey, Microsoft::WRL::ComPtr<IDWriteTextLayout>, TextLayoutKeyHash> textLayoutCache;

    // Recursos GDI
    HDC hdc = nullptr;
    HDC backDC = nullptr;
    HBITMAP backBM = nullptr;
    HGDIOBJ oldBackBM = nullptr;
    int backW = 0, backH = 0;

    HDC bgDC = nullptr;
    HBITMAP bgBM = nullptr;
    HGDIOBJ oldBgBM = nullptr;
    int bgW = 0, bgH = 0;
    DWORD lastBgDrawTime = 0;
    Theme lastBgTheme = Theme::TealGold;

    HDC checkDC = nullptr;
    HBITMAP checkBM = nullptr;
    HGDIOBJ oldCheckBM = nullptr;
    int cachedCheckBigSz = 0;
    HDC pixelDC = nullptr;
    HBITMAP pixelBM = nullptr;
    HGDIOBJ oldPixelBM = nullptr;

    HFONT fontRegular = nullptr, fontBold = nullptr;
    std::unordered_map<uint32_t, HBRUSH> brushCache;
    std::unordered_map<uint64_t, HPEN> penCache;

    // Estado Compartilhado da UI
    std::unordered_map<std::string, int> textWidthCacheRegular;
    std::unordered_map<std::string, int> textWidthCacheBold;
	
	struct WrapCacheKey {
    std::string text;
    int maxLogicalW;
    bool bold;
    bool operator==(const WrapCacheKey& o) const {
        return maxLogicalW == o.maxLogicalW && bold == o.bold && text == o.text;
    }
    };
    struct WrapCacheKeyHash {
    size_t operator()(const WrapCacheKey& k) const {
        size_t h = std::hash<std::string>{}(k.text);
        h ^= (static_cast<size_t>(k.maxLogicalW) << 8) ^ static_cast<size_t>(k.bold);
        return h;
    }
    };
    std::unordered_map<WrapCacheKey, std::vector<std::string>, WrapCacheKeyHash> wrapTextCache;

    std::function<void(Context&)> renderCallback;

    int clientW = 0, clientH = 0;
    POINT mousePos = { -1, -1 };
    bool mouseDown = false;
    bool mouseClicked = false;
    bool mouseDoubleClicked = false;
    bool rightMouseClicked = false;
    bool trackingMouse = false;
    bool lastItemActive = false;

    std::array<bool, 256> keyDown{};
    std::array<bool, 256> keyPressed{};

    std::vector<RECT> interactiveRects;
    int lastHoveredIndex = -1;

    int widgetCounter = 0;
    int activeDragId = 0;
    std::unordered_map<int, float> animValues;
    bool hasActiveAnimations = false;
    bool hasBackdropAnimation = false;
    bool animTimerRunning = false;
    UINT currentTimerIntervalMs = 0;
    DWORD lastRedrawRequestMs = 0;
    DWORD lastFrameTime = 0;
    float deltaTime = 0.033f;

    int cursorX = 0, cursorY = 0;
    int regionLeft = 0, regionRight = 0;
    int lastItemTop = 0, lastItemBottom = 0, lastItemLeft = 0, lastItemRight = 0;
    bool alignTextToFrame = false;

    int colCount = 1, colIndex = 0;
    int savedRegionLeft = 0, savedRegionRight = 0;
    int colStartY = 0, colMaxY = 0;

    std::vector<bool> disabledStack;
    std::vector<uint32_t> idStack;

    struct PanelState {
        int startY = 0, left = 0, right = 0;
        int savedRegionLeft = 0, savedRegionRight = 0;
        std::string title;
        Color titleColor;
        std::vector<std::function<void()>> deferredDraws;
		std::vector<std::string> titleLines;
    };
    std::vector<PanelState> panelStack;

    struct ChildState {
        int id;
        int x, y, w, h;
        bool border;
        int savedRegionLeft, savedRegionRight;
        int savedCursorX, savedCursorY;
        int startContentY;
    };
    std::vector<ChildState> childStack;
    std::unordered_map<int, float> childScrollMap;
    std::unordered_map<int, float> childMaxScrollMap;
    int hoveredChildId = 0;
    float pendingWheelDelta = 0.0f;
    float childDragOffsetY = 0.0f;

    std::vector<RECT> clipStack;

    int focusedInputId = 0;
    bool inputRenderedThisFrame = false;
    size_t textCursorPos = 0;
    DWORD cursorBlinkStart = 0;
    std::vector<wchar_t> pendingChars;
    std::vector<UINT> pendingKeys;

    int openComboId = 0;
    std::vector<std::function<void()>> comboOverlayDraws;

    struct ModalSavedState {
        int regionLeft, regionRight, cursorX, cursorY;
        bool modalBlocking;
    } savedModalState{};

    std::string activePopupId;
    bool modalBlocking = false;
    bool inModalRender = false;
    std::vector<std::function<void()>> modalDraws;

    std::string pendingTooltipText;
    POINT pendingTooltipPos = { 0, 0 };

    std::string toastText;
    Color toastColor;
    DWORD toastStartTime = 0;
    DWORD toastDurationMs = 2500;

    int S(int v) const { return static_cast<int>(std::round(v * dpiScale)); }
    bool IsDisabled() const { return !disabledStack.empty() && disabledStack.back(); }

    Color EnsureContrast(Color col) const {
        if (currentTheme != Theme::LightClean) return col;
        float luma = 0.299f * col.r + 0.587f * col.g + 0.114f * col.b;
        if (luma > 135.0f) {
            return Color::Lerp(col, Color(15, 20, 30, col.a), 0.48f);
        }
        return col;
    }

    static std::string StripIdSuffix(const std::string& label) {
        size_t pos = label.find("##");
        return (pos == std::string::npos) ? label : label.substr(0, pos);
    }
	
	const std::vector<std::string>& WrapTextCached(const std::string& disp, int availLogicalW, bool bold) {
    availLogicalW = std::max(24, availLogicalW);
    WrapCacheKey key{ disp, availLogicalW, bold };
    auto it = wrapTextCache.find(key);
    if (it != wrapTextCache.end()) return it->second;
    if (wrapTextCache.size() > 256) wrapTextCache.clear();

    std::vector<std::string> outLines;
    if (disp.empty()) {
        outLines.emplace_back("");
        return wrapTextCache.emplace(std::move(key), std::move(outLines)).first->second;
    }

    size_t pos = 0;
    while (pos <= disp.size()) {
        size_t nextNl = disp.find('\n', pos);
        std::string paragraph = (nextNl == std::string::npos) ? disp.substr(pos) : disp.substr(pos, nextNl - pos);
        if (!paragraph.empty() && paragraph.back() == '\r') paragraph.pop_back();

        if (paragraph.empty()) {
            outLines.emplace_back("");
        } else {
            std::string currentLine;
            size_t wStart = 0;
            while (wStart < paragraph.size()) {
                size_t wEnd = paragraph.find_first_of(" /\\", wStart);
                std::string token = (wEnd == std::string::npos)
                    ? paragraph.substr(wStart)
                    : paragraph.substr(wStart, wEnd - wStart + 1);

                std::string candidate = currentLine + token;
                if (!currentLine.empty() && GetTextWidth(candidate, bold) > availLogicalW) {
                    while (!currentLine.empty() && currentLine.back() == ' ') currentLine.pop_back();
                    outLines.push_back(currentLine);
                    currentLine = token;
                    while (!currentLine.empty() && currentLine.front() == ' ') currentLine.erase(0, 1);
                } else {
                    currentLine = candidate;
                }

                while (GetTextWidth(currentLine, bold) > availLogicalW && currentLine.size() > 1) {
                    size_t cut = currentLine.size() - 1;
                    while (cut > 1 && (static_cast<unsigned char>(currentLine.at(cut)) & 0xC0) == 0x80) {
                        --cut;
                    }
                    while (cut > 1 && GetTextWidth(currentLine.substr(0, cut), bold) > availLogicalW) {
                        --cut;
                        while (cut > 1 && (static_cast<unsigned char>(currentLine.at(cut)) & 0xC0) == 0x80) {
                            --cut;
                        }
                    }
                    if (cut == 0) cut = 1;
                    outLines.push_back(currentLine.substr(0, cut));
                    currentLine.erase(0, cut);
                }

                if (wEnd == std::string::npos) break;
                wStart = wEnd + 1;
            }
            if (!currentLine.empty()) {
                while (!currentLine.empty() && currentLine.back() == ' ') currentLine.pop_back();
                outLines.push_back(currentLine);
            }
        }

        if (nextNl == std::string::npos) break;
        pos = nextNl + 1;
    }

    return wrapTextCache.emplace(std::move(key), std::move(outLines)).first->second;
    }

    int HashString(const std::string& str) const {
        uint32_t h = idStack.empty() ? 2166136261u : idStack.back();
        for (unsigned char c : str) {
            h ^= c;
            h *= 16777619u;
        }
        int res = static_cast<int>(h & 0x7FFFFFFF);
        return (res == 0) ? 1 : res;
    }

    int MakeWidgetId(const std::string& tag) {
        int idx = ++widgetCounter;
        if (!idStack.empty()) {
            return HashString(tag + "_" + std::to_string(idx));
        }
        return idx;
    }

    float Animate(int id, float target, float speed = 15.0f) {
        auto it = animValues.find(id);
        float current = (it != animValues.end()) ? it->second : 0.0f;
        float diff = target - current;
        if (std::abs(diff) < 0.005f) {
            current = target;
            if (target == 0.0f && it != animValues.end()) {
                animValues.erase(it);
                return 0.0f;
            }
        } else {
            current += diff * std::min(1.0f, deltaTime * speed);
            hasActiveAnimations = true;
        }
        animValues.insert_or_assign(id, current);
        return current;
    }

    bool IsMouseInRect(int x, int y, int w, int h) const {
        return (mousePos.x >= x && mousePos.x <= x + w && mousePos.y >= y && mousePos.y <= y + h);
    }

    bool IsInsideActiveClip(int x, int y, int w, int h) const {
        if (clipStack.empty()) return true;
        const RECT& c = clipStack.back();
        return (x + w > c.left && x < c.right && y + h > c.top && y < c.bottom);
    }

    bool RegisterInteractive(int x, int y, int w, int h, bool disabled) {
        if (disabled || modalBlocking || openComboId != 0 || !IsInsideActiveClip(x, y, w, h)) {
            return false;
        }
        interactiveRects.push_back({ x, y, x + w, y + h });
        return IsMouseInRect(x, y, w, h);
    }

    int GetHoveredRectIndex(POINT pt) const {
        for (int i = 0; i < static_cast<int>(interactiveRects.size()); ++i) {
            const RECT& r = interactiveRects.at(static_cast<size_t>(i));
            if (pt.x >= r.left && pt.x <= r.right && pt.y >= r.top && pt.y <= r.bottom) {
                return i;
            }
        }
        return -1;
    }

    void AdvanceCursor(int itemW, int itemH) {
        lastItemLeft = cursorX;
        lastItemTop = cursorY;
        lastItemRight = cursorX + itemW;
        lastItemBottom = cursorY + itemH;
        cursorX = regionLeft;
        cursorY += itemH + S(style.itemSpacingY);
        if (colCount > 1) colMaxY = std::max(colMaxY, cursorY);
    }

    void UpdateColumnBounds() {
        int totalW = savedRegionRight - savedRegionLeft;
        int colW = totalW / colCount;
        regionLeft = savedRegionLeft + colIndex * colW;
        regionRight = regionLeft + colW - S(6);
        cursorX = regionLeft;
    }

    template<typename Fn>
    void QueueOrDraw(Fn&& fn) {
        if (inModalRender) modalDraws.emplace_back(std::forward<Fn>(fn));
        else if (!panelStack.empty()) panelStack.back().deferredDraws.emplace_back(std::forward<Fn>(fn));
        else fn();
    }

    void BeginFrameState() {
        interactiveRects.clear();
        modalDraws.clear();
        comboOverlayDraws.clear();
        pendingTooltipText.clear();
        clipStack.clear();
        childStack.clear();
        panelStack.clear();
        idStack.clear();
        hoveredChildId = 0;
        lastItemActive = false;
        inputRenderedThisFrame = false;

        DWORD now = GetTickCount();
        deltaTime = (lastFrameTime == 0) ? 0.033f : std::min(0.06f, (now - lastFrameTime) / 1000.0f);
        lastFrameTime = now;
        widgetCounter = 0;
        hasActiveAnimations = false;
        hasBackdropAnimation = false;

        int pad = S(style.windowPadding);
        regionLeft = pad;
        regionRight = clientW - pad;
        cursorX = regionLeft;
        cursorY = pad;
        colCount = 1;
        disabledStack.clear();

        modalBlocking = !activePopupId.empty();
        inModalRender = false;
    }

    void EndFrameState() {
        while (!childStack.empty()) EndChild();
        while (!panelStack.empty()) EndPanel();
        while (!clipStack.empty()) PopClip();

        if (focusedInputId != 0 && !inputRenderedThisFrame) {
            focusedInputId = 0;
        }

        if (!comboOverlayDraws.empty()) {
            for (auto& fn : comboOverlayDraws) fn();
            comboOverlayDraws.clear();
        }
        if (!modalDraws.empty()) {
            for (auto& fn : modalDraws) fn();
            modalDraws.clear();
        }
        if (!toastText.empty()) {
            DWORD elapsed = GetTickCount() - toastStartTime;
            if (elapsed < toastDurationMs) {
                hasActiveAnimations = true;
                float alpha = 1.0f;
                if (elapsed > toastDurationMs - 400) {
                    alpha = std::clamp(static_cast<float>(toastDurationMs - elapsed) / 400.0f, 0.0f, 1.0f);
                }
                DrawToastInternal(toastText, toastColor, alpha);
            } else {
                toastText.clear();
            }
        }
        if (!pendingTooltipText.empty()) {
            DrawTooltipInternal(pendingTooltipPos.x, pendingTooltipPos.y, pendingTooltipText);
            pendingTooltipText.clear();
        }
        lastHoveredIndex = GetHoveredRectIndex(mousePos);
        mouseClicked = false;
        mouseDoubleClicked = false;
        rightMouseClicked = false;
        pendingWheelDelta = 0.0f;
        pendingChars.clear();
        pendingKeys.clear();
        keyPressed.fill(false);
    }

    void DrawToastInternal(const std::string& textUtf8, Color accent, float alpha) {
        int padX = S(14);
        int textW = CalcTextWidth(textUtf8, true);
        int boxW = std::min(clientW - S(24), textW + padX * 2);
        int boxH = S(30);
        int bx = clientW - boxW - S(14);
        int by = clientH - boxH - S(14);

        Color bg(style.popupBg.r, style.popupBg.g, style.popupBg.b, static_cast<uint8_t>(245 * alpha));
        Color bdr(accent.r, accent.g, accent.b, static_cast<uint8_t>(230 * alpha));
        Color txt(style.text.r, style.text.g, style.text.b, static_cast<uint8_t>(255 * alpha));

        DrawRoundRect(bx, by, boxW, boxH, S(style.rounding), bg, bdr);
        DrawTextInternal(bx + padX, by, std::max(0, boxW - padX * 2), boxH, textUtf8, txt, true, DT_CENTER);
    }

    void DrawTooltipInternal(int x, int y, const std::string& textUtf8) {
        int padX = S(8);
        int textW = CalcTextWidth(textUtf8, false);
        int boxW = textW + padX * 2;
        int boxH = S(22);

        if (x + boxW > clientW - S(4)) x = clientW - boxW - S(4);
        if (x < S(4)) x = S(4);
        if (y < S(4)) y = mousePos.y + S(18);

        Color tipTextCol = (currentTheme == Theme::LightClean) ? Color(246, 243, 244, 255) : style.text;
        DrawRoundRect(x, y, boxW, boxH, S(4), style.tooltipBg, style.border);
        DrawTextInternal(x + padX, y, textW, boxH, textUtf8, tipTextCol, false, DT_LEFT);
    }

    std::wstring GetClipboardTextW() const {
        std::wstring result;
        if (OpenClipboard(hwnd)) {
            HANDLE hData = GetClipboardData(CF_UNICODETEXT);
            if (hData) {
                wchar_t* psz = static_cast<wchar_t*>(GlobalLock(hData));
                if (psz) {
                    result = psz;
                    GlobalUnlock(hData);
                }
            }
            CloseClipboard();
        }
        return result;
    }

    void SetClipboardTextW(const std::wstring& text) const {
        if (!OpenClipboard(hwnd)) return;
        EmptyClipboard();
        size_t bytes = (text.size() + 1) * sizeof(wchar_t);
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
        if (hMem) {
            void* dst = GlobalLock(hMem);
            if (dst) {
                memcpy(dst, text.c_str(), bytes);
                GlobalUnlock(hMem);
                SetClipboardData(CF_UNICODETEXT, hMem);
            } else {
                GlobalFree(hMem);
            }
        }
        CloseClipboard();
    }

    // --- INICIALIZAÇÃO HÍBRIDA (D2D DINÂMICO + FALLBACK GDI) ---

    ID2D1RenderTarget* ActiveD2DTarget() const {
        if (d2dContext) return d2dContext.Get();
        return d2dHwndTarget.Get();
    }

    bool TryLoadD2DRuntime() {
        if (!hModD2D1) hModD2D1 = LoadLibraryW(L"d2d1.dll");
        if (!hModDWrite) hModDWrite = LoadLibraryW(L"dwrite.dll");
        if (!hModD2D1 || !hModDWrite) return false;

        using D2D1CreateFactoryFn = HRESULT(WINAPI*)(D2D1_FACTORY_TYPE, REFIID, const D2D1_FACTORY_OPTIONS*, void**);
        using DWriteCreateFactoryFn = HRESULT(WINAPI*)(DWRITE_FACTORY_TYPE, REFIID, IUnknown**);

        auto pD2D1Create = reinterpret_cast<D2D1CreateFactoryFn>(
            reinterpret_cast<void*>(GetProcAddress(hModD2D1, "D2D1CreateFactory")));
        auto pDWriteCreate = reinterpret_cast<DWriteCreateFactoryFn>(
            reinterpret_cast<void*>(GetProcAddress(hModDWrite, "DWriteCreateFactory")));

        if (!pD2D1Create || !pDWriteCreate) return false;

        if (!d2dFactory && FAILED(pD2D1Create(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), nullptr,
                                              reinterpret_cast<void**>(d2dFactory.GetAddressOf())))) {
            return false;
        }
        if (!dwriteFactory && FAILED(pDWriteCreate(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                                                  reinterpret_cast<IUnknown**>(dwriteFactory.GetAddressOf())))) {
            return false;
        }
        return true;
    }

    void InitGraphicsBackend() {
        activeD2D = false;
        textWidthCacheRegular.clear();
        textWidthCacheBold.clear();

        if (preferD2D && TryLoadD2DRuntime()) {
            RebuildDWriteFormats();
            if (hwnd && d2dFactory && !externalSwapChain && !d2dHwndTarget) {
                RECT rc{};
                GetClientRect(hwnd, &rc);
                D2D1_SIZE_U sz = D2D1::SizeU(std::max<LONG>(1, rc.right - rc.left), std::max<LONG>(1, rc.bottom - rc.top));
                if (SUCCEEDED(d2dFactory->CreateHwndRenderTarget(
                        D2D1::RenderTargetProperties(),
                        D2D1::HwndRenderTargetProperties(hwnd, sz),
                        &d2dHwndTarget)) && d2dHwndTarget) {
                    d2dHwndTarget->SetDpi(96.0f, 96.0f);
                    if (SUCCEEDED(d2dHwndTarget->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1), &d2dBrush))) {
                        activeD2D = true;
                    }
                }
            } else if (ActiveD2DTarget() != nullptr) {
                activeD2D = true;
            }
        }

        if (!activeD2D) {
            CleanupD2DResources();
            RebuildGdiFonts();
        }
    }

    void RebuildDWriteFormats() {
        dwriteRegular.Reset();
        dwriteBold.Reset();
        textLayoutCache.clear();
        textWidthCacheRegular.clear();
        textWidthCacheBold.clear();
		wrapTextCache.clear();

        float fontSize = 14.0f * dpiScale;
        if (dwriteFactory) {
            dwriteFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
                                            DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                                            fontSize, L"", &dwriteRegular);
            dwriteFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_BOLD,
                                            DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                                            fontSize, L"", &dwriteBold);
            if (dwriteRegular) {
                dwriteRegular->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
                dwriteRegular->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
                Microsoft::WRL::ComPtr<IDWriteInlineObject> ellipsis;
                if (SUCCEEDED(dwriteFactory->CreateEllipsisTrimmingSign(dwriteRegular.Get(), &ellipsis))) {
                    DWRITE_TRIMMING trim{ DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0 };
                    dwriteRegular->SetTrimming(&trim, ellipsis.Get());
                }
            }
            if (dwriteBold) {
                dwriteBold->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
                dwriteBold->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
                Microsoft::WRL::ComPtr<IDWriteInlineObject> ellipsis;
                if (SUCCEEDED(dwriteFactory->CreateEllipsisTrimmingSign(dwriteBold.Get(), &ellipsis))) {
                    DWRITE_TRIMMING trim{ DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0 };
                    dwriteBold->SetTrimming(&trim, ellipsis.Get());
                }
            }
        }
    }

    bool InitD2DFromSwapChain(IDXGISwapChain1* swapChain) {
        if (!TryLoadD2DRuntime()) return false;
        RebuildDWriteFormats();
        Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
        if (FAILED(swapChain->GetDevice(IID_PPV_ARGS(&dxgiDevice)))) return false;
        if (FAILED(d2dFactory->CreateDevice(dxgiDevice.Get(), &d2dDevice))) return false;
        if (FAILED(d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &d2dContext))) return false;
        d2dContext->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1), &d2dBrush);
        return CreateD2DTargetFromSwapChain(swapChain);
    }

    bool CreateD2DTargetFromSwapChain(IDXGISwapChain1* swapChain) {
        Microsoft::WRL::ComPtr<IDXGISurface> dxgiBackBuffer;
        if (FAILED(swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer)))) return false;

        D2D1_BITMAP_PROPERTIES1 bp = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
        );
        if (FAILED(d2dContext->CreateBitmapFromDxgiSurface(dxgiBackBuffer.Get(), &bp, &d2dTargetBitmap))) return false;
        d2dContext->SetTarget(d2dTargetBitmap.Get());
        d2dContext->SetDpi(96.0f, 96.0f);
        return true;
    }

    void CleanupD2DResources() {
        d2dBgBitmap.Reset();
        d2dBgTarget.Reset();
        d2dBgW = 0;
        d2dBgH = 0;
        textLayoutCache.clear();
        d2dBrush.Reset();
        d2dTargetBitmap.Reset();
        d2dContext.Reset();
        d2dDevice.Reset();
        d2dHwndTarget.Reset();
        dwriteRegular.Reset();
        dwriteBold.Reset();
        dwriteFactory.Reset();
        d2dFactory.Reset();
    }

    void CleanupGdiResources() {
        for (auto& kv : brushCache) DeleteObject(kv.second);
        for (auto& kv : penCache) DeleteObject(kv.second);
        brushCache.clear();
        penCache.clear();
        if (fontRegular) { DeleteObject(fontRegular); fontRegular = nullptr; }
        if (fontBold) { DeleteObject(fontBold); fontBold = nullptr; }
        if (checkDC) {
            SelectObject(checkDC, oldCheckBM);
            DeleteObject(checkBM);
            DeleteDC(checkDC);
            checkDC = nullptr;
        }
        if (backDC) {
            SelectObject(backDC, oldBackBM);
            DeleteObject(backBM);
            DeleteDC(backDC);
            backDC = nullptr;
            backW = backH = 0;
        }
        if (bgDC) {
            SelectObject(bgDC, oldBgBM);
            DeleteObject(bgBM);
            DeleteDC(bgDC);
            bgDC = nullptr;
            bgW = bgH = 0;
        }
        if (pixelDC) {
            SelectObject(pixelDC, oldPixelBM);
            DeleteObject(pixelBM);
            DeleteDC(pixelDC);
            pixelDC = nullptr;
        }
    }

    void CleanupResources() {
        CleanupD2DResources();
        CleanupGdiResources();
    }

    // --- PRIMITIVAS DIRECT2D ---

    int CalcTextWidthD2D(const std::string& textUtf8, bool bold) {
        if (textUtf8.empty() || !dwriteFactory) return 0;
        auto& cache = bold ? textWidthCacheBold : textWidthCacheRegular;
        auto it = cache.find(textUtf8);
        if (it != cache.end()) return it->second;
        if (cache.size() > 512) cache.clear();

        std::wstring w = Utf8ToWide(textUtf8);
        IDWriteTextFormat* fmt = bold ? dwriteBold.Get() : dwriteRegular.Get();
        Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
        if (SUCCEEDED(dwriteFactory->CreateTextLayout(w.c_str(), static_cast<UINT32>(w.size()), fmt, 4096.0f, 100.0f, &layout))) {
            DWRITE_TEXT_METRICS metrics = {};
            layout->GetMetrics(&metrics);
            int width = static_cast<int>(std::ceil(metrics.widthIncludingTrailingWhitespace));
            cache.insert_or_assign(textUtf8, width);
            return width;
        }
        return 0;
    }

    void DrawGlassBackdropD2D(int w, int h) {
        ID2D1RenderTarget* mainRt = ActiveD2DTarget();
        if (!mainRt || w <= 0 || h <= 0) return;

        hasBackdropAnimation = true;
        DWORD now = GetTickCount();
        DWORD backdropIntervalMs = (style.backdropFps > 0) ? (1000u / static_cast<UINT>(style.backdropFps)) : 66u;
        bool sizeChanged = (!d2dBgTarget || d2dBgW != w || d2dBgH != h);
        bool themeChanged = (lastD2dBgTheme != currentTheme);
        bool timeElapsed = (now - lastD2dBgTime >= backdropIntervalMs);

        if (sizeChanged) {
            d2dBgBitmap.Reset();
            d2dBgTarget.Reset();
            D2D1_SIZE_F sz = D2D1::SizeF(static_cast<float>(w), static_cast<float>(h));
            if (SUCCEEDED(mainRt->CreateCompatibleRenderTarget(sz, &d2dBgTarget))) {
                d2dBgTarget->GetBitmap(&d2dBgBitmap);
                d2dBgW = w;
                d2dBgH = h;
            }
        }

        if (d2dBgTarget && (sizeChanged || themeChanged || timeElapsed)) {
            lastD2dBgTime = now;
            lastD2dBgTheme = currentTheme;

            d2dBgTarget->BeginDraw();
            d2dBgTarget->SetTransform(D2D1::Matrix3x2F::Identity());
            ID2D1RenderTarget* rt = d2dBgTarget.Get();

            static DWORD backdropStartMs = 0;
            if (backdropStartMs == 0) {
                backdropStartMs = now;
                backdropSeed = static_cast<float>(now % 10000) * 0.137f;
            }

            float t = ((now - backdropStartMs) * 0.0014f) + backdropSeed;

            float ox1 = (std::sin(t * 0.73f) * 0.6f + std::cos(t * 0.31f) * 0.4f) * S(75);
            float oy1 = (std::cos(t * 0.61f) * 0.6f + std::sin(t * 0.27f) * 0.4f) * S(60);

            float ox2 = (std::cos(t * 0.53f + 1.4f) * 0.6f + std::sin(t * 0.23f) * 0.4f) * S(85);
            float oy2 = (std::sin(t * 0.67f + 0.8f) * 0.6f + std::cos(t * 0.29f) * 0.4f) * S(65);

            float ox3 = std::sin(t * 0.41f + 2.1f) * S(65);
            float oy3 = std::cos(t * 0.37f + 1.1f) * S(55);

            Color bgTop(11, 14, 22, 255);
            Color bgBot(8, 11, 17, 255);
            Color cOrb1(24, 198, 152, 70);
            Color cOrb2(12, 138, 156, 65);
            Color cOrb3(16, 165, 135, 45);

            if (currentTheme == Theme::NeonRed) {
                bgTop = Color(14, 10, 14, 255);
                bgBot = Color(8, 8, 10, 255);
                cOrb1 = Color(245, 55, 95, 85);
                cOrb2 = Color(210, 40, 80, 70);
                cOrb3 = Color(180, 60, 100, 45);
            } else if (currentTheme == Theme::RoseGold) {
                bgTop = Color(14, 13, 16, 255);
                cOrb1 = Color(235, 145, 120, 90);
                cOrb2 = Color(205, 125, 110, 75);
                cOrb3 = Color(185, 135, 125, 50);
            } else if (currentTheme == Theme::CyberPurple) {
                bgTop = Color(13, 14, 22, 255);
                cOrb1 = Color(185, 110, 235, 85);
                cOrb2 = Color(130, 95, 220, 75);
                cOrb3 = Color(110, 140, 230, 50);
            } else if (currentTheme == Theme::LightClean) {
                bgTop = Color(206, 218, 234, 255);
                bgBot = Color(190, 205, 224, 255);
                cOrb1 = Color(110, 160, 235, 80);
                cOrb2 = Color(135, 180, 242, 70);
                cOrb3 = Color(235, 242, 252, 95);
            }

            D2D1_RECT_F fullRect = D2D1::RectF(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h));

            {
                std::array<D2D1_GRADIENT_STOP, 2> stops = {{
                    { 0.0f, bgTop.ToD2D() },
                    { 1.0f, bgBot.ToD2D() }
                }};
                Microsoft::WRL::ComPtr<ID2D1GradientStopCollection> stopCol;
                if (SUCCEEDED(rt->CreateGradientStopCollection(stops.data(), static_cast<UINT32>(stops.size()), &stopCol))) {
                    Microsoft::WRL::ComPtr<ID2D1LinearGradientBrush> gradBrush;
                    if (SUCCEEDED(rt->CreateLinearGradientBrush(
                        D2D1::LinearGradientBrushProperties(D2D1::Point2F(0.0f, 0.0f), D2D1::Point2F(static_cast<float>(w), static_cast<float>(h))),
                        stopCol.Get(), &gradBrush))) {
                        rt->FillRectangle(fullRect, gradBrush.Get());
                    }
                }
            }

            float minDim = static_cast<float>(std::min(w, h));
            float maxDim = static_cast<float>(std::max(w, h));

            auto drawRadialOrb = [&](float cx, float cy, float r, Color col) {
                Color edgeCol(col.r, col.g, col.b, 0);
                std::array<D2D1_GRADIENT_STOP, 2> stops = {{
                    { 0.0f, col.ToD2D() },
                    { 1.0f, edgeCol.ToD2D() }
                }};
                Microsoft::WRL::ComPtr<ID2D1GradientStopCollection> stopCol;
                if (SUCCEEDED(rt->CreateGradientStopCollection(stops.data(), static_cast<UINT32>(stops.size()), &stopCol))) {
                    Microsoft::WRL::ComPtr<ID2D1RadialGradientBrush> radBrush;
                    if (SUCCEEDED(rt->CreateRadialGradientBrush(
                        D2D1::RadialGradientBrushProperties(D2D1::Point2F(cx, cy), D2D1::Point2F(0.0f, 0.0f), r, r),
                        stopCol.Get(), &radBrush))) {
                        rt->FillEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), r, r), radBrush.Get());
                    }
                }
            };

            drawRadialOrb(w * 0.80f + ox1, h * 0.22f + oy1, minDim * 0.46f, cOrb1);
            drawRadialOrb(w * 0.66f + ox2, h * 0.72f + oy2, minDim * 0.43f, cOrb2);
            drawRadialOrb(w * 0.14f + ox3, h * 0.54f + oy3, maxDim * 0.36f, cOrb3);

            d2dBgTarget->EndDraw();
        }

        if (d2dBgBitmap) {
            D2D1_RECT_F destRect = D2D1::RectF(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h));
            mainRt->DrawBitmap(d2dBgBitmap.Get(), destRect);
        }
    }

    void DrawRoundRectD2D(int x, int y, int w, int h, int radius, Color fill, Color borderCol) {
        ID2D1RenderTarget* rt = ActiveD2DTarget();
        if (!rt || !d2dBrush) return;
        D2D1_ROUNDED_RECT rr = { D2D1::RectF(static_cast<float>(x), static_cast<float>(y), static_cast<float>(x + w), static_cast<float>(y + h)), static_cast<float>(radius), static_cast<float>(radius) };
        if (fill.a > 0) {
            d2dBrush->SetColor(fill.ToD2D());
            rt->FillRoundedRectangle(&rr, d2dBrush.Get());
        }
        if (borderCol.a > 0 && borderCol.Pack() != fill.Pack()) {
            d2dBrush->SetColor(borderCol.ToD2D());
            rt->DrawRoundedRectangle(&rr, d2dBrush.Get(), 1.0f);
        }
    }

    void DrawMeshRectD2D(int x, int y, int w, int h, int radius, Color c1, Color c2) {
        ID2D1RenderTarget* rt = ActiveD2DTarget();
        if (!rt || w <= 0 || h <= 0) return;
        D2D1_ROUNDED_RECT rr = { D2D1::RectF(static_cast<float>(x), static_cast<float>(y), static_cast<float>(x + w), static_cast<float>(y + h)), static_cast<float>(radius), static_cast<float>(radius) };

        if (c1.a > 0) {
            std::array<D2D1_GRADIENT_STOP, 2> stops = {{
                { 0.0f, c1.ToD2D() },
                { 1.0f, c2.ToD2D() }
            }};
            Microsoft::WRL::ComPtr<ID2D1GradientStopCollection> stopCol;
            if (SUCCEEDED(rt->CreateGradientStopCollection(stops.data(), static_cast<UINT32>(stops.size()), &stopCol))) {
                Microsoft::WRL::ComPtr<ID2D1LinearGradientBrush> gradBrush;
                if (SUCCEEDED(rt->CreateLinearGradientBrush(
                    D2D1::LinearGradientBrushProperties(D2D1::Point2F(static_cast<float>(x), static_cast<float>(y)), D2D1::Point2F(static_cast<float>(x), static_cast<float>(y + h))),
                    stopCol.Get(), &gradBrush))) {
                    rt->FillRoundedRectangle(&rr, gradBrush.Get());
                }
            }
        }

        std::array<D2D1_GRADIENT_STOP, 2> rimStops = {{
            { 0.0f, Color(255, 255, 255, 75).ToD2D() },
            { 1.0f, Color(255, 255, 255, 12).ToD2D() }
        }};
        Microsoft::WRL::ComPtr<ID2D1GradientStopCollection> rimCol;
        if (SUCCEEDED(rt->CreateGradientStopCollection(rimStops.data(), static_cast<UINT32>(rimStops.size()), &rimCol))) {
            Microsoft::WRL::ComPtr<ID2D1LinearGradientBrush> rimBrush;
            if (SUCCEEDED(rt->CreateLinearGradientBrush(
                D2D1::LinearGradientBrushProperties(D2D1::Point2F(static_cast<float>(x), static_cast<float>(y)), D2D1::Point2F(static_cast<float>(x), static_cast<float>(y + h))),
                rimCol.Get(), &rimBrush))) {
                D2D1_ROUNDED_RECT borderRR = {
                    D2D1::RectF(x + 0.5f, y + 0.5f, std::max(0.0f, static_cast<float>(x + w) - 0.5f), std::max(0.0f, static_cast<float>(y + h) - 0.5f)),
                    static_cast<float>(radius), static_cast<float>(radius)
                };
                rt->DrawRoundedRectangle(&borderRR, rimBrush.Get(), 1.0f);
            }
        }
    }

    void DrawGlassPanelRectD2D(int x, int y, int w, int h, int radius, Color fill, Color borderCol) {
        ID2D1RenderTarget* rt = ActiveD2DTarget();
        if (!rt || w <= 0 || h <= 0) return;

        DrawRoundRectD2D(x, y, w, h, radius, fill, Color(0, 0, 0, 0));

        Color topRim = (borderCol.a > 0) ? borderCol : Color(255, 255, 255, 75);
        Color botRim(topRim.r, topRim.g, topRim.b, static_cast<uint8_t>(topRim.a * 0.35f));

        std::array<D2D1_GRADIENT_STOP, 2> glassStops = {{
            { 0.0f, topRim.ToD2D() },
            { 1.0f, botRim.ToD2D() }
        }};
        Microsoft::WRL::ComPtr<ID2D1GradientStopCollection> glassCol;
        if (SUCCEEDED(rt->CreateGradientStopCollection(glassStops.data(), static_cast<UINT32>(glassStops.size()), &glassCol))) {
            Microsoft::WRL::ComPtr<ID2D1LinearGradientBrush> glassBrush;
            if (SUCCEEDED(rt->CreateLinearGradientBrush(
                D2D1::LinearGradientBrushProperties(D2D1::Point2F(static_cast<float>(x), static_cast<float>(y)), D2D1::Point2F(static_cast<float>(x), static_cast<float>(y + h))),
                glassCol.Get(), &glassBrush))) {
                D2D1_ROUNDED_RECT borderRR = {
                    D2D1::RectF(x + 0.5f, y + 0.5f, std::max(0.0f, static_cast<float>(x + w) - 0.5f), std::max(0.0f, static_cast<float>(y + h) - 0.5f)),
                    static_cast<float>(radius), static_cast<float>(radius)
                };
                rt->DrawRoundedRectangle(&borderRR, glassBrush.Get(), 1.0f);
            }
        }
    }

    void DrawLineD2D(int x1, int y1, int x2, int y2, Color col) {
        ID2D1RenderTarget* rt = ActiveD2DTarget();
        if (!rt || !d2dBrush) return;
        d2dBrush->SetColor(col.ToD2D());
        rt->DrawLine(D2D1::Point2F(static_cast<float>(x1), static_cast<float>(y1)), D2D1::Point2F(static_cast<float>(x2), static_cast<float>(y2)), d2dBrush.Get(), 1.0f);
    }

    void DrawCheckmarkD2D(int x, int y, int boxSz, Color chkCol) {
        ID2D1RenderTarget* rt = ActiveD2DTarget();
        if (!rt || !d2dBrush) return;
        d2dBrush->SetColor(chkCol.ToD2D());
        float stroke = std::max(1.8f, boxSz * 0.13f);
        D2D1_POINT_2F p1 = D2D1::Point2F(x + boxSz * 0.24f, y + boxSz * 0.52f);
        D2D1_POINT_2F p2 = D2D1::Point2F(x + boxSz * 0.43f, y + boxSz * 0.72f);
        D2D1_POINT_2F p3 = D2D1::Point2F(x + boxSz * 0.76f, y + boxSz * 0.30f);
        rt->DrawLine(p1, p2, d2dBrush.Get(), stroke);
        rt->DrawLine(p2, p3, d2dBrush.Get(), stroke);
    }

    void DrawTextD2D(int x, int y, int w, int h, const std::string& textUtf8, Color col, bool bold, UINT align) {
        ID2D1RenderTarget* rt = ActiveD2DTarget();
        IDWriteTextFormat* fmt = bold ? dwriteBold.Get() : dwriteRegular.Get();
        if (!rt || !d2dBrush || !dwriteFactory || !fmt || textUtf8.empty() || w <= 0) return;

        TextLayoutKey key{ textUtf8, w, h, bold, align };
        auto it = textLayoutCache.find(key);
        IDWriteTextLayout* layoutPtr = nullptr;

        if (it != textLayoutCache.end()) {
            layoutPtr = it->second.Get();
        } else {
            if (textLayoutCache.size() > 256) textLayoutCache.clear();
            std::wstring textW = Utf8ToWide(textUtf8);
            Microsoft::WRL::ComPtr<IDWriteTextLayout> newLayout;
            if (SUCCEEDED(dwriteFactory->CreateTextLayout(textW.c_str(), static_cast<UINT32>(textW.size()), fmt, static_cast<float>(w), static_cast<float>(h), &newLayout))) {
                if (align & DT_CENTER) newLayout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
                else if (align & DT_RIGHT) newLayout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
                else newLayout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);

                layoutPtr = newLayout.Get();
                textLayoutCache.emplace(std::move(key), std::move(newLayout));
            }
        }

        if (layoutPtr) {
            d2dBrush->SetColor(col.ToD2D());
            rt->DrawTextLayout(D2D1::Point2F(static_cast<float>(x), static_cast<float>(y)), layoutPtr, d2dBrush.Get(), D2D1_DRAW_TEXT_OPTIONS_NONE);
        }
    }

    // --- HELPERS E PRIMITIVAS GDI (FALLBACK) ---

    void RebuildGdiFonts() {
        if (fontRegular) { DeleteObject(fontRegular); fontRegular = nullptr; }
        if (fontBold) { DeleteObject(fontBold); fontBold = nullptr; }
        textWidthCacheRegular.clear();
        textWidthCacheBold.clear();
		wrapTextCache.clear();

        fontRegular = CreateFontW(static_cast<int>(16 * dpiScale), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        fontBold = CreateFontW(static_cast<int>(16 * dpiScale), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    }

    void EnsureBackBuffer(HDC realDC, int w, int h) {
        if (backDC && backW == w && backH == h) return;
        if (backDC) {
            SelectObject(backDC, oldBackBM);
            DeleteObject(backBM);
            DeleteDC(backDC);
        }
        backDC = CreateCompatibleDC(realDC);
        backBM = CreateCompatibleBitmap(realDC, w, h);
        oldBackBM = SelectObject(backDC, backBM);
        backW = w;
        backH = h;
    }

    void EnsureCheckBuffer(int bigSz) {
        if (checkDC && cachedCheckBigSz == bigSz) return;
        if (checkDC) {
            SelectObject(checkDC, oldCheckBM);
            DeleteObject(checkBM);
            DeleteDC(checkDC);
        }
        checkDC = CreateCompatibleDC(hdc);
        checkBM = CreateCompatibleBitmap(hdc, bigSz, bigSz);
        oldCheckBM = SelectObject(checkDC, checkBM);
        cachedCheckBigSz = bigSz;
    }

    HDC GetSolidPixelDC(Color col) {
        if (!pixelDC) {
            pixelDC = CreateCompatibleDC(hdc);
            pixelBM = CreateCompatibleBitmap(hdc, 1, 1);
            oldPixelBM = SelectObject(pixelDC, pixelBM);
        }
        SetPixel(pixelDC, 0, 0, col.ToWin32());
        return pixelDC;
    }

    HBRUSH GetBrush(Color c) {
        uint32_t key = c.Pack();
        auto it = brushCache.find(key);
        if (it != brushCache.end()) return it->second;
        HBRUSH b = CreateSolidBrush(c.ToWin32());
        brushCache.insert_or_assign(key, b);
        return b;
    }

    HPEN GetPen(Color c, int width = 1) {
        uint64_t key = (static_cast<uint64_t>(width) << 32) | c.Pack();
        auto it = penCache.find(key);
        if (it != penCache.end()) return it->second;
        HPEN p = CreatePen(PS_SOLID, width, c.ToWin32());
        penCache.insert_or_assign(key, p);
        return p;
    }

    int CalcTextWidthGDI(const std::string& textUtf8, bool bold) {
        if (!hdc || textUtf8.empty()) return 0;
        auto& cache = bold ? textWidthCacheBold : textWidthCacheRegular;
        auto it = cache.find(textUtf8);
        if (it != cache.end()) return it->second;
        if (cache.size() > 512) cache.clear();

        std::wstring w = Utf8ToWide(textUtf8);
        SelectObject(hdc, bold ? fontBold : fontRegular);
        SIZE sz = {};
        GetTextExtentPoint32W(hdc, w.c_str(), static_cast<int>(w.size()), &sz);
        cache.insert_or_assign(textUtf8, sz.cx);
        return sz.cx;
    }

    void DrawGlassBackdropGDI(int w, int h) {
        if (!hdc || w <= 0 || h <= 0) return;
        hasBackdropAnimation = true;

        DWORD now = GetTickCount();
        DWORD backdropIntervalMs = (style.backdropFps > 0) ? (1000u / static_cast<UINT>(style.backdropFps)) : 66u;
        bool sizeChanged = (!bgDC || bgW != w || bgH != h);
        bool themeChanged = (lastBgTheme != currentTheme);
        bool timeElapsed = (now - lastBgDrawTime >= backdropIntervalMs);

        if (sizeChanged) {
            if (bgDC) {
                SelectObject(bgDC, oldBgBM);
                DeleteObject(bgBM);
                DeleteDC(bgDC);
            }
            bgDC = CreateCompatibleDC(hdc);
            bgBM = CreateCompatibleBitmap(hdc, w, h);
            oldBgBM = SelectObject(bgDC, bgBM);
            bgW = w;
            bgH = h;
        }

        if (sizeChanged || themeChanged || timeElapsed) {
            lastBgDrawTime = now;
            lastBgTheme = currentTheme;
            HDC prevHdc = hdc;
            hdc = bgDC;

            static DWORD backdropStartMs = 0;
            if (backdropStartMs == 0) {
                backdropStartMs = now;
                backdropSeed = static_cast<float>(now % 10000) * 0.137f;
            }

            float t = ((now - backdropStartMs) * 0.0014f) + backdropSeed;

            float ox1 = (std::sin(t * 0.73f) * 0.6f + std::cos(t * 0.31f) * 0.4f) * S(75);
            float oy1 = (std::cos(t * 0.61f) * 0.6f + std::sin(t * 0.27f) * 0.4f) * S(60);

            float ox2 = (std::cos(t * 0.53f + 1.4f) * 0.6f + std::sin(t * 0.23f) * 0.4f) * S(85);
            float oy2 = (std::sin(t * 0.67f + 0.8f) * 0.6f + std::cos(t * 0.29f) * 0.4f) * S(65);

            Color bgTop(11, 14, 22, 255);
            Color bgBot(8, 11, 17, 255);
            Color cOrb1(24, 198, 152, 40);
            Color cOrb2(12, 138, 156, 36);

            if (currentTheme == Theme::NeonRed) {
                bgTop = Color(14, 10, 14, 255);
                bgBot = Color(8, 8, 10, 255);
                cOrb1 = Color(245, 55, 95, 65);
                cOrb2 = Color(210, 40, 80, 55);
            } else if (currentTheme == Theme::RoseGold) {
                bgTop = Color(14, 13, 16, 255);
                cOrb1 = Color(235, 145, 120, 65);
                cOrb2 = Color(205, 125, 110, 55);
            } else if (currentTheme == Theme::CyberPurple) {
                bgTop = Color(13, 14, 22, 255);
                cOrb1 = Color(185, 110, 235, 65);
                cOrb2 = Color(130, 95, 220, 55);
            } else if (currentTheme == Theme::LightClean) {
                bgTop = Color(206, 218, 234, 255);
                bgBot = Color(190, 205, 224, 255);
                cOrb1 = Color(110, 160, 235, 65);
                cOrb2 = Color(135, 180, 242, 55);
            }

            std::array<TRIVERTEX, 2> verts = {{
                { 0, 0, static_cast<COLOR16>(bgTop.r << 8), static_cast<COLOR16>(bgTop.g << 8), static_cast<COLOR16>(bgTop.b << 8), 0xFF00 },
                { w, h, static_cast<COLOR16>(bgBot.r << 8), static_cast<COLOR16>(bgBot.g << 8), static_cast<COLOR16>(bgBot.b << 8), 0xFF00 }
            }};
            GRADIENT_RECT gRect = { 0, 1 };
            GradientFill(hdc, verts.data(), static_cast<ULONG>(verts.size()), &gRect, 1, GRADIENT_FILL_RECT_V);

            auto drawSoftOrb = [&](int cx, int cy, int r, Color col) {
                HDC orbDC = GetSolidPixelDC(col);
                for (int step = 4; step >= 1; --step) {
                    int curR = (r * step) / 4;
                    BYTE alpha = static_cast<BYTE>(col.a / 4);
                    BLENDFUNCTION bf = { AC_SRC_OVER, 0, alpha, 0 };
                    AlphaBlend(hdc, cx - curR, cy - curR, curR * 2, curR * 2, orbDC, 0, 0, 1, 1, bf);
                }
            };

            int minDim = std::min(w, h);
            drawSoftOrb(static_cast<int>(w * 0.80f + ox1), static_cast<int>(h * 0.22f + oy1), static_cast<int>(minDim * 0.35f), cOrb1);
            drawSoftOrb(static_cast<int>(w * 0.66f + ox2), static_cast<int>(h * 0.72f + oy2), static_cast<int>(minDim * 0.32f), cOrb2);

            hdc = prevHdc;
        }

        BitBlt(hdc, 0, 0, w, h, bgDC, 0, 0, SRCCOPY);
    }

    // --- DISPATCH HÍBRIDO DAS PRIMITIVAS (D2D -> FALLBACK GDI) ---

    int CalcTextWidth(const std::string& textUtf8, bool bold) {
        if (activeD2D) return CalcTextWidthD2D(textUtf8, bold);
        return CalcTextWidthGDI(textUtf8, bold);
    }

    void DrawGlassBackdrop(int w, int h) {
        if (activeD2D) {
            DrawGlassBackdropD2D(w, h);
            return;
        }
        DrawGlassBackdropGDI(w, h);
    }

    void DrawRoundRect(int x, int y, int w, int h, int radius, Color fill, Color borderCol) {
        if (activeD2D) {
            DrawRoundRectD2D(x, y, w, h, radius, fill, borderCol);
            return;
        }
        if (!hdc || w <= 0 || h <= 0) return;
        HGDIOBJ oldB = SelectObject(hdc, GetBrush(fill));
        HGDIOBJ oldP = SelectObject(hdc, GetPen(borderCol, 1));
        if (radius > 0) RoundRect(hdc, x, y, x + w, y + h, radius, radius);
        else Rectangle(hdc, x, y, x + w, y + h);
        SelectObject(hdc, oldB);
        SelectObject(hdc, oldP);
    }

    void DrawMeshRect(int x, int y, int w, int h, int radius, Color c1, Color c2, Color) {
        if (activeD2D) {
            DrawMeshRectD2D(x, y, w, h, radius, c1, c2);
            return;
        }
        if (!hdc || w <= 0 || h <= 0) return;

        int saved = SaveDC(hdc);
        HRGN rgn = CreateRoundRectRgn(x, y, x + w + 1, y + h + 1, radius, radius);
        SelectClipRgn(hdc, rgn);

        std::array<TRIVERTEX, 2> verts = {{
            { x, y, static_cast<COLOR16>(c1.r << 8), static_cast<COLOR16>(c1.g << 8), static_cast<COLOR16>(c1.b << 8), 0xFF00 },
            { x + w, y + h, static_cast<COLOR16>(c2.r << 8), static_cast<COLOR16>(c2.g << 8), static_cast<COLOR16>(c2.b << 8), 0xFF00 }
        }};
        GRADIENT_RECT gRect = { 0, 1 };
        GradientFill(hdc, verts.data(), static_cast<ULONG>(verts.size()), &gRect, 1, GRADIENT_FILL_RECT_V);

        RestoreDC(hdc, saved);
        DeleteObject(rgn);

        Color rimCol = Color::Lerp(c1, Color(255, 255, 255, 255), 0.25f);
        HGDIOBJ oldB = SelectObject(hdc, GetStockObject(NULL_BRUSH));
        HGDIOBJ oldP = SelectObject(hdc, GetPen(rimCol, 1));
        RoundRect(hdc, x, y, x + w, y + h, radius, radius);
        SelectObject(hdc, oldB);
        SelectObject(hdc, oldP);
    }

    void DrawGlassPanelRect(int x, int y, int w, int h, int radius, Color fill, Color borderCol) {
        if (activeD2D) {
            DrawGlassPanelRectD2D(x, y, w, h, radius, fill, borderCol);
            return;
        }
        if (!hdc || w <= 0 || h <= 0) return;
        Color glassRim = Color::Lerp(borderCol, Color(255, 255, 255, 255), 0.18f);
        DrawRoundRect(x, y, w, h, radius, fill, glassRim);
    }

    void DrawLine(int x1, int y1, int x2, int y2, Color col) {
        if (activeD2D) {
            DrawLineD2D(x1, y1, x2, y2, col);
            return;
        }
        HGDIOBJ oldP = SelectObject(hdc, GetPen(col, 1));
        MoveToEx(hdc, x1, y1, nullptr);
        LineTo(hdc, x2, y2);
        SelectObject(hdc, oldP);
    }

    void DrawCheckmarkCached(int x, int y, int boxSz, Color bgCol, Color chkCol) {
        if (activeD2D) {
            DrawCheckmarkD2D(x, y, boxSz, chkCol);
            return;
        }
        const int scale = 3;
        int bigSz = boxSz * scale;
        EnsureCheckBuffer(bigSz);

        RECT r = { 0, 0, bigSz, bigSz };
        FillRect(checkDC, &r, GetBrush(bgCol));

        LOGBRUSH lb = { BS_SOLID, chkCol.ToWin32(), 0 };
        HPEN pen = ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_ROUND | PS_JOIN_ROUND,
                                std::max(3, static_cast<int>(bigSz * 0.13f)), &lb, 0, nullptr);
        HGDIOBJ oldP = SelectObject(checkDC, pen);

        MoveToEx(checkDC, static_cast<int>(bigSz * 0.24f), static_cast<int>(bigSz * 0.52f), nullptr);
        LineTo(checkDC, static_cast<int>(bigSz * 0.43f), static_cast<int>(bigSz * 0.72f));
        LineTo(checkDC, static_cast<int>(bigSz * 0.76f), static_cast<int>(bigSz * 0.30f));

        SelectObject(checkDC, oldP);
        DeleteObject(pen);

        SetStretchBltMode(hdc, HALFTONE);
        SetBrushOrgEx(hdc, 0, 0, nullptr);
        int pad = 2;
        StretchBlt(hdc, x + pad, y + pad, boxSz - pad * 2, boxSz - pad * 2,
                   checkDC, pad * scale, pad * scale, bigSz - pad * 2 * scale, bigSz - pad * 2 * scale, SRCCOPY);
    }

    void DrawTextInternal(int x, int y, int w, int h, const std::string& textUtf8, Color col, bool bold, UINT align) {
        if (activeD2D) {
            DrawTextD2D(x, y, w, h, textUtf8, col, bold, align);
            return;
        }
        if (textUtf8.empty() || w <= 0) return;
        std::wstring textW = Utf8ToWide(textUtf8);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, col.ToWin32());
        SelectObject(hdc, bold ? fontBold : fontRegular);
        RECT rc = { x, y, x + w, y + h };
        DrawTextW(hdc, textW.c_str(), -1, &rc, align | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    }

    void DrawDimOverlay(Color dimCol) {
        if (activeD2D) {
            DrawRoundRectD2D(0, 0, clientW, clientH, 0, dimCol, dimCol);
            return;
        }
        if (clientW <= 0 || clientH <= 0) return;
        HDC dimDC = GetSolidPixelDC(dimCol);
        BLENDFUNCTION bf = { AC_SRC_OVER, 0, dimCol.a, 0 };
        AlphaBlend(hdc, 0, 0, clientW, clientH, dimDC, 0, 0, 1, 1, bf);
    }

    void ApplyClipRect(const RECT& r) {
        if (activeD2D) {
            ID2D1RenderTarget* rt = ActiveD2DTarget();
            if (rt) rt->PushAxisAlignedClip(D2D1::RectF(static_cast<float>(r.left), static_cast<float>(r.top), static_cast<float>(r.right), static_cast<float>(r.bottom)), D2D1_ANTIALIAS_MODE_ALIASED);
            return;
        }
        if (!hdc) return;
        SaveDC(hdc);
        IntersectClipRect(hdc, r.left, r.top, r.right, r.bottom);
    }

    void RemoveClipRect() {
        if (activeD2D) {
            ID2D1RenderTarget* rt = ActiveD2DTarget();
            if (rt) rt->PopAxisAlignedClip();
            return;
        }
        if (hdc) RestoreDC(hdc, -1);
    }

    void PushClipLogical(int x, int y, int w, int h) {
        RECT r = { x, y, x + w, y + h };
        if (!clipStack.empty()) {
            const RECT& prev = clipStack.back();
            r.left   = std::max(r.left, prev.left);
            r.top    = std::max(r.top, prev.top);
            r.right  = std::max(r.left, std::min(r.right, prev.right));
            r.bottom = std::max(r.top, std::min(r.bottom, prev.bottom));
        }
        clipStack.push_back(r);
    }

    void PopClipLogical() {
        if (!clipStack.empty()) clipStack.pop_back();
    }

    void PushClip(int x, int y, int w, int h) {
        PushClipLogical(x, y, w, h);
        ApplyClipRect(clipStack.back());
    }

    void PopClip() {
        if (clipStack.empty()) return;
        clipStack.pop_back();
        RemoveClipRect();
    }

    static void EnableDPIAwareness() {
        HMODULE user32 = GetModuleHandleW(L"user32.dll");
        if (user32) {
            using SetProcessDpiAwarenessContextFn = BOOL(WINAPI*)(HANDLE);
            auto fn = reinterpret_cast<SetProcessDpiAwarenessContextFn>(
                reinterpret_cast<void*>(GetProcAddress(user32, "SetProcessDpiAwarenessContext")));
            if (fn) { fn(reinterpret_cast<HANDLE>(-4)); return; }
        }
        SetProcessDPIAware();
    }

    static float GetSystemDpiScale() {
        HDC hdc = GetDC(nullptr);
        int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(nullptr, hdc);
        return static_cast<float>(dpi) / 96.0f;
    }

    static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        Context* ctx = nullptr;
        if (msg == WM_NCCREATE) {
            CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
            ctx = reinterpret_cast<Context*>(cs->lpCreateParams);
            SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ctx));
        } else {
            ctx = reinterpret_cast<Context*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
        }
        if (ctx) return ctx->WndProc(hWnd, msg, wParam, lParam);
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }

    LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
            case WM_ERASEBKGND:
                return 1;

            case WM_DPICHANGED: {
                UINT newDpi = HIWORD(wParam);
                dpiScale = (newDpi > 0) ? (newDpi / 96.0f) : 1.0f;
                if (activeD2D) RebuildDWriteFormats();
                else RebuildGdiFonts();

                RECT* suggested = reinterpret_cast<RECT*>(lParam);
                if (suggested) {
                    SetWindowPos(hWnd, nullptr, suggested->left, suggested->top,
                                 suggested->right - suggested->left,
                                 suggested->bottom - suggested->top,
                                 SWP_NOZORDER | SWP_NOACTIVATE);
                }
                RequestRedraw(true);
                return 0;
            }

            case WM_GETMINMAXINFO: {
                MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
                mmi->ptMinTrackSize.x = S(minW);
                mmi->ptMinTrackSize.y = S(minH);
                return 0;
            }

            case WM_SIZE:
                clientW = LOWORD(lParam);
                clientH = HIWORD(lParam);
                if (activeD2D && d2dHwndTarget && clientW > 0 && clientH > 0) {
                    d2dHwndTarget->Resize(D2D1::SizeU(clientW, clientH));
                }
                RequestRedraw(true);
                return 0;

            case WM_TIMER:
                if (wParam == 1) {
                    RequestRedraw(true);
                } else if (wParam == 2 && onTimer) {
                    onTimer();
                    RequestRedraw(true);
                }
                return 0;

            case WM_DROPFILES: {
                HDROP hDrop = reinterpret_cast<HDROP>(wParam);
                UINT count = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
                for (UINT i = 0; i < count; ++i) {
                    UINT len = DragQueryFileW(hDrop, i, nullptr, 0);
                    if (len > 0 && onFileDrop) {
                        std::wstring path(len + 1, L'\0');
                        if (DragQueryFileW(hDrop, i, path.data(), len + 1) > 0) {
                            onFileDrop(WideToUtf8(path.c_str()));
                        }
                    }
                }
                DragFinish(hDrop);
                RequestRedraw(true);
                return 0;
            }

            case WM_MOUSEMOVE: {
                POINT pt = { static_cast<short>(LOWORD(lParam)), static_cast<short>(HIWORD(lParam)) };
                if (pt.x == mousePos.x && pt.y == mousePos.y) return 0;
                mousePos = pt;

                if (!trackingMouse) {
                    TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hWnd, 0 };
                    TrackMouseEvent(&tme);
                    trackingMouse = true;
                }

                int currentHover = GetHoveredRectIndex(mousePos);
                if (currentHover != lastHoveredIndex || activeDragId != 0 || openComboId != 0 || !pendingTooltipText.empty()) {
                    lastHoveredIndex = currentHover;
                    RequestRedraw(false);
                }
                return 0;
            }

            case WM_MOUSELEAVE:
                trackingMouse = false;
                mousePos = { -1, -1 };
                if (lastHoveredIndex != -1) {
                    lastHoveredIndex = -1;
                    RequestRedraw(true);
                }
                return 0;

            case WM_MOUSEWHEEL: {
                pendingWheelDelta += static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / static_cast<float>(WHEEL_DELTA);
                RequestRedraw(true);
                return 0;
            }

            case WM_LBUTTONDOWN:
                mouseDown = true;
                mouseClicked = true;
                SetCapture(hWnd);
                RequestRedraw(true);
                return 0;

            case WM_LBUTTONUP:
                mouseDown = false;
                activeDragId = 0;
                ReleaseCapture();
                RequestRedraw(true);
                return 0;

            case WM_LBUTTONDBLCLK:
                mouseDoubleClicked = true;
                mouseClicked = true;
                RequestRedraw(true);
                return 0;

            case WM_RBUTTONDOWN:
                rightMouseClicked = true;
                RequestRedraw(true);
                return 0;

            case WM_CHAR:
                if (focusedInputId != 0) {
                    pendingChars.push_back(static_cast<wchar_t>(wParam));
                    RequestRedraw(true);
                    return 0;
                }
                break;

            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
                if (wParam < keyDown.size()) {
                    if (!keyDown.at(wParam)) keyPressed.at(wParam) = true;
                    keyDown.at(wParam) = true;
                }
                if (focusedInputId != 0) {
                    pendingKeys.push_back(static_cast<UINT>(wParam));
                    RequestRedraw(true);
                    return 0;
                }
                if (openComboId != 0 && wParam == VK_ESCAPE) {
                    openComboId = 0;
                    RequestRedraw(true);
                    return 0;
                }
                break;

            case WM_KEYUP:
            case WM_SYSKEYUP:
                if (wParam < keyDown.size()) {
                    keyDown.at(wParam) = false;
                }
                break;

            case WM_KILLFOCUS:
                mouseDown = false;
                activeDragId = 0;
                focusedInputId = 0;
                openComboId = 0;
                mousePos = { -1, -1 };
                lastHoveredIndex = -1;
                keyDown.fill(false);
                keyPressed.fill(false);
                RequestRedraw(true);
                return 0;

            case WM_PAINT: {
                PAINTSTRUCT ps;
                HDC realDC = BeginPaint(hWnd, &ps);
                RECT rc;
                GetClientRect(hWnd, &rc);
                clientW = rc.right - rc.left;
                clientH = rc.bottom - rc.top;

                if (clientW > 0 && clientH > 0) {
                    if (activeD2D && d2dHwndTarget) {
                        d2dHwndTarget->BeginDraw();
                        d2dHwndTarget->SetTransform(D2D1::Matrix3x2F::Identity());
                        BeginFrameState();
                        DrawGlassBackdrop(clientW, clientH);
                        if (renderCallback) renderCallback(*this);
                        EndFrameState();
                        HRESULT hr = d2dHwndTarget->EndDraw();
                        if (hr == D2DERR_RECREATE_TARGET || FAILED(hr)) {
                            CleanupResources();
                            InitGraphicsBackend();
                            if (!activeD2D) {
                                RequestRedraw(true);
                            }
                        }
                    } else {
                        EnsureBackBuffer(realDC, clientW, clientH);
                        hdc = backDC;
                        BeginFrameState();
                        DrawGlassBackdrop(clientW, clientH);
                        if (renderCallback) renderCallback(*this);
                        EndFrameState();
                        BitBlt(realDC, 0, 0, clientW, clientH, backDC, 0, 0, SRCCOPY);
                    }

                    if (pendingBackendSwitch) {
                        pendingBackendSwitch = false;
                        preferD2D = pendingWantD2D;
                        CleanupResources();
                        InitGraphicsBackend();
                        RequestRedraw(true);
                    }

                    UINT nextDelayMs = GetNextFrameDelayMs();
                    if (nextDelayMs > 0) {
                        if (!animTimerRunning || currentTimerIntervalMs != nextDelayMs) {
                            SetTimer(hWnd, 1, nextDelayMs, nullptr);
                            animTimerRunning = true;
                            currentTimerIntervalMs = nextDelayMs;
                        }
                    } else if (animTimerRunning) {
                        KillTimer(hWnd, 1);
                        animTimerRunning = false;
                        currentTimerIntervalMs = 0;
                        lastFrameTime = 0;
                    }
                }

                EndPaint(hWnd, &ps);
                return 0;
            }

            case WM_DESTROY:
                if (animTimerRunning) {
                    KillTimer(hWnd, 1);
                    animTimerRunning = false;
                    currentTimerIntervalMs = 0;
                }
                PostQuitMessage(0);
                return 0;
        }
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
};

} // namespace ShawarmaUI
