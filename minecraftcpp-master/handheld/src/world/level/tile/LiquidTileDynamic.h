#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__LiquidTileDynamic_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__LiquidTileDynamic_H__

//package net.minecraft.world.level.tile;

#include "LiquidTile.h"
#include "../Level.h"

class LiquidTileDynamic: public LiquidTile
{
	typedef LiquidTile super;
public:
	LiquidTileDynamic(int id, const Material* material)
	:	super(id, material),
		maxCount(0)
	{}

    void tick(Level* level, int x, int y, int z, Random* random) {
        //printf("liquid-tick\n");
		int depth = getDepth(level, x, y, z);

        int dropOff = 1;
        if (material == Material::lava && !level->dimension->ultraWarm) dropOff = 2;

        bool becomeStatic = true;
        if (depth > 0) {
            int highest = -100;
            maxCount = 0;
            highest = getHighest(level, x - 1, y, z, highest);
            highest = getHighest(level, x + 1, y, z, highest);
            highest = getHighest(level, x, y, z - 1, highest);
            highest = getHighest(level, x, y, z + 1, highest);

            int newDepth = highest + dropOff;
            if (newDepth >= 8 || highest < 0) {
                newDepth = -1;
            }
            if (getDepth(level, x, y + 1, z) >= 0) {
                int above = getDepth(level, x, y + 1, z);
                if (above >= 8) newDepth = above;
                else newDepth = above + 8;
            }
            if (maxCount >= 2 && material == Material::water) {
                // Only spread spring if it's on top of an existing spring, or on top of solid ground.
                if (level->isSolidBlockingTile(x, y - 1, z)) {
                    newDepth = 0;
                } else if (level->getMaterial(x, y - 1, z) == material && level->getData(x, y, z) == 0) {
                    newDepth = 0;
                }
            }
            if (material == Material::lava) {
                if (depth < 8 && newDepth < 8) {
                    if (newDepth > depth) {
                        if (random->nextInt(4) != 0) {
                            newDepth = depth;
                            becomeStatic = false;
                        }
                    }
                }
            }
            if (newDepth != depth) {
                depth = newDepth;
                if (depth < 0) {
                    level->setTile(x, y, z, 0);
                } else {
                    level->setData(x, y, z, depth);
                    level->addToTickNextTick(x, y, z, id, getTickDelay());
                    level->updateNeighborsAt(x, y, z, id);
                }
            } else {
                if (becomeStatic) setStatic(level, x, y, z);
            }
        } else {
            setStatic(level, x, y, z);
        }
        if (canSpreadTo(level, x, y - 1, z)) {
            if (depth >= 8) level->setTileAndData(x, y - 1, z, id, depth);
            else level->setTileAndData(x, y - 1, z, id, depth + 8);
        } else if (depth >= 0 && (depth == 0 || isWaterBlocking(level, x, y - 1, z))) {
            bool* spreads = getSpread(level, x, y, z);
            int neighbor = depth + dropOff;
            if (depth >= 8) {
                neighbor = 1;
            }
            if (neighbor >= 8) return;
            if (spreads[0]) trySpreadTo(level, x - 1, y, z, neighbor);
            if (spreads[1]) trySpreadTo(level, x + 1, y, z, neighbor);
            if (spreads[2]) trySpreadTo(level, x, y, z - 1, neighbor);
            if (spreads[3]) trySpreadTo(level, x, y, z + 1, neighbor);
        }
    }

    /*protected*/
	int getHighest(Level* level, int x, int y, int z, int current) {
        int d = getDepth(level, x, y, z);
        if (d < 0) return current;
        if (d == 0) maxCount++;
        if (d >= 8) {
            d = 0;
        }
        return current < 0 || d < current ? d : current;
    }

