#include "ChestTile.h"
#include "entity/ChestTileEntity.h"
#include "../Level.h"
#include "../material/Material.h"
#include "../../Facing.h"
#include "../../entity/item/ItemEntity.h"

ChestTile::ChestTile( int id )
:	super(id, Material::wood)
{
	tex = 10 + 16;
	const float m = 0.025f;
	setShape(m, 0, m, 1-m, 1-m-m, 1-m);
}

bool ChestTile::isSolidRender()
{
	return false;
}

bool ChestTile::isCubeShaped()
{
	return false;
}

int ChestTile::getRenderShape()
{
	return super::getRenderShape();
	//return Tile::SHAPE_ENTITYTILE_ANIMATED;
}

void ChestTile::onPlace( Level* level, int x, int y, int z )
{
	super::onPlace(level, x, y, z);
	recalcLockDir(level, x, y, z);

	//@fullchest
	//int n = level->getTile(x, y, z - 1); // face = 2
	//int s = level->getTile(x, y, z + 1); // face = 3
	//int w = level->getTile(x - 1, y, z); // face = 4
	//int e = level->getTile(x + 1, y, z); // face = 5
	//if (n == id) recalcLockDir(level, x, y, z - 1);
	//if (s == id) recalcLockDir(level, x, y, z + 1);
	//if (w == id) recalcLockDir(level, x - 1, y, z);
	//if (e == id) recalcLockDir(level, x + 1, y, z);
}

void ChestTile::setPlacedBy( Level* level, int x, int y, int z, Mob* by )
{
	int n = level->getTile(x, y, z - 1); // face = 2
	int s = level->getTile(x, y, z + 1); // face = 3
	int w = level->getTile(x - 1, y, z); // face = 4
	int e = level->getTile(x + 1, y, z); // face = 5

	int facing = 0;
	int dir = (Mth::floor(by->yRot * 4 / (360) + 0.5f)) & 3;

	if (dir == 0) facing = Facing::NORTH;
	if (dir == 1) facing = Facing::EAST;
	if (dir == 2) facing = Facing::SOUTH;
	if (dir == 3) facing = Facing::WEST;

	if (n != id && s != id && w != id && e != id) {
		level->setData(x, y, z, facing);
	} else {
		if ((n == id || s == id) && (facing == Facing::WEST || facing == Facing::EAST)) {
			if (n == id) level->setData(x, y, z - 1, facing);
			else level->setData(x, y, z + 1, facing);
			level->setData(x, y, z, facing);
		}
		if ((w == id || e == id) && (facing == Facing::NORTH || facing == Facing::SOUTH)) {
			if (w == id) level->setData(x - 1, y, z, facing);
			else level->setData(x + 1, y, z, facing);
			level->setData(x, y, z, facing);
		}
	}
}

void ChestTile::recalcLockDir( Level* level, int x, int y, int z )
{
	if (level->isClientSide)
		return;

	int n = level->getTile(x, y, z - 1); // face = 2
	int s = level->getTile(x, y, z + 1); // face = 3
	int w = level->getTile(x - 1, y, z); // face = 4
	int e = level->getTile(x + 1, y, z); // face = 5

	// Long!
	//@fullchest
	int lockDir = 4;
	/*
	if (n == id || s == id) {
		int w2 = level->getTile(x - 1, y, n == id ? z - 1 : z + 1);
		int e2 = level->getTile(x + 1, y, n == id ? z - 1 : z + 1);

		lockDir = 5;

		int otherDir = -1;
		if (n == id) otherDir = level->getData(x, y, z - 1);
		else otherDir = level->getData(x, y, z + 1);
		if (otherDir == 4) lockDir = 4;

		if ((Tile::solid[w] || Tile::solid[w2]) && !Tile::solid[e] && !Tile::solid[e2]) lockDir = 5;
		if ((Tile::solid[e] || Tile::solid[e2]) && !Tile::solid[w] && !Tile::solid[w2]) lockDir = 4;
	} else if (w == id || e == id) {
		int n2 = level->getTile(w == id ? x - 1 : x + 1, y, z - 1);
		int s2 = level->getTile(w == id ? x - 1 : x + 1, y, z + 1);

		lockDir = 3;
		int otherDir = -1;
		if (w == id) otherDir = level->getData(x - 1, y, z);
		else otherDir = level->getData(x + 1, y, z);
		if (otherDir == 2) lockDir = 2;

		if ((Tile::solid[n] || Tile::solid[n2]) && !Tile::solid[s] && !Tile::solid[s2]) lockDir = 3;
		if ((Tile::solid[s] || Tile::solid[s2]) && !Tile::solid[n] && !Tile::solid[n2]) lockDir = 2;
	} else */ {
		lockDir = level->getData(x, y, z);
		if ((lockDir == Facing::NORTH && Tile::solid[n])
		||  (lockDir == Facing::SOUTH && Tile::solid[s])
		||  (lockDir == Facing::WEST  && Tile::solid[w])
		||  (lockDir == Facing::EAST  && Tile::solid[e])) {
			if (Tile::solid[n] && !Tile::solid[s]) lockDir = Facing::SOUTH;
			if (Tile::solid[s] && !Tile::solid[n]) lockDir = Facing::NORTH;
			if (Tile::solid[w] && !Tile::solid[e]) lockDir = Facing::EAST;
			if (Tile::solid[e] && !Tile::solid[w]) lockDir = Facing::WEST;
		}
	}

	level->setData(x, y, z, lockDir);
}

