#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__TopSnowTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__TopSnowTile_H__

//package net.minecraft.world.level->tile;

#include "Tile.h"
#include "../Level.h"
#include "../material/Material.h"
#include "../../entity/item/ItemEntity.h"
#include "../../item/ItemInstance.h"
#include "../../../util/Random.h"

class TopSnowTile: public Tile
{
public:
    TopSnowTile(int id, int tex)
    :	Tile(id, tex, Material::topSnow)
	{
        setShape(0, 0, 0, 1, 1 / 8.0f, 1);
        setTicking(true);
    }

    AABB* getAABB(Level* level, int x, int y, int z) {
        return NULL;
    }

    bool blocksLight() {
        return false;
    }

    bool isSolidRender() {
        return false;
    }

    bool isCubeShaped() {
        return false;
    }

    bool mayPlace(Level* level, int x, int y, int z) {
        int t = level->getTile(x, y - 1, z);
        if (t == 0 || !Tile::tiles[t]->isSolidRender()) return false;
        return level->getMaterial(x, y - 1, z)->blocksMotion();
    }

    void neighborChanged(Level* level, int x, int y, int z, int type) {
        checkCanSurvive(level, x, y, z);
    }

	void playerDestroy(Level* level, Player* player, int x, int y, int z, int data) {
		if (level->isClientSide)
			return;

		int type = Item::snowBall->id;
		float s = 0.7f;
		float xo = level->random.nextFloat() * s + (1 - s) * 0.5f;
		float yo = level->random.nextFloat() * s + (1 - s) * 0.5f;
		float zo = level->random.nextFloat() * s + (1 - s) * 0.5f;
		ItemEntity* item = new ItemEntity(level, x + xo, y + yo, z + zo, ItemInstance(type, 1, 0));
		item->throwTime = 10;
		level->addEntity(item);
		level->setTile(x, y, z, 0);
	}

    int getResource(int data, Random* random) {
        return Item::snowBall->id;
    }

    int getResourceCount(Random* random) {
        return 0;
    }

    void tick(Level* level, int x, int y, int z, Random* random) {
        if (level->getBrightness(LightLayer::Block, x, y, z) > 11) {
            this->spawnResources(level, x, y, z, level->getData(x, y, z));
            level->setTile(x, y, z, 0);
        }
    }

    bool shouldRenderFace(LevelSource* level, int x, int y, int z, int face) {
        const Material* m = level->getMaterial(x, y, z);
        if (face == 1) return true;
        if (m == this->material) return false;
        return Tile::shouldRenderFace(level, x, y, z, face);
    }

private:
	bool checkCanSurvive(Level* level, int x, int y, int z) {
        if (!mayPlace(level, x, y, z)) {
            this->spawnResources(level, x, y, z, level->getData(x, y, z));
            level->setTile(x, y, z, 0);
            return false;
        }
        return true;
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__TopSnowTile_H__*/
