#ifndef NET_MINECRAFT_WORLD_ITEM__FlintAndSteelItem_H__
#define NET_MINECRAFT_WORLD_ITEM__FlintAndSteelItem_H__

//package net.minecraft.world.item;

#include "Item.h"
#include "../level/Level.h"
#include "../level/tile/Tile.h"
#include "../entity/player/Player.h"

class FlintAndSteelItem: public Item
{
    typedef Item super;
public:
    FlintAndSteelItem(int id)
    :   super(id)
    {
		setMaxStackSize(1);
		setMaxDamage(64);
    }

    /*
    bool useOn(ItemInstance* instance, Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ) {
        if (face == 0) y--;
        if (face == 1) y++;
        if (face == 2) z--;
        if (face == 3) z++;
        if (face == 4) x--;
        if (face == 5) x++;

        int targetType = level->getTile(x, y, z);
        if (targetType == 0) {
            level->playSound(x + 0.5, y + 0.5, z + 0.5, "fire.ignite", 1, sharedRandom.nextFloat() * 0.4f + 0.8f);
            level->setTile(x, y, z, Tile::fire->id);
        }

        instance->hurt(1);
        return true;
    }
    */
};

#endif /*NET_MINECRAFT_WORLD_ITEM__FlintAndSteelItem_H__*/
