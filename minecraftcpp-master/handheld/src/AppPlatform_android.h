#ifndef APPPLATFORM_ANDROID_H__
#define APPPLATFORM_ANDROID_H__

#include "AppPlatform.h"
#include "client/renderer/gles.h"
#include "platform/log.h"
#include "platform/time.h"
#include <jni.h>
#include <cmath>
#include <exception>
#include <android/native_activity.h>
class JVMAttacher {
public:
    JVMAttacher(JavaVM* vm)
    :   _vm(vm),
        _isAttached(false),
        _env(NULL)
    {
        if( _vm->GetEnv((void**)&_env, JNI_VERSION_1_4 ) != JNI_OK ) {
            _vm->AttachCurrentThread(&_env, NULL);
            _isAttached = _env != NULL;
        }
    }

    ~JVMAttacher() {
        forceDetach();
    }

    JNIEnv* getEnv() {
        return _env;
    }

    void forceDetach() {
        if (!_isAttached) return;
        _vm->DetachCurrentThread();
        _isAttached = false;
    }

private:
    JavaVM* _vm;
    JNIEnv* _env;
    bool    _isAttached;
};

class AppPlatform_android: public AppPlatform
{
    typedef AppPlatform super;
public:
    AppPlatform_android()
    :   _vm(NULL),
        _isInited(false),

        instance(0),
        _activityClass(0),

        _methodSaveScreenshot(0),
        _postScreenshotToFacebook(0),
        _getImageData(0),
		_readAssetFile(0),
        _methodPlaySound(0),
        _showDialog(0),
        _methodTick(0),
        _methodFinish(0),
        
        _methodUserInputInitiate(0),
        _methodUserInputStatus(0),
        _methodUserInputString(0),

        _methodGetDateString(0),

        _methodCheckLicense(0),
        _methodHasBuyButton(0),
        _methodBuyGame(0),
        _methodVibrate(0),
        _methodSupportsTouchscreen(0),
		_methodSetIsPowerVR(0),
		_methodIsNetworkEnabled(0),

        _getScreenWidth(0),
        _getScreenHeight(0),
        _methodGetPixelsPerMillimeter(0),
		_methodGetPlatformStringVar(0),

		_classWindow(0),
		_classContext(0),
		_fieldINPUT_METHOD_SERVICE(0),
		_classInputManager(0),
		_methodGetSystemService(0),
		_methodGetWindow(0),
		_methodGetDecorView(0),
		_methodShowSoftInput(0),
		_classView(0),
		_methodGetWindowToken(0),
		_methodHideSoftInput(0),

        _screenWidth(854),
        _screenHeight(480),
		_hasSetPowerVR(false),
		_nativeActivity(0),

		_methodGetKeyFromKeyCode(0)
    {
    }

    int getScreenWidth() { return _screenWidth; }
    int getScreenHeight() { return _screenHeight; }

