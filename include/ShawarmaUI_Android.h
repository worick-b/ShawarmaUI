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
    ShawarmaGold,
    NeonRed,
    CyberPurple,
    LightClean
};

struct Style {
    Color windowBg      = Color(12, 12, 14, 255);
    Color panelBg       = Color(28, 28, 32, 120);   // Painel translúcido
    Color childBg       = Color(20, 20, 24, 110);   // Sub-painel translúcido
    Color popupBg       = Color(26, 26, 30, 225);
    Color tooltipBg     = Color(22, 22, 26, 230);
    Color modalDimBg    = Color(8, 8, 10, 180);
    Color border        = Color(255, 255, 255, 38); // Borda sutil de vidro
    Color borderFocus   = Color(232, 176, 56, 220);
    Color text          = Color(245, 241, 234, 255);
    Color textDim       = Color(168, 158, 146, 255);
    Color frameBg       = Color(40, 40, 46, 130);   // Inputs translúcidos
    Color frameHover    = Color(58, 58, 66, 165);
    Color checkMark     = Color(232, 176, 56, 255);
    Color sliderGrab    = Color(232, 176, 56, 255);
    Color button        = Color(194, 136, 30, 220);
    Color buttonHover   = Color(219, 158, 42, 240);
    Color buttonActive  = Color(164, 112, 22, 255);
    Color toggleActive  = Color(196, 54, 44, 235);
    Color toggleHover   = Color(222, 68, 56, 250);
    Color separator     = Color(255, 255, 255, 30);
    Color disabledBg    = Color(30, 28, 26, 140);
    Color disabledText  = Color(115, 108, 100, 255);
    Color scrollbarBg   = Color(28, 25, 23, 150);
    Color scrollbarGrab = Color(92, 84, 78, 220);

    // Dimensões confortáveis para toque no Android (em dp)
    int windowPadding   = 16;
    int framePaddingX   = 12;
    int framePaddingY   = 8;
    int itemSpacingX    = 10;
    int itemSpacingY    = 10;
    int rounding        = 8;
    int touchHeight     = 38;
    int animFps         = 30;
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
    ModalScope Modal(const std::string& id, int customW = 340, int customH = 160) {
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
        Shutdown();
    }

