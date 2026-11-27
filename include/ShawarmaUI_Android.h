#pragma once
#include <GLES3/gl3.h>
#include <android/log.h>
#include "nanovg.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <chrono>

namespace ShawarmaUI {

struct Color {
    uint8_t r, g, b, a;
    constexpr Color(uint8_t r = 255, uint8_t g = 255, uint8_t b = 255, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}
    uint32_t Pack() const { return (uint32_t(a) << 24) | (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(b); }

    NVGcolor ToNVG() const {
        return nvgRGBA(r, g, b, a);
    }

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
    NeonRed = 0,
    CyberPurple,
    RoseGold,
    LightClean
};

struct Style {
    Color windowBg          = Color(12, 12, 16, 255);
    Color panelBg           = Color(24, 16, 22, 125);
    Color childBg           = Color(20, 14, 18, 115);
    Color popupBg           = Color(24, 20, 28, 235);
    Color tooltipBg         = Color(20, 18, 24, 240);
    Color modalDimBg        = Color(8, 6, 10, 185);
    Color border            = Color(230, 25, 65, 110);
    Color borderFocus       = Color(255, 50, 90, 255);
    Color text              = Color(246, 243, 244, 255);
    Color textDim           = Color(168, 156, 160, 255);
    Color frameBg           = Color(36, 18, 24, 140);
    Color frameHover        = Color(62, 20, 34, 180);
    Color checkMark         = Color(235, 35, 75, 255);
    Color sliderGrab        = Color(255, 40, 85, 255);
    Color button            = Color(225, 32, 68, 235);
    Color buttonGradEnd     = Color(165, 16, 42, 245);
    Color buttonHover       = Color(245, 45, 82, 250);
    Color buttonActive      = Color(185, 20, 50, 255);
    Color toggleActive      = Color(225, 32, 68, 240);
    Color toggleHover       = Color(250, 48, 88, 255);
    Color separator         = Color(255, 255, 255, 28);
    Color disabledBg        = Color(30, 24, 26, 140);
    Color disabledText      = Color(115, 104, 108, 255);
    Color scrollbarBg       = Color(28, 22, 25, 150);
    Color scrollbarGrab     = Color(98, 76, 84, 220);
    Color secondaryBtn      = Color(43, 50, 64, 240);
    Color secondaryBtnHover = Color(58, 68, 86, 250);
    Color success           = Color(129, 199, 132, 255);
    Color warning           = Color(255, 213, 79, 255);
    Color danger            = Color(229, 115, 115, 255);

    // Dimensões confortáveis para toque no Android (em dp)
    int windowPadding   = 16;
    int framePaddingX   = 12;
    int framePaddingY   = 8;
    int itemSpacingX    = 10;
    int itemSpacingY    = 10;
    int rounding        = 8;
    int touchHeight     = 38;
    int animFps         = 30; // Interface (botões, scroll, widgets)
    int backdropFps     = 15; // Animação de fundo (orbes)
};

struct RectI {
    int left = 0, top = 0, right = 0, bottom = 0;
};

struct PointI {
    int x = -1, y = -1;
};

class Context {
public:
    Style style;
    Theme currentTheme = Theme::NeonRed;
    float backdropSeed = 0.0f;
    NVGcontext* vg = nullptr;
    float dpiScale = 1.0f;
    int safeAreaTop = 0;
    int safeAreaBottom = 0;

    std::function<void()> onRequestRedraw;
    std::function<void(bool show)> onShowKeyboard;

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
    ChildScope Child(const std::string& idStr, int customW = -1, int customH = 220, bool border = true) {
        return ChildScope(*this, idStr, customW, customH, border);
    }
    DisabledScope Disabled(bool disabled = true) {
        return DisabledScope(*this, disabled);
    }
	
    struct IdScope {
        Context& ctx;
        IdScope(Context& c, int id) : ctx(c) { ctx.PushID(id); }
        IdScope(Context& c, const std::string& id) : ctx(c) { ctx.PushID(id); }
        ~IdScope() { ctx.PopID(); }
        explicit operator bool() const { return true; }
    };

    ModalScope Modal(const std::string& id, int customW = 340, int customH = 160) {
        return ModalScope(*this, id, customW, customH);
    }
    IdScope ID(int id) { return IdScope(*this, id); }
    IdScope ID(const std::string& id) { return IdScope(*this, id); }

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
        Shutdown();
    }

    void ApplyTheme(Theme theme) {
        currentTheme = theme;
        style = Style{};
        if (theme == Theme::CyberPurple) {
            style.windowBg      = Color(13, 15, 20, 255);
            style.panelBg       = Color(24, 27, 36, 125);
            style.childBg       = Color(20, 23, 30, 115);
            style.popupBg       = Color(28, 32, 42, 235);
            style.border        = Color(140, 110, 190, 85);
            style.borderFocus   = Color(186, 104, 200, 255);
            style.frameBg       = Color(32, 36, 46, 140);
            style.frameHover    = Color(46, 52, 66, 180);
            style.checkMark     = Color(206, 147, 216, 255);
            style.sliderGrab    = Color(186, 104, 200, 255);
            style.button        = Color(142, 44, 205, 235);
            style.buttonGradEnd = Color(92, 22, 148, 245);
            style.buttonHover   = Color(142, 36, 170, 245);
            style.buttonActive  = Color(74, 20, 140, 255);
            style.toggleActive  = Color(123, 31, 162, 240);
            style.toggleHover   = Color(156, 39, 176, 255);
            } else if (theme == Theme::RoseGold) {
            style.windowBg      = Color(12, 12, 15, 255);
            style.panelBg       = Color(26, 25, 30, 125);
            style.childBg       = Color(20, 19, 24, 115);
            style.popupBg       = Color(24, 23, 28, 235);
            style.border        = Color(255, 255, 255, 38);
            style.borderFocus   = Color(224, 128, 108, 230);
            style.frameBg       = Color(38, 36, 43, 135);
            style.frameHover    = Color(54, 50, 60, 175);
            style.checkMark     = Color(224, 128, 108, 255);
            style.sliderGrab    = Color(230, 136, 115, 255);
            style.button        = Color(214, 98, 96, 235);
            style.buttonGradEnd = Color(168, 66, 68, 245);
            style.buttonHover   = Color(230, 114, 112, 250);
            style.buttonActive  = Color(178, 68, 76, 255);
            style.toggleActive  = Color(214, 98, 96, 245);
            style.toggleHover   = Color(235, 118, 116, 255);
        } else if (theme == Theme::LightClean) {
            style.windowBg          = Color(210, 220, 234, 255);
            style.panelBg           = Color(234, 240, 248, 195);
            style.childBg           = Color(226, 234, 244, 180);
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
			
        }
    }

    // Inicializa NanoVG (deve ser chamado dentro de onSurfaceCreated no thread GL)
    bool InitGLES3(NVGcontext* externalVg, float density = 2.5f) {
        vg = externalVg;
        dpiScale = std::max(1.0f, density);
        textWidthCacheRegular.clear();
        textWidthCacheBold.clear();

        if (!vg) return false;

        fontRegularId = nvgCreateFont(vg, "sans", "/system/fonts/Roboto-Regular.ttf");
        if (fontRegularId < 0) {
            fontRegularId = nvgCreateFont(vg, "sans", "/system/fonts/DroidSans.ttf");
        }
        fontBoldId = nvgCreateFont(vg, "sans-bold", "/system/fonts/Roboto-Bold.ttf");
        if (fontBoldId < 0) {
            fontBoldId = fontRegularId;
        }
        return (fontRegularId >= 0);
    }

    // Registra uma fonte de icones (.ttf) da memoria como fallback para textos e botoes
    bool LoadIconFontFromMemory(const uint8_t* ttfData, int dataSize) {
        if (!vg || !ttfData || dataSize <= 0 || fontRegularId < 0) return false;
        unsigned char* ptr = const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(ttfData));
        int iconId = nvgCreateFontMem(vg, "shawarma-icons", ptr, dataSize, 0);
        if (iconId < 0) return false;

        nvgAddFallbackFontId(vg, fontRegularId, iconId);
        if (fontBoldId >= 0 && fontBoldId != fontRegularId) {
            nvgAddFallbackFontId(vg, fontBoldId, iconId);
        }
        textWidthCacheRegular.clear();
        textWidthCacheBold.clear();
        return true;
    }

    void Shutdown() {
        textWidthCacheRegular.clear();
        textWidthCacheBold.clear();
        vg = nullptr;
    }

    void SetSafeAreaInsets(int topPx, int bottomPx) {
        safeAreaTop = std::max(0, topPx);
        safeAreaBottom = std::max(0, bottomPx);
    }

    void RequestRedraw(bool force = false) {
        if (!onRequestRedraw) return;
        uint64_t now = NowMs();
        uint64_t intervalMs = (style.animFps > 0) ? (1000ULL / style.animFps) : 33ULL;
        if (force || (now - lastRedrawRequestMs >= intervalMs)) {
            lastRedrawRequestMs = now;
            onRequestRedraw();
        }
    }

    // --- ENTRADA POR TOQUE (TOUCH EVENTS) ---

    void OnTouchDown(int x, int y) {
        touchPos = { x, y };
        touchStartPos = { x, y };
        lastTouchDragPos = { x, y };
        touchDown = true;
        touchClicked = true;
        touchIsDraggingScroll = false;
        touchDownTimeMs = NowMs();
        RequestRedraw(true);
    }

