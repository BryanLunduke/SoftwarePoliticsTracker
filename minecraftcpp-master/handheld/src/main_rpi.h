#ifndef MAIN_RPI_H__
#define MAIN_RPI_H__

#include <cassert>

#include "bcm_host.h"

//#include "GLES/gl.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"

#include <fstream>
#include <png.h>

#include <SDL/SDL.h>

#define check() assert(glGetError() == 0)

#include "App.h"
#include "platform/input/Mouse.h"
#include "platform/input/Multitouch.h"
#include "platform/input/Keyboard.h"

int width = 848;
int height = 480;

static void png_funcReadFile(png_structp pngPtr, png_bytep data, png_size_t length) {
	((std::istream*)png_get_io_ptr(pngPtr))->read((char*)data, length);
}

class AppPlatform_linux: public AppPlatform
{
public:
    bool isTouchscreen()  { return false; }

    TextureData loadTexture(const std::string& filename_, bool textureFolder)
    {
	TextureData out;

	std::string filename = textureFolder? "data/images/" + filename_
										: filename_;
	std::ifstream source(filename.c_str(), std::ios::binary);

	if (source) {
		png_structp pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

		if (!pngPtr)
			return out;

		png_infop infoPtr = png_create_info_struct(pngPtr);

		if (!infoPtr) {
			png_destroy_read_struct(&pngPtr, NULL, NULL);
			return out;
		}

		// Hack to get around the broken libpng for windows
		png_set_read_fn(pngPtr,(voidp)&source, png_funcReadFile);

		png_read_info(pngPtr, infoPtr);

		// Set up the texdata properties
		out.w = png_get_image_width(pngPtr, infoPtr);
		out.h = png_get_image_height(pngPtr, infoPtr);

		png_bytep* rowPtrs = new png_bytep[out.h];
		out.data = new unsigned char[4 * out.w * out.h];
		out.memoryHandledExternally = false;

		int rowStrideBytes = 4 * out.w;
		for (int i = 0; i < out.h; i++) {
			rowPtrs[i] = (png_bytep)&out.data[i*rowStrideBytes];
		}
		png_read_image(pngPtr, rowPtrs);

		// Teardown and return
		png_destroy_read_struct(&pngPtr, &infoPtr,(png_infopp)0);
		delete[] (png_bytep)rowPtrs;
		source.close();

		return out;
	}
	else
	{
		LOGI("Couldn't find file: %s\n", filename.c_str());
		return out;
	}
    }
};

static bool _inited_egl = false;
static bool _app_inited = false;
static int  _app_window_normal = true;

static void move_surface(App* app, AppContext* state, uint32_t x, uint32_t y, uint32_t w, uint32_t h);

static void initEgl(App* app, AppContext* state, uint32_t w, uint32_t h)
{
	move_surface(app, state, 16, 16, w, h);
}

