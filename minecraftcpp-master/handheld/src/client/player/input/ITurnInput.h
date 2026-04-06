#ifndef NET_MINECRAFT_CLIENT_PLAYER__ITurnInput_H__
#define NET_MINECRAFT_CLIENT_PLAYER__ITurnInput_H__

#include "../../../platform/time.h"
#include "../../IConfigListener.h"
#include <cmath>

/** Representing a Turn delta value */
class TurnDelta {
public:
	TurnDelta(float dx, float dy)
	:	x(dx),
		y(dy)
	{}

	float x, y;
};

/** Base class for classes providing TurnDeltas */
class ITurnInput: public IConfigListener {
public:
	virtual ~ITurnInput() {}
	virtual void onConfigChanged(const Config& c) {}

	virtual TurnDelta getTurnDelta() = 0;
protected:
	ITurnInput()
	:	_lastTime(-1.0f)
	{}

	float getDeltaTime(){
		if (_lastTime == -1.0f) {
			_lastTime = getTimeS();
		}
		float now = getTimeS();
		float dt =  now - _lastTime;
		_lastTime = now;
		return dt;
	}

	static float linearTransform(float value, float deadZone, float scale=1.0f, bool limit1=false) {
		float deadSigned = value >= 0? deadZone : -deadZone;
		if (std::abs(deadSigned) >= std::abs(value)) return 0;
		float ret = (value - deadSigned) * scale;
		if (limit1 && std::abs(ret) > 1) ret /= std::abs(ret);
		return ret;
	}
private:
	float _lastTime;
};

#endif /*NET_MINECRAFT_CLIENT_PLAYER__ITurnInput_H__*/
