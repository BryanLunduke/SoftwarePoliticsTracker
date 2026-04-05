#include "Painting.h"
#include "../level/Level.h"
#include "../item/Item.h"
Painting::Painting( Level* level ) : super(level) {
	entityRendererId = ER_PAINTING_RENDERER;
}

Painting::Painting( Level* level, int xTile, int yTile, int zTile, int dir )
:	super(level, xTile, yTile, zTile, dir) {
	setRandomMotive(dir);

	entityRendererId = ER_PAINTING_RENDERER;
}

Painting::Painting( Level* level, int x, int y, int z, int dir, const std::string& motiveName )
	: super(level, x, y, z, dir) {
	motive = Motive::getMotiveByName(motiveName);
	setDir(dir);
	entityRendererId = ER_PAINTING_RENDERER;
}

void Painting::addAdditonalSaveData( CompoundTag* tag ) {
	if(motive != NULL) {
		tag->putString("Motive", motive->name);
	}
	super::addAdditonalSaveData(tag);
}

void Painting::readAdditionalSaveData( CompoundTag* tag ) {
	std::string motiveName = tag->getString("Motive");
	motive = Motive::getMotiveByName(motiveName);
	super::readAdditionalSaveData(tag);
}

int Painting::getWidth() {
	return motive->w;
}

int Painting::getHeight() {
	return motive->h;
}

void Painting::dropItem() {
	if(level->getLevelData()->getGameType() != GameType::Creative)
		spawnAtLocation(Item::painting->id, 1);
}

int Painting::getEntityTypeId() const {
	return EntityTypes::IdPainting;
}

void Painting::setRandomMotive( int dir ) {
	std::vector<const Motive*> allMotives = Motive::getAllMotivesAsList();
	std::vector<const Motive*> survivableMotives;
	for(std::vector<const Motive*>::iterator i = allMotives.begin(); i != allMotives.end(); ++i) {
		if (!(*i)->isPublic)
			continue;

		motive = *i;
		setDir(dir);
		if(survives()) {
			survivableMotives.push_back(*i);
		}
	}
	if(!survivableMotives.empty()) {
		this->motive = survivableMotives[sharedRandom.nextInt(survivableMotives.size())];
		setDir(dir);
	}
	else {
		this->motive = Motive::DefaultImage;
		setDir(dir);
	}
}

bool Painting::isPickable() {
	return true;
}
