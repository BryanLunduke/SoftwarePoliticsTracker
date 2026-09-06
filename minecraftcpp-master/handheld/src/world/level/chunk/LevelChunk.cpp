#include "LevelChunk.h"
#include "../LightLayer.h"
#include "../tile/Tile.h"
#include "../Level.h"
#include "../dimension/Dimension.h"
#include "../../phys/AABB.h"
#include "../../entity/Entity.h"
#include "../TilePos.h"
#include "../tile/entity/TileEntity.h"
#include "../tile/EntityTile.h"
#include "../LevelConstants.h"

/*static*/
bool LevelChunk::touchedSky = false;

LevelChunk::LevelChunk( Level* level, int x, int z )
:	level(level),
	x(x),
	z(z),
	xt(x * CHUNK_WIDTH),
	zt(z * CHUNK_DEPTH)
{
	init();
}

LevelChunk::LevelChunk( Level* level, unsigned char* blocks, int x, int z )
:	level(level),
	x(x),
	z(z),
	xt(x * CHUNK_WIDTH),
	zt(z * CHUNK_DEPTH),
	blocks(blocks),
	data(ChunkBlockCount),
	skyLight(ChunkBlockCount),
	blockLight(ChunkBlockCount),
	blocksLength(ChunkBlockCount)
{
	init();
}

LevelChunk::~LevelChunk()
{
	//delete blocks;
}

void LevelChunk::init()
{
	terrainPopulated = false;
	dontSave = false;
	unsaved = false;
	lastSaveHadEntities = false;
	createdFromSave = false;
	lastSaveTime = 0;
	memset(heightmap, 0, 256);
	memset(updateMap, 0, 256);
}

/*public*/
bool LevelChunk::setTileAndData(int x, int y, int z, int tile_, int data_) {

    int tile = tile_ & 0xff;

    int oldHeight = heightmap[z << 4 | x];

    int old = blocks[x << 11 | z << 7 | y];
    if (old == tile && data.get(x, y, z) == data_) return false;
    int xOffs = xt + x;
    int zOffs = zt + z;
    blocks[x << 11 | z << 7 | y] = (unsigned char) tile;
    if (old != 0) {
		if (!level->isClientSide) {
			Tile::tiles[old]->onRemove(level, xOffs, y, zOffs);
		} else if (old != tile && Tile::isEntityTile[old]) {
			level->removeTileEntity(xOffs, y, zOffs);
		}
    }
    data.set(x, y, z, data_);

    if (!level->dimension->hasCeiling) {
        if (Tile::lightBlock[tile] != 0) {
            if (y >= oldHeight) {
                recalcHeight(x, y + 1, z);
            }
        } else { 
            if (y == oldHeight - 1) {
                recalcHeight(x, y, z);
            }
        }
        level->updateLight(LightLayer::Sky, xOffs, y, zOffs, xOffs, y, zOffs);
    }

    level->updateLight(LightLayer::Block, xOffs, y, zOffs, xOffs, y, zOffs);
    lightGaps(x, z);

    if (!level->isClientSide && tile != 0) {
        Tile::tiles[tile]->onPlace(level, xOffs, y, zOffs);
    }

    this->unsaved = true;
	this->updateMap[x | (z << 4)] |= (1 << (y >> UpdateMapBitShift));
    return true;
}


/*public*/
bool LevelChunk::setTile(int x, int y, int z, int tile_) {

    int tile = tile_ & 0xff;
    int oldHeight = heightmap[z << 4 | x] & 0xff;

    int old = blocks[x << 11 | z << 7 | y] & 0xff;
    if (old == tile_) return false;
    int xOffs = xt + x;
    int zOffs = zt + z;
    blocks[x << 11 | z << 7 | y] = (unsigned char) (tile & 0xff);
    if (old != 0) {
        Tile::tiles[old]->onRemove(level, xOffs, y, zOffs);
    }
    data.set(x, y, z, 0);

    if (Tile::lightBlock[tile & 0xff] != 0) {
        if (y >= oldHeight) {
            recalcHeight(x, y + 1, z);
        }
    } else {
        if (y == oldHeight - 1) {
            recalcHeight(x, y, z);
        }
    }

    level->updateLight(LightLayer::Sky, xOffs, y, zOffs, xOffs, y, zOffs);
    level->updateLight(LightLayer::Block, xOffs, y, zOffs, xOffs, y, zOffs);
    lightGaps(x, z);

    if (tile_ != 0) {
        if (!level->isClientSide) Tile::tiles[tile_]->onPlace(level, xOffs, y, zOffs);
    }

    this->unsaved = true;
	this->updateMap[x | (z << 4)] |= (1 << (y >> UpdateMapBitShift));
    return true;
}