    void OnTouchMove(int x, int y) {
        if (!touchDown) return;
        int dy = y - lastTouchDragPos.y;
        lastTouchDragPos = { x, y };
        touchPos = { x, y };

        if (!touchIsDraggingScroll && activeDragId == 0) {
            int totalMove = std::abs(x - touchStartPos.x) + std::abs(y - touchStartPos.y);
            if (totalMove > S(8)) {
                touchIsDraggingScroll = true;
            }
        }

        if (touchIsDraggingScroll) {
            if (activeScrollChildId != 0) {
                childScrollMap[activeScrollChildId] -= static_cast<float>(dy);
            } else {
                windowScrollY -= static_cast<float>(dy);
            }
        }
        RequestRedraw(false); // Limitado a 30 FPS durante o arrasto
    }

    void OnTouchUp(int x, int y) {
        touchPos = { x, y };
        touchDown = false;
        touchReleased = true;
        activeDragId = 0;
        activeScrollChildId = 0;
        touchIsDraggingScroll = false;
        RequestRedraw(true);
    }

    // --- ENTRADA DE TECLADO VIRTUAL ---

    void OnTextCommit(const std::string& utf8Text) {
        if (focusedInputId != 0 && !utf8Text.empty()) {
            pendingTextCommits.push_back(utf8Text);
            RequestRedraw(true);
        }
    }

    void OnKeyBackspace() {
        if (focusedInputId != 0) {
            pendingBackspaceCount++;
            RequestRedraw(true);
        }
    }

    void OnKeyEnter() {
        if (focusedInputId != 0) {
            pendingEnterPressed = true;
            RequestRedraw(true);
        }
    }

    // --- CICLO DE FRAME (MODO APP COMPLETO OU HUD OVERLAY) ---

