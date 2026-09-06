#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__Mushroom_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__Mushroom_H__

#include "Bush.h"

class Mushroom : public Bush
{
	typedef Bush super;
public:
	Mushroom(int id, int tex);

    void tick(Level* level, int x, int y, int z, Random* random);

    bool mayPlace(Level* level, int x, int y, int z, unsigned char face);
	bool mayPlaceOn(int tile);

    bool canSurvive(Level* level, int x, int y, int z);
};

#endif /* NET_MINECRAFT_WORLD_LEVEL_TILE__Mushroom_H__ */