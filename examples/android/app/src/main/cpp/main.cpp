#include <jni.h>
#include <GLES3/gl3.h>
#include "nanovg.h"
#define NANOVG_GLES3_IMPLEMENTATION
#include "nanovg_gl.h"
#include "ShawarmaUI_Android.h"
#include "ShawarmaIcons.h"

#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

struct TaskItem {
    std::string title;
    std::string tag;
    bool done;
};

static NVGcontext* g_vg = nullptr;
static ShawarmaUI::Context g_ui;
static JavaVM* g_jvm = nullptr;
static jobject g_activityObj = nullptr;
static jmethodID g_midRequestRender = nullptr;
static jmethodID g_midShowKeyboard = nullptr;

static int g_width = 0;
static int g_height = 0;

// Showcase State
static int g_currentTab = 0;
static int g_themeIndex = 0;
static const std::vector<std::string> g_themeNames = {
    "Teal Gold (Default)",
    "Neon Red",
    "Cyber Purple",
    "Rose Gold",
    "Light Clean"
};

// Tab 0: Controls & Inputs
static std::string g_username = "Thunderstruck";
static std::string g_password = "123456";
static bool g_gpuAccel = true;
static bool g_autoSave = false;
static int g_targetFps = 60;
static float g_opacity = 0.85f;
static int g_qualityMode = 1;
static bool g_showSpinner = false;

// Tab 1: Dynamic Lists & Multiline Editor
static std::vector<TaskItem> g_tasks = {
    { "Frosted glass & 15 FPS cached backdrop", "GLES3", true },
    { "Load embedded TTF vector icon font", "Icons", true },
    { "Cached word-wrap & multiline text box", "Android", true },
    { "Publish automated APK workflow", "CI/CD", false }
};
static int g_selectedTask = 0;
static std::string g_newTaskName = "";
static std::string g_noteText =
    "ShawarmaUI Android!\n"
    "Toque aqui para editar textos longos com quebra automatica de linha (WrapTextCached) e barra de rolagem.\n\n"
    "Caminhos longos tambem quebram naturalmente: /storage/emulated/0/Download/ShawarmaUI/include/ShawarmaUI_Android.h";

static void CallVoidMethodJNI(jmethodID mid) {
    if (!g_jvm || !g_activityObj || !mid) return;
    JNIEnv* env = nullptr;
    bool attached = false;
    if (g_jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        if (g_jvm->AttachCurrentThread(&env, nullptr) != JNI_OK) return;
        attached = true;
    }
    env->CallVoidMethod(g_activityObj, mid);
    if (attached) g_jvm->DetachCurrentThread();
}

static void CallBoolMethodJNI(jmethodID mid, bool val) {
    if (!g_jvm || !g_activityObj || !mid) return;
    JNIEnv* env = nullptr;
    bool attached = false;
    if (g_jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        if (g_jvm->AttachCurrentThread(&env, nullptr) != JNI_OK) return;
        attached = true;
    }
    env->CallVoidMethod(g_activityObj, mid, static_cast<jboolean>(val));
    if (attached) g_jvm->DetachCurrentThread();
}

static void OnRequestRedrawJNI() {
    CallVoidMethodJNI(g_midRequestRender);
}

static void OnShowKeyboardJNI(bool show) {
    CallBoolMethodJNI(g_midShowKeyboard, show);
}