void LevelChunk::setTileRaw(int x, int y, int z, int tile) {
	blocks[x << 11 | z << 7 | y] = tile;
}

/*public*/
int LevelChunk::getData(int x, int y, int z) {
    return data.get(x, y, z);
}

/*public*/
void LevelChunk::recalcHeightmapOnly() {
    int min = Level::DEPTH - 1;
    for (int x = 0; x < 16; x++)
        for (int z = 0; z < 16; z++) {
            int y = Level::DEPTH - 1;
            int p = x << 11 | z << 7;
            while (y > 0 && Tile::lightBlock[blocks[p + y - 1] & 0xff] == 0)
                y--;
            heightmap[z << 4 | x] = (char) y;
            if (y < min) min = y;
        }

    this->minHeight = min;
    //this->unsaved = true;
}

/*public?*/
void LevelChunk::recalcHeightmap() {
    int min = Level::DEPTH - 1;
    for (int x = 0; x < 16; x++)
        for (int z = 0; z < 16; z++) {
            int y = Level::DEPTH - 1;
            int p = x << 11 | z << 7;
            while (y > 0 && Tile::lightBlock[blocks[p + y - 1] & 0xff] == 0)
                y--;
            heightmap[z << 4 | x] = (char) y;
            if (y < min) min = y;

            if (!level->dimension->hasCeiling) { 
                int br = Level::MAX_BRIGHTNESS;
                int yy = Level::DEPTH - 1;
                do {
                    br -= Tile::lightBlock[blocks[p + yy] & 0xff];
                    if (br > 0) {
                        skyLight.set(x, yy, z, br);
                    }
                    yy--;
                } while (yy > 0 && br > 0);

            }
        }

    this->minHeight = min;

    for (int x = 0; x < 16; x++)
        for (int z = 0; z < 16; z++) {
            lightGaps(x, z);
        }

    //this->unsaved = true;
}

/*private*/
void LevelChunk::recalcHeight(int x, int yStart, int z) {
    int yOld = heightmap[z << 4 | x] & 0xff;
    int y = yOld;
    if (yStart > yOld) y = yStart;

    int p = x << 11 | z << 7;
    while (y > 0 && Tile::lightBlock[blocks[p + y - 1] & 0xff] == 0)
        y--;
    if (y == yOld) return;

    const int xOffs = xt + x;
    const int zOffs = zt + z;
    level->lightColumnChanged(xOffs, zOffs, y, yOld);
    heightmap[z << 4 | x] = (char) y;

    if (y < minHeight) {
        minHeight = y;
    } else {
        int min = Level::DEPTH - 1;
        for (int _x = 0; _x < 16; _x++)
            for (int _z = 0; _z < 16; _z++) {
                if ((heightmap[_z << 4 | _x] & 0xff) < min) min = (heightmap[_z << 4 | _x] & 0xff);
            }
        this->minHeight = min;
    }

    if (y < yOld) {
        for (int yy = y; yy < yOld; yy++) {
            skyLight.set(x, yy, z, 15);
        }
    } else {
        level->updateLight(LightLayer::Sky, xOffs, yOld, zOffs, xOffs, y, zOffs);
        for (int yy = yOld; yy < y; yy++) {
            skyLight.set(x, yy, z, 0);
        }
    }

    int br = 15;
    int y0 = y;
    while (y > 0 && br > 0) {
        y--;
        int block = Tile::lightBlock[getTile(x, y, z)];
        if (block == 0) block = 1;
        br -= block;
        if (br < 0) br = 0;
        skyLight.set(x, y, z, br);
    }
    while (y > 0 && Tile::lightBlock[getTile(x, y - 1, z)] == 0)
        y--;
    if (y != y0) {
        level->updateLight(LightLayer::Sky, xOffs - 1, y, zOffs - 1, xOffs + 1, y0, zOffs + 1);
    }

    //this->unsaved = true;
}