int ChestTile::getTexture( LevelSource* level, int x, int y, int z, int face )
{
	if (face == 1) return tex - 1;
	if (face == 0) return tex - 1;

	/*
	int n = level->getTile(x, y, z - 1); // face = 2
	int s = level->getTile(x, y, z + 1); // face = 3
	int w = level->getTile(x - 1, y, z); // face = 4
	int e = level->getTile(x + 1, y, z); // face = 5
	*/

	// Long!
	//@fullchest
	/*
	if (n == id || s == id) { 
		if (face == 2 || face == 3) return tex;
		int offs = 0;
		if (n == id) {
			offs = -1;
		}

		int w2 = level->getTile(x - 1, y, n == id ? z - 1 : z + 1);
		int e2 = level->getTile(x + 1, y, n == id ? z - 1 : z + 1);

		if (face == 4) offs = -1 - offs;

		int lockDir = 5;
		if ((Tile::solid[w] || Tile::solid[w2]) && !Tile::solid[e] && !Tile::solid[e2]) lockDir = 5;
		if ((Tile::solid[e] || Tile::solid[e2]) && !Tile::solid[w] && !Tile::solid[w2]) lockDir = 4;
		return (face == lockDir ? tex + 16 : tex + 32) + offs;
	} else if (w == id || e == id) {
		if (face == 4 || face == 5) return tex;
		int offs = 0;
		if (w == id) {
			offs = -1;
		}

		int n2 = level->getTile(w == id ? x - 1 : x + 1, y, z - 1);
		int s2 = level->getTile(w == id ? x - 1 : x + 1, y, z + 1);

		if (face == 3) offs = -1 - offs;
		int lockDir = 3;
		if ((Tile::solid[n] || Tile::solid[n2]) && !Tile::solid[s] && !Tile::solid[s2]) lockDir = 3;
		if ((Tile::solid[s] || Tile::solid[s2]) && !Tile::solid[n] && !Tile::solid[n2]) lockDir = 2;

		return (face == lockDir ? tex + 16 : tex + 32) + offs;
	} else { */
		//int lockDir = 3;
		int lockDir = level->getData(x, y, z);

		/*
		if ((lockDir == Facing::NORTH && Tile::solid[n])
			||  (lockDir == Facing::SOUTH && Tile::solid[s])
			||  (lockDir == Facing::WEST  && Tile::solid[w])
			||  (lockDir == Facing::EAST  && Tile::solid[e])) {
				if (Tile::solid[n] && !Tile::solid[s]) lockDir = 3;
				if (Tile::solid[s] && !Tile::solid[n]) lockDir = 2;
				if (Tile::solid[w] && !Tile::solid[e]) lockDir = 5;
				if (Tile::solid[e] && !Tile::solid[w]) lockDir = 4;
		}
		*/

		/*
		if (Tile::solid[n] && !Tile::solid[s]) lockDir = 3;
		if (Tile::solid[s] && !Tile::solid[n]) lockDir = 2;
		if (Tile::solid[w] && !Tile::solid[e]) lockDir = 5;
		if (Tile::solid[e] && !Tile::solid[w]) lockDir = 4;
		*/
		return (face == lockDir)? tex + 1 : tex;
}

int ChestTile::getTexture( int face )
{
	if (face == 1) return tex - 1;
	if (face == 0) return tex - 1;
	if (face == 3) return tex + 1;
	return tex;
}