static void RenderDemoUI() {
    g_themeIndex = static_cast<int>(g_ui.currentTheme);

    // Header: Theme Combo + Responsive Badge
    g_ui.AlignText().Text("Theme:", true);
    g_ui.SameLine();
    if (g_ui.Combo("theme_selector", g_themeIndex, g_themeNames, 175)) {
        g_ui.ApplyTheme(static_cast<ShawarmaUI::Theme>(g_themeIndex));
        g_ui.Toast("Theme: " + g_themeNames.at(static_cast<size_t>(g_themeIndex)));
    }
    g_ui.SameLineWrap(125);
    g_ui.Badge("C++17 • GLES3", g_ui.style.accent);

    g_ui.Spacing(4);
    g_ui.TabBar(g_currentTab, { "Controls", "Lists & Notes", "About & Modal" });
    g_ui.Spacing(6);

    // =================================================================
    // TAB 0: TOUCH CONTROLS, SLIDERS, INPUTS & VECTOR ICONS
    // =================================================================
    if (g_currentTab == 0) {
        if (auto p = g_ui.Panel("General & Video Settings")) {
            g_ui.Text("User Profile", true);
            g_ui.InputText("inp_user", g_username, "Enter username...");
            g_ui.Tooltip("Hold finger for 450ms to view tooltips!");

            g_ui.InputText("inp_pass", g_password, "Password...", -1, true);

            g_ui.Spacing(4);
            g_ui.ToggleSwitch("AMediaCodec Hardware Accel (GPU)", g_gpuAccel);
            g_ui.Checkbox("Auto-save session state", g_autoSave);

            g_ui.Spacing(4);
            g_ui.SegmentedControl(g_qualityMode, { "Low", "Medium", "High" });

            g_ui.SeparatorText("Fine Tuning");
            {
                auto dis = g_ui.Disabled(!g_gpuAccel);
                g_ui.SliderInt(g_targetFps, 24, 120, -1, 28, " FPS");

                int pct = static_cast<int>(std::round(g_opacity * 100.0f));
                std::string opLabel = "Glass Opacity: " + std::to_string(pct) + "%";
                g_ui.SliderFloat(g_opacity, 0.10f, 1.0f, -1, 28, opLabel);
            }

            g_ui.ProgressBar(g_opacity, -1, 18, ShawarmaUI::Color(0, 0, 0, 0), "Synchronized Progress");
        }

        if (auto p2 = g_ui.Panel("Vector Icons (ShawarmaIcons.h) & Feedback")) {
            std::string playLabel = std::string(ShawarmaIcons::Play) + "  Play";
            std::string saveLabel = std::string(ShawarmaIcons::Save) + "  Save";
            int act = g_ui.ButtonRow({ playLabel, saveLabel, "Spinner" }, 38);
            if (act == 0) {
                g_ui.Toast("Playback started!", g_ui.style.success);
            } else if (act == 1) {
                g_ui.Toast("Settings saved!", g_ui.style.warning);
            } else if (act == 2) {
                g_ui.showSpinner = !g_ui.showSpinner;
            }
            if (g_ui.showSpinner) {
                g_ui.Spacing(4);
                g_ui.Spinner(28);
            }
        }
    }

    // =================================================================
    // TAB 1: DYNAMIC LISTS (PushID) & MULTILINE EDITOR
    // =================================================================
    else if (g_currentTab == 1) {
        if (auto p = g_ui.Panel("Dynamic Task List")) {
            g_ui.InputText("inp_new_task", g_newTaskName, "New task description...");
            std::string addLabel = std::string(ShawarmaIcons::PlusSquare) + "  Add Task";
            if (g_ui.Button(addLabel, -1, 38) && !g_newTaskName.empty()) {
                g_tasks.push_back({ g_newTaskName, "New", false });
                g_newTaskName.clear();
                g_ui.Toast("Task added!", g_ui.style.success);
            }

            g_ui.SeparatorText("Scrollable Child Container");

            if (auto child = g_ui.Child("task_list", -1, 220, true)) {
                int indexToDelete = -1;

                for (int i = 0; i < static_cast<int>(g_tasks.size()); ++i) {
                    auto scopeId = g_ui.ID(i);
                    TaskItem& item = g_tasks.at(static_cast<size_t>(i));

                    if (g_ui.SelectableRow(item.title, item.done ? "[Done]" : "[" + item.tag + "]", g_selectedTask == i, 44)) {
                        g_selectedTask = i;
                    }

                    g_ui.Checkbox("Completed##chk", item.done);
                    g_ui.SameLineRight(105);

                    std::string delLabel = std::string(ShawarmaIcons::Trash2) + " Remove##del";
                    if (g_ui.Button(delLabel, 105, 30,
                                    ShawarmaUI::Color(175, 35, 55, 230),
                                    ShawarmaUI::Color(215, 45, 70, 250))) {
                        indexToDelete = i;
                    }
                    g_ui.Separator();
                }

                if (indexToDelete >= 0) {
                    g_tasks.erase(g_tasks.begin() + indexToDelete);
                    if (g_selectedTask >= static_cast<int>(g_tasks.size())) {
                        g_selectedTask = std::max(0, static_cast<int>(g_tasks.size()) - 1);
                    }
                    g_ui.Toast("Task removed.");
                }
            }
        }

        if (auto pNotes = g_ui.Panel("Multiline Text Editor (Auto Word-Wrap)")) {
            g_ui.InputTextMultiline("editor_notes", g_noteText, -1, 140);
            g_ui.Spacing(4);
            std::string clearLabel = std::string(ShawarmaIcons::XSquare) + "  Clear Notes";
            if (g_ui.Button(clearLabel, -1, 36, g_ui.style.secondaryBtn, g_ui.style.secondaryBtnHover)) {
                g_noteText.clear();
                g_ui.Toast("Notes cleared!");
            }
        }
    }

    // =================================================================
    // TAB 2: LIBRARY INFO & BLOCKING MODAL POPUP
    // =================================================================
    else if (g_currentTab == 2) {
        if (auto p = g_ui.Panel("ShawarmaUI Android")) {
            g_ui.KeyValue("Backend", "OpenGL ES 3.0 + NanoVG");
            g_ui.KeyValue("Render Mode", "RENDERMODE_WHEN_DIRTY (0% Idle CPU)");
            g_ui.KeyValue("C++ Standard", "Modern C++17 (Zero C-Arrays)");

            g_ui.Spacing(4);
            g_ui.TextWrapped("Biblioteca C++17 header-only com quebra automatica em cache (WrapTextCached), "
                             "controles responsivos ao toque e renderizacao OpenGL ES 3.0 + NanoVG.",
                             g_ui.style.textDim);

            g_ui.Spacing(6);
            g_ui.Badge("Header-Only • Touch Ready", g_ui.style.success);

            g_ui.Spacing(8);
            if (g_ui.Button("Open Modal Dialog", -1, 40)) {
                g_ui.OpenPopup("confirm_modal");
            }
        }
    }

    // =================================================================
    // MODAL POPUP (Rendered on top of the entire UI)
    // =================================================================
    if (auto m = g_ui.Modal("confirm_modal", 330, 175)) {
        g_ui.Text("Confirm Action", true);
        g_ui.Spacing(4);
        g_ui.TextWrapped("This modal blocks touch events behind it.", g_ui.style.textDim);
        g_ui.Spacing(8);

        int mAct = g_ui.ButtonRow({ "Confirm", "Cancel" }, 38);
        if (mAct == 0) {
            g_ui.Toast("Confirmed!", g_ui.style.success);
            g_ui.CloseCurrentPopup();
        } else if (mAct == 1) {
            g_ui.CloseCurrentPopup();
        }
    }
}

