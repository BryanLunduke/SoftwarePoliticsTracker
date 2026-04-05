#ifndef NET_MINECRAFT_WORLD_ITEM__StoneSlabTileItem_H__
#define NET_MINECRAFT_WORLD_ITEM__StoneSlabTileItem_H__

//package net.minecraft.world.item;

#include "TileItem.h"
#include "../Facing.h"
#include "../level/Level.h"
#include "../entity/player/Player.h"
#include "../level/tile/StoneSlabTile.h"
#include "../level/tile/Tile.h"

class StoneSlabTileItem: public TileItem
{
	typedef TileItem super;
public:
    StoneSlabTileItem(int id)
	:	super(id)
	{
        setMaxDamage(0);
        setStackedByData(true);
    }

    /*@Override*/
    int getIcon(int itemAuxValue) {
        return Tile::stoneSlabHalf->getTexture(2, itemAuxValue);
    }

    /*@Override*/
    int getLevelDataForAuxValue(int auxValue) {
        return auxValue;
    }

    /*@Override*/
    std::string getDescriptionId(const ItemInstance* instance) const {
		int auxValue = instance->getAuxValue();
        if (auxValue < 0 || auxValue >= StoneSlabTile::SLAB_NAMES_COUNT)
            auxValue = 0;
        return super::getDescriptionId() + "." + StoneSlabTile::SLAB_NAMES[auxValue];
    }

    /*@Override*/
    bool useOn(ItemInstance* instance, Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ) {
        if (instance->count == 0) return false;
        //if (!player->mayBuild(x, y, z)) return false;

        int currentTile = level->getTile(x, y, z);
        int currentData = level->getData(x, y, z);
		int slabType = currentData & StoneSlabTile::TYPE_MASK;
		bool isUpper = (currentData & StoneSlabTile::TOP_SLOT_BIT) != 0;

        if (((face == Facing::UP && !isUpper) || (face == Facing::DOWN && isUpper)) && currentTile == Tile::stoneSlabHalf->id && slabType == instance->getAuxValue()) {
			bool unobstructed = level->isUnobstructed(*Tile::stoneSlab->getAABB(level, x, y, z));
            if (unobstructed && level->setTileAndData(x, y, z, Tile::stoneSlab->id, slabType)) {
                level->playSound(x + 0.5f, y + 0.5f, z + 0.5f, Tile::stoneSlab->soundType->getStepSound(), (Tile::stoneSlab->soundType->getVolume() + 1) / 2, Tile::stoneSlab->soundType->getPitch() * 0.8f);
                instance->count--;
            }
            return true;
        } else {
            return super::useOn(instance, player, level, x, y, z, face, clickX, clickY, clickZ);
        }
    }
};

#endif /*NET_MINECRAFT_WORLD_ITEM__StoneSlabTileItem_H__*/