    void onPlace(Level* level, int x, int y, int z) {
        super::onPlace(level, x, y, z);
        if (level->getTile(x, y, z) == id) {
            level->addToTickNextTick(x, y, z, id, getTickDelay());
        }
    }

private:
    void setStatic(Level* level, int x, int y, int z) {
        int d = level->getData(x, y, z);
        level->setTileAndDataNoUpdate(x, y, z, id + 1, d);
        level->setTilesDirty(x, y, z, x, y, z);
        level->sendTileUpdated(x, y, z);
    }

	bool canSpreadTo(Level* level, int x, int y, int z) {
        const Material* target = level->getMaterial(x, y, z);
        if (target == material) return false;
        if (target == Material::lava) return false;
        return !isWaterBlocking(level, x, y, z);
    }

    bool isWaterBlocking(Level* level, int x, int y, int z) {
        int t = level->getTile(x, y, z);
		if (t == 0) return false;
        if (t == Tile::door_wood->id /*|| t == Tile::door_iron->id */ || t == Tile::sign->id || t == Tile::ladder->id || t == Tile::reeds->id) {
            return true;
        }
        const Material* m = Tile::tiles[t]->material;
		if (m->blocksMotion()) return true;
        if (m->isSolid()) return true;
        return false;
    }

    bool* getSpread(Level* level, int x, int y, int z) {
        for (int d = 0; d < 4; d++) {
            dist[d] = 1000;
            int xx = x;
            int yy = y;
            int zz = z;

            if (d == 0) xx--;
            if (d == 1) xx++;
            if (d == 2) zz--;
            if (d == 3) zz++;
            if (isWaterBlocking(level, xx, yy, zz)) {
                continue;
            } else if (level->getMaterial(xx, yy, zz) == material && level->getData(xx, yy, zz) == 0) {
                continue;
            } else {
                if (!isWaterBlocking(level, xx, yy - 1, zz)) {
                    dist[d] = 0;
                } else {
                    dist[d] = getSlopeDistance(level, xx, yy, zz, 1, d);
                }
            }
        }

        int lowest = dist[0];
        for (int d = 1; d < 4; d++) {
            if (dist[d] < lowest) lowest = dist[d];
        }

        for (int d = 0; d < 4; d++) {
            result[d] = dist[d] == lowest;
        }
        return result;
    }

    void trySpreadTo(Level* level, int x, int y, int z, int neighbor) {
		if (canSpreadTo(level, x, y, z)) {
            int old = level->getTile(x, y, z);
            if (old > 0) {
                if (material == Material::lava) {
                    fizz(level, x, y, z);
                } else {
                    Tile::tiles[old]->spawnResources(level, x, y, z, level->getData(x, y, z));
                }
            }
            level->setTileAndData(x, y, z, id, neighbor);
        }
    }

    int getSlopeDistance(Level* level, int x, int y, int z, int pass, int from) {
        int lowest = 1000;
        for (int d = 0; d < 4; d++) {
            if (d == 0 && from == 1) continue;
            if (d == 1 && from == 0) continue;
            if (d == 2 && from == 3) continue;
            if (d == 3 && from == 2) continue;

            int xx = x;
            int yy = y;
            int zz = z;

            if (d == 0) xx--;
            if (d == 1) xx++;
            if (d == 2) zz--;
            if (d == 3) zz++;

            if (isWaterBlocking(level, xx, yy, zz)) {
                continue;
            } else if (level->getMaterial(xx, yy, zz) == material && level->getData(xx, yy, zz) == 0) {
                continue;
            } else {
                if (!isWaterBlocking(level, xx, yy - 1, zz)) {
                    return pass;
                } else {
                    if (pass < 4) {
                        int v = getSlopeDistance(level, xx, yy, zz, pass + 1, d);
                        if (v < lowest) lowest = v;
                    }
                }
            }
        }
        return lowest;
    }

    int maxCount;

	static const int NumDirections = 4;

	bool result[NumDirections];
    int dist[NumDirections];
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__LiquidTileDynamic_H__*/