/*public*/
void LevelChunk::skyBrightnessChanged() {
//	int x0 = xt;
//    int y0 = this->minHeight - 16;
//    int z0 = zt;
//    int x1 = xt + 16;
//    int y1 = Level::DEPTH - 1;
//    int z1 = zt + 16;

    //level->setTilesDirty(x0, y0, z0, x1, y1, z1);
}

/*public*/
bool LevelChunk::shouldSave(bool force) {
    if (dontSave) return false;
	/*
    if (force) {
        if (lastSaveHadEntities && level->getTime() != lastSaveTime) return true;
    } else {
        if (lastSaveHadEntities && level->getTime() >= lastSaveTime + 20 * 30) return true;
    }
	*/

    return unsaved;
}

/*public*/
void LevelChunk::setBlocks(unsigned char* newBlocks, int sub) { //@byte[]
	LOGI("LevelChunk::setBlocks\n");
	for (int i = 0; i < 128 * 16 * 4; i++) {
        blocks[sub * 128 * 16 * 4 + i] = newBlocks[i];
    }

    for (int x = sub * 4; x < sub * 4 + 4; x++) {
        for (int z = 0; z < 16; z++) {
            recalcHeight(x, 0, z);
        }
    }

    level->updateLight(LightLayer::Sky, xt + sub * 4, 0, zt, xt + sub * 4 + 4, 128, zt + 16);
    level->updateLight(LightLayer::Block, xt + sub * 4, 0, zt, xt + sub * 4 + 4, 128, zt + 16);

    //for (int x = sub * 4; x < sub * 4 + 4; x++) {
    //    for (int z = 0; z < 16; z++) {
    //        lightGaps(x, z);
    //    }
    //}
    level->setTilesDirty(xt + sub * 4, 0, zt, xt + sub * 4 + 4, 128, zt);
}

/*public*/
Random LevelChunk::getRandom(long l) {
    return /*new*/ Random((level->getSeed() + x * x * 4987142 + x * 5947611 + z * z * 4392871l + z * 389711) ^ l);
}

/*private*/
void LevelChunk::lightGap( int x, int z, int source )
{
	int height = level->getHeightmap(x, z);
	if (height > source) {
		level->updateLight(LightLayer::Sky, x, source, z, x, height, z);
	} else if (height < source) {
		level->updateLight(LightLayer::Sky, x, height, z, x, source, z);
	}
}

void LevelChunk::clearUpdateMap()
{
	memset(updateMap, 0x0, 256);
	unsaved = false;
}

void LevelChunk::deleteBlockData()
{
	delete [] blocks;
	blocks = NULL;
}

bool LevelChunk::isAt( int x, int z )
{
	return x == this->x && z == this->z;
}

int LevelChunk::getHeightmap( int x, int z )
{
	return heightmap[z << 4 | x] & 0xff;
}

void LevelChunk::recalcBlockLights()
{

}

void LevelChunk::lightGaps( int x, int z )
{
	int height = getHeightmap(x, z);

	int xOffs = xt + x;
	int zOffs = zt + z;
	lightGap(xOffs - 1, zOffs, height);
	lightGap(xOffs + 1, zOffs, height);
	lightGap(xOffs, zOffs - 1, height);
	lightGap(xOffs, zOffs + 1, height);
}

int LevelChunk::getTile( int x, int y, int z )
{
	return blocks[x << 11 | z << 7 | y] & 0xff;
}

void LevelChunk::setData( int x, int y, int z, int val )
{
	//this->unsaved = true;
	data.set(x, y, z, val);
}

