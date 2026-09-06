#ifndef NET_UTIL__PerfRenderer_H__
#define NET_UTIL__PerfRenderer_H__

#include <vector>
#include <string>
#include <sstream>

class Minecraft;
class Font;

//package net.minecraft.client;
class PerfRenderer
{
public:
    PerfRenderer( Minecraft* mc, Font* font);

    void debugFpsMeterKeyPress(int key);

    void renderFpsMeter(float tickTime);

private:
    std::string toPercentString(float percentage);

	Minecraft*	_mc;
    Font*		_font;
    std::string _debugPath;

	std::vector<float> frameTimes;
	std::vector<float> tickTimes;
	int frameTimePos;
	float lastTimer;
};

#endif /*NET_UTIL__PerfRenderer_H__*/
