#ifndef NET_MINECRAFT_CLIENT_PLAYER__XperiaPlayInput_H__
#define NET_MINECRAFT_CLIENT_PLAYER__XperiaPlayInput_H__

//package net.minecraft.client.player;

#include "KeyboardInput.h"

// @note: This is just copy-pasted from KeyboardInput right now.
class XperiaPlayInput: public KeyboardInput
{
	typedef KeyboardInput super;
public:
	XperiaPlayInput(Options* options)
	:	super(options),
		wasSneakTouched(false)
	{}

	void tick(Player* player) {
		bool _sneaking = sneaking;
		super::tick(player);
		sneaking = _sneaking;
		
		wantUp   = (jumping && keys[KEY_UP]);
		wantDown = (jumping && keys[KEY_DOWN]);
		if ((wantUp | wantDown) && (player && player->abilities.flying)) ya = 0;

		bool isSneakTouched = Controller::isTouched(1);
		if (isSneakTouched && !wasSneakTouched) {
			sneaking = !sneaking;
		}
		wasSneakTouched = isSneakTouched;

		if (sneaking) {
			xa *= 0.3f;
			ya *= 0.3f;
		}
	}

	/*
	if (Controller::isTouched(moveStick)) {
		xa = -Controller::getTransformedX(moveStick, 0.1f, 1.25f, true);
		ya = +Controller::getTransformedY(moveStick, 0.1f, 1.25f, true);
	}
	*/
private:
	bool wasSneakTouched;
};

#endif /*NET_MINECRAFT_CLIENT_PLAYER__XperiaPlayInput_H__*/