static void deinitEgl(AppContext* state) {
	//printf("deinitEgl: (inited: %d)\n", _inited_egl);
	if (!_inited_egl) {
		return;
	}

	eglSwapBuffers(state->display, state->surface);
	eglMakeCurrent(state->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroySurface(state->display, state->surface);
	eglDestroySurface(state->display, state->context);
	eglTerminate(state->display);

	//state->doRender = false;
	_inited_egl = false;
}

void move_surface(App* app, AppContext* state, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
   int32_t success = 0;
   EGLBoolean result;
   EGLint num_config;

   deinitEgl(state);
   //printf("initEgl\n");

   static EGL_DISPMANX_WINDOW_T nativewindow;

   DISPMANX_ELEMENT_HANDLE_T dispman_element;
   DISPMANX_DISPLAY_HANDLE_T dispman_display;
   DISPMANX_UPDATE_HANDLE_T dispman_update;
   VC_RECT_T dst_rect;
   VC_RECT_T src_rect;

   static const EGLint attribute_list[] =
   {
      EGL_RED_SIZE, 8,
      EGL_GREEN_SIZE, 8,
      EGL_BLUE_SIZE, 8,
      EGL_ALPHA_SIZE, 8,
      EGL_DEPTH_SIZE, 16,
      EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
      EGL_NONE
   };
   
   static const EGLint context_attributes[] = 
   {
      EGL_CONTEXT_CLIENT_VERSION, 1,
      EGL_NONE
   };
   EGLConfig config;

   // get an EGL display connection
   state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
   assert(state->display!=EGL_NO_DISPLAY);
   check();

   // initialize the EGL display connection
   result = eglInitialize(state->display, NULL, NULL);
   assert(EGL_FALSE != result);
   check();

   // get an appropriate EGL frame buffer configuration
   result = eglChooseConfig(state->display, attribute_list, &config, 1, &num_config);
   assert(EGL_FALSE != result);
   check();

   // get an appropriate EGL frame buffer configuration
   result = eglBindAPI(EGL_OPENGL_ES_API);
   assert(EGL_FALSE != result);
   check();

   // create an EGL rendering context
   state->context = eglCreateContext(state->display, config, EGL_NO_CONTEXT, context_attributes);
   assert(state->context!=EGL_NO_CONTEXT);
   check();

   // create an EGL window surface
   uint32_t dw, dh;
   success = graphics_get_display_size(0 /* LCD */, &dw, &dh);
   assert( success >= 0 );

   dst_rect.x = x;
   dst_rect.y = y;
   dst_rect.width = w;
   dst_rect.height = h;
      
   src_rect.x = 0;
   src_rect.y = 0;
   src_rect.width = w << 16;
   src_rect.height = h << 16;

   dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
   dispman_update = vc_dispmanx_update_start( 0 );
         
   dispman_element = vc_dispmanx_element_add ( dispman_update, dispman_display,
      0/*layer*/, &dst_rect, 0/*src*/,
      &src_rect, DISPMANX_PROTECTION_NONE, 0 /*alpha*/, 0/*clamp*/, (DISPMANX_TRANSFORM_T)0/*transform*/);
      
   nativewindow.element = dispman_element;
   nativewindow.width = w;//state->screen_width;
   nativewindow.height = h;//state->screen_height;
   vc_dispmanx_update_submit_sync( dispman_update );
      
   check();

   state->surface = eglCreateWindowSurface( state->display, config, &nativewindow, NULL );
   assert(state->surface != EGL_NO_SURFACE);
   check();

   // connect the context to the surface
   result = eglMakeCurrent(state->display, state->surface, state->surface, state->context);
   assert(EGL_FALSE != result);
   check();
   
   _inited_egl = true;
   
   if (!_app_inited) {
   	_app_inited = true;
   	app->init(*state);
   } else {
   	app->onGraphicsReset(*state);
   }
   app->setSize(w, h);
}

void teardown() {
	SDL_Quit();
	bcm_host_deinit();
}

/*
static bool isGrabbed = false;
static void setGrabbed(bool status) {
	SDL_WM_GrabInput(status? SDL_GRAB_ON : SDL_GRAB_OFF);
	SDL_ShowCursor  (status? 0 : 1);
	isGrabbed = status;
	printf("set grabbed: %d\n", isGrabbed);
}
*/

static unsigned char transformKey(int key) {
	// Handle ALL keys here. If not handled -> return 0 ("invalid")
	if (key == SDLK_LSHIFT) return Keyboard::KEY_LSHIFT;
	if (key == SDLK_DOWN) return 40;
	if (key == SDLK_UP)   return 38;
	if (key == SDLK_SPACE) return Keyboard::KEY_SPACE;
	if (key == SDLK_RETURN) return 13;//Keyboard::KEY_RETURN;
	if (key == SDLK_ESCAPE) return Keyboard::KEY_ESCAPE;
	if (key == SDLK_TAB) return 250;
	if (key >= 'a' && key <= 'z') return key - 32;
	if (key >= SDLK_0 && key <= SDLK_9) return '0' + (key - SDLK_0);
	return 0;
}

int handleEvents() {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (SDL_QUIT == event.type) {
			return -1;
		}
		if (SDL_KEYDOWN == event.type) {
			int key = event.key.keysym.sym;
			unsigned char transformed = transformKey(key);
/*
			if (SDLK_ESCAPE == key) {
				if (SDL_GRAB_ON == SDL_WM_GrabInput(SDL_GRAB_QUERY)) {
					setGrabbed(false);
				}
			}
*/
			//printf("KeyDown: %d => %d\n", key, transformed);
			if (transformed) Keyboard::feed(transformed, 1);
		}
		if (SDL_KEYUP == event.type) {
			int key = event.key.keysym.sym;
			unsigned char transformed = transformKey(key);
			if (transformed) Keyboard::feed(transformed, 0);
		}
		if (SDL_MOUSEBUTTONDOWN == event.type) {
			if (SDL_BUTTON_WHEELUP == event.button.button) {
				Mouse::feed(3, 0, event.button.x, event.button.y, 0, 1);
			} else
			if (SDL_BUTTON_WHEELDOWN == event.button.button) {
				Mouse::feed(3, 0, event.button.x, event.button.y, 0, -1);
			} else {
				bool left = SDL_BUTTON_LEFT == event.button.button;
				char button = left? 1 : 2;
				Mouse::feed(button, 1, event.button.x, event.button.y);
				Multitouch::feed(button, 1, event.button.x, event.button.y, 0);

				//if (!isGrabbed) setGrabbed(true);
			}
		}
		if (SDL_MOUSEBUTTONUP == event.type) {
			bool left = SDL_BUTTON_LEFT == event.button.button;
			char button = left? 1 : 2;//MouseAction::ACTION_LEFT : MouseAction::ACTION_RIGHT;
			Mouse::feed(button, 0, event.button.x, event.button.y);
			Multitouch::feed(button, 0, event.button.x, event.button.y, 0);

			//if (!isGrabbed) setGrabbed(true);
		}
		if (SDL_MOUSEMOTION == event.type) {
			//printf("mouse: %d, %d\n",  event.motion.xrel, event.motion.yrel);
			float x = event.motion.x;
			float y = event.motion.y;
			Multitouch::feed(0, 0, x, y, 0);

			if (1 /*|| isGrabbed*/) {
				Mouse::feed(0, 0, x, y, event.motion.xrel, event.motion.yrel);
			} else {
				Mouse::feed(0, 0, x, y, 0, 0);
			}
		}
		if  (SDL_ACTIVEEVENT == event.type) {
			if (SDL_APPACTIVE & event.active.state) _app_window_normal = event.active.gain;
			//printf("state: %d (%d), %d, %d, %d\n", event.active.state, event.active.gain, SDL_APPACTIVE, SDL_APPMOUSEFOCUS, SDL_APPINPUTFOCUS);
		}
/*
		if (SDL_VIDEORESIZE == event.type) {
			width =  event.resize.w;
			height = event.resize.h;
		}
*/
	}
	return 0;
}