    void BeginFrame(int width, int height) {
        clientW = width;
        clientH = height;
        if (!vg) return;

        glViewport(0, 0, width, height);
        glClearColor(style.windowBg.r / 255.0f, style.windowBg.g / 255.0f, style.windowBg.b / 255.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        nvgBeginFrame(vg, static_cast<float>(width), static_cast<float>(height), 1.0f);
        BeginFrameState();              // <-- PRIMEIRO limpa o estado
        DrawGlassBackdrop(width, height); // <-- DEPOIS desenha e marca hasBackdropAnimation = true
    }

    void EndFrame() {
        if (!vg) return;
        EndFrameState();
        nvgEndFrame(vg);
    }

    // Modo Overlay (para desenhar controles por cima de vídeo AMediaCodec ou imagem OpenGL ES)
    void BeginHUDFrame(int width, int height) {
        clientW = width;
        clientH = height;
        if (!vg) return;
        nvgBeginFrame(vg, static_cast<float>(width), static_cast<float>(height), 1.0f);
        BeginFrameState();
    }

    void EndHUDFrame() {
        if (!vg) return;
        EndFrameState();
        nvgEndFrame(vg);
    }

    bool HasActiveAnimations() const { return hasActiveAnimations || hasBackdropAnimation; }

    int GetNextFrameDelayMs() const {
        if (hasActiveAnimations) return (style.animFps > 0) ? (1000 / style.animFps) : 33;
        if (hasBackdropAnimation) return (style.backdropFps > 0) ? (1000 / style.backdropFps) : 66;
        return 0;
    }

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

    Context& Spacing(int dp = -1) {
        cursorY += (dp < 0) ? S(style.itemSpacingY) : S(dp);
        return *this;
    }

    Context& Separator() {
        int y = cursorY + S(3);
        int l = regionLeft, r = regionRight;
        Color c = style.border;
        QueueOrDraw([=]() { DrawLine(l, y, r, y, c); });
        cursorY += S(10);
        return *this;
    }

    Context& SeparatorText(const std::string& labelUtf8, Color labelCol = Color(0, 0, 0, 0)) {
        std::string disp = StripIdSuffix(labelUtf8);
        int h = S(22);
        int y = cursorY;
        int l = regionLeft, r = regionRight;
        int midY = y + h / 2;
        int textW = CalcTextWidth(disp, true);
        int pad = S(8);
        Color col = (labelCol.a > 0) ? labelCol : style.textDim;
        Color lineCol = style.border;

        QueueOrDraw([=, this]() {
            int line1End = l + S(12);
            if (line1End > l) DrawLine(l, midY, line1End, midY, lineCol);
            int textX = line1End + pad;
            int drawW = std::min(textW + S(4), std::max(0, r - textX));
            DrawTextInternal(textX, y, drawW, h, disp, col, true, ALIGN_LEFT);
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

    // --- CONTAINERS (PAINÉIS E ÁREA COM SCROLL POR TOQUE) ---

    void BeginPanel(const std::string& titleUtf8 = "", Color titleColor = Color(115, 191, 250), int customWidth = -1) {
        PanelState ps{};
        ps.startY = cursorY;
        ps.left = cursorX;
        ps.right = (customWidth > 0) ? (cursorX + S(customWidth)) : regionRight;
        ps.savedRegionLeft = regionLeft;
        ps.savedRegionRight = regionRight;
        ps.title = StripIdSuffix(titleUtf8);
        ps.titleColor = titleColor;
        panelStack.push_back(std::move(ps));

        int pad = S(12);
        regionLeft = panelStack.back().left + pad;
        regionRight = panelStack.back().right - pad;
        cursorX = regionLeft;
        cursorY += pad;
        if (!panelStack.back().title.empty()) cursorY += S(26);
    }

    void EndPanel() {
        if (panelStack.empty()) return;
        PanelState ps = std::move(panelStack.back());
        panelStack.pop_back();

        int pad = S(12);
        int panelBottom = cursorY + pad - S(style.itemSpacingY);
        int w = ps.right - ps.left;
        int h = panelBottom - ps.startY;
        float r = (float)S(style.rounding);
        Color bg = style.panelBg;
        Color bdr = style.border;

        auto drawSelf = [=, this]() {
            DrawRoundRect(ps.left, ps.startY, w, h, S(style.rounding), bg, Color(0, 0, 0, 0));

            if (vg && w > 0 && h > 0) {
                NVGpaint glassBorder = nvgLinearGradient(
                    vg, (float)ps.left, (float)ps.startY, (float)ps.left, (float)panelBottom,
                    nvgRGBA(255, 255, 255, 75),
                    nvgRGBA(255, 255, 255, 18)
                );
                nvgBeginPath(vg);
                nvgRoundedRect(vg, ps.left + 0.5f, ps.startY + 0.5f, w - 1.0f, h - 1.0f, r);
                nvgStrokePaint(vg, glassBorder);
                nvgStrokeWidth(vg, std::max(1.0f, 1.0f * dpiScale * 0.5f));
                nvgStroke(vg);
            }

            if (!ps.title.empty()) {
                int titleY = ps.startY + S(10);
                DrawTextInternal(ps.left + pad, titleY, w - pad * 2, S(22),
                                 ps.title, ps.titleColor, true, ALIGN_LEFT);
                int sepY = titleY + S(24);
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

    void BeginChild(const std::string& idStr, int customW = -1, int customH = 220, bool border = true) {
        int id = HashString("child_" + idStr);
        int w = (customW < 0) ? GetAvailWidth() : S(customW);
        int bottomSafe = std::max(S(style.windowPadding), safeAreaBottom);
        int h = (customH <= 0) ? std::max(S(80), clientH - cursorY - bottomSafe) : S(customH);
        int x = cursorX, y = cursorY;

        bool touchedInside = !modalBlocking && (openComboId == 0) && IsTouchInRect(x, y, w, h);
        if (touchedInside && touchDown && activeDragId == 0) {
            activeScrollChildId = id;
        }

        float maxPrev = childMaxScrollMap[id];
        float scrollY = std::clamp(childScrollMap[id], 0.0f, maxPrev);
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
        cs.startContentY = y + S(8) - (int)std::round(scrollY);
        childStack.push_back(cs);

        Color bgCol = style.childBg;
        Color bdrCol = border ? style.border : style.childBg;
        int rad = S(style.rounding);
        QueueOrDraw([=]() {
            DrawRoundRect(x, y, w, h, rad, bgCol, bdrCol);
        });

        int pad = S(10);
        int scrollBarSpace = S(8);
        regionLeft = x + pad;
        regionRight = x + w - pad - scrollBarSpace;
        cursorX = regionLeft;
        cursorY = cs.startContentY;

        int cx = x + 1, cy = y + 1;
        int cw = std::max(0, w - 2), ch = std::max(0, h - 2);
        PushClipLogical(cx, cy, cw, ch);
        QueueOrDraw([=]() {
            if (vg) {
                nvgSave(vg);
                nvgIntersectScissor(vg, (float)cx, (float)cy, (float)cw, (float)ch);
            }
        });
    }

    void EndChild() {
        if (childStack.empty()) return;
        ChildState cs = childStack.back();
        childStack.pop_back();

        PopClipLogical();
        QueueOrDraw([=]() {
            if (vg) nvgRestore(vg);
        });

        int contentH = (cursorY - cs.startContentY) + S(8);
        int maxScroll = std::max(0, contentH - cs.h);
		childMaxScrollMap[cs.id] = (float)maxScroll;
        float& scrollY = childScrollMap[cs.id];
        scrollY = std::clamp(scrollY, 0.0f, (float)maxScroll);

        if (maxScroll > 0 && cs.h > S(24)) {
            int barW = S(4);
            int barX = cs.x + cs.w - barW - S(3);
            int barY = cs.y + S(6);
            int barH = cs.h - S(12);
            Color sbBg = style.scrollbarBg;
            Color sbGrab = style.scrollbarGrab;

            float visibleRatio = std::clamp(float(cs.h) / float(contentH), 0.12f, 1.0f);
            int grabH = std::max(S(22), (int)std::round(barH * visibleRatio));
            float scrollRatio = scrollY / float(maxScroll);
            int grabY = barY + (int)std::round((barH - grabH) * scrollRatio);

            QueueOrDraw([=]() {
                DrawRoundRect(barX, barY, barW, barH, barW / 2, sbBg, sbBg);
                DrawRoundRect(barX, grabY, barW, grabH, barW / 2, sbGrab, sbGrab);
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
        int offsetY = alignTextToFrame ? S(8) : 0;
        alignTextToFrame = false;
        if (IsDisabled()) color = style.disabledText;
        else color = EnsureContrast(color);
        int h = S(22);
        int w = CalcTextWidth(disp, bold);
        int drawW = std::min(w + S(4), GetAvailWidth());
        int x = cursorX, y = cursorY + offsetY;

        QueueOrDraw([=, this]() {
            DrawTextInternal(x, y, drawW, h, disp, color, bold, ALIGN_LEFT);
        });
        AdvanceCursor(drawW, h);
        return *this;
    }

    Context& Badge(const std::string& textUtf8, Color accentColor = Color(89, 173, 250)) {
        accentColor = EnsureContrast(accentColor);
        std::string disp = StripIdSuffix(textUtf8);
        int padX = S(10);
        int h = S(24);
        int textW = CalcTextWidth(disp, true);
        int w = std::min(textW + padX * 2, GetAvailWidth());
        int x = cursorX, y = cursorY;

        Color bgCol = Color::Lerp(style.panelBg, accentColor, 0.20f);
        Color borderCol = Color::Lerp(style.panelBg, accentColor, 0.68f);

        QueueOrDraw([=, this]() {
            DrawRoundRect(x, y, w, h, h / 2, bgCol, borderCol);
            DrawTextInternal(x + padX, y, std::max(0, w - padX * 2), h, disp, accentColor, true, ALIGN_CENTER);
        });
        AdvanceCursor(w, h);
        return *this;
    }

    Context& KeyValue(const std::string& keyUtf8, const std::string& valUtf8, Color valColor = Color(0, 0, 0, 0)) {
        std::string dispKey = StripIdSuffix(keyUtf8);
        std::string dispVal = StripIdSuffix(valUtf8);
        int h = S(22);
        int x = cursorX, y = cursorY;
        int availW = GetAvailWidth();
        int keyW = std::min(CalcTextWidth(dispKey + ": ", false), availW);
        int valW = std::max(0, availW - keyW);
        Color kCol = IsDisabled() ? style.disabledText : style.textDim;
        Color vCol = IsDisabled() ? style.disabledText : ((valColor.a > 0) ? valColor : style.text);

        QueueOrDraw([=, this]() {
            DrawTextInternal(x, y, keyW, h, dispKey + ": ", kCol, false, ALIGN_LEFT);
            if (valW > 0) {
                DrawTextInternal(x + keyW, y, valW, h, dispVal, vCol, true, ALIGN_LEFT);
            }
        });
        AdvanceCursor(availW, h);
        return *this;
    }

    Context& InputTextReadOnly(const std::string& textUtf8, int customWidth = -1) {
        int w = (customWidth < 0) ? GetAvailWidth() : S(customWidth);
        int h = S(style.touchHeight);
        int x = cursorX, y = cursorY;

        QueueOrDraw([=, this]() {
            DrawRoundRect(x, y, w, h, S(style.rounding), style.frameBg, style.border);
            DrawTextInternal(x + S(10), y, std::max(0, w - S(20)), h, textUtf8,
                             textUtf8.empty() ? style.textDim : style.text, false, ALIGN_LEFT);
        });
        AdvanceCursor(w, h);
        return *this;
    }

    bool InputText(const std::string& idStr, std::string& bufferUtf8, const std::string& placeholderUtf8 = "",
                   int customWidth = -1, bool isPassword = false) {
        int id = HashString(idStr);
        int w = (customWidth < 0) ? GetAvailWidth() : S(customWidth);
        int h = S(style.touchHeight);
        int x = cursorX, y = cursorY;
        bool disabled = IsDisabled();

        bool touched = RegisterInteractive(x, y, w, h, disabled);

        if (touchReleased && !touchIsDraggingScroll && !disabled && !modalBlocking && openComboId == 0) {
            if (touched) {
                focusedInputId = id;
                cursorBlinkStart = NowMs();
                if (onShowKeyboard) onShowKeyboard(true);
            } else if (focusedInputId == id) {
                focusedInputId = 0;
                if (onShowKeyboard) onShowKeyboard(false);
            }
        }

        bool focused = (focusedInputId == id) && !disabled;
        bool changed = false;

        if (focused) {
            inputRenderedThisFrame = true;
            hasActiveAnimations = true;
            for (const auto& txt : pendingTextCommits) {
                cursorBlinkStart = NowMs();
                bufferUtf8 += txt;
                changed = true;
            }
            for (int b = 0; b < pendingBackspaceCount; ++b) {
                cursorBlinkStart = NowMs();
                if (!bufferUtf8.empty()) {
                    PopLastUtf8Char(bufferUtf8);
                    changed = true;
                }
            }
            if (pendingEnterPressed) {
                focusedInputId = 0;
                focused = false;
                if (onShowKeyboard) onShowKeyboard(false);
            }
        }

        std::string displayStr = bufferUtf8;
        if (isPassword && !bufferUtf8.empty()) {
            size_t chars = CountUtf8Chars(bufferUtf8);
            displayStr = std::string(chars, '*');
        }

        float focusT = Animate(id * 2, focused ? 1.0f : 0.0f, 16.0f);
        Color bgCol = disabled ? style.disabledBg : Color::Lerp(style.frameBg, style.frameHover, focusT);
        Color borderCol = focused ? style.borderFocus : style.border;

        QueueOrDraw([=, this]() {
            DrawRoundRect(x, y, w, h, S(style.rounding), bgCol, borderCol);
            int padX = S(10);
            int innerW = std::max(0, w - padX * 2);

            PushClip(x + padX, y + 2, innerW, h - 4);
            int textW = CalcTextWidth(displayStr, false);
            int scrollTextX = (focused && textW > innerW - S(4)) ? (textW - (innerW - S(4))) : 0;

            if (displayStr.empty() && !placeholderUtf8.empty()) {
                DrawTextInternal(x + padX, y, innerW, h, placeholderUtf8, style.textDim, false, ALIGN_LEFT);
            } else if (!displayStr.empty()) {
                DrawTextInternal(x + padX - scrollTextX, y, innerW + scrollTextX + S(20), h, displayStr,
                                 disabled ? style.disabledText : style.text, false, ALIGN_LEFT);
            }

            if (focused) {
                uint64_t elapsed = NowMs() - cursorBlinkStart;
                if ((elapsed / 500) % 2 == 0) {
                    int cx = x + padX + textW - scrollTextX;
                    DrawLine(cx, y + S(7), cx, y + h - S(7), style.text);
                }
            }
            PopClip();
        });

        AdvanceCursor(w, h);
        return changed;
    }

    // Editor de Texto Multilinha (ideal para Bloco de Notas, Editor de Texto e Presets FFmpeg)
    bool InputTextMultiline(const std::string& idStr, std::string& bufferUtf8, int customWidth = -1, int customHeight = 180) {
        int id = HashString(idStr);
        int w = (customWidth < 0) ? GetAvailWidth() : S(customWidth);
        int bottomSafe = std::max(S(style.windowPadding), safeAreaBottom);
        int h = (customHeight <= 0) ? std::max(S(100), clientH - cursorY - bottomSafe) : S(customHeight);
        int x = cursorX, y = cursorY;
        bool disabled = IsDisabled();

        bool touched = RegisterInteractive(x, y, w, h, disabled);
        if (touched && touchDown) {
            activeScrollChildId = id;
        }

        if (touchReleased && !touchIsDraggingScroll && !disabled && !modalBlocking && openComboId == 0) {
            if (touched) {
                focusedInputId = id;
                cursorBlinkStart = NowMs();
                if (onShowKeyboard) onShowKeyboard(true);
            } else if (focusedInputId == id) {
                focusedInputId = 0;
                if (onShowKeyboard) onShowKeyboard(false);
            }
        }

        bool focused = (focusedInputId == id) && !disabled;
        bool changed = false;

        if (focused) {
            inputRenderedThisFrame = true;
            hasActiveAnimations = true;
            for (const auto& txt : pendingTextCommits) {
                cursorBlinkStart = NowMs();
                bufferUtf8 += txt;
                changed = true;
            }
            for (int b = 0; b < pendingBackspaceCount; ++b) {
                cursorBlinkStart = NowMs();
                if (!bufferUtf8.empty()) {
                    PopLastUtf8Char(bufferUtf8);
                    changed = true;
                }
            }
            if (pendingEnterPressed) {
                cursorBlinkStart = NowMs();
                bufferUtf8.push_back('\n');
                changed = true;
            }
        }

        std::vector<std::string> lines = SplitLines(bufferUtf8);
        int lineH = S(22);
        int pad = S(10);
        int totalContentH = (int)lines.size() * lineH + pad * 2;
        int maxScroll = std::max(0, totalContentH - h);
        float& scrollY = childScrollMap[id];
        if (changed) scrollY = static_cast<float>(maxScroll);
        scrollY = std::clamp(scrollY, 0.0f, static_cast<float>(maxScroll));

        Color borderCol = focused ? style.borderFocus : style.border;
        Color bgCol = disabled ? style.disabledBg : style.frameBg;

        QueueOrDraw([=, this]() {
            DrawRoundRect(x, y, w, h, S(style.rounding), bgCol, borderCol);
            PushClip(x + 2, y + 2, std::max(0, w - 4), std::max(0, h - 4));

            int drawY = y + pad - (int)std::round(scrollY);
            int innerW = std::max(0, w - pad * 2);

            for (size_t i = 0; i < lines.size(); ++i) {
                int ly = drawY + (int)i * lineH;
                if (ly + lineH >= y && ly <= y + h) {
                    DrawTextInternal(x + pad, ly, innerW, lineH, lines.at(i),
                                     disabled ? style.disabledText : style.text, false, ALIGN_LEFT);
                }
            }

            if (focused) {
                uint64_t elapsed = NowMs() - cursorBlinkStart;
                if ((elapsed / 500) % 2 == 0) {
                    const std::string& lastLine = lines.empty() ? std::string() : lines.back();
                    int lastLineY = drawY + std::max(0, (int)lines.size() - 1) * lineH;
                    int cx = x + pad + CalcTextWidth(lastLine, false);
                    DrawLine(cx, lastLineY + S(2), cx, lastLineY + lineH - S(2), style.text);
                }
            }
            PopClip();
        });

        AdvanceCursor(w, h);
        return changed;
    }

    bool Checkbox(const std::string& labelUtf8, bool& value) {
        int id = MakeWidgetId("chk_" + labelUtf8);
        std::string disp = StripIdSuffix(labelUtf8);
        int boxSz = S(22);
        int h = S(30);
        int textW = CalcTextWidth(disp, false);
        int totalW = std::min(boxSz + S(10) + textW + S(4), GetAvailWidth());
        int x = cursorX, y = cursorY;
        bool disabled = IsDisabled();

        bool touched = RegisterInteractive(x, y, totalW, h, disabled);
        bool clicked = touched && touchReleased && !touchIsDraggingScroll;
        if (clicked) value = !value;

        float pressT = Animate(id * 2, (touched && touchDown) ? 1.0f : 0.0f, 18.0f);
        float checkT = Animate(id * 2 + 1, value ? 1.0f : 0.0f, 18.0f);

        Color bgCol = disabled ? style.disabledBg : Color::Lerp(style.frameBg, style.frameHover, pressT);
        Color txtCol = disabled ? style.disabledText : style.text;
        Color baseChk = disabled ? style.disabledText : style.checkMark;
        Color chkCol = Color::Lerp(bgCol, baseChk, checkT);

        QueueOrDraw([=, this]() {
            int boxY = y + (h - boxSz) / 2;
            DrawRoundRect(x, boxY, boxSz, boxSz, S(5), bgCol, style.border);
            if (checkT > 0.02f) {
                DrawCheckmarkNVG(x, boxY, boxSz, chkCol);
            }
            DrawTextInternal(x + boxSz + S(10), y, std::max(0, totalW - boxSz - S(10)), h, disp, txtCol, false, ALIGN_LEFT);
        });

        AdvanceCursor(totalW, h);
        return clicked;
    }

    bool ToggleSwitch(const std::string& labelUtf8, bool& value) {
        int id = MakeWidgetId("tgl_" + labelUtf8);
        std::string disp = StripIdSuffix(labelUtf8);
        int swW = S(44);
        int swH = S(24);
        int h = S(30);
        int textW = disp.empty() ? 0 : (CalcTextWidth(disp, false) + S(10));
        int totalW = std::min(swW + textW + S(4), GetAvailWidth());
        int x = cursorX, y = cursorY;
        bool disabled = IsDisabled();

        bool touched = RegisterInteractive(x, y, totalW, h, disabled);
        bool clicked = touched && touchReleased && !touchIsDraggingScroll;
        if (clicked) value = !value;

        float pressT = Animate(id * 2, (touched && touchDown) ? 1.0f : 0.0f, 18.0f);
        float switchT = Animate(id * 2 + 1, value ? 1.0f : 0.0f, 18.0f);

        Color offBg = Color::Lerp(style.frameBg, style.frameHover, pressT);
        Color onBg  = Color::Lerp(style.toggleActive, style.toggleHover, pressT);
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

            if (!labelUtf8.empty() && totalW > swW + S(10)) {
                DrawTextInternal(x + swW + S(10), y, totalW - swW - S(10), h, disp, txtCol, false, ALIGN_LEFT);
            }
        });

        AdvanceCursor(totalW, h);
        return clicked;
    }

    bool Button(const std::string& labelUtf8, int customW = 0, int customH = 0,
                Color customBg = Color(0, 0, 0, 0), Color customHover = Color(0, 0, 0, 0)) {
        int id = MakeWidgetId("btn_" + labelUtf8);
        std::string disp = StripIdSuffix(labelUtf8);
        int w = (customW > 0) ? S(customW) : (CalcTextWidth(disp, true) + S(style.framePaddingX * 2));
        if (customW == -1) w = GetAvailWidth();
        int h = (customH > 0) ? S(customH) : S(style.touchHeight);
        int x = cursorX, y = cursorY;
        bool disabled = IsDisabled();

        bool touched = RegisterInteractive(x, y, w, h, disabled);
        bool pressed = touched && touchDown && !touchIsDraggingScroll;
        bool clicked = touched && touchReleased && !touchIsDraggingScroll;

        float pressT = Animate(id * 2, pressed ? 1.0f : 0.0f, 22.0f);

        Color cNormal = (customBg.a > 0) ? customBg : style.button;
        Color cHover  = (customHover.a > 0) ? customHover : style.buttonHover;
        Color activeCol = Color::Lerp(cNormal, cHover, pressT);
        Color bgCol   = disabled ? style.disabledBg : activeCol;
		
        float luma    = 0.299f * bgCol.r + 0.587f * bgCol.g + 0.114f * bgCol.b;
        Color autoTxt = (luma < 145.0f) ? Color(246, 243, 244, 255) : Color(26, 32, 44, 255);
        Color txtCol  = disabled ? style.disabledText : autoTxt;

        QueueOrDraw([=, this]() {
            if (!disabled && customBg.a == 0) {
                Color endCol = Color::Lerp(style.buttonGradEnd, style.buttonHover, pressT);
                DrawMeshRect(x, y, w, h, S(style.rounding), bgCol, endCol, bgCol);
            } else {
                DrawRoundRect(x, y, w, h, S(style.rounding), bgCol, disabled ? style.border : bgCol);
            }
            DrawTextInternal(x, y, w, h, disp, txtCol, true, ALIGN_CENTER);
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

    bool IconButton(int customW, int customH, std::function<void(NVGcontext* vg, int x, int y, int w, int h)> onDrawIcon,
                    Color customBg = Color(0, 0, 0, 0), Color customHover = Color(0, 0, 0, 0)) {
        int id = ++widgetCounter;
        int w = S(customW);
        int h = S(customH);
        int x = cursorX, y = cursorY;
        bool disabled = IsDisabled();

        bool touched = RegisterInteractive(x, y, w, h, disabled);
        bool pressed = touched && touchDown && !touchIsDraggingScroll;
        bool clicked = touched && touchReleased && !touchIsDraggingScroll;

        float pressT = Animate(id * 2, pressed ? 1.0f : 0.0f, 22.0f);

        Color cNormal = (customBg.a > 0) ? customBg : style.button;
        Color cHover  = (customHover.a > 0) ? customHover : style.buttonHover;
        Color activeCol = Color::Lerp(cNormal, cHover, pressT);
        Color bgCol   = disabled ? style.disabledBg : activeCol;

        QueueOrDraw([=, this]() {
            DrawRoundRect(x, y, w, h, S(style.rounding), bgCol, disabled ? style.border : bgCol);
            if (onDrawIcon && vg) onDrawIcon(vg, x, y, w, h);
        });

        AdvanceCursor(w, h);
        return clicked;
    }

    bool TabBar(int& currentTab, const std::vector<std::string>& labels, int customHeight = 38) {
        if (labels.empty()) return false;
        int count = (int)labels.size();
        int totalW = GetAvailWidth();
        int h = S(customHeight);
        int x = cursorX, y = cursorY;
        int spacing = S(4);
        int tabW = std::max(S(32), (totalW - spacing * (count - 1)) / count);
        bool disabled = IsDisabled();
        bool changed = false;

        for (int i = 0; i < count; ++i) {
            int id = MakeWidgetId("tab_" + std::to_string(i));
            std::string disp = StripIdSuffix(labels.at(i));
            int tx = x + i * (tabW + spacing);
            int curW = (i == count - 1) ? std::max(tabW, (x + totalW) - tx) : tabW;
            bool isSelected = (currentTab == i);

            bool touched = RegisterInteractive(tx, y, curW, h, disabled);
            if (touched && touchReleased && !touchIsDraggingScroll && currentTab != i) {
                currentTab = i;
                changed = true;
            }

            float pressT = Animate(id * 2, (touched && touchDown) ? 1.0f : 0.0f, 18.0f);
            float activeT = Animate(id * 2 + 1, isSelected ? 1.0f : 0.0f, 18.0f);

            Color baseBg = Color::Lerp(style.frameBg, style.frameHover, pressT);
            Color activeBg = Color::Lerp(style.toggleActive, style.toggleHover, pressT);
            Color bgCol = disabled ? style.disabledBg : Color::Lerp(baseBg, activeBg, activeT);
            Color selectedTxt = (currentTheme == Theme::LightClean) ? Color(255, 255, 255, 255) : style.text;
            Color txtCol = disabled ? style.disabledText : (isSelected ? selectedTxt : Color::Lerp(style.textDim, style.text, pressT));

            QueueOrDraw([=, this]() {
                if (!disabled && isSelected) {
                    DrawMeshRect(tx, y, curW, h, S(style.rounding), bgCol, style.buttonGradEnd, style.borderFocus);
                } else {
                    DrawRoundRect(tx, y, curW, h, S(style.rounding), bgCol, style.border);
                }
                DrawTextInternal(tx, y, curW, h, disp, txtCol, isSelected, ALIGN_CENTER);
            });
        }

        AdvanceCursor(totalW, h);
        return changed;
    }

    bool SegmentedControl(int& currentIndex, const std::vector<std::string>& options, int customWidth = -1, int customHeight = 36) {
        if (options.empty()) return false;
        int count = (int)options.size();
        int totalW = (customWidth < 0) ? GetAvailWidth() : S(customWidth);
        int h = S(customHeight);
        int x = cursorX, y = cursorY;
        int segW = std::max(S(28), totalW / count);
        bool disabled = IsDisabled();
        bool changed = false;

        QueueOrDraw([=, this]() {
            DrawRoundRect(x, y, totalW, h, S(style.rounding), style.frameBg, style.border);
        });

        for (int i = 0; i < count; ++i) {
            int id = MakeWidgetId("seg_" + std::to_string(i));
			std::string disp = StripIdSuffix(options.at(i));
            int sx = x + i * segW;
            int curW = (i == count - 1) ? std::max(segW, (x + totalW) - sx) : segW;
            bool isSelected = (currentIndex == i);

            bool touched = RegisterInteractive(sx, y, curW, h, disabled);
            if (touched && touchReleased && !touchIsDraggingScroll && currentIndex != i) {
                currentIndex = i;
                changed = true;
            }

            float pressT = Animate(id * 2, (touched && touchDown) ? 1.0f : 0.0f, 18.0f);
            float activeT = Animate(id * 2 + 1, isSelected ? 1.0f : 0.0f, 18.0f);

            Color activeBg = Color::Lerp(style.toggleActive, style.toggleHover, pressT);
            Color pressBg  = Color(style.frameHover.r, style.frameHover.g, style.frameHover.b, (uint8_t)(pressT * 180));
            Color segCol   = disabled ? style.disabledBg : Color::Lerp(pressBg, activeBg, activeT);
            Color selectedTxt = (currentTheme == Theme::LightClean) ? Color(255, 255, 255, 255) : style.text;
            Color txtCol = disabled ? style.disabledText : (isSelected ? selectedTxt : Color::Lerp(style.textDim, style.text, pressT));

            QueueOrDraw([=, this]() {
                if (activeT > 0.02f || pressT > 0.02f) {
                    int pad = S(3);
                    int rx = sx + pad, ry = y + pad;
                    int rw = std::max(0, curW - pad * 2), rh = std::max(0, h - pad * 2);
                    int rr = std::max(2, S(style.rounding) - 2);
                    if (!disabled && isSelected) {
                        DrawMeshRect(rx, ry, rw, rh, rr, segCol, style.buttonGradEnd, Color(0, 0, 0, 0));
                    } else {
                        DrawRoundRect(rx, ry, rw, rh, rr, segCol, segCol);
                    }
                }
                DrawTextInternal(sx, y, curW, h, disp, txtCol, isSelected, ALIGN_CENTER);
            });
        }

        AdvanceCursor(totalW, h);
        return changed;
    }

    bool SelectableRow(const std::string& titleUtf8, const std::string& subtitleUtf8 = "", bool selected = false, int customHeight = 46) {
        int id = MakeWidgetId("row_" + titleUtf8);
        std::string dispTitle = StripIdSuffix(titleUtf8);
        std::string dispSub = StripIdSuffix(subtitleUtf8);
        int w = GetAvailWidth();
        int h = S(customHeight);
        int x = cursorX, y = cursorY;
        bool disabled = IsDisabled();

        bool touched = RegisterInteractive(x, y, w, h, disabled);
        bool clicked = touched && touchReleased && !touchIsDraggingScroll;

        float pressT = Animate(id * 2, (touched && touchDown && !touchIsDraggingScroll) ? 1.0f : 0.0f, 18.0f);
        float selT   = Animate(id * 2 + 1, selected ? 1.0f : 0.0f, 18.0f);

        Color baseCol = Color::Lerp(style.panelBg, style.frameHover, pressT);
        Color selCol  = Color::Lerp(style.toggleActive, style.toggleHover, pressT);
        Color bgCol   = disabled ? style.disabledBg : Color::Lerp(baseCol, selCol, selT * 0.45f);
        Color bdrCol  = selected ? style.borderFocus : style.border;

        QueueOrDraw([=, this]() {
            DrawRoundRect(x, y, w, h, S(style.rounding), bgCol, bdrCol);
            int padX = S(12);
            int subW = dispSub.empty() ? 0 : (CalcTextWidth(dispSub, false) + S(12));
            int titleW = std::max(0, w - padX * 2 - subW);
            DrawTextInternal(x + padX, y, titleW, h, dispTitle, disabled ? style.disabledText : style.text, true, ALIGN_LEFT);
            if (subW > 0) {
                DrawTextInternal(x + w - padX - subW, y, subW, h, dispSub, style.textDim, false, ALIGN_RIGHT);
            }
        });

        AdvanceCursor(w, h);
        return clicked;
    }

    bool Combo(const std::string& idStr, int& currentIndex, const std::vector<std::string>& items, int customWidth = -1) {
        int id = HashString(idStr);
        int w = (customWidth < 0) ? GetAvailWidth() : S(customWidth);
        int h = S(style.touchHeight);
        int x = cursorX, y = cursorY;
        bool disabled = IsDisabled();

        bool isOpen = (openComboId == id);
        bool canInteract = !disabled && !modalBlocking && (openComboId == 0 || isOpen);

        if (canInteract && IsInsideActiveClip(x, y, w, h)) {
            interactiveRects.push_back({ x, y, x + w, y + h });
        }

        bool touched = canInteract && IsTouchInRect(x, y, w, h) && IsInsideActiveClip(x, y, w, h);
        if (touched && touchReleased && !touchIsDraggingScroll) {
            openComboId = isOpen ? 0 : id;
            isOpen = (openComboId == id);
            touchReleased = false;
        }

        float pressT = Animate(id * 2, ((touched && touchDown) || isOpen) ? 1.0f : 0.0f, 18.0f);
        Color bgCol = disabled ? style.disabledBg : Color::Lerp(style.frameBg, style.frameHover, pressT);
        Color borderCol = isOpen ? style.borderFocus : style.border;

        std::string preview = (!items.empty() && currentIndex >= 0 && currentIndex < (int)items.size())
                              ? StripIdSuffix(items.at(currentIndex)) : "Select...";

        QueueOrDraw([=, this]() {
            DrawRoundRect(x, y, w, h, S(style.rounding), bgCol, borderCol);
            int arrowW = S(28);
            DrawTextInternal(x + S(10), y, std::max(0, w - arrowW - S(10)), h, preview,
                             disabled ? style.disabledText : style.text, false, ALIGN_LEFT);
            DrawComboArrowNVG(x + w - arrowW, y, arrowW, h, isOpen,
                              disabled ? style.disabledText : style.textDim);
        });

        bool selectionChanged = false;
        if (isOpen && !items.empty()) {
            int itemH = S(36);
            int dropH = (int)items.size() * itemH + S(8);
            int dropY = y + h + S(4);
            int bottomSafe = std::max(S(6), safeAreaBottom);
            int topSafe = std::max(S(6), safeAreaTop);
            if (dropY + dropH > clientH - bottomSafe && y - dropH - S(4) >= topSafe) {
                dropY = y - dropH - S(4);
            }

            interactiveRects.push_back({ x, dropY, x + w, dropY + dropH });

            int touchedOption = -1;
            for (int i = 0; i < (int)items.size(); ++i) {
                int optY = dropY + S(4) + i * itemH;
                if (IsTouchInRect(x + S(4), optY, w - S(8), itemH)) {
                    touchedOption = i;
                    if (touchReleased && !touchIsDraggingScroll) {
                        if (currentIndex != i) {
                            currentIndex = i;
                            selectionChanged = true;
                        }
                        openComboId = 0;
                        touchReleased = false;
                    }
                }
            }

            if (touchReleased && !touchIsDraggingScroll && !IsTouchInRect(x, dropY, w, dropH) && !touched) {
                openComboId = 0;
            }

            comboOverlayDraws.push_back([=, this]() {
                DrawRoundRect(x + S(2), dropY + S(4), w, dropH, S(style.rounding), Color(10, 10, 14, 210), Color(10, 10, 14, 210));
                DrawRoundRect(x, dropY, w, dropH, S(style.rounding), style.popupBg, style.borderFocus);
                for (int i = 0; i < (int)items.size(); ++i) {
                    int optY = dropY + S(4) + i * itemH;
                    if (i == touchedOption && touchDown) {
                        DrawRoundRect(x + S(4), optY, w - S(8), itemH, S(6), style.buttonHover, style.buttonHover);
                    } else if (i == currentIndex) {
                        DrawRoundRect(x + S(4), optY, w - S(8), itemH, S(6), style.frameHover, style.frameHover);
                    }
                    DrawTextInternal(x + S(12), optY, std::max(0, w - S(24)), itemH, StripIdSuffix(items.at(i)), style.text, i == currentIndex, ALIGN_LEFT);
                }
            });
        }

        AdvanceCursor(w, h);
        return selectionChanged;
    }

    bool SliderFloat(float& value, float minVal, float maxVal, int customWidth = -1, int customHeight = 26,
                     const std::string& overlayUtf8 = "", bool* outIsDragging = nullptr) {
        int id = ++widgetCounter;
        int w = (customWidth < 0) ? GetAvailWidth() : S(customWidth);
        int h = S(customHeight);
        int x = cursorX, y = cursorY;
        bool disabled = IsDisabled();

        bool touched = RegisterInteractive(x, y, w, h, disabled);
        if (touched && touchClicked) {
            activeDragId = id;
            activeScrollChildId = 0;
            touchIsDraggingScroll = false;
        }
        if (!touchDown && activeDragId == id) {
            activeDragId = 0;
        }

        bool dragging = (activeDragId == id);
        if (outIsDragging) *outIsDragging = dragging;

        bool changed = false;
        if (dragging && w > 0 && maxVal > minVal) {
            float t = std::clamp(float(touchPos.x - x) / float(w), 0.0f, 1.0f);
            float newVal = minVal + t * (maxVal - minVal);
            if (std::abs(newVal - value) > 0.0001f) {
                value = newVal;
                changed = true;
            }
        }

        float norm = (maxVal > minVal) ? std::clamp((value - minVal) / (maxVal - minVal), 0.0f, 1.0f) : 0.0f;
        float dragT = Animate(id * 2, dragging ? 1.0f : 0.0f, 18.0f);

        Color trackCol = disabled ? style.disabledBg : Color::Lerp(style.frameBg, style.frameHover, dragT);
        Color fillCol  = disabled ? style.disabledText : style.sliderGrab;

        QueueOrDraw([=, this]() {
            int trackH = std::max(S(8), h / 3);
            int trackY = y + (h - trackH) / 2;
            DrawRoundRect(x, trackY, w, trackH, trackH / 2, trackCol, style.border);

            int fillW = (int)std::round(w * norm);
            if (fillW > 0) {
                DrawRoundRect(x, trackY, fillW, trackH, trackH / 2, fillCol, fillCol);
            }

            int grabSz = std::max(S(18), h - S(2));
            int grabX = std::clamp(x + fillW - grabSz / 2, x, x + std::max(0, w - grabSz));
            int grabY = y + (h - grabSz) / 2;
            DrawRoundRect(grabX, grabY, grabSz, grabSz, grabSz / 2, style.text, fillCol);

            if (!overlayUtf8.empty()) {
                DrawTextInternal(x, y, w, h, overlayUtf8, style.text, true, ALIGN_CENTER);
            }
        });

        AdvanceCursor(w, h);
        return changed;
    }

    bool SliderInt(int& value, int minVal, int maxVal, int customWidth = -1, int customHeight = 26,
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

    Context& ProgressBar(float fraction, int customWidth = -1, int customHeight = 18,
                         Color fillCol = Color(0, 0, 0, 0), const std::string& overlayUtf8 = "") {
        fraction = std::clamp(fraction, 0.0f, 1.0f);
        int w = (customWidth < 0) ? GetAvailWidth() : S(customWidth);
        int h = S(customHeight);
        int x = cursorX, y = cursorY;
        Color barCol = (fillCol.a > 0) ? fillCol : style.button;

        QueueOrDraw([=, this]() {
            DrawRoundRect(x, y, w, h, S(4), style.frameBg, style.border);
            int fillW = (int)std::round((w - 2) * fraction);
            if (fillW > 0) {
                DrawRoundRect(x + 1, y + 1, fillW, h - 2, S(4), barCol, barCol);
            }
            if (!overlayUtf8.empty()) {
                DrawTextInternal(x, y, w, h, overlayUtf8, style.text, true, ALIGN_CENTER);
            }
        });

        AdvanceCursor(w, h);
        return *this;
    }

    Context& Spinner(int diameter = 24, Color color = Color(0, 0, 0, 0)) {
        hasActiveAnimations = true;
        int sz = S(diameter);
        int x = cursorX, y = cursorY;
        Color col = (color.a > 0) ? color : style.sliderGrab;
        float angle = (NowMs() % 1000) * 0.0062831853f;

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

    void Toast(const std::string& textUtf8, Color accentColor = Color(89, 173, 250), uint64_t durationMs = 2500) {
        if (textUtf8.empty()) return;
        toastText = textUtf8;
        toastColor = accentColor;
        toastStartTime = NowMs();
        toastDurationMs = durationMs;
        hasActiveAnimations = true;
        RequestRedraw(true);
    }

    bool IsLastItemTouched() const {
        return !modalBlocking && openComboId == 0 &&
               IsTouchInRect(lastItemLeft, lastItemTop, lastItemRight - lastItemLeft, lastItemBottom - lastItemTop);
    }

    float GetLastItemTouchRatioX() const {
        int w = lastItemRight - lastItemLeft;
        if (w <= 0) return 0.0f;
        return std::clamp(float(touchPos.x - lastItemLeft) / float(w), 0.0f, 1.0f);
    }

    // No celular, segurar o dedo por ~450ms mostra a dica acima do dedo
    Context& SetTooltip(const std::string& textUtf8) {
        if (textUtf8.empty() || !touchDown || touchIsDraggingScroll || !IsLastItemTouched()) return *this;
        uint64_t heldMs = NowMs() - touchDownTimeMs;
        if (heldMs >= 450) {
            pendingTooltipText = textUtf8;
            pendingTooltipPos = { touchPos.x - S(40), touchPos.y - S(44) };
        } else {
            hasActiveAnimations = true;
        }
        return *this;
    }

    Context& Tooltip(const std::string& textUtf8) {
        return SetTooltip(textUtf8);
    }

    // --- POPUP MODAL ---

    void OpenPopup(const std::string& id) {
        activePopupId = id;
        openComboId = 0;
        focusedInputId = 0;
        if (onShowKeyboard) onShowKeyboard(false);
        RequestRedraw(true);
    }

    void CloseCurrentPopup() {
        activePopupId.clear();
        focusedInputId = 0;
        if (onShowKeyboard) onShowKeyboard(false);
        RequestRedraw(true);
    }

    bool BeginPopupModal(const std::string& id, int customW = 340, int customH = 160) {
        if (activePopupId != id) return false;

        savedModalState = { regionLeft, regionRight, cursorX, cursorY, modalBlocking };

        int modalW = std::min(clientW - S(28), S(customW));
        int modalH = std::min(clientH - S(28), S(customH));
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

private:
    enum TextAlignFlags {
        ALIGN_LEFT   = 0,
        ALIGN_CENTER = 1,
        ALIGN_RIGHT  = 2
    };

    int fontRegularId = -1;
    int fontBoldId = -1;
    std::unordered_map<std::string, int> textWidthCacheRegular;
    std::unordered_map<std::string, int> textWidthCacheBold;

    int clientW = 0, clientH = 0;
    PointI touchPos = { -1, -1 };
    PointI touchStartPos = { -1, -1 };
    PointI lastTouchDragPos = { -1, -1 };
    bool touchDown = false;
    bool touchClicked = false;
    bool touchReleased = false;
    bool touchIsDraggingScroll = false;
    uint64_t touchDownTimeMs = 0;

    std::vector<RectI> interactiveRects;

    int widgetCounter = 0;
    int activeDragId = 0;
    int activeScrollChildId = 0;
    std::unordered_map<int, float> animValues;
    bool hasActiveAnimations = false;
    bool hasBackdropAnimation = false;
    uint64_t lastFrameTimeMs = 0;
    uint64_t lastRedrawRequestMs = 0;
    uint64_t lastBackdropTickMs = 0;
    float deltaTime = 0.016f;

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

    std::vector<RectI> clipStack;

    int focusedInputId = 0;    
    bool inputRenderedThisFrame = false;
    float windowScrollY = 0.0f;
    uint64_t cursorBlinkStart = 0;
    std::vector<std::string> pendingTextCommits;
    int pendingBackspaceCount = 0;
    bool pendingEnterPressed = false;

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
    PointI pendingTooltipPos = { 0, 0 };

    std::string toastText;
    Color toastColor;
    uint64_t toastStartTime = 0;
    uint64_t toastDurationMs = 2500;

    static uint64_t NowMs() {
        using namespace std::chrono;
        return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
    }

    int S(int v) const { return (int)std::round(v * dpiScale); }
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

    static size_t CountUtf8Chars(const std::string& s) {
        size_t count = 0;
        for (unsigned char c : s) {
            if ((c & 0xC0) != 0x80) count++;
        }
        return count;
    }

    static void PopLastUtf8Char(std::string& s) {
        while (!s.empty()) {
            unsigned char c = static_cast<unsigned char>(s.back());
            s.pop_back();
            if ((c & 0xC0) != 0x80) break;
        }
    }

    static std::vector<std::string> SplitLines(const std::string& s) {
        std::vector<std::string> lines;
        std::string cur;
        for (char c : s) {
            if (c == '\r') continue;
            if (c == '\n') {
                lines.push_back(cur);
                cur.clear();
            } else {
                cur.push_back(c);
            }
        }
        lines.push_back(cur);
        return lines;
    }

    float Animate(int id, float target, float speed = 16.0f) {
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

    bool IsTouchInRect(int x, int y, int w, int h) const {
        return (touchPos.x >= x && touchPos.x <= x + w && touchPos.y >= y && touchPos.y <= y + h);
    }

    bool IsInsideActiveClip(int x, int y, int w, int h) const {
        if (clipStack.empty()) return true;
        const RectI& c = clipStack.back();
        return (x + w > c.left && x < c.right && y + h > c.top && y < c.bottom);
    }

    bool RegisterInteractive(int x, int y, int w, int h, bool disabled) {
        if (disabled || modalBlocking || openComboId != 0 || !IsInsideActiveClip(x, y, w, h)) {
            return false;
        }
        interactiveRects.push_back({ x, y, x + w, y + h });
        return IsTouchInRect(x, y, w, h);
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
        inputRenderedThisFrame = false;

        uint64_t now = NowMs();
        deltaTime = (lastFrameTimeMs == 0) ? 0.016f : std::min(0.06f, (now - lastFrameTimeMs) / 1000.0f);
        lastFrameTimeMs = now;
        widgetCounter = 0;
        hasActiveAnimations = false;
		hasBackdropAnimation = false;

        int pad = S(style.windowPadding);
        int topStart = std::max(pad, safeAreaTop + S(8));
        windowScrollY = std::max(0.0f, windowScrollY);

        regionLeft = pad;
        regionRight = clientW - pad;
        cursorX = regionLeft;
        cursorY = topStart - static_cast<int>(std::round(windowScrollY));
        colCount = 1;
        disabledStack.clear();

        modalBlocking = !activePopupId.empty();
        inModalRender = false;
    }

    void EndFrameState() {
		while (!childStack.empty()) EndChild();
        while (!panelStack.empty()) EndPanel();
        while (!clipStack.empty()) PopClip();
        int bottomSafe = std::max(S(style.windowPadding), safeAreaBottom);
        int contentBottom = cursorY + static_cast<int>(std::round(windowScrollY)) + bottomSafe;
        int maxWinScroll = std::max(0, contentBottom - clientH);
        windowScrollY = std::clamp(windowScrollY, 0.0f, static_cast<float>(maxWinScroll));

        if (focusedInputId != 0 && !inputRenderedThisFrame) {
            focusedInputId = 0;
            if (onShowKeyboard) onShowKeyboard(false);
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
            uint64_t elapsed = NowMs() - toastStartTime;
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

        if (touchReleased) {
            touchPos = { -1, -1 };
            touchReleased = false;
        }
        touchClicked = false;
        pendingTextCommits.clear();
        pendingBackspaceCount = 0;
        pendingEnterPressed = false;
    }

    void DrawToastInternal(const std::string& textUtf8, Color accent, float alpha) {
        int padX = S(16);
        int textW = CalcTextWidth(textUtf8, true);
        int boxW = std::min(clientW - S(28), textW + padX * 2);
        int boxH = S(34);
        int bx = (clientW - boxW) / 2;
        int by = std::max(S(14), safeAreaTop + S(8));

        Color bg(style.popupBg.r, style.popupBg.g, style.popupBg.b, (uint8_t)(248 * alpha));
        Color bdr(accent.r, accent.g, accent.b, (uint8_t)(235 * alpha));
        Color txt(style.text.r, style.text.g, style.text.b, (uint8_t)(255 * alpha));

        DrawRoundRect(bx, by, boxW, boxH, boxH / 2, bg, bdr);
        DrawTextInternal(bx + padX, by, std::max(0, boxW - padX * 2), boxH, textUtf8, txt, true, ALIGN_CENTER);
    }

    void DrawTooltipInternal(int x, int y, const std::string& textUtf8) {
        int padX = S(10);
        int textW = CalcTextWidth(textUtf8, false);
        int boxW = textW + padX * 2;
        int boxH = S(26);
        int topSafe = std::max(S(6), safeAreaTop);

        if (x + boxW > clientW - S(6)) x = clientW - boxW - S(6);
        if (x < S(6)) x = S(6);
        if (y < topSafe) y = touchPos.y + S(24);

        DrawRoundRect(x, y, boxW, boxH, S(6), style.tooltipBg, style.border);
        DrawTextInternal(x + padX, y, textW, boxH, textUtf8, style.text, false, ALIGN_LEFT);
    }

    int CalcTextWidth(const std::string& textUtf8, bool bold) {
        if (!vg || textUtf8.empty()) return 0;
        auto& cache = bold ? textWidthCacheBold : textWidthCacheRegular;
        auto it = cache.find(textUtf8);
        if (it != cache.end()) return it->second;
        if (cache.size() > 512) cache.clear();

        nvgFontFaceId(vg, bold ? fontBoldId : fontRegularId);
        nvgFontSize(vg, 15.0f * dpiScale);
        float adv = nvgTextBounds(vg, 0.0f, 0.0f, textUtf8.c_str(), nullptr, nullptr);
        int w = (int)std::ceil(adv);
        cache[textUtf8] = w;
        return w;
    }

    void DrawGlassBackdrop(int w, int h) {
        if (!vg || w <= 0 || h <= 0) return;

        static uint64_t backdropStartMs = 0;
        static float cachedBackdropT = 0.0f;
        uint64_t now = NowMs();

        if (backdropStartMs == 0) {
            backdropStartMs = now;
            backdropSeed = static_cast<float>(now % 10000) * 0.137f;
            cachedBackdropT = backdropSeed;
            lastBackdropTickMs = now;
        }

        uint64_t backdropIntervalMs = (style.backdropFps > 0) ? (1000ULL / style.backdropFps) : 66ULL;
        if (now - lastBackdropTickMs >= backdropIntervalMs) {
            lastBackdropTickMs = now;
            cachedBackdropT = ((now - backdropStartMs) * 0.0014f) + backdropSeed;
        }

        hasBackdropAnimation = true;
        float t = cachedBackdropT;

        // Ondas combinadas com amplitude maior para cruzar por trás dos painéis
        float ox1 = (std::sin(t * 0.73f) * 0.6f + std::cos(t * 0.31f) * 0.4f) * S(75);
        float oy1 = (std::cos(t * 0.61f) * 0.6f + std::sin(t * 0.27f) * 0.4f) * S(60);

        float ox2 = (std::cos(t * 0.53f + 1.4f) * 0.6f + std::sin(t * 0.23f) * 0.4f) * S(85);
        float oy2 = (std::sin(t * 0.67f + 0.8f) * 0.6f + std::cos(t * 0.29f) * 0.4f) * S(65);

        float ox3 = std::sin(t * 0.41f + 2.1f) * S(65);
        float oy3 = std::cos(t * 0.37f + 1.1f) * S(55);

        // Cores das luzes adaptadas ao tema atual (Padrão: NeonRed)
        NVGcolor bgTop = nvgRGBA(14, 10, 14, 255);
        NVGcolor bgBot = nvgRGBA(8, 8, 10, 255);
        NVGcolor cOrb1 = nvgRGBA(245, 55, 95, 85);    // Brilho Vermelho Neon superior
        NVGcolor cOrb2 = nvgRGBA(210, 40, 80, 70);    // Anel Carmesim inferior
        NVGcolor cOrb3 = nvgRGBA(180, 60, 100, 45);   // Luz suave esquerda
        NVGcolor cHole = nvgRGBA(12, 11, 14, 150);

        if (currentTheme == Theme::RoseGold) {
            bgTop = nvgRGBA(14, 13, 16, 255);
            cOrb1 = nvgRGBA(235, 145, 120, 90);
            cOrb2 = nvgRGBA(205, 125, 110, 75);
            cOrb3 = nvgRGBA(185, 135, 125, 50);
        } else if (currentTheme == Theme::CyberPurple) {
            bgTop = nvgRGBA(13, 14, 22, 255);
            cOrb1 = nvgRGBA(185, 110, 235, 85);
            cOrb2 = nvgRGBA(130, 95, 220, 75);
            cOrb3 = nvgRGBA(110, 140, 230, 50);
        } else if (currentTheme == Theme::LightClean) {
            bgTop = nvgRGBA(206, 218, 234, 255);
            bgBot = nvgRGBA(190, 205, 224, 255);
            cOrb1 = nvgRGBA(110, 160, 235, 80);
            cOrb2 = nvgRGBA(135, 180, 242, 70);
            cOrb3 = nvgRGBA(235, 242, 252, 95);
            cHole = nvgRGBA(198, 210, 228, 140);
        }

        // 1. Fundo base
        NVGpaint bgGrad = nvgLinearGradient(vg, 0.0f, 0.0f, (float)w, (float)h, bgTop, bgBot);
        nvgBeginPath(vg);
        nvgRect(vg, 0.0f, 0.0f, (float)w, (float)h);
        nvgFillPaint(vg, bgGrad);
        nvgFill(vg);

        float minDim = (float)std::min(w, h);
        float maxDim = (float)std::max(w, h);

        // 2. Orbe 1 (superior direito) - desenha apenas o círculo do raio do orbe
        float x1 = w * 0.80f + ox1, y1 = h * 0.22f + oy1, r1 = minDim * 0.46f;
        NVGpaint orb1 = nvgRadialGradient(
            vg, x1, y1, minDim * 0.04f, r1,
            cOrb1, nvgRGBA(cOrb1.r * 255, cOrb1.g * 255, cOrb1.b * 255, 0)
        );
        nvgBeginPath(vg);
        nvgCircle(vg, x1, y1, r1);
        nvgFillPaint(vg, orb1);
        nvgFill(vg);

        // 3. Orbe 2 (inferior direito)
        float x2 = w * 0.66f + ox2, y2 = h * 0.72f + oy2, r2 = minDim * 0.43f;
        NVGpaint orb2 = nvgRadialGradient(
            vg, x2, y2, minDim * 0.08f, r2,
            cOrb2, nvgRGBA(cOrb2.r * 255, cOrb2.g * 255, cOrb2.b * 255, 0)
        );
        nvgBeginPath(vg);
        nvgCircle(vg, x2, y2, r2);
        nvgFillPaint(vg, orb2);
        nvgFill(vg);

        // 4. Luz secundária 3 (esquerda)
        float x3 = w * 0.14f + ox3, y3 = h * 0.54f + oy3, r3 = maxDim * 0.36f;
        NVGpaint orb3 = nvgRadialGradient(
            vg, x3, y3, minDim * 0.02f, r3,
            cOrb3, nvgRGBA(cOrb3.r * 255, cOrb3.g * 255, cOrb3.b * 255, 0)
        );
        nvgBeginPath(vg);
        nvgCircle(vg, x3, y3, r3);
        nvgFillPaint(vg, orb3);
        nvgFill(vg);
    }

    void DrawRoundRect(int x, int y, int w, int h, int radius, Color fill, Color borderCol) {
        if (!vg || w <= 0 || h <= 0) return;
        float r = std::min((float)radius, std::min(w, h) * 0.5f);

        if (fill.a > 0) {
            nvgBeginPath(vg);
            nvgRoundedRect(vg, (float)x, (float)y, (float)w, (float)h, r);
            nvgFillColor(vg, fill.ToNVG());
            nvgFill(vg);
        }
        if (borderCol.a > 0 && borderCol.Pack() != fill.Pack()) {
            nvgBeginPath(vg);
            nvgRoundedRect(vg, x + 0.5f, y + 0.5f, std::max(0.0f, w - 1.0f), std::max(0.0f, h - 1.0f), r);
            nvgStrokeColor(vg, borderCol.ToNVG());
            nvgStrokeWidth(vg, std::max(1.0f, 1.0f * dpiScale * 0.5f));
            nvgStroke(vg);
        }
    }

    void DrawMeshRect(int x, int y, int w, int h, int radius, Color c1, Color c2, Color borderCol) {
        if (!vg || w <= 0 || h <= 0) return;
        float r = std::min((float)radius, std::min(w, h) * 0.5f);

        if (c1.a > 0) {
            // Degradê vertical sutil (de cima para baixo) simulando iluminação natural
            NVGpaint mesh = nvgLinearGradient(
                vg, (float)x, (float)y, (float)x, (float)(y + h),
                c1.ToNVG(), c2.ToNVG()
            );
            nvgBeginPath(vg);
            nvgRoundedRect(vg, (float)x, (float)y, (float)w, (float)h, r);
            nvgFillPaint(vg, mesh);
            nvgFill(vg);
        }

        // Brilho especular sutil na borda (mais claro no topo, mais suave embaixo)
        NVGpaint rimLight = nvgLinearGradient(
            vg, (float)x, (float)y, (float)x, (float)(y + h),
            nvgRGBA(255, 255, 255, 75),
            nvgRGBA(255, 255, 255, 12)
        );
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x + 0.5f, y + 0.5f, std::max(0.0f, w - 1.0f), std::max(0.0f, h - 1.0f), r);
        nvgStrokePaint(vg, rimLight);
        nvgStrokeWidth(vg, std::max(1.0f, 1.0f * dpiScale * 0.5f));
        nvgStroke(vg);
    }

    void DrawLine(int x1, int y1, int x2, int y2, Color col) {
        if (!vg || col.a == 0) return;
        nvgBeginPath(vg);
        nvgMoveTo(vg, (float)x1, (float)y1);
        nvgLineTo(vg, (float)x2, (float)y2);
        nvgStrokeColor(vg, col.ToNVG());
        nvgStrokeWidth(vg, 1.0f);
        nvgStroke(vg);
    }

    void DrawCheckmarkNVG(int x, int y, int boxSz, Color chkCol) {
        if (!vg) return;
        nvgBeginPath(vg);
        nvgMoveTo(vg, x + boxSz * 0.24f, y + boxSz * 0.52f);
        nvgLineTo(vg, x + boxSz * 0.43f, y + boxSz * 0.72f);
        nvgLineTo(vg, x + boxSz * 0.76f, y + boxSz * 0.30f);
        nvgStrokeColor(vg, chkCol.ToNVG());
        nvgStrokeWidth(vg, std::max(2.0f, boxSz * 0.13f));
        nvgLineCap(vg, NVG_ROUND);
        nvgLineJoin(vg, NVG_ROUND);
        nvgStroke(vg);
    }

    void DrawComboArrowNVG(int x, int y, int w, int h, bool isOpen, Color col) {
        if (!vg) return;
        float cx = x + w * 0.5f;
        float cy = y + h * 0.5f;
        float sz = S(4);

        nvgBeginPath(vg);
        if (isOpen) {
            nvgMoveTo(vg, cx - sz, cy + sz * 0.55f);
            nvgLineTo(vg, cx,      cy - sz * 0.55f);
            nvgLineTo(vg, cx + sz, cy + sz * 0.55f);
        } else {
            nvgMoveTo(vg, cx - sz, cy - sz * 0.55f);
            nvgLineTo(vg, cx,      cy + sz * 0.55f);
            nvgLineTo(vg, cx + sz, cy - sz * 0.55f);
        }
        nvgStrokeColor(vg, col.ToNVG());
        nvgStrokeWidth(vg, std::max(1.8f, 1.5f * dpiScale));
        nvgLineCap(vg, NVG_ROUND);
        nvgLineJoin(vg, NVG_ROUND);
        nvgStroke(vg);
    }

    void DrawTextInternal(int x, int y, int w, int h, const std::string& textUtf8, Color col, bool bold, int align) {
        if (!vg || textUtf8.empty() || w <= 0) return;
        nvgSave(vg);
        nvgIntersectScissor(vg, (float)x, (float)y, (float)w, (float)h);

        nvgFontFaceId(vg, bold ? fontBoldId : fontRegularId);
        nvgFontSize(vg, 15.0f * dpiScale);
        nvgFillColor(vg, col.ToNVG());

        float drawX = (float)x;
        int nvgAlign = NVG_ALIGN_MIDDLE;
        if (align == ALIGN_CENTER) {
            drawX = x + w * 0.5f;
            nvgAlign |= NVG_ALIGN_CENTER;
        } else if (align == ALIGN_RIGHT) {
            drawX = (float)(x + w);
            nvgAlign |= NVG_ALIGN_RIGHT;
        } else {
            nvgAlign |= NVG_ALIGN_LEFT;
        }

        nvgTextAlign(vg, nvgAlign);
        nvgText(vg, drawX, y + h * 0.5f, textUtf8.c_str(), nullptr);
        nvgRestore(vg);
    }

    void DrawDimOverlay(Color dimCol) {
        DrawRoundRect(0, 0, clientW, clientH, 0, dimCol, dimCol);
    }

    void PushClipLogical(int x, int y, int w, int h) {
        RectI r = { x, y, x + w, y + h };
        if (!clipStack.empty()) {
            const RectI& prev = clipStack.back();
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
        const RectI& r = clipStack.back();
        if (vg) {
            nvgSave(vg);
            nvgIntersectScissor(vg, (float)r.left, (float)r.top,
                                (float)std::max(0, r.right - r.left),
                                (float)std::max(0, r.bottom - r.top));
        }
    }

    void PopClip() {
        if (clipStack.empty()) return;
        clipStack.pop_back();
        if (vg) nvgRestore(vg);
    }
};

} // namespace ShawarmaUI
