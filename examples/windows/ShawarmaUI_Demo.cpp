#include "ShawarmaUI.h"
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

struct TaskItem {
    std::string title;
    std::string tag;
    bool done;
};

struct ShowcaseState {
    int currentTab = 0;
    int themeIndex = 0;
    const std::vector<std::string> themeNames = {
        "Teal Gold (Default)",
        "Neon Red",
        "Cyber Purple",
        "Rose Gold",
        "Light Clean"
    };

    // Tab 0: Controls & Inputs
    std::string username = "Thunderstruck";
    std::string password = "123456";
    std::string droppedFile = "Drag and drop any file onto this window...";
    bool enableTurbo = true;
    bool autoSave = false;
    int targetFps = 30;
    float opacity = 0.85f;
    int qualityMode = 1;
    bool showSpinner = false;

    // Tab 1: Dynamic Lists & Containers
    std::vector<TaskItem> tasks = {
        { "Hybrid Direct2D + GDI runtime fallback", "C++17", true },
        { "Cached word-wrap & multiline text box", "Win32", true },
        { "Dynamic 30 FPS UI / 15 FPS backdrop", "Perf", true },
        { "Test release build on GitHub Actions", "CI/CD", false }
    };
    int selectedTask = 0;
    std::string newTaskName = "";

    // Tab 2: Multiline & Word-Wrap Showcase
    std::string multilineNote =
        "Digite ou cole textos longos aqui! Esta caixa possui quebra automatica de linha (WrapTextCached), "
        "rolagem com a roda do mouse e atalhos Ctrl+C / Ctrl+V.\n\n"
        "Caminhos longos tambem quebram naturalmente: C:\\Users\\Thunderstruck\\Documents\\Projetos\\ShawarmaUI\\include\\ShawarmaUI.h";
} g_demo;

ShawarmaUI::Context g_ui;

void OnFileDropDemo(const std::string& filePath) {
    g_demo.droppedFile = filePath;
    g_ui.Toast("File received via Drag & Drop!", g_ui.style.success);
}

