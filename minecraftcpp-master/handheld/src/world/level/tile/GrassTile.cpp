#include "GrassTile.h"
#include "../material/Material.h"
#include "../../entity/item/ItemEntity.h"

GrassTile::GrassTile(int id)
:	super(id, Material::dirt)
{
	tex = 3;
	setTicking(true);
}

int GrassTile::getTexture( LevelSource* level, int x, int y, int z, int face ) {
	if (face == 1) return 0;
	if (face == 0) return 2;
	const Material* above = level->getMaterial(x, y + 1, z);
	if (above == Material::topSnow || above == Material::snow) return 4 * 16 + 4;
	else return 3;
}

int GrassTile::getTexture( int face, int data ) {
	if (face == 1) return 0;
	if (face == 0) return 2;
	return 3;
}

int GrassTile::getColor( LevelSource* level, int x, int y, int z ) {
	//level.getBiomeSource().getBiomeBlock(x, z, 1, 1);
	//float temp = level.getBiomeSource().temperatures[0];
	//float rain = level.getBiomeSource().downfalls[0];

	return 0x339933;//GrassColor.get(temp, rain);
}

void GrassTile::tick( Level* level, int x, int y, int z, Random* random ) {
	if (level->isClientSide) return;

	if (level->getRawBrightness(x, y + 1, z) < MIN_BRIGHTNESS && level->getMaterial(x, y + 1, z)->blocksLight()) {
		if (random->nextInt(4) != 0) return;
		level->setTile(x, y, z, Tile::dirt->id);
	} else {
		if (level->getRawBrightness(x, y + 1, z) >= level->MAX_BRIGHTNESS - 6) {
			int xt = x + random->nextInt(3) - 1;
			int yt = y + random->nextInt(5) - 3;
			int zt = z + random->nextInt(3) - 1;
			if (level->getTile(xt, yt, zt) == Tile::dirt->id && level->getRawBrightness(xt, yt + 1, zt) >= MIN_BRIGHTNESS && !level->getMaterial(xt, yt + 1, zt)->blocksLight()) {
				level->setTile(xt, yt, zt, Tile::grass->id);
			}
		}
	}
}

int GrassTile::getResource( int data, Random* random ) {
	return Tile::dirt->getResource(0, random);
}