int LevelChunk::getBrightness( const LightLayer& layer, int x, int y, int z )
{
	if (&layer == &LightLayer::Sky) return skyLight.get(x, y, z);
	else if (&layer == &LightLayer::Block) return blockLight.get(x, y, z);
	else return 0;
}

void LevelChunk::setBrightness( const LightLayer& layer, int x, int y, int z, int brightness )
{
	//this->unsaved = true;
	if (&layer == &LightLayer::Sky) skyLight.set(x, y, z, brightness);
	else if (&layer == &LightLayer::Block) blockLight.set(x, y, z, brightness);
	else return;
}

int LevelChunk::getRawBrightness( int x, int y, int z, int skyDampen )
{
	int light = skyLight.get(x, y, z);
	if (light > 0) LevelChunk::touchedSky = true;
	light -= skyDampen;
	int block = blockLight.get(x, y, z);
	if (block > light) light = block;

	return light;
}

void LevelChunk::addEntity( Entity* e )
{
	lastSaveHadEntities = true;

	int xc = Mth::floor(e->x / 16);
	int zc = Mth::floor(e->z / 16);
	if (xc != this->x || zc != this->z) {
		LOGE("ERR: WRONG LOCATION : %d, %d, %d, %d", xc, x, zc, z);
		//Thread.dumpStack();
	}
	int yc = Mth::floor(e->y / 16);
	if (yc < 0) yc = 0;
	if (yc >= EntityBlocksArraySize) yc = EntityBlocksArraySize - 1;
	e->inChunk = true;
	e->xChunk = x;
	e->yChunk = yc;
	e->zChunk = z;
	entityBlocks[yc].push_back(e);
}

void LevelChunk::removeEntity( Entity* e )
{
	removeEntity(e, e->yChunk);
}

void LevelChunk::removeEntity( Entity* e, int yc )
{
	if (yc < 0) yc = 0;
	if (yc >= EntityBlocksArraySize) yc = EntityBlocksArraySize - 1;

	EntityList::iterator newEnd = std::remove( entityBlocks[yc].begin(), entityBlocks[yc].end(), e);
	entityBlocks[yc].erase(newEnd, entityBlocks[yc].end());
}

bool LevelChunk::isSkyLit( int x, int y, int z )
{
	return y >= (heightmap[z << 4 | x] & 0xff);
}

void LevelChunk::load()
{
	loaded = true;
	/*        level->tileEntityList.addAll(tileEntities.values());
	for (int i = 0; i < entityBlocks.length; i++) {
	level->addEntities(entityBlocks[i]);
	}
	*/
}

void LevelChunk::unload()
{
	loaded = false;
	/*        level->tileEntityList.removeAll(tileEntities.values());
	for (int i = 0; i < entityBlocks.length; i++) {
	level->removeEntities(entityBlocks[i]);
	}
	*/
}

void LevelChunk::markUnsaved()
{
	this->unsaved = true;
}

void LevelChunk::getEntities( Entity* except, const AABB& bb, std::vector<Entity*>& es )
{
	int yc0 = Mth::floor((bb.y0 - 2) / 16);
	int yc1 = Mth::floor((bb.y1 + 2) / 16);
	if (yc0 < 0) yc0 = 0;
	if (yc1 >= EntityBlocksArraySize) yc1 = EntityBlocksArraySize - 1;
	for (int yc = yc0; yc <= yc1; yc++) {
		std::vector<Entity*>& entities = entityBlocks[yc];
		for (unsigned int i = 0; i < entities.size(); i++) {
			Entity* e = entities[i];
			if (e != except && e->bb.intersects(bb)){
				es.push_back(e);
			}
		}
	}
}

