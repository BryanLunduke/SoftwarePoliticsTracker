#ifndef MAIN_CLASS
#error "Error: MAIN_CLASS must have been defined to your main class (e.g. #define MAIN_CLASS MyApp)"
#endif

#include <jni.h>
#include <math.h>
#include <errno.h>

#include <EGL/egl.h>
#include <GLES/gl.h>

#include <android/log.h>
#include <android_native_app_glue.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>

#define TAG "MAIN_CLASS"

#include "platform/log.h"
#include "platform/input/Controller.h"
#include "platform/input/Keyboard.h"
#include "platform/input/Mouse.h"
#include "platform/input/Multitouch.h"

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

/**
 * Initialize an EGL context for the current display.
 */
static
int
engine_init_display( struct ENGINE* engine )
{
    LOGI("1) Initing display. Get display\n");
    // initialize OpenGL ES 2.0 and EGL
    const EGLint attribs[] = \
    {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            //EGL_BUFFER_SIZE, 32,
            //EGL_BLUE_SIZE, 5,
            //EGL_GREEN_SIZE, 6,
            //EGL_RED_SIZE, 5,
            ///EGL_ALPHA_SIZE, 8,
            //0x3098, 1,
            //EGL_TRANSPARENT_TYPE, EGL_TRANSPARENT_RGB,
            //EGL_ALPHA_SIZE, 8,//5,
            EGL_DEPTH_SIZE, 16, //8
            //EGL_STENCIL_SIZE, 8,
            //EGL_DEPTH_SIZE, 16,
            //EGL_CONFIG_CAVEAT,   EGL_NONE,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT,
            EGL_NONE
    };

    EGLint w, h, format;
    EGLint numConfigs;
    EGLConfig config;
    EGLSurface surface;
    EGLContext context;

    EGLDisplay display = eglGetDisplay( EGL_DEFAULT_DISPLAY );

    LOGI("2) Initialize egl\n");

    if( display == NULL )
    {
        LOGW( "!!! NO DISPLAY !!! eglGetDisplay" );
    }

    eglInitialize( display, 0, 0 );
    //EGLConfigPrinter::printAllConfigs(display);
    LOGI("3) Choose config\n");

    /* Here, the application chooses the configuration it desires. In this
     * sample, we have a very simplified selection process, where we pick
     * the first EGLConfig that matches our criteria */
    eglChooseConfig( display, attribs, &config, 1, &numConfigs );
    LOGI("4) Get config attributes\n");

    /* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
     * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
     * As soon as we picked a EGLConfig, we can safely reconfigure the
     * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
    eglGetConfigAttrib( display, config, EGL_NATIVE_VISUAL_ID, &format );
    LOGI("5) Choose config, set window geometry\n");

    ANativeWindow_setBuffersGeometry( engine->app->window, 0, 0, format );

    eglChooseConfig( display, attribs, NULL, 0, &numConfigs );

        EGLConfig* configs = new EGLConfig[numConfigs];
        eglChooseConfig(display, attribs, configs, numConfigs, &numConfigs);

        for (int i = 0; i < numConfigs; ++i) {
            EGLSurface surf = eglCreateWindowSurface( display, configs[i], engine->app->window, NULL );
            if (surf != EGL_NO_SURFACE) {
                config = configs[i];
                surface = surf;
                break;
            }
        }

        delete[] configs;


    //surface = eglCreateWindowSurface( display, config, engine->app->window, NULL );
    LOGI("6) Creating context\n");

    context = eglCreateContext( display, config, NULL, NULL );
    LOGI("7) Make current\n");

    if( eglMakeCurrent( display, surface, surface, context ) == EGL_FALSE )
    {
        LOGW("Unable to eglMakeCurrent");
        return -1;
    }
    LOGI("8) Query stats and set values\n");

    eglQuerySurface( display, surface, EGL_WIDTH, &w );
    eglQuerySurface( display, surface, EGL_HEIGHT, &h );

	if (w < h) {
		int tmp = w;
		w = h;
		h = tmp;
	}

    engine->display = engine->appContext.display = display;
    engine->context = engine->appContext.context = context;
    engine->surface = engine->appContext.surface = surface;
    engine->width   = w;
    engine->height  = h;
    //Minecraft::width = w;
    //Minecraft::height = h;
    LOGI("9) Set-up viewport\n");

    // Initialize GL state.
    glViewport( 0, 0, w, h );
    LOGI("X) Graphics set-up finished!\n");

    // Don't need to reload graphics first time
    if (engine->is_inited) {
        engine->userApp->onGraphicsReset(engine->appContext);
        engine->userApp->setSize(w, h);
    }

	eglSwapInterval(eglGetCurrentDisplay(), 2);
    engine->is_inited = true;

    return 0;
}

/**
 * Tear down the EGL context currently associated with the display.
 */
static
void
engine_term_display( struct ENGINE* engine )
{
    //LOGI( "engine_term_display" );

    if( engine->display != EGL_NO_DISPLAY )
    {
        eglMakeCurrent( engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );

        if( engine->context != EGL_NO_CONTEXT )
        {
            eglDestroyContext( engine->display, engine->context );
        }

        if( engine->surface != EGL_NO_SURFACE )
        {
            eglDestroySurface( engine->display, engine->surface );
        }

        eglTerminate( engine->display );
    }

    engine->display = EGL_NO_DISPLAY;
    engine->context = EGL_NO_CONTEXT;
    engine->surface = EGL_NO_SURFACE;
}

static int socketDesc;

static int broadcastData(int port, void* msg, int msgLen)
{
    struct sockaddr_in broadcastAddr;

    broadcastAddr.sin_family      = AF_INET;
    broadcastAddr.sin_port        = htons(port);
    broadcastAddr.sin_addr.s_addr = inet_addr("192.168.0.255");

    return sendto(socketDesc, msg, msgLen, 0, (struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr));
}

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

static void pointerDown(int pointerId, int x, int y) {
    Multitouch::feed(1, 1, x, y, pointerId);
}
static void pointerUp(int pointerId, int x, int y) {
    Multitouch::feed(1, 0, x, y, pointerId);
}
static void pointerMove(int pointerId, int x, int y) {
    Multitouch::feed(0, 0, x, y, pointerId);
}

__inline static const float padXtoSigned(int x) {
    return (x - 483.0f) * 0.002070393374741201f;
}
__inline static const float padYtoSigned(int y) {
    return (y - 180.0f) * 0.005555555555555556f;
}

static void trackpadPress(int x, int y) {
    Controller::feed(1, Controller::STATE_TOUCH, padXtoSigned(x), padYtoSigned(y));
}
static void trackpadMove(int x, int y) {
    Controller::feed(1, Controller::STATE_MOVE, padXtoSigned(x), padYtoSigned(y));
}
static void trackpadRelease(int x, int y) {
    Controller::feed(1, Controller::STATE_RELEASE, padXtoSigned(x), padYtoSigned(y));
}

static int isTouchpadTouched = 0;

static
int32_t
handle_xperia_input( struct android_app* app, AInputEvent* event )
{
    if( AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION )
    {
        struct ENGINE* engine = (struct ENGINE*)app->userData;
        int nPointerCount   = AMotionEvent_getPointerCount( event );
        int nSourceId       = AInputEvent_getSource( event );
        int nAction     = AMOTION_EVENT_ACTION_MASK & AMotionEvent_getAction( event );
        int isTrackpad  =(nSourceId == AINPUT_SOURCE_TOUCHPAD);
        int n;

        for( n = 0 ; n < nPointerCount ; ++n )
        {
            int nPointerId  = AMotionEvent_getPointerId( event, n );

            // We don't care about secondary pointers right now
            if( !isTrackpad ) {
                if (nAction == AMOTION_EVENT_ACTION_POINTER_DOWN || nAction == AMOTION_EVENT_ACTION_POINTER_UP )
                    continue;
            }

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
}
static int convertAndroidKeyCodeToWindowsKeyCode(int keyCode) {
	switch(keyCode) {
        /*
	case 29: return Keyboard::KEY_A;
		case 30: return Keyboard::KEY_B;
		case 31: return Keyboard::KEY_C;
		case 32: return Keyboard::KEY_D;
		case 33: return Keyboard::KEY_E;
		case 34: return Keyboard::KEY_F;
		case 35: return Keyboard::KEY_G;
		case 36: return Keyboard::KEY_H;
		case 37: return Keyboard::KEY_I;
		case 38: return Keyboard::KEY_J;
		case 39: return Keyboard::KEY_K;
		case 40: return Keyboard::KEY_L;
		case 41: return Keyboard::KEY_M;
		case 42: return Keyboard::KEY_N;
		case 43: return Keyboard::KEY_O;
		case 44: return Keyboard::KEY_P;
		case 45: return Keyboard::KEY_Q;
		case 46: return Keyboard::KEY_R;
		case 47: return Keyboard::KEY_S;
		case 48: return Keyboard::KEY_T;
		case 49: return Keyboard::KEY_U;
		case 50: return Keyboard::KEY_V;
		case 51: return Keyboard::KEY_W;
		case 52: return Keyboard::KEY_X;
		case 53: return Keyboard::KEY_Y;
		case 54: return Keyboard::KEY_Z;
		*/
		case 67: return Keyboard::KEY_BACKSPACE;
		case 66: return Keyboard::KEY_RETURN;
//		case 62: return Keyboard::KEY_SPACE;
		default: return 0;
	}
}
static
int32_t
handle_keyboard_input( struct android_app* app, AInputEvent* event )
{
    if( AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY )
    {
       struct ENGINE* engine = (struct ENGINE*)app->userData;
       bool isDown = AKeyEvent_getAction(event) == AKEY_EVENT_ACTION_DOWN;
       int keyCode = AKeyEvent_getKeyCode(event);
	   int metaState = AKeyEvent_getMetaState(event);
	   int deviceId = AInputEvent_getDeviceId(event);
       bool pressedBack = (keyCode == 4) && ((AKeyEvent_getMetaState(event) & AMETA_ALT_ON) == 0);
       bool isRepeat = AKeyEvent_getRepeatCount(event) > 0;
       if (!isRepeat){
           //int scanCode = AKeyEvent_getScanCode(event);
           //LOGI("key-%s : %d", isDown?"key-down":"key-up", keyCode);
           //LOGI("getId: %d\n", AInputEvent_getDeviceId(event));
           //LOGI("flags: %d\n", AKeyEvent_getFlags(event));
           //LOGI("scancode: %d\n", AKeyEvent_getScanCode(event));

           //unsigned char msg[2] = {(unsigned char)(keyCode), (unsigned char)(isDown)};
           //broadcastData(BroadcastPort, msg, 2);
		   LOGW("New keycode: %d", keyCode);
		   if (!pressedBack) {

               int key = convertAndroidKeyCodeToWindowsKeyCode(keyCode);
               if (key == 0) key = keyCode;
			   Keyboard::feed(key, isDown);
			   if(isDown) {
				   int uniChar = ((struct ENGINE*)app->userData)->appContext.platform->getKeyFromKeyCode(keyCode, metaState, deviceId);
				   char charI = (char)uniChar;
				   LOGW("Unichar: %d", uniChar);
				   if(charI > 31) {
					   LOGW("New Char %c[%d]", charI, uniChar);
					   Keyboard::feedText((char)charI);
				   }
			   }
		   }
       }
       if (pressedBack)
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

static
int32_t
handle_touch_input( struct android_app* app, AInputEvent* event )
{
    if( AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION )
        return 0;

    int nSourceId   = AInputEvent_getSource( event );
	if (nSourceId == AINPUT_SOURCE_TOUCHPAD)
		return 0;

    struct ENGINE* engine = (struct ENGINE*)app->userData;

    int fullAction  = AMotionEvent_getAction( event );
    int nAction     = AMOTION_EVENT_ACTION_MASK & fullAction;
    int pointerIndex = (fullAction & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
    int pointerId = AMotionEvent_getPointerId(event, pointerIndex);
    int x = (int)AMotionEvent_getX(event, pointerIndex);
    int y = (int)AMotionEvent_getY(event, pointerIndex);

	//LOGI("xy:\t%d, %d\n", x, y);

    switch (nAction) {
        case AMOTION_EVENT_ACTION_POINTER_DOWN:
        case AMOTION_EVENT_ACTION_DOWN:
            mouseDown(1, x, y);
            Multitouch::feed(1, 1, x, y, pointerId);
            break;
        case AMOTION_EVENT_ACTION_POINTER_UP:
        case AMOTION_EVENT_ACTION_UP:
            mouseUp(1, x, y);
            Multitouch::feed(1, 0, x, y, pointerId);
            break;
        case AMOTION_EVENT_ACTION_MOVE:
    		int pcount = AMotionEvent_getPointerCount(event);
    		for (int i = 0; i < pcount; ++i) {
    			int pp = AMotionEvent_getPointerId(event, i); // @attn wtf?
    			float xx = AMotionEvent_getX(event, i);
    			float yy = AMotionEvent_getY(event, i);
        		//	System.err.println("   " + p + " @ " + x + ", " + y);
                //LOGI("> %.2f, %.2f\n", xx, yy);
                mouseMove(xx, yy);
                Multitouch::feed(0, 0, xx, yy, pp);
    		}
            break;
    }
    return 0;
}

/**
 * Process the next input event.
 */
static
int32_t
engine_handle_input( struct android_app* app, AInputEvent* event )
{
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY)
        return handle_keyboard_input(app, event);

    const bool isTouchscreen = ((struct ENGINE*)app->userData)->appContext.platform->supportsTouchscreen();;
    if (isTouchscreen) {
        return handle_touch_input(app, event);
    } else {
        // We only have xperia by now, but in the best of worlds
        // one function will feed all controllers
        return handle_xperia_input(app, event);
    }
}

/**
 * Process the next main command.
 */
static
void
engine_handle_cmd( struct android_app* app, int32_t cmd )
{
    struct ENGINE* engine = (struct ENGINE*)app->userData;
    switch( cmd )
    {
        case APP_CMD_SAVE_STATE:
        {
            //LOGI("save-state\n");
            // The system has asked us to save our current state.  Do so if needed
            
            // query first
            void* savedState = 0;
            int savedStateSize = 0;
            engine->userApp->saveState(&savedState, &savedStateSize);

            // Seems OK - save
            if (savedStateSize > 0) {
                app->savedState = savedState;
                app->savedStateSize = savedStateSize;
            }
        }
        break;

        case APP_CMD_INIT_WINDOW:
            //LOGI("init-window\n");
            // The window is being shown, get it ready.
            if( engine->app->window != NULL )
            {
                engine_init_display( engine );
                //engine->has_focus = 1;
               // LOGI("has-inited-display\n");
                //ANativeWindow_setBuffersGeometry(engine->app->window, engine->width, engine->height, 1);
            }
            break;

        case APP_CMD_TERM_WINDOW:
            //LOGI("term-window\n");
            // The window is being hidden or closed, clean it up.
            engine_term_display( engine );
            break;

        case APP_CMD_GAINED_FOCUS:
            //LOGI("gained-focus\n");
            //engine->has_focus = 1;
            break;

        case APP_CMD_LOST_FOCUS:
            //engine->has_focus = 0;
            //engine_draw_frame( engine );
            break;

        case APP_CMD_PAUSE:
            engine->has_focus = 0;
            break;
        case APP_CMD_RESUME:
            engine->has_focus = 1;
            break;
        default:
            //LOGI("unknown: %d\n", cmd);
            break;
    }
}

static int initSocket(int port)
{
    int s = socket(PF_INET, SOCK_DGRAM, 0);

    struct addrinfo hints;
    struct addrinfo *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo("192.168.0.110", "1999", &hints, &res); // ...

    int b = bind(s, res->ai_addr, res->ai_addrlen);

    int broadcast = 1;
    if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &broadcast,
        sizeof(broadcast)) == -1) {
        perror("setsockopt (SO_BROADCAST)");
        return 0;
    }

    return s;
}

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things (rendering).
 */
void
android_main( struct android_app* state );
