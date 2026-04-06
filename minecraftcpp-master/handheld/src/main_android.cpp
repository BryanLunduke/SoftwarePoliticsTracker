#include "App.h"
#include "AppPlatform_android.h"

// Horrible, I know. / A
#ifndef MAIN_CLASS
#include "main.cpp"
#endif

#include <pthread.h>

// References for JNI
static pthread_mutex_t g_activityMutex = PTHREAD_MUTEX_INITIALIZER;
static jobject g_pActivity  = 0;
static AppPlatform_android appPlatform;

static void setupExternalPath(struct android_app* state, MAIN_CLASS* app)
{
    LOGI("setupExternalPath");

    JNIEnv* env = state->activity->env;
    state->activity->vm->AttachCurrentThread(&env, NULL);

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

    // We're done, detach!
    state->activity->vm->DetachCurrentThread();
}

extern "C" {
JNIEXPORT jint JNICALL
	JNI_OnLoad( JavaVM * vm, void * reserved )
	{
		pthread_mutex_init(&g_activityMutex, 0);
		pthread_mutex_lock(&g_activityMutex);

		LOGI("Entering OnLoad %d\n", pthread_self());
		return appPlatform.init(vm);
	}

	// Register/save a reference to the java main activity instance
	JNIEXPORT void JNICALL
	Java_com_mojang_minecraftpe_MainActivity_nativeRegisterThis(JNIEnv* env, jobject clazz) {
		LOGI("@RegisterThis %d\n", pthread_self());
		g_pActivity = (jobject)env->NewGlobalRef( clazz );

		pthread_mutex_unlock(&g_activityMutex);
	}

	// Unregister/delete the reference to the java main activity instance
	JNIEXPORT void JNICALL
	Java_com_mojang_minecraftpe_MainActivity_nativeUnregisterThis(JNIEnv* env, jobject clazz) {
		LOGI("@UnregisterThis %d\n", pthread_self());
		env->DeleteGlobalRef( g_pActivity ); 
		g_pActivity = 0;

		pthread_mutex_destroy(&g_activityMutex);
	}

	JNIEXPORT void JNICALL
	Java_com_mojang_minecraftpe_MainActivity_nativeStopThis(JNIEnv* env, jobject clazz) {
			LOGI("Lost Focus!");
	}
}

static void internal_process_input(struct android_app* app, struct android_poll_source* source) {
	AInputEvent* event = NULL;
	if (AInputQueue_getEvent(app->inputQueue, &event) >= 0) {
		LOGV("New input event: type=%d\n", AInputEvent_getType(event));
		bool isBackButtonDown = AKeyEvent_getKeyCode(event) == AKEYCODE_BACK && AKeyEvent_getAction(event) == AKEY_EVENT_ACTION_DOWN;
		if(!(appPlatform.isKeyboardVisible() && isBackButtonDown)) {
			if (AInputQueue_preDispatchEvent(app->inputQueue, event)) {
				return;
			}
		}
		int32_t handled = 0;
		if (app->onInputEvent != NULL) handled = app->onInputEvent(app, event);
		AInputQueue_finishEvent(app->inputQueue, event, handled);
	} else {
		LOGE("Failure reading next input event: %s\n", strerror(errno));
	}
}

void
android_main( struct android_app* state )
{
    struct ENGINE engine;

    // Make sure glue isn't stripped.
    app_dummy();

    memset( (void*)&engine, 0, sizeof(engine) );
    state->userData     = (void*)&engine;
    state->onAppCmd     = engine_handle_cmd;
    state->onInputEvent = engine_handle_input;
    state->destroyRequested = 0;

    pthread_mutex_lock(&g_activityMutex);
    appPlatform.instance = g_pActivity;
    pthread_mutex_unlock(&g_activityMutex);

    appPlatform.initConsts();

    //LOGI("socket-stuff\n");
    //socketDesc = initSocket(1999);

    App* app = new MAIN_CLASS();

    engine.userApp      = app;
    engine.app          = state;
    engine.is_inited    = false;
    engine.appContext.doRender = true;
    engine.appContext.platform = &appPlatform;

    setupExternalPath(state, (MAIN_CLASS*)app);

    if( state->savedState != NULL )
    {
        // We are starting with a previous saved state; restore from it.
       app->loadState(state->savedState, state->savedStateSize);
    }

    bool inited = false;
    bool teardownPhase = false;
	appPlatform._nativeActivity = state->activity;
    // our 'main loop'
    while( 1 )
    {
        // Read all pending events.
        int ident;
        int events;
        struct android_poll_source* source;
		
        while( (ident = ALooper_pollAll( 0, NULL, &events, (void**)&source) ) >= 0 )
        {
            // Process this event.
            // This will call the function pointer android_app::onInputEvent() which in our case is
            // engine_handle_input()
            if( source != NULL ) {
                if(source->id == 2) {
					// Back button is intercepted by the ime on android 4.1/4.2 resulting in the application stopping to respond.
					internal_process_input( state, source );
				} else {
					source->process( state, source );
				}
            }

        }
         // Check if we are exiting.
         if( state->destroyRequested )
         {
             //engine_term_display( &engine );
             delete app;
             return;
         }

		 if (!inited && engine.is_inited) {
			 app->init(engine.appContext);
			 app->setSize(engine.width, engine.height);
			 inited = true;
		 }

        if (inited && engine.is_inited && engine.has_focus) {
            app->update();
        } else {
            sleepMs(50);
        }    

        if (!teardownPhase && app->wantToQuit()) {
            teardownPhase = true;
            LOGI("tearing down!");
            ANativeActivity_finish(state->activity);
        }
    }
}
