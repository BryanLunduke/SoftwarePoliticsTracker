#include "App.h"
#include "AppPlatform_android.h"

//#include "main_android_java.h"
#include "platform/input/Multitouch.h"

// Horrible, I know. / A
#ifndef MAIN_CLASS
#include "main.cpp"
#endif


// References for JNI
static jobject g_pActivity  = 0;
static AppPlatform_android appPlatform;

static void setupExternalPath(JNIEnv* env, MAIN_CLASS* app)
{
    //JVMAttacher ta(vm);
    //JNIEnv* env = ta.getEnv();

    LOGI("setupExternalPath");

    if (env)
    {
        LOGI("Environment exists");
    }
    jclass clazz = env->FindClass("android/os/Environment");
    jmethodID method = env->GetStaticMethodID(clazz, "getExternalStorageDirectory", "()Ljava/io/File;");
    if (env->ExceptionOccurred()) {
        env->ExceptionDescribe();
    }
    jobject file = env->CallStaticObjectMethod(clazz, method);

    jclass fileClass = env->GetObjectClass(file);
    jmethodID fileMethod = env->GetMethodID(fileClass, "getAbsolutePath", "()Ljava/lang/String;");
    jobject pathString = env->CallObjectMethod(file, fileMethod);

    const char* str = env->GetStringUTFChars((jstring) pathString, NULL);
    app->externalStoragePath = str;
	app->externalCacheStoragePath = str;
    LOGI(str);

    env->ReleaseStringUTFChars((jstring)pathString, str);
}

static void pointerDown(int pointerId, int x, int y) {
    Multitouch::feed(1, 1, x, y, pointerId);
}
static void pointerUp(int pointerId, int x, int y) {
    Multitouch::feed(1, 0, x, y, pointerId);
}
static void pointerMove(int pointerId, int x, int y) {
    Multitouch::feed(0, 0, x, y, pointerId);
}


static App* gApp = 0;
static AppContext gContext;

extern "C" {
JNIEXPORT jint JNICALL
JNI_OnLoad( JavaVM * vm, void * reserved )
{
    LOGI("Entering OnLoad\n");
    return appPlatform.init(vm);
}

// Register/save a reference to the java main activity instance
JNIEXPORT void JNICALL
Java_com_mojang_minecraftpe_MainActivity_nativeRegisterThis(JNIEnv* env, jobject clazz) {
    LOGI("@RegisterThis\n");
    g_pActivity = (jobject)env->NewGlobalRef( clazz );
}

// Unregister/delete the reference to the java main activity instance
JNIEXPORT void JNICALL
Java_com_mojang_minecraftpe_MainActivity_nativeUnregisterThis(JNIEnv* env, jobject clazz) {
    LOGI("@UnregisterThis\n");
    env->DeleteGlobalRef( g_pActivity );
}

JNIEXPORT void JNICALL
Java_com_mojang_minecraftpe_MainActivity_nativeOnCreate(JNIEnv* env) {
    LOGI("@nativeOnCreate\n");

    appPlatform.instance = g_pActivity;
    appPlatform.initConsts();
    gContext.doRender = false;
    gContext.platform = &appPlatform;

    gApp = new MAIN_CLASS();
    setupExternalPath(env, (MAIN_CLASS*)gApp);
    //gApp->init(gContext);
}

JNIEXPORT void JNICALL
Java_com_mojang_minecraftpe_GLRenderer_nativeOnSurfaceCreated(JNIEnv* env) {
    LOGI("@nativeOnSurfaceCreated\n");

     if (gApp) {
//          gApp->setSize( gContext.platform->getScreenWidth(),
//                         gContext.platform->getScreenHeight(),
//                         gContext.platform->isTouchscreen());

         // Don't call onGraphicsReset the first time
        if (gApp->isInited())
            gApp->onGraphicsReset(gContext);

        if (!gApp->isInited())
            gApp->init(gContext);
     }
}

JNIEXPORT void JNICALL
Java_com_mojang_minecraftpe_GLRenderer_nativeOnSurfaceChanged(JNIEnv* env, jclass cls, jint w, jint h) {
    LOGI("@nativeOnSurfaceChanged: %p\n", pthread_self());

    if (gApp) {
        gApp->setSize(w, h);

        if (!gApp->isInited())
            gApp->init(gContext);

        if (!gApp->isInited())
            LOGI("nativeOnSurfaceChanged: NOT INITED!\n");
    }
}

JNIEXPORT void JNICALL
Java_com_mojang_minecraftpe_MainActivity_nativeOnDestroy(JNIEnv* env) {
    LOGI("@nativeOnDestroy\n");

    delete gApp;
    gApp = 0;
    //gApp->onGraphicsReset(gContext);
}

JNIEXPORT void JNICALL
Java_com_mojang_minecraftpe_GLRenderer_nativeUpdate(JNIEnv* env) {
    //LOGI("@nativeUpdate: %p\n", pthread_self());
    if (gApp) {
        if (!gApp->isInited())
            gApp->init(gContext);

        gApp->update();

        if (gApp->wantToQuit())
            appPlatform.finish();
    }
}

//
// Keyboard events
//
JNIEXPORT void JNICALL
Java_com_mojang_minecraftpe_MainActivity_nativeOnKeyDown(JNIEnv* env, jclass cls, jint keyCode) {
    LOGI("@nativeOnKeyDown: %d\n", keyCode);
    Keyboard::feed(keyCode, true);
}
JNIEXPORT void JNICALL
Java_com_mojang_minecraftpe_MainActivity_nativeOnKeyUp(JNIEnv* env, jclass cls, jint keyCode) {
    LOGI("@nativeOnKeyUp: %d\n", (int)keyCode);
    Keyboard::feed(keyCode, false);
}

JNIEXPORT jboolean JNICALL
Java_com_mojang_minecraftpe_MainActivity_nativeHandleBack(JNIEnv* env, jclass cls, jboolean isDown) {
    LOGI("@nativeHandleBack: %d\n", isDown);
    if (gApp) return gApp->handleBack(isDown)? JNI_TRUE : JNI_FALSE;
    return JNI_FALSE;
}

//
// Mouse events
//
JNIEXPORT void JNICALL
Java_com_mojang_minecraftpe_MainActivity_nativeMouseDown(JNIEnv* env, jclass cls, jint pointerId, jint buttonId, jfloat x, jfloat y) {
    //LOGI("@nativeMouseDown: %f %f\n", x, y);
    mouseDown(1, x, y);
    pointerDown(pointerId, x, y);
}
JNIEXPORT void JNICALL
Java_com_mojang_minecraftpe_MainActivity_nativeMouseUp(JNIEnv* env, jclass cls, jint pointerId, jint buttonId, jfloat x, jfloat y) {
    //LOGI("@nativeMouseUp: %f %f\n", x, y);
    mouseUp(1, x, y);
    pointerUp(pointerId, x, y);
}
JNIEXPORT void JNICALL
Java_com_mojang_minecraftpe_MainActivity_nativeMouseMove(JNIEnv* env, jclass cls, jint pointerId, jfloat x, jfloat y) {
    //LOGI("@nativeMouseMove: %f %f\n", x, y);
    mouseMove(x, y);
    pointerMove(pointerId, x, y);
}
}
