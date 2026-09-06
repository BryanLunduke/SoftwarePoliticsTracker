#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__LeafTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__LeafTile_H__

//package net.minecraft.world.level.tile;

#include "TransparentTile.h"
#include "../Level.h"
#include "../material/Material.h"
#include "../../item/Item.h"
#include "../../item/ItemInstance.h"
#include "../FoliageColor.h"

class Entity;

class LeafTile: public TransparentTile
{
	typedef TransparentTile super;

public:
	static const int LEAF_TYPE_MASK = 3;
	static const int REQUIRED_WOOD_RANGE = 4;

	//@attn @note:	PERSISTENT_LEAF_BIT and UPDATE_LEAF_BIT are reversed
	//				here, compared to desktop version
	static const int PERSISTENT_LEAF_BIT = 8; // player-placed
    static const int UPDATE_LEAF_BIT = 4;
    static const int NORMAL_LEAF = 0;
    static const int EVERGREEN_LEAF = 1;
    static const int BIRCH_LEAF = 2;

	LeafTile(int id, int tex)
	:	super(id, tex, Material::leaves, false),
		oTex(tex),
		checkBuffer(NULL)
	{
        setTicking(true);
    }

	~LeafTile() {
		if (checkBuffer != NULL)
			delete[] checkBuffer;
	}

	int getRenderLayer() {
        return isSolidRender()? Tile::RENDERLAYER_OPAQUE : Tile::RENDERLAYER_ALPHATEST;
    }

    int getColor(LevelSource* level, int x, int y, int z) {

        int data = (level->getData(x, y, z) & LEAF_TYPE_MASK);
        if (data == EVERGREEN_LEAF) {
            return FoliageColor::getEvergreenColor();
        }
        if (data == BIRCH_LEAF) {
            return FoliageColor::getBirchColor();
        }

        return FoliageColor::getDefaultColor();
    }

    void onRemove(Level* level, int x, int y, int z) {
        int r = 1;
        int r2 = r + 1;

        if (level->hasChunksAt(x - r2, y - r2, z - r2, x + r2, y + r2, z + r2)) {
            for (int xo = -r; xo <= r; xo++)
            for (int yo = -r; yo <= r; yo++)
            for (int zo = -r; zo <= r; zo++) {
                int t = level->getTile(x + xo, y + yo, z + zo);
                if (t == Tile::leaves->id) {
                    int currentData = level->getData(x + xo, y + yo, z + zo);
                    level->setDataNoUpdate(x + xo, y + yo, z + zo, currentData | UPDATE_LEAF_BIT);
                }
            }
        }
    }

    int* checkBuffer; //@todo Rewrite this?

