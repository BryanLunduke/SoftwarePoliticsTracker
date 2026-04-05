#ifndef NET_MINECRAFT_WORLD_ENTITY__MobCategory_H__
#define NET_MINECRAFT_WORLD_ENTITY__MobCategory_H__

#include "EntityTypes.h"
#include "MobCategory.h"
#include "../level/material/Material.h"


const MobCategory MobCategory::monster(
    MobTypes::BaseEnemy,
    10,
	20,
    false);

	//

const MobCategory MobCategory::creature(
    MobTypes::BaseCreature,
    10,
	15,
    true);

    //

const MobCategory MobCategory::waterCreature(
    MobTypes::BaseWaterCreature,
    5,
	10,
    true);

//
// Init an array with all defined MobCategory'ies
//
const MobCategory* const MobCategory::values[] = {
	&MobCategory::monster,
	&MobCategory::creature,
	&MobCategory::waterCreature
};

/*static*/
void MobCategory::initMobCategories() {
	monster.setMaterial(Material::air);
	creature.setMaterial(Material::air);
	waterCreature.setMaterial(Material::water);
}


const int MobCategory::numValues = sizeof(values) / sizeof(values[0]);

#endif /*NET_MINECRAFT_WORLD_ENTITY__MobCategory_H__*/
