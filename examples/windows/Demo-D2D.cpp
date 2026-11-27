#define SHAWARMAUI_USE_D2D
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
        "Neon Red (Default)",
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
        { "Optimize gradients with std::array", "C++17", true },
        { "Build persistent GDI pixel cache", "Win32", true },
        { "Port PushID and panelStack to Android", "GLES3", true },
        { "Test release build on GitHub Actions", "CI/CD", false }
    };
    int selectedTask = 0;
    std::string newTaskName = "";
} g_demo;

ShawarmaUI::Context g_ui;

void OnFileDropDemo(const std::string& filePath) {
    g_demo.droppedFile = filePath;
    g_ui.Toast("File received via Drag & Drop!", g_ui.style.success);
}

void RenderShowcase(ShawarmaUI::Context& ctx) {
    g_demo.themeIndex = static_cast<int>(ctx.currentTheme);

    // Header: Theme Selector + Badge
    ctx.AlignText().Text("Theme:", true);
    ctx.SameLine();
    if (ctx.Combo("theme_selector", g_demo.themeIndex, g_demo.themeNames, 200)) {
        ctx.ApplyTheme(static_cast<ShawarmaUI::Theme>(g_demo.themeIndex));
        ctx.Toast("Theme switched: " + g_demo.themeNames.at(g_demo.themeIndex));
    }
    ctx.SameLineRight(185);
    ctx.Badge("C++17 • Zero C-Arrays", ShawarmaUI::Color(89, 173, 250));

    ctx.Spacing();
    ctx.TabBar(g_demo.currentTab, { "Controls & Inputs", "Lists & Containers", "About & Modal" });
    ctx.Spacing();

    // =================================================================
    // TAB 1: CONTROLS, SLIDERS, INPUTS AND TOOLTIPS
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
    // TAB 2: NESTED PANELS, CHILD SCROLL AND DYNAMIC LIST (PushID)
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

                for (int i = 0; i < (int)g_demo.tasks.size(); ++i) {
                    auto scopeId = ctx.ID(i);
                    TaskItem& item = g_demo.tasks.at(i);

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
                    if (g_demo.selectedTask >= (int)g_demo.tasks.size()) {
                        g_demo.selectedTask = std::max(0, (int)g_demo.tasks.size() - 1);
                    }
                    ctx.Toast("Item safely removed.");
                }
            }
        }
    }

    // =================================================================
    // TAB 3: LIBRARY INFO & BLOCKING MODAL POPUP
    // =================================================================
    else if (g_demo.currentTab == 2) {
        if (auto p = ctx.Panel("Library Information")) {
            ctx.KeyValue("Library", "ShawarmaUI (Single-Header)");
            ctx.KeyValue("C++ Standard", "Modern C++17 (Zero C-Arrays)");
            ctx.KeyValue("Active Backend", "Direct2D + DirectWrite (GPU)", ctx.style.success);
            ctx.Spacing();

            if (ctx.Button("Open Modal Window", 200, 32)) {
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
    g_ui.ApplyTheme(ShawarmaUI::Theme::NeonRed);
    g_ui.EnableThemePersistence(L"ShawarmaUIShowcaseD2D");

    if (!g_ui.Create("ShawarmaUI - Direct2D Showcase (C++17)", 640, 560)) {
        return 1;
    }

    g_ui.onFileDrop = OnFileDropDemo;
    g_ui.Run(RenderShowcase);

    return 0;
}
