#include "FurnaceTile.h"
#include "entity/FurnaceTileEntity.h"
#include "../material/Material.h"
#include "../../Facing.h"
#include "../../entity/Mob.h"
#include "../../entity/item/ItemEntity.h"
#include "../../entity/player/Player.h"
#include "../../item/ItemInstance.h"
#include "../../../util/Mth.h"
#include "../LevelSource.h"
#include "../Level.h"

bool FurnaceTile::noDrop = false;

FurnaceTile::FurnaceTile( int id, bool lit )
:   super(id, Material::stone),
	lit(lit)
{
	tex = 13 + 16 * 2;
}

int FurnaceTile::getResource( int data, Random* random/*, int playerBonusLevel*/ )
{
	return Tile::furnace->id;
}

void FurnaceTile::onPlace( Level* level, int x, int y, int z )
{
	super::onPlace(level, x, y, z);
	recalcLockDir(level, x, y, z);
}

int FurnaceTile::getTexture( LevelSource* level, int x, int y, int z, int face )
{
	if (face == 1) return tex + 17;
	if (face == 0) return tex + 17;

	int lockDir = level->getData(x, y, z);

	if (face != lockDir) return tex;
	if (lit) return tex + 16;
	else return tex - 1;
}

int FurnaceTile::getTexture( int face )
{
	if (face == 1) return tex + 17;
	if (face == 0) return tex + 17;
	if (face == 3) return tex - 1;
	return tex;
}

void FurnaceTile::animateTick( Level* level, int xt, int yt, int zt, Random* random )
{
	if (!lit) return;

	int dir = level->getData(xt, yt, zt);

	float x = xt + 0.5f;
	float y = yt + 0.0f + random->nextFloat() * 6 / 16.0f;
	float z = zt + 0.5f;
	float r = 0.52f;
	float ss = random->nextFloat() * 0.6f - 0.3f;

	if (dir == 4) {
		level->addParticle(PARTICLETYPE(smoke), x - r, y, z + ss, 0, 0, 0);
		level->addParticle(PARTICLETYPE(flame), x - r, y, z + ss, 0, 0, 0);
	} else if (dir == 5) {
		level->addParticle(PARTICLETYPE(smoke), x + r, y, z + ss, 0, 0, 0);
		level->addParticle(PARTICLETYPE(flame), x + r, y, z + ss, 0, 0, 0);
	} else if (dir == 2) {
		level->addParticle(PARTICLETYPE(smoke), x + ss, y, z - r, 0, 0, 0);
		level->addParticle(PARTICLETYPE(flame), x + ss, y, z - r, 0, 0, 0);
	} else if (dir == 3) {
		level->addParticle(PARTICLETYPE(smoke), x + ss, y, z + r, 0, 0, 0);
		level->addParticle(PARTICLETYPE(flame), x + ss, y, z + r, 0, 0, 0);
	}
}

bool FurnaceTile::use( Level* level, int x, int y, int z, Player* player )
{
	if (level->isClientSide)
		return true;

	FurnaceTileEntity* furnace = static_cast<FurnaceTileEntity*>(level->getTileEntity(x, y, z));
	if (furnace != NULL) player->openFurnace(furnace);
	return true;
}

/*static*/
void FurnaceTile::setLit( bool lit, Level* level, int x, int y, int z )
{
	int data = level->getData(x, y, z);
	TileEntity* te = level->getTileEntity(x, y, z);

	noDrop = true;
	if (lit) level->setTile(x, y, z, Tile::furnace_lit->id);
	else level->setTile(x, y, z, Tile::furnace->id);
	noDrop = false;

	level->setData(x, y, z, data);

	LOGI("lit? %d @ %d, %d, %d\n", lit, x, y, z);

	if (te != NULL) {
		te->clearRemoved();
		level->setTileEntity(x, y, z, te);
	}
}

TileEntity* FurnaceTile::newTileEntity()
{
	return TileEntityFactory::createTileEntity(TileEntityType::Furnace);
}

void FurnaceTile::setPlacedBy( Level* level, int x, int y, int z, Mob* by )
{
	int dir = (Mth::floor(by->yRot * 4 / (360) + 0.5f)) & 3;

	if (dir == 0) level->setData(x, y, z, Facing::NORTH);
	if (dir == 1) level->setData(x, y, z, Facing::EAST);
	if (dir == 2) level->setData(x, y, z, Facing::SOUTH);
	if (dir == 3) level->setData(x, y, z, Facing::WEST);
}

void FurnaceTile::onRemove( Level* level, int x, int y, int z )
{
	if (!noDrop && !level->isClientSide) {
		FurnaceTileEntity* container = (FurnaceTileEntity*) level->getTileEntity(x, y, z);
		if (container != NULL) {
			for (int i = 0; i < container->getContainerSize(); i++) {
				ItemInstance* item = container->getItem(i);
				if (item) {
					float xo = random.nextFloat() * 0.8f + 0.1f;
					float yo = random.nextFloat() * 0.8f + 0.1f;
					float zo = random.nextFloat() * 0.8f + 0.1f;

					while (item->count > 0) {
						int count = random.nextInt(21) + 10;
						if (count > item->count) count = item->count;
						item->count -= count;

						ItemEntity* itemEntity = new ItemEntity(level, x + xo, y + yo, z + zo, ItemInstance(item->id, count, item->getAuxValue()));
						float pow = 0.05f;
						itemEntity->xd = random.nextGaussian() * pow;
						itemEntity->yd = random.nextGaussian() * pow + 0.2f;
						itemEntity->zd = random.nextGaussian() * pow;
						level->addEntity(itemEntity);
					}
				}
			}
		}
	}
	super::onRemove(level, x, y, z);
}

void FurnaceTile::recalcLockDir( Level* level, int x, int y, int z )
{
	if (level->isClientSide) {
		return;
	}

	int n = level->getTile(x, y, z - 1); // face = 2
	int s = level->getTile(x, y, z + 1); // face = 3
	int w = level->getTile(x - 1, y, z); // face = 4
	int e = level->getTile(x + 1, y, z); // face = 5

	int lockDir = 3;
	if (Tile::solid[e] && !Tile::solid[w]) lockDir = 4;
	else if (Tile::solid[w] && !Tile::solid[e]) lockDir = 5;
	else if (Tile::solid[s] && !Tile::solid[n]) lockDir = 2;
	else if (Tile::solid[n] && !Tile::solid[s]) lockDir = 3;

	level->setData(x, y, z, lockDir);
}
