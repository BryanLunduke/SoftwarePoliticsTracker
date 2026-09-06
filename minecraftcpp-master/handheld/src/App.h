#ifndef APP_H__
#define APP_H__

#ifdef __APPLE__
#define NO_EGL
#endif
#ifdef STANDALONE_SERVER
#define NO_EGL
#endif

#include "AppPlatform.h"
#ifndef NO_EGL 
    #include <EGL/egl.h>
#endif
#include "platform/log.h"

typedef struct AppContext {
#ifndef NO_EGL
	EGLDisplay display;
	EGLContext context;
	EGLSurface surface;
#endif
	AppPlatform* platform;
	bool doRender;
} AppContext;


class App
{
public:
    App()
	:	_finished(false),
		_inited(false)
	{
		_context.platform = 0;
	}
	virtual ~App() {}

	void init(AppContext& c) {
        _context = c;
		init();
		_inited = true;
    }
	bool isInited() { return _inited; }

	virtual AppPlatform* platform() { return _context.platform; }

	void onGraphicsReset(AppContext& c) {
		_context = c;
		onGraphicsReset();
	}

    virtual void audioEngineOn () {}
    virtual void audioEngineOff() {}
    
	virtual void destroy() {}

    virtual void loadState(void* state, int stateSize) {}
    virtual bool saveState(void** state, int* stateSize) { return false; }
    
	void swapBuffers() {
#ifndef NO_EGL
		if (_context.doRender)
			eglSwapBuffers(_context.display, _context.surface);
#endif
	}

	virtual void draw() {}
	virtual void update() {};// = 0;
	virtual void setSize(int width, int height) {}
	
	virtual void quit() { _finished = true; }
	virtual bool wantToQuit() { return _finished; }
	virtual bool handleBack(bool isDown) { return false; }

protected:
	virtual void init() {}
	//virtual void onGraphicsLost() = 0;
	virtual void onGraphicsReset() = 0;

private:
	bool _inited;
	bool _finished;
    AppContext _context;
};

#endif//APP_H__
