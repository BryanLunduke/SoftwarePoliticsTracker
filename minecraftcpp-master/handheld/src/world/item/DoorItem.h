#ifndef NET_MINECRAFT_WORLD_ITEM__DoorItem_H__
#define NET_MINECRAFT_WORLD_ITEM__DoorItem_H__

//package net.minecraft.world.item;

#include "Item.h"
#include "ItemInstance.h"
#include "../Facing.h"
#include "../entity/player/Player.h"
#include "../level/Level.h"
#include "../level/material/Material.h"
#include "../level/tile/Tile.h"
#include "../../util/Mth.h"

class DoorItem: public Item
{
	typedef Item super;
    const Material* material;
public:
    DoorItem(int id, const Material* material)
    :   super(id),
        material(material)
    {
        maxDamage = 64;
        maxStackSize = 1;
    }

    bool useOn(ItemInstance* instance, Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ) {
		if (face != Facing::UP) return false;
		y++;

		Tile* tile;

		if (material == Material::wood) tile = Tile::door_wood;
		else tile = Tile::door_iron;
		//if (!player->mayUseItemAt(x, y, z, face, instance) || !player->mayUseItemAt(x, y + 1, z, face, instance)) return false;
		if (!tile->mayPlace(level, x, y, z)) return false;

		int dir = Mth::floor(((player->yRot + 180) * 4) / 360 - 0.5f) & 3;

		place(level, x, y, z, dir, tile);

		instance->count--;
		return true;
    }
	static void place(Level* level, int x, int y, int z, int dir, Tile* tile) {
		int xra = 0;
		int zra = 0;
		if (dir == 0) zra = +1;
		if (dir == 1) xra = -1;
		if (dir == 2) zra = -1;
		if (dir == 3) xra = +1;

		int solidLeft = (level->isSolidBlockingTile(x - xra, y, z - zra) ? 1 : 0) + (level->isSolidBlockingTile(x - xra, y + 1, z - zra) ? 1 : 0);
		int solidRight = (level->isSolidBlockingTile(x + xra, y, z + zra) ? 1 : 0) + (level->isSolidBlockingTile(x + xra, y + 1, z + zra) ? 1 : 0);

		bool doorLeft = (level->getTile(x - xra, y, z - zra) == tile->id) || (level->getTile(x - xra, y + 1, z - zra) == tile->id);
		bool doorRight = (level->getTile(x + xra, y, z + zra) == tile->id) || (level->getTile(x + xra, y + 1, z + zra) == tile->id);

		bool flip = false;
		if (doorLeft && !doorRight) flip = true;
		else if (solidRight > solidLeft) flip = true;

		level->noNeighborUpdate = true;
		level->setTileAndData(x, y, z, tile->id, dir);
		level->setTileAndData(x, y + 1, z, tile->id, 8 | (flip ? 1 : 0));
		level->noNeighborUpdate = false;
		level->updateNeighborsAt(x, y, z, tile->id);
		level->updateNeighborsAt(x, y + 1, z, tile->id);
	}
};

#endif /*NET_MINECRAFT_WORLD_ITEM__DoorItem_H__*/