    // Note, this has to be called from the main thread (e.g. do it from JNI_onLoad)
    // Somewhere between calling this, and calling the AppPlatform methods,
    // this->instance will be assigned by its creator
    int init(JavaVM* vm) {
        if (_isInited)
            return -1;

        JVMAttacher ta(vm);
        JNIEnv* env = ta.getEnv();

        if(!env) {
            LOGI("%s - Failed to get the environment using JVMAttacher::getEnv()", __FUNCTION__);
            return -2;
        }

        // Get Class reference
        const char* interface_path = "com/mojang/minecraftpe/MainActivity";
        jclass clazz = NULL;
        if( (clazz = env->FindClass( interface_path )) == 0 ) {
            return -3;
        }
        _activityClass = (jclass)env->NewGlobalRef(clazz);

        // Save all the method IDs
        _methodSaveScreenshot = env->GetStaticMethodID( _activityClass, "saveScreenshot", "(Ljava/lang/String;II[I)V");
        _postScreenshotToFacebook = env->GetMethodID( _activityClass, "postScreenshotToFacebook", "(Ljava/lang/String;II[I)V");
        _getImageData = env->GetMethodID( _activityClass, "getImageData", "(Ljava/lang/String;)[I");
		_readAssetFile = env->GetMethodID( _activityClass, "getFileDataBytes", "(Ljava/lang/String;)[B");
        #if defined(PRE_ANDROID23)
            _methodPlaySound = env->GetMethodID( _activityClass, "playSound", "(Ljava/lang/String;FF)V");
        #endif
        _showDialog = env->GetMethodID( _activityClass, "displayDialog", "(I)V");
        _methodTick = env->GetMethodID( _activityClass, "tick", "()V");
        _methodFinish = env->GetMethodID( _activityClass, "quit", "()V");

        _methodUserInputInitiate = env->GetMethodID( _activityClass, "initiateUserInput", "(I)V");
        _methodUserInputStatus = env->GetMethodID( _activityClass, "getUserInputStatus", "()I");
        _methodUserInputString = env->GetMethodID( _activityClass, "getUserInputString", "()[Ljava/lang/String;");

        _methodGetDateString = env->GetMethodID( _activityClass, "getDateString", "(I)Ljava/lang/String;");

        _methodCheckLicense = env->GetMethodID( _activityClass, "checkLicense", "()I");
        _methodHasBuyButton = env->GetMethodID( _activityClass, "hasBuyButtonWhenInvalidLicense", "()Z");
        _methodBuyGame = env->GetMethodID( _activityClass, "buyGame", "()V");

        _methodVibrate = env->GetMethodID( _activityClass, "vibrate", "(I)V");
        _methodSupportsTouchscreen = env->GetMethodID( _activityClass, "supportsTouchscreen", "()Z");
		_methodSetIsPowerVR  = env->GetMethodID( _activityClass, "setIsPowerVR", "(Z)V");
		_methodIsNetworkEnabled = env->GetMethodID( _activityClass, "isNetworkEnabled", "(Z)Z");

		_methodGetPixelsPerMillimeter = env->GetMethodID( _activityClass, "getPixelsPerMillimeter", "()F");
		_methodGetPlatformStringVar = env->GetMethodID( _activityClass, "getPlatformStringVar", "(I)Ljava/lang/String;");

		_classWindow = (jclass)env->NewGlobalRef(env->FindClass("android/view/Window"));
		_classContext = (jclass)env->NewGlobalRef(env->FindClass("android/content/Context"));
		_classView = (jclass)env->NewGlobalRef(env->FindClass( "android/view/View"));
		_classInputManager = (jclass)env->NewGlobalRef(env->FindClass("android/view/inputmethod/InputMethodManager"));

		_fieldINPUT_METHOD_SERVICE = env->GetStaticFieldID(_classContext, "INPUT_METHOD_SERVICE", "Ljava/lang/String;");
		_methodGetSystemService = env->GetMethodID(_activityClass, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
		_methodGetWindow = env->GetMethodID( _activityClass, "getWindow", "()Landroid/view/Window;");
		_methodGetDecorView = env->GetMethodID(_classWindow, "getDecorView", "()Landroid/view/View;");
		_methodShowSoftInput = env->GetMethodID(_classInputManager, "showSoftInput", "(Landroid/view/View;I)Z");
		_methodGetWindowToken = env->GetMethodID(_classView, "getWindowToken", "()Landroid/os/IBinder;");
		_methodHideSoftInput = env->GetMethodID(_classInputManager, "hideSoftInputFromWindow", "(Landroid/os/IBinder;I)Z");
		
		_methodGetKeyFromKeyCode = env->GetMethodID( _activityClass, "getKeyFromKeyCode", "(III)I");

        if (env->ExceptionOccurred()) {
            env->ExceptionDescribe();
        }

 //       LOGI("Class: %d\n. Methods: %d,%d\n", _activityClass,
   //         _methodSaveScreenshot,
     //       _postScreenshotToFacebook);

        _vm = vm;
        _isInited = true;

        return JNI_VERSION_1_4;
    }

    // Stuff that should only be written once, e.g. device specs
    // @note: This is called after instance is set, BUT this will
    //        be rewritten later on anyway
    int initConsts() {

        JVMAttacher ta(_vm);
        JNIEnv* env = ta.getEnv();

        jmethodID fWidth = env->GetMethodID( _activityClass, "getScreenWidth", "()I");
        jmethodID fHeight = env->GetMethodID( _activityClass, "getScreenHeight", "()I");

        _screenWidth = env->CallIntMethod(instance, fWidth);
        _screenHeight = env->CallIntMethod(instance, fHeight);
    }

    void tick() {
        if (!_isInited) return;
        if (!_methodTick) return;

        JVMAttacher ta(_vm);
        JNIEnv* env = ta.getEnv();

        env->CallVoidMethod(instance, _methodTick);
    }

    void showDialog(int dialogId) {
        if (!_isInited) return;
        if (!_showDialog) return;

        JVMAttacher ta(_vm);
        JNIEnv* env = ta.getEnv();

        env->CallVoidMethod(instance, _showDialog, dialogId);
    }

    void createUserInput() {
        if (!_isInited) return;
        if (!_methodUserInputInitiate) return;

        JVMAttacher ta(_vm);
        JNIEnv* env = ta.getEnv();

        env->CallVoidMethod(instance, _methodUserInputInitiate, 1);
    }

    int getUserInputStatus() {
        if (!_isInited) return -2;
        if (!_methodUserInputStatus) return -2;

        JVMAttacher ta(_vm);
        JNIEnv* env = ta.getEnv();

        return env->CallIntMethod(instance, _methodUserInputStatus);
    }

    float getPixelsPerMillimeter() {
        if (!_isInited) return 10;
        if (!_methodGetPixelsPerMillimeter) return 10;

        JVMAttacher ta(_vm);
        JNIEnv* env = ta.getEnv();

        return env->CallFloatMethod(instance, _methodGetPixelsPerMillimeter);
    }

    StringVector getUserInput() {
        if (!_isInited) return std::vector<std::string>();
        if (!_methodUserInputString) return std::vector<std::string>();

        JVMAttacher ta(_vm);
        JNIEnv* env = ta.getEnv();

        jobjectArray sa = (jobjectArray) env->CallObjectMethod(instance, _methodUserInputString);
        jsize len = env->GetArrayLength(sa);

        StringVector out;
        for (int i = 0; i < len; ++i) {

            jstring s = (jstring)env->GetObjectArrayElement(sa, i);
            int length = env->GetStringLength (s);
            if (length > 1023)
                length = 1023;
            static char buf[1024];

            // Only supporting 7 bit chars, right now
            const char* str = env->GetStringUTFChars(s, 0);
            strncpy(buf, str, 1023);
            buf[1023] = 0;

            out.push_back(std::string(buf));
        }
        return out;
    }

    void saveScreenshot(const std::string& filename, int glWidth, int glHeight) {
        if (!_isInited) return;
        if (!_methodSaveScreenshot) return;

        JVMAttacher ta(_vm);
        JNIEnv* env = ta.getEnv();

        const int numPixels = glWidth * glHeight;
        unsigned int* pixels = new unsigned int[numPixels];

        if (!pixels)
            return;

        // Create the OpenGL "screenshot"
        glReadPixels(0, 0, glWidth, glHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        convertOpenGlToAndroidPixels(glWidth, glHeight, pixels);

        // Tell Android to save the screenshot
        jintArray jPixels = env->NewIntArray(numPixels);
        if (jPixels != NULL)
        {
            jobject gpixRef = env->NewGlobalRef(jPixels);
            env->SetIntArrayRegion(jPixels, 0, numPixels, (int*)pixels);
//                env->CallVoidMethod(    instance,
//                                        _postScreenshotToFacebook,
//                                        env->NewStringUTF(filename.c_str()),
//                                        glWidth, glHeight, jPixels);
            env->CallStaticVoidMethod(  _activityClass,
                                        _methodSaveScreenshot,
                                        env->NewStringUTF(filename.c_str()),
                                        glWidth, glHeight, jPixels);
            // Teardown
            env->DeleteGlobalRef(gpixRef);
            delete[] pixels;
        }
    }

    __inline unsigned int rgbToBgr(unsigned int p) {
        return (p & 0xff00ff00) | ((p >> 16) & 0xff) | ((p << 16) & 0xff0000);
    }

    void convertOpenGlToAndroidPixels(int w, int h, unsigned int* pixels) {
        // Making the conversion in-place
        for (int y = 0; y < h/2; ++y)
        for (int x = 0; x < w;  ++x) {
            const int i0 = y * w + x;
            const int i1 = (h - y - 1) * w + x;
            const unsigned int p1 = pixels[i1];
            const unsigned int p0 = pixels[i0];
            pixels[i0] = rgbToBgr(p1);
            pixels[i1] = rgbToBgr(p0);
        }
        // Handle the middle row, if any
        if (h & 1) {
            const int start = w * h/2;
            const int stop = start + w;
            for (int i = start; i < stop; ++i) {
                const unsigned int p = pixels[i];
                pixels[i] = rgbToBgr(p);
            }
        }
    }

    virtual void playSound(const std::string& filename, float volume, float pitch) {
        if (!_isInited || !_methodPlaySound) return;

        //static Stopwatch w;
        //w.start();
        JVMAttacher ta(_vm);
        JNIEnv* env = ta.getEnv();

        env->CallVoidMethod(instance, _methodPlaySound, env->NewStringUTF(filename.c_str()), volume, pitch);
        //w.stop();
        //w.printEvery(1, "playSound-java");
    }

    virtual void uploadPlatformDependentData(int id, void* data) {
        if (!_isInited) return;

        JVMAttacher ta(_vm);
        JNIEnv* env = ta.getEnv();

        if (id == 1) {
        }
        if (id == 2) {
        }
    }

    TextureData loadTexture(const std::string& filename, bool textureFolder) {
        if (!_isInited) return TextureData();
        if (!_getImageData) return TextureData();

        JVMAttacher ta(_vm);
        JNIEnv* env = ta.getEnv();

        jintArray arr = (jintArray)env->CallObjectMethod(
            instance, _getImageData, env->NewStringUTF(filename.c_str()));

        if (!arr)
            return TextureData();

        jsize len = env->GetArrayLength(arr);
        int numPixels = len-2;

        // pixels = [width, height, p_0, p_1, ..., p_numPixels-1]
        int* pixels = new int[numPixels];
        jint* body = env->GetIntArrayElements(arr, 0);

        int w = body[0];
        int h = body[1];
        memcpy(pixels, &body[2], numPixels * sizeof(int));

        // Convert pixels
        for (int i = 0; i < numPixels; ++i)
            pixels[i] = rgbToBgr(pixels[i]);

        //LOGI("Read-Image-Data: Decided file size to be: %d x %d (len %d)\n", w, h, len);

        env->ReleaseIntArrayElements(arr, body, 0);
  //      LOGI("loadtexture: 9\n");

        // Set the result
        TextureData out;
        out.w = w;
        out.h = h;
        out.data = (unsigned char*)pixels;
        out.memoryHandledExternally = false;
        return out;
    }

	BinaryBlob readAssetFile(const std::string& filename) {
		if (!_isInited) return BinaryBlob();
		if (!_readAssetFile) return BinaryBlob();

		JVMAttacher ta(_vm);
		JNIEnv* env = ta.getEnv();

		jbyteArray arr = (jbyteArray)env->CallObjectMethod(
			instance, _readAssetFile, env->NewStringUTF(filename.c_str()));

		if (!arr)
			return BinaryBlob();

		jsize len = env->GetArrayLength(arr);
		BinaryBlob blob(new unsigned char[len], len);
		jbyte* body = env->GetByteArrayElements(arr, 0);
		memcpy(blob.data, body, len);

		env->ReleaseByteArrayElements(arr, body, 0);
		return blob;
	}

	std::string getDateString(int s) {
        if (!_isInited) return "";
        if (!_methodGetDateString) return "";

        JVMAttacher ta(_vm);
        JNIEnv* env = ta.getEnv();

        std::string out;
        jstring dateString = (jstring)env->CallObjectMethod(instance, _methodGetDateString, s);
        const char* str = env->GetStringUTFChars(dateString, NULL);
        if (str)
            out = str;
        env->ReleaseStringUTFChars(dateString, str);
        return out;
    }

	std::string getPlatformStringVar(int s) {
		if (!_isInited) return "";
		if (!_methodGetPlatformStringVar) return "";

		JVMAttacher ta(_vm);
		JNIEnv* env = ta.getEnv();

		std::string out;
		jstring stringVar = (jstring)env->CallObjectMethod(instance, _methodGetPlatformStringVar, s);
		const char* str = env->GetStringUTFChars(stringVar, NULL);
		if (str)
			out = str;
		env->ReleaseStringUTFChars(stringVar, str);
		return out;
	}

	int checkLicense() {
        if (!_isInited) return -2;
        if (!_methodCheckLicense) return -2;

        JVMAttacher ta(_vm);
        JNIEnv* env = ta.getEnv();

        return env->CallIntMethod(instance, _methodCheckLicense);
    }

    bool hasBuyButtonWhenInvalidLicense() {
        if (!_isInited) return false;
        if (!_methodHasBuyButton) return false;

        JVMAttacher ta(_vm);
        JNIEnv* env = ta.getEnv();

        return JNI_TRUE == env->CallBooleanMethod(instance, _methodHasBuyButton);
    }

    void buyGame() {
        if (!_isInited) return;
        if (!_methodBuyGame) return;

        JVMAttacher ta(_vm);
        JNIEnv* env = ta.getEnv();

        env->CallVoidMethod(instance, _methodBuyGame);
    }

    virtual void finish() {
        if (!_isInited) return;
        if (!_methodFinish) return;

        JVMAttacher ta(_vm);
        JNIEnv* env = ta.getEnv();

        env->CallVoidMethod(instance, _methodFinish);
        env->DeleteGlobalRef( _activityClass );

        env->DeleteGlobalRef( _classWindow );
        env->DeleteGlobalRef( _classContext );
        env->DeleteGlobalRef( _classView );
        env->DeleteGlobalRef( _classInputManager );
    }

    virtual bool supportsTouchscreen() {
        if (!_isInited) return true;
        if (!_methodSupportsTouchscreen) return true;

        JVMAttacher ta(_vm);
        JNIEnv* env = ta.getEnv();

        return env->CallBooleanMethod(instance, _methodSupportsTouchscreen);
    }

	virtual void vibrate(int milliSeconds) {
        if (!_isInited) return;
        if (!_methodVibrate) return;

        JVMAttacher ta(_vm);
        JNIEnv* env = ta.getEnv();

        return env->CallVoidMethod(instance, _methodVibrate, milliSeconds);
    }

	virtual bool isPowerVR() {
	    bool is = super::isPowerVR();

		if (_methodSetIsPowerVR && _isInited && !_hasSetPowerVR ) {
			JVMAttacher ta(_vm);
			JNIEnv* env = ta.getEnv();
			_hasSetPowerVR = true;
			env->CallVoidMethod(instance, _methodSetIsPowerVR, is);
		}
		return is;
	}

	virtual bool isNetworkEnabled(bool onlyWifiAllowed) {
		if (!_isInited) return false;
		if (!_methodIsNetworkEnabled) return false;

		JVMAttacher ta(_vm);
		JNIEnv* env = ta.getEnv();

		return env->CallBooleanMethod(instance, _methodIsNetworkEnabled, onlyWifiAllowed);
	}

    static __inline bool isSquare(int n) {
        int L = n & 0xf;
        if ((1 << L) & 0x213 == 0) return false;

        int t = (int) sqrt((double) n) + 0.5;
        return t*t == n;
    }

	virtual void showKeyboard() {
		showKeyboard(true);
		super::showKeyboard();
	}
	virtual void hideKeyboard() {
		showKeyboard(false);
		super::hideKeyboard();
	}
	virtual void showKeyboard(bool bShow) {
		if (!_isInited) return;

		jint lResult = 0;
		jint lFlags = 0;

		JVMAttacher ta(_vm);
		JNIEnv* env = ta.getEnv();

 		jobject INPUT_METHOD_SERVICE =
 			env->GetStaticObjectField(_classContext,
 			_fieldINPUT_METHOD_SERVICE);

		jobject lInputMethodManager = env->CallObjectMethod(
			instance, _methodGetSystemService,
			INPUT_METHOD_SERVICE);

		jobject lWindow = env->CallObjectMethod(instance,
			_methodGetWindow);

		jobject lDecorView = env->CallObjectMethod(lWindow,
			_methodGetDecorView);

		if (bShow) {
			// Runs lInputMethodManager.showSoftInput(...).
			jboolean lResult = env->CallBooleanMethod(
				lInputMethodManager, _methodShowSoftInput,
				lDecorView, lFlags);
		} else {
			// Runs lWindow.getViewToken()
			jobject lBinder = env->CallObjectMethod(lDecorView,
				_methodGetWindowToken);
			jboolean lRes = env->CallBooleanMethod(
				lInputMethodManager, _methodHideSoftInput,
				lBinder, lFlags);
		}
	}
	virtual int getKeyFromKeyCode(int keyCode, int metaState, int deviceId) {
		JVMAttacher ta(_vm);
		JNIEnv* env = ta.getEnv();

		return (int) env->CallIntMethod(instance, _methodGetKeyFromKeyCode, keyCode, metaState, deviceId);
	}
public:
    jobject instance;

private:
    bool _isInited;
    JavaVM* _vm;

    jclass _activityClass;

    jmethodID _methodSaveScreenshot;
    jmethodID _postScreenshotToFacebook;
    jmethodID _getImageData;
	jmethodID _readAssetFile;
    jmethodID _methodPlaySound;
    jmethodID _showDialog;
    jmethodID _methodTick;
    jmethodID _methodFinish;

    jmethodID _methodUserInputInitiate;
    jmethodID _methodUserInputStatus;
    jmethodID _methodUserInputString;

    jmethodID _methodGetDateString;

    jmethodID _methodCheckLicense;
    jmethodID _methodHasBuyButton;
    jmethodID _methodBuyGame;

    jmethodID _getScreenWidth;
    jmethodID _getScreenHeight;
    jmethodID _methodGetPixelsPerMillimeter;
    jmethodID _methodVibrate;
    jmethodID _methodSupportsTouchscreen;
    jmethodID _methodSetIsPowerVR;
	jmethodID _methodIsNetworkEnabled;

	jmethodID _methodGetPlatformStringVar;

	jclass _classWindow;
	jclass _classContext;
	jfieldID _fieldINPUT_METHOD_SERVICE;
	jclass _classInputManager;
	jmethodID _methodGetSystemService;
	jmethodID _methodGetWindow;
	jmethodID _methodGetDecorView;
	jmethodID _methodShowSoftInput;
	jclass _classView;
	jmethodID _methodGetWindowToken;
	jmethodID _methodHideSoftInput;

	jmethodID _methodGetKeyFromKeyCode;

    int _screenWidth;
    int _screenHeight;
	bool _hasSetPowerVR;
public:
	ANativeActivity* _nativeActivity;
};

#endif /*APPPLATFORM_ANDROID_H__*/
