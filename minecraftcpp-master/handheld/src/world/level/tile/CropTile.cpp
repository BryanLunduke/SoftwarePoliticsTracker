#include "CropTile.h"
#include "Tile.h"
#include "../../entity/item/ItemEntity.h"

CropTile::CropTile( int id, int tex ) : super(id, tex) {
	this->tex = tex;
	setTicking(true);
	float ss = 0.5f;
	setShape(0.5f - ss, 0, 0.5f - ss, 0.5f + ss, 0.25f, 0.5f + ss);
}

void CropTile::tick( Level* level, int x, int y, int z, Random* random ) {
	super::tick(level, x, y, z, random);
	if(level->getRawBrightness(x, y, z) >= Level::MAX_BRIGHTNESS - 6) {
		int age = level->getData(x, y, z);
		if(age < 7) {
			float growthSpeed = getGrowthSpeed(level, x, y, z);
			if(random->nextInt(int(25 / growthSpeed)) == 0) {
				age++;
				level->setData(x, y, z, age);
			}
		}
	}
}

void CropTile::growCropsToMax( Level* level, int x, int y, int z ) {
	level->setData(x, y, z, 7);
}

float CropTile::getGrowthSpeed( Level* level, int x, int y, int z ) {
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

	for (int xx = x - 1; xx <= x + 1; xx++) {
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
	}
	if (diagonal || (horizontal && vertical)) speed /= 2;

	return speed;
}

int CropTile::getTexture( LevelSource* level, int x, int y, int z, int face ) {
	int data = level->getData(x, y, z);
	if (data < 0) data = 7;
	return tex + data;
}

int CropTile::getTexture( int face, int data ) {
	if (data < 0) data = 7;
	return tex + data;
}

int CropTile::getRenderShape() {
	return Tile::SHAPE_ROWS;
}
void CropTile::spawnResources( Level* level, int x, int y, int z, int data, float odds ) {
	super::spawnResources(level, x, y, z, data, odds);

	if (level->isClientSide) {
		return;
	}
	int count = 3;
	for (int i = 0; i < count; i++) {
		if (level->random.nextInt(5 * 3) > data) continue;
		float s = 0.7f;
		float xo = level->random.nextFloat() * s + (1 - s) * 0.5f;
		float yo = level->random.nextFloat() * s + (1 - s) * 0.5f;
		float zo = level->random.nextFloat() * s + (1 - s) * 0.5f;
		ItemEntity* item = new ItemEntity(level, float(x) + xo, float(y) + yo, float(z) + zo, ItemInstance(Item::seeds_wheat));
		item->throwTime = 10;
		level->addEntity(item);
	}
}

int CropTile::getResource( int data, Random* random ) {
	if (data == 7) {
		return Item::wheat->id;
	}
	return -1;
}

int CropTile::getResourceCount( Random* random ) {
	return 1;
}

bool CropTile::mayPlaceOn( int tile ) {
	return tile == Tile::farmland->id;
}