void getWindowPosition(int* x, int* y, int* w, int* h);

void updateWindowPosition(App* app, AppContext* state) {
	static int lx = -999, ly = -999;
	static int lw = -999, lh = -999;
	static int resetTicks = 0;
	static bool lastWindowNormal = true;
	static bool allowCreate = true;
	int x, y;
	getWindowPosition(&x, &y, &width, &height);

	if (x != lx || y != ly || width != lw || height != lh) {
		lx = x;
		ly = y;
		lw = width;
		lh = height;
		resetTicks = 8;
		deinitEgl(state);
		//printf("move surface! (%d)\n", _inited_egl );
	}
	if (--resetTicks == 0 && allowCreate) {
		move_surface(app, state, x, y, width, height);
	}

	if (lastWindowNormal != _app_window_normal) {
		lastWindowNormal = _app_window_normal;
		allowCreate = lastWindowNormal;
		//printf("lastWindowNormal: %d\n", lastWindowNormal);

		if (lastWindowNormal) {
			//printf("x, y, width, height: %d, %d, %d, %d\n", x, y, width, height);
			move_surface(app, state, x, y, width, height);
			//resetTicks = 8;
		} else {
			deinitEgl(state);
			lx = ly = -10000;
		}
//			move_surface(app, state, 0, 0, 0, 0);
	}
}

int main(int argc, char** argv) {
	bcm_host_init();

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("Couldn't initialize SDL\n");
		return -1;
	}

	SDL_Surface* sdlSurface = SDL_SetVideoMode(width,height,32,SDL_SWSURFACE|SDL_RESIZABLE);
	if (sdlSurface == NULL) {
		printf("Couldn't create SDL window\n");
		return -2;
	}

	//printf("argv[0]: %s\n", argv[0]);

	std::string path = argv[0];
	int e = path.rfind('/');
	if (e != std::string::npos) {
		path = path.substr(0, e);
		chdir(path.c_str());
	}

	char buf[1024];
	getcwd(buf, 1000);
	//printf("getcwd: %s\n", buf);

	//printf("HOME: %s\n", getenv("HOME"));

	atexit(teardown);
	SDL_WM_SetCaption("Minecraft - Pi edition", 0);
	//setGrabbed(false);;

	MAIN_CLASS* app = new MAIN_CLASS();
	std::string storagePath = getenv("HOME");
	storagePath += "/.minecraft/";
	app->externalStoragePath = storagePath;
	app->externalCacheStoragePath = storagePath;

	int commandPort = 0;
	if (argc > 1) {
		commandPort = atoi(argv[1]);
	}

	if (commandPort != 0)
		app->commandPort = commandPort;

	//printf("storage: %s\n", app->externalStoragePath.c_str());

	AppContext context;
	AppPlatform_linux platform;
	context.doRender = true;
	context.platform = &platform;

	initEgl(app, &context, width, height);

	bool running = true;
	while (running) {

		updateWindowPosition(app, &context);

		running = handleEvents() == 0;
		app->update();
	}

	deinitEgl(&context);

	return 0;
}

#endif
