#include "Cow.h"
#include "../player/Player.h"
#include "../../level/Level.h"
#include "../../item/Item.h"

Cow::Cow( Level* level )
:	super(level)
{
	entityRendererId = ER_COW_RENDERER;

	textureName = "mob/cow.png";
	setSize(0.9f, 1.3f);
}

int Cow::getEntityTypeId() const {
	return MobTypes::Cow;
}

int Cow::getMaxHealth() {
	return 10;
}

void Cow::addAdditonalSaveData( CompoundTag* tag ) {
	super::addAdditonalSaveData(tag);
}

void Cow::readAdditionalSaveData( CompoundTag* tag ) {
	super::readAdditionalSaveData(tag);
}

bool Cow::interact( Player* player ) {
	//ItemInstance item = player->inventory.getSelected();
	//if (item != NULL && item.id == Item::bucket_empty->id) {
	//    player->inventory.setItem(player->inventory.selected, /*new*/ ItemInstance(Item::milk));
	//    return true;
	//}
	return super::interact(player);
}

const char* Cow::getAmbientSound() {
	return "mob.cow";
}

std::string Cow::getHurtSound() {
	return "mob.cowhurt";
}

std::string Cow::getDeathSound() {
	return "mob.cowhurt";
}

float Cow::getSoundVolume() {
	return 0.4f;
}

int Cow::getDeathLoot() {
	return Item::leather->id;
}

void Cow::dropDeathLoot( /*bool wasKilledByPlayer, int playerBonusLevel*/ ) {
	// drop some leather
	int count = random.nextInt(3);// + random.nextInt(1 + playerBonusLevel);
	for (int i = 0; i < count; i++) {
	    spawnAtLocation(Item::leather->id, 1);
	}
	// and some meat
	count = random.nextInt(3) + 1;
	for (int i = 0; i < count; i++) {
	//    if (isOnFire()) { //@fire
	//        spawnAtLocation(Item::beef_cooked->id, 1);
	//    } else {
	        spawnAtLocation(Item::beef_raw->id, 1);
	//    }
	}
}

Animal* Cow::getBreedOffspring( Animal* target ) {
	return new Cow(level);
}
