#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <cmath>
#include <cstdint>

#ifdef SHAWARMAUI_USE_D2D
#ifdef DrawText
#undef DrawText
#endif
#include <d2d1_1.h>
#include <dwrite.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#endif

namespace ShawarmaUI {

struct Color {
    uint8_t r, g, b, a;
    constexpr Color(uint8_t r = 255, uint8_t g = 255, uint8_t b = 255, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}
    COLORREF ToWin32() const { return RGB(r, g, b); }
    uint32_t Pack() const { return (uint32_t(a) << 24) | (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(b); }

#ifdef SHAWARMAUI_USE_D2D
    D2D1_COLOR_F ToD2D() const {
        return D2D1::ColorF(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
    }
#endif

    static Color Lerp(Color a, Color b, float t) {
        t = std::clamp(t, 0.0f, 1.0f);
        return Color(
            (uint8_t)std::round(a.r + (b.r - a.r) * t),
            (uint8_t)std::round(a.g + (b.g - a.g) * t),
            (uint8_t)std::round(a.b + (b.b - a.b) * t),
            (uint8_t)std::round(a.a + (b.a - a.a) * t)
        );
    }
};

enum class Theme {
    ShawarmaGold,
    NeonRed,
    CyberPurple,
    LightClean
};

struct Style {
    Color windowBg      = Color(22, 20, 19, 255);
    Color panelBg       = Color(33, 30, 28, 245);
    Color childBg       = Color(27, 25, 23, 240);
    Color popupBg       = Color(38, 34, 32, 252);
    Color tooltipBg     = Color(28, 25, 23, 245);
    Color modalDimBg    = Color(10, 9, 8, 175);

    Color border        = Color(64, 58, 54, 255);
    Color borderFocus   = Color(232, 176, 56, 255);
    Color text          = Color(245, 241, 234, 255);
    Color textDim       = Color(158, 148, 136, 255);
    Color frameBg       = Color(46, 42, 39, 255);
    Color frameHover    = Color(61, 55, 51, 255);
    Color checkMark     = Color(232, 176, 56, 255);
    Color sliderGrab    = Color(232, 176, 56, 255);
    Color button        = Color(194, 136, 30, 255);
    Color buttonHover   = Color(219, 158, 42, 255);
    Color buttonActive  = Color(164, 112, 22, 255);
    Color toggleActive  = Color(196, 54, 44, 255);
    Color toggleHover   = Color(222, 68, 56, 255);
    Color separator     = Color(64, 58, 54, 255);
    Color disabledBg    = Color(30, 28, 26, 255);
    Color disabledText  = Color(115, 108, 100, 255);

    int windowPadding   = 16;
    int framePaddingX   = 10;
    int framePaddingY   = 6;
    int itemSpacingX    = 10;
    int itemSpacingY    = 8;
    int rounding        = 6;
    int animFps         = 30;
};

class Context {
public:
    Style style;
    HWND hwnd = nullptr;
    float dpiScale = 1.0f;
    std::function<void(const std::string&)> onFileDrop;
    std::function<void()> onTimer;

    // --- ESCOPOS RAII OPCIONAIS ---
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

    PanelScope Panel(const std::string& titleUtf8 = "", Color titleColor = Color(115, 191, 250), int customWidth = -1) {
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

    Context() {
        interactiveRects.reserve(64);
        deferredDraws.reserve(64);
        modalDraws.reserve(16);
        comboOverlayDraws.reserve(8);
        clipStack.reserve(8);
        childStack.reserve(4);
    }

    ~Context() {
        CleanupResources();
    }

    void ApplyTheme(Theme theme) {
        style = Style{};
        if (theme == Theme::NeonRed) {
            style.windowBg     = Color(16, 16, 20, 255);
            style.panelBg      = Color(18, 14, 20, 232);
            style.childBg      = Color(22, 18, 25, 235);
            style.popupBg      = Color(24, 20, 28, 252);
            style.border       = Color(230, 25, 65, 210);
            style.borderFocus  = Color(255, 50, 90, 255);
            style.frameBg      = Color(32, 16, 22, 235);
            style.frameHover   = Color(62, 20, 34, 245);
            style.checkMark    = Color(230, 35, 75, 255);
            style.sliderGrab   = Color(255, 35, 80, 255);
            style.button       = Color(36, 18, 24, 235);
            style.buttonHover  = Color(215, 25, 65, 245);
            style.buttonActive = Color(255, 40, 85, 255);
            style.toggleActive = Color(225, 25, 65, 250);
            style.toggleHover  = Color(255, 45, 88, 255);
        } else if (theme == Theme::CyberPurple) {
            style.windowBg     = Color(17, 19, 24, 255);
            style.panelBg      = Color(24, 27, 34, 245);
            style.childBg      = Color(20, 23, 30, 245);
            style.popupBg      = Color(28, 32, 42, 252);
            style.border       = Color(46, 52, 66, 255);
            style.borderFocus  = Color(186, 104, 200, 255);
            style.frameBg      = Color(30, 34, 42, 255);
            style.frameHover   = Color(42, 48, 60, 255);
            style.checkMark    = Color(206, 147, 216, 255);
            style.sliderGrab   = Color(186, 104, 200, 255);
            style.button       = Color(106, 27, 154, 255);
            style.buttonHover  = Color(142, 36, 170, 255);
            style.buttonActive = Color(74, 20, 140, 255);
            style.toggleActive = Color(123, 31, 162, 255);
            style.toggleHover  = Color(156, 39, 176, 255);
        } else if (theme == Theme::LightClean) {
            style.windowBg     = Color(242, 244, 248, 255);
            style.panelBg      = Color(255, 255, 255, 250);
            style.childBg      = Color(248, 250, 252, 250);
            style.popupBg      = Color(255, 255, 255, 255);
            style.tooltipBg    = Color(30, 34, 42, 245);
            style.border       = Color(205, 212, 224, 255);
            style.borderFocus  = Color(45, 115, 215, 255);
            style.text         = Color(28, 32, 40, 255);
            style.textDim      = Color(110, 118, 132, 255);
            style.frameBg      = Color(232, 236, 242, 255);
            style.frameHover   = Color(220, 226, 236, 255);
            style.checkMark    = Color(45, 115, 215, 255);
            style.sliderGrab   = Color(45, 115, 215, 255);
            style.button       = Color(225, 230, 240, 255);
            style.buttonHover  = Color(208, 216, 230, 255);
            style.buttonActive = Color(190, 202, 222, 255);
            style.toggleActive = Color(45, 115, 215, 255);
            style.toggleHover  = Color(60, 130, 230, 255);
            style.disabledBg   = Color(235, 237, 240, 255);
            style.disabledText = Color(160, 166, 176, 255);
        }
    }

    // Modo 1: Cria janela gerenciada pela ShawarmaUI
    bool Create(const std::string& titleUtf8, int baseW, int baseH, HICON customIcon = nullptr) {
        EnableDPIAwareness();
        dpiScale = GetSystemDpiScale();

        WNDCLASSEXW wc = { sizeof(wc), CS_HREDRAW | CS_VREDRAW, StaticWndProc, 0, 0,
                           GetModuleHandleW(nullptr), customIcon, LoadCursor(nullptr, IDC_ARROW),
                           nullptr, nullptr, L"ShawarmaUI_WindowClass", customIcon };
        RegisterClassExW(&wc);

        RECT workArea = {};
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
        int screenW = workArea.right - workArea.left;
        int screenH = workArea.bottom - workArea.top;

        int winW = std::min((int)(baseW * dpiScale), screenW - 40);
        int winH = std::min((int)(baseH * dpiScale), screenH - 40);
        int posX = workArea.left + (screenW - winW) / 2;
        int posY = workArea.top + (screenH - winH) / 2;

        std::wstring titleW = Utf8ToWide(titleUtf8);
        hwnd = CreateWindowW(wc.lpszClassName, titleW.c_str(),
                             WS_OVERLAPPEDWINDOW,
                             posX, posY, winW, winH, nullptr, nullptr, wc.hInstance, this);
        if (!hwnd) return false;

        DragAcceptFiles(hwnd, TRUE);
        BOOL useDark = TRUE;
        DwmSetWindowAttribute(hwnd, 20, &useDark, sizeof(useDark));
        DwmSetWindowAttribute(hwnd, 19, &useDark, sizeof(useDark));

        InitGraphicsBackend();

        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
        return true;
    }

    void Run(std::function<void(Context&)> onRender) {
        renderCallback = std::move(onRender);
        RequestRedraw();
        MSG msg;
        while (GetMessageW(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    void RequestRedraw() {
        if (hwnd) InvalidateRect(hwnd, nullptr, FALSE);
    }

#ifdef SHAWARMAUI_USE_D2D
    bool AttachDXGISwapChain(HWND targetHwnd, IDXGISwapChain1* swapChain) {
        hwnd = targetHwnd;
        dpiScale = GetSystemDpiScale();
        externalSwapChain = swapChain;
        return InitD2DFromSwapChain(swapChain);
    }

    void ReleaseDXGISwapChain() {
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
            InitGraphicsBackend();
        }
        if (d2dHwndTarget && !d2dBrush) {
            d2dHwndTarget->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1), &d2dBrush);
        }
    }

    void ReleaseD2DHwndTarget() {
        textLayoutCache.clear();
        d2dBrush.Reset();
        d2dHwndTarget.Reset();
    }

    void BeginExternalFrame(int width, int height) {
        clientW = width;
        clientH = height;
        if (!d2dContext) return;
        d2dContext->BeginDraw();
        d2dContext->SetTransform(D2D1::Matrix3x2F::Identity());
        BeginFrameState();
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
                mousePos = { (short)LOWORD(lParam), (short)HIWORD(lParam) };
                return IsAnyItemHovered() || activeDragId != 0 || openComboId != 0;
            case WM_MOUSELEAVE:
                mousePos = { -1, -1 };
                return false;
            case WM_MOUSEWHEEL:
                if (hoveredChildId != 0 || openComboId != 0) {
                    pendingWheelDelta += (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
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
            case WM_CHAR:
                if (focusedInputId != 0) {
                    pendingChars.push_back(static_cast<wchar_t>(wParam));
                    return true;
                }
                return false;
            case WM_KEYDOWN:
                if (focusedInputId != 0) {
                    pendingKeys.push_back(static_cast<UINT>(wParam));
                    return true;
                }
                if (openComboId != 0 && wParam == VK_ESCAPE) {
                    openComboId = 0;
                    return true;
                }
                return false;
            case WM_KILLFOCUS:
                mouseDown = false;
                activeDragId = 0;
                focusedInputId = 0;
                openComboId = 0;
                mousePos = { -1, -1 };
                return false;
        }
        return false;
    }

    bool IsAnyItemHovered() const {
        return GetHoveredRectIndex(mousePos) != -1 || hoveredChildId != 0;
    }
#endif

    // --- LAYOUT E CONTROLE FLUENTE ---

    int GetAvailWidth() const { return std::max(0, regionRight - cursorX); }

    Context& SetCursorPos(int x, int y) {
        cursorX = x;
        cursorY = y;
        return *this;
    }

    int GetCursorX() const { return cursorX; }
    int GetCursorY() const { return cursorY; }
    int GetClientWidth() const { return clientW; }
    int GetClientHeight() const { return clientH; }

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
        int h = S(20);
        int y = cursorY;
        int l = regionLeft, r = regionRight;
        int midY = y + h / 2;
        int textW = CalcTextWidth(labelUtf8, true);
        int pad = S(8);
        Color col = (labelCol.a > 0) ? labelCol : style.textDim;
        Color lineCol = style.border;

        QueueOrDraw([=, this]() {
            int line1End = l + S(12);
            if (line1End > l) DrawLine(l, midY, line1End, midY, lineCol);
            int textX = line1End + pad;
            int drawW = std::min(textW + S(4), std::max(0, r - textX));
            DrawTextInternal(textX, y, drawW, h, labelUtf8, col, true, DT_LEFT);
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

    // --- CONTAINERS (PAINÉIS E ÁREA COM SCROLL) ---

    void BeginPanel(const std::string& titleUtf8 = "", Color titleColor = Color(115, 191, 250), int customWidth = -1) {
        panelActive = true;
        panelStartY = cursorY;
        panelLeft = cursorX;
        panelRight = (customWidth > 0) ? (cursorX + S(customWidth)) : regionRight;
        panelTitle = titleUtf8;
        panelTitleColor = titleColor;

        int pad = S(10);
        regionLeft = panelLeft + pad;
        regionRight = panelRight - pad;
        cursorX = regionLeft;
        cursorY += pad;
        if (!panelTitle.empty()) cursorY += S(24);
    }

    void EndPanel() {
        if (!panelActive) return;
        int pad = S(10);
        int panelBottom = cursorY + pad - S(style.itemSpacingY);

        DrawRoundRect(panelLeft, panelStartY, panelRight - panelLeft, panelBottom - panelStartY,
                      S(style.rounding), style.panelBg, style.border);

        if (!panelTitle.empty()) {
            int titleY = panelStartY + S(8);
            DrawTextInternal(panelLeft + pad, titleY, (panelRight - panelLeft) - pad * 2, S(20),
                             panelTitle, panelTitleColor, true, DT_LEFT);
            int sepY = titleY + S(22);
            DrawLine(panelLeft + pad, sepY, panelRight - pad, sepY, style.border);
        }

        regionLeft = panelLeft;
        regionRight = panelRight;
        cursorX = regionLeft;
        cursorY = panelBottom + S(style.itemSpacingY);
        panelActive = false;

        FlushDeferredDraws();
    }

    void BeginChild(const std::string& idStr, int customW = -1, int customH = 200, bool border = true) {
        int id = HashString(idStr);
        int w = (customW < 0) ? GetAvailWidth() : S(customW);
        int h = (customH <= 0) ? std::max(S(60), clientH - cursorY - S(style.windowPadding)) : S(customH);
        int x = cursorX, y = cursorY;

        bool hovered = !modalBlocking && (openComboId == 0) && IsMouseInRect(x, y, w, h);
        if (hovered) {
            hoveredChildId = id;
            if (std::abs(pendingWheelDelta) > 0.001f) {
                childScrollMap[id] -= pendingWheelDelta * S(36);
            }
        }

        float scrollY = std::max(0.0f, childScrollMap[id]);
        childScrollMap[id] = scrollY;

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
        cs.startContentY = y + S(6) - (int)std::round(scrollY);
        childStack.push_back(cs);

        DrawRoundRect(x, y, w, h, S(style.rounding), style.childBg, border ? style.border : style.childBg);

        int pad = S(8);
        int scrollBarSpace = S(10);
        regionLeft = x + pad;
        regionRight = x + w - pad - scrollBarSpace;
        cursorX = regionLeft;
        cursorY = cs.startContentY;

        PushClip(x + 1, y + 1, std::max(0, w - 2), std::max(0, h - 2));
    }

    void EndChild() {
        if (childStack.empty()) return;
        ChildState cs = childStack.back();
        childStack.pop_back();

        PopClip();

        int contentH = (cursorY - cs.startContentY) + S(6);
        int maxScroll = std::max(0, contentH - cs.h);
        float& scrollY = childScrollMap[cs.id];
        scrollY = std::clamp(scrollY, 0.0f, (float)maxScroll);

        if (maxScroll > 0 && cs.h > S(20)) {
            int barW = S(6);
            int barX = cs.x + cs.w - barW - S(3);
            int barY = cs.y + S(4);
            int barH = cs.h - S(8);
            DrawRoundRect(barX, barY, barW, barH, barW / 2, style.scrollbarBg, style.scrollbarBg);

            float visibleRatio = std::clamp(float(cs.h) / float(contentH), 0.12f, 1.0f);
            int grabH = std::max(S(18), (int)std::round(barH * visibleRatio));
            float scrollRatio = scrollY / float(maxScroll);
            int grabY = barY + (int)std::round((barH - grabH) * scrollRatio);
            DrawRoundRect(barX, grabY, barW, grabH, barW / 2, style.scrollbarGrab, style.scrollbarGrab);
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
        int offsetY = alignTextToFrame ? S(4) : 0;
        alignTextToFrame = false;
        if (IsDisabled()) color = style.disabledText;
        int h = S(20);
        int w = CalcTextWidth(textUtf8, bold);
        int drawW = std::min(w + S(4), GetAvailWidth());
        int x = cursorX, y = cursorY + offsetY;

        QueueOrDraw([=, this]() {
            DrawTextInternal(x, y, drawW, h, textUtf8, color, bold, DT_LEFT);
        });
        AdvanceCursor(drawW, h);
        return *this;
    }

    Context& Badge(const std::string& textUtf8, Color accentColor = Color(89, 173, 250)) {
        int padX = S(8);
        int h = S(22);
        int textW = CalcTextWidth(textUtf8, true);
        int w = std::min(textW + padX * 2, GetAvailWidth());
        int x = cursorX, y = cursorY;

        Color bgCol = Color::Lerp(style.panelBg, accentColor, 0.18f);
        Color borderCol = Color::Lerp(style.panelBg, accentColor, 0.65f);

        QueueOrDraw([=, this]() {
            DrawRoundRect(x, y, w, h, h / 2, bgCol, borderCol);
            DrawTextInternal(x + padX, y, std::max(0, w - padX * 2), h, textUtf8, accentColor, true, DT_CENTER);
        });
        AdvanceCursor(w, h);
        return *this;
    }

    Context& KeyValue(const std::string& keyUtf8, const std::string& valUtf8, Color valColor = Color(0, 0, 0, 0)) {
        int h = S(20);
        int x = cursorX, y = cursorY;
        int availW = GetAvailWidth();
        int keyW = std::min(CalcTextWidth(keyUtf8 + ": ", false), availW);
        int valW = std::max(0, availW - keyW);
        Color kCol = IsDisabled() ? style.disabledText : style.textDim;
        Color vCol = IsDisabled() ? style.disabledText : ((valColor.a > 0) ? valColor : style.text);

        QueueOrDraw([=, this]() {
            DrawTextInternal(x, y, keyW, h, keyUtf8 + ": ", kCol, false, DT_LEFT);
            if (valW > 0) {
                DrawTextInternal(x + keyW, y, valW, h, valUtf8, vCol, true, DT_LEFT);
            }
        });
        AdvanceCursor(availW, h);
        return *this;
    }

    Context& InputTextReadOnly(const std::string& textUtf8, int customWidth = -1) {
        int w = (customWidth < 0) ? GetAvailWidth() : S(customWidth);
        int h = S(28);
        int x = cursorX, y = cursorY;

        QueueOrDraw([=, this]() {
            DrawRoundRect(x, y, w, h, S(style.rounding), style.frameBg, style.border);
            DrawTextInternal(x + S(8), y, w - S(16), h, textUtf8,
                             textUtf8.empty() ? style.textDim : style.text, false, DT_LEFT | DT_PATH_ELLIPSIS);
        });
        AdvanceCursor(w, h);
        return *this;
    }

    bool InputText(const std::string& idStr, std::string& bufferUtf8, const std::string& placeholderUtf8 = "",
                   int customWidth = -1, bool isPassword = false) {
        int id = HashString(idStr);
        int w = (customWidth < 0) ? GetAvailWidth() : S(customWidth);
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

        AdvanceCursor(w, h);
        return changed;
    }

    bool Checkbox(const std::string& labelUtf8, bool& value) {
        int id = ++widgetCounter;
        int boxSz = S(18);
        int h = S(22);
        int textW = CalcTextWidth(labelUtf8, false);
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
            DrawTextInternal(x + boxSz + S(8), y, totalW - boxSz - S(8), h, labelUtf8, txtCol, false, DT_LEFT);
        });

        AdvanceCursor(totalW, h);
        return clicked;
    }

    bool ToggleSwitch(const std::string& labelUtf8, bool& value) {
        int id = ++widgetCounter;
        int swW = S(36);
        int swH = S(20);
        int h = S(22);
        int textW = labelUtf8.empty() ? 0 : (CalcTextWidth(labelUtf8, false) + S(8));
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
            int knobX = x + pad + (int)std::round(maxTravel * switchT);
            int knobY = swY + pad;
            DrawRoundRect(knobX, knobY, knobSz, knobSz, knobSz / 2, knobCol, knobCol);

            if (!labelUtf8.empty() && totalW > swW + S(8)) {
                DrawTextInternal(x + swW + S(8), y, totalW - swW - S(8), h, labelUtf8, txtCol, false, DT_LEFT);
            }
        });

        AdvanceCursor(totalW, h);
        return clicked;
    }

    bool Button(const std::string& labelUtf8, int customW = 0, int customH = 0,
                Color customBg = Color(0, 0, 0, 0), Color customHover = Color(0, 0, 0, 0)) {
        int id = ++widgetCounter;
        int w = (customW > 0) ? S(customW) : (CalcTextWidth(labelUtf8, true) + S(style.framePaddingX * 2));
        if (customW == -1) w = GetAvailWidth();
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
        Color txtCol  = disabled ? style.disabledText : style.text;

        QueueOrDraw([=, this]() {
            DrawRoundRect(x, y, w, h, S(style.rounding), bgCol, disabled ? style.border : bgCol);
            DrawTextInternal(x, y, w, h, labelUtf8, txtCol, true, DT_CENTER);
        });

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
        int id = ++widgetCounter;
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
            DrawRoundRect(x, y, w, h, S(style.rounding), bgCol, disabled ? style.border : bgCol);
            if (onDrawIcon) onDrawIcon(x, y, w, h);
        });

        AdvanceCursor(w, h);
        return clicked;
    }

    bool TabBar(int& currentTab, const std::vector<std::string>& labels, int customHeight = 30) {
        if (labels.empty()) return false;
        int count = (int)labels.size();
        int totalW = GetAvailWidth();
        int h = S(customHeight);
        int x = cursorX, y = cursorY;
        int spacing = S(4);
        int tabW = std::max(S(30), (totalW - spacing * (count - 1)) / count);
        bool disabled = IsDisabled();
        bool changed = false;

        for (int i = 0; i < count; ++i) {
            int id = ++widgetCounter;
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
            Color txtCol = disabled ? style.disabledText : (isSelected ? style.text : Color::Lerp(style.textDim, style.text, hoverT));

            QueueOrDraw([=, this]() {
                DrawRoundRect(tx, y, curW, h, S(style.rounding), bgCol, isSelected ? style.borderFocus : style.border);
                DrawTextInternal(tx, y, curW, h, labels.at(i), txtCol, isSelected, DT_CENTER);
            });
        }

        AdvanceCursor(totalW, h);
        return changed;
    }

    bool SegmentedControl(int& currentIndex, const std::vector<std::string>& options, int customWidth = -1, int customHeight = 28) {
        if (options.empty()) return false;
        int count = (int)options.size();
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
            int id = ++widgetCounter;
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
            Color hoverBg  = Color(style.frameHover.r, style.frameHover.g, style.frameHover.b, (uint8_t)(hoverT * 180));
            Color segCol   = disabled ? style.disabledBg : Color::Lerp(hoverBg, activeBg, activeT);
            Color txtCol   = disabled ? style.disabledText : (isSelected ? style.text : Color::Lerp(style.textDim, style.text, hoverT));

            QueueOrDraw([=, this]() {
                if (activeT > 0.02f || hoverT > 0.02f) {
                    int pad = S(2);
                    DrawRoundRect(sx + pad, y + pad, std::max(0, curW - pad * 2), std::max(0, h - pad * 2),
                                  std::max(2, S(style.rounding) - 2), segCol, segCol);
                }
                DrawTextInternal(sx, y, curW, h, options.at(i), txtCol, isSelected, DT_CENTER);
            });
        }

        AdvanceCursor(totalW, h);
        return changed;
    }

