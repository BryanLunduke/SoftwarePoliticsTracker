#ifndef NET_MINECRAFT_CLIENT_GAMEMODE__CreativeMode_H__
#define NET_MINECRAFT_CLIENT_GAMEMODE__CreativeMode_H__

//package net.minecraft.client.gamemode;

#include "GameMode.h"

class CreativeMode: public GameMode
{
	typedef GameMode super;
public:
    CreativeMode(Minecraft* minecraft);

    void startDestroyBlock(int x, int y, int z, int face);
    void continueDestroyBlock(int x, int y, int z, int face);
    void stopDestroyBlock();

	bool isCreativeType();

	void initAbilities(Abilities& abilities);

	void releaseUsingItem(Player* player);
private:
	void creativeDestroyBlock(int x, int y, int z, int face);
};

#endif /*NET_MINECRAFT_CLIENT_GAMEMODE__CreativeMode_H__*/
