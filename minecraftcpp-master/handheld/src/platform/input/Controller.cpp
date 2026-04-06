#include "Controller.h"

static int   _abs(int x)   { return x>=0? x:-x; }
static float _abs(float x) { return x>=0? x:-x; }

/*static*/ float Controller::stickValuesX[NUM_STICKS] = {0};
/*static*/ float Controller::stickValuesY[NUM_STICKS] = {0};
/*static*/ bool Controller::isTouchedValues[NUM_STICKS] = {0};

bool Controller::isTouched( int stickIndex )
{
	if (!isValidStick(stickIndex)) return false;

	return isTouchedValues[stickIndex-1];
}

void Controller::feed( int stickIndex, int state, float dx, float dy )
{
	if (!isValidStick(stickIndex)) return;

	if (NUM_STICKS == 2)
		stickIndex = dx<0? 1 : 2;

	isTouchedValues[stickIndex-1] = (state != STATE_RELEASE);

	// @note: Since I don't know where to put the Xperia Play specific
	// calculations, I put them here! (normally I would probably have
	// some kind of (XperiaPlay)ControllerReader but it doesn't make much
	// more sense as long as we cant figure out (in code) whether or not
	// we actually use an Xperia -> hardcode it here (Note#2, we CAN figure
	// figure this out, at least by JNI/java-call but we arent doing it)
	static float offsets[3] = {0, 0.64f, -0.64f};
	dx =  linearTransform(dx + offsets[stickIndex], 0, 2.78f, true);

	stickValuesX[stickIndex-1] = dx;
	stickValuesY[stickIndex-1] = dy;
}

float Controller::getX( int stickIndex )
{
	if (!isValidStick(stickIndex)) return 0;
	return stickValuesX[stickIndex-1];
}

float Controller::getY( int stickIndex )
{
	if (!isValidStick(stickIndex)) return 0;
	return stickValuesY[stickIndex-1];
}

float Controller::getTransformedX( int stickIndex, float deadZone, float scale/*=1.0f*/, bool limit1/*=false*/ )
{
	if (!isValidStick(stickIndex)) return 0;
	return linearTransform(stickValuesX[stickIndex-1], deadZone, scale, limit1);
}

float Controller::getTransformedY( int stickIndex, float deadZone, float scale/*=1.0f*/, bool limit1/*=false*/ )
{
	if (!isValidStick(stickIndex)) return 0;
	return linearTransform(stickValuesY[stickIndex-1], deadZone, scale, limit1);
}

float Controller::linearTransform( float value, float deadZone, float scale/*=1.0f*/, bool limit1/*=false*/ )
{
	float deadSigned = value >= 0? deadZone : -deadZone;
	if (_abs(deadSigned) >= _abs(value)) return 0;
	float ret = (value - deadSigned) * scale;
	if (limit1 && _abs(ret) > 1) ret = ret>0.0f? 1.0f : -1.0f;
	return ret;
}

/*static*/
bool Controller::isValidStick(int stick) {
	return stick > 0 && stick <= NUM_STICKS;
}
