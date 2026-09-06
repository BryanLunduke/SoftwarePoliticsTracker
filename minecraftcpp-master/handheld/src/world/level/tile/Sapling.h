#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__Sapling_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__Sapling_H__

//package net.minecraft.world.level->tile;

#include "Bush.h"
#include "../Level.h"
#include "../levelgen/feature/SpruceFeature.h"
#include "../levelgen/feature/BirchFeature.h"
#include "../levelgen/feature/TreeFeature.h"

class Sapling: public Bush
{
    typedef Bush super;

    static const int TYPE_MASK = 3;
    static const int AGE_BIT = 8;
public:
    Sapling(int id, int tex)
    :   super(id, tex)
    {
        float ss = 0.4f;
        setShape(0.5f - ss, 0, 0.5f - ss, 0.5f + ss, ss * 2, 0.5f + ss);
    }

    void tick(Level* level, int x, int y, int z, Random* random) {
        if (level->isClientSide) return;

        super::tick(level, x, y, z, random);

        if (level->getRawBrightness(x, y + 1, z) >= Level::MAX_BRIGHTNESS - 6) {
            if (random->nextInt(7) == 0) {
                int data = level->getData(x, y, z);
                if ((data & AGE_BIT) == 0) {
					//@attn @fix : was setData, but it only works if using sendTileUpdate
                    level->setDataNoUpdate(x, y, z, data | AGE_BIT);
                } else {
                    growTree(level, x, y, z, random);
                }
            }
        }
    }

    /*@Override*/
    int getTexture(int face, int data) {
        data = data & LeafTile::LEAF_TYPE_MASK;
        if (data == LeafTile::EVERGREEN_LEAF) {
            return 15 + 16 * 3;
        } else if (data == LeafTile::BIRCH_LEAF) {
            return 15 + 16 * 4;
        //} else if (data == TYPE_JUNGLE) {
        //    return 14 + 16;
        } else {
            return super::getTexture(face, data);
        }
    }

    void growTree(Level* level, int x, int y, int z, Random* random) {
        int data = level->getData(x, y, z) & TYPE_MASK;

        Feature* f = NULL;

        int ox = 0, oz = 0;
        bool multiblock = false;

        if (data == LeafTile::EVERGREEN_LEAF) {
            f = new SpruceFeature(true);
        } else if (data == LeafTile::BIRCH_LEAF) {
            f = new BirchFeature(true);
//         } else if (data == TYPE_JUNGLE) {
// 
//             // check for mega tree
//             for (ox = 0; ox >= -1; ox--) {
//                 for (oz = 0; oz >= -1; oz--) {
//                     if (isSapling(level, x + ox, y, z + oz, TYPE_JUNGLE) &&
//                             isSapling(level, x + ox + 1, y, z + oz, TYPE_JUNGLE) &&
//                             isSapling(level, x + ox, y, z + oz + 1, TYPE_JUNGLE) &&
//                             isSapling(level, x + ox + 1, y, z + oz + 1, TYPE_JUNGLE)) {
//                         f = /*new*/ MegaTreeFeature(true, 10 + random.nextInt(20), TreeTile::JUNGLE_TRUNK, LeafTile::JUNGLE_LEAF);
//                         multiblock = true;
//                         break;
//                     }
//                 }
//                 if (f) {
//                     break;
//                 }
//             }
//             if (!f) {
//                 ox = oz = 0;
//                 f = new TreeFeature(true, 4 + random.nextInt(7), TreeTile::JUNGLE_TRUNK, LeafTile::JUNGLE_LEAF, false);
//             }
        } else {
            //if (random->nextInt(10) == 0) {
            //    f = new BasicTree(true);
            //} else
                f = new TreeFeature(true);
        }

        if (multiblock) {
            level->setTileNoUpdate(x + ox, y, z + oz, 0);
            level->setTileNoUpdate(x + ox + 1, y, z + oz, 0);
            level->setTileNoUpdate(x + ox, y, z + oz + 1, 0);
            level->setTileNoUpdate(x + ox + 1, y, z + oz + 1, 0);
        } else {
            level->setTileNoUpdate(x, y, z, 0);
        }

        if (!f->place(level, random, x + ox, y, z + oz)) {
            if (multiblock) {
                level->setTileAndDataNoUpdate(x + ox, y, z + oz, this->id, data);
                level->setTileAndDataNoUpdate(x + ox + 1, y, z + oz, this->id, data);
                level->setTileAndDataNoUpdate(x + ox, y, z + oz + 1, this->id, data);
                level->setTileAndDataNoUpdate(x + ox + 1, y, z + oz + 1, this->id, data);
            } else {
                level->setTileAndDataNoUpdate(x, y, z, this->id, data);
            }
        }

        if (f) delete f;
    }

    bool isSapling(Level* level, int x, int y, int z, int type) {
        return (level->getTile(x, y, z) == id) && ((level->getData(x, y, z) & TYPE_MASK) == type);
    }

protected:
    int getSpawnResourcesAuxValue(int data) {
        return data & TYPE_MASK;
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__Sapling_H__*/