void LevelChunk::getEntitiesOfType( int entityType, const AABB& bb, EntityList& list )
{
	int yc0 = Mth::floor((bb.y0 - 2) / 16);
	int yc1 = Mth::floor((bb.y1 + 2) / 16);
	if (yc0 < 0) {
		yc0 = 0;
	} else if (yc0 >= EntityBlocksArraySize) {
		yc0 = EntityBlocksArraySize - 1;
	}
	if (yc1 >= EntityBlocksArraySize) {
		yc1 = EntityBlocksArraySize - 1;
	} else if (yc1 < 0) {
		yc1 = 0;
	}
	for (int yc = yc0; yc <= yc1; yc++) {
		std::vector<Entity*>& entities = entityBlocks[yc];
		for (unsigned int i = 0; i < entities.size(); i++) {
			Entity* e = entities[i];
			if (e->getEntityTypeId() == entityType)
				list.push_back(e);
			//if (baseClass.isAssignableFrom(e.getClass()) && e.bb.intersects(bb)) es.add(e);
		}
	}
}

void LevelChunk::getEntitiesOfClass( int type, const AABB& bb, EntityList& list )
{
	int yc0 = Mth::floor((bb.y0 - 2) / 16);
	int yc1 = Mth::floor((bb.y1 + 2) / 16);
	if (yc0 < 0) {
		yc0 = 0;
	} else if (yc0 >= EntityBlocksArraySize) {
		yc0 = EntityBlocksArraySize - 1;
	}
	if (yc1 >= EntityBlocksArraySize) {
		yc1 = EntityBlocksArraySize - 1;
	} else if (yc1 < 0) {
		yc1 = 0;
	}
	for (int yc = yc0; yc <= yc1; yc++) {
		std::vector<Entity*>& entities = entityBlocks[yc];
		for (unsigned int i = 0; i < entities.size(); i++) {
			Entity* e = entities[i];
			if (e->getCreatureBaseType() == type)
				list.push_back(e);
			//if (baseClass.isAssignableFrom(e.getClass()) && e.bb.intersects(bb)) es.add(e);
		}
	}
}

int LevelChunk::countEntities()
{
	int entityCount = 0;
	for (int yc = 0; yc < EntityBlocksArraySize; yc++) {
		entityCount += entityBlocks[yc].size();
	}
	return entityCount;
}

//@note: @todo @te Verify
TileEntity* LevelChunk::getTileEntity(int x, int y, int z) {
	TilePos pos(x, y, z);
	TEMap::iterator cit = tileEntities.find(pos);
	TileEntity* tileEntity = NULL;
	//bool ex = cit != tileEntities.end();
	//LOGI("Getting tile entity @ %d, %d, %d. Already existing? %d (%p)\n", x, y, z, ex, ex?cit->second:0);
	if (cit == tileEntities.end())
	{
		int t = getTile(x, y, z);
		if (t <= 0 || !Tile::isEntityTile[t])
			return NULL;

		if (tileEntity == NULL) {
			tileEntity = ((EntityTile*) Tile::tiles[t])->newTileEntity();
			level->setTileEntity(xt + x, y, zt + z, tileEntity);
		}
		// @attn @bug @fix: Don't go via level->setTileEntity since we
		//                  know this tile is loaded if we are here
		//cit = tileEntities.find(pos);
		//tileEntity = (cit != tileEntities.end())? cit->second : NULL;
	}
	else {
		tileEntity = cit->second;
	}
	if (tileEntity != NULL && tileEntity->isRemoved()) {
		tileEntities.erase(cit);
		return NULL;
	}

	return tileEntity;
}

bool LevelChunk::hasTileEntityAt( int x, int y, int z )
{
	return tileEntities.find(TilePos(x, y, z)) != tileEntities.end();
}
bool LevelChunk::hasTileEntityAt( TileEntity* te )
{
	return tileEntities.find(TilePos(te->x & 15, te->y, te->z & 15)) != tileEntities.end();
}

void LevelChunk::addTileEntity(TileEntity* te) {

	int xx = te->x - xt;
	int yy = te->y;
	int zz = te->z - zt;
	setTileEntity(xx, yy, zz, te);
	if (loaded) {
		level->tileEntities.push_back(te);
	}
}

