#ifndef MAIN_CLASS
#error "Error: MAIN_CLASS must have been defined to your main class (e.g. #define MAIN_CLASS MyApp)"
#endif

#include <jni.h>
#include <math.h>
#include <errno.h>

#include <EGL/egl.h>
#include <GLES/gl.h>

#include <android/log.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>

#define TAG "MAIN_CLASS"

#include "platform/log.h"
#include "platform/input/Keyboard.h"
#include "platform/input/Mouse.h"
#include "platform/input/Controller.h"

#include "EGLConfigPrinter.h"

const int BroadcastPort = 9991;

/**
 * Shared state for our app.
 */
struct ENGINE
{
    struct android_app* app;

    int         render;
    EGLDisplay  display;
    EGLSurface  surface;
    EGLContext  context;
    int         width;
    int         height;
    int         has_focus;
    App*        userApp;
    bool is_inited;
    //bool init_gl;
    struct AppContext  appContext;
};

static void mouseDown(int buttonId, int x, int y) {
    int msg[] = {buttonId, 0, x, y};
    //broadcastData(BroadcastPort, msg, sizeof(msg));

    Mouse::feed(buttonId, 1, x, y);
}

static void mouseUp(int buttonId, int x, int y) {
    int msg[] = {buttonId, 0, x, y};
    //broadcastData(BroadcastPort, msg, sizeof(msg));

    Mouse::feed(buttonId, 0, x, y);
}

static void mouseMove(int x, int y) {
    int msg[] = {0, 0, x, y};
    //broadcastData(BroadcastPort, msg, sizeof(msg));

    Mouse::feed(0, 0, x, y);
}

#if 0

/**
 * Process the next input event.
 */
static
int32_t
engine_handle_input( struct android_app* app, AInputEvent* event )
{
    struct ENGINE* engine = (struct ENGINE*)app->userData;
    if( AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION )
    {
        //engine->render        = 1;
        int nPointerCount   = AMotionEvent_getPointerCount( event );
        int nSourceId       = AInputEvent_getSource( event );
        int n;

        for( n = 0 ; n < nPointerCount ; ++n )
        {
            int nPointerId  = AMotionEvent_getPointerId( event, n );
            int nAction     = AMOTION_EVENT_ACTION_MASK & AMotionEvent_getAction( event );
            int isTrackpad = nSourceId == AINPUT_SOURCE_TOUCHPAD;

            // We don't care about secondary pointers right now
            if( !isTrackpad && (nAction == AMOTION_EVENT_ACTION_POINTER_DOWN || nAction == AMOTION_EVENT_ACTION_POINTER_UP ))
                continue;

            int x = AMotionEvent_getX( event, n );
            int y = AMotionEvent_getY( event, n );

            if( nAction == AMOTION_EVENT_ACTION_DOWN || nAction == AMOTION_EVENT_ACTION_POINTER_DOWN ) {
                //LOGI("action down %d\n", isTrackpad);
                if (isTrackpad) {
                    trackpadPress(x, y);
                } else {
                    mouseDown(1, x, y);
                    //LOGI("mouse-pointer down");
                }
            }
            else if( nAction == AMOTION_EVENT_ACTION_UP || nAction == AMOTION_EVENT_ACTION_POINTER_UP /*|| nAction == AMOTION_EVENT_ACTION_CANCEL*/ ) {
                //LOGI("action up %d\n", isTrackpad);
                if (isTrackpad) {
                    //LOGI("trackpad-up\n");
                    trackpadRelease(x, y);
                }
                else {
                    //LOGI("mouse-pointer up\n");
                    mouseUp(1, x, y);
                }
            } else if (nAction == AMOTION_EVENT_ACTION_MOVE ) {
                if (isTrackpad)
                    trackpadMove(x, y);
                else
                    mouseMove(x, y);
            }
        }
        return 1;
    }
    else if( AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY )
    {
       bool isDown = AKeyEvent_getAction(event) == AKEY_EVENT_ACTION_DOWN;
       int keyCode = AKeyEvent_getKeyCode(event);
       bool pressedBack = keyCode == 4 && AInputEvent_getDeviceId(event) == 0;
       bool isRepeat = AKeyEvent_getRepeatCount(event) > 0;
       if (!isRepeat){
           //int scanCode = AKeyEvent_getScanCode(event);
           //LOGI("key-%s : %d", isDown?"key-down":"key-up", keyCode);
           //LOGI("getId: %d\n", AInputEvent_getDeviceId(event));
           //LOGI("flags: %d\n", AKeyEvent_getFlags(event));
           //LOGI("scancode: %d\n", AKeyEvent_getScanCode(event));

           //unsigned char msg[2] = {(unsigned char)(keyCode), (unsigned char)(isDown)};
           //broadcastData(BroadcastPort, msg, 2);

		   if (!pressedBack)
			   Keyboard::feed(keyCode, isDown);
       }
       if (keyCode == 4)
       {
           if (!isRepeat && !engine->userApp->handleBack(isDown))
           {
               //LOGI("Returning 0 from engine_handle_input\n");
               return 0;
           }
           return 1;
       }

       bool volumeButton = (keyCode == 24) || (keyCode == 25);
       bool handled = !pressedBack && !volumeButton;
       return handled;
    }

    return 0;
}

#endif

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things (rendering).
 */
void
android_main( struct android_app* state );
