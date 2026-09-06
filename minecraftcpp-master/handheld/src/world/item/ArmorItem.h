#ifndef NET_MINECRAFT_WORLD_ITEM__ArmorItem_H__
#define NET_MINECRAFT_WORLD_ITEM__ArmorItem_H__

//package net.minecraft.world.item;

#include "Item.h"

class ArmorItem: public Item
{
	typedef Item super;
public:
    static const int SLOT_HEAD = 0;
    static const int SLOT_TORSO = 1;
    static const int SLOT_LEGS = 2;
    static const int SLOT_FEET = 3;

    static const int healthPerSlot[4];

    class ArmorMaterial
	{
	public:
		/// p0-p3 are slot protection values
        ArmorMaterial(int durabilityMultiplier, int p0, int p1, int p2, int p3);//, int enchantmentValue);

        int getHealthForSlot(int slot) const;

        int getDefenseForSlot(int slot) const; 
        /* int getEnchantmentValue() const {
            return enchantmentValue;
        } */
	private:
		int durabilityMultiplier;
		int slotProtections[4];
		//int enchantmentValue;
    };

	static const ArmorMaterial CLOTH;
	static const ArmorMaterial CHAIN;
	static const ArmorMaterial IRON;
	static const ArmorMaterial GOLD;
	static const ArmorMaterial DIAMOND;

    const int slot;
    const int defense;
    const int modelIndex;
    const ArmorMaterial& armorType;

    ArmorItem(int id, const ArmorMaterial& armorType, int icon, int slot);

	bool isArmor() const;
/*
    int getEnchantmentValue() const {
        return armorType.getEnchantmentValue();
    }
*/
};

#endif /*NET_MINECRAFT_WORLD_ITEM__ArmorItem_H__*/
