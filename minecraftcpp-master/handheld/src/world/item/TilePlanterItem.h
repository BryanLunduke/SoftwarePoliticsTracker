#ifndef NET_MINECRAFT_WORLD_ITEM__TilePlanterItem_H__
#define NET_MINECRAFT_WORLD_ITEM__TilePlanterItem_H__

//package net.minecraft.world.item;

#include "../entity/player/Player.h"
#include "../level/Level.h"
#include "../level/tile/Tile.h"
#include "Item.h"
#include "ItemInstance.h"

class TilePlanterItem: public Item
{
	typedef Item super;

	int tileId;
public:
    TilePlanterItem(int id, Tile* tile)
	:	super(id)
	{
        tileId = tile->id;
    }

    bool useOn(ItemInstance* instance, Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ) {
        if (level->getTile(x, y, z) == Tile::topSnow->id) {
            face = 0;
        } else {
			switch (face) {
				case Facing::DOWN : y--; break;
                case Facing::UP   : y++; break;
                case Facing::NORTH: z--; break;
                case Facing::SOUTH: z++; break;
                case Facing::WEST : x--; break;
                case Facing::EAST : x++; break;
			}
        }

        if (instance->count == 0) return false;

        if (level->mayPlace(tileId, x, y, z, false, face)) {
            //Tile* tile = Tile::tiles[tileId];
            if (level->setTile(x, y, z, tileId)) {
                //Tile::tiles[tileId]->setPlacedOnFace(level, x, y, z, face);
                Tile::tiles[tileId]->setPlacedBy(level, x, y, z, player);
                //level->playSound(x + 0.5f, y + 0.5f, z + 0.5f, tile->soundType.getStepSound(), (tile->soundType.getVolume() + 1) / 2, tile->soundType.getPitch() * 0.8f);
                instance->count--;
            }
        }
        return true;
    }
};

#endif /*NET_MINECRAFT_WORLD_ITEM__TilePlanterItem_H__*/
