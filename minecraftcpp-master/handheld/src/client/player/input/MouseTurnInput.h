#ifndef NET_MINECRAFT_CLIENT_PLAYER_INPUT_MouseTurnInput_H__
#define NET_MINECRAFT_CLIENT_PLAYER_INPUT_MouseTurnInput_H__

#include "ITurnInput.h"
#include "../../../platform/input/Mouse.h"

/** A Mouse Turn input */
class MouseTurnInput : public ITurnInput {
public:
	static const int MODE_OFFSET = 1;
	static const int MODE_DELTA  = 2;

	MouseTurnInput(int mode_, int centerX, int centerY)
	:	mode(mode_),
		_centerX((float)centerX),
		_centerY((float)centerY)
	{}

	void onConfigChanged(const Config& config) {
		_centerX = ((float) config.width ) * 0.5f;
		_centerY = ((float) config.height) * 0.5f;
	}

	TurnDelta getTurnDelta() {
		float dx = 0, dy = 0;
		if ( MODE_DELTA == mode ) {
			dx = (float)Mouse::getDX();
			dy = (float)Mouse::getDY();
		} else if (MODE_OFFSET == mode) {
			float dt = getDeltaTime();
			dx = linearTransform( (float)Mouse::getX() - _centerX, 40, 0.4f ) * dt;
			dy = linearTransform( (float)Mouse::getY() - _centerY, 30, 0.6f ) * dt;
		}
		return TurnDelta(dx, dy);
	}

	int mode;
private:
	float _centerX;
	float _centerY;
};

#endif /*NET_MINECRAFT_CLIENT_PLAYER_INPUT_MouseTurnInput_H__*/
