#ifndef NET_MINECRAFT_WORLD_ITEM__DiggerItem_H__
#define NET_MINECRAFT_WORLD_ITEM__DiggerItem_H__

//package net.minecraft.world.item;

#include "../level/tile/Tile.h"

#include "Item.h"
#include "ItemInstance.h"

class DiggerItem: public Item
{
    typedef Item super;
protected:
	typedef std::vector<Tile*> TileList;
public:
    float getDestroySpeed(ItemInstance* itemInstance, Tile* tile) {
		if (hasTile(tile))
			return speed;
        return 1;
    }

    //@Override
    void hurtEnemy(ItemInstance* itemInstance, Mob* mob/*, Mob* attacker*/) {
        itemInstance->hurt(2);//, attacker); //@todo
        //return true;
    }

    //@Override
    bool mineBlock(ItemInstance* itemInstance, int tile, int x, int y, int z/*, Mob* owner*/) {
        itemInstance->hurt(1);//, owner);
        return true;
    }

    int getAttackDamage(Entity* entity) {
        return attackDamage;
    }

    bool isHandEquipped() const {
        return true;
    }

	void setTiles(const TileList& tiles) {
		for (int i = 0; i < 256; ++i) {
			_bTiles[i] = false;
		}

		for (unsigned int i = 0; i < tiles.size(); ++i) {
			_bTiles[tiles[i]->id] = true;
		}
		this->tiles = tiles;
	}

    //@Override
    //int getEnchantmentValue() {
    //    return tier.getEnchantmentValue();
    //}
protected:
    float speed;

    const Tier& tier;

    DiggerItem(int id, int attackDamage, const Tier& tier, const TileList& tiles = TileList())
    :   super(id),
        speed(tier.getSpeed()),
        tier(tier)
    {
		setTiles(tiles);

        maxStackSize = 1;
        setMaxDamage(tier.getUses());
        this->attackDamage = attackDamage + tier.getAttackDamageBonus();
    }

	bool hasTile(Tile* tile) {
		return tile && _bTiles[tile->id];
	}

private:
    int attackDamage;
    TileList tiles;
	bool _bTiles[256];
};

#endif /*NET_MINECRAFT_WORLD_ITEM__DiggerItem_H__*/
