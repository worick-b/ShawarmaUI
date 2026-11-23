# Regras JNI padrao para apps Android que usam ShawarmaUI_Android.h
# Protege todos os callbacks chamados pelo C++ (via GetMethodID) contra remocao/renomeacao pelo R8

-keepclassmembers class * {
    public void requestRenderFromNative();
    public void setKeyboardVisible(boolean);
    public void *FromNative(...);
}
