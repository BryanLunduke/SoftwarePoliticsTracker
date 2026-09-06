#include "StemTile.h"
StemTile::StemTile( int id, Tile* fruit )
: super(id, 15 + 6 * 16),
  fruit(fruit) {
	setTicking(true);
	float ss = 0.125f;
	this->setShape(0.5f - ss, 0, 0.5f - ss, 0.5f + ss, 0.25f, 0.5f + ss);
}

bool StemTile::mayPlaceOn( int tile ) {
	return tile == Tile::farmland->id;
}

void StemTile::tick( Level* level, int x, int y, int z, Random* random ) {
	super::tick(level, x, y, z, random);
	if (level->getRawBrightness(x, y + 1, z) >= Level::MAX_BRIGHTNESS - 6) {

		float growthSpeed = getGrowthSpeed(level, x, y, z);

		if (random->nextInt((int) (25 / growthSpeed) + 1) == 0) {
			int age = level->getData(x, y, z);
			if (age < 7) {
				age++;
				level->setData(x, y, z, age);
			} else {
				if (level->getTile(x - 1, y, z) == fruit->id) return;
				if (level->getTile(x + 1, y, z) == fruit->id) return;
				if (level->getTile(x, y, z - 1) == fruit->id) return;
				if (level->getTile(x, y, z + 1) == fruit->id) return;
				for(int a = 0; a < 4; ++a) {
					int dir  = a; //random->nextInt(4);
					int xx = x;
					int zz = z;
					if (dir == 0) xx--;
					if (dir == 1) xx++;
					if (dir == 2) zz--;
					if (dir == 3) zz++;
					int below = level->getTile(xx, y - 1, zz);
					if (level->getTile(xx, y, zz) == 0 && (below == Tile::farmland->id || below == Tile::dirt->id || below == Tile::grass->id)) {
						level->setTile(xx, y, zz, fruit->id);
						break;
					}
				}
			}
		}
	}
}

void StemTile::growCropsToMax( Level* level, int x, int y, int z ) {
	level->setData(x, y, z, 7);
}

float StemTile::getGrowthSpeed( Level* level, int x, int y, int z ) {
	float speed = 1;

	int n = level->getTile(x, y, z - 1);
	int s = level->getTile(x, y, z + 1);
	int w = level->getTile(x - 1, y, z);
	int e = level->getTile(x + 1, y, z);

	int d0 = level->getTile(x - 1, y, z - 1);
	int d1 = level->getTile(x + 1, y, z - 1);
	int d2 = level->getTile(x + 1, y, z + 1);
	int d3 = level->getTile(x - 1, y, z + 1);

	bool horizontal = w == this->id || e == this->id;
	bool vertical = n == this->id || s == this->id;
	bool diagonal = d0 == this->id || d1 == this->id || d2 == this->id || d3 == this->id;

	for (int xx = x - 1; xx <= x + 1; xx++)
		for (int zz = z - 1; zz <= z + 1; zz++) {
			int t = level->getTile(xx, y - 1, zz);

			float tileSpeed = 0;
			if (t == Tile::farmland->id) {
				tileSpeed = 1;
				if (level->getData(xx, y - 1, zz) > 0) tileSpeed = 3;
			}

			if (xx != x || zz != z) tileSpeed /= 4;

			speed += tileSpeed;
		}

		if (diagonal || (horizontal && vertical)) speed /= 2;

		return speed;
}

int StemTile::getColor( int data ) {
	int r = data * 32;
	int g = 255 - data * 8;
	int b = data * 4;
	return r << 16 | g << 8 | b;
}

int StemTile::getColor( LevelSource* level, int x, int y, int z ) {
	return getColor(level->getData(x, y, z));
}

int StemTile::getTexture( int face, int data ) {
	return tex;
}

void StemTile::updateDefaultShape() {
	float ss = 0.125f;
	this->setShape(0.5f - ss, 0, 0.5f - ss, 0.5f + ss, 0.25f, 0.5f + ss);
}

void StemTile::updateShape( LevelSource* level, int x, int y, int z ) {
	yy1 = (level->getData(x, y, z) * 2 + 2) / 16.0f;
	float ss = 0.125f;
	this->setShape(0.5f - ss, 0, 0.5f - ss, 0.5f + ss, (float) yy1, 0.5f + ss);
}

int StemTile::getRenderShape() {
	return Tile::SHAPE_STEM;
}

int StemTile::getConnectDir( LevelSource* level, int x, int y, int z ) {
	int d = level->getData(x, y, z);
	if (d < 7) return -1;
	if (level->getTile(x - 1, y, z) == fruit->id) return 0;
	if (level->getTile(x + 1, y, z) == fruit->id) return 1;
	if (level->getTile(x, y, z - 1) == fruit->id) return 2;
	if (level->getTile(x, y, z + 1) == fruit->id) return 3;
	return -1;
}

void StemTile::spawnResources( Level* level, int x, int y, int z, int data, float odds ) {
	super::spawnResources(level, x, y, z, data, odds);

	if (level->isClientSide) {
		return;
	}

	Item* seed = NULL;
	//if (fruit == Tile::pumpkin) seed = Item::seeds_pumpkin;
	if (fruit == Tile::melon) seed = Item::seeds_melon;
	for (int i = 0; i < 3; i++) {
		if (level->random.nextInt(5 * 3) > data) continue;
		popResource(level, x, y, z, ItemInstance(seed));
	}
}

int StemTile::getResource( int data, Random* random ) {
	return -1;
}

int StemTile::getResourceCount( Random* random ) {
	return 1;
}

