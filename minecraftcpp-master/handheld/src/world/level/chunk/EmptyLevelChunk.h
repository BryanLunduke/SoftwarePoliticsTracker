#ifndef NET_MINECRAFT_WORLD_LEVEL_CHUNK__EmptyLevelChunk_H__
#define NET_MINECRAFT_WORLD_LEVEL_CHUNK__EmptyLevelChunk_H__

//package net.minecraft.world.level.chunk;

/* import java.util.* */

//#include "world/entity/Entity.h"
/* import net.minecraft.world.level.* */
//#include "world/level/tile/entity/TileEntity.h"
//#include "world/phys/AABB.h"

#include "LevelChunk.h"
#include "../tile/Tile.h"
#include "../Level.h"

class EmptyLevelChunk: public LevelChunk
{
public:
    EmptyLevelChunk(Level* level, int x, int z)
	:	LevelChunk(level, x, z)
	{
        dontSave = true;
    }

    EmptyLevelChunk(Level* level, unsigned char* blocks, int x, int z)
	:	LevelChunk(level, blocks, x, z)
	{
        dontSave = true;
    }

    bool isAt(int x, int z) {
        return x == this->x && z == this->z;
    }

    int getHeightmap(int x, int z) {
        return 0;
    }

    void recalcBlockLights() {
    }

    void recalcHeightmapOnly() {
        return;
    }

    void recalcHeightmap() {
        return;
    }

    void lightLava() {
        return;
    }

    int getTile(int x, int y, int z) {
		//if (y <= 1)
		//{
		//	return Tile::unbreakable->id;
		//}
		//else if (y < 64)
		//{
		//	return Tile::water->id;
		//}
		//return 0;
        return Tile::invisible_bedrock->id;
    }

    bool setTileAndData(int x, int y, int z, int _tile, int _data) {
        return true;
    }

    bool setTile(int x, int y, int z, int _tile) {
        return true;
    }

    int getData(int x, int y, int z) {
        return 0;
    }

    void setData(int x, int y, int z, int val) {
        return;
    }

    int getBrightness(const LightLayer& layer, int x, int y, int z) {
        return 7;
    }

    void setBrightness(const LightLayer& layer, int x, int y, int z, int brightness) {
        return;
    }

    int getRawBrightness(int x, int y, int z, int skyDampen) {
        return 7;
    }

    void addEntity(Entity* e) {
        return;
    }

    void removeEntity(Entity* e) {
        return;
    }

    void removeEntity(Entity* e, int yc) {
        return;
    }

    bool isSkyLit(int x, int y, int z) {
        return false;
    }

    void skyBrightnessChanged() {
        return;
    }
/*
    TileEntity getTileEntity(int x, int y, int z) {
        return NULL;
    }

    void addTileEntity(TileEntity te) {
        return;
    }

    void setTileEntity(int x, int y, int z, TileEntity tileEntity) {
        return;
    }

    void removeTileEntity(int x, int y, int z) {
        return;
    }
	*/

    void load() {
        return;
    }

    void unload() {
        return;
    }

    void markUnsaved() {
        return;
    }

	void getEntities(Entity* except, const AABB& bb, std::vector<Entity*>& es) {
        return;
    }
	/*
    void getEntitiesOfClass(Class<? extends Entity> baseClass, AABB bb, List<Entity> es) {
        return;
    }

    int countEntities() {
        return 0;
    }

    bool shouldSave(bool force) {
        return false;
    }
*/

    void setBlocks(unsigned char* newBlocks, int sub) {
        return;
    }

    int getBlocksAndData(unsigned char* data, int x0, int y0, int z0, int x1, int y1, int z1, int p) {
        int xs = x1 - x0;
        int ys = y1 - y0;
        int zs = z1 - z0;

        int s = xs * ys * zs;
        int len = s + s / 2 * 3;

		memset(data + p, Tile::invisible_bedrock->id, len); //Arrays.fill(data, p, p + len, (char) 0);
        return len;
    }

    int setBlocksAndData(unsigned char* data, int x0, int y0, int z0, int x1, int y1, int z1, int p) {
        int xs = x1 - x0;
        int ys = y1 - y0;
        int zs = z1 - z0;

        int s = xs * ys * zs;
        return s + s / 2 * 3;
    }

    Random getRandom(long l) {
        return /*new*/ Random((level->getSeed() + x * x * 4987142 + x * 5947611 + z * z * 4392871l + z * 389711) ^ l);
    }

    bool isEmpty() {
        return true;
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_CHUNK__EmptyLevelChunk_H__*/