void RenderShowcase(ShawarmaUI::Context& ctx) {
    g_demo.themeIndex = static_cast<int>(ctx.currentTheme);
    bool isD2D = ctx.IsUsingD2D();

    // Header: Theme Selector + Backend Badge
    ctx.AlignText().Text("Theme:", true);
    ctx.SameLine();
    if (ctx.Combo("theme_selector", g_demo.themeIndex, g_demo.themeNames, 195)) {
        ctx.ApplyTheme(static_cast<ShawarmaUI::Theme>(g_demo.themeIndex));
        ctx.Toast("Theme switched: " + g_demo.themeNames.at(static_cast<size_t>(g_demo.themeIndex)));
    }
    ctx.SameLineWrap(180);
    ctx.Badge(isD2D ? "C++17 • Direct2D Active" : "C++17 • GDI Fallback",
              isD2D ? ctx.style.success : ctx.style.warning);

    ctx.Spacing();
    ctx.TabBar(g_demo.currentTab, { "Controls", "Lists", "Wrap & D2D" });
    ctx.Spacing();

    // =================================================================
    // TAB 0: CONTROLS, SLIDERS, INPUTS AND TOOLTIPS
    // =================================================================
    if (g_demo.currentTab == 0) {
        if (auto p = ctx.Panel("General Settings")) {
            ctx.Columns(2);

            ctx.Text("User Profile", true);
            ctx.InputText("inp_user", g_demo.username, "Enter your name...");
            ctx.Tooltip("Text input with UTF-8, Ctrl+C, and Ctrl+V support");

            ctx.InputText("inp_pass", g_demo.password, "Password...", -1, true);
            ctx.Tooltip("Password mode (masks characters with *)");

            ctx.NextColumn();

            ctx.Text("Switches & States", true);
            ctx.ToggleSwitch("Turbo Mode (30 FPS)", g_demo.enableTurbo);
            ctx.Checkbox("Auto-save changes", g_demo.autoSave);

            ctx.Spacing(4);
            ctx.SegmentedControl(g_demo.qualityMode, { "Low", "Medium", "High" });

            ctx.Columns(1);
            ctx.SeparatorText("Fine Tuning");

            {
                auto dis = ctx.Disabled(!g_demo.enableTurbo);
                ctx.SliderInt(g_demo.targetFps, 15, 144, -1, 20, " FPS");

                int pct = static_cast<int>(std::round(g_demo.opacity * 100.0f));
                std::string opacityLabel = "Glass Opacity: " + std::to_string(pct) + "%";
                ctx.SliderFloat(g_demo.opacity, 0.10f, 1.0f, -1, 20, opacityLabel);
            }

            ctx.ProgressBar(g_demo.opacity, -1, 14, ShawarmaUI::Color(0, 0, 0, 0), "Synchronized Progress Bar");
        }

        if (auto p2 = ctx.Panel("Drag & Drop and Feedback")) {
            ctx.InputTextReadOnly(g_demo.droppedFile);
            ctx.Spacing(4);

            if (ctx.Button("Show Toast")) {
                ctx.Toast("Floating notification triggered!", ctx.style.warning);
            }
            ctx.SameLine();
            if (ctx.ToggleButton("Toggle Spinner", g_demo.showSpinner)) {
                g_demo.showSpinner = !g_demo.showSpinner;
            }
            if (g_demo.showSpinner) {
                ctx.SameLine();
                ctx.Spinner(24);
            }
        }
    }

    // =================================================================
    // TAB 1: NESTED PANELS, CHILD SCROLL AND DYNAMIC LIST (PushID)
    // =================================================================
    else if (g_demo.currentTab == 1) {
        if (auto p = ctx.Panel("Parent Panel (Nesting + Scroll)")) {
            if (auto sub = ctx.Panel("Add New Task (Nested Panel)", ctx.style.warning)) {
                ctx.InputText("inp_new_task", g_demo.newTaskName, "Task description...", 360);
                ctx.SameLine();
                if (ctx.Button("Add##add_task", -1, 28) && !g_demo.newTaskName.empty()) {
                    g_demo.tasks.push_back({ g_demo.newTaskName, "New", false });
                    g_demo.newTaskName.clear();
                    ctx.Toast("Task added!", ctx.style.success);
                }
            }

            ctx.SeparatorText("Dynamic List with ctx.ID() and ## Suffix");

            if (auto child = ctx.Child("task_list", -1, 230, true)) {
                int indexToDelete = -1;

                for (int i = 0; i < static_cast<int>(g_demo.tasks.size()); ++i) {
                    auto scopeId = ctx.ID(i);
                    TaskItem& item = g_demo.tasks.at(static_cast<size_t>(i));

                    bool clicked = ctx.SelectableRow(
                        item.title,
                        item.done ? "[Done]" : "[" + item.tag + "]",
                        g_demo.selectedTask == i,
                        34
                    );
                    if (clicked) {
                        g_demo.selectedTask = i;
                    }

                    ctx.Checkbox("Done##chk", item.done);
                    ctx.SameLineRight(95);
                    if (ctx.Button("Remove##del", 95, 24,
                                   ShawarmaUI::Color(175, 35, 55, 220),
                                   ShawarmaUI::Color(215, 45, 70, 250))) {
                        indexToDelete = i;
                    }
                    ctx.Separator();
                }

                if (indexToDelete >= 0) {
                    g_demo.tasks.erase(g_demo.tasks.begin() + indexToDelete);
                    if (g_demo.selectedTask >= static_cast<int>(g_demo.tasks.size())) {
                        g_demo.selectedTask = std::max(0, static_cast<int>(g_demo.tasks.size()) - 1);
                    }
                    ctx.Toast("Item safely removed.");
                }
            }
        }
    }

    // =================================================================
    // TAB 2: HYBRID BACKEND, MULTILINE WRAP & MODAL POPUP
    // =================================================================
    else if (g_demo.currentTab == 2) {
        if (auto p = ctx.Panel("Hybrid Graphics & Multiline Text Box")) {
            ctx.KeyValue("Active Backend",
                         isD2D ? "Direct2D + DirectWrite (Hardware / WARP)" : "Win32 GDI Fallback (WinPE Ready)",
                         isD2D ? ctx.style.success : ctx.style.warning);

            ctx.TextWrapped("Este paragrafo usa ctx.TextWrapped() com cache automatico (WrapTextCached), "
                            "ajustando-se perfeitamente a largura da janela sem vazar para a direita.",
                            ctx.style.textDim);

            ctx.Spacing(4);
            ctx.InputTextMultiline("demo_multiline", g_demo.multilineNote, -1, 130);
            ctx.Spacing(6);

            int act = ctx.ButtonRow({ isD2D ? "Test GDI Fallback" : "Switch to Direct2D", "Open Modal Window" }, 30);
            if (act == 0) {
                ctx.SwitchBackend(!isD2D);
                ctx.Toast(isD2D ? "Switched to GDI Fallback" : "Switched to Direct2D");
            } else if (act == 1) {
                ctx.OpenPopup("confirm_modal");
            }
        }
    }

    // =================================================================
    // MODAL POPUP (Rendered on top of the entire UI)
    // =================================================================
    if (auto m = ctx.Modal("confirm_modal", 400, 140)) {
        ctx.Text("Confirm Action", true);
        ctx.Spacing(4);
        ctx.TextColored(ctx.style.textDim, "This modal blocks clicks on widgets behind it.");
        ctx.Spacing();

        if (ctx.Button("Confirm", 120, 30)) {
            ctx.Toast("Confirmed!", ctx.style.success);
            ctx.CloseCurrentPopup();
        }
        ctx.SameLine();
        if (ctx.Button("Cancel", 120, 30, ctx.style.secondaryBtn, ctx.style.secondaryBtnHover)) {
            ctx.CloseCurrentPopup();
        }
    }
}

int main() {
    g_ui.ApplyTheme(ShawarmaUI::Theme::TealGold);
    g_ui.EnableThemePersistence(L"ShawarmaUIShowcaseHybrid");

    if (!g_ui.Create("ShawarmaUI - Hybrid Direct2D + GDI Showcase (C++17)", 640, 560)) {
        return 1;
    }

    g_ui.onFileDrop = OnFileDropDemo;
    g_ui.Run(RenderShowcase);

    return 0;
}