    bool SelectableRow(const std::string& titleUtf8, const std::string& subtitleUtf8 = "", bool selected = false, int customHeight = 36) {
        int id = ++widgetCounter;
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
            int subW = subtitleUtf8.empty() ? 0 : (CalcTextWidth(subtitleUtf8, false) + S(12));
            int titleW = std::max(0, w - padX * 2 - subW);
            DrawTextInternal(x + padX, y, titleW, h, titleUtf8, disabled ? style.disabledText : style.text, true, DT_LEFT);
            if (subW > 0) {
                DrawTextInternal(x + w - padX - subW, y, subW, h, subtitleUtf8, style.textDim, false, DT_RIGHT);
            }
        });

        AdvanceCursor(w, h);
        return clicked;
    }

    bool Combo(const std::string& idStr, int& currentIndex, const std::vector<std::string>& items, int customWidth = -1) {
        int id = HashString(idStr);
        int w = (customWidth < 0) ? GetAvailWidth() : S(customWidth);
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

        std::string preview = (!items.empty() && currentIndex >= 0 && currentIndex < (int)items.size())
                              ? items.at(currentIndex) : "Selecionar...";

        QueueOrDraw([=, this]() {
            DrawRoundRect(x, y, w, h, S(style.rounding), bgCol, borderCol);
            int arrowW = S(22);
            DrawTextInternal(x + S(8), y, std::max(0, w - arrowW - S(8)), h, preview,
                             disabled ? style.disabledText : style.text, false, DT_LEFT);
            DrawTextInternal(x + w - arrowW, y, arrowW, h, isOpen ? "▲" : "▼",
                             disabled ? style.disabledText : style.textDim, false, DT_CENTER);
        });

        bool selectionChanged = false;
        if (isOpen && !items.empty()) {
            int itemH = S(26);
            int dropH = (int)items.size() * itemH + S(6);
            int dropY = y + h + S(2);
            if (dropY + dropH > clientH - S(4) && y - dropH - S(2) >= S(4)) {
                dropY = y - dropH - S(2);
            }

            interactiveRects.push_back({ x, dropY, x + w, dropY + dropH });

            int hoveredOption = -1;
            for (int i = 0; i < (int)items.size(); ++i) {
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
                DrawRoundRect(x + S(2), dropY + S(3), w, dropH, S(style.rounding), Color(10, 10, 14, 200), Color(10, 10, 14, 200));
                DrawRoundRect(x, dropY, w, dropH, S(style.rounding), style.popupBg, style.borderFocus);
                for (int i = 0; i < (int)items.size(); ++i) {
                    int optY = dropY + S(3) + i * itemH;
                    if (i == hoveredOption) {
                        DrawRoundRect(x + S(3), optY, w - S(6), itemH, S(4), style.buttonHover, style.buttonHover);
                    } else if (i == currentIndex) {
                        DrawRoundRect(x + S(3), optY, w - S(6), itemH, S(4), style.frameHover, style.frameHover);
                    }
                    DrawTextInternal(x + S(10), optY, std::max(0, w - S(20)), itemH, items.at(i), style.text, i == currentIndex, DT_LEFT);
                }
            });
        }

        AdvanceCursor(w, h);
        return selectionChanged;
    }

    bool SliderFloat(float& value, float minVal, float maxVal, int customWidth = -1, int customHeight = 16,
                     const std::string& overlayUtf8 = "", bool* outIsDragging = nullptr) {
        int id = ++widgetCounter;
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
            float t = std::clamp(float(mousePos.x - x) / float(w), 0.0f, 1.0f);
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
            int trackH = std::max(S(6), h / 2);
            int trackY = y + (h - trackH) / 2;
            DrawRoundRect(x, trackY, w, trackH, trackH / 2, trackCol, style.border);

            int fillW = (int)std::round(w * norm);
            if (fillW > 0) {
                DrawRoundRect(x, trackY, fillW, trackH, trackH / 2, fillCol, fillCol);
            }

            int grabSz = std::max(S(12), h);
            int grabX = std::clamp(x + fillW - grabSz / 2, x, x + std::max(0, w - grabSz));
            int grabY = y + (h - grabSz) / 2;
            DrawRoundRect(grabX, grabY, grabSz, grabSz, grabSz / 2, style.text, fillCol);

            if (!overlayUtf8.empty()) {
                DrawTextInternal(x, y, w, h, overlayUtf8, style.text, true, DT_CENTER);
            }
        });

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
            int fillW = (int)std::round((w - 2) * fraction);
            if (fillW > 0) {
                DrawRoundRect(x + 1, y + 1, fillW, h - 2, S(3), barCol, barCol);
            }
            if (!overlayUtf8.empty()) {
                DrawTextInternal(x, y, w, h, overlayUtf8, style.text, true, DT_CENTER);
            }
        });

        AdvanceCursor(w, h);
        return *this;
    }

    Context& Spinner(int diameter = 20, Color color = Color(0, 0, 0, 0)) {
        hasActiveAnimations = true;
        int sz = S(diameter);
        int x = cursorX, y = cursorY;
        Color col = (color.a > 0) ? color : style.sliderGrab;
        float angle = (GetTickCount() % 1000) * 0.0062831853f; // 2 * PI * t

        QueueOrDraw([=, this]() {
            float cx = x + sz * 0.5f;
            float cy = y + sz * 0.5f;
            float r = sz * 0.38f;
            const int dots = 8;
            for (int i = 0; i < dots; ++i) {
                float a = angle - i * 0.65f;
                float alphaRatio = 1.0f - (float(i) / float(dots));
                Color dotCol(col.r, col.g, col.b, (uint8_t)(col.a * alphaRatio));
                int dotR = std::max(2, sz / 9);
                int dx = (int)std::round(cx + std::cos(a) * r) - dotR;
                int dy = (int)std::round(cy + std::sin(a) * r) - dotR;
                DrawRoundRect(dx, dy, dotR * 2, dotR * 2, dotR, dotCol, dotCol);
            }
        });

        AdvanceCursor(sz, sz);
        return *this;
    }

    void Toast(const std::string& textUtf8, Color accentColor = Color(89, 173, 250), DWORD durationMs = 2500) {
        if (textUtf8.empty()) return;
        toastText = textUtf8;
        toastColor = accentColor;
        toastStartTime = GetTickCount();
        toastDurationMs = durationMs;
        hasActiveAnimations = true;
        RequestRedraw();
    }

    bool IsLastItemHovered() const {
        return !modalBlocking && openComboId == 0 && IsMouseInRect(lastItemLeft, lastItemTop, lastItemRight - lastItemLeft, lastItemBottom - lastItemTop);
    }

    float GetLastItemHoverRatioX() const {
        int w = lastItemRight - lastItemLeft;
        if (w <= 0) return 0.0f;
        return std::clamp(float(mousePos.x - lastItemLeft) / float(w), 0.0f, 1.0f);
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

    // --- POPUP MODAL ---

    void OpenPopup(const std::string& id) {
        activePopupId = id;
        lastHoveredIndex = -1;
        openComboId = 0;
        focusedInputId = 0;
        RequestRedraw();
    }

    void CloseCurrentPopup() {
        activePopupId.clear();
        lastHoveredIndex = -1;
        focusedInputId = 0;
        RequestRedraw();
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
            DrawRoundRect(mx + S(3), my + S(4), modalW, modalH, S(6), Color(14, 14, 17, 220), Color(14, 14, 17, 220));
            DrawRoundRect(mx, my, modalW, modalH, S(6), style.popupBg, style.checkMark);
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

    // --- AUTO-AJUSTE DA JANELA ---

    void AutoResizeWindowHeight() {
        if (!hwnd || !activePopupId.empty()) return;
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
            RequestRedraw();
        }
    }

    // --- HELPERS DE CONVERSÃO UTF-8 <-> UTF-16 ---

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
#ifdef SHAWARMAUI_USE_D2D
    Microsoft::WRL::ComPtr<ID2D1Factory1> d2dFactory;
    Microsoft::WRL::ComPtr<ID2D1Device> d2dDevice;
    Microsoft::WRL::ComPtr<ID2D1DeviceContext> d2dContext;
    Microsoft::WRL::ComPtr<ID2D1Bitmap1> d2dTargetBitmap;
    Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> d2dHwndTarget;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> d2dBrush;
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
#else
    HDC hdc = nullptr;
    HDC backDC = nullptr;
    HBITMAP backBM = nullptr;
    HGDIOBJ oldBackBM = nullptr;
    int backW = 0, backH = 0;

    HDC checkDC = nullptr;
    HBITMAP checkBM = nullptr;
    HGDIOBJ oldCheckBM = nullptr;
    int cachedCheckBigSz = 0;

    HFONT fontRegular = nullptr, fontBold = nullptr;
    std::unordered_map<uint32_t, HBRUSH> brushCache;
    std::unordered_map<uint64_t, HPEN> penCache;
#endif

    std::unordered_map<std::string, int> textWidthCacheRegular;
    std::unordered_map<std::string, int> textWidthCacheBold;

    std::function<void(Context&)> renderCallback;

    int clientW = 0, clientH = 0;
    POINT mousePos = { -1, -1 };
    bool mouseDown = false;
    bool mouseClicked = false;
    bool trackingMouse = false;

    std::vector<RECT> interactiveRects;
    int lastHoveredIndex = -1;

    int widgetCounter = 0;
    int activeDragId = 0;
    std::unordered_map<int, float> animValues;
    bool hasActiveAnimations = false;
    bool animTimerRunning = false;
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

    bool panelActive = false;
    int panelStartY = 0, panelLeft = 0, panelRight = 0;
    std::string panelTitle;
    Color panelTitleColor;
    std::vector<std::function<void()>> deferredDraws;

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
    int hoveredChildId = 0;
    float pendingWheelDelta = 0.0f;

    std::vector<RECT> clipStack;

    int focusedInputId = 0;
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

    int S(int v) const { return (int)std::round(v * dpiScale); }
    bool IsDisabled() const { return !disabledStack.empty() && disabledStack.back(); }

    static int HashString(const std::string& str) {
        uint32_t h = 2166136261u;
        for (unsigned char c : str) {
            h ^= c;
            h *= 16777619u;
        }
        int res = static_cast<int>(h & 0x7FFFFFFF);
        return (res == 0) ? 1 : res;
    }

    float Animate(int id, float target, float speed = 15.0f) {
        float& current = animValues[id];
        float diff = target - current;
        if (std::abs(diff) < 0.005f) {
            current = target;
        } else {
            current += diff * std::min(1.0f, deltaTime * speed);
            hasActiveAnimations = true;
        }
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
        for (int i = 0; i < (int)interactiveRects.size(); ++i) {
            const RECT& r = interactiveRects[i];
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
        else if (panelActive) deferredDraws.emplace_back(std::forward<Fn>(fn));
        else fn();
    }

    void FlushDeferredDraws() {
        for (auto& fn : deferredDraws) fn();
        deferredDraws.clear();
    }

    void BeginFrameState() {
        interactiveRects.clear();
        modalDraws.clear();
        comboOverlayDraws.clear();
        pendingTooltipText.clear();
        clipStack.clear();
        childStack.clear();
        hoveredChildId = 0;

        DWORD now = GetTickCount();
        deltaTime = (lastFrameTime == 0) ? 0.033f : std::min(0.06f, (now - lastFrameTime) / 1000.0f);
        lastFrameTime = now;
        widgetCounter = 0;
        hasActiveAnimations = false;

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
                    alpha = std::clamp(float(toastDurationMs - elapsed) / 400.0f, 0.0f, 1.0f);
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
        pendingWheelDelta = 0.0f;
        pendingChars.clear();
        pendingKeys.clear();
    }

    void DrawToastInternal(const std::string& textUtf8, Color accent, float alpha) {
        int padX = S(14);
        int textW = CalcTextWidth(textUtf8, true);
        int boxW = std::min(clientW - S(24), textW + padX * 2);
        int boxH = S(30);
        int bx = clientW - boxW - S(14);
        int by = clientH - boxH - S(14);

        Color bg(style.popupBg.r, style.popupBg.g, style.popupBg.b, (uint8_t)(245 * alpha));
        Color bdr(accent.r, accent.g, accent.b, (uint8_t)(230 * alpha));
        Color txt(style.text.r, style.text.g, style.text.b, (uint8_t)(255 * alpha));

        DrawRoundRect(bx, by, boxW, boxH, S(style.rounding), bg, bdr);
        DrawTextInternal(bx + padX, by, std::max(0, boxW - padX * 2), boxH, textUtf8, txt, true, DT_CENTER);
    }

    void DrawTooltipInternal(int x, int y, const std::string& textUtf8) {
        int padX = S(8), padY = S(4);
        int textW = CalcTextWidth(textUtf8, false);
        int boxW = textW + padX * 2;
        int boxH = S(22);

        if (x + boxW > clientW - S(4)) x = clientW - boxW - S(4);
        if (x < S(4)) x = S(4);
        if (y < S(4)) y = mousePos.y + S(18);

        DrawRoundRect(x, y, boxW, boxH, S(4), style.tooltipBg, style.border);
        DrawTextInternal(x + padX, y, textW, boxH, textUtf8, style.text, false, DT_LEFT);
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

#ifdef SHAWARMAUI_USE_D2D
    ID2D1RenderTarget* ActiveD2DTarget() const {
        if (d2dContext) return d2dContext.Get();
        return d2dHwndTarget.Get();
    }

    void InitGraphicsBackend() {
        D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, d2dFactory.GetAddressOf());
        DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                            reinterpret_cast<IUnknown**>(dwriteFactory.GetAddressOf()));

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
            }
            if (dwriteBold) {
                dwriteBold->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
                dwriteBold->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
            }
        }

        if (hwnd && d2dFactory && !externalSwapChain && !d2dHwndTarget) {
            RECT rc;
            GetClientRect(hwnd, &rc);
            D2D1_SIZE_U sz = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
            d2dFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(),
                                               D2D1::HwndRenderTargetProperties(hwnd, sz),
                                               &d2dHwndTarget);
            if (d2dHwndTarget) {
                d2dHwndTarget->SetDpi(96.0f, 96.0f);
                d2dHwndTarget->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1), &d2dBrush);
            }
        }
    }

    bool InitD2DFromSwapChain(IDXGISwapChain1* swapChain) {
        InitGraphicsBackend();
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

    void CleanupResources() {
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

    int CalcTextWidth(const std::string& textUtf8, bool bold) {
        if (textUtf8.empty() || !dwriteFactory) return 0;
        auto& cache = bold ? textWidthCacheBold : textWidthCacheRegular;
        auto it = cache.find(textUtf8);
        if (it != cache.end()) return it->second;
        if (cache.size() > 512) cache.clear();

        std::wstring w = Utf8ToWide(textUtf8);
        IDWriteTextFormat* fmt = bold ? dwriteBold.Get() : dwriteRegular.Get();
        Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
        if (SUCCEEDED(dwriteFactory->CreateTextLayout(w.c_str(), (UINT32)w.size(), fmt, 4096.0f, 100.0f, &layout))) {
            DWRITE_TEXT_METRICS metrics = {};
            layout->GetMetrics(&metrics);
            int width = (int)std::ceil(metrics.widthIncludingTrailingWhitespace);
            cache[textUtf8] = width;
            return width;
        }
        return 0;
    }

    void DrawRoundRect(int x, int y, int w, int h, int radius, Color fill, Color borderCol) {
        ID2D1RenderTarget* rt = ActiveD2DTarget();
        if (!rt || !d2dBrush) return;
        D2D1_ROUNDED_RECT rr = { D2D1::RectF((float)x, (float)y, (float)(x + w), (float)(y + h)), (float)radius, (float)radius };
        if (fill.a > 0) {
            d2dBrush->SetColor(fill.ToD2D());
            rt->FillRoundedRectangle(&rr, d2dBrush.Get());
        }
        if (borderCol.a > 0 && borderCol.Pack() != fill.Pack()) {
            d2dBrush->SetColor(borderCol.ToD2D());
            rt->DrawRoundedRectangle(&rr, d2dBrush.Get(), 1.0f);
        }
    }

    void DrawLine(int x1, int y1, int x2, int y2, Color col) {
        ID2D1RenderTarget* rt = ActiveD2DTarget();
        if (!rt || !d2dBrush) return;
        d2dBrush->SetColor(col.ToD2D());
        rt->DrawLine(D2D1::Point2F((float)x1, (float)y1), D2D1::Point2F((float)x2, (float)y2), d2dBrush.Get(), 1.0f);
    }

    void DrawCheckmarkCached(int x, int y, int boxSz, Color, Color chkCol) {
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

    void DrawTextInternal(int x, int y, int w, int h, const std::string& textUtf8, Color col, bool bold, UINT align) {
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
            if (SUCCEEDED(dwriteFactory->CreateTextLayout(textW.c_str(), (UINT32)textW.size(), fmt, (float)w, (float)h, &newLayout))) {
                if (align & DT_CENTER) newLayout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
                else if (align & DT_RIGHT) newLayout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
                else newLayout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);

                layoutPtr = newLayout.Get();
                textLayoutCache.emplace(std::move(key), std::move(newLayout));
            }
        }

        if (layoutPtr) {
            d2dBrush->SetColor(col.ToD2D());
            rt->DrawTextLayout(D2D1::Point2F((float)x, (float)y), layoutPtr, d2dBrush.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }
    }

    void DrawDimOverlay(Color dimCol) {
        DrawRoundRect(0, 0, clientW, clientH, 0, dimCol, dimCol);
    }

    void ApplyClipRect(const RECT& r) {
        ID2D1RenderTarget* rt = ActiveD2DTarget();
        if (rt) rt->PushAxisAlignedClip(D2D1::RectF((float)r.left, (float)r.top, (float)r.right, (float)r.bottom), D2D1_ANTIALIAS_MODE_ALIASED);
    }

    void RemoveClipRect() {
        ID2D1RenderTarget* rt = ActiveD2DTarget();
        if (rt) rt->PopAxisAlignedClip();
    }

#else
    void InitGraphicsBackend() {
        fontRegular = CreateFontW((int)(16 * dpiScale), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        fontBold = CreateFontW((int)(16 * dpiScale), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
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

    HBRUSH GetBrush(Color c) {
        uint32_t key = c.Pack();
        auto it = brushCache.find(key);
        if (it != brushCache.end()) return it->second;
        HBRUSH b = CreateSolidBrush(c.ToWin32());
        brushCache[key] = b;
        return b;
    }

    HPEN GetPen(Color c, int width = 1) {
        uint64_t key = (uint64_t(width) << 32) | c.Pack();
        auto it = penCache.find(key);
        if (it != penCache.end()) return it->second;
        HPEN p = CreatePen(PS_SOLID, width, c.ToWin32());
        penCache[key] = p;
        return p;
    }

    void CleanupResources() {
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
        }
    }

    int CalcTextWidth(const std::string& textUtf8, bool bold) {
        if (!hdc || textUtf8.empty()) return 0;
        auto& cache = bold ? textWidthCacheBold : textWidthCacheRegular;
        auto it = cache.find(textUtf8);
        if (it != cache.end()) return it->second;
        if (cache.size() > 512) cache.clear();

        std::wstring w = Utf8ToWide(textUtf8);
        SelectObject(hdc, bold ? fontBold : fontRegular);
        SIZE sz = {};
        GetTextExtentPoint32W(hdc, w.c_str(), (int)w.size(), &sz);
        cache[textUtf8] = sz.cx;
        return sz.cx;
    }

    void DrawRoundRect(int x, int y, int w, int h, int radius, Color fill, Color borderCol) {
        HGDIOBJ oldB = SelectObject(hdc, GetBrush(fill));
        HGDIOBJ oldP = SelectObject(hdc, GetPen(borderCol, 1));
        if (radius > 0) RoundRect(hdc, x, y, x + w, y + h, radius, radius);
        else Rectangle(hdc, x, y, x + w, y + h);
        SelectObject(hdc, oldB);
        SelectObject(hdc, oldP);
    }

    void DrawLine(int x1, int y1, int x2, int y2, Color col) {
        HGDIOBJ oldP = SelectObject(hdc, GetPen(col, 1));
        MoveToEx(hdc, x1, y1, nullptr);
        LineTo(hdc, x2, y2);
        SelectObject(hdc, oldP);
    }

    void DrawCheckmarkCached(int x, int y, int boxSz, Color bgCol, Color chkCol) {
        const int scale = 3;
        int bigSz = boxSz * scale;
        EnsureCheckBuffer(bigSz);

        RECT r = { 0, 0, bigSz, bigSz };
        FillRect(checkDC, &r, GetBrush(bgCol));

        LOGBRUSH lb = { BS_SOLID, chkCol.ToWin32(), 0 };
        HPEN pen = ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_ROUND | PS_JOIN_ROUND,
                                std::max(3, (int)(bigSz * 0.13f)), &lb, 0, nullptr);
        HGDIOBJ oldP = SelectObject(checkDC, pen);

        MoveToEx(checkDC, (int)(bigSz * 0.24f), (int)(bigSz * 0.52f), nullptr);
        LineTo(checkDC, (int)(bigSz * 0.43f), (int)(bigSz * 0.72f));
        LineTo(checkDC, (int)(bigSz * 0.76f), (int)(bigSz * 0.30f));

        SelectObject(checkDC, oldP);
        DeleteObject(pen);

        SetStretchBltMode(hdc, HALFTONE);
        SetBrushOrgEx(hdc, 0, 0, nullptr);
        int pad = 2;
        StretchBlt(hdc, x + pad, y + pad, boxSz - pad * 2, boxSz - pad * 2,
                   checkDC, pad * scale, pad * scale, bigSz - pad * 2 * scale, bigSz - pad * 2 * scale, SRCCOPY);
    }

    void DrawTextInternal(int x, int y, int w, int h, const std::string& textUtf8, Color col, bool bold, UINT align) {
        if (textUtf8.empty() || w <= 0) return;
        std::wstring textW = Utf8ToWide(textUtf8);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, col.ToWin32());
        SelectObject(hdc, bold ? fontBold : fontRegular);
        RECT rc = { x, y, x + w, y + h };
        DrawTextW(hdc, textW.c_str(), -1, &rc, align | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }

    void DrawDimOverlay(Color dimCol) {
        if (clientW <= 0 || clientH <= 0) return;
        HDC dimDC = CreateCompatibleDC(hdc);
        HBITMAP dimBM = CreateCompatibleBitmap(hdc, 1, 1);
        HGDIOBJ oldBM = SelectObject(dimDC, dimBM);
        SetPixel(dimDC, 0, 0, dimCol.ToWin32());

        BLENDFUNCTION bf = { AC_SRC_OVER, 0, dimCol.a, 0 };
        AlphaBlend(hdc, 0, 0, clientW, clientH, dimDC, 0, 0, 1, 1, bf);

        SelectObject(dimDC, oldBM);
        DeleteObject(dimBM);
        DeleteDC(dimDC);
    }

    void ApplyClipRect(const RECT& r) {
        if (!hdc) return;
        SaveDC(hdc);
        IntersectClipRect(hdc, r.left, r.top, r.right, r.bottom);
    }

    void RemoveClipRect() {
        if (hdc) RestoreDC(hdc, -1);
    }
#endif

    void PushClip(int x, int y, int w, int h) {
        RECT r = { x, y, x + w, y + h };
        if (!clipStack.empty()) {
            const RECT& prev = clipStack.back();
            r.left   = std::max(r.left, prev.left);
            r.top    = std::max(r.top, prev.top);
            r.right  = std::max(r.left, std::min(r.right, prev.right));
            r.bottom = std::max(r.top, std::min(r.bottom, prev.bottom));
        }
        clipStack.push_back(r);
        ApplyClipRect(r);
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
            auto fn = reinterpret_cast<SetProcessDpiAwarenessContextFn>(GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
            if (fn) { fn(reinterpret_cast<HANDLE>(-4)); return; }
        }
        SetProcessDPIAware();
    }

    static float GetSystemDpiScale() {
        HDC hdc = GetDC(nullptr);
        int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(nullptr, hdc);
        return (float)dpi / 96.0f;
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

            case WM_SIZE:
                clientW = LOWORD(lParam);
                clientH = HIWORD(lParam);
#ifdef SHAWARMAUI_USE_D2D
                if (d2dHwndTarget && clientW > 0 && clientH > 0) {
                    d2dHwndTarget->Resize(D2D1::SizeU(clientW, clientH));
                }
#endif
                RequestRedraw();
                return 0;

            case WM_TIMER:
                if (wParam == 1) {
                    RequestRedraw();
                } else if (wParam == 2 && onTimer) {
                    onTimer();
                    RequestRedraw();
                }
                return 0;

            case WM_DROPFILES: {
                HDROP hDrop = reinterpret_cast<HDROP>(wParam);
                UINT len = DragQueryFileW(hDrop, 0, nullptr, 0);
                if (len > 0 && onFileDrop) {
                    std::wstring path(len + 1, L'\0');
                    if (DragQueryFileW(hDrop, 0, path.data(), len + 1) > 0) {
                        onFileDrop(WideToUtf8(path.c_str()));
                    }
                }
                DragFinish(hDrop);
                RequestRedraw();
                return 0;
            }

            case WM_MOUSEMOVE: {
                POINT pt = { (short)LOWORD(lParam), (short)HIWORD(lParam) };
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
                    RequestRedraw();
                }
                return 0;
            }

            case WM_MOUSELEAVE:
                trackingMouse = false;
                mousePos = { -1, -1 };
                if (lastHoveredIndex != -1) {
                    lastHoveredIndex = -1;
                    RequestRedraw();
                }
                return 0;

            case WM_MOUSEWHEEL: {
                pendingWheelDelta += (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
                RequestRedraw();
                return 0;
            }

            case WM_LBUTTONDOWN:
                mouseDown = true;
                mouseClicked = true;
                SetCapture(hWnd);
                RequestRedraw();
                return 0;

            case WM_LBUTTONUP:
                mouseDown = false;
                activeDragId = 0;
                ReleaseCapture();
                RequestRedraw();
                return 0;

            case WM_CHAR:
                if (focusedInputId != 0) {
                    pendingChars.push_back(static_cast<wchar_t>(wParam));
                    RequestRedraw();
                    return 0;
                }
                break;

            case WM_KEYDOWN:
                if (focusedInputId != 0) {
                    pendingKeys.push_back(static_cast<UINT>(wParam));
                    RequestRedraw();
                    return 0;
                }
                if (openComboId != 0 && wParam == VK_ESCAPE) {
                    openComboId = 0;
                    RequestRedraw();
                    return 0;
                }
                break;

            case WM_KILLFOCUS:
                mouseDown = false;
                activeDragId = 0;
                focusedInputId = 0;
                openComboId = 0;
                mousePos = { -1, -1 };
                lastHoveredIndex = -1;
                RequestRedraw();
                return 0;

            case WM_PAINT: {
                PAINTSTRUCT ps;
                HDC realDC = BeginPaint(hWnd, &ps);
                RECT rc;
                GetClientRect(hWnd, &rc);
                clientW = rc.right - rc.left;
                clientH = rc.bottom - rc.top;

                if (clientW > 0 && clientH > 0) {
#ifdef SHAWARMAUI_USE_D2D
                    if (d2dHwndTarget) {
                        d2dHwndTarget->BeginDraw();
                        d2dHwndTarget->SetTransform(D2D1::Matrix3x2F::Identity());
                        d2dHwndTarget->Clear(style.windowBg.ToD2D());
                        BeginFrameState();
                        if (renderCallback) renderCallback(*this);
                        EndFrameState();
                        if (d2dHwndTarget->EndDraw() == D2DERR_RECREATE_TARGET) {
                            d2dHwndTarget.Reset();
                            InitGraphicsBackend();
                        }
                    }
#else
                    EnsureBackBuffer(realDC, clientW, clientH);
                    hdc = backDC;
                    BeginFrameState();
                    DrawRoundRect(0, 0, clientW, clientH, 0, style.windowBg, style.windowBg);
                    if (renderCallback) renderCallback(*this);
                    EndFrameState();
                    BitBlt(realDC, 0, 0, clientW, clientH, backDC, 0, 0, SRCCOPY);
#endif
                    if (hasActiveAnimations && !animTimerRunning) {
                        UINT intervalMs = (style.animFps > 0) ? std::max(10u, 1000u / (UINT)style.animFps) : 33u;
                        SetTimer(hWnd, 1, intervalMs, nullptr);
                        animTimerRunning = true;
                    } else if (!hasActiveAnimations && animTimerRunning) {
                        KillTimer(hWnd, 1);
                        animTimerRunning = false;
                        lastFrameTime = 0;
                    }
                }

                EndPaint(hWnd, &ps);
                return 0;
            }

            case WM_DESTROY:
                if (animTimerRunning) KillTimer(hWnd, 1);
                PostQuitMessage(0);
                return 0;
        }
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
};

} // namespace ShawarmaUI
