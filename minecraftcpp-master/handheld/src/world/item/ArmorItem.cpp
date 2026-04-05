#include "ArmorItem.h"

//
// ArmorMaterial
//
ArmorItem::ArmorMaterial::ArmorMaterial( int durabilityMultiplier, int p0, int p1, int p2, int p3 )
	:	durabilityMultiplier(durabilityMultiplier)
{
	slotProtections[0] = p0;
	slotProtections[1] = p1;
	slotProtections[2] = p2;
	slotProtections[3] = p3;
	//this->enchantmentValue = enchantmentValue;
}

int ArmorItem::ArmorMaterial::getHealthForSlot( int slot ) const {
	return healthPerSlot[slot] * durabilityMultiplier;
}

int ArmorItem::ArmorMaterial::getDefenseForSlot( int slot ) const {
	return slotProtections[slot];
}

//
// ArmorItem
//
const int ArmorItem::healthPerSlot[4] = {
	11, 16, 15, 13
};

ArmorItem::ArmorItem( int id, const ArmorMaterial& armorType, int icon, int slot ) :	super(id),
	armorType(armorType),
	slot(slot),
	modelIndex(icon),
	defense(armorType.getDefenseForSlot(slot))
{
	setMaxDamage(armorType.getHealthForSlot(slot));
	maxStackSize = 1;
}

bool ArmorItem::isArmor() const {
	return true;
}

//
// Singleton ArmorMaterials
//

const ArmorItem::ArmorMaterial ArmorItem::CLOTH(
	5,				// durability
	1, 3, 2, 1		// protection values
	//15,			// enchantment
);

const ArmorItem::ArmorMaterial ArmorItem::CHAIN(
	15,
	2, 5, 4, 1
	//12,
);

const ArmorItem::ArmorMaterial ArmorItem::IRON(
	15,
	2, 6, 5, 2
	//9
);
        
const ArmorItem::ArmorMaterial ArmorItem::GOLD(
	7,
	2, 5, 3, 1
	//25
);

const ArmorItem::ArmorMaterial ArmorItem::DIAMOND(
	33,
	3, 8, 6, 3
	//10
);
