#ifndef CONFIGLISTENER_H__
#define CONFIGLISTENER_H__

#include "PixelCalc.h"
class Minecraft;
class Options;

class Config {
public:
    // Screen dimensions and world-to-screen conversion
    void setScreenSize(int width, int height, float scale) {
        this->width = width;
        this->height = height;
        this->guiScale = scale;
        this->invGuiScale = 1.0f / scale;
        this->guiWidth = (int)(width * invGuiScale);
        this->guiHeight = (int)(height * invGuiScale);
    }

	int width;
	int height;

	float guiScale;
    float invGuiScale;
    int guiWidth;
    int guiHeight;

    PixelCalc pixelCalc;
    PixelCalc pixelCalcUi;

    Minecraft* minecraft;
    Options* options;
};

Config createConfig(Minecraft* mc);

// Interface for Configuration-Changed listener
// This can mean (for instance);
// - Screen has changed dimensions, or rotation if rotations are enabled
// - New input device or control mechanism
class IConfigListener
{
public:
    virtual ~IConfigListener() {}
    virtual void onConfigChanged(const Config& config) = 0;
};

#endif /*CONFIGLISTENER_H__*/
