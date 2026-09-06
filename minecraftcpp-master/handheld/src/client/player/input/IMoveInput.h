#ifndef NET_MINECRAFT_CLIENT_PLAYER__IMoveInput_H__
#define NET_MINECRAFT_CLIENT_PLAYER__IMoveInput_H__

//package net.minecraft.client.player;

#include "../../IConfigListener.h"
class Player;
class Minecraft;

class IMoveInput
{
protected:
	IMoveInput()
	:	xa(0),
		ya(0),
		wasJumping(false),
		jumping(false),
		sneaking(false),
		wantUp(false),
		wantDown(false),
		isChangingFlightHeight(false)
	{}
public:
	virtual ~IMoveInput() {}

    virtual void tick(Player* player) {}
	virtual void render(float a) {}
	
	virtual void setKey(int eventKey, bool eventKeyState) {}
    virtual void releaseAllKeys() {}

	virtual void onConfigChanged(const Config& c) {}

public:
	float xa;
	float ya;

	bool wasJumping;
	bool jumping;
	bool sneaking;

	bool wantUp;
	bool wantDown;
	bool isChangingFlightHeight;
};

#endif /*NET_MINECRAFT_CLIENT_PLAYER__IMoveInput_H__*/
