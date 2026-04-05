#include "Animal.h"
#include "../../level/Level.h"
#include "../../level/tile/Tile.h"

#include "../../../nbt/CompoundTag.h"


Animal::Animal( Level* level )
:	super(level),
	inLove(0)
{
	//entityData.define(DATA_AGE_ID, (SynchedEntityData::TypeInt)0);
}

bool Animal::hurt( Entity* source, int dmg )
{
	fleeTime = 3 * SharedConstants::TicksPerSecond;
	attackTargetId = 0;
	inLove = 0;
	return super::hurt(source, dmg);
}

bool Animal::canSpawn()
{
	int xt = Mth::floor(x);
	int yt = Mth::floor(bb.y0);
	int zt = Mth::floor(z);
	return level->getTile(xt, yt - 1, zt) == ((Tile*)Tile::grass)->id && level->getRawBrightness(xt, yt, zt) > 8 && super::canSpawn();
}

int Animal::getAmbientSoundInterval()
{
	return 12 * SharedConstants::TicksPerSecond;
}

float Animal::getWalkTargetValue( int x, int y, int z )
{
	if (level->getTile(x, y - 1, z) == ((Tile*)Tile::grass)->id) return 10;
	return level->getBrightness(x, y, z) - 0.5f;
}

Entity* Animal::findAttackTarget()
{
	return NULL;

	//if (fleeTime > 0)
	//	return NULL;

	//int inLove = -1;

	//float r = 8;
	//if (inLove > 0) {
	//	EntityList others;
	//	level->getEntitiesOfType(getEntityTypeId(), bb.expand(r, r, r), others);
	//	for (unsigned int i = 0; i < others.size(); i++) {
	//		Animal* p = (Animal*) others[i];
	//		if (p != this && p->inLove > 0) {
	//			return p;
	//		}
	//	}
	//} else {
	//	/*			if (getAge() == 0) {
	//	List<Entity> players = level.getEntitiesOfClass(Player.class, bb.expand(r, r, r));
	//	for (int i = 0; i < players.size(); i++) {
	//	Player p = (Player) players.get(i);
	//	if (p.getSelectedItem() != null && this.isFood(p.getSelectedItem())) {
	//	return p;
	//	}
	//	}
	//	} else if (getAge() > 0) {
	//	EntityList others;
	//	level->getEntitiesOfType(getEntityTypeId(), bb.expand(r, r, r), others);
	//	for (unsigned int i = 0; i < others.size(); i++) {
	//	Animal* p = (Animal*) others[i];
	//	if (p != this && p->getAge() < 0) {
	//	return p;
	//	}
	//	}
	//	}
	//	*/
	//}
	//return NULL;
}

bool Animal::removeWhenFarAway()
{
	return false;
}

int Animal::getCreatureBaseType() const {
	return MobTypes::BaseCreature;
}