void LevelChunk::setTileEntity(int x, int y, int z, TileEntity* tileEntity)
{
	tileEntity->setLevelAndPos(level, xt + x, y, zt + z);
	int t = getTile(x, y, z);
	if (t == 0 || !Tile::isEntityTile[t]) {
		LOGW("Attempted to place a tile entity where there was no entity tile! %d, %d, %d\n",
			tileEntity->x, tileEntity->y, tileEntity->z);
		return;
	}

	tileEntity->clearRemoved();
	tileEntities.insert(std::make_pair(TilePos(x, y, z), tileEntity));
}

void LevelChunk::removeTileEntity(int x, int y, int z) {
	if (loaded) {
		TilePos pos(x, y, z);
		TEMap::iterator cit = tileEntities.find(pos);
		if (cit != tileEntities.end()) {
			cit->second->setRemoved();

			if (!level->isClientSide) {
				for (unsigned int i = 0; i < level->players.size(); ++i) {
					level->players[i]->tileEntityDestroyed(cit->second->runningId);
				}
			}
			tileEntities.erase(cit);
		}
	}
}


int LevelChunk::getBlocksAndData( unsigned char* data, int x0, int y0, int z0, int x1, int y1, int z1, int p )
{
	int len = y1 - y0;
	for (int x = x0; x < x1; x++)
	for (int z = z0; z < z1; z++) {
		int slot = x << 11 | z << 7 | y0;
		memcpy(data + p, blocks + slot, len); //System.arraycopy(blocks, slot, data, p, len);
		p += len;
	}

	len = (y1 - y0) / 2;
	for (int x = x0; x < x1; x++)
	for (int z = z0; z < z1; z++) {
		int slot = (x << 11 | z << 7 | y0) >> 1;
		memcpy(data + p, this->data.data + slot, len); //System.arraycopy(this->data.data, slot, data, p, len);
		p += len;
	}

	//len = (y1 - y0) / 2;
	for (int x = x0; x < x1; x++)
	for (int z = z0; z < z1; z++) {
		int slot = (x << 11 | z << 7 | y0) >> 1;
		memcpy(data + p, blockLight.data + slot, len); //System.arraycopy(blockLight.data, slot, data, p, len);
		p += len;
	}

	//len = (y1 - y0) / 2;
	for (int x = x0; x < x1; x++)
	for (int z = z0; z < z1; z++) {
		int slot = (x << 11 | z << 7 | y0) >> 1;
		memcpy(data + p, skyLight.data + slot, len); //System.arraycopy(skyLight.data, slot, data, p, len);
		p += len;
	}

	return p;
}

int LevelChunk::setBlocksAndData( unsigned char* data, int x0, int y0, int z0, int x1, int y1, int z1, int p )
{
	int len = y1 - y0;
	for (int x = x0; x < x1; x++)
	for (int z = z0; z < z1; z++) {
		int slot = x << 11 | z << 7 | y0;
		memcpy(blocks + slot, data + p, len); //System.arraycopy(data, p, blocks, slot, len);
		p += len;
	}

	recalcHeightmapOnly();

	len = (y1 - y0) / 2;
	for (int x = x0; x < x1; x++)
	for (int z = z0; z < z1; z++) {
		int slot = (x << 11 | z << 7 | y0) >> 1;
		memcpy(this->data.data + slot, data + p, len); //System.arraycopy(data, p, this->data.data, slot, len);
		p += len;
	}

	//len = (y1 - y0) / 2;
	for (int x = x0; x < x1; x++)
	for (int z = z0; z < z1; z++) {
		int slot = (x << 11 | z << 7 | y0) >> 1;
		memcpy(blockLight.data + slot, data + p, len); //System.arraycopy(data, p, blockLight.data, slot, len);
		p += len;
	}

	//len = (y1 - y0) / 2;
	for (int x = x0; x < x1; x++)
	for (int z = z0; z < z1; z++) {
		int slot = (x << 11 | z << 7 | y0) >> 1;
		memcpy(skyLight.data + slot, data + p, len); //System.arraycopy(data, p, skyLight.data, slot, len);
		p += len;
	}

	return p;
}

bool LevelChunk::isEmpty()
{
	return false;
}

const LevelChunk::TEMap& LevelChunk::getTileEntityMap() const {
    return tileEntities;
}
