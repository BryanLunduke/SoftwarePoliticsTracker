#ifndef NET_MINECRAFT_WORLD_ITEM__WeaponItem_H__
#define NET_MINECRAFT_WORLD_ITEM__WeaponItem_H__

//package net.minecraft.world.item;

#include "Item.h"
#include "../entity/Entity.h"
#include "../entity/Mob.h"
#include "../entity/player/Player.h"
#include "../level/Level.h"
#include "../level/tile/Tile.h"

class WeaponItem: public Item
{
	typedef Item super;
public:
    WeaponItem(int id, const Item::Tier& tier)
	:	super(id),
		tier(tier)
	{
        maxStackSize = 1;
        setMaxDamage(tier.getUses());

        damage = 4 + tier.getAttackDamageBonus();
    }

    /*@Override*/
    float getDestroySpeed(ItemInstance* itemInstance, Tile* tile) {
        //@todo
        if (tile->id == Tile::web->id) {
            // swords can quickly cut web
            return 15;
        }
        return 1.5f;
    }

    void hurtEnemy(ItemInstance* itemInstance, Mob* mob/*, Mob* attacker*/) {
        itemInstance->hurt(1);// attacker);
        //return true;
    }

    bool mineBlock(ItemInstance* itemInstance, int tile, int x, int y, int z/*, Mob* owner*/) {
        itemInstance->hurt(2);//, owner);
        return true;
    }

    int getAttackDamage(Entity* entity) {
        return damage;
    }

    bool isHandEquipped() const {
        return true;
    }

    //UseAnim getUseAnimation(ItemInstance itemInstance) {
    //    return UseAnim.block;
    //}

    int getUseDuration(ItemInstance* itemInstance) {
        return 20 * 60 * 60; // Block for a maximum of one hour!
    }

    ItemInstance* use(ItemInstance* instance, Level* level, Player* player) {
        //player->startUsingItem(instance, getUseDuration(instance)); //@todo
        return instance;
    }

    // /*@Override*/
    bool canDestroySpecial(const Tile* tile) const {
        return tile->id == Tile::web->id;
    }

    ///*@Override*/
    //int getEnchantmentValue() {
    //    return tier.getEnchantmentValue();
    //}
private:
    int damage;
    const Tier& tier;
};

#endif /*NET_MINECRAFT_WORLD_ITEM__WeaponItem_H__*/