    void ApplyTheme(Theme theme) {
        style = Style{};
        if (theme == Theme::NeonRed) {
            style.windowBg     = Color(12, 12, 16, 255);
            style.panelBg      = Color(24, 16, 22, 125);
            style.childBg      = Color(20, 14, 18, 115);
            style.popupBg      = Color(24, 20, 28, 235);
            style.border       = Color(230, 25, 65, 120);
            style.borderFocus  = Color(255, 50, 90, 255);
            style.frameBg      = Color(36, 18, 24, 140);
            style.frameHover   = Color(62, 20, 34, 180);
            style.checkMark    = Color(230, 35, 75, 255);
            style.sliderGrab   = Color(255, 35, 80, 255);
            style.button       = Color(185, 22, 55, 220);
            style.buttonHover  = Color(215, 25, 65, 245);
            style.buttonActive = Color(255, 40, 85, 255);
            style.toggleActive = Color(225, 25, 65, 240);
            style.toggleHover  = Color(255, 45, 88, 255);
        } else if (theme == Theme::CyberPurple) {
            style.windowBg     = Color(13, 15, 20, 255);
            style.panelBg      = Color(24, 27, 36, 125);
            style.childBg      = Color(20, 23, 30, 115);
            style.popupBg      = Color(28, 32, 42, 235);
            style.border       = Color(140, 110, 190, 85);
            style.borderFocus  = Color(186, 104, 200, 255);
            style.frameBg      = Color(32, 36, 46, 140);
            style.frameHover   = Color(46, 52, 66, 180);
            style.checkMark    = Color(206, 147, 216, 255);
            style.sliderGrab   = Color(186, 104, 200, 255);
            style.button       = Color(106, 27, 154, 220);
            style.buttonHover  = Color(142, 36, 170, 245);
            style.buttonActive = Color(74, 20, 140, 255);
            style.toggleActive = Color(123, 31, 162, 240);
            style.toggleHover  = Color(156, 39, 176, 255);
        } else if (theme == Theme::LightClean) {
            style.windowBg      = Color(235, 238, 245, 255);
            style.panelBg       = Color(255, 255, 255, 165);
            style.childBg       = Color(248, 250, 252, 150);
            style.popupBg       = Color(255, 255, 255, 245);
            style.tooltipBg     = Color(30, 34, 42, 240);
            style.border        = Color(255, 255, 255, 140);
            style.borderFocus   = Color(45, 115, 215, 255);
            style.text          = Color(28, 32, 40, 255);
            style.textDim       = Color(110, 118, 132, 255);
            style.frameBg       = Color(232, 236, 242, 170);
            style.frameHover    = Color(220, 226, 236, 200);
            style.checkMark     = Color(45, 115, 215, 255);
            style.sliderGrab    = Color(45, 115, 215, 255);
            style.button        = Color(215, 222, 235, 220);
            style.buttonHover   = Color(200, 210, 226, 240);
            style.buttonActive  = Color(185, 198, 220, 255);
            style.toggleActive  = Color(45, 115, 215, 240);
            style.toggleHover   = Color(60, 130, 230, 255);
            style.disabledBg    = Color(235, 237, 240, 160);
            style.disabledText  = Color(160, 166, 176, 255);
            style.scrollbarBg   = Color(232, 236, 242, 160);
            style.scrollbarGrab = Color(180, 188, 202, 220);
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
        // Limita pedidos contínuos (arrastar/scroll) a ~30 FPS (33ms), mas aceita cliques imediatos (force = true)
        if (force || (now - lastRedrawRequestMs >= 33)) {
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
        DrawGlassBackdrop(width, height);
        BeginFrameState();
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

    bool HasActiveAnimations() const { return hasActiveAnimations; }

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
        QueueOrDraw([=, this]() { DrawLine(l, y, r, y, c); });
        cursorY += S(10);
        return *this;
    }

    Context& SeparatorText(const std::string& labelUtf8, Color labelCol = Color(0, 0, 0, 0)) {
        int h = S(22);
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
            DrawTextInternal(textX, y, drawW, h, labelUtf8, col, true, ALIGN_LEFT);
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
        panelActive = true;
        panelStartY = cursorY;
        panelLeft = cursorX;
        panelRight = (customWidth > 0) ? (cursorX + S(customWidth)) : regionRight;
        panelTitle = titleUtf8;
        panelTitleColor = titleColor;

        int pad = S(12);
        regionLeft = panelLeft + pad;
        regionRight = panelRight - pad;
        cursorX = regionLeft;
        cursorY += pad;
        if (!panelTitle.empty()) cursorY += S(26);
    }

    void EndPanel() {
        if (!panelActive) return;
        int pad = S(12);
        int panelBottom = cursorY + pad - S(style.itemSpacingY);
        int w = panelRight - panelLeft;
        int h = panelBottom - panelStartY;
        float r = (float)S(style.rounding);

        // Fundo translúcido do painel
        DrawRoundRect(panelLeft, panelStartY, w, h, S(style.rounding), style.panelBg, Color(0, 0, 0, 0));

        // Borda com degradê de luz simulando espessura de vidro
        if (vg && w > 0 && h > 0) {
            NVGpaint glassBorder = nvgLinearGradient(
                vg, (float)panelLeft, (float)panelStartY, (float)panelLeft, (float)panelBottom,
                nvgRGBA(255, 255, 255, 75),
                nvgRGBA(255, 255, 255, 18)
            );
            nvgBeginPath(vg);
            nvgRoundedRect(vg, panelLeft + 0.5f, panelStartY + 0.5f, w - 1.0f, h - 1.0f, r);
            nvgStrokePaint(vg, glassBorder);
            nvgStrokeWidth(vg, std::max(1.0f, 1.0f * dpiScale * 0.5f));
            nvgStroke(vg);
        }

        if (!panelTitle.empty()) {
            int titleY = panelStartY + S(10);
            DrawTextInternal(panelLeft + pad, titleY, w - pad * 2, S(22),
                             panelTitle, panelTitleColor, true, ALIGN_LEFT);
            int sepY = titleY + S(24);
            DrawLine(panelLeft + pad, sepY, panelRight - pad, sepY, style.border);
        }

        regionLeft = panelLeft;
        regionRight = panelRight;
        cursorX = regionLeft;
        cursorY = panelBottom + S(style.itemSpacingY);
        panelActive = false;

        FlushDeferredDraws();
    }

    void BeginChild(const std::string& idStr, int customW = -1, int customH = 220, bool border = true) {
        int id = HashString(idStr);
        int w = (customW < 0) ? GetAvailWidth() : S(customW);
        int bottomSafe = std::max(S(style.windowPadding), safeAreaBottom);
        int h = (customH <= 0) ? std::max(S(80), clientH - cursorY - bottomSafe) : S(customH);
        int x = cursorX, y = cursorY;

        bool touchedInside = !modalBlocking && (openComboId == 0) && IsTouchInRect(x, y, w, h);
        if (touchedInside && touchDown && activeDragId == 0) {
            activeScrollChildId = id;
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
        cs.startContentY = y + S(8) - (int)std::round(scrollY);
        childStack.push_back(cs);

        DrawRoundRect(x, y, w, h, S(style.rounding), style.childBg, border ? style.border : style.childBg);

        int pad = S(10);
        int scrollBarSpace = S(8);
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

        int contentH = (cursorY - cs.startContentY) + S(8);
        int maxScroll = std::max(0, contentH - cs.h);
        float& scrollY = childScrollMap[cs.id];
        scrollY = std::clamp(scrollY, 0.0f, (float)maxScroll);

        if (maxScroll > 0 && cs.h > S(24)) {
            int barW = S(4);
            int barX = cs.x + cs.w - barW - S(3);
            int barY = cs.y + S(6);
            int barH = cs.h - S(12);
            DrawRoundRect(barX, barY, barW, barH, barW / 2, style.scrollbarBg, style.scrollbarBg);

            float visibleRatio = std::clamp(float(cs.h) / float(contentH), 0.12f, 1.0f);
            int grabH = std::max(S(22), (int)std::round(barH * visibleRatio));
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
        int offsetY = alignTextToFrame ? S(8) : 0;
        alignTextToFrame = false;
        if (IsDisabled()) color = style.disabledText;
        int h = S(22);
        int w = CalcTextWidth(textUtf8, bold);
        int drawW = std::min(w + S(4), GetAvailWidth());
        int x = cursorX, y = cursorY + offsetY;

        QueueOrDraw([=, this]() {
            DrawTextInternal(x, y, drawW, h, textUtf8, color, bold, ALIGN_LEFT);
        });
        AdvanceCursor(drawW, h);
        return *this;
    }

    Context& Badge(const std::string& textUtf8, Color accentColor = Color(89, 173, 250)) {
        int padX = S(10);
        int h = S(24);
        int textW = CalcTextWidth(textUtf8, true);
        int w = std::min(textW + padX * 2, GetAvailWidth());
        int x = cursorX, y = cursorY;

        Color bgCol = Color::Lerp(style.panelBg, accentColor, 0.20f);
        Color borderCol = Color::Lerp(style.panelBg, accentColor, 0.68f);

        QueueOrDraw([=, this]() {
            DrawRoundRect(x, y, w, h, h / 2, bgCol, borderCol);
            DrawTextInternal(x + padX, y, std::max(0, w - padX * 2), h, textUtf8, accentColor, true, ALIGN_CENTER);
        });
        AdvanceCursor(w, h);
        return *this;
    }

    Context& KeyValue(const std::string& keyUtf8, const std::string& valUtf8, Color valColor = Color(0, 0, 0, 0)) {
        int h = S(22);
        int x = cursorX, y = cursorY;
        int availW = GetAvailWidth();
        int keyW = std::min(CalcTextWidth(keyUtf8 + ": ", false), availW);
        int valW = std::max(0, availW - keyW);
        Color kCol = IsDisabled() ? style.disabledText : style.textDim;
        Color vCol = IsDisabled() ? style.disabledText : ((valColor.a > 0) ? valColor : style.text);

        QueueOrDraw([=, this]() {
            DrawTextInternal(x, y, keyW, h, keyUtf8 + ": ", kCol, false, ALIGN_LEFT);
            if (valW > 0) {
                DrawTextInternal(x + keyW, y, valW, h, valUtf8, vCol, true, ALIGN_LEFT);
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
        int id = ++widgetCounter;
        int boxSz = S(22);
        int h = S(30);
        int textW = CalcTextWidth(labelUtf8, false);
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
            DrawTextInternal(x + boxSz + S(10), y, std::max(0, totalW - boxSz - S(10)), h, labelUtf8, txtCol, false, ALIGN_LEFT);
        });

        AdvanceCursor(totalW, h);
        return clicked;
    }

    bool ToggleSwitch(const std::string& labelUtf8, bool& value) {
        int id = ++widgetCounter;
        int swW = S(44);
        int swH = S(24);
        int h = S(30);
        int textW = labelUtf8.empty() ? 0 : (CalcTextWidth(labelUtf8, false) + S(10));
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
                DrawTextInternal(x + swW + S(10), y, totalW - swW - S(10), h, labelUtf8, txtCol, false, ALIGN_LEFT);
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
        Color txtCol  = disabled ? style.disabledText : style.text;

        QueueOrDraw([=, this]() {
            DrawRoundRect(x, y, w, h, S(style.rounding), bgCol, disabled ? style.border : bgCol);
            DrawTextInternal(x, y, w, h, labelUtf8, txtCol, true, ALIGN_CENTER);
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
            int id = ++widgetCounter;
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
            Color txtCol = disabled ? style.disabledText : (isSelected ? style.text : Color::Lerp(style.textDim, style.text, pressT));

            QueueOrDraw([=, this]() {
                DrawRoundRect(tx, y, curW, h, S(style.rounding), bgCol, isSelected ? style.borderFocus : style.border);
                DrawTextInternal(tx, y, curW, h, labels.at(i), txtCol, isSelected, ALIGN_CENTER);
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
            int id = ++widgetCounter;
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
            Color txtCol   = disabled ? style.disabledText : (isSelected ? style.text : Color::Lerp(style.textDim, style.text, pressT));

            QueueOrDraw([=, this]() {
                if (activeT > 0.02f || pressT > 0.02f) {
                    int pad = S(3);
                    DrawRoundRect(sx + pad, y + pad, std::max(0, curW - pad * 2), std::max(0, h - pad * 2),
                                  std::max(2, S(style.rounding) - 2), segCol, segCol);
                }
                DrawTextInternal(sx, y, curW, h, options.at(i), txtCol, isSelected, ALIGN_CENTER);
            });
        }

        AdvanceCursor(totalW, h);
        return changed;
    }

    bool SelectableRow(const std::string& titleUtf8, const std::string& subtitleUtf8 = "", bool selected = false, int customHeight = 46) {
        int id = ++widgetCounter;
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
            int subW = subtitleUtf8.empty() ? 0 : (CalcTextWidth(subtitleUtf8, false) + S(12));
            int titleW = std::max(0, w - padX * 2 - subW);
            DrawTextInternal(x + padX, y, titleW, h, titleUtf8, disabled ? style.disabledText : style.text, true, ALIGN_LEFT);
            if (subW > 0) {
                DrawTextInternal(x + w - padX - subW, y, subW, h, subtitleUtf8, style.textDim, false, ALIGN_RIGHT);
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
                              ? items.at(currentIndex) : "Selecionar...";

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
                    DrawTextInternal(x + S(12), optY, std::max(0, w - S(24)), itemH, items.at(i), style.text, i == currentIndex, ALIGN_LEFT);
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

    static int HashString(const std::string& str) {
        uint32_t h = 2166136261u;
        for (unsigned char c : str) {
            h ^= c;
            h *= 16777619u;
        }
        int res = static_cast<int>(h & 0x7FFFFFFF);
        return (res == 0) ? 1 : res;
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
        inputRenderedThisFrame = false;

        uint64_t now = NowMs();
        deltaTime = (lastFrameTimeMs == 0) ? 0.016f : std::min(0.06f, (now - lastFrameTimeMs) / 1000.0f);
        lastFrameTimeMs = now;
        widgetCounter = 0;
        hasActiveAnimations = false;

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

        // Limita a animação do fundo a ~10 FPS (100ms) para economia máxima de bateria
        const uint64_t backdropIntervalMs = 100;
        uint64_t now = NowMs();
        uint64_t quantizedTime = (now / backdropIntervalMs) * backdropIntervalMs;
        float t = quantizedTime * 0.0005f;

        float offsetX = std::sin(t) * S(24);
        float offsetY = std::cos(t * 0.8f) * S(16);

        if (now - lastBackdropTickMs >= backdropIntervalMs) {
            lastBackdropTickMs = now;
            hasActiveAnimations = true;
        }

        // 1. Base escura profunda
        NVGpaint bgGrad = nvgLinearGradient(
            vg, 0.0f, 0.0f, (float)w, (float)h,
            nvgRGBA(14, 14, 16, 255),
            nvgRGBA(8, 8, 10, 255)
        );
        nvgBeginPath(vg);
        nvgRect(vg, 0.0f, 0.0f, (float)w, (float)h);
        nvgFillPaint(vg, bgGrad);
        nvgFill(vg);

        float minDim = (float)std::min(w, h);
        float maxDim = (float)std::max(w, h);

        // 2. Orbe desfocado superior direito
        NVGpaint orb1 = nvgRadialGradient(
            vg, w * 0.82f + offsetX, h * 0.22f + offsetY, minDim * 0.04f, minDim * 0.45f,
            nvgRGBA(225, 230, 235, 95),
            nvgRGBA(225, 230, 235, 0)
        );
        nvgBeginPath(vg);
        nvgRect(vg, 0.0f, 0.0f, (float)w, (float)h);
        nvgFillPaint(vg, orb1);
        nvgFill(vg);

        // 3. Anel desfocado inferior direito
        NVGpaint orb2Outer = nvgRadialGradient(
            vg, w * 0.68f - offsetX, h * 0.72f - offsetY, minDim * 0.08f, minDim * 0.42f,
            nvgRGBA(200, 195, 185, 85),
            nvgRGBA(200, 195, 185, 0)
        );
        nvgBeginPath(vg);
        nvgRect(vg, 0.0f, 0.0f, (float)w, (float)h);
        nvgFillPaint(vg, orb2Outer);
        nvgFill(vg);

        NVGpaint orb2Hole = nvgRadialGradient(
            vg, w * 0.68f - offsetX, h * 0.72f - offsetY, minDim * 0.01f, minDim * 0.16f,
            nvgRGBA(12, 12, 14, 150),
            nvgRGBA(12, 12, 14, 0)
        );
        nvgBeginPath(vg);
        nvgRect(vg, 0.0f, 0.0f, (float)w, (float)h);
        nvgFillPaint(vg, orb2Hole);
        nvgFill(vg);

        // 4. Ponto de luz secundário à esquerda
        NVGpaint orb3 = nvgRadialGradient(
            vg, w * 0.12f + (offsetY * 0.6f), h * 0.55f + (offsetX * 0.6f), minDim * 0.02f, maxDim * 0.35f,
            nvgRGBA(180, 185, 195, 55),
            nvgRGBA(180, 185, 195, 0)
        );
        nvgBeginPath(vg);
        nvgRect(vg, 0.0f, 0.0f, (float)w, (float)h);
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

    void PushClip(int x, int y, int w, int h) {
        RectI r = { x, y, x + w, y + h };
        if (!clipStack.empty()) {
            const RectI& prev = clipStack.back();
            r.left   = std::max(r.left, prev.left);
            r.top    = std::max(r.top, prev.top);
            r.right  = std::max(r.left, std::min(r.right, prev.right));
            r.bottom = std::max(r.top, std::min(r.bottom, prev.bottom));
        }
        clipStack.push_back(r);
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
