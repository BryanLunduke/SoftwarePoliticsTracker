//#include "main_rpi.h"

#include <SDL/SDL_syswm.h>

void getWindowPosition(int* x, int* y, int* width, int* height) {
	static SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWMInfo(&wmInfo);

	static XWindowAttributes attr;
	Display* display = wmInfo.info.x11.display;
	Window window = wmInfo.info.x11.wmwindow;
	Window wndvoid;

	wmInfo.info.x11.lock_func();
	int status = XGetWindowAttributes(display, window, &attr);
	*width = attr.width;
	*height = attr.height;
	XTranslateCoordinates(display, window, attr.root, 0, 0, x, y, &wndvoid);
	wmInfo.info.x11.unlock_func();
}