bool ChestTile::mayPlace( Level* level, int x, int y, int z, unsigned char face )
{
	int chestCount = 0;

	if (level->getTile(x - 1, y, z) == id) chestCount++;
	if (level->getTile(x + 1, y, z) == id) chestCount++;
	if (level->getTile(x, y, z - 1) == id) chestCount++;
	if (level->getTile(x, y, z + 1) == id) chestCount++;

	//@fullchest
	if (chestCount > 0) return false;

	//if (isFullChest(level, x - 1, y, z)) return false;
	//if (isFullChest(level, x + 1, y, z)) return false;
	//if (isFullChest(level, x, y, z - 1)) return false;
	//if (isFullChest(level, x, y, z + 1)) return false;
	return true;
}

void ChestTile::neighborChanged( Level* level, int x, int y, int z, int type )
{
	super::neighborChanged(level, x, y, z, type);
	recalcLockDir(level, x, y, z);
	TileEntity* te = level->getTileEntity(x, y, z);
	if (te != NULL) te->clearCache();
}

void ChestTile::onRemove( Level* level, int x, int y, int z )
{
	if (!level->isClientSide) {
		TileEntity* te = level->getTileEntity(x, y, z);
		if (te != NULL && te->type == TileEntityType::Chest) {
			Container* container = (ChestTileEntity*) te;
			for (int i = 0; i < container->getContainerSize(); i++) {
				ItemInstance* item = container->getItem(i);
				if (item != NULL) {
					float xo = random.nextFloat() * 0.8f + 0.1f;
					float yo = random.nextFloat() * 0.8f + 0.1f;
					float zo = random.nextFloat() * 0.8f + 0.1f;

					while (item->count > 0) {
						int count = random.nextInt(21) + 10;
						if (count > item->count) count = item->count;
						item->count -= count;
						ItemEntity* itemEntity = new ItemEntity(level, x + xo, y + yo, z + zo, ItemInstance(item->id, count, item->getAuxValue()));
						float pow = 0.05f;
						itemEntity->xd = (float) random.nextGaussian() * pow;
						itemEntity->yd = (float) random.nextGaussian() * pow + 0.2f;
						itemEntity->zd = (float) random.nextGaussian() * pow;
						//if (item->hasTag()) {
						//    itemEntity->item->setTag((CompoundTag*) item->getTag()->copy());
						//}
						level->addEntity(itemEntity);
					}
				}
			}
		}
	}
	super::onRemove(level, x, y, z);
}

bool ChestTile::use( Level* level, int x, int y, int z, Player* player )
{
	TileEntity* te = level->getTileEntity(x, y, z);
	if (!TileEntity::isType(te, TileEntityType::Chest))
		return true;

	ChestTileEntity* chest = (ChestTileEntity*) te;

	if (level->isSolidBlockingTile(x, y + 1, z)) return true;

	// @fullchest
	//if (level->getTile(x - 1, y, z) == id && (level->isSolidBlockingTile(x - 1, y + 1, z))) return true;
	//if (level->getTile(x + 1, y, z) == id && (level->isSolidBlockingTile(x + 1, y + 1, z))) return true;
	//if (level->getTile(x, y, z - 1) == id && (level->isSolidBlockingTile(x, y + 1, z - 1))) return true;
	//if (level->getTile(x, y, z + 1) == id && (level->isSolidBlockingTile(x, y + 1, z + 1))) return true;

	//if (level->getTile(x - 1, y, z) == id) container = /*new*/ CompoundContainer("Large chest", (ChestTileEntity) level->getTileEntity(x - 1, y, z), container);
	//if (level->getTile(x + 1, y, z) == id) container = /*new*/ CompoundContainer("Large chest", container, (ChestTileEntity) level->getTileEntity(x + 1, y, z));
	//if (level->getTile(x, y, z - 1) == id) container = /*new*/ CompoundContainer("Large chest", (ChestTileEntity) level->getTileEntity(x, y, z - 1), container);
	//if (level->getTile(x, y, z + 1) == id) container = /*new*/ CompoundContainer("Large chest", container, (ChestTileEntity) level->getTileEntity(x, y, z + 1));

	if (level->isClientSide) {
		return true;
	}

	player->openContainer(chest);

	return true;
}

TileEntity* ChestTile::newTileEntity()
{
	return TileEntityFactory::createTileEntity(TileEntityType::Chest);
}

bool ChestTile::isFullChest( Level* level, int x, int y, int z )
{
	return false; //@fullchest
	/*
	if (level->getTile(x, y, z) != id) return false;
	if (level->getTile(x - 1, y, z) == id) return true;
	if (level->getTile(x + 1, y, z) == id) return true;
	if (level->getTile(x, y, z - 1) == id) return true;
	if (level->getTile(x, y, z + 1) == id) return true;
	return false;
	*/
}