    void tick(Level* level, int x, int y, int z, Random* random) {
        if (level->isClientSide) return;

        int currentData = level->getData(x, y, z);
        if ((currentData & UPDATE_LEAF_BIT) != 0 && (currentData & PERSISTENT_LEAF_BIT) == 0) {
            const int r = LeafTile::REQUIRED_WOOD_RANGE;
            int r2 = r + 1;

            const int W = 32;
            const int WW = W * W;
            const int WO = W / 2;
            if (!checkBuffer) {
                checkBuffer = new int[W * W * W];
            }

            if (level->hasChunksAt(x - r2, y - r2, z - r2, x + r2, y + r2, z + r2)) {
                for (int xo = -r; xo <= r; xo++)
                for (int yo = -r; yo <= r; yo++)
                for (int zo = -r; zo <= r; zo++) {
                    int t = level->getTile(x + xo, y + yo, z + zo);
                    if (t == Tile::treeTrunk->id) {
                        checkBuffer[(xo + WO) * WW + (yo + WO) * W + (zo + WO)] = 0;
                    } else if (t == Tile::leaves->id) {
                        checkBuffer[(xo + WO) * WW + (yo + WO) * W + (zo + WO)] = -2;
                    } else {
                        checkBuffer[(xo + WO) * WW + (yo + WO) * W + (zo + WO)] = -1;
                    }
                }
                for (int i = 1; i <= LeafTile::REQUIRED_WOOD_RANGE; i++) {
                    for (int xo = -r; xo <= r; xo++)
                    for (int yo = -r; yo <= r; yo++)
                    for (int zo = -r; zo <= r; zo++) {
                        if (checkBuffer[(xo + WO) * WW + (yo + WO) * W + (zo + WO)] == i - 1) {
                            if (checkBuffer[(xo + WO - 1) * WW + (yo + WO) * W + (zo + WO)] == -2) {
                                checkBuffer[(xo + WO - 1) * WW + (yo + WO) * W + (zo + WO)] = i;
                            }
                            if (checkBuffer[(xo + WO + 1) * WW + (yo + WO) * W + (zo + WO)] == -2) {
                                checkBuffer[(xo + WO + 1) * WW + (yo + WO) * W + (zo + WO)] = i;
                            }
                            if (checkBuffer[(xo + WO) * WW + (yo + WO - 1) * W + (zo + WO)] == -2) {
                                checkBuffer[(xo + WO) * WW + (yo + WO - 1) * W + (zo + WO)] = i;
                            }
                            if (checkBuffer[(xo + WO) * WW + (yo + WO + 1) * W + (zo + WO)] == -2) {
                                checkBuffer[(xo + WO) * WW + (yo + WO + 1) * W + (zo + WO)] = i;
                            }
                            if (checkBuffer[(xo + WO) * WW + (yo + WO) * W + (zo + WO - 1)] == -2) {
                                checkBuffer[(xo + WO) * WW + (yo + WO) * W + (zo + WO - 1)] = i;
                            }
                            if (checkBuffer[(xo + WO) * WW + (yo + WO) * W + (zo + WO + 1)] == -2) {
                                checkBuffer[(xo + WO) * WW + (yo + WO) * W + (zo + WO + 1)] = i;
                            }
                        }
                    }
                }
            }

            int mid = checkBuffer[(WO) * WW + (WO) * W + (WO)];
            if (mid >= 0) {
                level->setDataNoUpdate(x, y, z, currentData & ~UPDATE_LEAF_BIT);
            } else {
                die(level, x, y, z);
            }
        }
    }

	void playerDestroy(Level* level, Player* player, int x, int y, int z, int data) {
		if (!level->isClientSide) {
			ItemInstance* item = player->inventory->getSelected();
			if (item && item->id == ((Item*)Item::shears)->id) {
				// drop leaf block instead of sapling
				popResource(level, x, y, z, ItemInstance(Tile::leaves->id, 1, data & LEAF_TYPE_MASK));
				return;
			}
		}
		super::playerDestroy(level, player, x, y, z, data);
	}

    int getResourceCount(Random* random) {
        return random->nextInt(20) == 0 ? 1 : 0;
    }

    int getResource(int data, Random* random) {
        return Tile::sapling->id;
    }

	void spawnResources(Level* level, int x, int y, int z, int data, float odds) {
		if (!level->isClientSide) {
			int chance = 20;
			if (level->random.nextInt(chance) == 0) {
				int type = getResource(data, &level->random);
				popResource(level, x, y, z, ItemInstance(type, 1, getSpawnResourcesAuxValue(data)));
			}

			if ((data & LEAF_TYPE_MASK) == NORMAL_LEAF && level->random.nextInt(200) == 0) {
				popResource(level, x, y, z, ItemInstance(Item::apple, 1, 0));
			}
		}
	}

    bool isSolidRender() {
        return !allowSame;
    }

    int getTexture(int face, int data) {
        if ((data & LEAF_TYPE_MASK) == EVERGREEN_LEAF) {
			return (this == Tile::leaves)?	tex + 5 * 16
										:	tex -     16;
        }
        return tex;
    }

    void setFancy(bool fancyGraphics) {
        allowSame = fancyGraphics;
        tex = oTex + (fancyGraphics ? 0 : 1);
    }
protected:
	int getSpawnResourcesAuxValue(int data) {
		return data & LEAF_TYPE_MASK;
	}
private:
    void die(Level* level, int x, int y, int z) {
        spawnResources(level, x, y, z, level->getData(x, y, z) & LEAF_TYPE_MASK, 0);
        level->setTile(x, y, z, 0);
    }

	int oTex;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__LeafTile_H__*/
