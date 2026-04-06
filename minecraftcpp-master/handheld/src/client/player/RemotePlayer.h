#ifndef NET_MINECRAFT_CLIENT_PLAYER__RemotePlayer_H__
#define NET_MINECRAFT_CLIENT_PLAYER__RemotePlayer_H__

//package net.minecraft.client.player;

#include "../../world/entity/player/Player.h"

class Level;

class RemotePlayer: public Player
{
	typedef Player super;
public:
	RemotePlayer(Level* level, bool isCreative);
	void aiStep();
	void tick();
private:
	bool hasStartedUsingItem;
};

#endif /*NET_MINECRAFT_CLIENT_PLAYER__RemotePlayer_H__*/