extern "C" {

JNIEXPORT void JNICALL
Java_com_shawarmaui_demo_MainActivity_nativeInit(JNIEnv* env, jobject thiz, jfloat density) {
    env->GetJavaVM(&g_jvm);
    if (g_activityObj) env->DeleteGlobalRef(g_activityObj);
    g_activityObj = env->NewGlobalRef(thiz);

    jclass cls = env->GetObjectClass(thiz);
    g_midRequestRender = env->GetMethodID(cls, "requestRenderFromNative", "()V");
    g_midShowKeyboard  = env->GetMethodID(cls, "setKeyboardVisible", "(Z)V");

    if (g_vg) {
        nvgDeleteGLES3(g_vg);
        g_vg = nullptr;
    }
    g_vg = nvgCreateGLES3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
    g_ui.InitGLES3(g_vg, density);
    g_ui.LoadIconFontFromMemory(ShawarmaUI::kIconFontTTF.data(), static_cast<int>(ShawarmaUI::kIconFontTTF.size()));
    g_ui.ApplyTheme(ShawarmaUI::Theme::TealGold);

    g_ui.onRequestRedraw = OnRequestRedrawJNI;
    g_ui.onShowKeyboard  = OnShowKeyboardJNI;
}

JNIEXPORT void JNICALL
Java_com_shawarmaui_demo_MainActivity_nativeResize(JNIEnv*, jobject, jint w, jint h, jint topInset, jint bottomInset) {
    g_width = w;
    g_height = h;
    g_ui.SetSafeAreaInsets(topInset, bottomInset);
}

JNIEXPORT jint JNICALL
Java_com_shawarmaui_demo_MainActivity_nativeRender(JNIEnv*, jobject) {
    if (g_width <= 0 || g_height <= 0) return 0;
    g_ui.BeginFrame(g_width, g_height);
    RenderDemoUI();
    g_ui.EndFrame();
    return static_cast<jint>(g_ui.GetNextFrameDelayMs());
}

JNIEXPORT void JNICALL
Java_com_shawarmaui_demo_MainActivity_nativeTouch(JNIEnv*, jobject, jint action, jint x, jint y) {
    if (action == 0) g_ui.OnTouchDown(x, y);
    else if (action == 1) g_ui.OnTouchUp(x, y);
    else if (action == 2) g_ui.OnTouchMove(x, y);
}

JNIEXPORT void JNICALL
Java_com_shawarmaui_demo_MainActivity_nativeTextCommit(JNIEnv* env, jobject, jstring text) {
    const char* utf = env->GetStringUTFChars(text, nullptr);
    if (utf) {
        g_ui.OnTextCommit(utf);
        env->ReleaseStringUTFChars(text, utf);
    }
}

JNIEXPORT void JNICALL
Java_com_shawarmaui_demo_MainActivity_nativeKeySpecial(JNIEnv*, jobject, jint code) {
    if (code == 1) g_ui.OnKeyBackspace();
    else if (code == 2) g_ui.OnKeyEnter();
}

} // extern "C"
