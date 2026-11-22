#include <jni.h>
#include <GLES3/gl3.h>
#include "nanovg.h"
#define NANOVG_GLES3_IMPLEMENTATION
#include "nanovg_gl.h"
#include "ShawarmaUI_Android.h"

static NVGcontext* g_vg = nullptr;
static ShawarmaUI::Context g_ui;
static JavaVM* g_jvm = nullptr;
static jobject g_activityObj = nullptr;
static jmethodID g_midRequestRender = nullptr;
static jmethodID g_midShowKeyboard = nullptr;

static int g_width = 0;
static int g_height = 0;

// Estado da tela de teste
static int g_tab = 0;
static int g_themeIdx = 0;
static bool g_gpuAccel = true;
static int g_crf = 23;
static std::string g_noteText = "ShawarmaUI Android!\nToque para editar...";

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

static void RenderDemoUI() {
    g_ui.TabBar(g_tab, { "Controles", "Editor", "Sobre" });
    g_ui.Spacing(4);

    if (g_tab == 0) {
        if (auto p = g_ui.Panel("Configuracoes de Video")) {
            g_ui.ToggleSwitch("Aceleracao AMediaCodec (GPU)", g_gpuAccel);
            g_ui.SliderInt(g_crf, 14, 35, -1, 28, " CRF");

            g_ui.SeparatorText("Tema Visual");
            if (g_ui.SegmentedControl(g_themeIdx, { "Blue", "Neon", "Cyber", "Light" })) {
                ShawarmaUI::Theme themes[4] = {
                    ShawarmaUI::Theme::DarkBlue,
                    ShawarmaUI::Theme::NeonRed,
                    ShawarmaUI::Theme::CyberPurple,
                    ShawarmaUI::Theme::LightClean
                };
                g_ui.ApplyTheme(themes[g_themeIdx]);
            }

            g_ui.Spacing(6);
            if (g_ui.Button("Testar Notificacao Toast", -1, 40)) {
                g_ui.Toast("Funcionando no Android!", ShawarmaUI::Color(115, 224, 140));
            }
            g_ui.Tooltip("Segure o dedo para ver dicas!");
        }
    } else if (g_tab == 1) {
        if (auto p = g_ui.Panel("Bloco de Notas / Presets")) {
            g_ui.InputTextMultiline("editor_notas", g_noteText, -1, 180);
            g_ui.Spacing(4);
            if (g_ui.Button("Limpar Texto", -1, 36)) {
                g_noteText.clear();
                g_ui.Toast("Texto limpo!");
            }
        }
    } else {
        if (auto p = g_ui.Panel("ShawarmaUI Android")) {
            g_ui.KeyValue("Backend", "OpenGL ES 3.0 + NanoVG");
            g_ui.KeyValue("Modo", "RENDERMODE_WHEN_DIRTY (0% CPU parado)");
            g_ui.Badge("Header-Only • C++17", ShawarmaUI::Color(115, 224, 140));
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
    g_ui.ApplyTheme(ShawarmaUI::Theme::DarkBlue);

    g_ui.onRequestRedraw = []() { CallVoidMethodJNI(g_midRequestRender); };
    g_ui.onShowKeyboard  = [](bool show) { CallBoolMethodJNI(g_midShowKeyboard, show); };
}

JNIEXPORT void JNICALL
Java_com_shawarmaui_demo_MainActivity_nativeResize(JNIEnv*, jobject, jint w, jint h, jint topInset, jint bottomInset) {
    g_width = w;
    g_height = h;
    g_ui.SetSafeAreaInsets(topInset, bottomInset);
}

JNIEXPORT jboolean JNICALL
Java_com_shawarmaui_demo_MainActivity_nativeRender(JNIEnv*, jobject) {
    if (g_width <= 0 || g_height <= 0) return JNI_FALSE;
    g_ui.BeginFrame(g_width, g_height);
    RenderDemoUI();
    g_ui.EndFrame();
    return g_ui.HasActiveAnimations() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_shawarmaui_demo_MainActivity_nativeTouch(JNIEnv*, jobject, jint action, jint x, jint y) {
    if (action == 0) g_ui.OnTouchDown(x, y);      // ACTION_DOWN
    else if (action == 1) g_ui.OnTouchUp(x, y);   // ACTION_UP
    else if (action == 2) g_ui.OnTouchMove(x, y); // ACTION_MOVE
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
