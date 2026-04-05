#ifndef NET_MINECRAFT_CLIENT_GAMEMODE__SurvivalMode_H__
#define NET_MINECRAFT_CLIENT_GAMEMODE__SurvivalMode_H__

#include "GameMode.h"

class Abilities;
class Minecraft;

class SurvivalMode: public GameMode
{
	typedef GameMode super;
public:
	SurvivalMode(Minecraft* minecraft);

	bool destroyBlock(int x, int y, int z, int face);
	void startDestroyBlock(int x, int y, int z, int face);
	void continueDestroyBlock(int x, int y, int z, int face);
	void stopDestroyBlock();

	bool canHurtPlayer() { return true; }

	bool isSurvivalType() { return true; }

	void initAbilities( Abilities& abilities );
private:
	int xDestroyBlock;
	int yDestroyBlock;
	int zDestroyBlock;
};

#endif /*NET_MINECRAFT_CLIENT_GAMEMODE__SurvivalMode_H__*/
