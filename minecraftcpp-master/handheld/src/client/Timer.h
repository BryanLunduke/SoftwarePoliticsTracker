#ifndef NET_MINECRAFT_CLIENT__Timer_H__
#define NET_MINECRAFT_CLIENT__Timer_H__

//package net.minecraft.client;
#include "../platform/time.h"

class Timer
{
public:
	Timer(float ticksPerSecond)
	:	ticksPerSecond(ticksPerSecond),
		adjustTime(1.0f),
		timeScale(1.0f),
		passedTime(0)
	{
        lastMs = getTimeMs();
        lastMsSysTime = lastMs;
        lastTime = lastMs / 1000.0f;
    }

    void advanceTime() {
        long nowMs = getTimeMs();
        long passedMs = nowMs - lastMs;
        long msSysTime = nowMs;//System.nanoTime() / 1000000;

        if (passedMs > 1000) {
            long passedMsSysTime = msSysTime - lastMsSysTime;
            if (passedMsSysTime == 0)
                passedMs = passedMsSysTime = 1;

            float adjustTimeT = passedMs / (float) passedMsSysTime;
            adjustTime += (adjustTimeT - adjustTime) * 0.2f;

            lastMs = nowMs;
            lastMsSysTime = msSysTime;
        }
        if (passedMs < 0) {
            lastMs = nowMs;
            lastMsSysTime = msSysTime;
        }

		float now = msSysTime / 1000.0f;
        float passedSeconds = (now - lastTime) * adjustTime;
        lastTime = now;

        if (passedSeconds < 0) passedSeconds = 0;
        if (passedSeconds > 1) passedSeconds = 1;
        //LOGI("passed s: %f\n", passedSeconds);

        passedTime += passedSeconds * timeScale * ticksPerSecond;

        ticks = (int) passedTime;
        passedTime -= ticks;
        if (ticks > MAX_TICKS_PER_UPDATE) ticks = MAX_TICKS_PER_UPDATE;
        a = passedTime;
    }

    /**
     * Advances time the max number of ticks per second.
     */
    void advanceTimeQuickly() {

        float passedSeconds = (float) MAX_TICKS_PER_UPDATE / (float) ticksPerSecond;

        passedTime += passedSeconds * timeScale * ticksPerSecond;
        ticks = (int) passedTime;
        passedTime -= ticks;
        a = passedTime;

        lastMs = getTimeMs();//System.currentTimeMillis();
        lastMsSysTime = lastMs;
    }

    void skipTime() {
        long nowMs = getTimeMs();//System.currentTimeMillis();
        long passedMs = nowMs - lastMs;
        long msSysTime = nowMs;//System.nanoTime() / 1000000;

        if (passedMs > 1000) {
            long passedMsSysTime = msSysTime - lastMsSysTime;
            if (passedMsSysTime == 0)
                passedMs = passedMsSysTime = 1;

            float adjustTimeT = passedMs / (float) passedMsSysTime;
            adjustTime += (adjustTimeT - adjustTime) * 0.2f;

            lastMs = nowMs;
            lastMsSysTime = msSysTime;
        }
        if (passedMs < 0) {
            lastMs = nowMs;
            lastMsSysTime = msSysTime;
        }

		float now = msSysTime / 1000.0f;
        float passedSeconds = (now - lastTime) * adjustTime;
        lastTime = now;

        if (passedSeconds < 0) passedSeconds = 0;
        if (passedSeconds > 1) passedSeconds = 1;

        passedTime += passedSeconds * timeScale * ticksPerSecond;

        ticks = (int) 0;
        if (ticks > MAX_TICKS_PER_UPDATE) ticks = MAX_TICKS_PER_UPDATE;
        passedTime -= ticks;
    }

public:
	float ticksPerSecond;
	int ticks;
	float a;
	float timeScale;
	float passedTime;
private:
	static const int MAX_TICKS_PER_UPDATE = 10;

	float lastTime;
	long lastMs;
	long lastMsSysTime;
	float adjustTime;
};

#endif /*NET_MINECRAFT_CLIENT__Timer_H__*/
