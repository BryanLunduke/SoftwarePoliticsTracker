#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__NetherReactor_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__NetherReactor_H__

#include "EntityTile.h"

class Material;
class NetherReactor : public EntityTile {
	typedef EntityTile super;
public:
	int getTexture(int face, int data);
	NetherReactor(int id, int tex, const Material* material);
	bool use(Level* level, int x, int y, int z, Player* player);
	static void setPhase(Level* level, int x, int y, int z, int phase);
	TileEntity* newTileEntity();
	bool canSpawnStartNetherReactor( Level* level, int x, int y, int z, Player* player );
	bool allPlayersCloseToReactor( Level* level, int x, int y, int z );
};

#endif /* NET_MINECRAFT_WORLD_LEVEL_TILE__NetherReactor_H__ */
