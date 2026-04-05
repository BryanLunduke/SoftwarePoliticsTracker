#ifndef CONTROLLER_H__
#define CONTROLLER_H__

#include "../log.h"

class Controller
{
public:
	static const int NUM_STICKS = 2;
	static const int STATE_TOUCH	= 1;
	static const int STATE_RELEASE	= 0;
	static const int STATE_MOVE		=-1;

	static bool isTouched(int stickIndex);

	static void feed(int stickIndex, int state, float dx, float dy);

	static float getX(int stickIndex);
	static float getY(int stickIndex);

	static float getTransformedX(int stickIndex, float deadZone, float scale=1.0f, bool limit1=false);
	static float getTransformedY(int stickIndex, float deadZone, float scale=1.0f, bool limit1=false);

private:
	static bool isValidStick(int stick);
	static float linearTransform(float value, float deadZone, float scale=1.0f, bool limit1=false);

	static float stickValuesX[NUM_STICKS];
	static float stickValuesY[NUM_STICKS];
	static bool isTouchedValues[NUM_STICKS];
};

#endif /*CONTROLLER_H__*/
