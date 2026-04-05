#ifndef NET_MINECRAFT_CLIENT_PLAYER__KeyboardInput_H__
#define NET_MINECRAFT_CLIENT_PLAYER__KeyboardInput_H__

//package net.minecraft.client.player;

#include "IMoveInput.h"

class Options;
class Player;

// @todo: extract a separate MoveInput (-> merge XperiaPlayInput)
class KeyboardInput: public IMoveInput
{
	static const int NumKeys = 10;
public:
    static const int KEY_UP = 0;
    static const int KEY_DOWN = 1;
    static const int KEY_LEFT = 2;
    static const int KEY_RIGHT = 3;
    static const int KEY_JUMP = 4;
    static const int KEY_SNEAK = 5;
	static const int KEY_CRAFT = 6;

    KeyboardInput(Options* options);

	void tick(Player* player);

	void setKey(int key, bool state);
    void releaseAllKeys();

protected:
	bool keys[NumKeys];
	Options* options;
};

#endif /*NET_MINECRAFT_CLIENT_PLAYER__KeyboardInput_H__*/
