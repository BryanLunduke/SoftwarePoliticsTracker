#include "AppPlatform_win32.h"
#include "util/Mth.h"

int AppPlatform_win32::getScreenWidth()  { return 854; }
int AppPlatform_win32::getScreenHeight() { return 480; }

float AppPlatform_win32::getPixelsPerMillimeter() {
	// assuming 24" @ 1920x1200
	const int w = 1920;
	const int h = 1200;
	const float pixels = Mth::sqrt(w*w + h*h);
	const float mm	   = 24 * 25.4f;
	return pixels / mm;
}

bool AppPlatform_win32::supportsTouchscreen()  { return true; }
bool AppPlatform_win32::hasBuyButtonWhenInvalidLicense() { return true; }
